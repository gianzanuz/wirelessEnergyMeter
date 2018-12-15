#include "ESP8266.h"
#include "ADS1115.h"
#include <Wire.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>
//#include <MemoryFree.h>
#include <math.h> 

/*************************************************************************************
* Private macros
*************************************************************************************/
/* Visualizar memória disponível */
//#define FREE_MEMORY_DISPLAY

/* Modulo WiFi ESP8266 */
#define ESP_ENABLE_PIN      2
#define ESP_AP_LIST_SIZE    10
#define ESP_SLEEP_TIMEOUT   1  /* Minutes */

/* ADC ADS1115 16bits */
#define SCALE_1             50000.0/1000.0   /* Relação do TC */
#define SCALE_2             50000.0/1000.0   /* Relação do TC */
#define DATA_SIZE         500            /* Quantidade de amostras */

/* Chave bipolar */
#define SWT1        3
#define SWT2        4

/* Salva páginas HTML na memória de programa */
const char HTML_PAGE[3][325] PROGMEM = {
  { "<html><body><form action=\"send\" method=\"post\" target=\"_blank\"><fieldset><legend>Configuracoes:</legend>SSID:<br><input type=\"text\" name=\"SSID\"><br>PASS:<br><input type=\"password\" name=\"pass\"><br>KEY:<br><input type=\"text\" name=\"key\"> <input type=\"submit\" value=\"Enviar\"></fieldset></form></body></html>" },
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

#ifdef ADS1115_ENABLE
/* Conversor ADC ADS1115 */
ADS1115 ads;
ADS1115::ADS1115_config_t ADS1115Config;
ADS1115::ADS1115_data_t   ADS1115Data;
#endif

/* LCD 16x2 */
LiquidCrystal lcd(10, 11, 6, 7, 8, 9);

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
  lcd.begin(16, 2);

#ifdef FREE_MEMORY_DISPLAY
  lcd.clear();
  lcd.print(F("FREE MEMORY:")); lcd.setCursor(0,1);
  lcd.print(freeMemory());
  delay(1000);
#endif

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
    espUrl.key  =    "";

    /* Obtém AP e key da EEPROM, caso haja */
    if(EEPROM_read_AP(espAp, espUrl))
    {
        lcd.clear();
        lcd.print(F("EEPROM FOUND:")); lcd.setCursor(0,1);
        lcd.print(F("AP,Key"));
        delay(1000);

        lcd.clear();
        lcd.print(F("SSID: ")); lcd.print(espAp.SSID); lcd.setCursor(0,1);
        lcd.print(F("pass: ")); lcd.print(espAp.password);
        delay(1000);

        lcd.clear();
        lcd.print(F("Key: "));  lcd.setCursor(0,1);
        lcd.print(espUrl.key);
        delay(1000);
    }
    else
    {
        lcd.clear();
        lcd.print(F("EEPROM NOT FOUND:")); lcd.setCursor(0,1);
        lcd.print(F("AP,Key"));
        while(true) {};
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
    espUrl.key  =    "";
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
    unsigned int value1=0;
    unsigned int value2=0;
    float rmsSum=0.0;
    
#ifdef ADS1115_ENABLE
    /* Insere configurações do primeiro ADS */
    /* Pino de endereço I2C = GND */
    /* Canal diferencial = A0 - A1 */
    /* Conversão contínua */
    /* PGA FS = +-2048mV */
    /* 860 samples/seg */
    /* Comparador desabilitado */
    memset(&ADS1115Config,0,sizeof(ADS1115Config));
    ADS1115Config.status            = ADS1115::OS_N_EFF;
    ADS1115Config.mux               = ADS1115::MUX_0_1;
    ADS1115Config.gain              = ADS1115::PGA_2048;
    ADS1115Config.mode              = ADS1115::MODE_CONT;
    ADS1115Config.rate              = ADS1115::DR_860;
    ADS1115Config.comp_queue        = ADS1115::COMP_DISABLE;
    ADS1115Config.i2c_addr          = ADS1115::ADDR_GND;
    ads.config(&ADS1115Config);

    /* Remove primeiras 5 amostras */
    ADS1115Data.data_size = 5;
    ADS1115Data.i2c_addr = ADS1115::ADDR_GND;
    ads.readData(&ADS1115Data);

    rmsSum = 0.0;
    for(int i=0; i<DATA_SIZE; i++)
    {
      /* Leitura do valor do ADC */
      /* Solicita 1 amostra */
      ADS1115Data.data_size = 1;
      ADS1115Data.i2c_addr = ADS1115::ADDR_GND;
      ads.readData(&ADS1115Data);

      /* Realiza os cálculos */
      float adc = ADS1115Data.data_byte[0]*0.0625;  /* 0.0625 = 2048.0/32768.0 */
      rmsSum += adc*adc;
    }
    value1 = (unsigned int) (sqrt(rmsSum/DATA_SIZE)*SCALE_1);  /* Corrente RMS [mA] */

    /* Insere configurações do primeiro ADS */
    /* Pino de endereço I2C = GND */
    /* Canal diferencial = A2 - A3 */
    /* Conversão contínua */
    /* PGA FS = +-2048mV */
    /* 860 samples/seg */
    /* Comparador desabilitado */
    memset(&ADS1115Config,0,sizeof(ADS1115Config));
    ADS1115Config.status            = ADS1115::OS_N_EFF;
    ADS1115Config.mux               = ADS1115::MUX_2_3;
    ADS1115Config.gain              = ADS1115::PGA_2048;
    ADS1115Config.mode              = ADS1115::MODE_CONT;
    ADS1115Config.rate              = ADS1115::DR_860;
    ADS1115Config.comp_queue        = ADS1115::COMP_DISABLE;
    ADS1115Config.i2c_addr          = ADS1115::ADDR_GND;
    ads.config(&ADS1115Config);

    /* Remove primeiras 5 amostras */
    ADS1115Data.data_size = 5;
    ADS1115Data.i2c_addr = ADS1115::ADDR_GND;
    ads.readData(&ADS1115Data);

    rmsSum = 0.0;
    for(int i=0; i<DATA_SIZE; i++)
    {
      /* Leitura do valor do ADC */
      /* Solicita 1 amostra */
      ADS1115Data.data_size = 1;
      ADS1115Data.i2c_addr = ADS1115::ADDR_GND;
      ads.readData(&ADS1115Data);

      /* Realiza os cálculos */
      float adc = ADS1115Data.data_byte[0]*0.0625;  /* 0.0625 = 2048.0/32768.0 */
      rmsSum += adc*adc;
    }
    value2 = (unsigned int) (sqrt(rmsSum/DATA_SIZE)*SCALE_2);  /* Corrente RMS [mA] */
#endif

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

        char headers[52];
        char key[17]; espUrl.key.toCharArray(key, sizeof(key));
        sprintf(headers, "?api_key=%s&field1=%u&field2=%u", key, value1, value2);

        if(esp.send((char*) NULL, headers, espUrl, ESP8266::HTML_GET))
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
        char payload[325];
        char strBuffer[65];
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
        /* ENVIO SSID & PASSWORD & KEY */
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
          tkn = strtok(NULL, "&");
          if(tkn)
            espAp.password = String(tkn+5);
          else
            espAp.password = "";

          /* Key */
          tkn = strtok(NULL, "\r\n");
          if(tkn)
            espUrl.key = String(tkn+4);
          else
            espUrl.key = "";
  
          if(espAp.SSID.length() && espAp.password.length() && espUrl.key.length())
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
            if(EEPROM_write_AP(espAp, espUrl))
            {
                lcd.clear();
                lcd.print(F("EEPROM SAVED:")); lcd.setCursor(0,1);
                lcd.print(F("AP"));
                delay(1000);

                lcd.clear();
                lcd.print(F("SSID: ")); lcd.print(espAp.SSID); lcd.setCursor(0,1);
                lcd.print(F("pass: ")); lcd.print(espAp.password);
                delay(1000);
                
                lcd.clear();
                lcd.print(F("Key: "));lcd.setCursor(0,1);
                lcd.print(espUrl.key);
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
bool EEPROM_write_AP(ESP8266::esp_AP_parameter_t &ap, ESP8266::esp_URL_parameter_t &url)
{
    uint16_t addr=0;

    /* Verifica se possui ap e key */
    if(!ap.SSID.length() || !ap.password.length() || !url.key.length())
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

    /* Key */
    for(uint16_t i=0; i<url.key.length(); i++)
    {
        EEPROM.write(addr, url.key[i]);
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
bool EEPROM_read_AP(ESP8266::esp_AP_parameter_t &ap, ESP8266::esp_URL_parameter_t &url)
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
    /* Key */ 
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
          url.key = String(strBuffer);
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


