#include <xc.h>
#include <stdint.h>
#include "app_config.h"
#include "eev.h"

#define FCY APP_FCY_HZ
#include <libpic30.h>

#define EEV_SEQUENCE_LENGTH        8U

/*
    Debug protection:
    This project currently uses #pragma config ICS = PGD2. On this hardware,
    RB0/RB1 are EEV coil pins and may share the active debug PGC/PGD pair.
    Normal programming can still work, but MPLAB debug can fail if firmware
    drives those pins.

    Set to 1 only if MPLAB debug fails because external EEV hardware loads
    RB0/RB1 on the PGD2/PGC2 pair. When set to 1, debug builds skip real EEV
    pin driving and startup close movement.

    Keep this enabled while debugging on PGD2. Production programming builds
    do not define __DEBUG, so real EEV startup close and movement still run
    when you load/program without starting a debug session.
*/
#define EEV_DISABLE_PGD2_PINS_DURING_DEBUG  1

#if defined(__DEBUG) && (EEV_DISABLE_PGD2_PINS_DURING_DEBUG != 0)
#define EEV_DEBUG_PINS_DISABLED             1
#else
#define EEV_DEBUG_PINS_DISABLED             0
#endif

volatile uint16_t g_eev_position_steps = 0;
volatile uint8_t g_eev_position_percent = 0;
volatile uint16_t g_eev_startup_close_done = 0;
volatile uint16_t g_eev_init_done = 0;

static uint8_t eev_sequence_index = 0;

static void EEV_UpdatePercent(void)
{
    g_eev_position_percent =
        (uint8_t)(((uint32_t)g_eev_position_steps * 100UL +
                   (EEV_MAX_POSITION_STEPS / 2U)) /
                  EEV_MAX_POSITION_STEPS);
}

#if (EEV_DEBUG_PINS_DISABLED == 0)
static void EEV_ApplySequence(uint8_t sequence_step)
{
    /*
        8-step half-step coil sequence.

        Step  A1/RB0  B1/RB1  A2/RD10  B2/RC7
        0       1       0       0        0
        1       1       1       0        0
        2       0       1       0        0
        3       0       1       1        0
        4       0       0       1        0
        5       0       0       1        1
        6       0       0       0        1
        7       1       0       0        1

        Opening advances through the table. Closing reverses through it.
    */
    switch (sequence_step & 0x07U)
    {
        case 0U:
            LATBbits.LATB0 = 1U;
            LATBbits.LATB1 = 0U;
            LATDbits.LATD10 = 0U;
            LATCbits.LATC7 = 0U;
            break;

        case 1U:
            LATBbits.LATB0 = 1U;
            LATBbits.LATB1 = 1U;
            LATDbits.LATD10 = 0U;
            LATCbits.LATC7 = 0U;
            break;

        case 2U:
            LATBbits.LATB0 = 0U;
            LATBbits.LATB1 = 1U;
            LATDbits.LATD10 = 0U;
            LATCbits.LATC7 = 0U;
            break;

        case 3U:
            LATBbits.LATB0 = 0U;
            LATBbits.LATB1 = 1U;
            LATDbits.LATD10 = 1U;
            LATCbits.LATC7 = 0U;
            break;

        case 4U:
            LATBbits.LATB0 = 0U;
            LATBbits.LATB1 = 0U;
            LATDbits.LATD10 = 1U;
            LATCbits.LATC7 = 0U;
            break;

        case 5U:
            LATBbits.LATB0 = 0U;
            LATBbits.LATB1 = 0U;
            LATDbits.LATD10 = 1U;
            LATCbits.LATC7 = 1U;
            break;

        case 6U:
            LATBbits.LATB0 = 0U;
            LATBbits.LATB1 = 0U;
            LATDbits.LATD10 = 0U;
            LATCbits.LATC7 = 1U;
            break;

        default:
            LATBbits.LATB0 = 1U;
            LATBbits.LATB1 = 0U;
            LATDbits.LATD10 = 0U;
            LATCbits.LATC7 = 1U;
            break;
    }
}

static void EEV_PulseSequenceStep(uint8_t sequence_step)
{
    EEV_ApplySequence(sequence_step);
    __delay_ms(EEV_PULSE_ON_MS);
    EEV_AllOutputsOff();
    __delay_ms(EEV_PULSE_OFF_MS);
}
#endif

void EEV_AllOutputsOff(void)
{
#if (EEV_DEBUG_PINS_DISABLED != 0)
    return;
#else
    LATBbits.LATB0 = 0U;
    LATBbits.LATB1 = 0U;
    LATDbits.LATD10 = 0U;
    LATCbits.LATC7 = 0U;
#endif
}

#if (EEV_DEBUG_PINS_DISABLED == 0)
static void EEV_SendStartupClosePulses(void)
{
    uint16_t step_count;

    for (step_count = 0U; step_count < EEV_STARTUP_CLOSE_STEPS; step_count++)
    {
        if (eev_sequence_index == 0U)
        {
            eev_sequence_index = EEV_SEQUENCE_LENGTH - 1U;
        }
        else
        {
            eev_sequence_index--;
        }

        EEV_PulseSequenceStep(eev_sequence_index);
    }
}
#endif

void EEV_Init(void)
{
    g_eev_position_steps = 0U;
    g_eev_position_percent = 0U;
    g_eev_startup_close_done = 0U;
    g_eev_init_done = 0U;
    eev_sequence_index = 0U;

#if (EEV_DEBUG_PINS_DISABLED != 0)
    return;
#else
    /*
        EEV coil outputs:
        A1 = RB0
        B1 = RB1
        A2 = RD10
        B2 = RC7
    */
    ANSELBbits.ANSELB0 = 0U;
    ANSELBbits.ANSELB1 = 0U;
    ANSELDbits.ANSELD10 = 0U;
    ANSELCbits.ANSELC7 = 0U;

    EEV_AllOutputsOff();

    TRISBbits.TRISB0 = 0U;
    TRISBbits.TRISB1 = 0U;
    TRISDbits.TRISD10 = 0U;
    TRISCbits.TRISC7 = 0U;

    EEV_SendStartupClosePulses();

    g_eev_position_steps = 0U;
    g_eev_position_percent = 0U;
    g_eev_startup_close_done = 1U;
    g_eev_init_done = 1U;
    EEV_AllOutputsOff();
#endif
}

void EEV_StepOpen(void)
{
#if (EEV_DEBUG_PINS_DISABLED != 0)
    return;
#else
    if (g_eev_position_steps >= EEV_MAX_POSITION_STEPS)
    {
        return;
    }

    EEV_PulseSequenceStep(eev_sequence_index);
    eev_sequence_index++;

    if (eev_sequence_index >= EEV_SEQUENCE_LENGTH)
    {
        eev_sequence_index = 0U;
    }

    g_eev_position_steps++;
    EEV_UpdatePercent();
#endif
}

void EEV_StepClose(void)
{
#if (EEV_DEBUG_PINS_DISABLED != 0)
    return;
#else
    if (g_eev_position_steps == 0U)
    {
        return;
    }

    if (eev_sequence_index == 0U)
    {
        eev_sequence_index = EEV_SEQUENCE_LENGTH - 1U;
    }
    else
    {
        eev_sequence_index--;
    }

    EEV_PulseSequenceStep(eev_sequence_index);

    g_eev_position_steps--;
    EEV_UpdatePercent();
#endif
}

void EEV_MoveStepsOpen(uint16_t steps)
{
    uint16_t move_steps;
    uint16_t step_count;

    if (g_eev_position_steps >= EEV_MAX_POSITION_STEPS)
    {
        return;
    }

    move_steps = EEV_MAX_POSITION_STEPS - g_eev_position_steps;

    if (steps < move_steps)
    {
        move_steps = steps;
    }

    for (step_count = 0U; step_count < move_steps; step_count++)
    {
        EEV_StepOpen();
    }
}

void EEV_MoveStepsClose(uint16_t steps)
{
    uint16_t move_steps;
    uint16_t step_count;

    move_steps = g_eev_position_steps;

    if (steps < move_steps)
    {
        move_steps = steps;
    }

    for (step_count = 0U; step_count < move_steps; step_count++)
    {
        EEV_StepClose();
    }
}

void EEV_MoveToPercent(uint8_t percent)
{
    uint16_t target_steps;

    if (percent > 100U)
    {
        percent = 100U;
    }

    target_steps = (uint16_t)(((uint32_t)percent * EEV_MAX_POSITION_STEPS +
                               50UL) /
                              100UL);

    if (target_steps > g_eev_position_steps)
    {
        EEV_MoveStepsOpen(target_steps - g_eev_position_steps);
    }
    else if (target_steps < g_eev_position_steps)
    {
        EEV_MoveStepsClose(g_eev_position_steps - target_steps);
    }
    else
    {
        EEV_UpdatePercent();
    }
}

uint16_t EEV_GetPositionSteps(void)
{
    return g_eev_position_steps;
}

uint8_t EEV_GetPositionPercent(void)
{
    return g_eev_position_percent;
}
