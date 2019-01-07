#include "ESP8266.h"
#include "ADS1115.h"
#include <Wire.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <MemoryFree.h>
#include <math.h>

/*************************************************************************************
  Private macros
*************************************************************************************/
/* Visualizar memória disponível */
#define FREE_MEMORY_DISPLAY

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
const char HTML_PAGE_INDEX[] PROGMEM = "<html><body><form action=\"send.html\" method=\"post\"><fieldset><legend>Configuracoes:</legend>SSID:<br><input type=\"text\" name=\"ssid\"><br>PASS:<br><input type=\"password\" name=\"pass\"><br>KEY:<br><input type=\"text\" name=\"key\"><br><input type=\"submit\" value=\"Enviar\"></fieldset></form></body></html>";
const char HTML_PAGE_SUCCESS[] PROGMEM = "<html><body><fieldset><legend>Informações do WiFi:</legend>Atualizado informações com sucesso!<br></fieldset></body></html>";
const char HTML_PAGE_FAIL[] PROGMEM = "<html><body><fieldset><legend>Informações do WiFi:</legend>Erro: Verifique as informações inseridas.<br></fieldset></body></html>";

/*************************************************************************************
  Private variables
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
  Public prototypes
*************************************************************************************/
bool IOT_send_GET(const char* path, const char* query, const char* host);

void WEB_init(void);
bool WEB_process_GET(uint8_t connection, char* path, char* parameters, uint32_t parametersSize);
bool WEB_process_POST(uint8_t connection, char* path, char* body, uint32_t bodySize);

void serial_flush(void);
bool serial_get(const char* stringChecked, uint32_t timeout, char* serialBuffer = (char*) NULL);

/* Converts a hex character to its integer value */
char from_hex(char ch);
char to_hex(char code);
void url_encode(char *str, char* encoded);
void url_decode(char *str, char* decoded, int size);
void str_safe(char *str, uint32_t size);

/*************************************************************************************
  Private functions
*************************************************************************************/
void setup()
{
  lcd.begin(16, 2);

#ifdef FREE_MEMORY_DISPLAY
  lcd.clear();
  lcd.print(F("FREE MEMORY:")); lcd.setCursor(0, 1);
  lcd.print(freeMemory());
  delay(1000);
#endif

  /* Switch para inicialização do ESP em modo 'client' ou 'url' */
  pinMode(SWT1, INPUT_PULLUP);
  pinMode(SWT2, INPUT_PULLUP);
  if (digitalRead(SWT1))
  {
    lcd.clear();
    lcd.print(F("ESP MODE:")); lcd.setCursor(0, 1);
    lcd.print(F("CLIENT"));
    delay(1000);

    espMode = ESP8266::ESP_CLIENT_MODE;
    espAp.SSID =     ESP_CLIENT_SSID;
    espAp.password = ESP_CLIENT_PASSWORD;
    
    strncpy(espUrl.host, ESP_CLIENT_IP_DEFAULT, sizeof(espUrl.host));
    espUrl.host[sizeof(espUrl.host) - 1] = '\0';
    strncpy(espUrl.path, ESP_CLIENT_PATH_DEFAULT, sizeof(espUrl.path));
    espUrl.path[sizeof(espUrl.path) - 1] = '\0';
    espUrl.port   = ESP_CLIENT_PORT_DEFAULT;
    espUrl.key[0] = '\0';

    /* Obtém AP e key da EEPROM, caso haja */
    if (EEPROM_read_AP(espAp, espUrl))
    {
      lcd.clear();
      lcd.print(F("EEPROM FOUND:")); lcd.setCursor(0, 1);
      lcd.print(F("AP,Key"));
      delay(1000);

      lcd.clear();
      lcd.print(F("SSID: ")); lcd.print(espAp.SSID); lcd.setCursor(0, 1);
      lcd.print(F("pass: ")); lcd.print(espAp.password);
      delay(1000);

      lcd.clear();
      lcd.print(F("Key: "));  lcd.setCursor(0, 1);
      lcd.print(espUrl.key);
      delay(1000);
    }
    else
    {
      lcd.clear();
      lcd.print(F("EEPROM NOT FOUND:")); lcd.setCursor(0, 1);
      lcd.print(F("AP,Key"));
      while (true) {};
    }
  }
  else
  {
    lcd.clear();
    lcd.print(F("ESP MODE:")); lcd.setCursor(0, 1);
    lcd.print(F("SERVER"));
    delay(1000);

    espMode = ESP8266::ESP_SERVER_MODE;
    espAp.SSID      = ESP_SERVER_SSID;
    espAp.password  = ESP_SERVER_PASSWORD;

    strncpy(espUrl.host, ESP_SERVER_IP_DEFAULT, sizeof(espUrl.host));
    espUrl.host[sizeof(espUrl.host) - 1] = '\0';
    strncpy(espUrl.path, ESP_SERVER_PATH_DEFAULT, sizeof(espUrl.path));
    espUrl.path[sizeof(espUrl.path) - 1] = '\0';
    espUrl.port     = ESP_SERVER_PORT_DEFAULT;
    espUrl.key[0]   = '\0';
  }

  /* Configuração inicial do ESP */
  if (!esp.config(espMode))
  {
    lcd.clear();
    lcd.print(F("ESP CONFIG:")); lcd.setCursor(0, 1);
    lcd.print(F("ERROR"));
    while (true) {};
  }
  else
  {
    lcd.clear();
    lcd.print(F("ESP CONFIG:")); lcd.setCursor(0, 1);
    lcd.print(F("OK"));
  }

  /* Ajuste do Access Point */
  if (!esp.setAP(espMode, espAp))
  {
    lcd.clear();
    lcd.print(F("SET AP:")); lcd.setCursor(0, 1);
    lcd.print(F("ERROR"));
    while (true) {};
  }
  else
  {
    lcd.clear();
    lcd.print(F("SET AP:")); lcd.setCursor(0, 1);
    lcd.print(F("OK"));
  }

  if (espMode == ESP8266::ESP_SERVER_MODE)
  {
    /* Inicializa servidor */
    if (!esp.connect(espMode, espUrl))
    {
      lcd.clear();
      lcd.print(F("SERVER INIT:")); lcd.setCursor(0, 1);
      lcd.print(F("ERROR"));
      while (true) {};
    }
    else
    {
      lcd.clear();
      lcd.print(F("SERVER INIT:")); lcd.setCursor(0, 1);
      lcd.print(F("OK"));
    }
  }
}

void loop()
{
  if (espMode == ESP8266::ESP_CLIENT_MODE)
  {
    unsigned int value1 = 0;
    unsigned int value2 = 0;
    float rmsSum = 0.0;

#ifdef ADS1115_ENABLE
    /* Insere configurações do primeiro ADS */
    /* Pino de endereço I2C = GND */
    /* Canal diferencial = A0 - A1 */
    /* Conversão contínua */
    /* PGA FS = +-2048mV */
    /* 860 samples/seg */
    /* Comparador desabilitado */
    memset(&ADS1115Config, 0, sizeof(ADS1115Config));
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
    for (int i = 0; i < DATA_SIZE; i++)
    {
      /* Leitura do valor do ADC */
      /* Solicita 1 amostra */
      ADS1115Data.data_size = 1;
      ADS1115Data.i2c_addr = ADS1115::ADDR_GND;
      ads.readData(&ADS1115Data);

      /* Realiza os cálculos */
      float adc = ADS1115Data.data_byte[0] * 0.0625; /* 0.0625 = 2048.0/32768.0 */
      rmsSum += adc * adc;
    }
    value1 = (unsigned int) (sqrt(rmsSum / DATA_SIZE) * SCALE_1); /* Corrente RMS [mA] */

    /* Insere configurações do primeiro ADS */
    /* Pino de endereço I2C = GND */
    /* Canal diferencial = A2 - A3 */
    /* Conversão contínua */
    /* PGA FS = +-2048mV */
    /* 860 samples/seg */
    /* Comparador desabilitado */
    memset(&ADS1115Config, 0, sizeof(ADS1115Config));
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
    for (int i = 0; i < DATA_SIZE; i++)
    {
      /* Leitura do valor do ADC */
      /* Solicita 1 amostra */
      ADS1115Data.data_size = 1;
      ADS1115Data.i2c_addr = ADS1115::ADDR_GND;
      ads.readData(&ADS1115Data);

      /* Realiza os cálculos */
      float adc = ADS1115Data.data_byte[0] * 0.0625; /* 0.0625 = 2048.0/32768.0 */
      rmsSum += adc * adc;
    }
    value2 = (unsigned int) (sqrt(rmsSum / DATA_SIZE) * SCALE_2); /* Corrente RMS [mA] */
#endif

    /* Obtém grandezas */
    if (esp.checkWifi(10, 1000))
    {
      lcd.clear();
      lcd.print(F("ESP CONNECT AP:")); lcd.setCursor(0, 1);
      lcd.print(F("OK"));

      if (esp.connect(espMode, espUrl))
      {
        lcd.clear();
        lcd.print(F("ESP CONNECT:")); lcd.setCursor(0, 1);
        lcd.print(F("OK"));

        char headers[] = "/update";
        char query[65];
        sprintf(query, "?api_key=%s&field1=%u&field2=%u", espUrl.key, value1, value2);

        if(IOT_send_GET(headers, query, espUrl.host))
        {
          lcd.clear();
          lcd.print(F("ESP SEND:")); lcd.setCursor(0, 1);
          lcd.print(F("OK"));
        }
        else
        {
          lcd.clear();
          lcd.print(F("ESP SEND:")); lcd.setCursor(0, 1);
          lcd.print(F("ERROR"));
        }
      }
      else
      {
        lcd.clear();
        lcd.print(F("ESP CONNECT:")); lcd.setCursor(0, 1);
        lcd.print(F("ERROR"));
      }
      esp.close(WIFI_CLOSE_ALL);
    }
    else
    {
      lcd.clear();
      lcd.print(F("ESP CONNECT AP:")); lcd.setCursor(0, 1);
      lcd.print(F("ERROR"));
    }
  }

  /* Aguarda período de publicação */
  while (true)
  {
    static uint32_t previousMillis = 0;
    uint32_t currentMillis = millis();
    if ((uint32_t) (currentMillis - previousMillis) > (uint32_t) ESP_SLEEP_TIMEOUT * 60 * 1000)
    {
      previousMillis = currentMillis;
      break;
    }

    if (espMode == ESP8266::ESP_SERVER_MODE)
    {
      /* Verifica se houve conexão ao servidor do ESP8266 */
      if (Serial.available())
        WEB_init();
    }
  }
}


/************************************************************************************
  IOT_send_GET

  .

************************************************************************************/
bool IOT_send_GET(const char* path, const char* query, const char* host)
{
    /* ESP8266: Inicializar envio */
    serial_flush();
    Serial.print(F("AT+CIPSENDEX=0,2047\r\n"));
    if(!serial_get(">", 100, NULL))                  /* Aguarda '>' */
    {
        esp.close(WIFI_CLOSE_ALL);
        return false;
    }

    /* HTTP HEADERS */  
    serial_flush();
    Serial.print(F("GET "));
    /* Path */
    Serial.print(path);
    /* Query */
    Serial.print(query);
    /* HTTP/1.1 */
    Serial.print(F(" HTTP/1.1\r\n"));
    /* Host */
    Serial.print(F("Host: "));
    Serial.print(host);
    Serial.print(F("\r\n"));
    /* Connection */
    //Serial.print(F("Connection: Close\r\n"));
    /* Header End */
    Serial.print(F("\r\n"));
    
    /* AT: End Send */
    Serial.write("\\0");

    /* Return */
    serial_get((char*) "SEND OK", 1000, NULL);
    if(!serial_get((char*) "CLOSED\r\n", 1000, NULL))
    {
        esp.close(WIFI_CLOSE_ALL);
        return false;
    }
    return true;
}

void WEB_init()
{
  char strBuffer[350];
  char path[25];
  uint8_t connection;

  /* Verifica se cliente se conectou na página */
  if (!serial_get((char*) "CONNECT\r\n", 100, strBuffer))
  {
    /* Fecha todas as conexões */
    esp.close(WIFI_CLOSE_ALL);
    return;
  }

  /* Obtém os parâmetros da requisição feita pelo browser */
  if (!serial_get((char*) "HTTP/1.1\r\n", 100, strBuffer))
  {
    /* Fecha todas as conexões */
    esp.close(WIFI_CLOSE_ALL);
    return;
  }

  /* Obtém o index da conexão */
  char* tkn;
  strtok(strstr(strBuffer, "+IPD"), ",");
  tkn = strtok(NULL, ",");
  connection = (uint8_t) atoi(tkn);

  /* Obtém tipo de requisição: GET ou POST */
  strtok(NULL, ":");
  tkn = strtok(NULL, " ");
  if (!strcmp(tkn, "GET"))
  {
    /* Obtém a rota da conexão */
    memset(path, 0, sizeof(path));
    tkn = strtok(NULL, " ?");
    if (tkn)
    {
      strncpy(path, tkn, sizeof(path));
      path[sizeof(path) - 1] = '\0';
    }

    /* Obtém parâmetro do GET, caso haja */
    tkn = strtok(NULL, " ?");
    if (tkn)
    {
      uint32_t lenght = strlen(tkn);
      memmove(strBuffer, tkn, lenght);
      strBuffer[lenght] = '\0';
    }

    /* Inicializa GET */
    WEB_process_GET(connection, path, strBuffer, sizeof(strBuffer));
  }
  else if (!strcmp(tkn, "POST"))
  {
    /* Obtém a rota da conexão */
    memset(path, 0, sizeof(path));
    tkn = strtok(NULL, " ?");
    if (tkn)
    {
      strncpy(path, tkn, sizeof(path));
      path[sizeof(path) - 1] = '\0';
    }

    /* Varre todos os parâmetros do header, até encontrar o body com a mensagem do POST */
    while(serial_get("\r\n", 100, strBuffer)) {};

    /* Inicializa POST */
    WEB_process_POST(connection, path, strBuffer, sizeof(strBuffer));
  }
  /* 405 - METHOD NOT ALLOWED */
  else
  {
    /* ESP8266: Inicializar envio */
    serial_flush();
    Serial.print("AT+CIPSENDEX=");    /* AT: Send command */
    Serial.print(connection);
    Serial.print(",2047\r\n");
    if (!serial_get(">", 100, NULL))                 /* Aguarda '>' */
    {
      esp.close(WIFI_CLOSE_ALL);
      return;
    }

    /* Header */
    Serial.print(F("HTTP/1.1 405 Method Not Allowed\r\n"));

    /* Finaliza comando de envio ao módulo WiFi */
    Serial.print("\\0");

    /* Obtém resposta e encerra conexão */
    serial_get("SEND OK\r\n", 100, NULL);
    esp.close(connection);
  }

  serial_flush();
  return;
}


/************************************************************************************
  WEB_process_GET

  .

************************************************************************************/
bool WEB_process_GET(uint8_t connection, char* path, char* parameters, uint32_t parametersSize)
{
  /* INDEX */
  if (!strcmp(path, "/") || !strcmp(path, "/index.html"))
  {
    delay(100);

    lcd.clear();
    lcd.print(F("ESP CLIENT:")); lcd.setCursor(0, 1);
    lcd.print(F("CONNECTED"));

    /* ESP8266: Inicializar envio */
    serial_flush();
    Serial.print(F("AT+CIPSENDEX="));
    Serial.print(connection);
    Serial.print(F(",2047\r\n"));
    if(!serial_get(">", 100, NULL))                  /* Aguarda '>' */
    {
        esp.close(WIFI_CLOSE_ALL);
        return false;
    }

    /* HTTP HEADERS */  
    serial_flush();
    Serial.print(F("HTTP/1.1 200 OK\r\n"));
    /* Hosts */
    Serial.print(F("Host: 192.168.1.4\r\n"));
    /* Connection */
    Serial.print(F("Connection: Close\r\n"));
    /* Content-Length */
    Serial.print(F("Content-Length: "));
    Serial.print((strlen(HTML_PAGE_INDEX)));
    Serial.print(F("\r\n"));
    /* Header End */
    Serial.print(F("\r\n"));

    /* Envia página HTML da PROGMEM */
    Serial.print((__FlashStringHelper *) HTML_PAGE_INDEX);

    /* AT: End Send */
    Serial.write("\\0");

    /* Return */
    serial_get((char*) "SEND OK", 1000, NULL);
    if(!serial_get((char*) "CLOSED\r\n", 1000, NULL))
    {
        esp.close(WIFI_CLOSE_ALL);
        return false;
    }
    return true;
  }
  /* 404 - NOT FOUND */
  else
  {
    /* ESP8266: Inicializar envio */
    serial_flush();
    Serial.print("AT+CIPSENDEX=");    /* AT: Send command */
    Serial.print(connection);
    Serial.print(",2047\r\n");
    if (!serial_get(">", 100, NULL))                 /* Aguarda '>' */
    {
      esp.close(WIFI_CLOSE_ALL);
      return false;
    }

    /* Header */
    Serial.print(F("HTTP/1.1 404 Not Found\r\n"));

    /* Finaliza comando de envio ao módulo WiFi */
    Serial.print("\\0");

    /* Obtém resposta e encerra conexão */
    serial_get("SEND OK\r\n", 100, NULL);
    esp.close(connection);
  }
  
  return false;
}

/************************************************************************************
  WEB_process_POST

  .

************************************************************************************/
bool WEB_process_POST(uint8_t connection, char* path, char* body, uint32_t bodySize)
{
  /* ENVIO SSID & PASSWORD & KEY */
  if(strstr(path, "/send.html"))
  {
    /* SSID */
    char* tkn = strtok(body, "=&");
    if(!strcmp(tkn, "ssid"))
    {
        tkn = strtok(NULL, "=&");
        espAp.SSID = tkn;
        //url_decode(tkn, espAp.SSID, sizeof(espAp.SSID)); /* Decodifica caracteres especiais */
        //str_safe(espAp.SSID, sizeof(espAp.SSID)); /* Torna a string 'segura' */
    }
    else
      espAp.SSID[0] = '\0';

    /* Password */
    tkn = strtok(NULL, "=&");
    if(!strcmp(tkn, "pass"))
    {
        tkn = strtok(NULL, "=&");
        espAp.password = tkn;
        //url_decode(tkn, espAp.password, sizeof(espAp.password)); /* Decodifica caracteres especiais */
        //str_safe(espAp.password, sizeof(espAp.password)); /* Torna a string 'segura' */
    }
    else
      espAp.password[0] = '\0';

    /* Key */
    tkn = strtok(NULL, "=&");
    if(!strcmp(tkn, "key"))
    {
        tkn = strtok(NULL, "=&");
        url_decode(tkn, espUrl.key, sizeof(espUrl.key)); /* Decodifica caracteres especiais */
        str_safe(espUrl.key, sizeof(espUrl.key)); /* Torna a string 'segura' */
    }
    else
      espUrl.key[0] = '\0';

    if (espAp.SSID.length() && espAp.password.length() && strlen(espUrl.key))
    {
      lcd.clear();
      lcd.print(F("ESP CLIENT:")); lcd.setCursor(0, 1);
      lcd.print(F("GOT NEW WIFI"));
      delay(1000);
      
      /* Salva AP na EEPROM */
      if(EEPROM_write_AP(espAp, espUrl))
      {
        lcd.clear();
        lcd.print(F("EEPROM SAVED:")); lcd.setCursor(0, 1);
        lcd.print(F("AP"));
        delay(1000);

        lcd.clear();
        lcd.print(F("SSID: ")); lcd.print(espAp.SSID); lcd.setCursor(0, 1);
        lcd.print(F("pass: ")); lcd.print(espAp.password);
        delay(1000);

        lcd.clear();
        lcd.print(F("Key: ")); lcd.setCursor(0, 1);
        lcd.print(espUrl.key);
        delay(1000);
      }

      /* ESP8266: Inicializar envio */
      serial_flush();
      Serial.print(F("AT+CIPSENDEX="));
      Serial.print(connection);
      Serial.print(F(",2047\r\n"));
      if(!serial_get(">", 100, NULL))                  /* Aguarda '>' */
      {
          esp.close(WIFI_CLOSE_ALL);
          return false;
      }
  
      /* HTTP HEADERS */  
      serial_flush();
      Serial.print(F("HTTP/1.1 200 OK\r\n"));
      /* Hosts */
      Serial.print(F("Host: 192.168.1.4\r\n"));
      /* Connection */
      Serial.print(F("Connection: Close\r\n"));
      /* Content-Length */
      Serial.print(F("Content-Length: "));
      Serial.print((strlen(HTML_PAGE_SUCCESS)));
      Serial.print(F("\r\n"));
      /* Header End */
      Serial.print(F("\r\n"));
  
      /* Envia página HTML da PROGMEM */
      Serial.print((__FlashStringHelper *) HTML_PAGE_SUCCESS);
  
      /* AT: End Send */
      Serial.write("\\0");

      /* Return */
      serial_get((char*) "SEND OK", 1000, NULL);
      if(!serial_get((char*) "CLOSED\r\n", 1000, NULL))
      {
          esp.close(WIFI_CLOSE_ALL);
          return false;
      }
      return true;
    }
    else
    {
      lcd.clear();
      lcd.print(F("ESP CLIENT:")); lcd.setCursor(0, 1);
      lcd.print(F("WRONG INPUT"));

      /* ESP8266: Inicializar envio */
      serial_flush();
      Serial.print(F("AT+CIPSENDEX="));
      Serial.print(connection);
      Serial.print(F(",2047\r\n"));
      if(!serial_get(">", 100, NULL))                  /* Aguarda '>' */
      {
          esp.close(WIFI_CLOSE_ALL);
          return false;
      }
  
      /* HTTP HEADERS */  
      serial_flush();
      Serial.print(F("HTTP/1.1 200 OK\r\n"));
      /* Hosts */
      Serial.print(F("Host: 192.168.1.4\r\n"));
      /* Connection */
      Serial.print(F("Connection: Close\r\n"));
      /* Content-Length */
      Serial.print(F("Content-Length: "));
      Serial.print((strlen(HTML_PAGE_FAIL)));
      Serial.print(F("\r\n"));
      /* Header End */
      Serial.print(F("\r\n"));
  
      /* Envia página HTML da PROGMEM */
      Serial.print((__FlashStringHelper *) HTML_PAGE_FAIL);
  
      /* AT: End Send */
      Serial.write("\\0");

      /* Return */
      serial_get((char*) "SEND OK", 1000, NULL);
      if(!serial_get((char*) "CLOSED\r\n", 1000, NULL))
      {
          esp.close(WIFI_CLOSE_ALL);
          return false;
      }
      return false;
    }
  }
  /* 404 - NOT FOUND */
  else
  {
    /* ESP8266: Inicializar envio */
    serial_flush();
    Serial.print("AT+CIPSENDEX=");    /* AT: Send command */
    Serial.print(connection);
    Serial.print(",2047\r\n");
    if (!serial_get(">", 100, NULL))                 /* Aguarda '>' */
    {
      esp.close(WIFI_CLOSE_ALL);
      return false;
    }

    /* Header */
    Serial.print(F("HTTP/1.1 404 Not Found\r\n"));

    /* Finaliza comando de envio ao módulo WiFi */
    Serial.print("\\0");

    /* Obtém resposta e encerra conexão */
    serial_get("SEND OK\r\n", 100, NULL);
    esp.close(connection);
  }

  return false;
}


/************************************************************************************
  EEPROM_write_AP

  This function flushes the serial buffer.

************************************************************************************/
bool EEPROM_write_AP(ESP8266::esp_AP_parameter_t &ap, ESP8266::esp_URL_parameter_t &url)
{
  uint16_t addr = 0;

  /* Verifica se possui ap e key */
  if (!ap.SSID.length() || !ap.password.length() || !strlen(url.key))
    return false;

  /* Indicador de início */
  EEPROM.write(addr, 0xFE);
  addr++;

  /* SSID */
  for (uint16_t i = 0; i < ap.SSID.length(); i++)
  {
    EEPROM.write(addr, ap.SSID[i]);
    addr++;
  }
  /* NULL char */
  EEPROM.write(addr, '\0');
  addr++;

  /* Password */
  for (uint16_t i = 0; i < ap.password.length(); i++)
  {
    EEPROM.write(addr, ap.password[i]);
    addr++;
  }
  /* NULL char */
  EEPROM.write(addr, '\0');
  addr++;

  /* Key */
  for (uint16_t i = 0; i < strlen(url.key); i++)
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
  EEPROM_read_AP

  This function flushes the serial buffer.

************************************************************************************/
bool EEPROM_read_AP(ESP8266::esp_AP_parameter_t &ap, ESP8266::esp_URL_parameter_t &url)
{
  uint16_t addr = 0;
  uint8_t i = 0;
  char strBuffer[50];

  /* Indicador de início */
  if (EEPROM.read(addr) != 0xFE)
    return false;
  addr++;

  /* SSID */
  while (true)
  {
    char u8byte = EEPROM.read(addr);
    addr++;

    if (u8byte)
    {
      strBuffer[i] = u8byte;
      i++;
    }
    else
    {
      strBuffer[i] = '\0';
      i = 0;
      ap.SSID = String(strBuffer);
      break;
    }
  }
  /* Password */
  while (true)
  {
    char u8byte = EEPROM.read(addr);
    addr++;

    if (u8byte)
    {
      strBuffer[i] = u8byte;
      i++;
    }
    else
    {
      strBuffer[i] = '\0';
      i = 0;
      ap.password = String(strBuffer);
      break;
    }
  }
  /* Key */
  while (true)
  {
    char u8byte = EEPROM.read(addr);
    addr++;

    if (u8byte)
    {
      strBuffer[i] = u8byte;
      i++;
    }
    else
    {
      strBuffer[i] = '\0';
      i = 0;
      strncpy(url.key, strBuffer, sizeof(url.key));
      url.key[sizeof(url.key) - 1] = '\0';
      break;
    }
  }

  return true;
}


/************************************************************************************
  serial_flush

  This function flushes the serial buffer.

************************************************************************************/
void serial_flush(void)
{
  /* Obt�m bytes dispon�veis */
  while (Serial.available())
    Serial.read();

  return;
}

/************************************************************************************
  serial_get

  This function gets messages from serial, during a certain timeout.

************************************************************************************/
bool serial_get(const char* stringChecked, uint32_t timeout, char* serialBuffer)
{
  char* u8buffer;
  char* u8bufferInit;
  char u8tempBuffer[50];
  uint32_t u32counter = 0;

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

  while (true)
  {
    /* Enquanto houver dados no buffer, receber e comparar com a string a ser checada */
    while (Serial.available())
    {
      char inChar = (char)Serial.read();
      *u8buffer = inChar;  u8buffer++;
      *u8buffer = '\0';

      if (strstr(u8bufferInit, stringChecked))
        return true;

      if (strlen(u8bufferInit) >= (sizeof(u8tempBuffer) - 1))
        u8buffer = u8bufferInit;
    }

    /* Incrementa contador  de timeout */
    u32counter++;
    if (u32counter >= timeout * 1000)
      return false;
    delayMicroseconds(1);
  }
}

/******************************************************************************/

/* Converts a hex character to its integer value */
char from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character*/
char to_hex(char code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

/* Returns a url-encoded version of str */
void url_encode(char *str, char* encoded) {
    char *pstr = str, *pbuf = encoded;
    while (*pstr) {
        if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~')
            *pbuf++ = *pstr;
        else if (*pstr == ' ')
            *pbuf++ = '+';
        else
            *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
        pstr++;
    }
    *pbuf = '\0';
    return;
}

/* Returns a url-decoded version of str */
void url_decode(char *str, char* decoded, int size)
{
    char *pstr = str, *pbuf = decoded;
    int count = 0;

    while(*pstr)
    {
        /* Verifica se atingiu tamanho máximo */
        /* Sempre finaliza string com '\0' */
        count++;
        if((count + 1) > size)
            break;

        /* Codificado em HEX */
        if(*pstr == '%')
        {
            if(pstr[1] && pstr[2])
            {
                *pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
                pstr += 2;
            }
        }
        /* Espaço */
        else if(*pstr == '+')
        {
            *pbuf++ = ' ';
        }
        /* Demais caracteres */
        else
        {
            *pbuf++ = *pstr;
        }
        pstr++;
    }
    *pbuf = '\0';
    return;
}

/* Returns a safe version of str */
void str_safe(char *str, uint32_t size)
{
    /* Substitui caracteres não permitidos */
    for(uint32_t i=0; i<size; i++)
    {
        if(str[i] == '\0')
            break;
        else if(str[i] < ' ')
            str[i] = ' ';
        else if(str[i] > '~')
            str[i] = ' ';
        else if(str[i] == '"' || str[i] == '\'' || str[i] == '&' || str[i] == '\\')
            str[i] = ' ';
    }
}
