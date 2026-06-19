#ifndef OUTPUTS_H
#define OUTPUTS_H

#include <stdbool.h>
#include <stdint.h>

#define OUTPUT_COMPRESSOR_MASK       0x0001U
#define OUTPUT_HEATER_MASK           0x0002U
#define OUTPUT_FAN_MASK              0x0004U
#define OUTPUT_PUMP_MASK             0x0008U
#define OUTPUT_FOUR_WAY_VALVE_MASK   0x0010U

extern volatile uint16_t g_output_states;
extern volatile uint16_t g_output_init_done;

void Outputs_Init(void);
void Outputs_Write(uint16_t output_mask);
void Outputs_Set(uint16_t output_mask);
void Outputs_Clear(uint16_t output_mask);
uint16_t Outputs_Get(void);

void Outputs_SetCompressor(bool enabled);
void Outputs_SetHeater(bool enabled);
void Outputs_SetFan(bool enabled);
void Outputs_SetPump(bool enabled);
void Outputs_SetFourWayValve(bool enabled);

#endif
