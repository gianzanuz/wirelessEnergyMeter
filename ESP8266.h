/** @file ESP8266.h 
 *  @brief Header to the ESP8266 WiFi module functions.
 */

#ifndef _ESP8266_H_
#define _ESP8266_H_

/*************************************************************************************
* Includes
*************************************************************************************/
#include "Arduino.h"

/*************************************************************************************
* Public macros
*************************************************************************************/

/* Hardware */
#define ESP_ENABLE_PIN (2)
#define ESP_DESATIVA digitalWrite(enablePin, LOW)
#define ESP_ATIVA digitalWrite(enablePin, HIGH)

/* Config */
#define ESP_SLEEP /**< Enable/Disable the module entering deep-sleep */
#define ESP_AP_LIST_SIZE (5)

/* Delay */
#define ESP_SHORT_DELAY (100)
#define ESP_MEDIUM_DELAY (1000)
#define ESP_LONG_DELAY (15000)

/* Other */
#define ESP_CLOSE_ALL (5)

/*************************************************************************************
* Public prototypes
*************************************************************************************/
extern void serial_flush(void);
extern bool serial_get(const char *stringChecked, uint32_t timeout, char *serialBuffer, uint16_t serialBufferSize);

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
		ESP_SERVER_MODE,
		ESP_CLIENT_AND_SERVER_MODE,
	};

	/*************************************************************************************
	* Public struct
	*************************************************************************************/
	struct esp_AP_parameter_t
	{
		char ssid[33];
		char password[33];
	};

	struct esp_URL_parameter_t
	{
		char host[50];
		char auth[50];
		char client[30];
	};

	struct esp_AP_list_t
	{
		char ssid[33];
		int16_t rssi;
	};

	/*************************************************************************************
	* Public prototypes
	*************************************************************************************/
	ESP8266(int pin = 0);
	int getAPList(esp_AP_list_t *apList, int apList_size = 20);
	uint32_t getUnixTimestamp(void);
	bool connect_ap(const esp_AP_parameter_t &AP);
	bool set_ap(const esp_AP_parameter_t &AP);
	bool config(void);
	bool checkWifi(void);
	bool connect(const esp_URL_parameter_t &url);
	bool server_start(void);
	bool server_stop(void);
	bool close(uint8_t connection);
	void wakeup(void);
	void sleep(void);

private:
	/*************************************************************************************
	* Private variables
	*************************************************************************************/
	int enablePin;
};

#endif /* _ESP8266_H_ */
