#include <xc.h>
#include <stdint.h>
#include "app_config.h"
#include "LCD_I2C.h"
#include "eeprom_24fc04.h"
#include "ntc_loops.h"
#include "faults/fault_inputs.h"
#include "heatpump_control.h"
#include "eev.h"

#ifndef FCY
#define FCY APP_FCY_HZ
#endif
#include <libpic30.h>

/*
    I2C 1602 LCD using the common PCF8574 backpack.

    Assumed backpack bit mapping:
    P0 = RS, P1 = RW, P2 = E, P3 = Backlight, P4..P7 = LCD D4..D7.

    I2C1 pins on dsPIC33CK256MP505:
    RB9 = SDA1, RB8 = SCL1. I2C1 is selected between the dedicated SDA1/SCL1
    and ASDA1/ASCL1 pins by the ALTI2C1 config bit, not by normal RPOR/RPINR
    PPS registers. main.c sets ALTI2C1 = OFF for the SDA1/SCL1 pair.
*/

#define LCD_I2C_FSCL_HZ             100000UL
#define LCD_I2C_TIMEOUT_COUNTS      10000UL
#define LCD_I2C_PAGE_HOLD_UPDATES   6U
#define LCD_I2C_PAGE_COUNT          4U

#define LCD_I2C_RS                  0x01U
#define LCD_I2C_RW                  0x02U
#define LCD_I2C_ENABLE              0x04U
#define LCD_I2C_BACKLIGHT           0x08U

#define LCD_CMD_CLEAR               0x01U
#define LCD_CMD_HOME                0x02U
#define LCD_CMD_ENTRY_MODE          0x06U
#define LCD_CMD_DISPLAY_ON          0x0CU
#define LCD_CMD_FUNCTION_SET_4BIT   0x28U
#define LCD_CMD_SET_DDRAM           0x80U

#define LCD_ROW0_ADDRESS            0x00U
#define LCD_ROW1_ADDRESS            0x40U

#define LCD_ERROR_NONE              0U
#define LCD_ERROR_START_TIMEOUT     1U
#define LCD_ERROR_STOP_TIMEOUT      2U
#define LCD_ERROR_WRITE_TIMEOUT     3U
#define LCD_ERROR_ACK               4U
#define LCD_ERROR_NOT_FOUND         5U

#define LCD_SENSOR_FAULT_SHWTS_MASK     0x0001U
#define LCD_SENSOR_FAULT_EVPTS_MASK     0x0002U
#define LCD_SENSOR_FAULT_RHWTS_MASK     0x0004U
#define LCD_SENSOR_FAULT_TOPDHWTS_MASK  0x0008U
#define LCD_SENSOR_FAULT_BOTDHWTS_MASK  0x0010U
#define LCD_SENSOR_FAULT_OST_MASK       0x0020U

volatile uint16_t g_lcd_i2c_state = 0;
volatile uint16_t g_lcd_i2c_error = 0;
volatile uint16_t g_lcd_i2c_updates = 0;
volatile uint16_t g_lcd_i2c_address = 0;
volatile uint16_t g_lcd_i2c_detected = 0;

extern volatile float g_shwts_temperature_c;

static uint8_t s_lcd_address = LCD_I2C_DEFAULT_ADDRESS;
static uint8_t s_lcd_backlight = LCD_I2C_BACKLIGHT;
static uint8_t s_lcd_page = 0;
static uint8_t s_lcd_page_hold = 0;
static uint8_t s_lcd_ready = 0;

static void LCD_I2C_PinInit(void);
static uint8_t LCD_I2C_Start(void);
static uint8_t LCD_I2C_Stop(void);
static uint8_t LCD_I2C_WriteByte(uint8_t data);
static uint8_t LCD_I2C_ProbeAddress(uint8_t address);
static uint8_t LCD_I2C_WriteExpander(uint8_t data);
static void LCD_I2C_PulseEnable(uint8_t data);
static void LCD_I2C_Write4Bits(uint8_t nibble, uint8_t control);
static void LCD_I2C_Send(uint8_t value, uint8_t control);
static void LCD_I2C_Command(uint8_t command);
static void LCD_I2C_WriteChar(char value);
static void LCD_I2C_PrintFixedWidth(const char *text, uint8_t width);
static void LCD_I2C_PrintTempValue(float temperature_c);
static void LCD_I2C_PrintSetpointValue(float setpoint_c);
static void LCD_I2C_PrintPercentValue(uint8_t percent);
static void LCD_I2C_PrintSpaces(uint8_t count);
static uint8_t LCD_I2C_ShowFaultIfActive(void);
static uint8_t LCD_I2C_ShowSensorFaultIfActive(void);

void LCD_I2C_Init(uint8_t address)
{
    uint16_t brg;

    g_lcd_i2c_state = 1;
    g_lcd_i2c_error = LCD_ERROR_NONE;

    s_lcd_ready = 0;
    s_lcd_page = 0;
    s_lcd_page_hold = 0;
    g_lcd_i2c_address = 0;
    g_lcd_i2c_detected = 0;

    LCD_I2C_PinInit();

    PMD1bits.I2C1MD = 0;

    I2C1CONLbits.I2CEN = 0;
    I2C1CONLbits.DISSLW = 1;
    I2C1STATbits.I2COV = 0;
    I2C1STATbits.IWCOL = 0;

    brg = (uint16_t)((APP_FCY_HZ / (2UL * LCD_I2C_FSCL_HZ)) - 1UL);
    I2C1BRG = brg;

    I2C1CONLbits.I2CEN = 1;

    __delay_ms(10);

    if (address != 0U)
    {
        if (LCD_I2C_ProbeAddress(address) != 0U)
        {
            s_lcd_address = address;
        }
        else
        {
            g_lcd_i2c_error = LCD_ERROR_NOT_FOUND;
            g_lcd_i2c_state = 0xE005U;
            return;
        }
    }
    else if (LCD_I2C_ProbeAddress(LCD_I2C_DEFAULT_ADDRESS) != 0U)
    {
        s_lcd_address = LCD_I2C_DEFAULT_ADDRESS;
    }
    else if (LCD_I2C_ProbeAddress(LCD_I2C_ALT_ADDRESS) != 0U)
    {
        s_lcd_address = LCD_I2C_ALT_ADDRESS;
    }
    else
    {
        g_lcd_i2c_error = LCD_ERROR_NOT_FOUND;
        g_lcd_i2c_state = 0xE005U;
        return;
    }

    s_lcd_ready = 1;
    g_lcd_i2c_address = s_lcd_address;
    g_lcd_i2c_detected = 1;

    __delay_ms(40);

    /*
        HD44780 power-up sequence for 4-bit mode through an expander.
    */
    LCD_I2C_Write4Bits(0x30U, 0U);
    __delay_ms(5);
    LCD_I2C_Write4Bits(0x30U, 0U);
    __delay_us(150);
    LCD_I2C_Write4Bits(0x30U, 0U);
    __delay_us(150);
    LCD_I2C_Write4Bits(0x20U, 0U);

    LCD_I2C_Command(LCD_CMD_FUNCTION_SET_4BIT);
    LCD_I2C_Command(LCD_CMD_DISPLAY_ON);
    LCD_I2C_Command(LCD_CMD_CLEAR);
    __delay_ms(2);
    LCD_I2C_Command(LCD_CMD_ENTRY_MODE);

    g_lcd_i2c_state = 2;
}

uint8_t LCD_I2C_IsReady(void)
{
    return s_lcd_ready;
}

void LCD_I2C_Clear(void)
{
    if (s_lcd_ready == 0U)
    {
        return;
    }

    LCD_I2C_Command(LCD_CMD_CLEAR);
    __delay_ms(2);
}

void LCD_I2C_Home(void)
{
    if (s_lcd_ready == 0U)
    {
        return;
    }

    LCD_I2C_Command(LCD_CMD_HOME);
    __delay_ms(2);
}

void LCD_I2C_SetCursor(uint8_t row, uint8_t column)
{
    uint8_t row_address;

    if (s_lcd_ready == 0U)
    {
        return;
    }

    if (row >= LCD_I2C_ROWS)
    {
        row = LCD_I2C_ROWS - 1U;
    }

    if (column >= LCD_I2C_COLUMNS)
    {
        column = LCD_I2C_COLUMNS - 1U;
    }

    row_address = (row == 0U) ? LCD_ROW0_ADDRESS : LCD_ROW1_ADDRESS;
    LCD_I2C_Command(LCD_CMD_SET_DDRAM | (row_address + column));
}

void LCD_I2C_Print(const char *text)
{
    if (s_lcd_ready == 0U)
    {
        return;
    }

    while ((text != 0) && (*text != '\0'))
    {
        LCD_I2C_WriteChar(*text);
        text++;
    }
}

void LCD_I2C_PrintTemperature(const char *label, float temperature_c)
{
    if (s_lcd_ready == 0U)
    {
        return;
    }

    LCD_I2C_PrintFixedWidth(label, 8U);
    LCD_I2C_PrintTempValue(temperature_c);
    LCD_I2C_WriteChar('C');
}

void LCD_I2C_ShowNTCTemperatures(void)
{
    if (s_lcd_ready == 0U)
    {
        return;
    }

    if (LCD_I2C_ShowFaultIfActive() != 0U)
    {
        g_lcd_i2c_updates++;
        return;
    }

    if (s_lcd_page == 0U)
    {
        LCD_I2C_SetCursor(0U, 0U);
        LCD_I2C_PrintTemperature("Suction", g_sucts_temperature_c);
        LCD_I2C_PrintSpaces(1U);
        LCD_I2C_SetCursor(1U, 0U);
        LCD_I2C_PrintTemperature("Evap", g_evpts_temperature_c);
        LCD_I2C_PrintSpaces(1U);
    }
    else if (s_lcd_page == 1U)
    {
        LCD_I2C_SetCursor(0U, 0U);
        LCD_I2C_PrintTemperature("DHW Top", g_topdhwts_temperature_c);
        LCD_I2C_PrintSpaces(1U);
        LCD_I2C_SetCursor(1U, 0U);
        LCD_I2C_PrintTemperature("DHW Bot", g_botdhwts_temperature_c);
        LCD_I2C_PrintSpaces(1U);
    }
    else if (s_lcd_page == 2U)
    {
        LCD_I2C_SetCursor(0U, 0U);
        LCD_I2C_PrintFixedWidth("EEV", 8U);
        LCD_I2C_PrintPercentValue(EEV_GetPositionPercent());
        LCD_I2C_WriteChar('%');
        LCD_I2C_PrintSpaces(4U);
        LCD_I2C_SetCursor(1U, 0U);
        LCD_I2C_PrintFixedWidth("DHW Set", 8U);
        LCD_I2C_PrintSetpointValue(g_tank_temp_setpoint_c);
        LCD_I2C_WriteChar('C');
        LCD_I2C_PrintSpaces(1U);
    }
    else
    {
        LCD_I2C_SetCursor(0U, 0U);
        LCD_I2C_PrintTemperature("SuperHt", g_hp_superheat_delta_t_c);
        LCD_I2C_PrintSpaces(1U);
        LCD_I2C_SetCursor(1U, 0U);
        LCD_I2C_PrintTemperature("SH Target", g_hp_superheat_target_c);
        LCD_I2C_PrintSpaces(1U);
    }

    s_lcd_page_hold++;

    if (s_lcd_page_hold >= LCD_I2C_PAGE_HOLD_UPDATES)
    {
        s_lcd_page_hold = 0;
        s_lcd_page++;

        if (s_lcd_page >= LCD_I2C_PAGE_COUNT)
        {
            s_lcd_page = 0U;
        }
    }

    g_lcd_i2c_updates++;
}


static void LCD_I2C_PinInit(void)
{
    /* RB9 = SDA1, RB8 = SCL1 with external 4.7k pull-up resistors. */
    ANSELBbits.ANSELB9 = 0;
    ANSELBbits.ANSELB8 = 0;

    LATBbits.LATB9 = 1;
    LATBbits.LATB8 = 1;

    TRISBbits.TRISB9 = 1;
    TRISBbits.TRISB8 = 1;

    ODCBbits.ODCB9 = 1;
    ODCBbits.ODCB8 = 1;
}

static uint8_t LCD_I2C_Start(void)
{
    uint16_t timeout = LCD_I2C_TIMEOUT_COUNTS;

    I2C1CONLbits.SEN = 1;

    while ((I2C1CONLbits.SEN != 0U) && (timeout > 0U))
    {
        timeout--;
    }

    if (timeout == 0U)
    {
        g_lcd_i2c_error = LCD_ERROR_START_TIMEOUT;
        return 0U;
    }

    return 1U;
}

static uint8_t LCD_I2C_Stop(void)
{
    uint16_t timeout = LCD_I2C_TIMEOUT_COUNTS;

    I2C1CONLbits.PEN = 1;

    while ((I2C1CONLbits.PEN != 0U) && (timeout > 0U))
    {
        timeout--;
    }

    if (timeout == 0U)
    {
        g_lcd_i2c_error = LCD_ERROR_STOP_TIMEOUT;
        return 0U;
    }

    return 1U;
}

static uint8_t LCD_I2C_WriteByte(uint8_t data)
{
    uint16_t timeout = LCD_I2C_TIMEOUT_COUNTS;

    I2C1TRN = data;

    while ((I2C1STATbits.TRSTAT != 0U) && (timeout > 0U))
    {
        timeout--;
    }

    if (timeout == 0U)
    {
        g_lcd_i2c_error = LCD_ERROR_WRITE_TIMEOUT;
        return 0U;
    }

    if (I2C1STATbits.ACKSTAT != 0U)
    {
        g_lcd_i2c_error = LCD_ERROR_ACK;
        return 0U;
    }

    return 1U;
}

static uint8_t LCD_I2C_ProbeAddress(uint8_t address)
{
    uint8_t ok;

    ok = LCD_I2C_Start();

    if (ok != 0U)
    {
        ok = LCD_I2C_WriteByte((uint8_t)(address << 1));
    }

    (void)LCD_I2C_Stop();

    return ok;
}

static uint8_t LCD_I2C_WriteExpander(uint8_t data)
{
    uint8_t ok;

    g_lcd_i2c_state = 3;

    ok = LCD_I2C_Start();

    if (ok != 0U)
    {
        ok = LCD_I2C_WriteByte((uint8_t)(s_lcd_address << 1));
    }

    if (ok != 0U)
    {
        ok = LCD_I2C_WriteByte((uint8_t)(data | s_lcd_backlight));
    }

    (void)LCD_I2C_Stop();

    if (ok != 0U)
    {
        g_lcd_i2c_error = LCD_ERROR_NONE;
    }

    return ok;
}

static void LCD_I2C_PulseEnable(uint8_t data)
{
    if (s_lcd_ready == 0U)
    {
        return;
    }

    (void)LCD_I2C_WriteExpander((uint8_t)(data | LCD_I2C_ENABLE));
    __delay_us(1);
    (void)LCD_I2C_WriteExpander((uint8_t)(data & (uint8_t)(~LCD_I2C_ENABLE)));
    __delay_us(50);
}

static void LCD_I2C_Write4Bits(uint8_t nibble, uint8_t control)
{
    uint8_t data = (uint8_t)((nibble & 0xF0U) | control);

    LCD_I2C_PulseEnable(data);
}

static void LCD_I2C_Send(uint8_t value, uint8_t control)
{
    LCD_I2C_Write4Bits((uint8_t)(value & 0xF0U), control);
    LCD_I2C_Write4Bits((uint8_t)((value << 4) & 0xF0U), control);
}

static void LCD_I2C_Command(uint8_t command)
{
    LCD_I2C_Send(command, 0U);
}

static void LCD_I2C_WriteChar(char value)
{
    LCD_I2C_Send((uint8_t)value, LCD_I2C_RS);
}

static void LCD_I2C_PrintFixedWidth(const char *text, uint8_t width)
{
    uint8_t count = 0;

    while ((text != 0) && (*text != '\0') && (count < width))
    {
        LCD_I2C_WriteChar(*text);
        text++;
        count++;
    }

    while (count < width)
    {
        LCD_I2C_WriteChar(' ');
        count++;
    }
}

static void LCD_I2C_PrintSpaces(uint8_t count)
{
    while (count > 0U)
    {
        LCD_I2C_WriteChar(' ');
        count--;
    }
}

static void LCD_I2C_PrintSetpointValue(float setpoint_c)
{
    LCD_I2C_PrintTempValue(setpoint_c);
}

static void LCD_I2C_PrintPercentValue(uint8_t percent)
{
    if (percent > 100U)
    {
        percent = 100U;
    }

    if (percent >= 100U)
    {
        LCD_I2C_WriteChar('1');
        LCD_I2C_WriteChar('0');
        LCD_I2C_WriteChar('0');
    }
    else if (percent >= 10U)
    {
        LCD_I2C_WriteChar(' ');
        LCD_I2C_WriteChar((char)('0' + (percent / 10U)));
        LCD_I2C_WriteChar((char)('0' + (percent % 10U)));
    }
    else
    {
        LCD_I2C_WriteChar(' ');
        LCD_I2C_WriteChar(' ');
        LCD_I2C_WriteChar((char)('0' + percent));
    }
}

static uint8_t LCD_I2C_ShowFaultIfActive(void)
{
    uint16_t faults;

    faults = (uint16_t)(g_hp_active_fault | g_hp_latched_fault);

    if ((faults & FAULT_INPUT_HP_MASK) != 0U)
    {
        LCD_I2C_SetCursor(0U, 0U);
        LCD_I2C_PrintFixedWidth("HP Fault", LCD_I2C_COLUMNS);
        LCD_I2C_SetCursor(1U, 0U);
        LCD_I2C_PrintSpaces(LCD_I2C_COLUMNS);
        return 1U;
    }

    if ((faults & FAULT_INPUT_LP_MASK) != 0U)
    {
        LCD_I2C_SetCursor(0U, 0U);
        LCD_I2C_PrintFixedWidth("LP Fault", LCD_I2C_COLUMNS);
        LCD_I2C_SetCursor(1U, 0U);
        LCD_I2C_PrintSpaces(LCD_I2C_COLUMNS);
        return 1U;
    }

    if ((faults & FAULT_INPUT_FLOW_MASK) != 0U)
    {
        LCD_I2C_SetCursor(0U, 0U);
        LCD_I2C_PrintFixedWidth("FS Fault", LCD_I2C_COLUMNS);
        LCD_I2C_SetCursor(1U, 0U);
        LCD_I2C_PrintSpaces(LCD_I2C_COLUMNS);
        return 1U;
    }

    if ((faults & HP_CONTROL_FAULT_HIGH_TEMP_MASK) != 0U)
    {
        LCD_I2C_SetCursor(0U, 0U);
        LCD_I2C_PrintFixedWidth("HIGH TEMP Fault", LCD_I2C_COLUMNS);
        LCD_I2C_SetCursor(1U, 0U);
        LCD_I2C_PrintSpaces(LCD_I2C_COLUMNS);
        return 1U;
    }

    if ((faults & HP_CONTROL_FAULT_FREEZE_MASK) != 0U)
    {
        LCD_I2C_SetCursor(0U, 0U);
        LCD_I2C_PrintFixedWidth("FREEZE Fault", LCD_I2C_COLUMNS);
        LCD_I2C_SetCursor(1U, 0U);
        LCD_I2C_PrintSpaces(LCD_I2C_COLUMNS);
        return 1U;
    }

    if ((faults & HP_CONTROL_FAULT_TANK_OVERTEMP_MASK) != 0U)
    {
        LCD_I2C_SetCursor(0U, 0U);
        LCD_I2C_PrintFixedWidth("TANK TEMP Fault", LCD_I2C_COLUMNS);
        LCD_I2C_SetCursor(1U, 0U);
        LCD_I2C_PrintSpaces(LCD_I2C_COLUMNS);
        return 1U;
    }

    if ((faults & HP_CONTROL_FAULT_EEV_MIN_OPEN_MASK) != 0U)
    {
        LCD_I2C_SetCursor(0U, 0U);
        LCD_I2C_PrintFixedWidth("EEV MIN Fault", LCD_I2C_COLUMNS);
        LCD_I2C_SetCursor(1U, 0U);
        LCD_I2C_PrintSpaces(LCD_I2C_COLUMNS);
        return 1U;
    }

    if (faults != 0U)
    {
        if (LCD_I2C_ShowSensorFaultIfActive() != 0U)
        {
            return 1U;
        }

        LCD_I2C_SetCursor(0U, 0U);
        LCD_I2C_PrintFixedWidth("SENSOR Fault", LCD_I2C_COLUMNS);
        LCD_I2C_SetCursor(1U, 0U);
        LCD_I2C_PrintSpaces(LCD_I2C_COLUMNS);
        return 1U;
    }

    return 0U;
}

static uint8_t LCD_I2C_ShowSensorFaultIfActive(void)
{
    const char *sensor_name = 0;

    /*
        Sensor fault priority follows the bit order used by main.c
        UpdateSensorFaults(). g_sensor_fault_inputs remains the raw NTC
        open/short debug bitfield.
    */
    if ((g_sensor_fault_inputs & LCD_SENSOR_FAULT_SHWTS_MASK) != 0U)
    {
        sensor_name = "SHWTS Fault";
    }
    else if ((g_sensor_fault_inputs & LCD_SENSOR_FAULT_EVPTS_MASK) != 0U)
    {
        sensor_name = "EvpTS Fault";
    }
    else if ((g_sensor_fault_inputs & LCD_SENSOR_FAULT_RHWTS_MASK) != 0U)
    {
        sensor_name = "RHWTS Fault";
    }
    else if ((g_sensor_fault_inputs & LCD_SENSOR_FAULT_TOPDHWTS_MASK) != 0U)
    {
        sensor_name = "TopDHWTS Fault";
    }
    else if ((g_sensor_fault_inputs & LCD_SENSOR_FAULT_BOTDHWTS_MASK) != 0U)
    {
        sensor_name = "BotDHWTS Fault";
    }
    else if ((g_sensor_fault_inputs & LCD_SENSOR_FAULT_OST_MASK) != 0U)
    {
        sensor_name = "OST Fault";
    }
    else
    {
        return 0U;
    }

    LCD_I2C_SetCursor(0U, 0U);
    LCD_I2C_PrintFixedWidth("SENSOR Fault", LCD_I2C_COLUMNS);
    LCD_I2C_SetCursor(1U, 0U);
    LCD_I2C_PrintFixedWidth(sensor_name, LCD_I2C_COLUMNS);

    return 1U;
}

static void LCD_I2C_PrintTempValue(float temperature_c)
{
    int16_t scaled;
    int16_t whole;
    int16_t tenths;

    if (temperature_c < -99.9f)
    {
        temperature_c = -99.9f;
    }
    else if (temperature_c > 999.9f)
    {
        temperature_c = 999.9f;
    }

    if (temperature_c >= 0.0f)
    {
        scaled = (int16_t)((temperature_c * 10.0f) + 0.5f);
    }
    else
    {
        scaled = (int16_t)((temperature_c * 10.0f) - 0.5f);
    }

    if (scaled < 0)
    {
        LCD_I2C_WriteChar('-');
        scaled = (int16_t)(-scaled);
    }
    else
    {
        LCD_I2C_WriteChar(' ');
    }

    whole = (int16_t)(scaled / 10);
    tenths = (int16_t)(scaled % 10);

    if (whole >= 100)
    {
        LCD_I2C_WriteChar((char)('0' + (whole / 100)));
        LCD_I2C_WriteChar((char)('0' + ((whole / 10) % 10)));
        LCD_I2C_WriteChar((char)('0' + (whole % 10)));
    }
    else if (whole >= 10)
    {
        LCD_I2C_WriteChar(' ');
        LCD_I2C_WriteChar((char)('0' + (whole / 10)));
        LCD_I2C_WriteChar((char)('0' + (whole % 10)));
    }
    else
    {
        LCD_I2C_WriteChar(' ');
        LCD_I2C_WriteChar(' ');
        LCD_I2C_WriteChar((char)('0' + whole));
    }

    LCD_I2C_WriteChar('.');
    LCD_I2C_WriteChar((char)('0' + tenths));
}
