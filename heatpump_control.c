#include <stdbool.h>
#include <stdint.h>
#include "faults/fault_inputs.h"
#include "ntc_loops.h"
#include "outputs.h"
#include "eeprom_24fc04.h"
#include "eev.h"
#include "heatpump_control.h"

extern volatile uint16_t g_ntc_adc_raw;
extern volatile float g_shwts_temperature_c;

volatile uint16_t g_hp_state = HP_STATE_IDLE;
volatile uint16_t g_hp_previous_state = HP_STATE_IDLE;
volatile uint16_t g_hp_active_fault = 0;
volatile uint16_t g_hp_latched_fault = 0;
volatile uint16_t g_hp_heating_request = 0;
volatile uint16_t g_hp_compressor_allowed = 0;
volatile uint16_t g_hp_pump_command = 0;
volatile uint16_t g_hp_fan_command = 0;
volatile uint16_t g_hp_heater_command = 0;
volatile uint16_t g_hp_eev_target_percent = 0;
volatile uint16_t g_hp_eev_preposition_done = 0;
volatile uint16_t g_hp_eev_target_steps = 0;
volatile float g_hp_top_dhwt_temp_c = 0.0f;
volatile float g_hp_bottom_dhwt_temp_c = 0.0f;
volatile float g_hp_ost_temp_c = 0.0f;
volatile float g_hp_dhwt_compensated_setpoint_c = 0.0f;
volatile float g_hp_supply_hot_water_temp_c = 0.0f;
volatile float g_hp_return_water_temp_c = 0.0f;
volatile uint16_t g_hp_top_dhwt_valid = 0;
volatile uint16_t g_hp_bottom_dhwt_valid = 0;
volatile uint16_t g_hp_ost_valid = 0;
volatile uint16_t g_hp_supply_hot_water_valid = 0;
volatile uint16_t g_hp_return_water_valid = 0;
volatile uint16_t g_hp_safety_fault = 0;
volatile uint32_t g_hp_state_elapsed_ms = 0;
volatile uint16_t g_hp_pump_pre_run_remaining_sec = 0;
volatile uint16_t g_hp_pump_post_run_remaining_sec = 0;
volatile uint16_t g_hp_anti_short_cycle_remaining_sec = 0;
volatile uint16_t g_hp_compressor_min_off_remaining_sec = 0;
volatile uint16_t g_hp_min_run_remaining_sec = 0;
volatile uint16_t g_hp_lp_bypass_remaining_sec = 0;
volatile uint16_t g_hp_compressor_start_ready = 0;
volatile uint16_t g_hp_flow_proving_active = 0;
volatile uint16_t g_hp_flow_proven = 0;
volatile uint16_t g_hp_fault_retry_count = 0;
volatile uint16_t g_hp_fault_retry_limit = 0;
volatile uint16_t g_hp_fault_retry_pending = 0;
volatile uint16_t g_hp_fault_lockout = 0;
volatile uint16_t g_hp_last_fault = 0;
volatile float g_hp_suction_temp_c = 0.0f;
volatile float g_hp_evap_temp_c = 0.0f;
volatile float g_hp_superheat_delta_t_c = 0.0f;
volatile float g_hp_superheat_target_c = 0.0f;
volatile uint16_t g_hp_suction_temp_valid = 0;
volatile uint16_t g_hp_evap_temp_valid = 0;
volatile uint16_t g_hp_eev_control_enabled = 0;
volatile uint16_t g_hp_eev_control_remaining_sec = 0;
volatile int16_t g_hp_eev_last_step_command = 0;
volatile uint16_t g_hp_eev_control_max_percent = 0;
volatile uint16_t g_hp_eev_control_max_steps = 0;
volatile uint16_t g_hp_eev_control_limit_active = 0;
volatile uint16_t g_hp_eev_control_min_percent = 0;
volatile uint16_t g_hp_eev_control_min_steps = 0;
volatile uint16_t g_hp_eev_control_min_limit_active = 0;

/*
    Bench test only:
    Set to 1 while NTC sensors are unplugged on the bench so the Part 1 state
    machine can be observed without entering HP_STATE_FAULT from sensor bits.

    Set to 0 before real compressor testing so sensor faults block control.
*/
#define HP_IGNORE_SENSOR_FAULTS_FOR_BENCH_TEST    0
#define HP_SENSOR_FAULT_SHIFT                      8U
#define HP_ENABLE_COMPRESSOR_OUTPUT                1
#define HP_ENABLE_FAN_OUTPUT                       1
#define HP_CONTROL_TASK_PERIOD_MS                  500UL
#define HP_EEV_PREPOSITION_PERCENT                 25U
#define HP_FAULT_AUTO_RETRY_LIMIT                  3U
#define HP_MIN_COMPRESSOR_RUN_SEC                  120U
#define HP_EEV_CONTROL_INTERVAL_MS                 5000UL
#define HP_SUPERHEAT_DEADBAND_C                    0.5f
#define HP_EEV_CONTROL_MAX_PERCENT                 40U
#define HP_EEV_CONTROL_MIN_PERCENT                 10U
#define HP_DHWT_COMP_OST_LOW_C                     5.0f
#define HP_DHWT_COMP_OST_HIGH_C                    20.0f
#define HP_DHWT_COMP_SETPOINT_AT_LOW_OST_C         70.0f
#define HP_DHWT_COMP_SETPOINT_AT_HIGH_OST_C        50.0f
#define HP_HIGH_WATER_TEMP_STOP_C                  75.0f
#define HP_EVAP_FREEZE_STOP_C                      2.0f
#define HP_TANK_OVERTEMP_STOP_C                    75.0f

static void HeatPumpControl_SetState(HeatPumpControl_State next_state);
static void HeatPumpControl_AllCommandsOff(void);
static void HeatPumpControl_ApplyCommands(void);
static uint16_t HeatPumpControl_ReadFaults(void);
static void HeatPumpControl_UpdateSafetySensors(void);
static uint16_t HeatPumpControl_ReadSafetyFaults(void);
static void HeatPumpControl_RecordFault(uint16_t faults);
static bool HeatPumpControl_RawFlowFaultIsActive(void);
static bool HeatPumpControl_PrepositionEEV(void);
static uint16_t HeatPumpControl_EEVPercentToSteps(uint16_t percent);
static uint16_t HeatPumpControl_EEVControlMaxSteps(void);
static uint16_t HeatPumpControl_EEVControlMinSteps(void);
static void HeatPumpControl_UpdateHeatingRequest(void);
static void HeatPumpControl_UpdateTankTemperatures(void);
static float HeatPumpControl_CalculateCompensatedSetpoint(void);
static bool HeatPumpControl_TankSensorsAreValid(void);
static void HeatPumpControl_UpdateElapsedTime(void);
static void HeatPumpControl_UpdateCompressorOffTimer(void);
static uint16_t HeatPumpControl_SecondsRemaining(uint32_t target_ms);
static uint32_t HeatPumpControl_PumpPreRunTargetMs(void);
static uint32_t HeatPumpControl_PumpPostRunTargetMs(void);
static uint32_t HeatPumpControl_AntiShortCycleTargetMs(void);
static uint32_t HeatPumpControl_MinRunTargetMs(void);
static uint32_t HeatPumpControl_LPBypassTargetMs(void);
static bool HeatPumpControl_LPBypassIsActive(void);
static bool HeatPumpControl_EEVMinimumOpeningIsSafe(void);
static void HeatPumpControl_UpdateRunningEEVControl(void);
static void HeatPumpControl_ResetRunningEEVControl(void);
static void HeatPumpControl_ReadSettings(void);

static uint32_t s_hp_eev_control_elapsed_ms = 0UL;
static uint32_t s_hp_compressor_off_elapsed_ms = 0xFFFFFFFFUL;

void HeatPumpControl_Init(void)
{
    Outputs_Init();
    Outputs_Write(0U);

    HeatPumpControl_AllCommandsOff();

    g_hp_state = HP_STATE_IDLE;
    g_hp_previous_state = HP_STATE_IDLE;
    g_hp_active_fault = 0U;
    g_hp_latched_fault = 0U;
    g_hp_heating_request = 0U;
    g_hp_top_dhwt_temp_c = 0.0f;
    g_hp_bottom_dhwt_temp_c = 0.0f;
    g_hp_ost_temp_c = 0.0f;
    g_hp_dhwt_compensated_setpoint_c = g_tank_temp_setpoint_c;
    g_hp_supply_hot_water_temp_c = 0.0f;
    g_hp_return_water_temp_c = 0.0f;
    g_hp_top_dhwt_valid = 0U;
    g_hp_bottom_dhwt_valid = 0U;
    g_hp_ost_valid = 0U;
    g_hp_supply_hot_water_valid = 0U;
    g_hp_return_water_valid = 0U;
    g_hp_safety_fault = 0U;
    g_hp_state_elapsed_ms = 0UL;
    g_hp_pump_pre_run_remaining_sec = 0U;
    g_hp_pump_post_run_remaining_sec = 0U;
    g_hp_anti_short_cycle_remaining_sec = 0U;
    g_hp_compressor_min_off_remaining_sec = 0U;
    g_hp_min_run_remaining_sec = 0U;
    g_hp_lp_bypass_remaining_sec = 0U;
    g_hp_compressor_start_ready = 0U;
    g_hp_flow_proving_active = 0U;
    g_hp_flow_proven = 0U;
    g_hp_fault_retry_count = 0U;
    g_hp_fault_retry_limit = HP_FAULT_AUTO_RETRY_LIMIT;
    g_hp_fault_retry_pending = 0U;
    g_hp_fault_lockout = 0U;
    g_hp_last_fault = 0U;
    g_hp_eev_control_max_percent = HP_EEV_CONTROL_MAX_PERCENT;
    g_hp_eev_control_max_steps = HeatPumpControl_EEVControlMaxSteps();
    g_hp_eev_control_limit_active = 0U;
    g_hp_eev_control_min_percent = HP_EEV_CONTROL_MIN_PERCENT;
    g_hp_eev_control_min_steps = HeatPumpControl_EEVControlMinSteps();
    g_hp_eev_control_min_limit_active = 0U;
    HeatPumpControl_ResetRunningEEVControl();
    g_hp_eev_preposition_done = 0U;
    g_hp_eev_target_steps = 0U;
    s_hp_compressor_off_elapsed_ms = HeatPumpControl_AntiShortCycleTargetMs();
}

void HeatPumpControl_Task(void)
{
    HeatPumpControl_UpdateElapsedTime();
    HeatPumpControl_UpdateCompressorOffTimer();
    HeatPumpControl_ReadSettings();
    HeatPumpControl_UpdateHeatingRequest();
    HeatPumpControl_UpdateSafetySensors();

    g_hp_active_fault = HeatPumpControl_ReadFaults();

    if (g_hp_fault_lockout != 0U)
    {
        if (g_hp_latched_fault == 0U)
        {
            g_hp_latched_fault = g_hp_last_fault;
        }

        HeatPumpControl_AllCommandsOff();
        HeatPumpControl_ApplyCommands();
        HeatPumpControl_SetState(HP_STATE_FAULT);
        return;
    }

    if (g_hp_active_fault != 0U)
    {
        HeatPumpControl_RecordFault(g_hp_active_fault);
        HeatPumpControl_AllCommandsOff();
        HeatPumpControl_ApplyCommands();
        HeatPumpControl_SetState(HP_STATE_FAULT);
        return;
    }

    if (HeatPumpControl_TankSensorsAreValid() == false)
    {
        HeatPumpControl_AllCommandsOff();
        HeatPumpControl_ApplyCommands();
        HeatPumpControl_SetState(HP_STATE_IDLE);
        return;
    }

    switch ((HeatPumpControl_State)g_hp_state)
    {
        case HP_STATE_IDLE:
            HeatPumpControl_AllCommandsOff();
            g_hp_flow_proving_active = 0U;
            g_hp_flow_proven = 0U;
            g_hp_eev_preposition_done = 0U;
            g_hp_eev_target_steps = 0U;
            g_hp_anti_short_cycle_remaining_sec = 0U;
            g_hp_min_run_remaining_sec = 0U;
            g_hp_lp_bypass_remaining_sec = 0U;
            g_hp_pump_post_run_remaining_sec = 0U;
            g_hp_compressor_start_ready = 0U;
            HeatPumpControl_ResetRunningEEVControl();

            if (g_hp_heating_request != 0U)
            {
                HeatPumpControl_SetState(HP_STATE_PRECHECK);
            }
            break;

        case HP_STATE_PRECHECK:
            HeatPumpControl_AllCommandsOff();
            g_hp_flow_proving_active = 0U;
            g_hp_flow_proven = 0U;
            g_hp_eev_preposition_done = 0U;
            g_hp_eev_target_steps = 0U;
            g_hp_anti_short_cycle_remaining_sec = 0U;
            g_hp_min_run_remaining_sec = 0U;
            g_hp_lp_bypass_remaining_sec = 0U;
            g_hp_pump_post_run_remaining_sec = 0U;
            g_hp_compressor_start_ready = 0U;
            HeatPumpControl_ResetRunningEEVControl();

            if (g_hp_heating_request == 0U)
            {
                HeatPumpControl_SetState(HP_STATE_IDLE);
            }
            else
            {
                HeatPumpControl_SetState(HP_STATE_PUMP_PRE_RUN);
            }
            break;

        case HP_STATE_PUMP_PRE_RUN:
            HeatPumpControl_AllCommandsOff();
            g_hp_pump_command = 1U;
            g_hp_flow_proving_active = 1U;
            g_hp_flow_proven = 0U;
            g_hp_compressor_start_ready = 0U;
            HeatPumpControl_ResetRunningEEVControl();
            g_hp_pump_pre_run_remaining_sec =
                HeatPumpControl_SecondsRemaining(HeatPumpControl_PumpPreRunTargetMs());

            if (g_hp_heating_request == 0U)
            {
                HeatPumpControl_AllCommandsOff();
                HeatPumpControl_SetState(HP_STATE_IDLE);
            }
            else if (g_hp_state_elapsed_ms >= HeatPumpControl_PumpPreRunTargetMs())
            {
                g_hp_pump_pre_run_remaining_sec = 0U;

                if (HeatPumpControl_RawFlowFaultIsActive() != false)
                {
                    g_hp_flow_proving_active = 0U;
                    g_hp_active_fault |= FAULT_INPUT_FLOW_MASK;
                    HeatPumpControl_RecordFault(g_hp_active_fault);
                    HeatPumpControl_AllCommandsOff();
                    HeatPumpControl_SetState(HP_STATE_FAULT);
                }
                else
                {
                    g_hp_flow_proving_active = 0U;
                    g_hp_flow_proven = 1U;
                    HeatPumpControl_SetState(HP_STATE_EEV_PREPOSITION);
                }
            }
            break;

        case HP_STATE_EEV_PREPOSITION:
            HeatPumpControl_AllCommandsOff();
            g_hp_pump_command = 1U;
            g_hp_pump_pre_run_remaining_sec = 0U;
            g_hp_flow_proving_active = 0U;
            g_hp_compressor_start_ready = 0U;
            HeatPumpControl_ResetRunningEEVControl();
            g_hp_eev_target_percent = HP_EEV_PREPOSITION_PERCENT;
            g_hp_eev_target_steps =
                HeatPumpControl_EEVPercentToSteps(g_hp_eev_target_percent);

            if (g_hp_heating_request == 0U)
            {
                HeatPumpControl_AllCommandsOff();
                HeatPumpControl_SetState(HP_STATE_IDLE);
            }
            else if (g_hp_eev_preposition_done == 0U)
            {
                if (HeatPumpControl_PrepositionEEV() != false)
                {
                    HeatPumpControl_SetState(HP_STATE_COMPRESSOR_START);
                }
            }
            else
            {
                HeatPumpControl_SetState(HP_STATE_COMPRESSOR_START);
            }
            break;

        case HP_STATE_FAULT:
            HeatPumpControl_AllCommandsOff();
            g_hp_flow_proving_active = 0U;
            g_hp_flow_proven = 0U;
            g_hp_compressor_start_ready = 0U;
            g_hp_lp_bypass_remaining_sec = 0U;
            g_hp_min_run_remaining_sec = 0U;
            HeatPumpControl_ResetRunningEEVControl();

            if (g_hp_fault_lockout != 0U)
            {
                g_hp_latched_fault = g_hp_last_fault;
            }
            else if (g_hp_active_fault == 0U)
            {
                g_hp_latched_fault = 0U;
                g_hp_fault_retry_pending = 0U;
                HeatPumpControl_SetState(HP_STATE_IDLE);
            }
            break;

        case HP_STATE_COMPRESSOR_START:
            HeatPumpControl_AllCommandsOff();
            g_hp_pump_command = 1U;
            g_hp_fan_command = 1U;
            g_hp_lp_bypass_remaining_sec =
                HeatPumpControl_SecondsRemaining(HeatPumpControl_LPBypassTargetMs());
            g_hp_anti_short_cycle_remaining_sec =
                HeatPumpControl_SecondsRemaining(HeatPumpControl_AntiShortCycleTargetMs());
            g_hp_min_run_remaining_sec = 0U;
            HeatPumpControl_ResetRunningEEVControl();
            g_hp_eev_target_percent = HP_EEV_PREPOSITION_PERCENT;
            g_hp_eev_target_steps =
                HeatPumpControl_EEVPercentToSteps(g_hp_eev_target_percent);

            if (g_hp_heating_request == 0U)
            {
                HeatPumpControl_AllCommandsOff();
                g_hp_pump_command = 1U;
                g_hp_fan_command = 1U;
                HeatPumpControl_SetState(HP_STATE_POST_RUN);
            }
            else if ((g_hp_anti_short_cycle_remaining_sec == 0U) &&
                     (g_hp_compressor_min_off_remaining_sec == 0U) &&
                     (HeatPumpControl_EEVMinimumOpeningIsSafe() != false))
            {
                g_hp_compressor_start_ready = 1U;
                g_hp_compressor_allowed = 1U;
                HeatPumpControl_SetState(HP_STATE_RUNNING);
            }
            break;

        case HP_STATE_RUNNING:
            HeatPumpControl_AllCommandsOff();
            g_hp_pump_command = 1U;
            g_hp_fan_command = 1U;
            g_hp_compressor_start_ready = 1U;
            g_hp_compressor_allowed = 1U;
            g_hp_lp_bypass_remaining_sec =
                HeatPumpControl_SecondsRemaining(HeatPumpControl_LPBypassTargetMs());
            g_hp_min_run_remaining_sec =
                HeatPumpControl_SecondsRemaining(HeatPumpControl_MinRunTargetMs());
            g_hp_anti_short_cycle_remaining_sec = 0U;
            g_hp_compressor_min_off_remaining_sec = 0U;
            HeatPumpControl_UpdateRunningEEVControl();

            if ((g_hp_heating_request == 0U) &&
                (g_hp_min_run_remaining_sec == 0U))
            {
                HeatPumpControl_AllCommandsOff();
                g_hp_pump_command = 1U;
                g_hp_fan_command = 1U;
                HeatPumpControl_ResetRunningEEVControl();
                HeatPumpControl_SetState(HP_STATE_POST_RUN);
            }
            break;

        case HP_STATE_POST_RUN:
            HeatPumpControl_AllCommandsOff();
            g_hp_pump_command = 1U;
            g_hp_fan_command = 1U;
            g_hp_compressor_start_ready = 0U;
            g_hp_lp_bypass_remaining_sec = 0U;
            g_hp_anti_short_cycle_remaining_sec = 0U;
            g_hp_min_run_remaining_sec = 0U;
            HeatPumpControl_ResetRunningEEVControl();
            g_hp_pump_post_run_remaining_sec =
                HeatPumpControl_SecondsRemaining(HeatPumpControl_PumpPostRunTargetMs());

            if (g_hp_state_elapsed_ms >= HeatPumpControl_PumpPostRunTargetMs())
            {
                g_hp_pump_post_run_remaining_sec = 0U;
                HeatPumpControl_AllCommandsOff();
                HeatPumpControl_SetState(HP_STATE_IDLE);
            }
            break;

        default:
            HeatPumpControl_AllCommandsOff();
            HeatPumpControl_SetState(HP_STATE_IDLE);
            break;
    }

    HeatPumpControl_ApplyCommands();
}

static void HeatPumpControl_SetState(HeatPumpControl_State next_state)
{
    if (g_hp_state != (uint16_t)next_state)
    {
        g_hp_previous_state = g_hp_state;
        g_hp_state = (uint16_t)next_state;
        g_hp_state_elapsed_ms = 0UL;
    }
}

static void HeatPumpControl_AllCommandsOff(void)
{
    g_hp_compressor_allowed = 0U;
    g_hp_pump_command = 0U;
    g_hp_fan_command = 0U;
    g_hp_heater_command = 0U;
    g_hp_eev_target_percent = 0U;
    g_hp_pump_pre_run_remaining_sec = 0U;
}

static void HeatPumpControl_ApplyCommands(void)
{
#if (HP_ENABLE_COMPRESSOR_OUTPUT != 0)
    Outputs_SetCompressor(g_hp_compressor_allowed != 0U);
#else
    Outputs_SetCompressor(false);
#endif

    Outputs_SetHeater(g_hp_heater_command != 0U);
#if (HP_ENABLE_FAN_OUTPUT != 0)
    Outputs_SetFan(g_hp_fan_command != 0U);
#else
    Outputs_SetFan(false);
#endif
    Outputs_SetPump(g_hp_pump_command != 0U);
    Outputs_SetFourWayValve(false);
}

static uint16_t HeatPumpControl_ReadFaults(void)
{
    uint16_t faults;
    uint16_t raw_faults;

    (void)FaultInputs_IsActive();
    raw_faults = g_fault_inputs;

    faults = raw_faults & FAULT_INPUT_HP_MASK;
    g_hp_safety_fault = HeatPumpControl_ReadSafetyFaults();
    faults |= g_hp_safety_fault;

    if ((raw_faults & FAULT_INPUT_LP_MASK) != 0U)
    {
        if (HeatPumpControl_LPBypassIsActive() == false)
        {
            faults |= FAULT_INPUT_LP_MASK;
        }
    }

    if ((raw_faults & FAULT_INPUT_FLOW_MASK) != 0U)
    {
        /*
            Flow is active-high fault in the input module. On this system it is
            expected to be high before the pump runs, then low after flow proves.
        */
        switch ((HeatPumpControl_State)g_hp_state)
        {
            case HP_STATE_IDLE:
            case HP_STATE_PRECHECK:
            case HP_STATE_PUMP_PRE_RUN:
            case HP_STATE_FAULT:
                break;

            default:
                faults |= FAULT_INPUT_FLOW_MASK;
                break;
        }
    }

#if (HP_IGNORE_SENSOR_FAULTS_FOR_BENCH_TEST == 0)
    if (g_sensor_fault_inputs != 0U)
    {
        faults |= (uint16_t)(g_sensor_fault_inputs << HP_SENSOR_FAULT_SHIFT);
    }
#endif

    return faults;
}

static void HeatPumpControl_UpdateSafetySensors(void)
{
    g_hp_supply_hot_water_temp_c = g_shwts_temperature_c;
    g_hp_return_water_temp_c = g_rhwts_temperature_c;
    g_hp_evap_temp_c = g_evpts_temperature_c;
    g_hp_suction_temp_c = g_sucts_temperature_c;

    g_hp_supply_hot_water_valid =
        (FaultInputs_IsSensorOpenOrShort(g_ntc_adc_raw) == false) ? 1U : 0U;
    g_hp_return_water_valid =
        (FaultInputs_IsSensorOpenOrShort(g_rhwts_adc_raw) == false) ? 1U : 0U;
    g_hp_evap_temp_valid =
        (FaultInputs_IsSensorOpenOrShort(g_evpts_adc_raw) == false) ? 1U : 0U;
    g_hp_suction_temp_valid =
        (FaultInputs_IsSensorOpenOrShort(g_sucts_adc_raw) == false) ? 1U : 0U;
}

static uint16_t HeatPumpControl_ReadSafetyFaults(void)
{
    uint16_t faults = 0U;
    bool compressor_state;

    compressor_state =
        (((HeatPumpControl_State)g_hp_state == HP_STATE_COMPRESSOR_START) ||
         ((HeatPumpControl_State)g_hp_state == HP_STATE_RUNNING));

    if ((compressor_state != false) &&
        (g_hp_supply_hot_water_valid != 0U) &&
        (g_hp_supply_hot_water_temp_c >= HP_HIGH_WATER_TEMP_STOP_C))
    {
        faults |= HP_CONTROL_FAULT_HIGH_TEMP_MASK;
    }

    if ((compressor_state != false) &&
        (g_hp_return_water_valid != 0U) &&
        (g_hp_return_water_temp_c >= HP_HIGH_WATER_TEMP_STOP_C))
    {
        faults |= HP_CONTROL_FAULT_HIGH_TEMP_MASK;
    }

    if ((compressor_state != false) &&
        (g_hp_evap_temp_valid != 0U) &&
        (g_hp_evap_temp_c <= HP_EVAP_FREEZE_STOP_C))
    {
        faults |= HP_CONTROL_FAULT_FREEZE_MASK;
    }

    if (((g_hp_top_dhwt_valid != 0U) &&
         (g_hp_top_dhwt_temp_c >= HP_TANK_OVERTEMP_STOP_C)) ||
        ((g_hp_bottom_dhwt_valid != 0U) &&
         (g_hp_bottom_dhwt_temp_c >= HP_TANK_OVERTEMP_STOP_C)))
    {
        faults |= HP_CONTROL_FAULT_TANK_OVERTEMP_MASK;
    }

    if (((HeatPumpControl_State)g_hp_state == HP_STATE_RUNNING) &&
        (g_eev_init_done != 0U) &&
        (EEV_GetPositionSteps() < HeatPumpControl_EEVControlMinSteps()))
    {
        faults |= HP_CONTROL_FAULT_EEV_MIN_OPEN_MASK;
    }

    return faults;
}

static void HeatPumpControl_RecordFault(uint16_t faults)
{
    if (faults == 0U)
    {
        return;
    }

    g_hp_last_fault = faults;
    g_hp_latched_fault = faults;

    /*
        Count one retry per fault event, not once per polling loop. The first
        three fault events may auto-restart after the fault clears. A later
        fault locks out until MCU reset or power cycle.
    */
    if (g_hp_fault_retry_pending == 0U)
    {
        g_hp_fault_retry_pending = 1U;

        if (g_hp_fault_retry_count < g_hp_fault_retry_limit)
        {
            g_hp_fault_retry_count++;
        }
        else
        {
            g_hp_fault_lockout = 1U;
        }
    }
}

static bool HeatPumpControl_RawFlowFaultIsActive(void)
{
    (void)FaultInputs_IsActive();
    return ((g_fault_inputs & FAULT_INPUT_FLOW_MASK) != 0U);
}

static bool HeatPumpControl_PrepositionEEV(void)
{
    uint16_t preposition_percent;

    /*
        TODO: Add a separate EEPROM setting for EEV startup open position.
        The current EEPROM "startup" EEV setting is the startup close count,
        so the staged control uses a fixed 25% opening before compressor-start permission.
    */
    preposition_percent = HP_EEV_PREPOSITION_PERCENT;
    if (preposition_percent > HP_EEV_CONTROL_MAX_PERCENT)
    {
        preposition_percent = HP_EEV_CONTROL_MAX_PERCENT;
    }

    g_hp_eev_target_percent = preposition_percent;
    g_hp_eev_target_steps =
        HeatPumpControl_EEVPercentToSteps(g_hp_eev_target_percent);
    g_hp_eev_control_max_percent = HP_EEV_CONTROL_MAX_PERCENT;
    g_hp_eev_control_max_steps = HeatPumpControl_EEVControlMaxSteps();
    g_hp_eev_control_min_percent = HP_EEV_CONTROL_MIN_PERCENT;
    g_hp_eev_control_min_steps = HeatPumpControl_EEVControlMinSteps();

    if (g_eev_init_done == 0U)
    {
        g_hp_eev_preposition_done = 0U;
        return false;
    }

    EEV_MoveToPercent((uint8_t)preposition_percent);

    if (EEV_GetPositionSteps() == g_hp_eev_target_steps)
    {
        g_hp_eev_preposition_done = 1U;
        return true;
    }

    g_hp_eev_preposition_done = 0U;
    return false;
}

static uint16_t HeatPumpControl_EEVPercentToSteps(uint16_t percent)
{
    uint32_t steps;

    if (percent > 100U)
    {
        percent = 100U;
    }

    steps = ((uint32_t)percent * (uint32_t)EEV_MAX_POSITION_STEPS) / 100UL;

    if (steps > EEV_MAX_POSITION_STEPS)
    {
        steps = EEV_MAX_POSITION_STEPS;
    }

    return (uint16_t)steps;
}

static uint16_t HeatPumpControl_EEVControlMaxSteps(void)
{
    return HeatPumpControl_EEVPercentToSteps(HP_EEV_CONTROL_MAX_PERCENT);
}

static uint16_t HeatPumpControl_EEVControlMinSteps(void)
{
    return HeatPumpControl_EEVPercentToSteps(HP_EEV_CONTROL_MIN_PERCENT);
}

static void HeatPumpControl_UpdateHeatingRequest(void)
{
    float start_temperature_c;
    float stop_temperature_c;

    HeatPumpControl_UpdateTankTemperatures();
    g_hp_dhwt_compensated_setpoint_c =
        HeatPumpControl_CalculateCompensatedSetpoint();

    if (HeatPumpControl_TankSensorsAreValid() == false)
    {
        g_hp_heating_request = 0U;
        return;
    }

    stop_temperature_c = g_hp_dhwt_compensated_setpoint_c;
    start_temperature_c = stop_temperature_c - g_tank_hysteresis_c;

    if (g_hp_heating_request != 0U)
    {
        if (g_hp_top_dhwt_temp_c >= stop_temperature_c)
        {
            g_hp_heating_request = 0U;
        }
    }
    else if (g_hp_bottom_dhwt_temp_c <= start_temperature_c)
    {
        g_hp_heating_request = 1U;
    }
}

static void HeatPumpControl_UpdateTankTemperatures(void)
{
    /*
        DHWT tank demand sensors:
        TopDHWTS stops heating at setpoint.
        BotDHWTS starts heating at setpoint minus hysteresis.
    */
    g_hp_top_dhwt_temp_c = g_topdhwts_temperature_c;
    g_hp_bottom_dhwt_temp_c = g_botdhwts_temperature_c;
    g_hp_ost_temp_c = g_ost_temperature_c;

    g_hp_top_dhwt_valid =
        (FaultInputs_IsSensorOpenOrShort(g_topdhwts_adc_raw) == false) ? 1U : 0U;
    g_hp_bottom_dhwt_valid =
        (FaultInputs_IsSensorOpenOrShort(g_botdhwts_adc_raw) == false) ? 1U : 0U;
    g_hp_ost_valid =
        (FaultInputs_IsSensorOpenOrShort(g_ost_adc_raw) == false) ? 1U : 0U;
}

static float HeatPumpControl_CalculateCompensatedSetpoint(void)
{
    float ost_span_c;
    float setpoint_span_c;
    float ratio;

    /*
        Outdoor temperature compensation:
        OST <= 5 C  -> DHWT setpoint 70 C.
        OST >= 20 C -> DHWT setpoint 50 C.
        Between 5 C and 20 C the setpoint is linearly interpolated.

        If the OST sensor is invalid, keep using the EEPROM tank setpoint.
    */
    if (g_hp_ost_valid == 0U)
    {
        return g_tank_temp_setpoint_c;
    }

    if (g_hp_ost_temp_c <= HP_DHWT_COMP_OST_LOW_C)
    {
        return HP_DHWT_COMP_SETPOINT_AT_LOW_OST_C;
    }

    if (g_hp_ost_temp_c >= HP_DHWT_COMP_OST_HIGH_C)
    {
        return HP_DHWT_COMP_SETPOINT_AT_HIGH_OST_C;
    }

    ost_span_c = HP_DHWT_COMP_OST_HIGH_C - HP_DHWT_COMP_OST_LOW_C;
    setpoint_span_c = HP_DHWT_COMP_SETPOINT_AT_LOW_OST_C -
                      HP_DHWT_COMP_SETPOINT_AT_HIGH_OST_C;
    ratio = (g_hp_ost_temp_c - HP_DHWT_COMP_OST_LOW_C) / ost_span_c;

    return HP_DHWT_COMP_SETPOINT_AT_LOW_OST_C - (ratio * setpoint_span_c);
}

static bool HeatPumpControl_TankSensorsAreValid(void)
{
    return ((g_hp_top_dhwt_valid != 0U) &&
            (g_hp_bottom_dhwt_valid != 0U));
}

static void HeatPumpControl_UpdateElapsedTime(void)
{
    if (g_hp_state_elapsed_ms <= (0xFFFFFFFFUL - HP_CONTROL_TASK_PERIOD_MS))
    {
        g_hp_state_elapsed_ms += HP_CONTROL_TASK_PERIOD_MS;
    }

}

static void HeatPumpControl_UpdateCompressorOffTimer(void)
{
    uint32_t target_ms;

    target_ms = HeatPumpControl_AntiShortCycleTargetMs();

    if (g_hp_compressor_allowed != 0U)
    {
        s_hp_compressor_off_elapsed_ms = 0UL;
        g_hp_compressor_min_off_remaining_sec = 0U;
        return;
    }

    if (s_hp_compressor_off_elapsed_ms < target_ms)
    {
        s_hp_compressor_off_elapsed_ms += HP_CONTROL_TASK_PERIOD_MS;
    }

    if (s_hp_compressor_off_elapsed_ms >= target_ms)
    {
        g_hp_compressor_min_off_remaining_sec = 0U;
    }
    else
    {
        g_hp_compressor_min_off_remaining_sec =
            (uint16_t)(((target_ms - s_hp_compressor_off_elapsed_ms) + 999UL) / 1000UL);
    }
}

static uint16_t HeatPumpControl_SecondsRemaining(uint32_t target_ms)
{
    uint32_t remaining_ms;

    if (g_hp_state_elapsed_ms >= target_ms)
    {
        return 0U;
    }

    remaining_ms = target_ms - g_hp_state_elapsed_ms;
    return (uint16_t)((remaining_ms + 999UL) / 1000UL);
}

static uint32_t HeatPumpControl_PumpPreRunTargetMs(void)
{
    return ((uint32_t)g_pump_pre_run_sec * 1000UL);
}

static uint32_t HeatPumpControl_PumpPostRunTargetMs(void)
{
    return ((uint32_t)g_pump_post_run_sec * 1000UL);
}

static uint32_t HeatPumpControl_AntiShortCycleTargetMs(void)
{
    return ((uint32_t)g_compressor_anti_short_cycle_sec * 1000UL);
}

static uint32_t HeatPumpControl_MinRunTargetMs(void)
{
    return ((uint32_t)HP_MIN_COMPRESSOR_RUN_SEC * 1000UL);
}

static uint32_t HeatPumpControl_LPBypassTargetMs(void)
{
    return ((uint32_t)g_lp_startup_bypass_sec * 1000UL);
}

static bool HeatPumpControl_LPBypassIsActive(void)
{
    switch ((HeatPumpControl_State)g_hp_state)
    {
        case HP_STATE_COMPRESSOR_START:
        case HP_STATE_RUNNING:
            return (g_hp_state_elapsed_ms < HeatPumpControl_LPBypassTargetMs());

        default:
            break;
    }

    return false;
}

static bool HeatPumpControl_EEVMinimumOpeningIsSafe(void)
{
    if (g_eev_init_done == 0U)
    {
        return false;
    }

    if (EEV_GetPositionSteps() < HeatPumpControl_EEVControlMinSteps())
    {
        EEV_MoveToPercent((uint8_t)HP_EEV_CONTROL_MIN_PERCENT);
    }

    return (EEV_GetPositionSteps() >= HeatPumpControl_EEVControlMinSteps());
}

static void HeatPumpControl_UpdateRunningEEVControl(void)
{
    float error_c;

    g_hp_suction_temp_c = g_sucts_temperature_c;
    g_hp_evap_temp_c = g_evpts_temperature_c;
    g_hp_superheat_target_c = g_superheat_delta_t_setpoint_c;

    g_hp_suction_temp_valid =
        (FaultInputs_IsSensorOpenOrShort(g_sucts_adc_raw) == false) ? 1U : 0U;
    g_hp_evap_temp_valid =
        (FaultInputs_IsSensorOpenOrShort(g_evpts_adc_raw) == false) ? 1U : 0U;

    if ((g_hp_suction_temp_valid == 0U) ||
        (g_hp_evap_temp_valid == 0U) ||
        (g_eev_init_done == 0U))
    {
        g_hp_eev_control_enabled = 0U;
        g_hp_eev_last_step_command = 0;
        g_hp_eev_control_remaining_sec = 0U;
        return;
    }

    g_hp_superheat_delta_t_c = g_hp_suction_temp_c - g_hp_evap_temp_c;
    g_hp_eev_control_enabled = 1U;
    g_hp_eev_control_max_percent = HP_EEV_CONTROL_MAX_PERCENT;
    g_hp_eev_control_max_steps = HeatPumpControl_EEVControlMaxSteps();
    g_hp_eev_control_min_percent = HP_EEV_CONTROL_MIN_PERCENT;
    g_hp_eev_control_min_steps = HeatPumpControl_EEVControlMinSteps();
    g_hp_eev_target_steps = EEV_GetPositionSteps();
    g_hp_eev_target_percent = EEV_GetPositionPercent();
    g_hp_eev_control_limit_active =
        (g_hp_eev_target_steps >= g_hp_eev_control_max_steps) ? 1U : 0U;
    g_hp_eev_control_min_limit_active =
        (g_hp_eev_target_steps <= g_hp_eev_control_min_steps) ? 1U : 0U;

    if (s_hp_eev_control_elapsed_ms < HP_EEV_CONTROL_INTERVAL_MS)
    {
        s_hp_eev_control_elapsed_ms += HP_CONTROL_TASK_PERIOD_MS;
    }

    if (s_hp_eev_control_elapsed_ms < HP_EEV_CONTROL_INTERVAL_MS)
    {
        g_hp_eev_control_remaining_sec =
            (uint16_t)(((HP_EEV_CONTROL_INTERVAL_MS -
                         s_hp_eev_control_elapsed_ms) + 999UL) / 1000UL);
        g_hp_eev_last_step_command = 0;
        return;
    }

    s_hp_eev_control_elapsed_ms = 0UL;
    g_hp_eev_control_remaining_sec = 0U;
    error_c = g_hp_superheat_delta_t_c - g_hp_superheat_target_c;

    if (EEV_GetPositionSteps() > g_hp_eev_control_max_steps)
    {
        EEV_MoveStepsClose(1U);
        g_hp_eev_control_limit_active = 1U;
        g_hp_eev_control_min_limit_active = 0U;
        g_hp_eev_last_step_command = -1;
    }
    else if (EEV_GetPositionSteps() < g_hp_eev_control_min_steps)
    {
        EEV_MoveStepsOpen(1U);
        g_hp_eev_control_min_limit_active = 1U;
        g_hp_eev_last_step_command = 1;
    }
    else if (error_c > HP_SUPERHEAT_DEADBAND_C)
    {
        if (EEV_GetPositionSteps() < g_hp_eev_control_max_steps)
        {
            EEV_MoveStepsOpen(1U);
            g_hp_eev_last_step_command = 1;
        }
        else
        {
            g_hp_eev_control_limit_active = 1U;
            g_hp_eev_last_step_command = 0;
        }
    }
    else if (error_c < -HP_SUPERHEAT_DEADBAND_C)
    {
        if (EEV_GetPositionSteps() > g_hp_eev_control_min_steps)
        {
            EEV_MoveStepsClose(1U);
            g_hp_eev_last_step_command = -1;
        }
        else
        {
            g_hp_eev_control_min_limit_active = 1U;
            g_hp_eev_last_step_command = 0;
        }
    }
    else
    {
        g_hp_eev_last_step_command = 0;
    }

    g_hp_eev_target_steps = EEV_GetPositionSteps();
    g_hp_eev_target_percent = EEV_GetPositionPercent();
    g_hp_eev_control_limit_active =
        (g_hp_eev_target_steps >= g_hp_eev_control_max_steps) ? 1U : 0U;
    g_hp_eev_control_min_limit_active =
        (g_hp_eev_target_steps <= g_hp_eev_control_min_steps) ? 1U : 0U;
}

static void HeatPumpControl_ResetRunningEEVControl(void)
{
    s_hp_eev_control_elapsed_ms = 0UL;
    g_hp_suction_temp_c = 0.0f;
    g_hp_evap_temp_c = 0.0f;
    g_hp_superheat_delta_t_c = 0.0f;
    g_hp_superheat_target_c = g_superheat_delta_t_setpoint_c;
    g_hp_suction_temp_valid = 0U;
    g_hp_evap_temp_valid = 0U;
    g_hp_eev_control_enabled = 0U;
    g_hp_eev_control_remaining_sec = 0U;
    g_hp_eev_last_step_command = 0;
    g_hp_eev_control_max_percent = HP_EEV_CONTROL_MAX_PERCENT;
    g_hp_eev_control_max_steps = HeatPumpControl_EEVControlMaxSteps();
    g_hp_eev_control_limit_active = 0U;
    g_hp_eev_control_min_percent = HP_EEV_CONTROL_MIN_PERCENT;
    g_hp_eev_control_min_steps = HeatPumpControl_EEVControlMinSteps();
    g_hp_eev_control_min_limit_active = 0U;
}

static void HeatPumpControl_ReadSettings(void)
{
    /*
        The staged control parts use pump pre-run timing and continue observing
        EEPROM-backed timing settings for later safe parts.
    */
    (void)g_tank_temp_setpoint_c;
    (void)g_tank_hysteresis_c;
    (void)g_pump_pre_run_sec;
    (void)g_pump_post_run_sec;
    (void)g_compressor_anti_short_cycle_sec;
    (void)g_superheat_delta_t_setpoint_c;
    (void)g_lp_startup_bypass_sec;
}
