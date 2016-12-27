/** @file ADS1115.c 
 *  @brief Functions related with the ADS1115 I2C 16bits ADC.
 */
#include "ADS1115.h"
#include <Wire.h>

#ifdef ADS1115_ENABLE
/*******************************************************************************
   config
****************************************************************************//**
 * @brief Inicialization of the ADS1115 module.
 * @param void
 * @return true if passed sanity check.\n
           false if opposite.
*******************************************************************************/
void ADS1115::config(void)
{
  	uint8_t configBytes[2];
  	configBytes[0] = OS_N_EFF | MUX_0_1 | PGA_0256 | MODE_CONT; /* FIRST CONFIG BYTE */
  	configBytes[1] = DR_860 | COMP_DISABLE; 					/* SECOND CONFIG BYTE */

    /* Configuração */
    Wire.beginTransmission(ADDR);
    Wire.write(REG_CONFIG); /* POINTER REGISTER */
    Wire.write(configBytes[0]);
    Wire.write(configBytes[1]);
    Wire.endTransmission();
}

/*******************************************************************************
   readData
****************************************************************************//**
 * @brief Read raw data from the ADS1115.
 * @param data Output buffer.
 * @param size Size of the buffer to read.
 * @return void
*******************************************************************************/
void ADS1115::readData(int16_t* data, uint16_t size)
{
	  /* Indica registrador leitura */
    Wire.beginTransmission(ADDR);
    Wire.write(REG_CONVERSION); /* POINTER REGISTER */
    Wire.endTransmission();

  	/* Obtém a quantidade de amostras solicitada */
  	for(int i=0; i<size; i++)
    {
        /* Recepção */
        /* Req. Leitura de 2 Bytes */
        Wire.requestFrom(ADDR, 2);
        while(!Wire.available()) {};
        data[i] |= (Wire.read() << 8);
        data[i] |=  Wire.read();
    }
}
#endif  /* ADS1115_ENABLE */
