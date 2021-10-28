/** @file Timer.cpp 
 *  @brief Header to timer functions
 */

#include <Timer.h>

/*******************************************************************************
   getElapsedTime
****************************************************************************/
/**
 * @brief Get the elapsed time since last timer reset.
 * @param interval The interval to check.
 * @return The elapsed time.
*******************************************************************************/
uint32_t Timer::getElapsedTime(void)
{
    uint32_t currentMillis = millis();
    uint32_t elapsedTime;

    /* Verifica overflow */
    if(currentMillis < this->previousMillis)
    {
        /* Obtém a diferença considerando o overflow */
        elapsedTime = UINT32_MAX - this->previousMillis + currentMillis;
    }
    else
    {
        /* Apenas a diferença simples */
        elapsedTime = currentMillis - this->previousMillis;
    }

    return elapsedTime;
}