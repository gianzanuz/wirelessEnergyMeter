/** @file ADS1115.c 
 *  @brief Functions related with the ADS1115 I2C 16bits ADC.
 */
#include "ADS1115.h"
#include <Wire.h>

/*******************************************************************************
   config
***************************************************************************/
/**
 * @brief Inicialization of the ADS1115 module.
 * @param void
 * @return true if passed sanity check.\n
           false if opposite.
*******************************************************************************/
void ADS1115::config(ADS1115_config_t *config)
{
    /* Prepara buffer de envio */
    uint8_t i2c_buffer[3];
    i2c_buffer[0] = REG_CONFIG;
    i2c_buffer[1] = config->status | config->mux | config->gain | config->mode;
    i2c_buffer[2] = config->rate | config->comp_mode | config->comp_polarity | config->comp_latching | config->comp_queue;

    /* Configuração */
    Wire.beginTransmission(config->i2c_addr);
    Wire.write(i2c_buffer[0]); /* POINTER REGISTER */
    Wire.write(i2c_buffer[1]); /* FIRST CONFIG BYTE */
    Wire.write(i2c_buffer[2]); /* SECOND CONFIG BYTE */
    Wire.endTransmission();

    /* Indica registrador leitura */
    Wire.beginTransmission(config->i2c_addr);
    Wire.write(REG_CONVERSION); /* POINTER REGISTER */
    Wire.endTransmission();
}

/*******************************************************************************
   readData
****************************************************************************/
/**
 * @brief Read raw data from the ADS1115.
 * @param data Output buffer.
 * @param size Size of the buffer to read.
 * @return void
*******************************************************************************/
void ADS1115::readData(ADS1115_data_t *data)
{
    /* Verifica se buffer solicitado é maior que o permitido */
    if ((data->data_size) > ADS1115_max_buffer_size)
        return;

    /* Contador para timeout */
    uint16_t counter = UINT16_MAX;

    /* Recebe dados do ADC */
    /* Converte uint8_t em int16_t */
    for (uint16_t i = 0; i < data->data_size; i++)
    {
        /* Recepção */
        /* Req. Leitura de 2 Bytes */
        uint8_t dataRaw[2] = {0, 0};
        Wire.requestFrom(data->i2c_addr, 2);
        while (!Wire.available())
        {
            if (0 == counter--)
                return;
        };
        dataRaw[0] = Wire.read();
        dataRaw[1] = Wire.read();
        data->data_byte[i] = (dataRaw[0] << 8) | (dataRaw[1]);
    }
}
