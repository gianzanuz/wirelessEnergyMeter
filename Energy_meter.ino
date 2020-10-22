#include "ESP8266.h"
#include "ADS1115.h"
#include "CRC.h"
#include <Wire.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <math.h>
#include <avr/wdt.h>

/*************************************************************************************
  Private macros
*************************************************************************************/

/* ADC ADS1115 16bits */
#define SCALE_1   (50)    /* 50A - 1V */
#define SCALE_2   (50)    /* 50A - 1V */
#define DATA_SIZE (500)   /* Quantidade de amostras */

/* Power */
#define DEF_VOLTAGE       (127)
#define DEF_POWER_FACTOR  (0.866)

/* Types */
#define MEASURE_ELECTRICAL_CURRENT_AMPERE   (0x50)
#define MEASURE_ELECTRICAL_ENERGY_KHW       (0x70)

/* Chave bipolar */
#define SWT1  (3)
#define SWT2  (4)

/* Configurações Firebase */
#define FIREBASE_SOURCE_ID  "02:AB:C4:FF:3F"
#define FIREBASE_HOST       "wirelessrht.firebaseio.com"
#define FIREBASE_PORT       (443)
#define FIREBASE_AUTH       "iUwJfO2uiu820Q2uzXMbBGqFM9iTUKsYDpokPcbI"
#define FIREBASE_CLIENT     "AarXB9uiCDfQX53NqsXJbLY2Umz2" /* gianzanuz@gmail.com */

/* Período de publicação */
#define MESSAGE_SAMPLE_RATE     (180)

/* EEPROM */
#define EEPROM_ESP_AP_OFFSET    (0)
#define EEPROM_ESP_URL_OFFSET   (EEPROM.length()/2)

/* LDC */
#define LCD_ENABLE
// #define LCD_REFRESH_MEASURE

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

/* Valores de corrente */
float value1  = 0.0;
float value2  = 0.0;
uint32_t count = 0;

/*************************************************************************************
  Public prototypes
*************************************************************************************/
bool IOT_send_GET(const char* path, const char* query, const char* host);
bool IOT_send_POST(float value, uint32_t timestamp);

void WEB_init(void);
bool WEB_process_GET(uint8_t connection, char* path, char* parameters, uint32_t parametersSize);
bool WEB_process_POST(uint8_t connection, char* path, char* body, uint32_t bodySize);

bool WEB_chunk_init(uint8_t connection);
void WEB_chunk_send(char* chuck);
bool WEB_chunk_finish(void);
bool WEB_headers(uint8_t connection);

void serial_flush(void);
bool serial_get(const char* stringChecked, uint32_t timeout, char* serialBuffer, uint16_t serialBufferSize);

bool EEPROM_write(const uint8_t* buffer, int size, int addr);
bool EEPROM_read(uint8_t* buffer, int size, int addr);

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
#ifdef LCD_ENABLE
  lcd.begin(16, 2);
#endif

#ifdef FREE_MEMORY_DISPLAY
#ifdef LCD_ENABLE
  lcd.clear();
  lcd.print(F("FREE MEMORY:")); lcd.setCursor(0, 1);
  lcd.print(freeMemory());
  delay(1000);
#endif
#endif

  /* Configuração inicial do ESP */
  if(!esp.config(ESP8266::ESP_CLIENT_AND_SERVER_MODE))
  {
#ifdef LCD_ENABLE
    lcd.clear();
    lcd.print(F("ESP CONFIG:")); lcd.setCursor(0, 1);
    lcd.print(F("ERROR"));
#endif

    /* Aplica um soft-reset após delay */
    delay(60000);
    softReset();
  }
  else
  {
#ifdef LCD_ENABLE
    lcd.clear();
    lcd.print(F("ESP CONFIG:")); lcd.setCursor(0, 1);
    lcd.print(F("OK"));
    delay(1000);
#endif
  }

  /* Configurações do Soft Acess Point */
  strncpy(espAp.ssid, ESP_SERVER_SSID, sizeof(espAp.ssid));
  espAp.ssid[sizeof(espAp.ssid) - 1] = '\0';
  strncpy(espAp.password, ESP_SERVER_PASSWORD, sizeof(espAp.password));
  espAp.password[sizeof(espAp.password) - 1] = '\0';

  /* Ajuste do Soft Access Point */
  if(!esp.setAP(ESP8266::ESP_SERVER_MODE, espAp))
  {
#ifdef LCD_ENABLE
    lcd.clear();
    lcd.print(F("SET AP:")); lcd.setCursor(0, 1);
    lcd.print(F("ERROR"));
#endif

    /* Aplica um soft-reset após delay */
    delay(60000);
    softReset();
  }
  else
  {
#ifdef LCD_ENABLE
    lcd.clear();
    lcd.print(F("SET AP:")); lcd.setCursor(0, 1);
    lcd.print(F("OK"));
    delay(1000);
#endif
  }

  /* Configurações servidor */
  strncpy(espUrl.host, ESP_SERVER_IP_DEFAULT, sizeof(espUrl.host));
  espUrl.host[sizeof(espUrl.host) - 1] = '\0';
  strncpy(espUrl.path, ESP_SERVER_PATH_DEFAULT, sizeof(espUrl.path));
  espUrl.path[sizeof(espUrl.path) - 1] = '\0';
  espUrl.port     = ESP_SERVER_PORT_DEFAULT;
  //espUrl.key[0]   = '\0';

  /* Inicializa servidor */
  if(!esp.server(espUrl))
  {
#ifdef LCD_ENABLE
    lcd.clear();
    lcd.print(F("SERVER INIT:")); lcd.setCursor(0, 1);
    lcd.print(F("ERROR"));
#endif
    while (true) {};
  }
  else
  {
#ifdef LCD_ENABLE
    lcd.clear();
    lcd.print(F("SERVER INIT:")); lcd.setCursor(0, 1);
    lcd.print(F("OK"));
    delay(1000);
#endif
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
  //espUrl.key[0] = '\0';

  /* Obtém AP da EEPROM, caso haja */
  if(EEPROM_read((uint8_t*) &espAp, sizeof(espAp), EEPROM_ESP_AP_OFFSET))
  {
#ifdef LCD_ENABLE
    lcd.clear();
    lcd.print(F("EEPROM FOUND:")); lcd.setCursor(0, 1);
    lcd.print(F("AP"));
    delay(1000);

    lcd.clear();
    lcd.print(F("SSID: ")); lcd.print(espAp.ssid); lcd.setCursor(0, 1);
    lcd.print(F("pass: ")); lcd.print(espAp.password);
    delay(1000);
#endif

    /* Aplica novo AP recebido */
    esp.setAP(ESP8266::ESP_CLIENT_MODE, espAp);
  }
  else
  {
#ifdef LCD_ENABLE
    lcd.clear();
    lcd.print(F("EEPROM NOT FOUND:")); lcd.setCursor(0, 1);
    lcd.print(F("AP"));
    delay(1000);
#endif
  }

  /* Obtém URL da EEPROM, caso haja */
  if(EEPROM_read((uint8_t*) &espUrl, sizeof(espUrl), EEPROM_ESP_URL_OFFSET))
  {
#ifdef LCD_ENABLE
    lcd.clear();
    lcd.print(F("EEPROM FOUND:")); lcd.setCursor(0, 1);
    lcd.print(F("SERVER"));
    delay(1000);

    lcd.clear();
    lcd.print(F("host: ")); lcd.print(espUrl.host); lcd.setCursor(0, 1);
    lcd.print(F("path: ")); lcd.print(espUrl.path);
    delay(1000);

    lcd.clear();
    //lcd.print(F("key: ")); lcd.print(espUrl.key); lcd.setCursor(0, 1);
    lcd.print(F("port: ")); lcd.print(espUrl.port);
    delay(1000);
#endif
  }
  else
  {
#ifdef LCD_ENABLE
    lcd.clear();
    lcd.print(F("EEPROM NOT FOUND:")); lcd.setCursor(0, 1);
    lcd.print(F("SERVER"));
    delay(1000);
#endif
  }

  strncpy(espUrl.host, FIREBASE_HOST, sizeof(espUrl.host));
  espUrl.host[sizeof(espUrl.host) - 1] = '\0';
  espUrl.port = FIREBASE_PORT;

  /* Habilita watchdog */
  wdt_enable(WDTO_8S);
}

void loop()
{
  static uint32_t previousMillis = 0;
  uint32_t currentMillis = millis();
  if((uint32_t) (currentMillis - previousMillis) > ((uint32_t) MESSAGE_SAMPLE_RATE * 1000))
  {
    /* Obtém a média de corrente elétrica no período */
    value1 /= count;
    value2 /= count;

    /* Obtém o consumo energético */
    float energy1 = value1 * DEF_VOLTAGE * DEF_POWER_FACTOR * (currentMillis - previousMillis) / 3600000000 ;
    float energy2 = value2 * DEF_VOLTAGE * DEF_POWER_FACTOR * (currentMillis - previousMillis) / 3600000000 ;

    /* Envia para servidores */
    IOT_connect(value1, value2, energy1, energy2);
    serial_flush();

    /* Reset nos registradores */
    value1 = 0.0;
    value2 = 0.0;
    count = 0;

    /* Atualiza tempo decorrido */
    previousMillis = currentMillis;
  }

  /* Verifica se houve conexão ao servidor do ESP8266 */
  if(Serial.available())
    WEB_init();

  /* Realiza medida */
  /* Incrementa contagem */
  float current1, current2;
  IOT_measure(&current1, &current2);
  value1 += current1;
  value2 += current2;
  count++;

  /* Realiza o soft-reset */
  if(resetFlag)
    softReset();

  /* Atualiza watchdog */
  wdt_reset();
}

/************************************************************************************
  IOT_measure

  .

************************************************************************************/
bool IOT_measure(float* current1, float* current2)
{
    (*current1)    = 0.0;
    (*current2)    = 0.0;
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

    /* Remove primeiras 10 amostras */
    ADS1115Data.data_size = 10;
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
    (*current1) = sqrt(rmsSum / DATA_SIZE) * SCALE_1 * 1e-3; /* Corrente RMS [A] */

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

    /* Remove primeiras 10 amostras */
    ADS1115Data.data_size = 10;
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
    (*current2) = sqrt(rmsSum / DATA_SIZE) * SCALE_2 * 1e-3; /* Corrente RMS [A] */
#endif

#ifdef LCD_ENABLE
  #ifdef LCD_REFRESH_MEASURE
    /* Mostra na tela a cada 10 interações */
    if(!(count % 10))
    {
      lcd.clear();
      lcd.print("I: " + String((*current1) * 2) + " A rms"); lcd.setCursor(0, 1);
      lcd.print("P: " + String((*current1) * 2 * DEF_VOLTAGE * DEF_POWER_FACTOR / 1000) + " kW");
    }
  #endif
#endif

    return true;
}

/************************************************************************************
  IOT_connect

  .

************************************************************************************/
bool IOT_connect(float value1, float value2, float energy1, float energy2)
{
    /* Obtém grandezas */
    if(esp.checkWifi(10, 1000, espAp))
    {
#ifdef LCD_ENABLE
      lcd.clear();
      lcd.print(F("ESP CONNECT AP:")); lcd.setCursor(0, 1);
      lcd.print(F("OK"));
#endif

      /* Obtém timestamp atual */
      uint32_t timestamp = esp.getUnixTimestamp();
      if(!timestamp)
      {
#ifdef LCD_ENABLE
        lcd.clear();
        lcd.print(F("ESP TIMESTAMP:")); lcd.setCursor(0, 1);
        lcd.print(F("ERROR"));
#endif

        esp.close(ESP_CLOSE_ALL);
        return false;
      }
      else
      {
#ifdef LCD_ENABLE
        lcd.clear();
        lcd.print(F("ESP TIMESTAMP:")); lcd.setCursor(0, 1);
        lcd.print(timestamp);
#endif
      }

      /* Abre conexão com servidor */
      if(esp.connect(espUrl))
      {
#ifdef LCD_ENABLE
        lcd.clear();
        lcd.print(F("ESP CONNECT:")); lcd.setCursor(0, 1);
        lcd.print(F("OK"));
#endif

        /* Envia conteudo */
        if( IOT_send_POST(0, value1, MEASURE_ELECTRICAL_CURRENT_AMPERE, timestamp) && 
            IOT_send_POST(1, value2, MEASURE_ELECTRICAL_CURRENT_AMPERE, timestamp) && 
            IOT_send_POST(0, energy1, MEASURE_ELECTRICAL_ENERGY_KHW, timestamp) && 
            IOT_send_POST(1, energy2, MEASURE_ELECTRICAL_ENERGY_KHW, timestamp))
        {
#ifdef LCD_ENABLE
          lcd.clear();
          lcd.print(F("ESP SEND:")); lcd.setCursor(0, 1);
          lcd.print(F("OK"));
#endif
          esp.close(ESP_CLOSE_ALL);
        }
        else
        {
#ifdef LCD_ENABLE
          lcd.clear();
          lcd.print(F("ESP SEND:")); lcd.setCursor(0, 1);
          lcd.print(F("ERROR"));
#endif
          esp.close(ESP_CLOSE_ALL);
          return false;
        }
      }
      else
      {
#ifdef LCD_ENABLE
        lcd.clear();
        lcd.print(F("ESP CONNECT:")); lcd.setCursor(0, 1);
        lcd.print(F("ERROR"));
#endif
        esp.close(ESP_CLOSE_ALL);
        return false;
      }
    }
    else
    {
#ifdef LCD_ENABLE
      lcd.clear();
      lcd.print(F("ESP CONNECT AP:")); lcd.setCursor(0, 1);
      lcd.print(F("ERROR"));
#endif
      return false;
    }

    return true;
}


/************************************************************************************
  IOT_send_POST

  .

************************************************************************************/
bool IOT_send_POST(uint8_t channel, float value, uint8_t type, uint32_t timestamp)
{
    /* Sequencial number */
    static uint32_t seqNumber = 0;

    /* ESP8266: Inicializar envio */
    serial_flush();
    Serial.print(F("AT+CIPSENDEX=0,2047\r\n"));
    if(!serial_get(">", 100, NULL, 0))                  /* Aguarda '>' */
    {
        esp.close(ESP_CLOSE_ALL);
        return false;
    }

    /* HTTP HEADERS */
    /* HTTP POST */
    serial_flush();
    Serial.print(F("POST /users/" FIREBASE_CLIENT "/measures/"));
    Serial.print((type == MEASURE_ELECTRICAL_CURRENT_AMPERE) ? F("current") : F("energy"));
    Serial.print(F(".json?auth=" FIREBASE_AUTH " HTTP/1.1\r\n"));
    /* Host */
    Serial.print(F("Host: " FIREBASE_HOST "\r\n"));
    /* Connection */
    Serial.print(F("Connection: keep-alive\r\n"));
    /* Chunked */
    Serial.print(F("Transfer-Encoding: chunked\r\n"));
    /* Header End */
    Serial.print(F("\r\n"));
    
    /* Formata body */
    char buffer[50];
    sprintf(buffer, "{\"value\":%s,\r\n", String(value, 5).c_str());
    Serial.println(strlen(buffer) - 2, HEX);
    Serial.print(buffer);

    sprintf(buffer, "\"channel\":%u,\r\n", channel);
    Serial.println(strlen(buffer) - 2, HEX);
    Serial.print(buffer);

    sprintf(buffer, "\"type\":%u,\r\n", type);    /* MEASURE_ELECTRICAL_CURRENT_AMPERE : 0x50 / MEASURE_ELECTRICAL_ENERGY_KHW : 0x60 */
    Serial.println(strlen(buffer) - 2, HEX);
    Serial.print(buffer);

    sprintf(buffer, "\"seqNumber\":%lu,\r\n", seqNumber++);
    Serial.println(strlen(buffer) - 2, HEX);
    Serial.print(buffer);

    sprintf(buffer, "\"timestamp\":%lu,\r\n", timestamp);
    Serial.println(strlen(buffer) - 2, HEX);
    Serial.print(buffer);

    sprintf(buffer, "\"device\":\"%s\"}\r\n", FIREBASE_SOURCE_ID);
    Serial.println(strlen(buffer) - 2, HEX);
    Serial.print(buffer);

    /* End chunk */
    Serial.write("0\r\n\r\n");

    /* AT: End Send */
    Serial.write("\\0");

    /* Return */
    serial_get((char*) "SEND OK", 1000, NULL, 0);
    if(!serial_get((char*) "HTTP/1.1 200 OK\r\n", 1000, NULL, 0))
    {
        esp.close(ESP_CLOSE_ALL);
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
    esp.close(ESP_CLOSE_ALL);
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
      esp.close(ESP_CLOSE_ALL);
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
#ifdef LCD_ENABLE
  lcd.clear();
  lcd.print(F("ESP SERVER:")); lcd.setCursor(0, 1);
  lcd.print(F("GET"));
#endif

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
      esp.close(ESP_CLOSE_ALL);
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
#ifdef LCD_ENABLE
  lcd.clear();
  lcd.print(F("ESP SERVER:")); lcd.setCursor(0, 1);
  lcd.print(F("POST"));
#endif

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
#ifdef LCD_ENABLE
      lcd.clear();
      lcd.print(F("ESP SERVER:")); lcd.setCursor(0, 1);
      lcd.print(F("GOT NEW WIFI"));
      delay(1000);
#endif
      /* Salva AP na EEPROM */
      if(EEPROM_write((uint8_t*) &espAp, sizeof(espAp), EEPROM_ESP_AP_OFFSET))
      {
#ifdef LCD_ENABLE
        lcd.clear();
        lcd.print(F("EEPROM SAVED:")); lcd.setCursor(0, 1);
        lcd.print(F("AP"));
        delay(1000);

        lcd.clear();
        lcd.print(F("SSID: ")); lcd.print(espAp.ssid); lcd.setCursor(0, 1);
        lcd.print(F("pass: ")); lcd.print(espAp.password);
        delay(1000);
#endif
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
#ifdef LCD_ENABLE
      lcd.clear();
      lcd.print(F("ESP SERVER:")); lcd.setCursor(0, 1);
      lcd.print(F("WRONG INPUT"));
      delay(1000);
#endif
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
        //url_decode(tkn, espUrl.key, sizeof(espUrl.key)); /* Decodifica caracteres especiais */
        //str_safe(espUrl.key, sizeof(espUrl.key)); /* Torna a string 'segura' */
    }
    else
      //espUrl.key[0] = '\0';

    if(strlen(espUrl.host) && strlen(espUrl.path) && espUrl.port)//strlen(espUrl.key) 
    {
#ifdef LCD_ENABLE
      lcd.clear();
      lcd.print(F("ESP SERVER:")); lcd.setCursor(0, 1);
      lcd.print(F("GOT NEW SERVER"));
      delay(1000);
#endif
      /* Salva URL na EEPROM */
      if(EEPROM_write((uint8_t*) &espUrl, sizeof(espUrl), EEPROM_ESP_URL_OFFSET))
      {
#ifdef LCD_ENABLE
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
        //lcd.print(F("key: ")); lcd.print(espUrl.key);
        delay(1000);
#endif
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
#ifdef LCD_ENABLE
      lcd.clear();
      lcd.print(F("ESP SERVER:")); lcd.setCursor(0, 1);
      lcd.print(F("WRONG INPUT"));
      delay(1000);
#endif
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
      esp.close(ESP_CLOSE_ALL);
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
        esp.close(ESP_CLOSE_ALL);
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
        esp.close(ESP_CLOSE_ALL);
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
  EEPROM_write

  

************************************************************************************/
bool EEPROM_write(const uint8_t* buffer, int size, int addr)
{
  /* Escreve buffer na EEPROM */
  /* Salva CRC8 no final */
  for(int i=0; i<size; i++)
    EEPROM.write(addr++, buffer[i]);
  EEPROM.write(addr, CRC_8(buffer, size, CRC_8_MAXIM_POLY));

  return true;
}

/************************************************************************************
  EEPROM_read

  

************************************************************************************/
bool EEPROM_read(uint8_t* buffer, int size, int addr)
{
  /* Escreve buffer na EEPROM */
  for(int i=0; i<size; i++)
    buffer[i] = EEPROM.read(addr++);

  /* Verifica CRC8 no final */
  return (EEPROM.read(addr) == CRC_8(buffer, size, CRC_8_MAXIM_POLY));
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

    /* Atualiza watchdog */
    wdt_reset();
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
