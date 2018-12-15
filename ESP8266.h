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
    Configuration urls for WiFi communication. 
    @{ */

/*************************************************************************************
* Public macros
*************************************************************************************/
#define ESP8266_ENABLE     /**< Enable/Disable the WiFi ESP8266 module, which transmits the packages to the urls */        
        
#ifdef ESP8266_ENABLE  
  #define DESATIVA_ESP  digitalWrite(enablePin,LOW);
  #define ATIVA_ESP     digitalWrite(enablePin,HIGH);
  
  #define ESP_SLEEP       /**< Enable/Disable the module entering deep-sleep */

  #define ESP_SERVER_SSID               "ESP8266_AP"
  #define ESP_SERVER_PASSWORD           "1234"
  #define ESP_SERVER_IP_DEFAULT         "192.168.1.1"
  #define ESP_SERVER_PATH_DEFAULT       ""
  #define ESP_SERVER_PORT_DEFAULT       "80"

  #define ESP_CLIENT_SSID               "WiFi WaFer"
  #define ESP_CLIENT_PASSWORD           "moussedebanana"
  #define ESP_CLIENT_IP_DEFAULT         "api.thingspeak.com"
  #define ESP_CLIENT_PATH_DEFAULT       "/update"
  #define ESP_CLIENT_PORT_DEFAULT       "80"

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
    enum HTML_method_t
    {
      HTML_GET = 0,
      HTML_POST,
      HTML_RESPONSE_OK
    };

    enum esp_mode_t
    {
      ESP_CLIENT_MODE = 1,
      ESP_SERVER_MODE
    };
 
		/*************************************************************************************
		* Public struct
		*************************************************************************************/
		struct esp_AP_parameter_t
		{
			String SSID;
			String password;
			String signal;
		};

		struct esp_URL_parameter_t
		{
			String host;
			String path;
			String port;
      String key;
		};

		/*************************************************************************************
		* Public prototypes
		*************************************************************************************/
		ESP8266(int pin = 0);
		int getAPList(esp_AP_parameter_t* apList, int apList_size = 20);
		bool setAP(esp_mode_t &mode, const esp_AP_parameter_t &AP);
		bool config(esp_mode_t &mode);
		bool checkWifi(uint32_t retries, uint32_t delayMS);
		bool connect(esp_mode_t &mode, esp_URL_parameter_t &url);
    bool send(const char* messagePayload, const char* messageHeaders, esp_URL_parameter_t &url, HTML_method_t method);
		bool close(void);
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
