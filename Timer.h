/** @file Timer.h
 *  @brief Header to timer functions
 */

#include <Arduino.h>

class Timer
{
public:
    uint32_t getElapsedTime(void);
    bool checkIntervalPassed(uint32_t interval) { return this->getElapsedTime() > interval; }
    void resetTimer(void) { previousMillis = millis(); }

private:
    uint32_t previousMillis = millis();
};
