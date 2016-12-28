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
int ESP8266::getAPList(esp_AP_parameter_t* apList, int apList_size)
{
    int n_aps = 0;
    char serialBuffer[100];

    /* Limpeza de vari�veis */
    memset(apList,0,sizeof(esp_AP_parameter_t)*apList_size);
    memset(serialBuffer,0,sizeof(serialBuffer));

    /* Obt�m lista de APs dispon�veis, separando os par�metros obtidos da lista */
    serial_flush();
    Serial.write("AT+CWLAP\r\n");
    serial_get((char*) "\r\n", 1000, serialBuffer);
    do
    {
        char* tkn = strtok(serialBuffer, "\"");
        if(tkn)
        {
            /* Primeiro token est� entre aspas */
            tkn = strtok(NULL, "\"");
            if(tkn)
                apList[n_aps].SSID = String(tkn);
                //strncpy(apList[n_aps].SSID, tkn, sizeof(apList[n_aps].SSID) - 1);
            else
                break;
            
            /* O segundo ap�s a virgula */
            tkn = strtok(NULL, ",");
            if(tkn)
                apList[n_aps].signal = String(tkn);
                //strncpy(apList[n_aps].signal, tkn, sizeof(apList[n_aps].signal) - 1);
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
        serial_get((char*) "\r\n", 100, serialBuffer);

    } while(true);
    
    /* Caso foi descoberto pelo menos 1 ponto de acesso */
    if(n_aps>0)
    {
        /* Organizar pela qualidade do sinal */
        for(int i=1; i<n_aps; i++)
        {
            for(int j=0; j<n_aps-i; j++) 
            {
                if (apList[j].signal.toInt() < apList[j + 1].signal.toInt())
                {
                    esp_AP_parameter_t temp = apList[j];
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
bool ESP8266::setAP(esp_mode_t &mode, const esp_AP_parameter_t &AP)
{
    char strBuffer[100];

    switch(mode)
    {
      case ESP_CLIENT_MODE:
      {
        /* AT: Desconecta do AP */
        serial_flush();
        Serial.write("AT+CWQAP\r\n");
        serial_get((char*) "OK\r\n", 100); /* timeout: 100ms */
        delay(100);
    
        /* AT: Connectar com AP */
        serial_flush();
        sprintf(strBuffer, "AT+CWJAP_DEF=\"%s\",\"%s\"\r\n", AP.SSID.c_str(), AP.password.c_str());
        Serial.write(strBuffer); 
        serial_get((char*) "\r\n", 10000, strBuffer); /* timeout: 10s */
        
        return strstr(strBuffer,"WIFI CONNECTED\r\n"); // Valor esperado: 'WIFI CONNECTED'.
      } break;
      
      case ESP_SERVER_MODE:
      {    
        /* Configurar modo AP */
        sprintf(strBuffer, "AT+CWSAP_DEF=\"%s\",\"%s\",6,0\r\n", AP.SSID.c_str(), AP.password.c_str());
        Serial.write(strBuffer);
        return serial_get((char*) "OK\r\n",100);    
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
bool ESP8266::config(esp_mode_t &mode)
{
    char strBuffer[100];
    uint32_t delayMS = 50;
      
  	/* Inicializa serial em 115200 baud/s */
  	Serial.end();
  	Serial.begin(115200);
  	
    /* Reset do módulo WiFi */
    DESATIVA_ESP;
    delay(delayMS);
    ATIVA_ESP;
    /* Delay para inicializa��o */
    delay(250);
  
    /* Remove mensagem de eco da serial */
    Serial.print("ATE0\r\n");
    delay(delayMS);
    
    /* Teste de sanidade do m�dulo ESP8266 */
    /* Valor esperado: 'OK'. Timeout: 100ms. */
    serial_flush();
    Serial.write("AT\r\n");
    if(!serial_get((char*) "OK\r\n",100))
        return false;

    /* Conexoes multiplas = TRUE */
    Serial.write("AT+CIPMUX=1\r\n");
    if(!serial_get((char*) "OK\r\n",100))
        return false;

    /* Define modo de operação */
    /* 'client' / 'server' / 'client' & 'server' */
    sprintf(strBuffer, "AT+CWMODE_DEF=%d\r\n", mode);
    Serial.write(strBuffer);
    if(!serial_get((char*) "OK\r\n",100))
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
bool ESP8266::checkWifi(uint32_t retries, uint32_t delayMS)
{
    char strBuffer[100];
    
    /* Envia comando para verificar se conectado, pelo numero de vezes dado por 'retries'*/
    for(uint32_t i=0;i<retries;i++) 
    {
        serial_flush();
        Serial.write("AT+CWJAP_DEF?\r\n");
        serial_get((char*) "\r\n", 100, strBuffer); // Timeout: 100ms.

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
bool ESP8266::connect(esp_mode_t &mode, esp_URL_parameter_t &url)
{
    char strBuffer[100];

    switch(mode)
    {
      case ESP_CLIENT_MODE:
      {
        sprintf(strBuffer, "AT+CIPSTART=0,\"TCP\",\"%s\",%s\r\n", url.host.c_str(), url.port.c_str());
        
        /* Conecta ao servidor */
        serial_flush();
        Serial.write(strBuffer); //AT: Connect to the cloud.
        return serial_get((char*) "CONNECT\r\n",1000); // Valor esperado: 'CONNECT'. Timeout: 1s.
      } break;

      case ESP_SERVER_MODE:
      {
          /* Inicializa modo AP */
          sprintf(strBuffer, "AT+CIPAP=\"%s\"\r\n", url.host.c_str());
          Serial.write(strBuffer);
          if(!serial_get((char*) "OK\r\n",100))
             return false;
        
          /* Inicializar AP */
          sprintf(strBuffer, "AT+CIPSERVER=1,%s\r\n", url.port.c_str());
          Serial.write(strBuffer);
          return serial_get((char*) "OK\r\n",100);
      } break;

      default:
          return false;
      break;
    }
}

/*******************************************************************************
   SENDurl
****************************************************************************//**
 * @brief Send string data to the cloud or to the local url through the ESP8266 WiFi module.
 * @param messagePayload The message to be sent.
 * @param url Controls settings for diferent urls:\n
          "url_WEB": Headers + payload / urls: 'action' & 'message' / ACK: 'success'.\n
          "url_LOCAL" & "url_BACKUP": Payload only / urls: none / ACK: 'LOCAL: MESSAGE RECEIVED'.
 * @param messagesAmount The number of messages to send.
 * @return true if the ACK was received.\n
           FALSE if connection failed or no ACK was received.
*******************************************************************************/
bool ESP8266::send(const char* messagePayload, const char* messageHeaders, esp_URL_parameter_t &url, HTML_method_t method)
{
  	String headers;
  	int totalSize;
    
  	/************************************* HEADER *************************************/
  	/* Formatação do cabeçalho */
    switch(method)
    {
      case HTML_GET:
      {
        headers  = "GET " + url.path + messageHeaders + " HTTP/1.1" + "\r\n";
        headers += "Host: " + url.host + "\r\n";

        totalSize = headers.length() + 2;  /* The last 2 is for 'CR' + 'LF' in FOOTER */
      } break;
      case HTML_POST:
      {
        headers  = "POST " + url.path + " HTTP/1.1" + "\r\n";
        headers += "Host: " + url.host + "\r\n";
        if(strlen(messageHeaders))
          headers += messageHeaders;
        headers += "Content-Length: " + String(strlen(messagePayload)) + String("\r\n\r\n");

        totalSize = headers.length() + strlen(messagePayload) + 4; /* The last 4 is for 'CR' + 'LF' + 'CR' + 'LF' in FOOTER */
      } break;
      case HTML_RESPONSE_OK:
      {
        headers  = "HTTP/1.1 200 OK\r\n";
        headers += "Host: " + url.host + "\r\n";
        if(strlen(messageHeaders))
          headers += messageHeaders;
        headers += "Content-Length: " + String(strlen(messagePayload)) + String("\r\n\r\n");

        totalSize = headers.length() + strlen(messagePayload) + 4; /* The last 4 is for 'CR' + 'LF' + 'CR' + 'LF' in FOOTER */
      } break;
    }

    char espSend[25];
    sprintf(espSend, "AT+CIPSEND=0,%d\r\n", totalSize);

  	serial_flush();
  	Serial.write(espSend);    	        /* AT: Send command */
  	serial_get((char*) ">", 100);       /* Aguarda '>' */
  	Serial.write(headers.c_str());      /* Headers */  
  
    /************************************* BODY *************************************/
    if(method == HTML_POST || method == HTML_RESPONSE_OK)
      Serial.write(messagePayload);    		/* Payload */

    /************************************* FOOTER *************************************/
    if(method == HTML_POST || method == HTML_RESPONSE_OK)
  	  Serial.write("\r\n\r\n");             	/* End of HTTP POST: 'CR' + 'LF' + 'CR' + 'LF' */ 
    else if(method == HTML_GET)
      Serial.write("\r\n");                   /* End of HTTP GET: 'CR' + 'LF' */ 

    return serial_get((char*) "SEND OK", 1000); // Valor esperado: variável 'confirmationString'. Timeout: 1s.
}

/*******************************************************************************
   CLOSEurl
****************************************************************************//**
 * @brief Close the ESP8266 WiFi module connection with the cloud.
 * @param void
 * @return true if no problems occured.\n
           FALSE if could not close connection.
*******************************************************************************/
bool ESP8266::close(void)
{
    serial_flush();
    Serial.write("AT+CIPCLOSE=0\r\n");
    return serial_get((char*) "CLOSED\r\n", 100); // Valor esperado: "CLOSED". Timeout: 100ms.
}

/*******************************************************************************
   SLEEP
****************************************************************************//**
 * @brief Puts the ESP8266 WiFi module in deep sleep / Wakeup.
 * @param mode Two modes:\n
          "SLEEP": Enters deep sleep.\n
          "WAKEUP": Wakes up the module.
 * @return true if connected with the WiFi in "WAKEUP" mode.\n
           FALSE if not connected.
*******************************************************************************/
void ESP8266::sleep(uint8_t type)
{
    /* Dois modos: "SLEEP" entrar em deep-sleep. "WAKEUP" acordar com sinal do GPIO */
    switch(type)
    {
        case SLEEP:
        {
            /* Entra em soft-sleep */
            /* Caso n�o aceite comando, for�a pino de reset */
            serial_flush();
            Serial.write("AT+GSLP=0\r\n");
            if(!serial_get((char*) "OK\r\n", 100));
                DESATIVA_ESP;
        }
        break;

        case WAKEUP:
        {
            DESATIVA_ESP;
            delay(50);
            ATIVA_ESP;
            delay(250);

            /* Remove mensagem de eco da serial */
            Serial.write("ATE0\r\n");
            delay(50);
        }
        break;
    }
}

// /*******************************************************************************
   // ESP_GET_TIME
// ****************************************************************************//**
 // * @brief This function gets the online time from the WEB.
 // * @param dayTimePointer Pointer to which the timestamp will be stored, in the form yyMMddHHmmss.
 // * @return true if timestamp is valid.
           // FALSE if could not connect with the time url.
// *******************************************************************************/
// bool ESP8266::ESP_get_time(uint8_t *dayTimePointer)
// {  
    // char host[4][25] = {"time.nist.gov",     //url: time.nist.gov.
                        // "time-nw.nist.gov",  //url: time-nw.nist.gov.
                        // "time-a.nist.gov",   //url: time-a.nist.gov.
                        // "time-b.nist.gov"};  //url: time-b.nist.gov.
    
    // char AT_CIPSTART_command[50] = {'\0'};
    // char* strConfirmation;
    // char serialBuffer[100] = {'\0'};
    // bool_t ESP_OK_status = FALSE;
    
    // for(uint8_t i=0;i<4;i++)
    // {
        // /* Conectar ao servidor. Porta padr�o: 13 */
        // sprintf(AT_CIPSTART_command,"AT+CIPSTART=\"TCP\",\"%s\",13\r\n",host[i]);
        // serial_flush(UART_2);
        // serial_send(UART_2, AT_CIPSTART_command);
        // serial_get("CLOSED\r\n", serialBuffer, 1000, UART_2); // Valor esperado: "CLOSED". Timeout: 1s.
        
        // /* Verifica se servidor enviou timestamp */
        // strConfirmation = strstr(serialBuffer,"+IPD");
        // if(strConfirmation)
        // {
            // /* Coloca string no formato AAMMDDhhmmss */
            // strConfirmation += 15;
            // for (uint8_t j=0;j<6;j++)
            // {
                // *dayTimePointer = *strConfirmation;
                // dayTimePointer++;
                // strConfirmation++;
                // *dayTimePointer = *strConfirmation;
                // dayTimePointer++;  
                // strConfirmation += 2;
            // }
            // *dayTimePointer = '\0';
            // ESP_OK_status = true;
            // break;
        // }
        // else
            // ESP_close_connection();
        
        // memset(serialBuffer,'\0',sizeof(serialBuffer));
    // }
    
    // /* ERRO: pisca 4 vezes, longo, mant�m aceso */
    // if (!ESP_OK_status)
    // {
        // for(uint8_t i=0;i<4;i++)
        // {
            // LED_TurnOffLed(VERMELHO);
            // DelayMs(300);
            // LED_TurnOnLed(VERMELHO);
            // DelayMs(300);
        // }
    // }
    // else
        // LED_TurnOffLed(VERMELHO);
    
    // return ESP_OK_status;
// }



#endif  //ESP8266_ENABLE
