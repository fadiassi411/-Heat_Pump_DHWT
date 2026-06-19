#ifndef EEPROM_24FC04_H
#define EEPROM_24FC04_H

#include <stdbool.h>
#include <stdint.h>
#include "app_config.h"

#define EEPROM24FC04_SIZE_BYTES              512U
#define EEPROM24FC04_BLOCK_SIZE_BYTES        256U
#define EEPROM24FC04_PAGE_SIZE_BYTES         16U

/*
    24FC04/24AA04/24LC04B address map used by this firmware.
    The device has 512 bytes: 0x000..0x1FF.
    Addresses 0x000..0x0FF use control-byte block select B0 = 0.
    Addresses 0x100..0x1FF use control-byte block select B0 = 1.
    A0/A1/A2 package pins are not used for 24FC04 addressing.
*/
#define EEPROM24FC04_ADDR_SETPOINTS          0x000U
#define EEPROM24FC04_SETPOINTS_LENGTH        31U
#define EEPROM24FC04_ADDR_TEST               0x1F0U
#define EEPROM24FC04_TEST_LENGTH             4U

#define EEPROM24FC04_TANK_TEMP_DEFAULT_C     55.0f
#define EEPROM24FC04_SUPERHEAT_DT_DEFAULT_C  3.0f
#define EEPROM24FC04_TANK_HYST_DEFAULT_C     TANK_HYSTERESIS_DEFAULT_C
#define EEPROM24FC04_EEV_STARTUP_DEFAULT     EEV_STARTUP_CLOSE_STEPS
#define EEPROM24FC04_EEV_MAX_DEFAULT         EEV_MAX_POSITION_STEPS
#define EEPROM24FC04_COMP_ASC_DEFAULT_SEC    COMPRESSOR_ANTI_SHORT_CYCLE_SEC
#define EEPROM24FC04_LP_BYPASS_DEFAULT_SEC   LP_STARTUP_BYPASS_SEC
#define EEPROM24FC04_PUMP_PRE_DEFAULT_SEC    PUMP_PRE_RUN_SEC
#define EEPROM24FC04_PUMP_POST_DEFAULT_SEC   PUMP_POST_RUN_SEC
#define EEPROM24FC04_HEATER_DEFAULT_ENABLED  HEATER_ENABLED_DEFAULT

#define EEPROM24FC04_ERROR_NONE              0U
#define EEPROM24FC04_ERROR_NO_ACK            1U
#define EEPROM24FC04_ERROR_WRITE_TIMEOUT     2U
#define EEPROM24FC04_ERROR_READ_TIMEOUT      3U
#define EEPROM24FC04_ERROR_INVALID_ADDRESS   4U
#define EEPROM24FC04_ERROR_CHECKSUM_INVALID  5U
#define EEPROM24FC04_ERROR_PAGE_BOUNDARY     6U

typedef struct
{
    float tank_temp_setpoint_c;
    float superheat_delta_t_setpoint_c;
    float tank_hysteresis_c;
    uint16_t eev_startup_steps;
    uint16_t eev_max_steps;
    uint16_t compressor_anti_short_cycle_sec;
    uint16_t lp_startup_bypass_sec;
    uint16_t pump_pre_run_sec;
    uint16_t pump_post_run_sec;
    bool heater_enabled;
} EEPROM24FC04_Settings;

extern volatile uint16_t g_eeprom_present;
extern volatile uint16_t g_eeprom_error;
extern volatile uint16_t g_eeprom_last_addr;
extern volatile uint16_t g_eeprom_last_data;
extern volatile uint16_t g_eeprom_write_count;
extern volatile uint16_t g_eeprom_read_count;
extern volatile float g_tank_temp_setpoint_c;
extern volatile float g_superheat_delta_t_setpoint_c;
extern volatile float g_tank_hysteresis_c;
extern volatile uint16_t g_eev_startup_steps_setting;
extern volatile uint16_t g_eev_max_steps_setting;
extern volatile uint16_t g_compressor_anti_short_cycle_sec;
extern volatile uint16_t g_lp_startup_bypass_sec;
extern volatile uint16_t g_pump_pre_run_sec;
extern volatile uint16_t g_pump_post_run_sec;
extern volatile uint16_t g_heater_enabled_setting;

void EEPROM24FC04_Init(void);
bool EEPROM24FC04_IsPresent(void);
bool EEPROM24FC04_WriteByte(uint16_t mem_addr, uint8_t data);
bool EEPROM24FC04_ReadByte(uint16_t mem_addr, uint8_t *data);
bool EEPROM24FC04_WriteData(uint16_t mem_addr, const uint8_t *data, uint16_t len);
bool EEPROM24FC04_ReadData(uint16_t mem_addr, uint8_t *data, uint16_t len);
void EEPROM24FC04_LoadDefaultSettings(EEPROM24FC04_Settings *settings);
bool EEPROM24FC04_SaveSettings(const EEPROM24FC04_Settings *settings);
bool EEPROM24FC04_LoadSettings(EEPROM24FC04_Settings *settings);
bool EEPROM24FC04_SaveSetpoints(float tank_temp_sp_c, float superheat_dt_sp_c);
bool EEPROM24FC04_LoadSetpoints(float *tank_temp_sp_c, float *superheat_dt_sp_c);
bool EEPROM24FC04_RestoreDefaults(void);

#endif
