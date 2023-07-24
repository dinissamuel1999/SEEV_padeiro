#include "Arduino.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_task_wdt.h"
#include <Servo.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
//********************************************CONSTANTES****************************************************************************************
#define PEDAL_TRAVAO_ATIVO 1
#define PEDAL_TRAVAO_INATIVO 0

#define ADC_RESOLUTION 8

struct tipo_dados_queue_posicao_servos {
	int posicao_servo_1_altura;
	int posicao_servo_2_inclinacao;
};

//********************************************PINOS****************************************************************************************
#define PINO_BTN_Travagem 21
#define PINO_Servo_1_altura 12
#define PINO_Servo_2_inclinacao 27
#define PINO_ADC_velocidade_pretendida 14
#define PINO_PWM_MOTOR 16
#define PINO_INT_VELOCIDADE_MOTOR 22
#define PINO_LUZ_STOP 25

#define TFT_MISO  19
#define TFT_SCK   18
#define TFT_MOSI  23
#define TFT_DC    0
#define TFT_RESET 2
#define TFT_CS    15

//********************************************Instancia****************************************************************************************
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK,
TFT_RESET, TFT_MISO);
//********************************************CABEÇALHOS_INTERRUPCOES****************************************************************************************
static void IRAM_ATTR vInterruptHandler_Btn_Travagem(void);
static void IRAM_ATTR vInterruptHandler_Encoder_Velocidade(void);

//********************************************MUTEXES****************************************************************************************
SemaphoreHandle_t xMutex_Acesso_UART;

//********************************************SEMEFOROS****************************************************************************************
SemaphoreHandle_t xBinarySemaphore_Btn_Travagem;
SemaphoreHandle_t xCountingSemaphore_Encoder_Velocidade;

//********************************************QUEUES****************************************************************************************
QueueHandle_t xQueue_BTN_Travagem = NULL;
QueueHandle_t xQueue_Valor_ADC_PWM_Motor = NULL;
QueueHandle_t xQueue_Posicao_Servos = NULL;
QueueHandle_t xQueue_Velocidade_Motor = NULL;

//********************************************DESCRICAO TAREFAS****************************************************************************************
const char *pcTextFor_Brain = "BRAIN task is running ";
const char *pcTextFor_Lcd = "LCD task is running ";
const char *pcTextFor_Encoder_Velocidade = "ENCODER_VELOCIDADE task is running ";
const char *pcTextFor_Controla_Motor_DC = "CONTROLA_MOTOR_DC task is running ";
const char *pcTextFor_Potenciometro_veloc_motor =
		"POTENCIOMETRO_MOTOR task is running ";
const char *pcTextFor_Controla_Servos = "CONTROLA_SERVOS task is running ";
const char *pcTextFor_BTN_Travagem = "BTN_TRAVAGEM task is running ";
const char *pcTextFor_Controlo_Luz_STOP = "CONTROLO_LUZ_STOP task is running ";

//********************************************TAREFAS****************************************************************************************
void vTask_Brain(void *pvParameters);
void vTask_LCD(void *pvParameters);
void vTask_Encoder_Velocidade(void *pvParameters);
void vTask_Controla_Motor_DC(void *pvParameters);
void vTask_Potenciometro_Motor_DC(void *pvParameters);
void vTask_Controla_Servos(void *pvParameters);
void vTask_BTN_Travagem(void *pvParameters);
void vTask_ControloLuz_Stop(void *pvParameters);

//********************************************SETUP****************************************************************************************
void setup() {
	vTaskPrioritySet(NULL, configMAX_PRIORITIES - 1);
	Serial.begin(115200);
	bool var_aux_verificacao_criacao_objetos = false;

	pinMode(PINO_BTN_Travagem, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(PINO_BTN_Travagem),
			&vInterruptHandler_Btn_Travagem, CHANGE);

	pinMode(PINO_INT_VELOCIDADE_MOTOR, INPUT);
	attachInterrupt(digitalPinToInterrupt(PINO_INT_VELOCIDADE_MOTOR),
			&vInterruptHandler_Encoder_Velocidade, RISING);

	xMutex_Acesso_UART = xSemaphoreCreateMutex();
	if (xMutex_Acesso_UART == NULL) {
		var_aux_verificacao_criacao_objetos = false;
	}
	//***************************************Semaforos***************************************

	vSemaphoreCreateBinary(xBinarySemaphore_Btn_Travagem);
	if (xBinarySemaphore_Btn_Travagem == NULL) {
		var_aux_verificacao_criacao_objetos = false;
	}

	xCountingSemaphore_Encoder_Velocidade = xSemaphoreCreateCounting(1000000,
			0);
	if (xCountingSemaphore_Encoder_Velocidade == NULL) {
		var_aux_verificacao_criacao_objetos = false;
	}
//***************************************Queues***************************************
	xQueue_BTN_Travagem = xQueueCreate(1, sizeof(char));
	if (xQueue_BTN_Travagem == NULL) {
		var_aux_verificacao_criacao_objetos = false;
	}

	xQueue_Valor_ADC_PWM_Motor = xQueueCreate(1, sizeof(uint8_t));
	if (xQueue_Valor_ADC_PWM_Motor == NULL) {
		var_aux_verificacao_criacao_objetos = false;
	}

	xQueue_Posicao_Servos = xQueueCreate(1,
			sizeof(tipo_dados_queue_posicao_servos));
	if (xQueue_Posicao_Servos == NULL) {
		var_aux_verificacao_criacao_objetos = false;
	}

	xQueue_Velocidade_Motor = xQueueCreate(1, sizeof(float));
	if (xQueue_Velocidade_Motor == NULL) {
		var_aux_verificacao_criacao_objetos = false;
	}

//***************************************Tarefas***************************************
	if (var_aux_verificacao_criacao_objetos == false) {
		xTaskCreatePinnedToCore(vTask_Brain, "BRAIN", 1024,
				(void*) pcTextFor_Brain, 8, NULL, 1);
		xTaskCreatePinnedToCore(vTask_LCD, "LCD", 1024, (void*) pcTextFor_Lcd,
				5,
				NULL, 1);
		xTaskCreatePinnedToCore(vTask_Encoder_Velocidade, "ENCODER_VELOCIDADE",
				1024, (void*) pcTextFor_Encoder_Velocidade, 5, NULL, 1);
		xTaskCreatePinnedToCore(vTask_Controla_Motor_DC, "CONTROLA_MOTOR_DC",
				1024, (void*) pcTextFor_Controla_Motor_DC, 5, NULL, 1);
		xTaskCreatePinnedToCore(vTask_Potenciometro_Motor_DC,
				"POTENCIOMETRO_MOTOR_DC", 1024,
				(void*) pcTextFor_Potenciometro_veloc_motor, 5, NULL, 1);
		xTaskCreatePinnedToCore(vTask_Controla_Servos, "CONTROLA_SERVOS", 1024,
				(void*) pcTextFor_Controla_Servos, 8, NULL, 1);
		xTaskCreatePinnedToCore(vTask_BTN_Travagem, "BTN_TRAVAGEM", 1024,
				(void*) pcTextFor_BTN_Travagem, 5, NULL, 1);
		xTaskCreatePinnedToCore(vTask_ControloLuz_Stop, "LUZ_STOP", 1024,
				(void*) pcTextFor_Controlo_Luz_STOP, 5, NULL, 1);

	}interrupts();
}

void loop() {
	vTaskDelete(NULL);
}

void vTask_ControloLuz_Stop(void *pvParameters) {
	char *pcTaskName;
	TickType_t xLastWakeTime;
	pcTaskName = (char*) pvParameters;
	xLastWakeTime = xTaskGetTickCount();

	pinMode(PINO_LUZ_STOP, OUTPUT);
	bool estado_pedal_travao = PEDAL_TRAVAO_INATIVO;

	for (;;) {
		xSemaphoreTake(xMutex_Acesso_UART, portMAX_DELAY);
		{
			Serial.println(pcTaskName);
		}
		xSemaphoreGive(xMutex_Acesso_UART);

		xQueuePeek(xQueue_BTN_Travagem, &estado_pedal_travao, 0);

		if (estado_pedal_travao == PEDAL_TRAVAO_ATIVO) {
			digitalWrite(PINO_LUZ_STOP, HIGH);
		} else if (estado_pedal_travao == PEDAL_TRAVAO_INATIVO) {
			digitalWrite(PINO_LUZ_STOP, LOW);
		}

		vTaskDelayUntil(&xLastWakeTime, (100 / portTICK_PERIOD_MS));
	}
}

void vTask_Brain(void *pvParameters) {
	char *pcTaskName;
	TickType_t xLastWakeTime;
	pcTaskName = (char*) pvParameters;
	xLastWakeTime = xTaskGetTickCount();

	float velocidade_rpm_motor = 0;
	bool estado_pedal_travao = PEDAL_TRAVAO_INATIVO;

	tipo_dados_queue_posicao_servos posicoes_servos = { 0, 0 };

	for (;;) {
		xSemaphoreTake(xMutex_Acesso_UART, portMAX_DELAY);
		{
			Serial.println(pcTaskName);
		}
		xSemaphoreGive(xMutex_Acesso_UART);

		xQueuePeek(xQueue_Velocidade_Motor, &velocidade_rpm_motor, 0);
		xQueuePeek(xQueue_BTN_Travagem, &estado_pedal_travao, 0);

		if (velocidade_rpm_motor > 7000) {
			velocidade_rpm_motor = 7000;

		} else if (velocidade_rpm_motor < 1000) {
			velocidade_rpm_motor = 1000;
		}

		posicoes_servos.posicao_servo_1_altura = ((velocidade_rpm_motor - 1000)
				* 180) / (7000 - 1000);

		if (velocidade_rpm_motor < 4000) {
			posicoes_servos.posicao_servo_2_inclinacao = 90;
		} else if (velocidade_rpm_motor
				> 4000&& estado_pedal_travao == PEDAL_TRAVAO_INATIVO) {
			posicoes_servos.posicao_servo_2_inclinacao = 20;

		} else if (velocidade_rpm_motor
				> 4000&& estado_pedal_travao == PEDAL_TRAVAO_ATIVO) {
			posicoes_servos.posicao_servo_2_inclinacao = 160;
		} else {
			posicoes_servos.posicao_servo_2_inclinacao = 90;
		}

		xQueueOverwrite(xQueue_Posicao_Servos, &posicoes_servos);

		vTaskDelayUntil(&xLastWakeTime, (100 / portTICK_PERIOD_MS));
	}
}

void vTask_LCD(void *pvParameters) {
	char *pcTaskName;
	TickType_t xLastWakeTime;
	pcTaskName = (char*) pvParameters;
	xLastWakeTime = xTaskGetTickCount();

	tft.begin();
	tft.fillScreen(ILI9341_BLACK);
	tft.setRotation(1);
	tft.setTextSize(3);
	tft.setTextWrap(true);

	tft.setCursor(20, 10);
	tft.setTextColor(ILI9341_WHITE);
	tft.setTextSize(2);
	tft.println("  Projeto SEEV");

	float velocidade_rpm_motor_ant = 0;
	float velocidade_rpm_motor = 0;
	char array[8];

	for (;;) {
		xSemaphoreTake(xMutex_Acesso_UART, portMAX_DELAY);
		{
			Serial.println(pcTaskName);
		}
		xSemaphoreGive(xMutex_Acesso_UART);

		xQueuePeek(xQueue_Velocidade_Motor, &velocidade_rpm_motor, 0);

		if (velocidade_rpm_motor_ant != velocidade_rpm_motor) {

			tft.setTextSize(2);
			tft.setCursor(15, 127);
			tft.setTextColor(ILI9341_WHITE, ILI9341_DARKGREY);
			tft.print("                        ");
			tft.setCursor(15, 127);
			dtostrf(velocidade_rpm_motor, 6, 2, array);
			tft.print(array);
			velocidade_rpm_motor_ant = velocidade_rpm_motor;
		}

		vTaskDelayUntil(&xLastWakeTime, (100 / portTICK_PERIOD_MS));
	}
}

void vTask_Encoder_Velocidade(void *pvParameters) {
	char *pcTaskName;
	TickType_t xLastWakeTime;
	pcTaskName = (char*) pvParameters;
	xLastWakeTime = xTaskGetTickCount();

	int numero_pulsos_motor = 0;
	float velocidade_rpm = 0;

	for (;;) {
		xSemaphoreTake(xMutex_Acesso_UART, portMAX_DELAY);
		{
			Serial.println(pcTaskName);
		}
		xSemaphoreGive(xMutex_Acesso_UART);

		numero_pulsos_motor = uxSemaphoreGetCount(
				xCountingSemaphore_Encoder_Velocidade);
		velocidade_rpm = (((float) numero_pulsos_motor / 20) / 200) * 1000 * 60;

		xQueueOverwrite(xQueue_Velocidade_Motor, &velocidade_rpm);
		xQueueReset(xCountingSemaphore_Encoder_Velocidade);
		vTaskDelayUntil(&xLastWakeTime, (200 / portTICK_PERIOD_MS));
	}
}

void vTask_Controla_Motor_DC(void *pvParameters) {
	char *pcTaskName;
	TickType_t xLastWakeTime;
	pcTaskName = (char*) pvParameters;
	xLastWakeTime = xTaskGetTickCount();

	//setup_task
	int freq = 5000;
	int ledChannel = 5;
	int resolution = 8;
	ledcSetup(ledChannel, freq, resolution);
	ledcAttachPin(PINO_PWM_MOTOR, ledChannel);

	pinMode(PINO_PWM_MOTOR, OUTPUT);

	uint8_t valor_pwm_motor = 0;

	for (;;) {

		xQueuePeek(xQueue_Valor_ADC_PWM_Motor, &valor_pwm_motor, 0);

		xSemaphoreTake(xMutex_Acesso_UART, portMAX_DELAY);
		{
			Serial.println(pcTaskName);
			Serial.print("Valor de pwm pretendido: ");
			Serial.println(valor_pwm_motor);
		}
		xSemaphoreGive(xMutex_Acesso_UART);

		ledcWrite(ledChannel, valor_pwm_motor);

		vTaskDelayUntil(&xLastWakeTime, (100 / portTICK_PERIOD_MS));

	}
}

void vTask_Potenciometro_Motor_DC(void *pvParameters) {
	char *pcTaskName;
	TickType_t xLastWakeTime;
	pcTaskName = (char*) pvParameters;
	xLastWakeTime = xTaskGetTickCount();

	analogReadResolution(ADC_RESOLUTION);
	uint8_t adc_valor_lido = 0;

	for (;;) {

		adc_valor_lido = analogRead(PINO_ADC_velocidade_pretendida);
		xQueueOverwrite(xQueue_Valor_ADC_PWM_Motor, &adc_valor_lido);

		xSemaphoreTake(xMutex_Acesso_UART, portMAX_DELAY);
		{
			Serial.println(pcTaskName);
			Serial.print("Valor de adc lido: ");
			Serial.println(adc_valor_lido);
		}
		xSemaphoreGive(xMutex_Acesso_UART);

		vTaskDelayUntil(&xLastWakeTime, (100 / portTICK_PERIOD_MS));
	}
}

void vTask_Controla_Servos(void *pvParameters) {
	char *pcTaskName;
	TickType_t xLastWakeTime;
	pcTaskName = (char*) pvParameters;
	xLastWakeTime = xTaskGetTickCount();

	Servo Servo_1_altura;
	Servo Servo_2_inclinacao;

	int posicao_servo_1_altura = 0;
	int posicao_servo_2_inclinacao = 0;

	tipo_dados_queue_posicao_servos posicao_servos = { 0, 90 };

	Servo_1_altura.attach(PINO_Servo_1_altura);
	Servo_2_inclinacao.attach(PINO_Servo_2_inclinacao);

	for (;;) {
		xSemaphoreTake(xMutex_Acesso_UART, portMAX_DELAY);
		{
			Serial.println(pcTaskName);
		}
		xSemaphoreGive(xMutex_Acesso_UART);

		xQueuePeek(xQueue_Posicao_Servos, &posicao_servos, 0);

		Servo_1_altura.write(posicao_servos.posicao_servo_1_altura);
		Servo_2_inclinacao.write(posicao_servos.posicao_servo_2_inclinacao);
		vTaskDelayUntil(&xLastWakeTime, (15 / portTICK_PERIOD_MS));

	}
}

void vTask_BTN_Travagem(void *pvParameters) {
	char *pcTaskName;
	TickType_t xLastWakeTime;
	pcTaskName = (char*) pvParameters;
	xLastWakeTime = xTaskGetTickCount();

	bool estado_pedal_travao = PEDAL_TRAVAO_INATIVO;
	xSemaphoreTake(xBinarySemaphore_Btn_Travagem, 0);

	for (;;) {

		xSemaphoreTake(xBinarySemaphore_Btn_Travagem, portMAX_DELAY);

		if (estado_pedal_travao == PEDAL_TRAVAO_ATIVO) {
			estado_pedal_travao = PEDAL_TRAVAO_INATIVO;
		} else if (estado_pedal_travao == PEDAL_TRAVAO_INATIVO) {
			estado_pedal_travao = PEDAL_TRAVAO_ATIVO;
		}

		xQueueOverwrite(xQueue_BTN_Travagem, &estado_pedal_travao);
		xSemaphoreTake(xMutex_Acesso_UART, portMAX_DELAY);
		{
			Serial.println(pcTaskName);
		}
		xSemaphoreGive(xMutex_Acesso_UART);
		vTaskDelayUntil(&xLastWakeTime, (100 / portTICK_PERIOD_MS));
	}
}

static void IRAM_ATTR vInterruptHandler_Btn_Travagem(void) {
	static signed portBASE_TYPE xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(xBinarySemaphore_Btn_Travagem,
			(signed portBASE_TYPE*)&xHigherPriorityTaskWoken);

	if (xHigherPriorityTaskWoken == pdTRUE) {
		portYIELD_FROM_ISR();
	}
}

static void IRAM_ATTR vInterruptHandler_Encoder_Velocidade(void) {
	static portBASE_TYPE xHigherPriorityTaskWoken;

	xHigherPriorityTaskWoken = pdFALSE;

	xSemaphoreGiveFromISR(xCountingSemaphore_Encoder_Velocidade,
			(BaseType_t* )&xHigherPriorityTaskWoken);
	if (xHigherPriorityTaskWoken == pdTRUE) {
		portYIELD_FROM_ISR();

	}
}
