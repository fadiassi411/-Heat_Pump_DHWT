#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "fault_inputs.h"

#define SENSOR_SHORT_ADC_MAX      5U
#define SENSOR_OPEN_ADC_MIN       4090U
#define SENSOR_ADC_TIMEOUT        0xFFFFU

volatile uint16_t g_fault_active = 0;
volatile uint16_t g_fault_inputs = 0;
volatile uint16_t g_sensor_fault_inputs = 0;

void FaultInputs_Init(void)
{
    /*
        Fault inputs:
        HP          = RC5, active-low
        LP          = RC4, active-low
        Flow Switch = RC10, active-high

        Fault LED:
        RD8 toggles every 500 ms while an HP, LP, or Flow fault is active.
        NTC sensor open/short status is tracked separately for now and does
        not make g_fault_active true.
    */
    TRISCbits.TRISC5 = 1;
    TRISCbits.TRISC4 = 1;
    TRISCbits.TRISC10 = 1;

    TRISDbits.TRISD8 = 0;
    LATDbits.LATD8 = 0;

    g_fault_active = 0;
    g_fault_inputs = 0;
    g_sensor_fault_inputs = 0;
}

bool FaultInputs_IsActive(void)
{
    uint16_t inputs = 0;

    if (PORTCbits.RC5 == 0U)
    {
        inputs |= FAULT_INPUT_HP_MASK;
    }

    if (PORTCbits.RC4 == 0U)
    {
        inputs |= FAULT_INPUT_LP_MASK;
    }

    if (PORTCbits.RC10 != 0U)
    {
        inputs |= FAULT_INPUT_FLOW_MASK;
    }

    g_fault_inputs = inputs;
    g_fault_active = (inputs != 0U) ? 1U : 0U;

    return (inputs != 0U);
}

bool FaultInputs_IsSensorOpenOrShort(uint16_t adc_raw)
{
    if (adc_raw == SENSOR_ADC_TIMEOUT)
    {
        return true;
    }

    return ((adc_raw <= SENSOR_SHORT_ADC_MAX) ||
            (adc_raw >= SENSOR_OPEN_ADC_MIN));
}

void FaultInputs_SetSensorFaults(uint16_t sensor_faults)
{
    g_sensor_fault_inputs = sensor_faults;
}
