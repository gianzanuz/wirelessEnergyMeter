/** @file ADS1115.h 
 *  @brief Header to the ADS1115 I2C 16bits ADC.
 */

#ifndef _ADS1115_H_
#define _ADS1115_H_

/*************************************************************************************
* Includes
*************************************************************************************/
#include "Arduino.h"

/*************************************************************************************
* Public macros
*************************************************************************************/
#define ADS1115_max_buffer_size (10)

/*************************************************************************************
* Public prototypes
*************************************************************************************/
class ADS1115
{
public:
	/*************************************************************************************
	* Public enumeration
	*************************************************************************************/
	/* I²C ADDRESS (7 bits) */
	typedef enum ADS1115_i2c_address_tag
	{
		ADDR_GND = 0x48,
		ADDR_VDD = 0x49,
		ADDR_SDA = 0x4A,
		ADDR_SCL = 0x4B
	} ADS1115_i2c_address_t;

	/* POINTER REGISTER */
	typedef enum ADS1115_reg_pointer_tag
	{
		REG_CONVERSION = 0,
		REG_CONFIG,
		REG_LO_THRESH,
		REG_HI_THRESH,
	} ADS1115_reg_pointer_t;

	/* FIRST CONFIG BYTE */
	typedef enum ADS1115_status_tag
	{
		OS_N_EFF = 0x00,
		OS_SINGLE_CONV = 0x80
	} ADS1115_status_t;

	typedef enum ADS1115_mux_config_tag
	{
		MUX_0_1 = 0x00,
		MUX_0_3 = 0x10,
		MUX_1_3 = 0x20,
		MUX_2_3 = 0x30,
		MUX_0_GND = 0x40,
		MUX_1_GND = 0x50,
		MUX_2_GND = 0x60,
		MUX_3_GND = 0x70
	} ADS1115_mux_config_t;

	typedef enum ADS1115_gain_tag
	{
		PGA_6144 = 0x00,
		PGA_4096 = 0x02,
		PGA_2048 = 0x04,
		PGA_1024 = 0x06,
		PGA_0512 = 0x08,
		PGA_0256 = 0x0A
	} ADS1115_gain_t;

	typedef enum ADS1115_mode_tag
	{
		MODE_CONT = 0x00,
		MODE_SINGLE = 0x01
	} ADS1115_mode_t;

	/* SECOND CONFIG BYTE */
	typedef enum ADS1115_rate_tag
	{
		DR_008 = 0x00,
		DR_016 = 0x20,
		DR_032 = 0x40,
		DR_064 = 0x60,
		DR_128 = 0x80,
		DR_250 = 0xA0,
		DR_475 = 0xC0,
		DR_860 = 0xE0
	} ADS1115_rate_t;

	typedef enum ADS1115_comp_mode_tag
	{
		COMP_MODE_TRADITIONAL = 0x00,
		COMP_MODE_WINDOW = 0x10
	} ADS1115_comp_mode_t;

	typedef enum ADS1115_comp_polarity_tag
	{
		COMP_POL_ACTIVE_LOW = 0x00,
		COMP_POL_ACTIVE_HIGH = 0x08
	} ADS1115_comp_polarity_t;

	typedef enum ADS1115_latching_comp_tag
	{
		COMP_LATCH_OFF = 0x00,
		COMP_LATCH_ON = 0x04
	} ADS1115_latching_comp_t;

	typedef enum ADS1115_comp_queue_tag
	{
		COMP_QUE_ONE_CONV = 0x00,
		COMP_QUE_TWO_CONV = 0x01,
		COMP_QUE_FOUR_CONV = 0x02,
		COMP_DISABLE = 0x03
	} ADS1115_comp_queue_t;

	/*************************************************************************************
	* Public struct
	*************************************************************************************/
	/* Struct de configuração */
	typedef struct ADS1115_config_tag
	{
		ADS1115_i2c_address_t i2c_addr;
		ADS1115_status_t status;
		ADS1115_mux_config_t mux;
		ADS1115_gain_t gain;
		ADS1115_mode_t mode;
		ADS1115_rate_t rate;
		ADS1115_comp_mode_t comp_mode;
		ADS1115_comp_polarity_t comp_polarity;
		ADS1115_latching_comp_t comp_latching;
		ADS1115_comp_queue_t comp_queue;
	} ADS1115_config_t;

	/* Struct de dados */
	typedef struct ADS1115_data_tag
	{
		ADS1115_i2c_address_t i2c_addr;
		uint16_t data_size;
		int16_t data_byte[ADS1115_max_buffer_size];
	} ADS1115_data_t;

	/*************************************************************************************
	* Public prototypes
	*************************************************************************************/
	void config(ADS1115_config_t *config);
	void readData(ADS1115_data_t *data);

private:
	/*************************************************************************************
	* Private variables
	*************************************************************************************/
};

#endif /* _ADS1115_H_ */
