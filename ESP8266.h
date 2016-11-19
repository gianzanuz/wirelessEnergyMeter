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
#define WIFI_ENABLE     /**< Enable/Disable the WiFi ESP8266 module, which transmits the packages to the servers */        
        
#ifdef WIFI_ENABLE  
  #define WIFI_SLEEP       /**< Enable/Disable the module entering deep-sleep */
#else
  #warning WiFi desabilitado
#endif /* WIFI_ENABLE */


#define DESATIVA_ESP	digitalWrite(enablePin,LOW);
#define ATIVA_ESP 		digitalWrite(enablePin,HIGH);

class ESP8266
{
	public:
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

//char* HTML_PAGE = (char*)
//"<!DOCTYPE html>\r\n\
//<html>\r\n\
//<h1>Conex�o ao Ponto de Acesso WiFi</h1>\r\n\
//<h3>Giancarlo Zanuz</h3>\r\n\
//<body>\r\n\
//<form method=\"post\">\r\n\
//	<div>\r\n\
//		<label for=\"name\">Name:</label>\r\n\
//		<input type=\"text\" id=\"name\" name=\"user_name\" />\r\n\
//	</div>\r\n\
//	<div>\r\n\
//		<label for=\"senha\">Senha:</label>\r\n\
//		<input type=\"password\" id=\"senha\" name=\"user_mail\" />\r\n\
//	</div>\r\n\
//	<div class=\"button\">\r\n\
//		<button type=\"submit\">Send your message</button>\r\n\
//	</div>\r\n\
//</form>\r\n\
//</body>\r\n\
//</html>\r\n";
	
		/*************************************************************************************
		* Public prototypes
		*************************************************************************************/
		ESP8266(int pin = 0);
		int listAP(esp_wifi_config_t* ap_list, int ap_list_size = 20);
		bool connectAP(esp_wifi_config_t &wifiConfig);
		bool config(void);
		bool checkWifi(uint32_t retries, uint32_t delayMS);
		bool connectServer(server_parameter_t &server);
		bool sendServer(String &messagePayload, server_parameter_t &server);
		bool closeServer(void);
		void sleep(uint8_t type);
	

	private:
		/*************************************************************************************
		* Private variables
		*************************************************************************************/
		int enablePin;
		
		
		
		
};

/*************************************************************************************
* Public variables
*************************************************************************************/
/* Vari�veis de status */
//extern bool_t status_awake_ESP;        /**< Flag to identify if the module is asleep/awake */   
//extern server_parameter_t ServerParameter[4];  /**< Struct that holds the parameters of the servers */

/** @} */
#endif /* _ESP8266_H_ */
