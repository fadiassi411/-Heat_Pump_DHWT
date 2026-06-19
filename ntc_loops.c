#include <xc.h>
#include <stdint.h>
#include "ntc_10k.h"
#include "ntc_loops.h"

/*
    Additional NTC 10K loops on dsPIC33CK256MP505-E/PT, 48-pin package.

    Requested thermistor pins:
    SuCTS:    pin 6  = ANN2/RD13
    EvpTS:    pin 7  = AN12/RC0
    RHWTS:    pin 9  = ANA1/RA1
    TopDHWTS: pin 10 = AN9/RA2
    BotDHWTS: pin 11 = AN3/RA3
    OST:      pin 12 = AN4/RA4

    SuCTS is on ANN2, the negative input for ADC channel 2. AN2/RB7 is driven
    low as the positive reference for this differential conversion.
*/

#define NTC_LOOPS_TIMEOUT_COUNTS    100000UL
#define NTC_LOOPS_SUCTS_CHANNEL     2U
#define NTC_LOOPS_EVPTS_CHANNEL     12U
#define NTC_LOOPS_RHWTS_CHANNEL     1U
#define NTC_LOOPS_TOPDHWTS_CHANNEL  9U
#define NTC_LOOPS_BOTDHWTS_CHANNEL  3U
#define NTC_LOOPS_OST_CHANNEL       4U

volatile uint16_t g_sucts_adc_raw = 0;
volatile uint16_t g_evpts_adc_raw = 0;
volatile uint16_t g_rhwts_adc_raw = 0;
volatile uint16_t g_topdhwts_adc_raw = 0;
volatile uint16_t g_botdhwts_adc_raw = 0;
volatile uint16_t g_ost_adc_raw = 0;

volatile float g_sucts_resistance = 0.0f;
volatile float g_evpts_resistance = 0.0f;
volatile float g_rhwts_resistance = 0.0f;
volatile float g_topdhwts_resistance = 0.0f;
volatile float g_botdhwts_resistance = 0.0f;
volatile float g_ost_resistance = 0.0f;

volatile float g_sucts_temperature_c = 0.0f;
volatile float g_evpts_temperature_c = 0.0f;
volatile float g_rhwts_temperature_c = 0.0f;
volatile float g_topdhwts_temperature_c = 0.0f;
volatile float g_botdhwts_temperature_c = 0.0f;
volatile float g_ost_temperature_c = 0.0f;

volatile uint16_t g_ntc_loops_timeout = 0;
volatile uint16_t g_ntc_loops_state = 0;
volatile uint16_t g_adc_c1en = 0;
volatile uint16_t g_adc_c1rdy = 0;
volatile uint16_t g_adc_shren = 0;
volatile uint16_t g_adc_shrrdy = 0;
volatile uint16_t g_evpts_adc_read_counter = 0;
volatile uint16_t g_rhwts_adc_read_counter = 0;
volatile uint16_t g_topdhwts_adc_read_counter = 0;
volatile uint16_t g_botdhwts_adc_read_counter = 0;
volatile uint16_t g_ost_adc_read_counter = 0;

static uint16_t ADC_Read_Channel(uint16_t channel);
static uint16_t ADC_Read_ChannelOnce(uint16_t channel);
static uint16_t ADC_Read_SuCTS_ANN2Once(void);
static uint16_t ADC_SuCTSDifferentialToRaw(uint16_t adc_value);
static void NTC_UpdateReading(uint16_t adc_raw,
                              volatile float *resistance,
                              volatile float *temperature_c);

void NTCLoops_Init(void)
{
    g_ntc_loops_state = 1;

    /*
        Configure only the added NTC pins here. The existing AN0 setup in
        ADC_Init_AN0() is intentionally left in main.c unchanged.
    */
    TRISDbits.TRISD13 = 1;       /* SuCTS, pin 6, ANN2/RD13 */
    ANSELDbits.ANSELD13 = 1;

    LATBbits.LATB7 = 0;          /* AN2/RB7 = SuCTS differential + reference */
    TRISBbits.TRISB7 = 0;
    ANSELBbits.ANSELB7 = 1;

    TRISCbits.TRISC0 = 1;        /* EvpTS, pin 7, AN12/RC0 */
    ANSELCbits.ANSELC0 = 1;

    TRISAbits.TRISA1 = 1;        /* RHWTS, existing RA1 setup unchanged */
    ANSELAbits.ANSELA1 = 1;

    TRISAbits.TRISA2 = 1;        /* TopDHWTS, pin 10, AN9/RA2 */
    ANSELAbits.ANSELA2 = 1;

    TRISAbits.TRISA3 = 1;        /* BotDHWTS, pin 11, AN3/RA3 */
    ANSELAbits.ANSELA3 = 1;

    TRISAbits.TRISA4 = 1;        /* OST, pin 12, AN4/RA4 */
    ANSELAbits.ANSELA4 = 1;

    /*
        Add Core 1 and the shared core without changing the existing Core 0
        configuration. Call this before ADC_Init_AN0(), while ADC is off.
    */
    ADCON1Lbits.ADON = 0;

    ADCON1Hbits.SHRRES = 3;      /* Shared core 12-bit result */
    ADCON2Lbits.SHRADCS = 5;
    ADCON2Hbits.SHRSAMC = 20;

    ADCON3Hbits.C1EN = 1;
    ADCON3Hbits.SHREN = 1;

    /*
        Select ANA1 for Dedicated Core 1. C1CHS = 01 selects the alternate
        analog input for Core 1 on this package. In the XC16 header for this
        device, C1CHS is exposed through ADCON4Hbits.
    */
    ADCON4Hbits.C1CHS = 1;
    ADCON4Lbits.SAMC1EN = 1;

    ADCORE1Hbits.ADCS = 5;
    ADCORE1Hbits.RES = 3;
    ADCORE1Lbits.SAMC = 20;

    ADMOD0Lbits.SIGN1 = 0;
    ADMOD0Lbits.DIFF1 = 0;
    ADMOD0Lbits.SIGN2 = 1;
    ADMOD0Lbits.DIFF2 = 1;
    ADMOD0Lbits.SIGN3 = 0;
    ADMOD0Lbits.DIFF3 = 0;
    ADMOD0Lbits.SIGN4 = 0;
    ADMOD0Lbits.DIFF4 = 0;
    ADMOD0Hbits.SIGN9 = 0;
    ADMOD0Hbits.DIFF9 = 0;
    ADMOD0Hbits.SIGN12 = 0;
    ADMOD0Hbits.DIFF12 = 0;

    ADTRIG0Lbits.TRGSRC1 = 1;
    ADTRIG0Hbits.TRGSRC2 = 1;
    ADTRIG0Hbits.TRGSRC3 = 1;
    ADTRIG1Lbits.TRGSRC4 = 1;
    ADTRIG2Lbits.TRGSRC9 = 1;
    ADTRIG3Lbits.TRGSRC12 = 1;

    g_adc_c1en = ADCON3Hbits.C1EN;
    g_adc_c1rdy = ADCON5Lbits.C1RDY;
    g_adc_shren = ADCON3Hbits.SHREN;
    g_adc_shrrdy = ADCON5Lbits.SHRRDY;
    g_ntc_loops_state = 2;
}

void NTCLoops_Start(void)
{
    uint32_t timeout;

    g_ntc_loops_state = 5;

    ADCON5Lbits.C1PWR = 1;
    ADCON5Lbits.SHRPWR = 1;

    timeout = NTC_LOOPS_TIMEOUT_COUNTS;

    while (((ADCON5Lbits.C1RDY == 0U) || (ADCON5Lbits.SHRRDY == 0U)) &&
           (timeout > 0U))
    {
        timeout--;
    }

    g_adc_c1en = ADCON3Hbits.C1EN;
    g_adc_c1rdy = ADCON5Lbits.C1RDY;
    g_adc_shren = ADCON3Hbits.SHREN;
    g_adc_shrrdy = ADCON5Lbits.SHRRDY;

    if (timeout == 0U)
    {
        g_ntc_loops_timeout = 2;
        g_ntc_loops_state = 0xE102U;
    }
    else
    {
        g_ntc_loops_timeout = 0;
        g_ntc_loops_state = 6;
    }
}

void NTCLoops_Task(void)
{
    g_ntc_loops_state = 3;

    /*
        SuCTS is physically on ANN2/RD13. Read channel 2 in differential mode
        using grounded AN2/RB7 as the positive input, then use ADCBUF2.
        Do not read SuCTS as RA1/ANA1, AN1/RB2, or AN2 single-ended.
    */
    (void)ADC_Read_SuCTS_ANN2Once();
    g_sucts_adc_raw = ADC_Read_SuCTS_ANN2Once();
    NTC_UpdateReading(g_sucts_adc_raw, &g_sucts_resistance, &g_sucts_temperature_c);

    g_evpts_adc_raw = ADC_Read_Channel(NTC_LOOPS_EVPTS_CHANNEL);
    NTC_UpdateReading(g_evpts_adc_raw, &g_evpts_resistance, &g_evpts_temperature_c);

    g_rhwts_adc_raw = ADC_Read_Channel(NTC_LOOPS_RHWTS_CHANNEL);
    NTC_UpdateReading(g_rhwts_adc_raw, &g_rhwts_resistance, &g_rhwts_temperature_c);

    g_topdhwts_adc_raw = ADC_Read_Channel(NTC_LOOPS_TOPDHWTS_CHANNEL);
    NTC_UpdateReading(g_topdhwts_adc_raw, &g_topdhwts_resistance, &g_topdhwts_temperature_c);

    g_botdhwts_adc_raw = ADC_Read_Channel(NTC_LOOPS_BOTDHWTS_CHANNEL);
    NTC_UpdateReading(g_botdhwts_adc_raw, &g_botdhwts_resistance, &g_botdhwts_temperature_c);

    g_ost_adc_raw = ADC_Read_Channel(NTC_LOOPS_OST_CHANNEL);
    NTC_UpdateReading(g_ost_adc_raw, &g_ost_resistance, &g_ost_temperature_c);

    g_adc_c1en = ADCON3Hbits.C1EN;
    g_adc_c1rdy = ADCON5Lbits.C1RDY;
    g_adc_shren = ADCON3Hbits.SHREN;
    g_adc_shrrdy = ADCON5Lbits.SHRRDY;

    g_ntc_loops_state = 4;
}

static uint16_t ADC_Read_Channel(uint16_t channel)
{
    (void)ADC_Read_ChannelOnce(channel);

    return ADC_Read_ChannelOnce(channel);
}

static uint16_t ADC_Read_SuCTS_ANN2Once(void)
{
    uint32_t timeout = NTC_LOOPS_TIMEOUT_COUNTS;

    if (ADCON1Lbits.ADON == 0U)
    {
        g_ntc_loops_timeout = 1;
        g_ntc_loops_state = 0xE101U;
        return NTC_LOOPS_TIMEOUT_RESULT;
    }

    if (ADCON5Lbits.SHRRDY == 0U)
    {
        g_ntc_loops_timeout = 4;
        g_ntc_loops_state = 0xE104U;
        return NTC_LOOPS_TIMEOUT_RESULT;
    }

    ADCON3Lbits.CNVCHSEL = NTC_LOOPS_SUCTS_CHANNEL;
    (void)ADCBUF2;

    ADCON3Lbits.CNVRTCH = 1;

    while ((ADCON3Lbits.CNVRTCH != 0U) && (timeout > 0U))
    {
        timeout--;
    }

    if (timeout == 0U)
    {
        g_ntc_loops_timeout = 5;
        g_ntc_loops_state = 0xE105U;
        return NTC_LOOPS_TIMEOUT_RESULT;
    }

    while ((timeout > 0U) && (ADSTATLbits.AN2RDY == 0U))
    {
        timeout--;
    }

    if (timeout == 0U)
    {
        g_ntc_loops_timeout = 5;
        g_ntc_loops_state = 0xE105U;
        return NTC_LOOPS_TIMEOUT_RESULT;
    }

    g_ntc_loops_timeout = 0;

    return ADC_SuCTSDifferentialToRaw(ADCBUF2);
}

static uint16_t ADC_SuCTSDifferentialToRaw(uint16_t adc_value)
{
    int16_t signed_value;
    int32_t raw_value;

    if ((adc_value & 0x8000U) != 0U)
    {
        signed_value = (int16_t)adc_value;
    }
    else if ((adc_value & 0x0800U) != 0U)
    {
        signed_value = (int16_t)(adc_value | 0xF000U);
    }
    else
    {
        signed_value = (int16_t)(adc_value & 0x0FFFU);
    }

    if (signed_value < 0)
    {
        raw_value = -(int32_t)signed_value;
    }
    else
    {
        raw_value = (int32_t)signed_value;
    }

    /*
        Differential signed results use half the unsigned 12-bit span for one
        polarity. Scale the magnitude back to a normal 0..4095 NTC count.
    */
    raw_value *= 2L;

    if (raw_value > 4095L)
    {
        raw_value = 4095L;
    }

    return (uint16_t)raw_value;
}

static uint16_t ADC_Read_ChannelOnce(uint16_t channel)
{
    uint32_t timeout = NTC_LOOPS_TIMEOUT_COUNTS;

    if (ADCON1Lbits.ADON == 0U)
    {
        g_ntc_loops_timeout = 1;
        g_ntc_loops_state = 0xE101U;
        return NTC_LOOPS_TIMEOUT_RESULT;
    }

    if ((channel == NTC_LOOPS_RHWTS_CHANNEL) && (ADCON5Lbits.C1RDY == 0U))
    {
        g_ntc_loops_timeout = 3;
        g_ntc_loops_state = 0xE103U;
        return NTC_LOOPS_TIMEOUT_RESULT;
    }

    if ((channel != NTC_LOOPS_RHWTS_CHANNEL) && (ADCON5Lbits.SHRRDY == 0U))
    {
        g_ntc_loops_timeout = 4;
        g_ntc_loops_state = 0xE104U;
        return NTC_LOOPS_TIMEOUT_RESULT;
    }

    ADCON3Lbits.CNVCHSEL = channel;

    switch (channel)
    {
        case NTC_LOOPS_EVPTS_CHANNEL:
            (void)ADCBUF12;
            break;

        case NTC_LOOPS_RHWTS_CHANNEL:
            (void)ADCBUF1;
            break;

        case NTC_LOOPS_TOPDHWTS_CHANNEL:
            (void)ADCBUF9;
            break;

        case NTC_LOOPS_BOTDHWTS_CHANNEL:
            (void)ADCBUF3;
            break;

        case NTC_LOOPS_OST_CHANNEL:
            (void)ADCBUF4;
            break;

        default:
            return NTC_LOOPS_TIMEOUT_RESULT;
    }

    ADCON3Lbits.CNVRTCH = 1;

    while ((ADCON3Lbits.CNVRTCH != 0U) && (timeout > 0U))
    {
        timeout--;
    }

    if (timeout == 0U)
    {
        g_ntc_loops_timeout = 5;
        g_ntc_loops_state = 0xE105U;
        return NTC_LOOPS_TIMEOUT_RESULT;
    }

    while ((timeout > 0U) &&
           (((channel == NTC_LOOPS_EVPTS_CHANNEL) && (ADSTATLbits.AN12RDY == 0U)) ||
            ((channel == NTC_LOOPS_RHWTS_CHANNEL) && (ADSTATLbits.AN1RDY == 0U)) ||
            ((channel == NTC_LOOPS_TOPDHWTS_CHANNEL) && (ADSTATLbits.AN9RDY == 0U)) ||
            ((channel == NTC_LOOPS_BOTDHWTS_CHANNEL) && (ADSTATLbits.AN3RDY == 0U)) ||
            ((channel == NTC_LOOPS_OST_CHANNEL) && (ADSTATLbits.AN4RDY == 0U))))
    {
        timeout--;
    }

    if (timeout == 0U)
    {
        g_ntc_loops_timeout = 5;
        g_ntc_loops_state = 0xE105U;
        return NTC_LOOPS_TIMEOUT_RESULT;
    }

    g_ntc_loops_timeout = 0;

    switch (channel)
    {
        case NTC_LOOPS_EVPTS_CHANNEL:
            g_evpts_adc_read_counter++;
            return ADCBUF12;

        case NTC_LOOPS_RHWTS_CHANNEL:
            g_rhwts_adc_read_counter++;
            return ADCBUF1;

        case NTC_LOOPS_TOPDHWTS_CHANNEL:
            g_topdhwts_adc_read_counter++;
            return ADCBUF9;

        case NTC_LOOPS_BOTDHWTS_CHANNEL:
            g_botdhwts_adc_read_counter++;
            return ADCBUF3;

        case NTC_LOOPS_OST_CHANNEL:
            g_ost_adc_read_counter++;
            return ADCBUF4;

        default:
            return NTC_LOOPS_TIMEOUT_RESULT;
    }
}

static void NTC_UpdateReading(uint16_t adc_raw,
                              volatile float *resistance,
                              volatile float *temperature_c)
{
    if (adc_raw == NTC_LOOPS_TIMEOUT_RESULT)
    {
        *resistance = 0.0f;
        *temperature_c = 0.0f;
    }
    else
    {
        *resistance = NTC10K_ADCToResistance(adc_raw);
        *temperature_c = NTC10K_ADCToTemperatureC(adc_raw);
    }
}
