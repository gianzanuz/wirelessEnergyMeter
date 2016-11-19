#include "ESP8266.h"
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#define ESP_ENABLE        2
#define ESP_AP_LIST_SIZE 10

#define INPUT1      A0
#define INPUT2      A1
#define INPUT3      A2

#define PWR1        3
#define PWR2        4

//#define DEBUG

/* Módulo WiFi ESP8266 */
/* Pino de Enable = 2 */
ESP8266 esp(ESP_ENABLE);

/* LCD I2C */
/* ADDR = 0x27 */
LiquidCrystal_I2C lcd(0x27, 16, 2);

/* Valores dos sensores */
int value1;
int value2;
int value3;

ESP8266::server_parameter_t server;
ESP8266::esp_wifi_config_t wifi;

/* Valores Iniciais */
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
    lcd.print("ESP: CONFIG ERROR");
    while(true) {};
  }
  else
  {
    lcd.clear();
    lcd.print("ESP: CONFIG OK");
  }

  wifi.SSID = "WiFi WaFer";
  wifi.password = "moussedebanana";

  server.host = "api.thingspeak.com";
  server.path = "/update";
  server.port = "80";

  if(!esp.connectAP(wifi))
  {
    lcd.clear();
    lcd.print("ESP: CONNECT ERROR");
    while(true) {};
  }
  else
  {
    lcd.clear();
    lcd.print("ESP: CONNECT OK");
  }
  
//  ESP8266::esp_wifi_config_t wifiList[ESP_AP_LIST_SIZE];
//  esp.listAP(wifiList, ESP_AP_LIST_SIZE);
//  lcd.print(wifiList[0].SSID); 
//  lcd.setCursor(0,1);
//  lcd.print(wifiList[0].signal);
}

void loop() 
{
#ifndef DEBUG
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

  //lcd.clear();
  //lcd.print("Valor 1 = "); lcd.print(value1); lcd.setCursor(0,1);
  //lcd.print("Valor 2 = "); lcd.print(value2); 

  
#endif

  if(esp.connectServer(server))
  {
      String payload;
      payload  = "field1=" + String(value1) + "&";
      payload += "field2=" + String(value2);
      lcd.clear();
      lcd.print("Thing: Connected OK");
      if(esp.sendServer(payload, server))
      {
          lcd.clear();
          lcd.print("Thing: Send OK");
      }
      else
      {
          lcd.clear();
          lcd.print("Thing: Send ERROR");
      }
  }
  else
  {
      lcd.clear();
      lcd.print("Thing: Connected ERROR");
  }
  delay(60000);
}


