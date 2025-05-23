/** @file ESP8266.cpp
 *  @brief Functions related with the ESP8266 WiFi module.
 */
#include "ESP8266.h"

/*******************************************************************************
   ESP8266
****************************************************************************/
/**
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
   getAPList
****************************************************************************/
/**
 * @brief List all the available AP.
 * @param[in] apList The AP list.
 * @param apList_size The size of the AP list. 
 * @return True if connection with the AP was successful.
*******************************************************************************/
int ESP8266::getAPList(esp_AP_list_t *apList, int apList_size)
{
    int n_aps = 0;
    char serialBuffer[100];

    /* Limpeza de vari�veis */
    memset(apList, 0, sizeof(esp_AP_parameter_t) * apList_size);
    memset(serialBuffer, 0, sizeof(serialBuffer));

    /* Obt�m lista de APs dispon�veis, separando os par�metros obtidos da lista */
    serial_flush();
    Serial.print(F("AT+CWLAP\r\n"));
    serial_get("\r\n", ESP_MEDIUM_DELAY, serialBuffer, sizeof(serialBuffer));
    do
    {
        char *tkn = strtok(serialBuffer, "\"");
        if (tkn)
        {
            /* Primeiro token est� entre aspas */
            tkn = strtok(NULL, "\"");
            if (tkn)
            {
                strncpy(apList[n_aps].ssid, tkn, sizeof(apList[n_aps].ssid));
                apList[n_aps].ssid[sizeof(apList[n_aps].ssid) - 1] = '\0';
            }
            else
                break;

            /* O segundo ap�s a virgula */
            tkn = strtok(NULL, ",");
            if (tkn)
                apList[n_aps].rssi = (int16_t)atoi(tkn);
            else
                break;

            /* Incrementa lista */
            n_aps++;

            if (n_aps > apList_size - 1)
                break;
        }
        else
            break;

        /* Obt�m pr�xima rede */
        memset(serialBuffer, 0, sizeof(serialBuffer));
        serial_get("\r\n", ESP_SHORT_DELAY, serialBuffer, sizeof(serialBuffer));

    } while (true);

    /* Caso foi descoberto pelo menos 1 ponto de acesso */
    if (n_aps > 0)
    {
        /* Organizar pela qualidade do sinal */
        for (int i = 1; i < n_aps; i++)
        {
            for (int j = 0; j < n_aps - i; j++)
            {
                if (apList[j].rssi < apList[j + 1].rssi)
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
   getUnixTimestamp
****************************************************************************/
/**
 * @brief Gets unix timestamp from server
 * @param void.
 * @return uint32_t timestamp.
*******************************************************************************/
uint32_t ESP8266::getUnixTimestamp(void)
{
    union Timestamp
    {
        uint8_t u8[4];
        uint32_t u32;
    };
    Timestamp timestamp;

    /* Conecta ao servidor */
    serial_flush();
    Serial.print(F("AT+CIPSTART=0,\"TCP\",\"time.nist.gov\",37\r\n"));
    if (!serial_get("+IPD,0,4:", ESP_LONG_DELAY, NULL, 0))
        return 0;

    /* Obtém timestamp */
    uint8_t buffer[4];
    if (!Serial.readBytes(buffer, sizeof(buffer)))
        return 0;

    /* Converte para segundos UNIX */
    timestamp.u8[3] = buffer[0];
    timestamp.u8[2] = buffer[1];
    timestamp.u8[1] = buffer[2];
    timestamp.u8[0] = buffer[3];
    return (timestamp.u32 - 2208988800ul);
}

/*******************************************************************************
   connect_ap
****************************************************************************/
/**
 * @brief Connect with the AP using the SSID and password.
 * @param mode Mode of operation.
 * @param ap The access point to connect. 
 * @return True if connection with the AP was successful.\n
*******************************************************************************/
bool ESP8266::connect_ap(const esp_AP_parameter_t &ap)
{
    /* AT: Desconecta do AP */
    serial_flush();
    Serial.print(F("AT+CWQAP\r\n"));
    serial_get("OK\r\n", 500, NULL, 0);
    delay(500);

    /* AT: Connectar com AP */
    serial_flush();
    Serial.print(F("AT+CWJAP_DEF=\""));
    Serial.print(ap.ssid);
    Serial.print(F("\",\""));
    Serial.print(ap.password);
    Serial.print(F("\"\r\n"));

    return serial_get("OK\r\n", ESP_LONG_DELAY, NULL, 0); /* timeout: 15s */
}

/*******************************************************************************
   connect_ap
****************************************************************************/
/**
 * @brief Connect with the AP using the SSID and password.
 * @param mode Mode of operation.
 * @param ap The access point to connect. 
 * @return True if connection with the AP was successful.\n
*******************************************************************************/
bool ESP8266::set_ap(const esp_AP_parameter_t &ap)
{
    /* Configurar modo AP */
    Serial.print(F("AT+CWSAP_DEF=\""));
    Serial.print(ap.ssid);
    Serial.print(F("\",\""));
    Serial.print(ap.password);
    Serial.print(F("\",6,0\r\n"));
    
    return serial_get("OK\r\n", ESP_LONG_DELAY, NULL, 0);

}

/*******************************************************************************
   config
****************************************************************************/
/**
 * @brief Inicialization of the ESP8266 WiFi module.
 * @param void
 * @return true if passed sanity check.\n
           false if opposite.
*******************************************************************************/
bool ESP8266::config(void)
{
    uint32_t delayMS = 250;

    /* Inicializa serial em 57600 baud/s */
    Serial.end();
    Serial.begin(57600);

    /* Reset do módulo WiFi */
    ESP_DESATIVA;
    delay(delayMS);
    ESP_ATIVA;

    /* Delay para inicialização */
    delay(ESP_MEDIUM_DELAY);

    /* Remove mensagem de eco da serial */
    Serial.print("ATE0\r\n");
    delay(ESP_SHORT_DELAY);

    /* Teste de sanidade do módulo ESP8266 */
    /* Valor esperado: 'OK'. Timeout: 100ms. */
    serial_flush();
    Serial.print(F("AT\r\n"));
    if (!serial_get("OK\r\n", ESP_SHORT_DELAY, NULL, 0))
        return false;

    /* Conexoes multiplas = TRUE */
    Serial.print(F("AT+CIPMUX=1\r\n"));
    if (!serial_get("OK\r\n", ESP_SHORT_DELAY, NULL, 0))
        return false;

    /* Define modo de operação */
    /* 1 = 'client' / 2 = 'server' / 3 = 'client' & 'server' */
    Serial.print(F("AT+CWMODE=3\r\n"));
    if (!serial_get("OK\r\n", ESP_SHORT_DELAY, NULL, 0))
        return false;

    /* Tamanho do buffer SSL */
    Serial.print(F("AT+CIPSSLSIZE=6144\r\n"));
    if (!serial_get("OK\r\n", ESP_SHORT_DELAY, NULL, 0))
        return false;

    /* Atualiza o DNS */
    Serial.print(F("AT+CIPDNS_CUR=1,\"8.8.8.8\",\"1.1.1.1\"\r\n"));
    if(!serial_get("OK\r\n", ESP_SHORT_DELAY, NULL, 0))
        return false;

    /* Atualiza nome do host na rede */
    Serial.print(F("AT+CWHOSTNAME=\"ESP_WIFIWAFER\"\r\n"));
    if(!serial_get("OK\r\n", ESP_SHORT_DELAY, NULL, 0))
        return false;

    /* Inicializa modo AP */
    Serial.print(F("AT+CIPAP=\"192.168.1.1\"\r\n"));
    if (!serial_get("OK\r\n", ESP_SHORT_DELAY, NULL, 0))
        return false;

    serial_flush();
    return true;
}

/*******************************************************************************
   CHECKWIFI
****************************************************************************/
/**
 * @brief Checks if connected with the AP.
 * @return true if connected with the AP.
           FALSE if opposite.
*******************************************************************************/
bool ESP8266::checkWifi(void)
{
    char strBuffer[100];

    serial_flush();
    Serial.print(F("AT+CWJAP_DEF?\r\n"));
    serial_get("\r\n", ESP_SHORT_DELAY, strBuffer, sizeof(strBuffer));

    if (strstr(strBuffer, "+CWJAP_DEF:")) // Valor esperado: '+CWJAP_DEF:'.
        return true;

    return false;
}

/*******************************************************************************
   CONNECTurl
****************************************************************************/
/**
 * @brief Connect the ESP8266 WiFi module to the cloud.
 * @param host The IP or URL of the host.
 * @param port The port of the connection.
 * @return true if connection with the url was successful.\n
           FALSE if oposite.
*******************************************************************************/
bool ESP8266::connect(const esp_URL_parameter_t &url)
{
    /* Conecta ao servidor */
    serial_flush();
    Serial.print(F("AT+CIPSTART=0,\"SSL\",\""));
    Serial.print(url.host);
    Serial.print(F("\",443\r\n"));

    return serial_get("CONNECT\r\n", ESP_LONG_DELAY, NULL, 0); // Valor esperado: 'CONNECT'. Timeout: 15s.
}

/*******************************************************************************
   server_start
****************************************************************************/
/**
 * @brief Starts the webserver.
 * @return true if connection with the url was successful.\n
           FALSE if oposite.
*******************************************************************************/
bool ESP8266::server_start()
{
    /* Inicializar AP */
    Serial.print(F("AT+CIPSERVER=1,80\r\n"));
    if (!serial_get("OK\r\n", ESP_MEDIUM_DELAY, NULL, 0))
        return false;

    /* Obtem endereço IP */
    Serial.print(F("AT+CIFSR\r\n"));
    if (!serial_get("OK\r\n", ESP_MEDIUM_DELAY, NULL, 0))
        return false;

    return true;
}

/*******************************************************************************
   server_stop
****************************************************************************/
/**
 * @brief Stops the webserver.
 * @return true if connection with the url was successful.\n
           FALSE if oposite.
*******************************************************************************/
bool ESP8266::server_stop()
{
    /* Inicializar AP */
    Serial.print(F("AT+CIPSERVER=0\r\n"));
    if (!serial_get("OK\r\n", ESP_MEDIUM_DELAY, NULL, 0))
        return false;

    return true;
}

/*******************************************************************************
   CLOSEurl
****************************************************************************/
/**
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

    return serial_get("CLOSED\r\n", ESP_SHORT_DELAY, NULL, 0); // Valor esperado: "CLOSED". Timeout: 100ms.
}

/*******************************************************************************
   sleep
****************************************************************************/
/**
 * @brief Puts the ESP8266 WiFi module in deep sleep / Wakeup.
 * @param void.
 * @return void.
*******************************************************************************/
void ESP8266::sleep()
{
    /* Entra em soft-sleep */
    /* Caso n�o aceite comando, for�a pino de reset */
    serial_flush();
    Serial.print(F("AT+GSLP=0\r\n"));
    if (!serial_get("OK\r\n", ESP_SHORT_DELAY, NULL, 0))
        ESP_DESATIVA;
}

/*******************************************************************************
   wakeup
****************************************************************************/
/**
 * @brief Puts the ESP8266 WiFi module in Wakeup.
 * @param void.
 * @return void.
*******************************************************************************/
void ESP8266::wakeup()
{
    ESP_DESATIVA;
    delay(50);
    ESP_ATIVA;
    delay(250);

    /* Remove mensagem de eco da serial */
    Serial.print(F("ATE0\r\n"));
    delay(50);
}
