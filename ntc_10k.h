#ifndef NTC_10K_H
#define NTC_10K_H

#include <stdint.h>

float NTC10K_ADCToResistance(uint16_t adc);
float NTC10K_ADCToTemperatureC(uint16_t adc);

#endif