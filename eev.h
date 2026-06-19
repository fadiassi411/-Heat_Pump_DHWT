#ifndef EEV_H
#define EEV_H

#include <stdint.h>
#include "app_config.h"

extern volatile uint16_t g_eev_position_steps;
extern volatile uint8_t g_eev_position_percent;
extern volatile uint16_t g_eev_startup_close_done;
extern volatile uint16_t g_eev_init_done;

void EEV_Init(void);
void EEV_AllOutputsOff(void);
void EEV_StepOpen(void);
void EEV_StepClose(void);
void EEV_MoveStepsOpen(uint16_t steps);
void EEV_MoveStepsClose(uint16_t steps);
void EEV_MoveToPercent(uint8_t percent);
uint16_t EEV_GetPositionSteps(void);
uint8_t EEV_GetPositionPercent(void);

#endif
