#include "config.h"
#include "ESP8266.h"
#include "ADS1115.h"
#include "Energy.h"
#include "CRC.h"
#include "Timer.h"
#include <Wire.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <math.h>
#include <avr/wdt.h>

/*************************************************************************************
  Private macros
*************************************************************************************/

/* Types */
#define MEASURE_ELECTRICAL_CURRENT_AMPERE (0x50)
#define MEASURE_ELECTRICAL_ENERGY_KHW (0x70)
#define MEASURE_ENERGY_COST_REAIS (0x80)

/* Chave bipolar */
#define SWT1 (3)
#define SWT2 (4)

/* Período de publicação, em segundos */
#define MESSAGE_SAMPLE_RATE (60u)

/* Período para atualizar a timestamp, em segundos */
#define TIMESTAMP_REFRESH_TIME (21600u)

/* EEPROM */
#define EEPROM_ESP_AP_OFFSET (0)
#define EEPROM_ESP_URL_OFFSET (1 * EEPROM.length() / 3)
#define EEPROM_ENERGY_OFFSET (2 * EEPROM.length() / 3)

/* LDC */
#define LCD_ENABLE
#define LCD_REFRESH_MEASURE

/* Canais */
#define CHANNEL_1 (0)
#define CHANNEL_2 (1)
#define CHANNEL_SIZE (2)

/*************************************************************************************
  Private variables
*************************************************************************************/
/* Módulo WiFi ESP8266 */
/* Pino de Enable = 2 */
static ESP8266 esp(ESP_ENABLE_PIN);
static ESP8266::esp_URL_parameter_t espUrl = {
    FIREBASE_HOST,
    FIREBASE_AUTH,
    FIREBASE_CLIENT};
static ESP8266::esp_AP_parameter_t espAp = {
    ESP_CLIENT_SSID,
    ESP_CLIENT_PASSWORD,
};

/* LCD 16x2 */
static LiquidCrystal lcd(10, 11, 6, 7, 8, 9);

/* Energia para cada canal */
static Energy energy[CHANNEL_SIZE] = {Energy(CHANNEL_1), Energy(CHANNEL_2)};

/* Timestamp atual */
static uint32_t timestamp = 0;

/* Timers */
static Timer timestampTimer = Timer();
static Timer publishTimer = Timer();

/*************************************************************************************
  Public prototypes
*************************************************************************************/
bool IOT_send_GET(const char *path, const char *query, const char *host);
bool IOT_send_POST(float value, uint32_t timestamp);

void WEB_init(void);
bool WEB_process_GET(uint8_t connection, char *path, char *parameters, uint32_t parametersSize);
bool WEB_process_POST(uint8_t connection, char *path, char *body, uint32_t bodySize);

bool WEB_chunk_init(uint8_t connection);
void WEB_chunk_send(char *chuck);
bool WEB_chunk_finish(void);
bool WEB_headers(uint8_t connection);

void serial_flush(void);
bool serial_get(const char *stringChecked, uint32_t timeout, char *returnBuffer, uint16_t returnBufferSize);

bool EEPROM_write(const uint8_t *buffer, int size, int addr);
bool EEPROM_read(uint8_t *buffer, int size, int addr);

/* Converts a hex character to its integer value */
char from_hex(char ch);
char to_hex(char code);
void url_encode(char *str, char *encoded);
void url_decode(char *str, char *decoded, int size);
void str_safe(char *str, uint32_t size);
void LCD_print(const __FlashStringHelper *line1, const __FlashStringHelper *line2, uint32_t delayMs = 0);

/* Soft Reset */
void (*softReset)(void) = 0;

/*******************************************************************************
   setup
****************************************************************************/
/**
 * @brief Initial setup.
 * @return void.
 *******************************************************************************/
void setup()
{
#ifdef LCD_ENABLE
  lcd.begin(16, 2);
#endif

#ifdef FREE_MEMORY_DISPLAY
#ifdef LCD_ENABLE
  /* Imprime a quantidade de memória disponível */
  lcd.clear();
  lcd.print(F("FREE MEMORY:"));
  lcd.setCursor(0, 1);
  lcd.print(freeMemory());
  delay(1000);
#endif
#endif

  /* Configuração inicial do ESP */
  if (!esp.config())
  {
    LCD_print(F("ESP CONFIG:"), F("ERROR"));

    /* Aplica um soft-reset após delay */
    delay(60000);
    softReset();
  }
  LCD_print(F("ESP CONFIG:"), F("OK"), 1000);

  /* Obtém AP da EEPROM, caso haja */
  if (EEPROM_read((uint8_t *)&espAp, sizeof(espAp), EEPROM_ESP_AP_OFFSET))
    LCD_print(F("EEPROM FOUND:"), F("AP"), 1000);
  else
    LCD_print(F("EEPROM NOT FOUND:"), F("AP"), 1000);

#ifdef LCD_ENABLE
  /* Imprime configuração AP atual */
  lcd.clear();
  lcd.print(espAp.ssid);
  lcd.setCursor(0, 1);
  lcd.print(espAp.password);
  delay(3000);
#endif

  /* Conecta ao AP */
  if (!esp.connect_ap(espAp))
  {
    LCD_print(F("ESP AP:"), F("ERROR"));

    /* Aplica um soft-reset após delay */
    delay(60000);
    softReset();
  }
  LCD_print(F("ESP AP:"), F("OK"), 1000);

  /* Obtém timestamp atual */
  while ((timestamp = esp.getUnixTimestamp()) == 0)
  {
    LCD_print(F("ESP TIMESTAMP:"), F("ERROR"));
    esp.close(ESP_CLOSE_ALL);

    /* Aplica um delay */
    delay(5000);
  }

  /* Reseta o timer para obter nova timestamp */
  timestampTimer.resetTimer();

#ifdef LCD_ENABLE
  lcd.clear();
  lcd.print(F("ESP TIMESTAMP:"));
  lcd.setCursor(0, 1);
  lcd.print(timestamp);
  delay(3000);
#endif

  /* Obtém SERVER da EEPROM, caso haja */
  if (EEPROM_read((uint8_t *)&espUrl, sizeof(espUrl), EEPROM_ESP_URL_OFFSET))
    LCD_print(F("EEPROM FOUND:"), F("SERVER"), 1000);
  else
    LCD_print(F("EEPROM NOT FOUND:"), F("SERVER"), 1000);

#ifdef LCD_ENABLE
  /* Imprime configuração SERVER atual */
  lcd.clear();
  lcd.print(espUrl.host);
  delay(3000);

  lcd.clear();
  lcd.print(espUrl.auth);
  lcd.setCursor(0, 1);
  lcd.print(espUrl.client);
  delay(3000);
#endif

  /* Obtém ENERGY da EEPROM, caso haja */
  if (EEPROM_read((uint8_t *)&energy[CHANNEL_1].config, sizeof(energy[CHANNEL_1].config), EEPROM_ENERGY_OFFSET))
  {
    /* Duplica para o segundo canal */
    energy[CHANNEL_2].config = energy[CHANNEL_1].config;
    LCD_print(F("EEPROM FOUND:"), F("ENERGY"), 1000);
  }
  else
    LCD_print(F("EEPROM NOT FOUND:"), F("ENERGY"), 1000);

#ifdef LCD_ENABLE
  /* Imprime configuração ENERGY atual */
  lcd.clear();
  lcd.print(F("dataSize: "));
  lcd.print(energy[CHANNEL_1].config.dataSize);
  lcd.setCursor(0, 1);
  lcd.print(F("scale: "));
  lcd.print(energy[CHANNEL_1].config.scale);
  delay(3000);

  lcd.clear();
  lcd.print(F("basePrice: "));
  lcd.print(energy[CHANNEL_1].config.basePrice);
  lcd.setCursor(0, 1);
  lcd.print(F("flagPrice: "));
  lcd.print(energy[CHANNEL_1].config.flagPrice);
  delay(3000);

  lcd.clear();
  lcd.print(F("lineVoltage: "));
  lcd.print(energy[CHANNEL_1].config.lineVoltage);
  lcd.setCursor(0, 1);
  lcd.print(F("powerFactor: "));
  lcd.print(energy[CHANNEL_1].config.powerFactor);
  delay(3000);
#endif

  /* Atualiza timestamp inicial */
  energy[CHANNEL_1].calculate(timestamp);
  energy[CHANNEL_2].calculate(timestamp);

  /* Restaura timer para publicação de dados */
  publishTimer.resetTimer();

  /* Habilita watchdog */
  wdt_enable(WDTO_8S);
}

/*******************************************************************************
   loop
****************************************************************************/
/**
 * @brief Main loop.
 * @return void.
 *******************************************************************************/
void loop()
{
  /* Verifica se já passou o período de publicação de dados */
  if (publishTimer.checkIntervalPassed((uint32_t) MESSAGE_SAMPLE_RATE * 1000u))
  {
    /* Incrementa a timestamp */
    timestamp += publishTimer.getElapsedTime() / 1000u;

    /* Reseta o timer de publicação */
    publishTimer.resetTimer();

    /* Finaliza o cálculo de energia elétrica */
    energy[CHANNEL_1].calculate(timestamp);
    energy[CHANNEL_2].calculate(timestamp);

    /* Desativa servidor */
    esp.server_stop();

    /* Envia para servidores */
    IOT_connect();

    /* Ativa servidor */
    esp.server_start();

    serial_flush();
  }

  /* Verifica se passou do período de obter nova timestamp */
  if (timestampTimer.checkIntervalPassed((uint32_t) TIMESTAMP_REFRESH_TIME * 1000u))
  {
    /* Reseta o timer para obter a timestamp */
    timestampTimer.resetTimer();

    /* Tenta obter nova timestamp do servidor */
    uint32_t newTimestamp = esp.getUnixTimestamp();
    if (newTimestamp != 0)
      timestamp = newTimestamp;

#ifdef LCD_ENABLE
    lcd.clear();
    lcd.print(F("ESP TIMESTAMP:"));
    lcd.setCursor(0, 1);
    lcd.print(newTimestamp);
    delay(3000);
#endif
  }

  /* Verifica se houve conexão ao servidor do ESP8266 */
  if (Serial.available())
    WEB_init();

  /* Realiza medida */
  energy[CHANNEL_1].measure();
  energy[CHANNEL_2].measure();

#ifdef LCD_ENABLE
#ifdef LCD_REFRESH_MEASURE
  /* Mostra na tela a cada 50 interações */
  uint32_t rmsCount = energy[CHANNEL_1].getRmsCount();
  if (!(rmsCount % 10))
  {
    lcd.clear();
    lcd.print("I: ");
    lcd.print(energy[CHANNEL_1].getRmsLast() + energy[CHANNEL_2].getRmsLast(), 1);
    lcd.print(" A");
    lcd.setCursor(0, 1);
    lcd.print("C: ");
    lcd.print(rmsCount);
  }
#endif
#endif

  /* Atualiza watchdog */
  wdt_reset();
}

/************************************************************************************
  IOT_connect

  .

************************************************************************************/
bool IOT_connect()
{
  /* Verifica conexão com o ponto de acesso wifi */
  if (!esp.checkWifi())
  {
    LCD_print(F("ESP CONNECT AP:"), F("ERROR"));
    return false;
  }
  LCD_print(F("ESP CONNECT AP:"), F("OK"));

  /* Abre conexão com servidor */
  if (!esp.connect(espUrl))
  {
    LCD_print(F("ESP CONNECT:"), F("ERROR"));
    esp.close(ESP_CLOSE_ALL);
    return false;
  }
  LCD_print(F("ESP CONNECT:"), F("OK"));

  /* Envia conteudo */
  if (IOT_send_POST(energy[CHANNEL_1].getElectricCurrentAmperes() + energy[CHANNEL_2].getElectricCurrentAmperes(), MEASURE_ELECTRICAL_CURRENT_AMPERE, timestamp) &&
      IOT_send_POST(energy[CHANNEL_1].getEnergyAccumulatedKiloWattsHour() + energy[CHANNEL_2].getEnergyAccumulatedKiloWattsHour(), MEASURE_ELECTRICAL_ENERGY_KHW, timestamp) &&
      IOT_send_POST(energy[CHANNEL_1].getCostAccumulatedReais() + energy[CHANNEL_2].getCostAccumulatedReais(), MEASURE_ENERGY_COST_REAIS, timestamp))
  {
    LCD_print(F("ESP SEND:"), F("OK"));
    esp.close(ESP_CLOSE_ALL);
  }
  else
  {
    LCD_print(F("ESP SEND:"), F("ERROR"));
    esp.close(ESP_CLOSE_ALL);
    return false;
  }

  return true;
}

/************************************************************************************
  IOT_send_POST

  .

************************************************************************************/
bool IOT_send_POST(float value, uint8_t type, uint32_t timestamp)
{
  /* Sequencial number */
  static uint32_t seqNumber = 0;

  /* ESP8266: Inicializar envio */
  serial_flush();
  Serial.print(F("AT+CIPSENDEX=0,2047\r\n"));
  if (!serial_get(">", 100, NULL, 0)) /* Aguarda '>' */
    return false;

  /* HTTP HEADERS */
  /* HTTP POST */
  serial_flush();
  Serial.print(F("POST /users/"));
  Serial.print(espUrl.client);
  Serial.print(F("/measures/"));

  if (type == MEASURE_ELECTRICAL_CURRENT_AMPERE)
    Serial.print(F("current"));
  else if (type == MEASURE_ELECTRICAL_ENERGY_KHW)
    Serial.print(F("energy"));
  else if (type == MEASURE_ENERGY_COST_REAIS)
    Serial.print(F("cost"));
  else
    return false;

  Serial.print(F(".json?auth="));
  Serial.print(espUrl.auth);
  Serial.print(F(" HTTP/1.1\r\n"));
  /* Host */
  Serial.print(F("Host: "));
  Serial.print(espUrl.host);
  Serial.print(F("\r\n"));
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

  sprintf(buffer, "\"type\":%u,\r\n", type); /* MEASURE_ELECTRICAL_CURRENT_AMPERE : 0x50 / MEASURE_ELECTRICAL_ENERGY_KHW : 0x60 */
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
  serial_get("SEND OK", 1000, NULL, 0);
  if (!serial_get("HTTP/1.1 200 OK\r\n", 1000, NULL, 0))
    return false;

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
  if (!serial_get("HTTP/1.1\r\n", 500, strBuffer, sizeof(strBuffer)))
  {
    /* Fecha todas as conexões */
    esp.close(ESP_CLOSE_ALL);
    return;
  }

  /* Obtém o index da conexão */
  char *tkn;
  strtok(strstr(strBuffer, "+IPD"), ",");
  tkn = strtok(NULL, ",");
  connection = (uint8_t)atoi(tkn);

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
    serial_get("\r\n\r\n", 500, NULL, 0);
    serial_get("\r\n", 500, strBuffer, sizeof(strBuffer));

    /* Inicializa POST */
    WEB_process_POST(connection, path, strBuffer, sizeof(strBuffer));
  }
  /* 405 - METHOD NOT ALLOWED */
  else
  {
    /* ESP8266: Inicializar envio */
    serial_flush();
    Serial.print("AT+CIPSENDEX="); /* AT: Send command */
    Serial.print(connection);
    Serial.print(",2047\r\n");
    if (!serial_get(">", 100, NULL, 0)) /* Aguarda '>' */
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
bool WEB_process_GET(uint8_t connection, char *path, char *parameter, uint32_t parameterSize)
{
  LCD_print(F("ESP SERVER:"), F("GET"), 1000);

  /* WIFI */
  if (!strcmp(path, "/wifi.json"))
  {
    /* ESP8266: Inicializar envio */
    if (!WEB_headers(connection))
      return false;

    /* ssid */
    sprintf(parameter, "{\"ssid\":\"%s\",\r\n", espAp.ssid);
    Serial.println(strlen(parameter) - 2, HEX);
    Serial.print(parameter);

    /* password */
    sprintf(parameter, "\"password\":\"%s\"}\r\n", espAp.password);
    Serial.println(strlen(parameter) - 2, HEX);
    Serial.print(parameter);

    /* End chunk */
    Serial.write("0\r\n\r\n");
    return WEB_chunk_finish();
  }

  /* SERVER */
  else if (!strcmp(path, "/server.json"))
  {
    /* ESP8266: Inicializar envio */
    if (!WEB_headers(connection))
      return false;

    /* host */
    sprintf(parameter, "{\"host\":\"%s\",\r\n", espUrl.host);
    Serial.println(strlen(parameter) - 2, HEX);
    Serial.print(parameter);

    /* auth */
    sprintf(parameter, "\"auth\":\"%s\",\r\n", espUrl.auth);
    Serial.println(strlen(parameter) - 2, HEX);
    Serial.print(parameter);

    /* client */
    sprintf(parameter, "\"client\":\"%s\"}\r\n", espUrl.client);
    Serial.println(strlen(parameter) - 2, HEX);
    Serial.print(parameter);

    /* End chunk */
    Serial.write("0\r\n\r\n");
    return WEB_chunk_finish();
  }

  /* ENERGY */
  else if (!strcmp(path, "/energy.json"))
  {
    /* ESP8266: Inicializar envio */
    if (!WEB_headers(connection))
      return false;

    /* dataSize */
    sprintf(parameter, "{\"dataSize\":%u,\r\n", energy[0].config.dataSize);
    Serial.println(strlen(parameter) - 2, HEX);
    Serial.print(parameter);

    /* scale */
    sprintf(parameter, "\"scale\":%u,\r\n", energy[0].config.scale);
    Serial.println(strlen(parameter) - 2, HEX);
    Serial.print(parameter);

    /* lineVoltage */
    sprintf(parameter, "\"lineVoltage\":%u,\r\n", energy[0].config.lineVoltage);
    Serial.println(strlen(parameter) - 2, HEX);
    Serial.print(parameter);

    /* powerFactor */
    sprintf(parameter, "\"powerFactor\":%u,\r\n", energy[0].config.powerFactor);
    Serial.println(strlen(parameter) - 2, HEX);
    Serial.print(parameter);

    /* basePrice */
    sprintf(parameter, "\"basePrice\":%s,\r\n", String(energy[0].config.basePrice, 3).c_str());
    Serial.println(strlen(parameter) - 2, HEX);
    Serial.print(parameter);

    /* flagPrice */
    sprintf(parameter, "\"flagPrice\":%s}\r\n", String(energy[0].config.flagPrice, 3).c_str());
    Serial.println(strlen(parameter) - 2, HEX);
    Serial.print(parameter);

    /* End chunk */
    Serial.write("0\r\n\r\n");
    return WEB_chunk_finish();
  }

  /* 404 - NOT FOUND */
  else
  {
    /* ESP8266: Inicializar envio */
    serial_flush();
    Serial.print("AT+CIPSENDEX="); /* AT: Send command */
    Serial.print(connection);
    Serial.print(",2047\r\n");
    if (!serial_get(">", 100, NULL, 0)) /* Aguarda '>' */
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
bool WEB_process_POST(uint8_t connection, char *path, char *body, uint32_t bodySize)
{
  LCD_print(F("ESP SERVER:"), F("POST"), 1000);

  /* Verifica o início do objeto */
  if (body[0] != '{')
  {
    WEB_400_bad_request(connection);
    return false;
  }

  /* Início do json */
  char *tkn = body + 1;

  /* WIFI */
  if (strstr(path, "/wifi.json"))
  {
    /* Para cada campo do objeto */
    while ((tkn = strtok(tkn, "\"")) != NULL)
    {
      /* SSID */
      if (!strcmp(tkn, "ssid"))
      {
        strtok(NULL, "\"");
        tkn = strtok(NULL, "\"");
        url_decode(tkn, espAp.ssid, sizeof(espAp.ssid)); /* Decodifica caracteres especiais */
        str_safe(espAp.ssid, sizeof(espAp.ssid));        /* Torna a string 'segura' */
      }
      /* PASSWORD */
      else if (!strcmp(tkn, "password"))
      {
        strtok(NULL, "\"");
        tkn = strtok(NULL, "\"");
        url_decode(tkn, espAp.password, sizeof(espAp.password)); /* Decodifica caracteres especiais */
        str_safe(espAp.password, sizeof(espAp.password));        /* Torna a string 'segura' */
      }

      /* Continua parser */
      tkn = NULL;
    }

    /* Salva AP na EEPROM */
    LCD_print(F("ESP SERVER:"), F("GOT NEW WIFI"), 1000);
    if (EEPROM_write((uint8_t *)&espAp, sizeof(espAp), EEPROM_ESP_AP_OFFSET))
    {
#ifdef LCD_ENABLE
      LCD_print(F("EEPROM SAVED:"), F("AP"), 1000);

      lcd.clear();
      lcd.print(espAp.ssid);
      lcd.setCursor(0, 1);
      lcd.print(espAp.password);
      delay(3000);
      wdt_reset();
#endif
    }
  }

  /* SERVERS */
  if (strstr(path, "/servers.json"))
  {
    /* Para cada campo do objeto */
    while ((tkn = strtok(tkn, "\"")) != NULL)
    {
      /* host */
      if (!strcmp(tkn, "host"))
      {
        strtok(NULL, "\"");
        tkn = strtok(NULL, "\"");
        url_decode(tkn, espUrl.host, sizeof(espUrl.host)); /* Decodifica caracteres especiais */
        str_safe(espUrl.host, sizeof(espUrl.host));        /* Torna a string 'segura' */
      }
      /* auth */
      else if (!strcmp(tkn, "auth"))
      {
        strtok(NULL, "\"");
        tkn = strtok(NULL, "\"");
        url_decode(tkn, espUrl.auth, sizeof(espUrl.auth)); /* Decodifica caracteres especiais */
        str_safe(espUrl.auth, sizeof(espUrl.auth));        /* Torna a string 'segura' */
      }
      /* client */
      else if (!strcmp(tkn, "client"))
      {
        strtok(NULL, "\"");
        tkn = strtok(NULL, "\"");
        url_decode(tkn, espUrl.client, sizeof(espUrl.client)); /* Decodifica caracteres especiais */
        str_safe(espUrl.client, sizeof(espUrl.client));        /* Torna a string 'segura' */
      }

      /* Continua parser */
      tkn = NULL;
    }

    /* Salva URL na EEPROM */
    LCD_print(F("ESP SERVER:"), F("GOT NEW SERVER"), 1000);
    if (EEPROM_write((uint8_t *)&espUrl, sizeof(espUrl), EEPROM_ESP_URL_OFFSET))
    {
#ifdef LCD_ENABLE
      LCD_print(F("EEPROM SAVED:"), F("SERVER"), 1000);

      lcd.clear();
      lcd.print(espUrl.host);
      delay(3000);
      wdt_reset();

      lcd.clear();
      lcd.print(espUrl.auth);
      lcd.setCursor(0, 1);
      lcd.print(espUrl.client);
      delay(3000);
      wdt_reset();
#endif
    }
  }

  /* ENERGY */
  if (strstr(path, "/energy.json"))
  {
    /* Para cada campo do objeto */
    while ((tkn = strtok(tkn, "\"")) != NULL)
    {
      /* dataSize */
      if (!strcmp(tkn, "dataSize"))
      {
        tkn = strtok(NULL, ":,}");
        energy[0].config.dataSize = atoi(tkn);
        energy[1].config.dataSize = atoi(tkn);
      }
      /* scale */
      else if (!strcmp(tkn, "scale"))
      {
        tkn = strtok(NULL, ":,}");
        energy[0].config.scale = atoi(tkn);
        energy[1].config.scale = atoi(tkn);
      }
      /* lineVoltage */
      else if (!strcmp(tkn, "lineVoltage"))
      {
        tkn = strtok(NULL, ":,}");
        energy[0].config.lineVoltage = atoi(tkn);
        energy[1].config.lineVoltage = atoi(tkn);
      }
      /* powerFactor */
      else if (!strcmp(tkn, "powerFactor"))
      {
        tkn = strtok(NULL, ":,}");
        energy[0].config.powerFactor = atoi(tkn);
        energy[1].config.powerFactor = atoi(tkn);
      }
      /* basePrice */
      else if (!strcmp(tkn, "basePrice"))
      {
        tkn = strtok(NULL, ":,}");
        energy[0].config.basePrice = atof(tkn);
        energy[1].config.basePrice = atof(tkn);
      }
      /* flagPrice */
      else if (!strcmp(tkn, "flagPrice"))
      {
        tkn = strtok(NULL, ":,}");
        energy[0].config.flagPrice = atof(tkn);
        energy[1].config.flagPrice = atof(tkn);
      }

      /* Continua parser */
      tkn = NULL;
    }

    /* Salva URL na EEPROM */
    LCD_print(F("ESP SERVER:"), F("GOT NEW ENERGY"), 1000);
    if (EEPROM_write((uint8_t *)&energy[0].config, sizeof(energy[0].config), EEPROM_ENERGY_OFFSET))
    {
#ifdef LCD_ENABLE
      LCD_print(F("EEPROM SAVED:"), F("ENERGY"), 1000);

      /* Imprime configuração SERVER atual */
      lcd.clear();
      lcd.print(F("dataSize: "));
      lcd.print(energy[CHANNEL_1].config.dataSize);
      lcd.setCursor(0, 1);
      lcd.print(F("scale: "));
      lcd.print(energy[CHANNEL_1].config.scale);
      delay(3000);
      wdt_reset();

      lcd.clear();
      lcd.print(F("basePrice: "));
      lcd.print(energy[CHANNEL_1].config.basePrice, 3);
      lcd.setCursor(0, 1);
      lcd.print(F("flagPrice: "));
      lcd.print(energy[CHANNEL_1].config.flagPrice, 3);
      delay(3000);
      wdt_reset();

      lcd.clear();
      lcd.print(F("lineVoltage: "));
      lcd.print(energy[CHANNEL_1].config.lineVoltage);
      lcd.setCursor(0, 1);
      lcd.print(F("powerFactor: "));
      lcd.print(energy[CHANNEL_1].config.powerFactor);
      delay(3000);
      wdt_reset();
#endif
    }
  }

  /* 404 - NOT FOUND */
  else
  {
    /* ESP8266: Inicializar envio */
    serial_flush();
    Serial.print("AT+CIPSENDEX="); /* AT: Send command */
    Serial.print(connection);
    Serial.print(",2047\r\n");
    if (!serial_get(">", 100, NULL, 0)) /* Aguarda '>' */
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

  /* ESP8266: Inicializar envio */
  if (!WEB_204_no_content(connection))
    return false;

  return false;
}

/*******************************************************************************
   WEB_chuck_init
****************************************************************************/
/**
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
  if (!serial_get(">", 1000, NULL, 0))
  {
    esp.close(ESP_CLOSE_ALL);
    return false;
  }
  return true;
}

/*******************************************************************************
   WEB_chuck_send
****************************************************************************/
/**
 * @brief
 * @param
 * @return
 *******************************************************************************/
void WEB_chunk_send(char *chuck)
{
  /* ENVIO DO CHUNCK */
  Serial.println(strlen(chuck), HEX); /* CHUNCK SIZE */
  Serial.println(chuck);
}

/*******************************************************************************
   WEB_chuck_finish
****************************************************************************/
/**
 * @brief
 * @param
 * @return
 *******************************************************************************/
bool WEB_chunk_finish(void)
{
  /* Finaliza envio e obtém confirmação */
  Serial.print("\\0");
  if (!serial_get("SEND OK\r\n", 1000, NULL, 0))
  {
    esp.close(ESP_CLOSE_ALL);
    return false;
  }
  esp.close(ESP_CLOSE_ALL);
  return true;
}

/*******************************************************************************
   ESP_local_server_init
****************************************************************************/
/**
 * @brief
 * @param
 * @return
 *******************************************************************************/
bool WEB_headers(uint8_t connection)
{
  /* ESP8266: Inicializar 1º envio */
  if (!WEB_chunk_init(connection))
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

  return true;
}

/*******************************************************************************
   WEB_204_no_content
****************************************************************************/
/**
 * @brief
 * @param
 * @return
 *******************************************************************************/
bool WEB_204_no_content(uint8_t connection)
{
  /* ESP8266: Inicializar 1º envio */
  if (!WEB_chunk_init(connection))
    return false;

  /* HTTP HEADERS */
  serial_flush();
  Serial.print(F("HTTP/1.1 204 No Content\r\n"));
  /* Hosts */
  Serial.print(F("Host: 192.168.4.1\r\n"));
  /* Connection */
  Serial.print(F("Connection: Close\r\n"));
  /* Header End */
  Serial.print(F("\r\n"));

  /* Finaliza conexão */
  if (!WEB_chunk_finish())
    return false;

  return true;
}

/*******************************************************************************
   WEB_204_no_content
****************************************************************************/
/**
 * @brief
 * @param
 * @return
 *******************************************************************************/
bool WEB_400_bad_request(uint8_t connection)
{
  /* ESP8266: Inicializar 1º envio */
  if (!WEB_chunk_init(connection))
    return false;

  /* HTTP HEADERS */
  serial_flush();
  Serial.print(F("HTTP/1.1 400 Bad Request\r\n"));
  /* Hosts */
  Serial.print(F("Host: 192.168.4.1\r\n"));
  /* Connection */
  Serial.print(F("Connection: Close\r\n"));
  /* Header End */
  Serial.print(F("\r\n"));

  /* Finaliza conexão */
  if (!WEB_chunk_finish())
    return false;

  return true;
}

/************************************************************************************
  EEPROM_write



************************************************************************************/
bool EEPROM_write(const uint8_t *buffer, int size, int addr)
{
  /* Escreve buffer na EEPROM */
  /* Salva CRC8 no final */
  for (int i = 0; i < size; i++)
    EEPROM.write(addr++, buffer[i]);
  EEPROM.write(addr, CRC_8(buffer, size, CRC_8_MAXIM_POLY));

  return true;
}

/************************************************************************************
  EEPROM_read



************************************************************************************/
bool EEPROM_read(uint8_t *buffer, int size, int addr)
{
  uint8_t *temp = (uint8_t *)malloc(size);
  if (!temp)
    return false;

  /* Escreve buffer na EEPROM */
  for (int i = 0; i < size; i++)
    temp[i] = EEPROM.read(addr++);

  /* Verifica CRC8 no final */
  if (EEPROM.read(addr) != CRC_8(temp, size, CRC_8_MAXIM_POLY))
  {
    free(temp);
    return false;
  }

  /* Copia o buffer recebido */
  memcpy(buffer, temp, size);
  free(temp);
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
bool serial_get(const char *stringChecked, uint32_t timeout, char *returnBuffer, uint16_t returnBufferSize)
{
  uint16_t position = 0;
  uint16_t returnBufferPosition = 0;
  Timer serialTimer = Timer();

  /* Verifica argumentos de entrada */
  if (stringChecked[position] == '\0' || timeout == 0)
    return false;
  if (returnBuffer != NULL)
  {
    if (returnBufferSize < 2)
      return false;
    returnBuffer[returnBufferPosition] = '\0';
  }

  while (true)
  {
    /* Enquanto houver dados no buffer, receber e comparar com a string a ser checada */
    while (Serial.available())
    {
      char buffer = (char)Serial.read();

      /* Caso tenha recebido null char, subtitui por um espaço */
      if (buffer == '\0')
        buffer = ' ';

      /* Adiciona byte recebido no buffer de retorno */
      if (returnBuffer != NULL)
      {
        returnBuffer[returnBufferPosition++] = buffer;
        returnBuffer[returnBufferPosition] = '\0';

        /* Caso tenha atingido o final do buffer de retorno, reinicia */
        if (returnBufferPosition == (returnBufferSize - 1))
          returnBufferPosition = 0;
      }

      /* Avança ponteiro e coloca NULL char para finalizar string */
      if (buffer == stringChecked[position])
      {
        /* Incrementa posição da string de verificação */
        position++;

        /* Verifica se atingiu o final da string de verificação */
        if (stringChecked[position] == '\0')
          return true;
      }
      else
      {
        /* Inicia verificação do início */
        position = 0;
      }
    }

    /* Verifica se já passou o limite */
    if (serialTimer.checkIntervalPassed(timeout))
      return false;

    /* Atualiza watchdog */
    wdt_reset();
  }
}

/******************************************************************************/

/* Converts a hex character to its integer value */
char from_hex(char ch)
{
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character*/
char to_hex(char code)
{
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

/* Returns a url-encoded version of str */
void url_encode(char *str, char *encoded)
{
  char *pstr = str, *pbuf = encoded;
  while (*pstr)
  {
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
void url_decode(char *str, char *decoded, int size)
{
  char *pstr = str, *pbuf = decoded;
  int count = 0;

  while (*pstr)
  {
    /* Verifica se atingiu tamanho máximo */
    /* Sempre finaliza string com '\0' */
    count++;
    if ((count + 1) > size)
      break;

    /* Codificado em HEX */
    if (*pstr == '%')
    {
      if (pstr[1] && pstr[2])
      {
        *pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
        pstr += 2;
      }
    }
    /* Espaço */
    else if (*pstr == '+')
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
  for (uint32_t i = 0; i < size; i++)
  {
    if (str[i] == '\0')
      break;
    else if (str[i] < ' ')
      str[i] = ' ';
    else if (str[i] > '~')
      str[i] = ' ';
    else if (str[i] == '"' || str[i] == '\'' || str[i] == '&' || str[i] == '\\')
      str[i] = ' ';
  }
}

void LCD_print(const __FlashStringHelper *line1, const __FlashStringHelper *line2, uint32_t delayMs)
{
#ifdef LCD_ENABLE
  lcd.clear();
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
  delay(delayMs);
  wdt_reset();
#else
  (void)line1;
  (void)line2;
  delay(delayMs);
  wdt_reset();
#endif
}