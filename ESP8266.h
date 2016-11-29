/** @file ESP8266.h 
 *  @brief Header to the ESP8266 WiFi module functions.
 */

#ifndef _ESP8266_H_
#define _ESP8266_H_

/*************************************************************************************
* Includes
*************************************************************************************/
#include "Arduino.h"

/** @defgroup WIFI_MODULE WiFi related.
    Configuration parameters for WiFi communication. 
    @{ */

/*************************************************************************************
* Public macros
*************************************************************************************/
#define ESP8266_ENABLE     /**< Enable/Disable the WiFi ESP8266 module, which transmits the packages to the servers */        
        
#ifdef ESP8266_ENABLE  
  #define DESATIVA_ESP  digitalWrite(enablePin,LOW);
  #define ATIVA_ESP     digitalWrite(enablePin,HIGH);
  
  #define ESP_SLEEP       /**< Enable/Disable the module entering deep-sleep */

  #define ESP_SERVER      /**< Enable/Disable ESP8266 as server */
  #ifdef ESP_SERVER
    #define ESP_SERVER_SSID       "ESP8266_AP"
    #define ESP_SERVER_PASSWORD   "1234"
    #define ESP_SERVER_IP         "192.168.1.1"
    #define ESP_SERVER_PORT       "80"
  #endif

/*************************************************************************************
* Public prototypes
*************************************************************************************/  
extern void serial_flush(void);
extern bool serial_get(const char* stringChecked, uint32_t timeout, char* serialBuffer = (char*) NULL);

class ESP8266
{
	public:
    /*************************************************************************************
    * Public enumeration
    *************************************************************************************/
    enum HTML_Method
    {
      HTML_GET = 0,
      HTML_POST,
      HTML_RESPONSE_OK
    };
 
		/*************************************************************************************
		* Public struct
		*************************************************************************************/
		struct esp_wifi_config_t
		{
			String SSID;
			String password;
			String signal;
		};

		struct server_parameter_t
		{
			String host;
			String path;
			String port;
		};

		/*************************************************************************************
		* Public prototypes
		*************************************************************************************/
		ESP8266(int pin = 0);
		int listAP(esp_wifi_config_t* ap_list, int ap_list_size = 20);
		bool connectAP(esp_wifi_config_t &wifiConfig);
		bool config(void);
		bool checkWifi(uint32_t retries, uint32_t delayMS);
		bool connectServer(server_parameter_t &server);
		bool sendServer(const char* messagePayload, const char* messageHeaders, server_parameter_t &server, HTML_Method method);
		bool closeServer(void);
		void sleep(uint8_t type);
	
	private:
		/*************************************************************************************
		* Private variables
		*************************************************************************************/
		int enablePin;
};

#else
  #warning WiFi desabilitado
#endif /* ESP8266_ENABLE */

/** @} */
#endif /* _ESP8266_H_ */
