#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "app_config.h"
#include "eeprom_24fc04.h"

#ifndef FCY
#define FCY APP_FCY_HZ
#endif
#include <libpic30.h>

#define EEPROM24FC04_I2C_FSCL_HZ             100000UL
#define EEPROM24FC04_I2C_TIMEOUT_COUNTS      10000UL
#define EEPROM24FC04_ACK_POLL_ATTEMPTS       20U
#define EEPROM24FC04_WRITE_CYCLE_DELAY_MS    1U
#define EEPROM24FC04_CONTROL_BASE_7BIT       0x50U
#define EEPROM24FC04_MAGIC                   0x4553U
#define EEPROM24FC04_VERSION                 4U

#define SETPOINTS_MAGIC_L_OFFSET             0U
#define SETPOINTS_MAGIC_H_OFFSET             1U
#define SETPOINTS_VERSION_OFFSET             2U
#define SETPOINTS_RESERVED_OFFSET            3U
#define SETPOINTS_TANK_OFFSET                4U
#define SETPOINTS_HEATER_TANK_OFFSET         8U
#define SETPOINTS_SUPERHEAT_OFFSET           12U
#define SETPOINTS_TANK_HYST_OFFSET           16U
#define SETPOINTS_HEATER_HYST_OFFSET         20U
#define SETPOINTS_EEV_STARTUP_OFFSET         24U
#define SETPOINTS_EEV_MAX_OFFSET             26U
#define SETPOINTS_COMP_ASC_OFFSET            28U
#define SETPOINTS_LP_BYPASS_OFFSET           30U
#define SETPOINTS_PUMP_PRE_OFFSET            32U
#define SETPOINTS_PUMP_POST_OFFSET           34U
#define SETPOINTS_HEATER_OFFSET              36U
#define SETPOINTS_CRC_OFFSET                 37U

volatile uint16_t g_eeprom_present = 0;
volatile uint16_t g_eeprom_error = EEPROM24FC04_ERROR_NONE;
volatile uint16_t g_eeprom_last_addr = 0;
volatile uint16_t g_eeprom_last_data = 0;
volatile uint16_t g_eeprom_write_count = 0;
volatile uint16_t g_eeprom_read_count = 0;
volatile float g_tank_temp_setpoint_c = EEPROM24FC04_TANK_TEMP_DEFAULT_C;
volatile float g_heater_temp_setpoint_c = EEPROM24FC04_HEATER_TEMP_DEFAULT_C;
volatile float g_superheat_delta_t_setpoint_c = EEPROM24FC04_SUPERHEAT_DT_DEFAULT_C;
volatile float g_tank_hysteresis_c = EEPROM24FC04_TANK_HYST_DEFAULT_C;
volatile float g_heater_hysteresis_c = EEPROM24FC04_HEATER_HYST_DEFAULT_C;
volatile uint16_t g_eev_startup_steps_setting = EEPROM24FC04_EEV_STARTUP_DEFAULT;
volatile uint16_t g_eev_max_steps_setting = EEPROM24FC04_EEV_MAX_DEFAULT;
volatile uint16_t g_compressor_anti_short_cycle_sec = EEPROM24FC04_COMP_ASC_DEFAULT_SEC;
volatile uint16_t g_lp_startup_bypass_sec = EEPROM24FC04_LP_BYPASS_DEFAULT_SEC;
volatile uint16_t g_pump_pre_run_sec = EEPROM24FC04_PUMP_PRE_DEFAULT_SEC;
volatile uint16_t g_pump_post_run_sec = EEPROM24FC04_PUMP_POST_DEFAULT_SEC;
volatile uint16_t g_heater_enabled_setting = 0;

static void EEPROM24FC04_EnsureI2C1Enabled(void);
static uint8_t EEPROM24FC04_ControlAddress(uint16_t mem_addr);
static bool EEPROM24FC04_AddressIsValid(uint16_t mem_addr, uint16_t len);
static bool EEPROM24FC04_Start(void);
static bool EEPROM24FC04_Restart(void);
static bool EEPROM24FC04_Stop(void);
static bool EEPROM24FC04_WriteI2CByte(uint8_t data);
static bool EEPROM24FC04_ReadI2CByte(uint8_t *data, bool ack);
static bool EEPROM24FC04_ProbeControl(uint8_t control_address);
static bool EEPROM24FC04_WaitWriteComplete(uint16_t mem_addr);
static bool EEPROM24FC04_WritePage(uint16_t mem_addr, const uint8_t *data, uint8_t len);
static uint8_t EEPROM24FC04_CRC8(const uint8_t *data, uint8_t len);
static void EEPROM24FC04_StoreU16(uint8_t *record, uint8_t offset, uint16_t value);
static uint16_t EEPROM24FC04_LoadU16(const uint8_t *record, uint8_t offset);
static void EEPROM24FC04_ApplySettingsToGlobals(const EEPROM24FC04_Settings *settings);
static void EEPROM24FC04_BuildSettingsRecord(const EEPROM24FC04_Settings *settings,
                                             uint8_t *record);
static bool EEPROM24FC04_ParseSettingsRecord(const uint8_t *record,
                                             EEPROM24FC04_Settings *settings);

void EEPROM24FC04_Init(void)
{
    g_eeprom_error = EEPROM24FC04_ERROR_NONE;
    g_eeprom_present = 0;
    g_eeprom_last_addr = 0;
    g_eeprom_last_data = 0;

    EEPROM24FC04_EnsureI2C1Enabled();

    if ((EEPROM24FC04_ProbeControl(EEPROM24FC04_CONTROL_BASE_7BIT) != false) ||
        (EEPROM24FC04_ProbeControl((uint8_t)(EEPROM24FC04_CONTROL_BASE_7BIT | 0x01U)) != false))
    {
        g_eeprom_present = 1;
        g_eeprom_error = EEPROM24FC04_ERROR_NONE;
    }
    else
    {
        g_eeprom_error = EEPROM24FC04_ERROR_NO_ACK;
    }
}

bool EEPROM24FC04_IsPresent(void)
{
    EEPROM24FC04_Init();

    return (g_eeprom_present != 0U);
}

bool EEPROM24FC04_WriteByte(uint16_t mem_addr, uint8_t data)
{
    return EEPROM24FC04_WriteData(mem_addr, &data, 1U);
}

bool EEPROM24FC04_ReadByte(uint16_t mem_addr, uint8_t *data)
{
    return EEPROM24FC04_ReadData(mem_addr, data, 1U);
}

bool EEPROM24FC04_WriteData(uint16_t mem_addr, const uint8_t *data, uint16_t len)
{
    uint16_t offset = 0;

    if ((data == 0) || (EEPROM24FC04_AddressIsValid(mem_addr, len) == false))
    {
        g_eeprom_error = EEPROM24FC04_ERROR_INVALID_ADDRESS;
        return false;
    }

    if (len == 0U)
    {
        return true;
    }

    if (g_eeprom_present == 0U)
    {
        if (EEPROM24FC04_IsPresent() == false)
        {
            return false;
        }
    }

    while (offset < len)
    {
        uint16_t current_addr = (uint16_t)(mem_addr + offset);
        uint8_t page_room = (uint8_t)(EEPROM24FC04_PAGE_SIZE_BYTES -
                                      (current_addr % EEPROM24FC04_PAGE_SIZE_BYTES));
        uint16_t remaining = (uint16_t)(len - offset);
        uint8_t chunk = (remaining > page_room) ? page_room : (uint8_t)remaining;

        if (chunk > EEPROM24FC04_PAGE_SIZE_BYTES)
        {
            g_eeprom_error = EEPROM24FC04_ERROR_PAGE_BOUNDARY;
            return false;
        }

        if (EEPROM24FC04_WritePage(current_addr, &data[offset], chunk) == false)
        {
            return false;
        }

        offset = (uint16_t)(offset + chunk);
    }

    return true;
}

bool EEPROM24FC04_ReadData(uint16_t mem_addr, uint8_t *data, uint16_t len)
{
    uint16_t index;

    if ((data == 0) || (EEPROM24FC04_AddressIsValid(mem_addr, len) == false))
    {
        g_eeprom_error = EEPROM24FC04_ERROR_INVALID_ADDRESS;
        return false;
    }

    if (len == 0U)
    {
        return true;
    }

    if (g_eeprom_present == 0U)
    {
        if (EEPROM24FC04_IsPresent() == false)
        {
            return false;
        }
    }

    if (EEPROM24FC04_Start() == false)
    {
        g_eeprom_error = EEPROM24FC04_ERROR_READ_TIMEOUT;
        return false;
    }

    if ((EEPROM24FC04_WriteI2CByte((uint8_t)(EEPROM24FC04_ControlAddress(mem_addr) << 1)) == false) ||
        (EEPROM24FC04_WriteI2CByte((uint8_t)(mem_addr & 0xFFU)) == false) ||
        (EEPROM24FC04_Restart() == false) ||
        (EEPROM24FC04_WriteI2CByte((uint8_t)((EEPROM24FC04_ControlAddress(mem_addr) << 1) | 0x01U)) == false))
    {
        (void)EEPROM24FC04_Stop();
        g_eeprom_error = EEPROM24FC04_ERROR_NO_ACK;
        return false;
    }

    for (index = 0; index < len; index++)
    {
        bool ack = (index < (uint16_t)(len - 1U));

        if (EEPROM24FC04_ReadI2CByte(&data[index], ack) == false)
        {
            (void)EEPROM24FC04_Stop();
            g_eeprom_error = EEPROM24FC04_ERROR_READ_TIMEOUT;
            return false;
        }

        g_eeprom_read_count++;
        g_eeprom_last_addr = (uint16_t)(mem_addr + index);
        g_eeprom_last_data = data[index];
    }

    (void)EEPROM24FC04_Stop();
    g_eeprom_error = EEPROM24FC04_ERROR_NONE;

    return true;
}

void EEPROM24FC04_LoadDefaultSettings(EEPROM24FC04_Settings *settings)
{
    if (settings == 0)
    {
        return;
    }

    settings->tank_temp_setpoint_c = EEPROM24FC04_TANK_TEMP_DEFAULT_C;
    settings->heater_temp_setpoint_c = EEPROM24FC04_HEATER_TEMP_DEFAULT_C;
    settings->superheat_delta_t_setpoint_c = EEPROM24FC04_SUPERHEAT_DT_DEFAULT_C;
    settings->tank_hysteresis_c = EEPROM24FC04_TANK_HYST_DEFAULT_C;
    settings->heater_hysteresis_c = EEPROM24FC04_HEATER_HYST_DEFAULT_C;
    settings->eev_startup_steps = EEPROM24FC04_EEV_STARTUP_DEFAULT;
    settings->eev_max_steps = EEPROM24FC04_EEV_MAX_DEFAULT;
    settings->compressor_anti_short_cycle_sec = EEPROM24FC04_COMP_ASC_DEFAULT_SEC;
    settings->lp_startup_bypass_sec = EEPROM24FC04_LP_BYPASS_DEFAULT_SEC;
    settings->pump_pre_run_sec = EEPROM24FC04_PUMP_PRE_DEFAULT_SEC;
    settings->pump_post_run_sec = EEPROM24FC04_PUMP_POST_DEFAULT_SEC;
    settings->heater_enabled = EEPROM24FC04_HEATER_DEFAULT_ENABLED;
}

bool EEPROM24FC04_SaveSettings(const EEPROM24FC04_Settings *settings)
{
    uint8_t record[EEPROM24FC04_SETPOINTS_LENGTH];
    uint8_t existing[EEPROM24FC04_SETPOINTS_LENGTH];

    if (settings == 0)
    {
        g_eeprom_error = EEPROM24FC04_ERROR_INVALID_ADDRESS;
        return false;
    }

    EEPROM24FC04_BuildSettingsRecord(settings, record);

    if (g_eeprom_present == 0U)
    {
        if (EEPROM24FC04_IsPresent() == false)
        {
            return false;
        }
    }

    if (EEPROM24FC04_ReadData(EEPROM24FC04_ADDR_SETPOINTS, existing, EEPROM24FC04_SETPOINTS_LENGTH) != false)
    {
        if (memcmp(existing, record, EEPROM24FC04_SETPOINTS_LENGTH) == 0)
        {
            EEPROM24FC04_ApplySettingsToGlobals(settings);
            return true;
        }
    }

    if (EEPROM24FC04_WriteData(EEPROM24FC04_ADDR_SETPOINTS, record, EEPROM24FC04_SETPOINTS_LENGTH) == false)
    {
        return false;
    }

    EEPROM24FC04_ApplySettingsToGlobals(settings);

    return true;
}

bool EEPROM24FC04_LoadSettings(EEPROM24FC04_Settings *settings)
{
    uint8_t record[EEPROM24FC04_SETPOINTS_LENGTH];
    bool valid = false;

    if (settings == 0)
    {
        g_eeprom_error = EEPROM24FC04_ERROR_INVALID_ADDRESS;
        return false;
    }

    EEPROM24FC04_LoadDefaultSettings(settings);

    if (g_eeprom_present == 0U)
    {
        (void)EEPROM24FC04_IsPresent();
    }

    if (g_eeprom_present != 0U)
    {
        if (EEPROM24FC04_ReadData(EEPROM24FC04_ADDR_SETPOINTS, record, EEPROM24FC04_SETPOINTS_LENGTH) != false)
        {
            valid = EEPROM24FC04_ParseSettingsRecord(record, settings);
        }

        if (valid == false)
        {
            EEPROM24FC04_LoadDefaultSettings(settings);
            g_eeprom_error = EEPROM24FC04_ERROR_CHECKSUM_INVALID;
            (void)EEPROM24FC04_SaveSettings(settings);
        }
    }
    else
    {
        g_eeprom_error = EEPROM24FC04_ERROR_NO_ACK;
    }

    EEPROM24FC04_ApplySettingsToGlobals(settings);

    return valid;
}

bool EEPROM24FC04_SaveSetpoints(float tank_temp_sp_c, float superheat_dt_sp_c)
{
    EEPROM24FC04_Settings settings;

    EEPROM24FC04_LoadDefaultSettings(&settings);
    settings.tank_temp_setpoint_c = tank_temp_sp_c;
    settings.superheat_delta_t_setpoint_c = superheat_dt_sp_c;

    return EEPROM24FC04_SaveSettings(&settings);
}

bool EEPROM24FC04_LoadSetpoints(float *tank_temp_sp_c, float *superheat_dt_sp_c)
{
    EEPROM24FC04_Settings settings;
    bool valid;

    if ((tank_temp_sp_c == 0) || (superheat_dt_sp_c == 0))
    {
        g_eeprom_error = EEPROM24FC04_ERROR_INVALID_ADDRESS;
        return false;
    }

    valid = EEPROM24FC04_LoadSettings(&settings);

    *tank_temp_sp_c = settings.tank_temp_setpoint_c;
    *superheat_dt_sp_c = settings.superheat_delta_t_setpoint_c;

    return valid;
}

bool EEPROM24FC04_RestoreDefaults(void)
{
    EEPROM24FC04_Settings settings;

    EEPROM24FC04_LoadDefaultSettings(&settings);

    return EEPROM24FC04_SaveSettings(&settings);
}

static void EEPROM24FC04_EnsureI2C1Enabled(void)
{
    if (I2C1CONLbits.I2CEN == 0U)
    {
        PMD1bits.I2C1MD = 0;
        I2C1CONLbits.DISSLW = 1;
        I2C1BRG = (uint16_t)((APP_FCY_HZ / (2UL * EEPROM24FC04_I2C_FSCL_HZ)) - 1UL);
        I2C1CONLbits.I2CEN = 1;
    }
}

static uint8_t EEPROM24FC04_ControlAddress(uint16_t mem_addr)
{
    return (uint8_t)(EEPROM24FC04_CONTROL_BASE_7BIT | ((mem_addr >> 8) & 0x01U));
}

static bool EEPROM24FC04_AddressIsValid(uint16_t mem_addr, uint16_t len)
{
    if (mem_addr >= EEPROM24FC04_SIZE_BYTES)
    {
        return false;
    }

    if (len > (uint16_t)(EEPROM24FC04_SIZE_BYTES - mem_addr))
    {
        return false;
    }

    return true;
}

static bool EEPROM24FC04_Start(void)
{
    uint16_t timeout = EEPROM24FC04_I2C_TIMEOUT_COUNTS;

    I2C1CONLbits.SEN = 1;

    while ((I2C1CONLbits.SEN != 0U) && (timeout > 0U))
    {
        timeout--;
    }

    return (timeout != 0U);
}

static bool EEPROM24FC04_Restart(void)
{
    uint16_t timeout = EEPROM24FC04_I2C_TIMEOUT_COUNTS;

    I2C1CONLbits.RSEN = 1;

    while ((I2C1CONLbits.RSEN != 0U) && (timeout > 0U))
    {
        timeout--;
    }

    return (timeout != 0U);
}

static bool EEPROM24FC04_Stop(void)
{
    uint16_t timeout = EEPROM24FC04_I2C_TIMEOUT_COUNTS;

    I2C1CONLbits.PEN = 1;

    while ((I2C1CONLbits.PEN != 0U) && (timeout > 0U))
    {
        timeout--;
    }

    return (timeout != 0U);
}

static bool EEPROM24FC04_WriteI2CByte(uint8_t data)
{
    uint16_t timeout = EEPROM24FC04_I2C_TIMEOUT_COUNTS;

    I2C1TRN = data;

    while ((I2C1STATbits.TRSTAT != 0U) && (timeout > 0U))
    {
        timeout--;
    }

    if (timeout == 0U)
    {
        return false;
    }

    return (I2C1STATbits.ACKSTAT == 0U);
}

static bool EEPROM24FC04_ReadI2CByte(uint8_t *data, bool ack)
{
    uint16_t timeout = EEPROM24FC04_I2C_TIMEOUT_COUNTS;

    I2C1CONLbits.RCEN = 1;

    while ((I2C1STATbits.RBF == 0U) && (timeout > 0U))
    {
        timeout--;
    }

    if (timeout == 0U)
    {
        return false;
    }

    *data = (uint8_t)I2C1RCV;

    timeout = EEPROM24FC04_I2C_TIMEOUT_COUNTS;
    I2C1CONLbits.ACKDT = (ack != false) ? 0U : 1U;
    I2C1CONLbits.ACKEN = 1;

    while ((I2C1CONLbits.ACKEN != 0U) && (timeout > 0U))
    {
        timeout--;
    }

    return (timeout != 0U);
}

static bool EEPROM24FC04_ProbeControl(uint8_t control_address)
{
    bool ok;

    ok = EEPROM24FC04_Start();

    if (ok != false)
    {
        ok = EEPROM24FC04_WriteI2CByte((uint8_t)(control_address << 1));
    }

    (void)EEPROM24FC04_Stop();

    return ok;
}

static bool EEPROM24FC04_WaitWriteComplete(uint16_t mem_addr)
{
    uint8_t attempt;
    uint8_t control_address = EEPROM24FC04_ControlAddress(mem_addr);

    for (attempt = 0; attempt < EEPROM24FC04_ACK_POLL_ATTEMPTS; attempt++)
    {
        if (EEPROM24FC04_ProbeControl(control_address) != false)
        {
            return true;
        }

        __delay_ms(EEPROM24FC04_WRITE_CYCLE_DELAY_MS);
    }

    g_eeprom_error = EEPROM24FC04_ERROR_WRITE_TIMEOUT;
    return false;
}

static bool EEPROM24FC04_WritePage(uint16_t mem_addr, const uint8_t *data, uint8_t len)
{
    uint8_t index;

    if (((mem_addr % EEPROM24FC04_PAGE_SIZE_BYTES) + len) > EEPROM24FC04_PAGE_SIZE_BYTES)
    {
        g_eeprom_error = EEPROM24FC04_ERROR_PAGE_BOUNDARY;
        return false;
    }

    if (EEPROM24FC04_Start() == false)
    {
        g_eeprom_error = EEPROM24FC04_ERROR_WRITE_TIMEOUT;
        return false;
    }

    if ((EEPROM24FC04_WriteI2CByte((uint8_t)(EEPROM24FC04_ControlAddress(mem_addr) << 1)) == false) ||
        (EEPROM24FC04_WriteI2CByte((uint8_t)(mem_addr & 0xFFU)) == false))
    {
        (void)EEPROM24FC04_Stop();
        g_eeprom_error = EEPROM24FC04_ERROR_NO_ACK;
        return false;
    }

    for (index = 0; index < len; index++)
    {
        if (EEPROM24FC04_WriteI2CByte(data[index]) == false)
        {
            (void)EEPROM24FC04_Stop();
            g_eeprom_error = EEPROM24FC04_ERROR_NO_ACK;
            return false;
        }

        g_eeprom_write_count++;
        g_eeprom_last_addr = (uint16_t)(mem_addr + index);
        g_eeprom_last_data = data[index];
    }

    (void)EEPROM24FC04_Stop();

    if (EEPROM24FC04_WaitWriteComplete(mem_addr) == false)
    {
        return false;
    }

    g_eeprom_error = EEPROM24FC04_ERROR_NONE;
    return true;
}

static uint8_t EEPROM24FC04_CRC8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    uint8_t index;
    uint8_t bit;

    for (index = 0; index < len; index++)
    {
        crc ^= data[index];

        for (bit = 0; bit < 8U; bit++)
        {
            if ((crc & 0x80U) != 0U)
            {
                crc = (uint8_t)((crc << 1) ^ 0x07U);
            }
            else
            {
                crc = (uint8_t)(crc << 1);
            }
        }
    }

    return crc;
}

static void EEPROM24FC04_StoreU16(uint8_t *record, uint8_t offset, uint16_t value)
{
    record[offset] = (uint8_t)(value & 0xFFU);
    record[(uint8_t)(offset + 1U)] = (uint8_t)(value >> 8);
}

static uint16_t EEPROM24FC04_LoadU16(const uint8_t *record, uint8_t offset)
{
    return (uint16_t)record[offset] |
           ((uint16_t)record[(uint8_t)(offset + 1U)] << 8);
}

static void EEPROM24FC04_ApplySettingsToGlobals(const EEPROM24FC04_Settings *settings)
{
    g_tank_temp_setpoint_c = settings->tank_temp_setpoint_c;
    g_heater_temp_setpoint_c = settings->heater_temp_setpoint_c;
    g_superheat_delta_t_setpoint_c = settings->superheat_delta_t_setpoint_c;
    g_tank_hysteresis_c = settings->tank_hysteresis_c;
    g_heater_hysteresis_c = settings->heater_hysteresis_c;
    g_eev_startup_steps_setting = settings->eev_startup_steps;
    g_eev_max_steps_setting = settings->eev_max_steps;
    g_compressor_anti_short_cycle_sec = settings->compressor_anti_short_cycle_sec;
    g_lp_startup_bypass_sec = settings->lp_startup_bypass_sec;
    g_pump_pre_run_sec = settings->pump_pre_run_sec;
    g_pump_post_run_sec = settings->pump_post_run_sec;
    g_heater_enabled_setting = (settings->heater_enabled != false) ? 1U : 0U;
}

static void EEPROM24FC04_BuildSettingsRecord(const EEPROM24FC04_Settings *settings,
                                             uint8_t *record)
{
    record[SETPOINTS_MAGIC_L_OFFSET] = (uint8_t)(EEPROM24FC04_MAGIC & 0xFFU);
    record[SETPOINTS_MAGIC_H_OFFSET] = (uint8_t)(EEPROM24FC04_MAGIC >> 8);
    record[SETPOINTS_VERSION_OFFSET] = EEPROM24FC04_VERSION;
    record[SETPOINTS_RESERVED_OFFSET] = 0U;
    (void)memcpy(&record[SETPOINTS_TANK_OFFSET], &settings->tank_temp_setpoint_c, sizeof(float));
    (void)memcpy(&record[SETPOINTS_HEATER_TANK_OFFSET], &settings->heater_temp_setpoint_c, sizeof(float));
    (void)memcpy(&record[SETPOINTS_SUPERHEAT_OFFSET], &settings->superheat_delta_t_setpoint_c, sizeof(float));
    (void)memcpy(&record[SETPOINTS_TANK_HYST_OFFSET], &settings->tank_hysteresis_c, sizeof(float));
    (void)memcpy(&record[SETPOINTS_HEATER_HYST_OFFSET], &settings->heater_hysteresis_c, sizeof(float));
    EEPROM24FC04_StoreU16(record, SETPOINTS_EEV_STARTUP_OFFSET, settings->eev_startup_steps);
    EEPROM24FC04_StoreU16(record, SETPOINTS_EEV_MAX_OFFSET, settings->eev_max_steps);
    EEPROM24FC04_StoreU16(record, SETPOINTS_COMP_ASC_OFFSET, settings->compressor_anti_short_cycle_sec);
    EEPROM24FC04_StoreU16(record, SETPOINTS_LP_BYPASS_OFFSET, settings->lp_startup_bypass_sec);
    EEPROM24FC04_StoreU16(record, SETPOINTS_PUMP_PRE_OFFSET, settings->pump_pre_run_sec);
    EEPROM24FC04_StoreU16(record, SETPOINTS_PUMP_POST_OFFSET, settings->pump_post_run_sec);
    record[SETPOINTS_HEATER_OFFSET] = (settings->heater_enabled != false) ? 1U : 0U;
    record[SETPOINTS_CRC_OFFSET] = EEPROM24FC04_CRC8(record, SETPOINTS_CRC_OFFSET);
}

static bool EEPROM24FC04_ParseSettingsRecord(const uint8_t *record,
                                             EEPROM24FC04_Settings *settings)
{
    uint16_t magic;
    uint8_t crc;

    magic = (uint16_t)record[SETPOINTS_MAGIC_L_OFFSET] |
            ((uint16_t)record[SETPOINTS_MAGIC_H_OFFSET] << 8);

    if ((magic != EEPROM24FC04_MAGIC) ||
        (record[SETPOINTS_VERSION_OFFSET] != EEPROM24FC04_VERSION))
    {
        g_eeprom_error = EEPROM24FC04_ERROR_CHECKSUM_INVALID;
        return false;
    }

    crc = EEPROM24FC04_CRC8(record, SETPOINTS_CRC_OFFSET);

    if (crc != record[SETPOINTS_CRC_OFFSET])
    {
        g_eeprom_error = EEPROM24FC04_ERROR_CHECKSUM_INVALID;
        return false;
    }

    (void)memcpy(&settings->tank_temp_setpoint_c, &record[SETPOINTS_TANK_OFFSET], sizeof(float));
    (void)memcpy(&settings->heater_temp_setpoint_c, &record[SETPOINTS_HEATER_TANK_OFFSET], sizeof(float));
    (void)memcpy(&settings->superheat_delta_t_setpoint_c, &record[SETPOINTS_SUPERHEAT_OFFSET], sizeof(float));
    (void)memcpy(&settings->tank_hysteresis_c, &record[SETPOINTS_TANK_HYST_OFFSET], sizeof(float));
    (void)memcpy(&settings->heater_hysteresis_c, &record[SETPOINTS_HEATER_HYST_OFFSET], sizeof(float));
    settings->eev_startup_steps = EEPROM24FC04_LoadU16(record, SETPOINTS_EEV_STARTUP_OFFSET);
    settings->eev_max_steps = EEPROM24FC04_LoadU16(record, SETPOINTS_EEV_MAX_OFFSET);
    settings->compressor_anti_short_cycle_sec = EEPROM24FC04_LoadU16(record, SETPOINTS_COMP_ASC_OFFSET);
    settings->lp_startup_bypass_sec = EEPROM24FC04_LoadU16(record, SETPOINTS_LP_BYPASS_OFFSET);
    settings->pump_pre_run_sec = EEPROM24FC04_LoadU16(record, SETPOINTS_PUMP_PRE_OFFSET);
    settings->pump_post_run_sec = EEPROM24FC04_LoadU16(record, SETPOINTS_PUMP_POST_OFFSET);
    settings->heater_enabled = (record[SETPOINTS_HEATER_OFFSET] != 0U);
    g_eeprom_error = EEPROM24FC04_ERROR_NONE;

    return true;
}
