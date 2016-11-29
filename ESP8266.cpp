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
 * @param ap_list The AP list.
 * @return true if connection with the AP was successful.\n
           FALSE if could not connect.
*******************************************************************************/
int ESP8266::listAP(esp_wifi_config_t* ap_list, int ap_list_size)
{
    int n_aps = 0;
    char serialBuffer[100];

    /* Limpeza de vari�veis */
    memset(ap_list,0,sizeof(esp_wifi_config_t)*ap_list_size);
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
                ap_list[n_aps].SSID = String(tkn);
                //strncpy(ap_list[n_aps].SSID, tkn, sizeof(ap_list[n_aps].SSID) - 1);
            else
                break;
            
            /* O segundo ap�s a virgula */
            tkn = strtok(NULL, ",");
            if(tkn)
                ap_list[n_aps].signal = String(tkn);
                //strncpy(ap_list[n_aps].signal, tkn, sizeof(ap_list[n_aps].signal) - 1);
            else
                break;
            
            /* Incrementa lista */
            n_aps++;

            if(n_aps > ap_list_size-1)
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
                if (ap_list[j].signal.toInt() < ap_list[j + 1].signal.toInt())
                {
                    esp_wifi_config_t temp = ap_list[j];
                    ap_list[j] = ap_list[j + 1];
                    ap_list[j + 1] = temp;
                }
            }
        }
    }

    serial_flush();
    return n_aps;
}

/*******************************************************************************
   CONNECTAP
****************************************************************************//**
 * @brief Connect with the AP using the SSID and password.
 * @param esp_wifi_config_t Configurations of the AP.
 * @return true if connection with the AP was successful.\n
           FALSE if could not connect.
*******************************************************************************/
bool ESP8266::connectAP(esp_wifi_config_t &wifiConfig)
{
    char strBuffer[50];
    
    /* AT: Desconecta do AP */
    serial_flush();
    Serial.write("AT+CWQAP\r\n");
    serial_get((char*) "OK\r\n", 100); /* timeout: 100ms */
    delay(100);

    /* AT: Connectar com AP */
    serial_flush();
    sprintf(strBuffer, "AT+CWJAP_DEF=\"%s\",\"%s\"\r\n", wifiConfig.SSID.c_str(), wifiConfig.password.c_str());
    Serial.write(strBuffer); 
    serial_get((char*) "\r\n", 10000, strBuffer); /* timeout: 10s */
    
    return strstr(strBuffer,"WIFI CONNECTED\r\n"); // Valor esperado: 'WIFI CONNECTED'.
}

/*******************************************************************************
   CONFIG
****************************************************************************//**
 * @brief Inicialization of the ESP8266 WiFi module.
 * @param void
 * @return true if passed sanity check.\n
           false if opposite.
*******************************************************************************/
bool ESP8266::config(void)
{
    uint32_t delayMS = 50;
      
  	/* Inicializa serial em 115200 baud/s */
  	Serial.end();
  	Serial.begin(115200);
  	
    /* Reset do m�dulo WiFi */
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
	
#ifdef ESP_SERVER
    /* Modo 'client' & 'server' */
    Serial.write("AT+CWMODE_DEF=2\r\n");
#else
    /* Modo 'client' apenas */
    Serial.write("AT+CWMODE_DEF=1\r\n");
#endif
    if(!serial_get((char*) "OK\r\n",100))
		    return false;

	  /* Conexoes multiplas = TRUE */
    Serial.write("AT+CIPMUX=1\r\n");
    if(!serial_get((char*) "OK\r\n",100))
        return false;

#ifdef ESP_SERVER
    char strBuffer[100];

	  /* Configurar modo AP */
    sprintf(strBuffer, "AT+CWSAP_DEF=\"%s\",\"%s\",6,0\r\n", ESP_SERVER_SSID, ESP_SERVER_PASSWORD);
	  Serial.write(strBuffer);
    if(!serial_get((char*) "OK\r\n",100))
      return false;
	
	  /* Inicializa modo AP */
    sprintf(strBuffer, "AT+CIPAP=\"%s\"\r\n", ESP_SERVER_IP);
	  Serial.write(strBuffer);
    if(!serial_get((char*) "OK\r\n",100))
       return false;
	
	  /* Inicializar AP */
    sprintf(strBuffer, "AT+CIPSERVER=1,%s\r\n", ESP_SERVER_PORT);
	  Serial.write(strBuffer);
	  if(!serial_get((char*) "OK\r\n",100))
      return false;
#endif
  
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
   CONNECTSERVER
****************************************************************************//**
 * @brief Connect the ESP8266 WiFi module to the cloud.
 * @param host The IP or URL of the host.
 * @param port The port of the connection.
 * @return true if connection with the server was successful.\n
           FALSE if oposite.
*******************************************************************************/
bool ESP8266::connectServer(server_parameter_t &server)
{
    String strBuffer;
    strBuffer = "AT+CIPSTART=0,\"TCP\",\"" + server.host + "\"," + server.port + "\r\n";
    
    /* Conecta ao servidor */
    serial_flush();
    Serial.write(strBuffer.c_str()); //AT: Connect to the cloud.
    return serial_get((char*) "CONNECT\r\n",1000); // Valor esperado: 'CONNECT'. Timeout: 1s.
}

/*******************************************************************************
   SENDSERVER
****************************************************************************//**
 * @brief Send string data to the cloud or to the local server through the ESP8266 WiFi module.
 * @param messagePayload The message to be sent.
 * @param SERVER Controls settings for diferent servers:\n
          "SERVER_WEB": Headers + payload / Parameters: 'action' & 'message' / ACK: 'success'.\n
          "SERVER_LOCAL" & "SERVER_BACKUP": Payload only / Parameters: none / ACK: 'LOCAL: MESSAGE RECEIVED'.
 * @param messagesAmount The number of messages to send.
 * @return true if the ACK was received.\n
           FALSE if connection failed or no ACK was received.
*******************************************************************************/
bool ESP8266::sendServer(const char* messagePayload, const char* messageHeaders, server_parameter_t &server, HTML_Method method)
{
  	String headers;
  	String espSend;
  	
  	/************************************* HEADER *************************************/
  	/* Formatação do cabeçalho */
    switch(method)
    {
      case HTML_GET:
      {
        headers  = "GET " + server.path + " HTTP/1.1" + "\r\n";
      } break;
      case HTML_POST:
      {
        headers  = "POST " + server.path + " HTTP/1.1" + "\r\n";
      } break;
      case HTML_RESPONSE_OK:
      {
        headers  = "HTTP/1.1 200 OK\r\n";
      } break;
    }
    
    headers += "Host: " + server.host + "\r\n";
    if(strlen(messageHeaders))
      headers += messageHeaders;
    //headers += "Connection: close\r\n";  
    headers += "Content-Length: " + String(strlen(messagePayload)) + String("\r\n\r\n");

    int totalSize = headers.length() + strlen(messagePayload) + 4; /* The last 4 is for 'CR' + 'LF' + 'CR' + 'LF' in FOOTER */
    espSend = "AT+CIPSEND=0," + String(totalSize) + String("\r\n");
  	
  	serial_flush();
  	Serial.write(espSend.c_str());    	/* AT: Send command */
  	serial_get((char*) ">", 100);       /* Aguarda '>' */
  	Serial.write(headers.c_str());      /* Headers */  
  
    /************************************* BODY *************************************/
    if(strlen(messagePayload))
      Serial.write(messagePayload);    		/* Payload */

    /************************************* FOOTER *************************************/
  	Serial.write("\r\n\r\n");             	/* End of HTTP POST: 'CR' + 'LF' + 'CR' + 'LF' */ 

    return serial_get((char*) "SEND OK", 1000); // Valor esperado: variável 'confirmationString'. Timeout: 1s.
}

/*******************************************************************************
   CLOSESERVER
****************************************************************************//**
 * @brief Close the ESP8266 WiFi module connection with the cloud.
 * @param void
 * @return true if no problems occured.\n
           FALSE if could not close connection.
*******************************************************************************/
bool ESP8266::closeServer(void)
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
           // FALSE if could not connect with the time server.
// *******************************************************************************/
// bool ESP8266::ESP_get_time(uint8_t *dayTimePointer)
// {  
    // char host[4][25] = {"time.nist.gov",     //Server: time.nist.gov.
                        // "time-nw.nist.gov",  //Server: time-nw.nist.gov.
                        // "time-a.nist.gov",   //Server: time-a.nist.gov.
                        // "time-b.nist.gov"};  //Server: time-b.nist.gov.
    
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
