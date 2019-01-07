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
#define SCALE_2             30000.0/1000.0   /* Relação do TC */
#define DATA_SIZE         500            /* Quantidade de amostras */

/* Chave bipolar */
#define SWT1        3
#define SWT2        4

/* Salva páginas HTML na memória de programa */
const char HTML_PAGE_INDEX[] PROGMEM = "<!DOCTYPE html><html><head> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"> <link rel=\"icon\" href=\"favicon.ico\" type=\"image/x-icon\"> <h2 style=\"text-align:center;\"><span style=\"font-family:arial;\">Menu Principal</span></h2></head><body> <h3>Configura&ccedil;&otilde;es B&aacute;sicas:</h3> <div class=\"panel\"> <form action=\"wifi.html\"> <input class=\"btn btn-info\" type=\"submit\" value=\"Rede Wi-Fi\"> </form> <form action=\"servers.html\"> <input class=\"btn btn-info\" type=\"submit\" value=\"Servidor de destino\"> </form> </div> <br></body>";
const char HTML_PAGE_WIFI[] PROGMEM = "<!DOCTYPE html><html><head> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"> <link rel=\"icon\" href=\"favicon.ico\" type=\"image/x-icon\"> <h2 style=\"text-align:center;\"><span style=\"font-family:arial;\">Rede Wi-Fi</span></h2></head><body><div class=\"panel\"><form action=\"wifi.html\" method=\"post\"><label><b>Nome da rede:</b><input type=\"text\" name=\"ssid\" placeholder=\"rede\" maxlength=\"32\" required></label><br><label><b>Senha da rede:</b><input type=\"password\" name=\"pass\" placeholder=\"senha\" maxlength=\"32\" required></label><br><input class=\"btn btn-success\" type=\"submit\" value=\"Enviar\"></form></div><form action=\"index.html\"><input class=\"btn btn-success\" type=\"submit\" value=\"Sair\"></form><form action=\"wifi.html\"><input class=\"btn btn-info\" type=\"submit\" value=\"Recarregar\" /></form></body>";
const char HTML_PAGE_SERVERS[] PROGMEM = "<!DOCTYPE html><html><head> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"> <link rel=\"icon\" href=\"favicon.ico\" type=\"image/x-icon\"> <h2 style=\"text-align:center;\"><span style=\"font-family:arial;\">Servidor de destino</span></h2></head><body> <div class=\"panel\"> <h3>Servidor Principal</h3> <form action=\"servers.html\" method=\"post\"> <label><b>Servidor:</b><input type=\"text\" name=\"host\" placeholder=\"host.com\" maxlength=\"49\" value=\"api.thingspeak.com\" required></label> <label><b>Caminho:</b><input type=\"text\" name=\"path\" placeholder=\"/path\" maxlength=\"49\" value=\"/update\" required></label> <label><b>Porta:</b><input type=\"number\" name=\"port\" placeholder=\"80\" min=\"0\" max=\"65535\" value=\"80\" required></label><label><b>Chave de acesso:</b><input type=\"text\" name=\"key\" placeholder=\"key\" maxlength=\"32\" value=\"0102030405060708\" required></label> <input class=\"btn btn-success\" type=\"submit\" value=\"Salvar\"> </form> </div> <br> <form action=\"index.html\"> <input class=\"btn btn-success\" type=\"submit\" value=\"Sair\"> </form><form action=\"servers.html\"><input class=\"btn btn-info\" type=\"submit\" value=\"Recarregar\" /></form></body>";
const char HTML_PAGE_STATUS[] PROGMEM = "<script> function myFunction(timeout) { setTimeout(showPage, timeout); } function showPage() { document.getElementById(\"loader\").style.display = \"none\"; document.getElementById(\"status\").style.display = \"block\"; }</script><style> #loader { margin: auto; border: 16px solid #f2f2f2; border-radius: 50%; border-top: 16px solid #5BC0DE; border-left: 16px solid #5BC0DE; border-bottom: 16px solid #5BC0DE; width: 120px; height: 120px; -webkit-animation: spin 2s linear infinite; animation: spin 2s linear infinite; } @-webkit-keyframes spin { 0% { -webkit-transform: rotate(0deg); } 100% { -webkit-transform: rotate(360deg); } } @keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } } .animate-bottom { position: relative; -webkit-animation-name: animatebottom; -webkit-animation-duration: 1s; animation-name: animatebottom; animation-duration: 1s } @-webkit-keyframes animatebottom { from { bottom: -100px; opacity: 0 } to { bottom: 0px; opacity: 1 } } @keyframes animatebottom { from { bottom: -100px; opacity: 0 } to { bottom: 0; opacity: 1 } }</style><!DOCTYPE html><html><head> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"> <link rel=\"icon\" href=\"favicon.ico\" type=\"image/x-icon\"> <h2 style=\"text-align:center;\"><span style=\"font-family:arial;\">Status</span></h2></head><body onload=\"myFunction(100)\"> <div id=\"loader\"></div> <div style=\"display:none;\" id=\"status\" class=\"animate-bottom\">";
const char HTML_PAGE_SUCCESS[] PROGMEM = "<div class=\"panel-green\"><h3><span style=\"color: #4CAF50;font-family:arial;\"><b>Configura&ccedil;&atilde;o efetuada com sucesso.</b></span></h3></div><br><form action=\"index.html\"><input class=\"btn btn-success\" type=\"submit\" value=\"Voltar\"></form> </div></body>";
const char HTML_PAGE_FAIL[] PROGMEM = "<div class=\"panel-red\"><h3><span style=\"color: #D9534F;font-family:arial;\"><b>Erro ao efetuar configura&ccedil;&atilde;o. Por favor, tente novamente.</b></span></h3></div><br><form action=\"index.html\"><input class=\"btn btn-success\" type=\"submit\" value=\"Voltar\"></form> </div></body>";
const char HTML_PAGE_CSS[] PROGMEM = "<style>.btn { width: 100%; color: white; padding: 14px 20px; margin: 8px 0; border: none; border-radius: 4px; cursor: pointer;}a.btn{ padding: 12px 20px; margin: 8px 0; display: inline-block; border: none; border-radius: 4px; box-sizing: border-box;text-align: center;text-decoration: none;font: 400 12px Arial;}.btn-success { background-color: #4CAF50;}.btn-info { background-color: #5BC0DE;}.btn-warning { background-color: #EC971F;}.btn-danger { background-color: #D9534F;}input[type=text],input[type=password],input[type=number],select { width: 100%; padding: 12px 20px; margin: 8px 0; display: inline-block; border: 2px solid #ccc; border-radius: 4px; box-sizing: border-box;}body {font-family: arial;}.panel{ border-radius: 5px; background-color: #f2f2f2; padding: 20px;}.panel-green{border-radius: 5px; background-color: #C5EAC6;padding: 20px;}.panel-red{border-radius: 5px; background-color: #F2C0BF;padding: 20px;}input.checkboxLrg{width: 25px;height: 25px;}input.checkboxMed{width: 20px;height: 20px;}</style>";
const char HTML_PAGE_FOOTER[] PROGMEM = "<footer> <br> <br>Vers&atilde;o do WEB firmware: v0.1.0 build 07/01/19 <br>Desenvolvido por <a href=\"mailto:gianzanuz@hotmail.com\">Giancarlo Zanuz</a> <br></footer></html>";

/*************************************************************************************
  Private variables
*************************************************************************************/
/* Módulo WiFi ESP8266 */
/* Pino de Enable = 2 */
ESP8266 esp(ESP_ENABLE_PIN);
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

/* Soft-reset */
bool resetFlag = false;

/*************************************************************************************
  Public prototypes
*************************************************************************************/
bool IOT_send_GET(const char* path, const char* query, const char* host);

void WEB_init(void);
bool WEB_process_GET(uint8_t connection, char* path, char* parameters, uint32_t parametersSize);
bool WEB_process_POST(uint8_t connection, char* path, char* body, uint32_t bodySize);

bool WEB_chunk_init(uint8_t connection);
void WEB_chunk_send(char* chuck);
bool WEB_chunk_finish(void);
bool WEB_headers(uint8_t connection);

void serial_flush(void);
bool serial_get(const char* stringChecked, uint32_t timeout, char* serialBuffer, uint16_t serialBufferSize);

/* Converts a hex character to its integer value */
char from_hex(char ch);
char to_hex(char code);
void url_encode(char *str, char* encoded);
void url_decode(char *str, char* decoded, int size);
void str_safe(char *str, uint32_t size);

/* Soft Reset */
void(* softReset) (void) = 0;

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

  /* Configuração inicial do ESP */
  if(!esp.config(ESP8266::ESP_CLIENT_AND_SERVER_MODE))
  {
    lcd.clear();
    lcd.print(F("ESP CONFIG:")); lcd.setCursor(0, 1);
    lcd.print(F("ERROR"));

    /* Aplica um soft-reset após delay */
    delay(60000);
    softReset();
  }
  else
  {
    lcd.clear();
    lcd.print(F("ESP CONFIG:")); lcd.setCursor(0, 1);
    lcd.print(F("OK"));
    delay(1000);
  }

  /* Configurações do Soft Acess Point */
  strncpy(espAp.ssid, ESP_SERVER_SSID, sizeof(espAp.ssid));
  espAp.ssid[sizeof(espAp.ssid) - 1] = '\0';
  strncpy(espAp.password, ESP_SERVER_PASSWORD, sizeof(espAp.password));
  espAp.password[sizeof(espAp.password) - 1] = '\0';

  /* Ajuste do Soft Access Point */
  if(!esp.setAP(ESP8266::ESP_SERVER_MODE, espAp))
  {
    lcd.clear();
    lcd.print(F("SET AP:")); lcd.setCursor(0, 1);
    lcd.print(F("ERROR"));

    /* Aplica um soft-reset após delay */
    delay(60000);
    softReset();
  }
  else
  {
    lcd.clear();
    lcd.print(F("SET AP:")); lcd.setCursor(0, 1);
    lcd.print(F("OK"));
    delay(1000);
  }

  /* Configurações servidor */
  strncpy(espUrl.host, ESP_SERVER_IP_DEFAULT, sizeof(espUrl.host));
  espUrl.host[sizeof(espUrl.host) - 1] = '\0';
  strncpy(espUrl.path, ESP_SERVER_PATH_DEFAULT, sizeof(espUrl.path));
  espUrl.path[sizeof(espUrl.path) - 1] = '\0';
  espUrl.port     = ESP_SERVER_PORT_DEFAULT;
  espUrl.key[0]   = '\0';

  /* Inicializa servidor */
  if(!esp.server(espUrl))
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
    delay(1000);
  }
    
  /* Configurações cliente */
  strncpy(espAp.ssid, ESP_CLIENT_SSID, sizeof(espAp.ssid));
  espAp.ssid[sizeof(espAp.ssid) - 1] = '\0';
  strncpy(espAp.password, ESP_CLIENT_PASSWORD, sizeof(espAp.password));
  espAp.password[sizeof(espAp.password) - 1] = '\0';

  strncpy(espUrl.host, ESP_CLIENT_IP_DEFAULT, sizeof(espUrl.host));
  espUrl.host[sizeof(espUrl.host) - 1] = '\0';
  strncpy(espUrl.path, ESP_CLIENT_PATH_DEFAULT, sizeof(espUrl.path));
  espUrl.path[sizeof(espUrl.path) - 1] = '\0';
  espUrl.port   = ESP_CLIENT_PORT_DEFAULT;
  espUrl.key[0] = '\0';

  /* Obtém AP da EEPROM, caso haja */
  if(EEPROM_read_AP(espAp))
  {
    lcd.clear();
    lcd.print(F("EEPROM FOUND:")); lcd.setCursor(0, 1);
    lcd.print(F("AP"));
    delay(1000);

    lcd.clear();
    lcd.print(F("SSID: ")); lcd.print(espAp.ssid); lcd.setCursor(0, 1);
    lcd.print(F("pass: ")); lcd.print(espAp.password);
    delay(1000);

    /* Aplica novo AP recebido */
    esp.setAP(ESP8266::ESP_CLIENT_MODE, espAp);
  }
  else
  {
    lcd.clear();
    lcd.print(F("EEPROM NOT FOUND:")); lcd.setCursor(0, 1);
    lcd.print(F("AP"));
    delay(1000);
  }

  /* Obtém URL da EEPROM, caso haja */
  if(EEPROM_read_URL(espUrl))
  {
    lcd.clear();
    lcd.print(F("EEPROM FOUND:")); lcd.setCursor(0, 1);
    lcd.print(F("SERVER"));
    delay(1000);

    lcd.clear();
    lcd.print(F("host: ")); lcd.print(espUrl.host); lcd.setCursor(0, 1);
    lcd.print(F("path: ")); lcd.print(espUrl.path);
    delay(1000);

    lcd.clear();
    lcd.print(F("key: ")); lcd.print(espUrl.key); lcd.setCursor(0, 1);
    lcd.print(F("port: ")); lcd.print(espUrl.port);
    delay(1000);
  }
  else
  {
    lcd.clear();
    lcd.print(F("EEPROM NOT FOUND:")); lcd.setCursor(0, 1);
    lcd.print(F("SERVER"));
    delay(1000);
  }
}

void loop()
{
  static uint32_t previousMillis = 0;
  uint32_t currentMillis = millis();
  if((uint32_t) (currentMillis - previousMillis) > (uint32_t) ESP_SLEEP_TIMEOUT * 60 * 1000)
  {
    previousMillis = currentMillis;
    IOT_measure();
    serial_flush();
  }

  /* Verifica se houve conexão ao servidor do ESP8266 */
  if(Serial.available())
    WEB_init();

  /* Realiza o soft-reset */
  if(resetFlag)
    softReset();
}

/************************************************************************************
  IOT_measure

  .

************************************************************************************/
bool IOT_measure(void)
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
    if(esp.checkWifi(10, 1000, espAp))
    {
      lcd.clear();
      lcd.print(F("ESP CONNECT AP:")); lcd.setCursor(0, 1);
      lcd.print(F("OK"));

      if(esp.connect(espUrl))
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

          esp.close(WIFI_CLOSE_ALL);
        }
        else
        {
          lcd.clear();
          lcd.print(F("ESP SEND:")); lcd.setCursor(0, 1);
          lcd.print(F("ERROR"));
          
          esp.close(WIFI_CLOSE_ALL);
          return false;
        }
      }
      else
      {
        lcd.clear();
        lcd.print(F("ESP CONNECT:")); lcd.setCursor(0, 1);
        lcd.print(F("ERROR"));

        esp.close(WIFI_CLOSE_ALL);
        return false;
      }
    }
    else
    {
      lcd.clear();
      lcd.print(F("ESP CONNECT AP:")); lcd.setCursor(0, 1);
      lcd.print(F("ERROR"));
      return false;
    }

    return true;
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
    if(!serial_get(">", 100, NULL, 0))                  /* Aguarda '>' */
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
    Serial.print(F("Connection: Close\r\n"));
    /* Header End */
    Serial.print(F("\r\n"));
    
    /* AT: End Send */
    Serial.write("\\0");

    /* Return */
    serial_get((char*) "SEND OK", 1000, NULL, 0);
    if(!serial_get((char*) "CLOSED\r\n", 1000, NULL, 0))
    {
        esp.close(WIFI_CLOSE_ALL);
        return false;
    }
    return true;
}

/************************************************************************************
  WEB_init

  .

************************************************************************************/
void WEB_init()
{
  char strBuffer[250];
  char path[25];
  uint8_t connection;

  /* Obtém os parâmetros da requisição feita pelo browser */
  if(!serial_get((char*) "HTTP/1.1\r\n", 500, strBuffer, sizeof(strBuffer)))
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
    while(serial_get("\r\n", 100, strBuffer, sizeof(strBuffer))) {};

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
    if (!serial_get(">", 100, NULL, 0))                 /* Aguarda '>' */
    {
      esp.close(WIFI_CLOSE_ALL);
      return;
    }

    /* Header */
    Serial.print(F("HTTP/1.1 405 Method Not Allowed\r\n"));

    /* Finaliza comando de envio ao módulo WiFi */
    Serial.print("\\0");

    /* Obtém resposta e encerra conexão */
    serial_get("SEND OK\r\n", 100, NULL, 0);
    esp.close(connection);
  }

  serial_flush();
  return;
}


/************************************************************************************
  WEB_process_GET

  .

************************************************************************************/
bool WEB_process_GET(uint8_t connection, char* path, char* parameter, uint32_t parameterSize)
{
  lcd.clear();
  lcd.print(F("ESP SERVER:")); lcd.setCursor(0, 1);
  lcd.print(F("GET"));

  /* INDEX */
  if(!strcmp(path, "/") || !strcmp(path, "/index.html"))
  {
    /* ESP8266: Inicializar envio */
    if(!WEB_headers(connection))
      return false;

    /* Inicializa envio do chunks */
    if(!WEB_chunk_init(connection))
      return false;

    /* Envia página HTML da PROGMEM */
    Serial.println(strlen(HTML_PAGE_INDEX), HEX);            /* CHUNCK SIZE */
    Serial.println((__FlashStringHelper *) HTML_PAGE_INDEX);

    /* Envia página HTML da PROGMEM */
    Serial.println(strlen(HTML_PAGE_FOOTER), HEX);            /* CHUNCK SIZE */
    Serial.println((__FlashStringHelper *) HTML_PAGE_FOOTER);

    /* End chunk */
    Serial.write("0\r\n\r\n");
    return WEB_chunk_finish();
  }
  /* WIFI */
  else if(!strcmp(path, "/wifi.html"))
  {
    /* ESP8266: Inicializar envio */
    if(!WEB_headers(connection))
      return false;

    /* INICIALIZA ENVIO DO CHUNK (WIFI + FOOTER) */
    if(!WEB_chunk_init(connection))
      return false;

    /* ENVIO DO CHUNCK (WIFI) */
    Serial.println(strlen(HTML_PAGE_WIFI), HEX);            /* CHUNCK SIZE */
    Serial.println((__FlashStringHelper *) HTML_PAGE_WIFI);

    /* ENVIO DO CHUNCK (FOOTER) */
    Serial.println(strlen(HTML_PAGE_FOOTER), HEX);            /* CHUNCK SIZE */
    Serial.println((__FlashStringHelper *) HTML_PAGE_FOOTER);

    /* FINALIZA ÚLTIMO ENVIO (WIFI + FOOTER) */
    Serial.write("0\r\n\r\n");
    return WEB_chunk_finish();
  }
  /* SERVERS */
  else if(!strcmp(path, "/servers.html"))
  {
    /* ESP8266: Inicializar envio */
    if(!WEB_headers(connection))
      return false;

    /* INICIALIZA ENVIO DO CHUNK (SERVERS + FOOTER) */
    if(!WEB_chunk_init(connection))
      return false;

    /* ENVIO DO CHUNCK (SERVERS) */
    Serial.println(strlen(HTML_PAGE_SERVERS), HEX);            /* CHUNCK SIZE */
    Serial.println((__FlashStringHelper *) HTML_PAGE_SERVERS);

    /* ENVIO DO CHUNCK (FOOTER) */
    Serial.println(strlen(HTML_PAGE_FOOTER), HEX);            /* CHUNCK SIZE */
    Serial.println((__FlashStringHelper *) HTML_PAGE_FOOTER);

    /* FINALIZA ÚLTIMO ENVIO (SERVERS + FOOTER) */
    Serial.write("0\r\n\r\n");
    return WEB_chunk_finish();
  }
  /* 404 - NOT FOUND */
  else
  {
    /* ESP8266: Inicializar envio */
    serial_flush();
    Serial.print("AT+CIPSENDEX=");    /* AT: Send command */
    Serial.print(connection);
    Serial.print(",2047\r\n");
    if (!serial_get(">", 100, NULL, 0))                 /* Aguarda '>' */
    {
      esp.close(WIFI_CLOSE_ALL);
      return false;
    }

    /* Header */
    Serial.print(F("HTTP/1.1 404 Not Found\r\n"));

    /* Finaliza comando de envio ao módulo WiFi */
    Serial.print("\\0");

    /* Obtém resposta e encerra conexão */
    serial_get("SEND OK\r\n", 100, NULL, 0);
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
  lcd.clear();
  lcd.print(F("ESP SERVER:")); lcd.setCursor(0, 1);
  lcd.print(F("POST"));
  
  /* WIFI */
  if(strstr(path, "/wifi.html"))
  {
    /* ESP8266: Inicializar envio */
    if(!WEB_headers(connection))
      return false;

    /* INICIALIZA ENVIO DO CHUNK (STATUS) */
    if(!WEB_chunk_init(connection))
      return false;

    /* ENVIO DO CHUNCK (STATUS) */
    Serial.println(strlen(HTML_PAGE_STATUS), HEX);            /* CHUNCK SIZE */
    Serial.println((__FlashStringHelper *) HTML_PAGE_STATUS);

    /* FINALIZA ENVIO DO CHUNK (STATUS) */
    if(!WEB_chunk_finish())
      return false;
    
    /* SSID */
    char* tkn = strtok(body, "=&");
    if(!strcmp(tkn, "ssid"))
    {
        tkn = strtok(NULL, "=&");
        url_decode(tkn, espAp.ssid, sizeof(espAp.ssid)); /* Decodifica caracteres especiais */
        str_safe(espAp.ssid, sizeof(espAp.ssid)); /* Torna a string 'segura' */
    }
    else
      espAp.ssid[0] = '\0';

    /* Password */
    tkn = strtok(NULL, "=&");
    if(!strcmp(tkn, "pass"))
    {
        tkn = strtok(NULL, "=&");
        url_decode(tkn, espAp.password, sizeof(espAp.password)); /* Decodifica caracteres especiais */
        str_safe(espAp.password, sizeof(espAp.password)); /* Torna a string 'segura' */
    }
    else
      espAp.password[0] = '\0';

    if(strlen(espAp.ssid) && strlen(espAp.password))
    {
      lcd.clear();
      lcd.print(F("ESP SERVER:")); lcd.setCursor(0, 1);
      lcd.print(F("GOT NEW WIFI"));
      delay(1000);
      
      /* Salva AP na EEPROM */
      if(EEPROM_write_AP(espAp))
      {
        lcd.clear();
        lcd.print(F("EEPROM SAVED:")); lcd.setCursor(0, 1);
        lcd.print(F("AP"));
        delay(1000);

        lcd.clear();
        lcd.print(F("SSID: ")); lcd.print(espAp.ssid); lcd.setCursor(0, 1);
        lcd.print(F("pass: ")); lcd.print(espAp.password);
        delay(1000);

        /* Ativa flag de reset */
        resetFlag = true;
      }

      /* INICIALIZA ENVIO DO CHUNK (SUCCESS) */
      if(!WEB_chunk_init(connection))
        return false;

      /* ENVIO DO CHUNCK (SUCCESS) */
      Serial.println(strlen(HTML_PAGE_SUCCESS), HEX);            /* CHUNCK SIZE */
      Serial.println((__FlashStringHelper *) HTML_PAGE_SUCCESS);
    }
    else
    {
      lcd.clear();
      lcd.print(F("ESP SERVER:")); lcd.setCursor(0, 1);
      lcd.print(F("WRONG INPUT"));
      delay(1000);

      /* INICIALIZA ENVIO DO CHUNK (FAIL) */
      if(!WEB_chunk_init(connection))
        return false;

      /* ENVIO DO CHUNCK (FAIL) */
      Serial.println(strlen(HTML_PAGE_FAIL), HEX);            /* CHUNCK SIZE */
      Serial.println((__FlashStringHelper *) HTML_PAGE_FAIL);
    }

    /* ENVIO DO CHUNCK (FOOTER) */
    Serial.println(strlen(HTML_PAGE_FOOTER), HEX);            /* CHUNCK SIZE */
    Serial.println((__FlashStringHelper *) HTML_PAGE_FOOTER);

    /* FINALIZA ÚLTIMO ENVIO (SUCCESS/FAIL + FOOTER) */
    Serial.write("0\r\n\r\n");
    return WEB_chunk_finish();
  }
  /* SERVERS */
  if(strstr(path, "/servers.html"))
  {
    /* ESP8266: Inicializar envio */
    if(!WEB_headers(connection))
      return false;

    /* INICIALIZA ENVIO DO CHUNK (STATUS) */
    if(!WEB_chunk_init(connection))
      return false;

    /* ENVIO DO CHUNCK (STATUS) */
    Serial.println(strlen(HTML_PAGE_STATUS), HEX);            /* CHUNCK SIZE */
    Serial.println((__FlashStringHelper *) HTML_PAGE_STATUS);

    /* FINALIZA ENVIO DO CHUNK (STATUS) */
    if(!WEB_chunk_finish())
      return false;
      
    /* host */
    char* tkn = strtok(body, "=&");
    if(!strcmp(tkn, "host"))
    {
        tkn = strtok(NULL, "=&");
        url_decode(tkn, espUrl.host, sizeof(espUrl.host)); /* Decodifica caracteres especiais */
        str_safe(espUrl.host, sizeof(espUrl.host)); /* Torna a string 'segura' */
    }
    else
      espUrl.host[0] = '\0';

    /* path */
    tkn = strtok(NULL, "=&");
    if(!strcmp(tkn, "path"))
    {
        tkn = strtok(NULL, "=&");
        url_decode(tkn, espUrl.path, sizeof(espUrl.path)); /* Decodifica caracteres especiais */
        str_safe(espUrl.path, sizeof(espUrl.path)); /* Torna a string 'segura' */
    }
    else
      espUrl.path[0] = '\0';

    /* port */
    tkn = strtok(NULL, "=&");
    if(!strcmp(tkn, "port"))
    {
        tkn = strtok(NULL, "=&");
        espUrl.port = (uint16_t) atoi(tkn);
    }
    else
      espUrl.port = 0;

    /* key */
    tkn = strtok(NULL, "=&");
    if(!strcmp(tkn, "key"))
    {
        tkn = strtok(NULL, "=&");
        url_decode(tkn, espUrl.key, sizeof(espUrl.key)); /* Decodifica caracteres especiais */
        str_safe(espUrl.key, sizeof(espUrl.key)); /* Torna a string 'segura' */
    }
    else
      espUrl.key[0] = '\0';

    if(strlen(espUrl.host) && strlen(espUrl.path) && strlen(espUrl.key) && espUrl.port)
    {
      lcd.clear();
      lcd.print(F("ESP SERVER:")); lcd.setCursor(0, 1);
      lcd.print(F("GOT NEW SERVER"));
      delay(1000);
      
      /* Salva URL na EEPROM */
      if(EEPROM_write_URL(espUrl))
      {
        lcd.clear();
        lcd.print(F("EEPROM SAVED:")); lcd.setCursor(0, 1);
        lcd.print(F("SERVER"));
        delay(1000);

        lcd.clear();
        lcd.print(F("host: ")); lcd.print(espUrl.host); lcd.setCursor(0, 1);
        lcd.print(F("path: ")); lcd.print(espUrl.path);
        delay(1000);

        lcd.clear();
        lcd.print(F("port: ")); lcd.print(espUrl.port); lcd.setCursor(0, 1);
        lcd.print(F("key: ")); lcd.print(espUrl.key);
        delay(1000);
      }

      /* INICIALIZA ENVIO DO CHUNK (SUCCESS) */
      if(!WEB_chunk_init(connection))
        return false;

      /* ENVIO DO CHUNCK (SUCCESS) */
      Serial.println(strlen(HTML_PAGE_SUCCESS), HEX);            /* CHUNCK SIZE */
      Serial.println((__FlashStringHelper *) HTML_PAGE_SUCCESS);
    }
    else
    {
      lcd.clear();
      lcd.print(F("ESP SERVER:")); lcd.setCursor(0, 1);
      lcd.print(F("WRONG INPUT"));
      delay(1000);

      /* INICIALIZA ENVIO DO CHUNK (FAIL) */
      if(!WEB_chunk_init(connection))
        return false;

      /* ENVIO DO CHUNCK (FAIL) */
      Serial.println(strlen(HTML_PAGE_FAIL), HEX);            /* CHUNCK SIZE */
      Serial.println((__FlashStringHelper *) HTML_PAGE_FAIL);
    }

    /* ENVIO DO CHUNCK (FOOTER) */
    Serial.println(strlen(HTML_PAGE_FOOTER), HEX);            /* CHUNCK SIZE */
    Serial.println((__FlashStringHelper *) HTML_PAGE_FOOTER);

    /* FINALIZA ÚLTIMO ENVIO (SUCCESS/FAIL + FOOTER) */
    Serial.write("0\r\n\r\n");
    return WEB_chunk_finish();
  }
  /* 404 - NOT FOUND */
  else
  {
    /* ESP8266: Inicializar envio */
    serial_flush();
    Serial.print("AT+CIPSENDEX=");    /* AT: Send command */
    Serial.print(connection);
    Serial.print(",2047\r\n");
    if (!serial_get(">", 100, NULL, 0))                 /* Aguarda '>' */
    {
      esp.close(WIFI_CLOSE_ALL);
      return false;
    }

    /* Header */
    Serial.print(F("HTTP/1.1 404 Not Found\r\n"));

    /* Finaliza comando de envio ao módulo WiFi */
    Serial.print("\\0");

    /* Obtém resposta e encerra conexão */
    serial_get("SEND OK\r\n", 100, NULL, 0);
    esp.close(connection);
  }

  return false;
}

/*******************************************************************************
   WEB_chuck_init
****************************************************************************//**
 * @brief
 * @param
 * @return
*******************************************************************************/
bool WEB_chunk_init(uint8_t connection)
{
    /* ESP8266: Inicializar envio */
    serial_flush();
    Serial.print(F("AT+CIPSENDEX="));
    Serial.print(connection);
    Serial.print(",2047\r\n");
    if(!serial_get(">", 1000, NULL, 0))
    {
        esp.close(WIFI_CLOSE_ALL);
        return false;
    }
    return true;
}

/*******************************************************************************
   WEB_chuck_send
****************************************************************************//**
 * @brief
 * @param
 * @return
*******************************************************************************/
void WEB_chunk_send(char* chuck)
{
    /* ENVIO DO CHUNCK */
    Serial.println(strlen(chuck), HEX);            /* CHUNCK SIZE */
    Serial.println(chuck);
}

/*******************************************************************************
   WEB_chuck_finish
****************************************************************************//**
 * @brief
 * @param
 * @return
*******************************************************************************/
bool WEB_chunk_finish(void)
{
    /* Finaliza envio e obtém confirmação */
    Serial.print("\\0");
    if(!serial_get("SEND OK\r\n", 1000, NULL, 0))
    {
        esp.close(WIFI_CLOSE_ALL);
        return false;
    }
    return true;
}

/*******************************************************************************
   ESP_local_server_init
****************************************************************************//**
 * @brief
 * @param
 * @return
*******************************************************************************/
bool WEB_headers(uint8_t connection)
{
    /* ESP8266: Inicializar 1º envio */
    if(!WEB_chunk_init(connection))
        return false;

    /* HTTP HEADERS */  
    serial_flush();
    Serial.print(F("HTTP/1.1 200 OK\r\n"));
    /* Hosts */
    Serial.print(F("Host: 192.168.4.1\r\n"));
    /* Connection */
    Serial.print(F("Connection: Close\r\n"));
    /* Transfer-Encoding */
    Serial.print(F("Transfer-Encoding: Chunked\r\n"));
    /* Header End */
    Serial.print(F("\r\n"));

    /* ENVIO DO CHUNCK: CSS STYLES */
    Serial.println(strlen(HTML_PAGE_CSS), HEX);            /* CHUNCK SIZE */
    Serial.println((__FlashStringHelper *) HTML_PAGE_CSS);

    /* Finaliza 1º envio e obtém confirmação */
    if(!WEB_chunk_finish())
        return false;

    return true;
}

/************************************************************************************
  EEPROM_write_AP

  

************************************************************************************/
bool EEPROM_write_AP(ESP8266::esp_AP_parameter_t &ap)
{
  uint16_t addr = 0;

  /* Verifica se possui ap */
  if(!strlen(ap.ssid) || !strlen(ap.password))
    return false;

  /* Indicador de início */
  EEPROM.write(addr, 0xFE);
  addr++;

  /* SSID */
  for (uint16_t i = 0; i < strlen(ap.ssid); i++)
  {
    EEPROM.write(addr, ap.ssid[i]);
    addr++;
  }
  /* NULL char */
  EEPROM.write(addr, '\0');
  addr++;

  /* Password */
  for (uint16_t i = 0; i < strlen(ap.password); i++)
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
  EEPROM_write_URL

  

************************************************************************************/
bool EEPROM_write_URL(ESP8266::esp_URL_parameter_t &url)
{
  uint16_t addr = EEPROM.length()/2;

  /* Verifica se possui ap */
  if(!strlen(url.host) || !strlen(url.path) || !strlen(url.key) || !url.port)
    return false;

  /* Indicador de início */
  EEPROM.write(addr, 0xFE);
  addr++;

  /* host */
  for (uint16_t i = 0; i < strlen(url.host); i++)
  {
    EEPROM.write(addr, url.host[i]);
    addr++;
  }
  /* NULL char */
  EEPROM.write(addr, '\0');
  addr++;

  /* path */
  for (uint16_t i = 0; i < strlen(url.path); i++)
  {
    EEPROM.write(addr, url.path[i]);
    addr++;
  }
  /* NULL char */
  EEPROM.write(addr, '\0');
  addr++;

  /* key */
  for (uint16_t i = 0; i < strlen(url.key); i++)
  {
    EEPROM.write(addr, url.key[i]);
    addr++;
  }
  /* NULL char */
  EEPROM.write(addr, '\0');
  addr++;

  /* port */
  char port[10];
  sprintf(port, "%u", url.port);
  for (uint16_t i = 0; i < strlen(port); i++)
  {
    EEPROM.write(addr, port[i]);
    addr++;
  }
  /* NULL char */
  EEPROM.write(addr, '\0');
  addr++;

  return true;
}

/************************************************************************************
  EEPROM_read_AP

  

************************************************************************************/
bool EEPROM_read_AP(ESP8266::esp_AP_parameter_t &ap)
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
      strncpy(ap.ssid, strBuffer, sizeof(ap.ssid));
      ap.ssid[sizeof(ap.ssid) - 1] = '\0';
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
      strncpy(ap.password, strBuffer, sizeof(ap.password));
      ap.password[sizeof(ap.password) - 1] = '\0';
      break;
    }
  }

  return true;
}

/************************************************************************************
  EEPROM_read_URL

  

************************************************************************************/
bool EEPROM_read_URL(ESP8266::esp_URL_parameter_t &url)
{
  uint16_t addr = EEPROM.length()/2;
  uint8_t i = 0;
  char strBuffer[50];

  /* Indicador de início */
  if (EEPROM.read(addr) != 0xFE)
    return false;
  addr++;

  /* host */
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
      strncpy(url.host, strBuffer, sizeof(url.host));
      url.host[sizeof(url.host) - 1] = '\0';
      break;
    }
  }
  /* path */
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
      strncpy(url.path, strBuffer, sizeof(url.path));
      url.path[sizeof(url.path) - 1] = '\0';
      break;
    }
  }
  /* key */
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
  /* port */
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
      url.port = (uint16_t) atoi(strBuffer);
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
bool serial_get(const char* stringChecked, uint32_t timeout, char* serialBuffer, uint16_t serialBufferSize)
{
  char buffer[50];
  char* buffer_ptr;
  char* buffer_ptr_init;
  uint16_t buffer_size;
  uint32_t counter = 0;

  /* Se o ponteiro � n�o NULO, recebe endere�o de serialBuffer */
  /* Ou se for NULO, recebe endere�o de buffer local */
  if(!serialBuffer)
  {
    buffer_ptr = buffer;
    buffer_size = sizeof(buffer);
  }
  else
  {
    buffer_ptr = serialBuffer;
    buffer_size = serialBufferSize;
  }

  /* Recebe endere�o da primeira posi��o */
  /* Limpa o primeiro byte do buffer */
  buffer_ptr_init = buffer_ptr;
  *buffer_ptr_init = '\0';
  
  while(true)
  {
    /* Enquanto houver dados no buffer, receber e comparar com a string a ser checada */
    while(Serial.available())
    {
      *buffer_ptr = (char)Serial.read();

      /* Caso tenha recebido null char, subtitui por um espaço */
      if(*buffer_ptr == '\0')
          *buffer_ptr = ' ';

      /* Avança ponteiro e coloca NULL char para finalizar string */
      buffer_ptr++;
      *buffer_ptr = '\0';
      
      /* Caso tenha atingido o limite do buffer, reinicia do começo */
      /* O último byte do buffer é sempre reservado para o NULL char */
      if(buffer_ptr == (buffer_ptr_init + buffer_size) - 1)
          buffer_ptr = buffer_ptr_init;

      /* Compara com string esperada */      
      if(strstr(buffer_ptr_init, stringChecked))
        return true;
    }

    /* Incrementa contador  de timeout */
    counter++;
    if (counter >= timeout * 1000)
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
