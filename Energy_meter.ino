#include "ESP8266.h"
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

/*************************************************************************************
* Private macros
*************************************************************************************/
/* Modulo WiFi ESP8266 */
#define ESP_ENABLE_PIN      2
#define ESP_AP_LIST_SIZE    10
#define ESP_SLEEP_TIMEOUT   15  /* Minutes */

/* Entradas Analógicas */
#define INPUT1      A1
#define INPUT2      A2
#define INPUT3      A3

/* Enable dos Sensores */
#define PWR1        3
#define PWR2        4

/*************************************************************************************
* Private variables
*************************************************************************************/
/* Módulo WiFi ESP8266 */
/* Pino de Enable = 2 */
ESP8266 esp(ESP_ENABLE_PIN);
ESP8266::server_parameter_t server;
ESP8266::esp_wifi_config_t wifi;

/* LCD I2C */
/* ADDR = 0x27 */
LiquidCrystal_I2C lcd(0x27, 16, 2);

/* Valores dos sensores */
int value1;
int value2;
int value3;

/*************************************************************************************
* Private functions
*************************************************************************************/
void setup()
{
  /* Entradas Analógicas */
  pinMode(INPUT1, INPUT);
  pinMode(INPUT2, INPUT);
  pinMode(INPUT3, INPUT);

  /* Saídas de Alimentação */
  pinMode(PWR1, OUTPUT);
  pinMode(PWR2, OUTPUT);

  lcd.begin();

  if(!esp.config())
  {
    lcd.clear();
    lcd.print("ESP CONFIG:"); lcd.setCursor(0,1);
    lcd.print("ERROR");
    while(true) {};
  }
  else
  {
    lcd.clear();
    lcd.print("ESP CONFIG:"); lcd.setCursor(0,1);
    lcd.print("OK");
  }

  wifi.SSID = "WiFi WaFer";
  wifi.password = "moussedebanana";

  server.host = "api.thingspeak.com";
  server.path = "/update";
  server.port = "80";

  if(!esp.connectAP(wifi))
  {
    lcd.clear();
    lcd.print("ESP CONNECT AP:"); lcd.setCursor(0,1);
    lcd.print("ERROR");
    while(true) {};
  }
  else
  {
    lcd.clear();
    lcd.print("ESP CONNECT AP:"); lcd.setCursor(0,1);
    lcd.print("OK");
  }
  
//  ESP8266::esp_wifi_config_t wifiList[ESP_AP_LIST_SIZE];
//  esp.listAP(wifiList, ESP_AP_LIST_SIZE);
//  lcd.print(wifiList[0].SSID); 
//  lcd.setCursor(0,1);
//  lcd.print(wifiList[0].signal);
}

void loop() 
{
  /* Liga Alimentação dos sensores */
  digitalWrite(PWR1,HIGH);
  digitalWrite(PWR2,HIGH);
  delay(250);

  /* Leitura dos valores */
  value1 = analogRead(INPUT1);
  value2 = analogRead(INPUT2);
  value3 = analogRead(INPUT3);

  /* Desliga Alimentação dos sensores */
  digitalWrite(PWR1,LOW);
  digitalWrite(PWR2,LOW);

  if(esp.connectServer(server))
  {
    lcd.clear();
    lcd.print("ESP SERVER:"); lcd.setCursor(0,1);
    lcd.print("CONNECTED");
  
    String payload;
    payload  = "field1=" + String(value1) + "&";
    payload += "field2=" + String(value2);
    
    if(esp.sendServer(payload, server))
    {
      lcd.clear();
      lcd.print("ESP SERVER:"); lcd.setCursor(0,1);
      lcd.print("SEND OK");
    }
    else
    {
      lcd.clear();
      lcd.print("ESP SERVER:"); lcd.setCursor(0,1);
      lcd.print("SEND ERROR");
    }
  }
  else
  {
    lcd.clear();
    lcd.print("ESP SERVER:"); lcd.setCursor(0,1);
    lcd.print("NOT CONNECTED");
  }

  /* Período de publicação */
  delay((uint32_t) ESP_SLEEP_TIMEOUT*60*1000); /* milisegundos */
}


