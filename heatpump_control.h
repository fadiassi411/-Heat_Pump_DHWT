#ifndef HEATPUMP_CONTROL_H
#define HEATPUMP_CONTROL_H

#include <stdint.h>

typedef enum
{
    HP_STATE_IDLE = 0U,
    HP_STATE_PRECHECK = 1U,
    HP_STATE_PUMP_PRE_RUN = 2U,
    HP_STATE_EEV_PREPOSITION = 3U,
    HP_STATE_COMPRESSOR_START = 4U,
    HP_STATE_RUNNING = 5U,
    HP_STATE_POST_RUN = 6U,
    HP_STATE_FAULT = 7U
} HeatPumpControl_State;

#define HP_CONTROL_FAULT_HIGH_TEMP_MASK       0x0010U
#define HP_CONTROL_FAULT_FREEZE_MASK          0x0020U
#define HP_CONTROL_FAULT_TANK_OVERTEMP_MASK   0x0040U
#define HP_CONTROL_FAULT_EEV_MIN_OPEN_MASK    0x0080U

extern volatile uint16_t g_hp_state;
extern volatile uint16_t g_hp_previous_state;
extern volatile uint16_t g_hp_active_fault;
extern volatile uint16_t g_hp_latched_fault;
extern volatile uint16_t g_hp_heating_request;
extern volatile uint16_t g_hp_compressor_allowed;
extern volatile uint16_t g_hp_pump_command;
extern volatile uint16_t g_hp_fan_command;
extern volatile uint16_t g_hp_heater_command;
extern volatile uint16_t g_hp_eev_target_percent;
extern volatile uint16_t g_hp_eev_preposition_done;
extern volatile uint16_t g_hp_eev_target_steps;
extern volatile float g_hp_top_dhwt_temp_c;
extern volatile float g_hp_bottom_dhwt_temp_c;
extern volatile float g_hp_ost_temp_c;
extern volatile float g_hp_dhwt_compensated_setpoint_c;
extern volatile float g_hp_supply_hot_water_temp_c;
extern volatile float g_hp_return_water_temp_c;
extern volatile uint16_t g_hp_top_dhwt_valid;
extern volatile uint16_t g_hp_bottom_dhwt_valid;
extern volatile uint16_t g_hp_ost_valid;
extern volatile uint16_t g_hp_supply_hot_water_valid;
extern volatile uint16_t g_hp_return_water_valid;
extern volatile uint16_t g_hp_safety_fault;
extern volatile uint32_t g_hp_state_elapsed_ms;
extern volatile uint16_t g_hp_pump_pre_run_remaining_sec;
extern volatile uint16_t g_hp_pump_post_run_remaining_sec;
extern volatile uint16_t g_hp_anti_short_cycle_remaining_sec;
extern volatile uint16_t g_hp_compressor_min_off_remaining_sec;
extern volatile uint16_t g_hp_min_run_remaining_sec;
extern volatile uint16_t g_hp_lp_bypass_remaining_sec;
extern volatile uint16_t g_hp_compressor_start_ready;
extern volatile uint16_t g_hp_flow_proving_active;
extern volatile uint16_t g_hp_flow_proven;
extern volatile uint16_t g_hp_fault_retry_count;
extern volatile uint16_t g_hp_fault_retry_limit;
extern volatile uint16_t g_hp_fault_retry_pending;
extern volatile uint16_t g_hp_fault_lockout;
extern volatile uint16_t g_hp_last_fault;
extern volatile float g_hp_suction_temp_c;
extern volatile float g_hp_evap_temp_c;
extern volatile float g_hp_superheat_delta_t_c;
extern volatile float g_hp_superheat_target_c;
extern volatile uint16_t g_hp_suction_temp_valid;
extern volatile uint16_t g_hp_evap_temp_valid;
extern volatile uint16_t g_hp_eev_control_enabled;
extern volatile uint16_t g_hp_eev_control_remaining_sec;
extern volatile int16_t g_hp_eev_last_step_command;
extern volatile uint16_t g_hp_eev_control_max_percent;
extern volatile uint16_t g_hp_eev_control_max_steps;
extern volatile uint16_t g_hp_eev_control_limit_active;
extern volatile uint16_t g_hp_eev_control_min_percent;
extern volatile uint16_t g_hp_eev_control_min_steps;
extern volatile uint16_t g_hp_eev_control_min_limit_active;

void HeatPumpControl_Init(void);
void HeatPumpControl_Task(void);

#endif
