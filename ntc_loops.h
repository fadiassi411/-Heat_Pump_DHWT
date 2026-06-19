#ifndef NTC_LOOPS_H
#define NTC_LOOPS_H

#include <stdint.h>

#define NTC_LOOPS_TIMEOUT_RESULT    0xFFFFU

extern volatile uint16_t g_sucts_adc_raw;
extern volatile uint16_t g_evpts_adc_raw;
extern volatile uint16_t g_rhwts_adc_raw;
extern volatile uint16_t g_topdhwts_adc_raw;
extern volatile uint16_t g_botdhwts_adc_raw;
extern volatile uint16_t g_ost_adc_raw;

extern volatile float g_sucts_resistance;
extern volatile float g_evpts_resistance;
extern volatile float g_rhwts_resistance;
extern volatile float g_topdhwts_resistance;
extern volatile float g_botdhwts_resistance;
extern volatile float g_ost_resistance;

extern volatile float g_sucts_temperature_c;
extern volatile float g_evpts_temperature_c;
extern volatile float g_rhwts_temperature_c;
extern volatile float g_topdhwts_temperature_c;
extern volatile float g_botdhwts_temperature_c;
extern volatile float g_ost_temperature_c;

extern volatile uint16_t g_ntc_loops_timeout;
extern volatile uint16_t g_ntc_loops_state;
extern volatile uint16_t g_adc_c1en;
extern volatile uint16_t g_adc_c1rdy;
extern volatile uint16_t g_adc_shren;
extern volatile uint16_t g_adc_shrrdy;
extern volatile uint16_t g_evpts_adc_read_counter;
extern volatile uint16_t g_rhwts_adc_read_counter;
extern volatile uint16_t g_topdhwts_adc_read_counter;
extern volatile uint16_t g_botdhwts_adc_read_counter;
extern volatile uint16_t g_ost_adc_read_counter;

void NTCLoops_Init(void);
void NTCLoops_Start(void);
void NTCLoops_Task(void);

#endif
