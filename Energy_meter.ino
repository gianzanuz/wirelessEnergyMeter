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

//#define ENABLE

/* Salva páginas HTML na memória de programa */
const char HTML_PAGE[3][300] PROGMEM = {
  { "<html><body><form action=\"send\" method=\"post\" target=\"_blank\"><fieldset><legend>Informações do WiFi:</legend>SSID:<br><input type=\"text\" name=\"SSID\"><br>Pass:<br><input type=\"password\" name=\"pass\"><br><br><input type=\"submit\" value=\"Enviar\"></fieldset></form></body></html>" },
  { "<html><body><fieldset><legend>Informações do WiFi:</legend>Atualizado informações com sucesso!<br></fieldset></body></html>" },
  { "<html><body><fieldset><legend>Informações do WiFi:</legend>Erro: Verifique as informações inseridas.<br></fieldset></body></html>" }
};

/*************************************************************************************
* Private variables
*************************************************************************************/
/* Módulo WiFi ESP8266 */
/* Pino de Enable = 2 */
ESP8266 esp(ESP_ENABLE_PIN);
ESP8266::server_parameter_t client;
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
* Public prototypes
*************************************************************************************/  
void serial_flush(void);
bool serial_get(const char* stringChecked, uint32_t timeout, char* serialBuffer = (char*) NULL);

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

  client.host = "api.thingspeak.com";
  client.path = "/update";
  client.port = "80";

  server.host = ESP_SERVER_IP;
  server.path = "";
  server.port = ESP_SERVER_PORT;
}

void loop() 
{

#ifdef ENABLE
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

  if(esp.checkWifi(5, 100))
  {
    lcd.clear();
    lcd.print("ESP CONNECT AP:"); lcd.setCursor(0,1);
    lcd.print("OK");

    if(esp.connectServer(client))
    {
      lcd.clear();
      lcd.print("ESP SERVER:"); lcd.setCursor(0,1);
      lcd.print("CONNECTED");
    
      String payload;
      payload  = "field1=" + String(value1) + "&";
      payload += "field2=" + String(value2);

      String headers;
      headers  = "X-THINGSPEAKAPIKEY: UF6TH2DLPKQMA4SP\r\n";
      headers += "Content-Type: application/x-www-form-urlencoded\r\n";
      
      if(esp.sendServer(payload, headers, client, ESP8266::HTML_POST))
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
    esp.closeServer();
  }
  else
  {
    lcd.clear();
    lcd.print("ESP CONNECT AP:"); lcd.setCursor(0,1);
    lcd.print("ERROR");
  }
#endif

  /* Aguarda período de publicação */
  while(true)
  {
    static uint32_t previousMillis = 0;
    uint32_t currentMillis = millis();
    if((uint32_t) (currentMillis - previousMillis) > (uint32_t) ESP_SLEEP_TIMEOUT*60*1000)
    {
      previousMillis = currentMillis;
      break;
    }

    /* Verifica se houve conexão ao servidor do ESP8266 */
    if(Serial.available())
    {
      char payload[300];
      char strBuffer[50];
      serial_get((char*) "HTTP", 100, strBuffer);

      /* PÁGINA INICIAL */
      if(strstr(strBuffer, " / "))
      {
        serial_flush();
        
        lcd.clear();
        lcd.print("ESP CLIENT:"); lcd.setCursor(0,1);
        lcd.print("CONNECTED");

        /* Copia página HTML da PROGMEM para RAM */
        memcpy_P(payload, &HTML_PAGE[0], sizeof(payload));

        /* Path */
        server.path = "/";
          
        esp.sendServer(payload, (char*) NULL, server, ESP8266::HTML_RESPONSE_OK);
        esp.closeServer();
      }
      /* ENVIO SSID & PASSWORD */
      else if(strstr(strBuffer, " /send "))
      {
        /* Obtém payload da resposta */
        serial_get((char*) "\r\n\r\n", 100);
        serial_get((char*) "\r\n\r\n", 100, strBuffer);
        serial_flush();

        /* SSID */
        char* tkn = strtok(strBuffer, "&");
        if(tkn)
          wifi.SSID = String(tkn+5);

        /* Password */
        tkn = strtok(NULL, "\r\n");
        if(tkn)
          wifi.password = String(tkn+5);

        if(wifi.SSID.length() && wifi.password.length())
        {
          lcd.clear();
          lcd.print("ESP CLIENT:"); lcd.setCursor(0,1);
          lcd.print("GOT NEW WIFI");
        
          /* Copia página HTML da PROGMEM para RAM */         
          memcpy_P(payload, &HTML_PAGE[1], sizeof(payload));

          /* Path */
          server.path = "/send";

          esp.sendServer(payload, (char*) NULL, server, ESP8266::HTML_RESPONSE_OK);
          esp.closeServer();
        }
        else
        {
          lcd.clear();
          lcd.print("ESP CONNECT AP:"); lcd.setCursor(0,1);
          lcd.print("WRONG PARAMETERS");
          
          /* Copia página HTML da PROGMEM para RAM */
          memcpy_P(payload, &HTML_PAGE[2], sizeof(payload));

          /* Path */
          server.path = "/send";

          esp.sendServer(payload, (char*) NULL, server, ESP8266::HTML_RESPONSE_OK);
          esp.closeServer();
        }
      }
      
      serial_flush();
    }
  }
}

/************************************************************************************
* serial_flush
*
* This function flushes the serial buffer.
*
************************************************************************************/
void serial_flush(void)
{
  /* Obt�m bytes dispon�veis */
  while (Serial.available()) 
    Serial.read();

  return;
}

/************************************************************************************
* serial_get
*
* This function gets messages from serial, during a certain timeout.
*
************************************************************************************/
bool serial_get(const char* stringChecked, uint32_t timeout, char* serialBuffer)
{
    char* u8buffer;
    char* u8bufferInit;
    char u8tempBuffer[100];
    uint32_t u32counter=0;
  
    /* Se o ponteiro � n�o NULO, recebe endere�o de serialBuffer */
    /* Ou se for NULO, recebe endere�o de buffer local */
    if(serialBuffer)
        u8buffer = serialBuffer;
    else
        u8buffer = u8tempBuffer;
  
    /* Recebe endere�o da primeira posi��o */
    u8bufferInit = u8buffer;
  
    /* Limpa o primeiro byte do buffer */
    *u8buffer = '\0';
  
    while(true)
    {
        /* Enquanto houver dados no buffer, receber e comparar com a string a ser checada */
        while(Serial.available())
        {
            char inChar = (char)Serial.read();
            *u8buffer = inChar;  u8buffer++;
            *u8buffer = '\0';
            
            if(strstr(u8bufferInit,stringChecked))
              return true;
            
            if(strlen(u8bufferInit) >= (sizeof(u8tempBuffer) - 1))
              u8buffer = u8bufferInit;
        }
        
        /* Incrementa contador  de timeout */
        u32counter++;
        if (u32counter>=timeout*1000)
            return false;
        delayMicroseconds(1);
    }
}


