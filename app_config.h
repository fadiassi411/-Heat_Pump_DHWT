#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#define APP_FCY_HZ                 4000000UL
#define APP_TASK_PERIOD_MS         100U

#define NTC_SENSOR_COUNT           7U
#define ADC_MAX_COUNTS             4095.0f
#define ADC_VREF_VOLTS             3.3f

// NTC 10K B3950 typical values
#define NTC_R_FIXED_OHMS           10000.0f
#define NTC_R25_OHMS               10000.0f
#define NTC_BETA_K                 3950.0f
#define NTC_T25_K                  298.15f

/*
 * Pressure sensor configuration
 * Sensor: 0 to 60 bar, 0 to 5 V output.
 * Hardware: voltage divider scales sensor 0 to 5 V into ADC 0 to 3.3 V.
 * Therefore:
 *   ADC input 0.0 V = 0 bar
 *   ADC input 3.3 V = 60 bar
 */
#define PRESSURE_SENSOR_MIN_BAR            0.0f
#define PRESSURE_SENSOR_MAX_BAR            60.0f
#define PRESSURE_SENSOR_MIN_VOLTS          0.0f
#define PRESSURE_SENSOR_MAX_VOLTS          5.0f
#define PRESSURE_ADC_MIN_VOLTS             0.0f
#define PRESSURE_ADC_MAX_VOLTS             ADC_VREF_VOLTS
#define PRESSURE_DIVIDER_RATIO             (PRESSURE_ADC_MAX_VOLTS / PRESSURE_SENSOR_MAX_VOLTS)

/*
 * Electronic Expansion Valve stepper configuration.
 * 3 ms on + 3 ms off gives about 166.7 Hz, within the 100 to 300 Hz spec.
 */
#define EEV_MAX_POSITION_STEPS             560U
#define EEV_STARTUP_CLOSE_STEPS            620U
#define EEV_PULSE_ON_MS                    3U
#define EEV_PULSE_OFF_MS                   3U

/*
 * Control timing defaults stored in EEPROM settings.
 */
#define TANK_HYSTERESIS_DEFAULT_C          3.0f
#define COMPRESSOR_ANTI_SHORT_CYCLE_SEC    180U
#define LP_STARTUP_BYPASS_SEC              60U
#define PUMP_PRE_RUN_SEC                   5U
#define PUMP_POST_RUN_SEC                  60U
#define HEATER_ENABLED_DEFAULT             false

#endif // APP_CONFIG_H
