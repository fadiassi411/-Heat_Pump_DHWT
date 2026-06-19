#ifndef FAULT_INPUTS_H
#define FAULT_INPUTS_H

#include <stdbool.h>
#include <stdint.h>

#define FAULT_INPUT_HP_MASK      0x0001U
#define FAULT_INPUT_LP_MASK      0x0002U
#define FAULT_INPUT_FLOW_MASK    0x0004U
#define FAULT_INPUT_SENSOR_MASK  0x0008U

extern volatile uint16_t g_fault_active;
extern volatile uint16_t g_fault_inputs;
extern volatile uint16_t g_sensor_fault_inputs;

void FaultInputs_Init(void);
bool FaultInputs_IsActive(void);
bool FaultInputs_IsSensorOpenOrShort(uint16_t adc_raw);
void FaultInputs_SetSensorFaults(uint16_t sensor_faults);
void FaultInputs_BlinkUntilClear(void);
void FaultInputs_DelayOrHandleFault(void);

#endif
