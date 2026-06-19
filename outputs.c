#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include "outputs.h"

volatile uint16_t g_output_states = 0;
volatile uint16_t g_output_init_done = 0;

static void Outputs_Apply(void)
{
    LATBbits.LATB13 = ((g_output_states & OUTPUT_COMPRESSOR_MASK) != 0U) ? 1U : 0U;
    LATCbits.LATC1 = ((g_output_states & OUTPUT_HEATER_MASK) != 0U) ? 1U : 0U;
    LATCbits.LATC2 = ((g_output_states & OUTPUT_FAN_MASK) != 0U) ? 1U : 0U;
    LATCbits.LATC6 = ((g_output_states & OUTPUT_PUMP_MASK) != 0U) ? 1U : 0U;
    LATCbits.LATC3 = ((g_output_states & OUTPUT_FOUR_WAY_VALVE_MASK) != 0U) ? 1U : 0U;
}

void Outputs_Init(void)
{
    /*
        Application outputs:
        Compressor    = RB13, active-high
        Heater        = RC1, active-high
        FAN           = RC2, active-high
        Pump          = RC6, active-high
        4 Way Valve   = RC3, active-high
    */
    /* RB13 has no ANSELB13 bit in the dsPIC33CK256MP505 device header. */
    ANSELCbits.ANSELC1 = 0;
    ANSELCbits.ANSELC2 = 0;
    ANSELCbits.ANSELC6 = 0;
    ANSELCbits.ANSELC3 = 0;

    g_output_states = 0;
    Outputs_Apply();

    TRISBbits.TRISB13 = 0;
    TRISCbits.TRISC1 = 0;
    TRISCbits.TRISC2 = 0;
    TRISCbits.TRISC6 = 0;
    TRISCbits.TRISC3 = 0;

    g_output_init_done = 1;
}

void Outputs_Write(uint16_t output_mask)
{
    g_output_states = output_mask &
                      (OUTPUT_COMPRESSOR_MASK |
                       OUTPUT_HEATER_MASK |
                       OUTPUT_FAN_MASK |
                       OUTPUT_PUMP_MASK |
                       OUTPUT_FOUR_WAY_VALVE_MASK);
    Outputs_Apply();
}

void Outputs_Set(uint16_t output_mask)
{
    Outputs_Write(g_output_states | output_mask);
}

void Outputs_Clear(uint16_t output_mask)
{
    Outputs_Write(g_output_states & (uint16_t)(~output_mask));
}

uint16_t Outputs_Get(void)
{
    return g_output_states;
}

void Outputs_SetCompressor(bool enabled)
{
    if (enabled)
    {
        Outputs_Set(OUTPUT_COMPRESSOR_MASK);
    }
    else
    {
        Outputs_Clear(OUTPUT_COMPRESSOR_MASK);
    }
}

void Outputs_SetHeater(bool enabled)
{
    if (enabled)
    {
        Outputs_Set(OUTPUT_HEATER_MASK);
    }
    else
    {
        Outputs_Clear(OUTPUT_HEATER_MASK);
    }
}

void Outputs_SetFan(bool enabled)
{
    if (enabled)
    {
        Outputs_Set(OUTPUT_FAN_MASK);
    }
    else
    {
        Outputs_Clear(OUTPUT_FAN_MASK);
    }
}

void Outputs_SetPump(bool enabled)
{
    if (enabled)
    {
        Outputs_Set(OUTPUT_PUMP_MASK);
    }
    else
    {
        Outputs_Clear(OUTPUT_PUMP_MASK);
    }
}

void Outputs_SetFourWayValve(bool enabled)
{
    if (enabled)
    {
        Outputs_Set(OUTPUT_FOUR_WAY_VALVE_MASK);
    }
    else
    {
        Outputs_Clear(OUTPUT_FOUR_WAY_VALVE_MASK);
    }
}
