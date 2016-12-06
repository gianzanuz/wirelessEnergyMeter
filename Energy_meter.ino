#include "ESP8266.h"
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <MemoryFree.h>

/*************************************************************************************
* Private macros
*************************************************************************************/
/* Modulo WiFi ESP8266 */
#define ESP_ENABLE_PIN      2
#define ESP_AP_LIST_SIZE    10
#define ESP_SLEEP_TIMEOUT   15  /* Minutes */

/* Chave bipolar */
#define SWT1        3
#define SWT2        4

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
ESP8266::esp_mode_t espMode;
ESP8266::esp_URL_parameter_t espUrl;
ESP8266::esp_AP_parameter_t espAp;

/* LCD I2C */
/* ADDR = 0x27 */
LiquidCrystal_I2C lcd(0x27, 16, 2);

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
  lcd.begin();

  lcd.clear();
  lcd.print(F("FREE MEMORY:")); lcd.setCursor(0,1);
  lcd.print(freeMemory());
  delay(1000);
  
  /* Switch para inicialização do ESP em modo 'client' ou 'url' */
  pinMode(SWT1, INPUT_PULLUP);
  pinMode(SWT2, INPUT_PULLUP);
  if(digitalRead(SWT1))
  {
    lcd.clear();
    lcd.print(F("ESP MODE:")); lcd.setCursor(0,1);
    lcd.print(F("CLIENT"));
    delay(1000);
    
    espMode = ESP8266::ESP_CLIENT_MODE;
    espAp.SSID =     ESP_CLIENT_SSID;
    espAp.password = ESP_CLIENT_PASSWORD;
    espUrl.host =    ESP_CLIENT_IP_DEFAULT;
    espUrl.path =    ESP_CLIENT_PATH_DEFAULT;
    espUrl.port =    ESP_CLIENT_PORT_DEFAULT;

    /* Obtém AP da EEPROM, caso haja */
    if(EEPROM_read_AP(espAp))
    {
        lcd.clear();
        lcd.print(F("EEPROM FOUND:")); lcd.setCursor(0,1);
        lcd.print(F("AP"));
        delay(1000);

        lcd.clear();
        lcd.print(F("SSID: ")); lcd.print(espAp.SSID); lcd.setCursor(0,1);
        lcd.print(F("pass: ")); lcd.print(espAp.password);
        delay(1000);
    }
    else
    {
        lcd.clear();
        lcd.print(F("EEPROM NOT FOUND:")); lcd.setCursor(0,1);
        lcd.print(F("AP"));
        delay(1000);
    }
  }
  else
  {
    lcd.clear();
    lcd.print(F("ESP MODE:")); lcd.setCursor(0,1);
    lcd.print(F("SERVER"));
    delay(1000);
    
    espMode = ESP8266::ESP_SERVER_MODE;
    espAp.SSID =     ESP_SERVER_SSID;
    espAp.password = ESP_SERVER_PASSWORD;
    espUrl.host =    ESP_SERVER_IP_DEFAULT;
    espUrl.path =    ESP_SERVER_PATH_DEFAULT;
    espUrl.port =    ESP_SERVER_PORT_DEFAULT;
  }
    
  /* Configuração inicial do ESP */
  if(!esp.config(espMode))
  {
    lcd.clear();
    lcd.print(F("ESP CONFIG:")); lcd.setCursor(0,1);
    lcd.print(F("ERROR"));
    while(true) {};
  }
  else
  {
    lcd.clear();
    lcd.print(F("ESP CONFIG:")); lcd.setCursor(0,1);
    lcd.print(F("OK"));
  }

  /* Ajuste do Access Point */
  if(!esp.setAP(espMode, espAp))
  {
    lcd.clear();
    lcd.print(F("SET AP:")); lcd.setCursor(0,1);
    lcd.print(F("ERROR"));
    while(true) {};
  }
  else
  {
    lcd.clear();
    lcd.print(F("SET AP:")); lcd.setCursor(0,1);
    lcd.print(F("OK"));
  }

  if(espMode == ESP8266::ESP_SERVER_MODE)
  {
    /* Inicializa servidor */
    if(!esp.connect(espMode, espUrl))
    {
      lcd.clear();
      lcd.print(F("SERVER INIT:")); lcd.setCursor(0,1);
      lcd.print(F("ERROR"));
      while(true) {};
    }
    else
    {
      lcd.clear();
      lcd.print(F("SERVER INIT:")); lcd.setCursor(0,1);
      lcd.print(F("OK"));
    }
  }
}

void loop() 
{
  if(espMode == ESP8266::ESP_CLIENT_MODE)
  {
    /* Obtém grandezas */
    if(esp.checkWifi(10, 1000))
    {
      lcd.clear();
      lcd.print(F("ESP CONNECT AP:")); lcd.setCursor(0,1);
      lcd.print(F("OK"));
  
      if(esp.connect(espMode, espUrl))
      {
        lcd.clear();
        lcd.print(F("ESP CONNECT:")); lcd.setCursor(0,1);
        lcd.print(F("OK"));
      
        char payload[50];
        strcpy(payload, "field1=0&field2=0");
  
        char headers[150];
        strcpy(headers, "X-THINGSPEAKAPIKEY: UF6TH2DLPKQMA4SP\r\n");
        strcat(headers, "Content-Type: application/x-www-form-urlencoded\r\n");
        
        if(esp.send(payload, headers, espUrl, ESP8266::HTML_POST))
        {
          lcd.clear();
          lcd.print(F("ESP SEND:")); lcd.setCursor(0,1);
          lcd.print(F("OK"));
        }
        else
        {
          lcd.clear();
          lcd.print(F("ESP SEND:")); lcd.setCursor(0,1);
          lcd.print(F("ERROR"));
        }
      }
      else
      {
        lcd.clear();
        lcd.print(F("ESP CONNECT:")); lcd.setCursor(0,1);
        lcd.print(F("ERROR"));
      }
      esp.close();
    }
    else
    {
      lcd.clear();
      lcd.print(F("ESP CONNECT AP:")); lcd.setCursor(0,1);
      lcd.print(F("ERROR"));
    }
  }

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

    if(espMode == ESP8266::ESP_SERVER_MODE)
    {
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
          delay(100);
          
          lcd.clear();
          lcd.print(F("ESP CLIENT:")); lcd.setCursor(0,1);
          lcd.print(F("CONNECTED"));
  
          /* Copia página HTML da PROGMEM para RAM */
          memcpy_P(payload, &HTML_PAGE[0], sizeof(payload));
  
          /* Path */
          espUrl.path = "/";
            
          esp.send(payload, (char*) NULL, espUrl, ESP8266::HTML_RESPONSE_OK);
          delay(100);
          esp.close();
        }
        /* ENVIO SSID & PASSWORD */
        else if(strstr(strBuffer, " /send "))
        {
          /* Obtém payload da resposta */
          serial_get((char*) "\r\n\r\n", 100);
          serial_get((char*) "\r\n\r\n", 100, strBuffer);
          serial_flush();
          delay(100);
  
          /* SSID */
          char* tkn = strtok(strBuffer, "&");
          if(tkn)
            espAp.SSID = String(tkn+5);
          else
            espAp.SSID = "";
  
          /* Password */
          tkn = strtok(NULL, "\r\n");
          if(tkn)
            espAp.password = String(tkn+5);
          else
            espAp.password = "";
  
          if(espAp.SSID.length() && espAp.password.length())
          {
            lcd.clear();
            lcd.print(F("ESP CLIENT:")); lcd.setCursor(0,1);
            lcd.print(F("GOT NEW WIFI"));
          
            /* Copia página HTML da PROGMEM para RAM */         
            memcpy_P(payload, &HTML_PAGE[1], sizeof(payload));
  
            /* Path */
            espUrl.path = "/send";
  
            esp.send(payload, (char*) NULL, espUrl, ESP8266::HTML_RESPONSE_OK);
            delay(100);
            esp.close();

            /* Salva AP na EEPROM */
            if(EEPROM_write_AP(espAp))
            {
                lcd.clear();
                lcd.print(F("EEPROM SAVED:")); lcd.setCursor(0,1);
                lcd.print(F("AP"));
                delay(1000);

                lcd.clear();
                lcd.print(F("SSID: ")); lcd.print(espAp.SSID); lcd.setCursor(0,1);
                lcd.print(F("pass: ")); lcd.print(espAp.password);
                delay(1000);
            }
          }
          else
          {
            lcd.clear();
            lcd.print(F("ESP CLIENT:")); lcd.setCursor(0,1);
            lcd.print(F("WRONG INPUT"));
            
            /* Copia página HTML da PROGMEM para RAM */
            memcpy_P(payload, &HTML_PAGE[2], sizeof(payload));
  
            /* Path */
            espUrl.path = "/send";
  
            esp.send(payload, (char*) NULL, espUrl, ESP8266::HTML_RESPONSE_OK);
            delay(100);
            esp.close();
          }
        }
        
        serial_flush();
      }
    }
  }
}

/************************************************************************************
* EEPROM_write_AP
*
* This function flushes the serial buffer.
*
************************************************************************************/
bool EEPROM_write_AP(ESP8266::esp_AP_parameter_t &ap)
{
    uint16_t addr=0;

    /* Verifica se possui ap */
    if(!ap.SSID.length() || !ap.password.length())
        return false;
    
    /* Indicador de início */
    EEPROM.write(addr, 0xFE);
    addr++;
    
    /* SSID */
    for(uint16_t i=0; i<ap.SSID.length(); i++)
    {
        EEPROM.write(addr, ap.SSID[i]);
        addr++;
    }
    /* NULL char */
    EEPROM.write(addr, '\0');
    addr++;
    
    /* Password */
    for(uint16_t i=0; i<ap.password.length(); i++)
    {
        EEPROM.write(addr, ap.password[i]);
        addr++;
    }
    /* NULL char */
    EEPROM.write(addr, '\0');
    addr++;
    
    return true;
}

/************************************************************************************
* EEPROM_read_AP
*
* This function flushes the serial buffer.
*
************************************************************************************/
bool EEPROM_read_AP(ESP8266::esp_AP_parameter_t &ap)
{
    uint16_t addr=0;
    uint8_t i=0;
    char strBuffer[50];

    /* Indicador de início */
    if(EEPROM.read(addr) != 0xFE)
        return false;
    addr++;

    /* SSID */ 
    while(true)
    {
      char u8byte = EEPROM.read(addr);
      addr++;

      if(u8byte)
      {
          strBuffer[i] = u8byte;
          i++;
      }
      else
      {
          strBuffer[i] = '\0';
          i=0;
          ap.SSID = String(strBuffer);
          break;
      }
    }
    /* Password */ 
    while(true)
    {
      char u8byte = EEPROM.read(addr);
      addr++;

      if(u8byte)
      {
          strBuffer[i] = u8byte;
          i++;
      }
      else
      {
          strBuffer[i] = '\0';
          i=0;
          ap.password = String(strBuffer);
          break;
      }
    }

    return true;
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


