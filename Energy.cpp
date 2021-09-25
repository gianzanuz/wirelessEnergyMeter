/** @file Energy.c 
 *  @brief Functions related with the Energy I2C 16bits ADC.
 */

#include "Energy.h"
#include "ADS1115.h"

/*******************************************************************************
   measure
****************************************************************************/
/**
 * @brief Read raw data from the Energy.
 * @param data Output buffer.
 * @param size Size of the buffer to read.
 * @return void
*******************************************************************************/
bool Energy::measure()
{
    /* Insere configurações do primeiro ADS */
    /* Pino de endereço I2C = GND */
    /* Canal diferencial = A0 - A1 ou A2 - A3 */
    /* Conversão contínua */
    /* PGA FS = +-2048mV */
    /* 860 samples/seg */
    /* Comparador desabilitado */
    ADS1115 ads;
    ADS1115::ADS1115_config_t ADS1115Config = {
        ADS1115::ADDR_GND,
        ADS1115::OS_N_EFF,
        (this->channel == 0) ? ADS1115::MUX_0_1 : ADS1115::MUX_2_3,
        ADS1115::PGA_2048,
        ADS1115::MODE_CONT,
        ADS1115::DR_860,
        ADS1115::COMP_MODE_TRADITIONAL,
        ADS1115::COMP_POL_ACTIVE_LOW,
        ADS1115::COMP_LATCH_OFF,
        ADS1115::COMP_DISABLE,
    };
    ads.config(&ADS1115Config);

    /* Remove primeiras 10 amostras */
    ADS1115::ADS1115_data_t ADS1115Data = {
        ADS1115::ADDR_GND,
        10,
    };
    ads.readData(&ADS1115Data);

    /* Realiza a leitura das amostras */
    float sum = 0;
    for (uint16_t i = 0; i < this->config.dataSize; i++)
    {
        /* Leitura do valor do ADC */
        /* Solicita 1 amostra */
        ADS1115Data.data_size = 1;
        ads.readData(&ADS1115Data);

        /* Realiza os cálculos */
        float adc = ADS1115Data.data_byte[0] * 0.0625; /* 0.0625 = 2048.0/32768.0 */
        sum += adc * adc;
    }
    this->rmsSum = sqrt(sum / this->config.dataSize) * this->config.scale * 1e-3; /* Corrente RMS [A] */
    this->rmsCount++;

    return true;
}

/*******************************************************************************
   measure
****************************************************************************/
/**
 * @brief Read raw data from the Energy.
 * @param data Output buffer.
 * @param size Size of the buffer to read.
 * @return void
*******************************************************************************/
bool Energy::calculate(uint32_t currentTimestamp)
{
    if (this->rmsCount == 0 || currentTimestamp < this->lastTimestamp)
    {
        this->currentAmperes = 0;
        this->energyAccumulatedKiloWattsHour = 0;
        this->lastTimestamp = currentTimestamp;

        return false;
    }

    /* Calcula o valor de corrente elétrica média no período, em amperes */
    this->currentAmperes = this->rmsSum / this->rmsCount;
    this->rmsSum = 0;
    this->rmsCount = 0;

    /* Verifica se houve troca no dia */
    /* TODO */

    /* Obtém o acumulado de corrente elétrica no perído, em amperes-hora */
    //float currentAccumulatedAmperesHour = this->currentAmperes * (currentTimestamp - this->lastTimestamp)/3600;

    /* Obtém a potência elétrica média da carga no período, em kW */
    float electricPowerKiloWatts = this->currentAmperes * this->config.lineVoltage * this->config.powerFactor / 100000;

    /* Obtém a energia elétrica no período, em kilowatts-hora */
    float energyKiloWattsHour = electricPowerKiloWatts * (currentTimestamp - this->lastTimestamp) / 3600;

    /* Obtém o acumulado de energia elétrica no dia, em kilowatts-hora */
    this->energyAccumulatedKiloWattsHour += energyKiloWattsHour;

    /* Obtém o valor total em R$ gasto no dia */
    this->costAccumulatedReais = this->energyAccumulatedKiloWattsHour * (this->config.basePrice + this->config.flagPrice);

    /* Atualiza a timestamp */
    this->lastTimestamp = currentTimestamp;

    return true;
}
