#include <math.h>
#include <stdint.h>
#include "ntc_10k.h"

/*
    NTC 10K B3950 calculation

    Wiring used:
    3.3V --- NTC --- ADC pin --- 10k fixed resistor --- GND

    ADC is 12-bit:
    0     = 0V
    4095  = 3.3V
*/

#define ADC_MAX_COUNTS     4095.0f
#define NTC_R_FIXED_OHM    10000.0f
#define NTC_R25_OHM        10000.0f
#define NTC_BETA           3950.0f
#define TEMP_25K           298.15f   // 25°C in Kelvin

float NTC10K_ADCToResistance(uint16_t adc)
{
    float adc_f;

    if (adc == 0U)
    {
        return 999999.0f; // open circuit or sensor error
    }

    if (adc >= 4095U)
    {
        return 0.0f;      // short to VDD or sensor error
    }

    adc_f = (float)adc;

    /*
        For:
        3.3V --- NTC --- ADC --- Rfixed --- GND

        Vadc = Vdd * Rfixed / (Rntc + Rfixed)

        So:
        Rntc = Rfixed * (4095 - ADC) / ADC
    */
    return (NTC_R_FIXED_OHM * (ADC_MAX_COUNTS - adc_f)) / adc_f;
}

float NTC10K_ADCToTemperatureC(uint16_t adc)
{
    float r_ntc;
    float temp_k;
    float temp_c;

    r_ntc = NTC10K_ADCToResistance(adc);

    if (r_ntc <= 0.0f)
    {
        return -100.0f;   // sensor short/error
    }

    if (r_ntc > 500000.0f)
    {
        return 200.0f;    // sensor open/error
    }

    /*
        Beta formula:
        1/T = 1/T0 + (1/B) * ln(R/R0)
    */
    temp_k = 1.0f / ((1.0f / TEMP_25K) + ((1.0f / NTC_BETA) * logf(r_ntc / NTC_R25_OHM)));
    temp_c = temp_k - 273.15f;

    return temp_c;
}

