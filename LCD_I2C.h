#ifndef LCD_I2C_H
#define LCD_I2C_H

#include <stdint.h>

#define LCD_I2C_DEFAULT_ADDRESS    0x27U
#define LCD_I2C_ALT_ADDRESS        0x3FU
#define LCD_I2C_COLUMNS            16U
#define LCD_I2C_ROWS               2U

extern volatile uint16_t g_lcd_i2c_state;
extern volatile uint16_t g_lcd_i2c_error;
extern volatile uint16_t g_lcd_i2c_updates;
extern volatile uint16_t g_lcd_i2c_address;
extern volatile uint16_t g_lcd_i2c_detected;

void LCD_I2C_Init(uint8_t address);
uint8_t LCD_I2C_IsReady(void);
void LCD_I2C_Clear(void);
void LCD_I2C_Home(void);
void LCD_I2C_SetCursor(uint8_t row, uint8_t column);
void LCD_I2C_Print(const char *text);
void LCD_I2C_PrintTemperature(const char *label, float temperature_c);
void LCD_I2C_ShowNTCTemperatures(void);

#endif
