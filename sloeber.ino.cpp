#ifdef __IN_ECLIPSE__
//This is a automatic generated file
//Please do not modify this file
//If you touch this file your change will be overwritten during the next build
//This file has been generated on 2023-07-17 17:16:08

#include "Arduino.h"
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

void setup() ;
void loop() ;
void vTask_ControloLuz_Stop(void *pvParameters) ;
void vTask_Brain(void *pvParameters) ;
void vTask_LCD(void *pvParameters) ;
void vTask_Encoder_Velocidade(void *pvParameters) ;
void vTask_Controla_Motor_DC(void *pvParameters) ;
void vTask_Potenciometro_Motor_DC(void *pvParameters) ;
void vTask_Controla_Servos(void *pvParameters) ;
void vTask_BTN_Travagem(void *pvParameters) ;
static void IRAM_ATTR vInterruptHandler_Btn_Travagem(void) ;
static void IRAM_ATTR vInterruptHandler_Encoder_Velocidade(void) ;

#include "SEEV_padeiro.ino"


#endif
