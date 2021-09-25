/** @file Energy.h 
 *  @brief Header to the energy calculations.
 */

#ifndef _ENERGY_H_
#define _ENERGY_H_

/*************************************************************************************
* Includes
*************************************************************************************/
#include "Arduino.h"

/*************************************************************************************
* Macros
*************************************************************************************/
#define ENERGY_DEFAULT_LINE_VOLTAGE_VOLTS (127)
#define ENERGY_DEFAULT_POWER_FACTOR_PERCENT (87)
#define ENERGY_DEFAULT_SCALE (50) /* 50A - 1V */
#define ENERGY_DEFAULT_DATA_SIZE (500)
#define ENERGY_DEFAULT_KWH_BASE_PRICE (0.828844) /* R$/kWh */
#define ENERGY_DEFAULT_KWH_FLAG_PRICE (0.142)	 /* R$/kWh */

/*************************************************************************************
* Public prototypes
*************************************************************************************/
class Energy
{
public:
	explicit Energy(uint8_t channel) { this->channel = channel; }

	bool measure(void);
	bool calculate(uint32_t currentTimestamp);

	float getElectricCurrentAmperes(void) { return this->currentAmperes; }
	float getEnergyAccumulatedKiloWattsHour(void) { return this->energyAccumulatedKiloWattsHour; }
	float getCostAccumulatedReais(void) { return this->costAccumulatedReais; }
	uint32_t getRmsCount(void) { return this->rmsCount; }
	float getRmsSum(void) { return this->rmsSum; }

	struct Config
	{
		uint16_t dataSize;
		uint16_t scale;
		uint8_t lineVoltage;
		uint8_t powerFactor;
		uint8_t reserved0;
		uint8_t reserved1;
		float basePrice;
		float flagPrice;
	};

	Config config = {
		ENERGY_DEFAULT_DATA_SIZE,
		ENERGY_DEFAULT_SCALE,
		ENERGY_DEFAULT_LINE_VOLTAGE_VOLTS,
		ENERGY_DEFAULT_POWER_FACTOR_PERCENT,
		0,
		0,
		ENERGY_DEFAULT_KWH_BASE_PRICE,
		ENERGY_DEFAULT_KWH_FLAG_PRICE,
	};

private:
	uint8_t channel;

	uint32_t lastTimestamp = 0;
	float currentAmperes = 0;
	float energyAccumulatedKiloWattsHour = 0;
	float costAccumulatedReais = 0;
	float rmsSum = 0;
	uint32_t rmsCount = 0;
};

#endif /* _ENERGY_H_ */
