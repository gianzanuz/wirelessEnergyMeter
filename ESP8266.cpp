/** @file ESP8266.c 
 *  @brief Functions related with the ESP8266 WiFi module.
 */
#include "ESP8266.h"

#ifdef ESP8266_ENABLE

/*************************************************************************************
* Private enumeration
*************************************************************************************/
enum sleep_wakeup_t
{
	SLEEP = 0,
	WAKEUP
};

/*******************************************************************************
   ESP8266
****************************************************************************//**
 * @brief Constructor: set enable pin.
 * @param void
 * @return void
*******************************************************************************/
ESP8266::ESP8266(int pin)
{
	enablePin = pin;
	return;
}

/*******************************************************************************
   LISTAP
****************************************************************************//**
 * @brief List all the available AP.
 * @param apList The AP list.
 * @return true if connection with the AP was successful.\n
           FALSE if could not connect.
*******************************************************************************/
int ESP8266::getAPList(esp_AP_list_t* apList, int apList_size)
{
    int n_aps = 0;
    char serialBuffer[100];

    /* Limpeza de vari�veis */
    memset(apList,0,sizeof(esp_AP_parameter_t)*apList_size);
    memset(serialBuffer,0,sizeof(serialBuffer));

    /* Obt�m lista de APs dispon�veis, separando os par�metros obtidos da lista */
    serial_flush();
    Serial.write("AT+CWLAP\r\n");
    serial_get((char*) "\r\n", 1000, serialBuffer, sizeof(serialBuffer));
    do
    {
        char* tkn = strtok(serialBuffer, "\"");
        if(tkn)
        {
            /* Primeiro token est� entre aspas */
            tkn = strtok(NULL, "\"");
            if(tkn)
            {
                strncpy(apList[n_aps].ssid, tkn, sizeof(apList[n_aps].ssid));
                apList[n_aps].ssid[sizeof(apList[n_aps].ssid) - 1] = '\0';
            }
            else
                break;
            
            /* O segundo ap�s a virgula */
            tkn = strtok(NULL, ",");
            if(tkn)
                apList[n_aps].rssi = (int16_t) atoi(tkn);
            else
                break;
            
            /* Incrementa lista */
            n_aps++;

            if(n_aps > apList_size-1)
                break;
        }
        else
            break;

        /* Obt�m pr�xima rede */
        memset(serialBuffer,0,sizeof(serialBuffer));
        serial_get((char*) "\r\n", 100, serialBuffer, sizeof(serialBuffer));

    } while(true);
    
    /* Caso foi descoberto pelo menos 1 ponto de acesso */
    if(n_aps>0)
    {
        /* Organizar pela qualidade do sinal */
        for(int i=1; i<n_aps; i++)
        {
            for(int j=0; j<n_aps-i; j++) 
            {
                if(apList[j].rssi < apList[j + 1].rssi)
                {
                    esp_AP_list_t temp = apList[j];
                    apList[j] = apList[j + 1];
                    apList[j + 1] = temp;
                }
            }
        }
    }

    serial_flush();
    return n_aps;
}

/*******************************************************************************
   setAP
****************************************************************************//**
 * @brief Connect with the AP using the SSID and password.
 * @param esp_wifi_config_t Configurations of the AP.
 * @return true if connection with the AP was successful.\n
           FALSE if could not connect.
*******************************************************************************/
bool ESP8266::setAP(esp_mode_t mode, const esp_AP_parameter_t &AP)
{
    switch(mode)
    {
      case ESP_CLIENT_MODE:
      {
        /* AT: Desconecta do AP */
        serial_flush();
        Serial.write("AT+CWQAP\r\n");
        serial_get("OK\r\n", 500, NULL, 0);
        delay(500);
        
        /* AT: Connectar com AP */
        serial_flush();
        Serial.write("AT+CWJAP_DEF=\"");
        Serial.write(AP.ssid);
        Serial.write("\",\"");
        Serial.write(AP.password);
        Serial.write("\"\r\n");
        return serial_get("OK\r\n", 2500, NULL, 0); /* timeout: 10s */
      } break;
      
      case ESP_SERVER_MODE:
      {
        /* Configurar modo AP */
        Serial.write("AT+CWSAP_DEF=\"");
        Serial.write(AP.ssid);
        Serial.write("\",\"");
        Serial.write(AP.password);
        Serial.write("\",6,0\r\n");
        return serial_get((char*) "OK\r\n", 100, NULL, 0);    
      } break;
      
      default:
        return false;
      break;
    }
}

/*******************************************************************************
   config
****************************************************************************//**
 * @brief Inicialization of the ESP8266 WiFi module.
 * @param void
 * @return true if passed sanity check.\n
           false if opposite.
*******************************************************************************/
bool ESP8266::config(esp_mode_t mode)
{
    char strBuffer[100];
    uint32_t delayMS = 250;

    /* Inicializa serial em 9600 baud/s */
    Serial.end();
    Serial.begin(57600);
    
    /* Reset do módulo WiFi */
    DESATIVA_ESP;
    delay(delayMS);
    ATIVA_ESP;
    /* Delay para inicializa��o */
    delay(1000);

    /* Remove mensagem de eco da serial */
    Serial.print("ATE0\r\n");
    delay(delayMS);
                                          
    /* Teste de sanidade do m�dulo ESP8266 */
    /* Valor esperado: 'OK'. Timeout: 100ms. */
    serial_flush();
    Serial.write("AT\r\n");
    if(!serial_get((char*) "OK\r\n", 100, NULL, 0))
        return false;

    /* Conexoes multiplas = TRUE */
    Serial.write("AT+CIPMUX=1\r\n");
    if(!serial_get((char*) "OK\r\n", 100, NULL, 0))
        return false;

    /* Define modo de operação */
    /* 'client' / 'server' / 'client' & 'server' */
    sprintf(strBuffer, "AT+CWMODE_DEF=%d\r\n", mode);
    Serial.write(strBuffer);
    if(!serial_get((char*) "OK\r\n", 100, NULL, 0))
	      return false;

	  serial_flush();
    return true;
}

/*******************************************************************************
   CHECKWIFI
****************************************************************************//**
 * @brief Checks if connected with the AP.
 * @param retries Number of times the command is sent.
 * @param delayMS The delay between each retry, in miliseconds.
 * @return true if connected with the AP.
           FALSE if opposite.
*******************************************************************************/
bool ESP8266::checkWifi(uint32_t retries, uint32_t delayMS, esp_AP_parameter_t &AP)
{
    char strBuffer[100];
    
    /* Envia comando para verificar se conectado, pelo numero de vezes dado por 'retries'*/
    for(uint32_t i=0;i<retries;i++) 
    {
        serial_flush();
        Serial.write("AT+CWJAP_DEF?\r\n");
        serial_get((char*) "\r\n", 100, strBuffer, sizeof(strBuffer)); // Timeout: 100ms.

        if(strstr(strBuffer,"+CWJAP_DEF:")) // Valor esperado: '+CWJAP_DEF:'.
            return true;
        else
            delay(delayMS);
    }
    
    return false;
}

/*******************************************************************************
   CONNECTurl
****************************************************************************//**
 * @brief Connect the ESP8266 WiFi module to the cloud.
 * @param host The IP or URL of the host.
 * @param port The port of the connection.
 * @return true if connection with the url was successful.\n
           FALSE if oposite.
*******************************************************************************/
bool ESP8266::connect(esp_URL_parameter_t &url)
{
    char strBuffer[100];
    sprintf(strBuffer, "AT+CIPSTART=0,\"TCP\",\"%s\",%u\r\n", url.host, url.port);
    
    /* Conecta ao servidor */
    serial_flush();
    Serial.write(strBuffer); //AT: Connect to the cloud.
    return serial_get((char*) "CONNECT\r\n", 1000, NULL, 0); // Valor esperado: 'CONNECT'. Timeout: 1s.
}

/*******************************************************************************
   CONNECTurl
****************************************************************************//**
 * @brief Connect the ESP8266 WiFi module to the cloud.
 * @param host The IP or URL of the host.
 * @param port The port of the connection.
 * @return true if connection with the url was successful.\n
           FALSE if oposite.
*******************************************************************************/
bool ESP8266::server(esp_URL_parameter_t &url)
{
    /* Inicializa modo AP */
    char strBuffer[100];
    sprintf(strBuffer, "AT+CIPAP=\"%s\"\r\n", url.host);
    Serial.write(strBuffer);
    if(!serial_get((char*) "OK\r\n", 100, NULL, 0))
       return false;
  
    /* Inicializar AP */
    sprintf(strBuffer, "AT+CIPSERVER=1,%u\r\n", url.port);
    Serial.write(strBuffer);
    return serial_get((char*) "OK\r\n", 100, NULL, 0);
}

/*******************************************************************************
   CLOSEurl
****************************************************************************//**
 * @brief Close the ESP8266 WiFi module connection with the cloud.
 * @param void
 * @return true if no problems occured.\n
           FALSE if could not close connection.
*******************************************************************************/
bool ESP8266::close(uint8_t connection)
{
    serial_flush();
    Serial.print("AT+CIPCLOSE=");
    Serial.print(connection);
    Serial.print("\r\n");
    return serial_get((char*) "CLOSED\r\n", 100, NULL, 0); // Valor esperado: "CLOSED". Timeout: 100ms.
}

/*******************************************************************************
   sleep
****************************************************************************//**
 * @brief Puts the ESP8266 WiFi module in deep sleep / Wakeup.
 * @param void.
 * @return void.
*******************************************************************************/
void ESP8266::sleep()
{
  /* Entra em soft-sleep */
  /* Caso n�o aceite comando, for�a pino de reset */
  serial_flush();
  Serial.write("AT+GSLP=0\r\n");
  if(!serial_get((char*) "OK\r\n", 100, NULL, 0));
      DESATIVA_ESP;
}

/*******************************************************************************
   wakeup
****************************************************************************//**
 * @brief Puts the ESP8266 WiFi module in Wakeup.
 * @param void.
 * @return void.
*******************************************************************************/
void ESP8266::wakeup()
{
  DESATIVA_ESP;
  delay(50);
  ATIVA_ESP;
  delay(250);
  
  /* Remove mensagem de eco da serial */
  Serial.write("ATE0\r\n");
  delay(50);
}

#endif  //ESP8266_ENABLE
