#include <xc.h>
#include <stdint.h>
#include <stdbool.h>
#include "faults/fault_inputs.h"
#include "ntc_10k.h"
#include "ntc_loops.h"
#include "LCD_I2C.h"
#include "eeprom_24fc04.h"
#include "eev.h"
#include "outputs.h"
#include "heatpump_control.h"

#pragma config FNOSC = FRC
#pragma config POSCMD = NONE
#pragma config OSCIOFNC = ON
#pragma config ALTI2C1 = OFF
#pragma config ICS = PGD2
#pragma config JTAGEN = OFF

#define FCY 4000000UL
#include <libpic30.h>

/*
    NTC 10K test on dsPIC33CK256MP505/MP508 family

    Hardware:
    3.3V ---- 10k fixed resistor ---- AN0 / RA0 ---- NTC 10k ---- GND

    Watch in MPLAB:
    g_ntc_adc_raw
    g_adcbuf0_direct
    g_adc_timeout
    g_adc_adon
    g_adc_c0en
    g_adc_c0rdy
    g_adc_pmd
    g_main_state
    g_loop_counter
    g_ntc_resistance
    g_ntc_temperature_c

    Expected at 25C with Vref = 3.3V:
    AN0 voltage ~1.65V
    ADC raw ~2048
    Resistance ~10000 ohm
    Temperature ~25C
*/

volatile uint16_t g_ntc_adc_raw = 0;
volatile uint16_t g_adcbuf0_direct = 0;
volatile uint16_t g_adc_timeout = 0;
volatile uint16_t g_adc_adon = 0;
volatile uint16_t g_adc_c0en = 0;
volatile uint16_t g_adc_c0rdy = 0;
volatile uint16_t g_adc_pmd = 0;
volatile uint16_t g_main_state = 0;
volatile uint16_t g_loop_counter = 0;
volatile uint32_t g_system_ms = 0UL;
volatile uint16_t g_shwts_adc_read_counter = 0;

volatile float g_ntc_resistance = 0.0f;
volatile float g_ntc_temperature_c = 0.0f;
volatile float g_shwts_temperature_c = 0.0f;

#define ADC_AN0_CHANNEL        0U
#define ADC_TIMEOUT_COUNTS     100000UL
#define ADC_TIMEOUT_RESULT     0xFFFFU
#define SENSOR_FAULT_SHWTS_MASK     0x0001U
#define SENSOR_FAULT_EVPTS_MASK     0x0002U
#define SENSOR_FAULT_RHWTS_MASK     0x0004U
#define SENSOR_FAULT_TOPDHWTS_MASK  0x0008U
#define SENSOR_FAULT_BOTDHWTS_MASK  0x0010U
#define SENSOR_FAULT_OST_MASK       0x0020U
#define MAIN_TASK_PERIOD_MS         500UL
#define FAULT_LED_BLINK_MS          500UL
#define TIMER1_PRESCALE_VALUE       8UL
#define TIMER1_1MS_PERIOD_COUNTS    ((FCY / TIMER1_PRESCALE_VALUE / 1000UL) - 1UL)


static void Timer1_Init_1ms(void)
{
    T1CONbits.TON = 0;
    T1CONbits.TCS = 0;
    T1CONbits.TGATE = 0;
    T1CONbits.TCKPS = 1;   /* 1:8 prescale, FCY = 4 MHz gives PR1 = 499 for 1 ms. */
    TMR1 = 0;
    PR1 = (uint16_t)TIMER1_1MS_PERIOD_COUNTS;
    IFS0bits.T1IF = 0;
    IEC0bits.T1IE = 1;
    T1CONbits.TON = 1;
}

void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void)
{
    IFS0bits.T1IF = 0;
    g_system_ms++;
}

static bool MainElapsedMs(uint32_t now_ms, uint32_t previous_ms, uint32_t period_ms)
{
    return ((uint32_t)(now_ms - previous_ms) >= period_ms);
}

static void MainImmediateCriticalFaultShutdown(void)
{
    /*
        HP/LP compressor protection must also be backed by a hardwired safety
        chain outside software. This software path only removes MCU commands
        as soon as the raw HP/LP inputs are seen.
    */
    if ((g_fault_inputs & (FAULT_INPUT_HP_MASK | FAULT_INPUT_LP_MASK)) != 0U)
    {
        Outputs_Write(0U);
        EEV_AllOutputsOff();
    }
}

static void FaultLed_Task(void)
{
    static uint32_t last_toggle_ms = 0UL;
    uint32_t now_ms = g_system_ms;
    uint16_t led_faults;

    led_faults = (uint16_t)(g_hp_active_fault | g_hp_latched_fault |
                            (g_fault_inputs & (FAULT_INPUT_HP_MASK | FAULT_INPUT_LP_MASK)));

    if (led_faults != 0U)
    {
        if (MainElapsedMs(now_ms, last_toggle_ms, FAULT_LED_BLINK_MS) != false)
        {
            last_toggle_ms = now_ms;
            LATDbits.LATD8 ^= 1U;
        }
    }
    else
    {
        LATDbits.LATD8 = 0;
        last_toggle_ms = now_ms;
    }
}
static void ADC_Init_AN0(void)
{
    g_main_state = 2;

    /* Make sure the ADC peripheral is not disabled by PMD. */
    PMD1bits.ADC1MD = 0;
    g_adc_pmd = PMD1bits.ADC1MD;

    /*
        RA0 = AN0 input.
        Pin 8 on 48-pin dsPIC33CK256MP505 package.
    */
    TRISAbits.TRISA0 = 1;
    ANSELAbits.ANSELA0 = 1;

    /*
        Disable ADC before configuration.
        The datasheet warns not to change ADC configuration while ADON = 1.
    */
    ADCON1Lbits.ADON = 0;
    g_adc_adon = ADCON1Lbits.ADON;

    /*
        Output format:
        FORM = 0 means integer result.
    */
    ADCON1Hbits.FORM = 0;

    /*
        Reference:
        REFSEL = 000 means AVDD / AVSS.
    */
    ADCON3Lbits.REFSEL = 0;

    /*
        Make sure triggers are not suspended.
    */
    ADCON3Lbits.SUSPEND = 0;

    /*
        ADC module clock.
        CLKSEL = 0: FP / peripheral clock.
        CLKDIV = 3: conservative divider for test.
    */
    ADCON3Hbits.CLKSEL = 0;
    ADCON3Hbits.CLKDIV = 3;

    /*
        Enable Dedicated ADC Core 0.
        AN0 belongs to Dedicated ADC Core 0.
    */
    ADCON3Hbits.C0EN = 1;

    /*
        Select positive input for Core 0:
        C0CHS = 00 means AN0.
    */
    ADCON4Hbits.C0CHS = ADC_AN0_CHANNEL;

    /*
        Enable extra sample time for Dedicated Core 0.
        This is useful for an NTC voltage divider.
    */
    ADCON4Lbits.SAMC0EN = 1;

    /*
        Dedicated Core 0 setup.
        RES = 3 means 12-bit.
    */
    ADCORE0Hbits.ADCS = 5;
    ADCORE0Hbits.RES = 3;
    ADCORE0Lbits.SAMC = 20;

    /*
        AN0 channel format and trigger:
        unsigned, single-ended, triggered by software.
    */
    ADMOD0Lbits.SIGN0 = 0;
    ADMOD0Lbits.DIFF0 = 0;
    ADTRIG0Lbits.TRGSRC0 = 1;

    /*
        Warm-up time.
    */
    ADCON5Hbits.WARMTIME = 15;

    /*
        Enable ADC module after configuration.
    */
    ADCON1Lbits.ADON = 1;
    g_adc_adon = ADCON1Lbits.ADON;
    g_main_state = 3;

    /*
        Power Dedicated Core 0 and wait until ready.
    */
    ADCON5Lbits.C0PWR = 1;
    g_main_state = 4;

    uint32_t core_timeout = ADC_TIMEOUT_COUNTS;

    while ((ADCON5Lbits.C0RDY == 0U) && (core_timeout > 0U))
    {
        core_timeout--;
    }

    /* Debug values */
    g_adc_adon = ADCON1Lbits.ADON;
    g_adc_c0en = ADCON3Hbits.C0EN;
    g_adc_c0rdy = ADCON5Lbits.C0RDY;
    g_adc_pmd = PMD1bits.ADC1MD;

    if (core_timeout == 0U)
    {
        g_adc_timeout = 2;   // Core 0 did not become ready
        g_main_state = 0xE002U;
    }
}

static uint16_t ADC_Read_AN0(void)
{
    uint32_t timeout = ADC_TIMEOUT_COUNTS;

    g_main_state = 5;

    if ((ADCON1Lbits.ADON == 0U) || (ADCON5Lbits.C0RDY == 0U))
    {
        g_main_state = 0xE001U;
        return ADC_TIMEOUT_RESULT;
    }

    /*
        Select AN0 for individual software conversion.
    */
    ADCON3Lbits.CNVCHSEL = ADC_AN0_CHANNEL;

    /*
        Drain any previous AN0 result so the ready flag cannot make this
        call return an old conversion.
    */
    (void)ADCBUF0;

    /*
        Start one conversion for channel selected by CNVCHSEL. Wait for the
        software trigger to complete first so an old ready flag cannot return
        the previous ADCBUF value as a fresh reading.
    */
    ADCON3Lbits.CNVRTCH = 1;

    while ((ADCON3Lbits.CNVRTCH != 0U) && (timeout > 0U))
    {
        timeout--;
    }

    if (timeout == 0U)
    {
        g_main_state = 0xE003U;
        return ADC_TIMEOUT_RESULT;
    }

    while ((ADSTATLbits.AN0RDY == 0U) && (timeout > 0U))
    {
        timeout--;
    }

    if (timeout == 0U)
    {
        g_main_state = 0xE003U;
        return ADC_TIMEOUT_RESULT;
    }

    g_shwts_adc_read_counter++;
    g_main_state = 6;
    return ADCBUF0;
}

static void UpdateSensorFaults(void)
{
    uint16_t sensor_faults = 0;

    if (FaultInputs_IsSensorOpenOrShort(g_ntc_adc_raw))
    {
        sensor_faults |= SENSOR_FAULT_SHWTS_MASK;
    }

    if (FaultInputs_IsSensorOpenOrShort(g_evpts_adc_raw))
    {
        sensor_faults |= SENSOR_FAULT_EVPTS_MASK;
    }

    if (FaultInputs_IsSensorOpenOrShort(g_rhwts_adc_raw))
    {
        sensor_faults |= SENSOR_FAULT_RHWTS_MASK;
    }

    if (FaultInputs_IsSensorOpenOrShort(g_topdhwts_adc_raw))
    {
        sensor_faults |= SENSOR_FAULT_TOPDHWTS_MASK;
    }

    if (FaultInputs_IsSensorOpenOrShort(g_botdhwts_adc_raw))
    {
        sensor_faults |= SENSOR_FAULT_BOTDHWTS_MASK;
    }

    if (FaultInputs_IsSensorOpenOrShort(g_ost_adc_raw))
    {
        sensor_faults |= SENSOR_FAULT_OST_MASK;
    }

    FaultInputs_SetSensorFaults(sensor_faults);
}


int main(void)
{
    uint32_t last_task_ms = 0UL;
    uint32_t now_ms;

    /*
        Disable watchdog for this ADC debug test.
    */
    WDTCONLbits.ON = 0;
    g_main_state = 1;

    FaultInputs_Init();
    NTCLoops_Init();
    ADC_Init_AN0();
    NTCLoops_Start();
    LCD_I2C_Init(0U);

    {
        EEPROM24FC04_Settings settings;

        /*
            I2C1 is already configured by LCD_I2C_Init(). The EEPROM shares the
            same bus and does not change the working RB9/RB8 pin setup.
        */
        EEPROM24FC04_Init();
        (void)EEPROM24FC04_LoadSettings(&settings);

        /*
            Future button/menu code should call EEPROM24FC04_SaveSettings() only
            after the user changes a setpoint. Do not write EEPROM every loop.

            Initial EEPROM hardware test in MPLAB Watch:
            1. Check g_eeprom_present after startup.
            2. Use EEPROM24FC04_WriteData(EEPROM24FC04_ADDR_TEST, data, len) with
               len <= EEPROM24FC04_TEST_LENGTH.
            3. Read it back with EEPROM24FC04_ReadData() and watch
               g_eeprom_error, g_eeprom_write_count, and g_eeprom_read_count.
        */
    }

    HeatPumpControl_Init();
    EEV_Init();
    Timer1_Init_1ms();

    g_adc_adon = ADCON1Lbits.ADON;
    g_adc_c0en = ADCON3Hbits.C0EN;
    g_adc_c0rdy = ADCON5Lbits.C0RDY;
    g_adc_pmd = PMD1bits.ADC1MD;

    while (1)
    {
        (void)FaultInputs_IsActive();
        MainImmediateCriticalFaultShutdown();
        FaultLed_Task();

        now_ms = g_system_ms;
        if (MainElapsedMs(now_ms, last_task_ms, MAIN_TASK_PERIOD_MS) == false)
        {
            continue;
        }
        last_task_ms = now_ms;

        g_loop_counter++;
        g_ntc_adc_raw = ADC_Read_AN0();

        g_adcbuf0_direct = ADCBUF0;
        g_adc_adon = ADCON1Lbits.ADON;
        g_adc_c0en = ADCON3Hbits.C0EN;
        g_adc_c0rdy = ADCON5Lbits.C0RDY;
        g_adc_pmd = PMD1bits.ADC1MD;

        if (g_ntc_adc_raw == ADC_TIMEOUT_RESULT)
        {
            g_adc_timeout = 1;   // Conversion timeout
            g_ntc_resistance = 0.0f;
            g_ntc_temperature_c = 0.0f;
            g_shwts_temperature_c = 0.0f;
        }
        else
        {
            g_adc_timeout = 0;
            g_ntc_resistance = NTC10K_ADCToResistance(g_ntc_adc_raw);
            g_ntc_temperature_c = NTC10K_ADCToTemperatureC(g_ntc_adc_raw);
            g_shwts_temperature_c = g_ntc_temperature_c;
        }

        NTCLoops_Task();
        LCD_I2C_ShowNTCTemperatures();
        UpdateSensorFaults();
        HeatPumpControl_Task();
    }

    return 0;
}
