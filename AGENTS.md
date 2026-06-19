# AGENTS.md - Test_Output.X Project Reference

## Project Identity

- Project: `Heat_Pump_DHWT.X`
- GitHub repository: `https://github.com/fadiassi411/-Heat_Pump_DHWT`
- MCU: `dsPIC33CK256MP505`
- Compiler/toolchain: Microchip `XC16` version `2.10`
- Device family pack: `Microchip dsPIC33CK-MP_DFP` version `1.15.423`
- Debug/programming tool configured in MPLAB X: `PK5Tool`
- Main purpose: dsPIC33CK ADC/debug firmware for reading multiple 10K NTC thermistors, showing TS readings on an I2C LCD with a PCF8574 backpack, tracking HP/LP/Flow faults and sensor open/short debug status, and exposing debug globals for MPLAB watch windows.

## Project Structure

MPLAB project metadata lists these project files:

- `main.c`
  - Contains configuration bits, clock definition, ADC setup for `AN0`, ADC read helper, global debug watch variables, and the main polling loop.
  - Configures the watchdog off at runtime for the ADC debug test.
  - Initializes ADC Dedicated Core 0, repeatedly reads AN0/SHWTS, converts the ADC value through the NTC module, calls the added NTC loop module, updates the I2C LCD, and services faults without freezing the display.

- `faults/fault_inputs.c`
  - Implements the fault input module.
  - Configures HP on `RC5` as active-low, LP on `RC4` as active-low, Flow Switch on `RC10` as active-high, and the fault LED on `RD8`.
  - Combines only HP/LP/Flow digital fault inputs into `g_fault_inputs` and `g_fault_active`.
  - `g_fault_inputs` remains raw input status. The heat pump control module filters Flow during pump proving and exposes the control decision in `g_hp_active_fault` / `g_hp_latched_fault`.
  - Exposes NTC sensor open/short status separately in `g_sensor_fault_inputs` for debug only.
  - NTC sensor open/short status does not stop the loop and does not light/blink `RD8` yet; that behavior will be added later after the rest of the code is complete.
  - The legacy blocking helpers still exist in the module, but the current `main.c` loop does not use them; it polls faults and toggles `RD8` while keeping ADC/LCD refresh alive.

- `faults/fault_inputs.h`
  - Public header for the fault input module.
  - Declares fault bit masks, debug globals, initialization, fault polling, sensor open/short detection, and RD8 blink helpers.

- `ntc_10k.c`
  - Implements 10K NTC conversion helpers.
  - `NTC10K_ADCToResistance(uint16_t adc)` converts 12-bit ADC counts into thermistor resistance.
  - `NTC10K_ADCToTemperatureC(uint16_t adc)` converts ADC counts into Celsius using the Beta formula.
  - Uses 10K fixed resistor, 10K at 25 C, Beta 3950, 12-bit ADC max 4095.

- `ntc_10k.h`
  - Public header for the NTC module.
  - Declares `NTC10K_ADCToResistance()` and `NTC10K_ADCToTemperatureC()`.

- `ntc_loops.c`
  - Implements additional NTC loop reads for SuCTS, EvpTS, RHWTS, TopDHWTS, BotDHWTS, and OST.
  - Keeps the existing AN0/Core 0 setup in `main.c` untouched.
  - SuCTS is documented as physically on `ANN2/RD13`; it is not read as normal single-ended AN2.

- `ntc_loops.h`
  - Public header for the added NTC loop module.
  - Exposes additional raw ADC, resistance, temperature, timeout, ADC core debug globals, and per-channel read counters.

- `outputs.c`
  - Implements application output control for the compressor, heater, fan, pump, and 4 way valve.
  - Configures `RB13`, `RC1`, `RC2`, `RC6`, and `RC3` as digital active-high outputs.
  - Initializes all controlled outputs off, then exposes bitmask and individual helper functions through the `Outputs_...` API.
  - Exposes output state and init status through MPLAB Watch debug globals.

- `outputs.h`
  - Public header for the output module.
  - Declares output bit masks, debug globals, initialization, whole-bitfield write/set/clear helpers, and individual output setters.


- `LCD_I2C.c`
  - Implements the PCF8574-backed HD44780 LCD driver over I2C1 at 100 kHz.
  - Configures RB9/SDA1 and RB8/SCL1 as digital open-drain inputs/outputs with external pull-ups.
  - Scans LCD I2C address `0x27` first, then `0x3F`, and uses timeouts so missing LCD hardware does not hang firmware.
  - Displays only the current requested service pages: SuCTS/EvpTS, TopDHWTS/BotDHWTS, and EEV opening percent/DHWT setpoint.
  - If a heat-pump HP, LP, or Flow fault is active or latched, the LCD stops rolling pages and shows `HP Fault`, `LP Fault`, or `FS Fault`.
  - Holds each LCD page for about 1 second using two 500 ms main-loop update passes.

- `LCD_I2C.h`
  - Public header for the LCD driver.
  - Declares LCD dimensions, default/alternate I2C addresses, LCD API, and debug globals for LCD state/address/detection/update count.


- `eeprom_24fc04.c`
  - Implements the Microchip 24FC04/24AA04/24LC04B-compatible 4-Kbit I2C EEPROM driver.
  - Shares the existing I2C1 bus with the LCD without changing the working RB9/SDA1 and RB8/SCL1 pin setup.
  - Uses 24FC04 block-select addressing: 0x000..0x0FF use B0 = 0, 0x100..0x1FF use B0 = 1, then only the lower 8-bit word address is sent.
  - Splits writes on 16-byte page boundaries, ACK-polls after writes, and uses timeouts to avoid hangs when EEPROM hardware is missing.
  - Stores control settings with magic/version/CRC8 validation: tank temperature, superheat delta-T, tank hysteresis, EEV startup/max steps, compressor and pump timing, LP bypass time, and heater enable.

- `eeprom_24fc04.h`
  - Public header for the EEPROM driver.
  - Defines EEPROM size, page size, selected memory addresses, defaults, error codes, `EEPROM24FC04_Settings`, debug globals, settings APIs, and legacy setpoint wrappers.

- `eev.c`
  - Implements the Electronic Expansion Valve stepper driver.
  - Configures EEV coil outputs `RB0`, `RB1`, `RD10`, and `RC7` as digital outputs and initializes all four off.
  - Uses an 8-step half-step sequence with 3 ms on / 3 ms off timing, about 166.7 Hz.
  - On startup sends 620 close pulses to guarantee the valve is fully closed, then sets the tracked position to 0 steps / 0%.
  - Tracks valve position in steps and percent with limits of 0 to 560 steps, where 560 steps is 100% open.
  - Contains `EEV_DISABLE_PGD2_PINS_DURING_DEBUG` as a debug-build EEV pin safety switch from earlier PGD2 troubleshooting.
  - For PGD2 debug sessions this macro is set to `1`, so `__DEBUG` builds skip EEV pin driving and startup movement while production/program builds still run real EEV initialization, startup close, and movement.

- `eev.h`
  - Public header for the EEV driver.
  - Declares EEV position limits, startup close count, debug globals, initialization, step, move, percent-target, and getter APIs.

- `heatpump_control.c`
  - Implements the real DHWT heat pump control state machine through Part 9.
  - Provides `HeatPumpControl_Init()` and `HeatPumpControl_Task()`.
  - Uses the existing `Outputs_...` API for safe output command handling.
  - Part 9 allows the pump output to turn on during pump pre-run and later running/post-run states.
  - Part 9 moves the EEV to a fixed startup preposition of 25% / 140 steps in `HP_STATE_EEV_PREPOSITION`, then moves to `HP_STATE_COMPRESSOR_START`.
  - Part 9 turns the fan output on in `HP_STATE_COMPRESSOR_START` after EEV preposition is complete.
  - Part 9 adds compressor-start permission timing and a transition to `HP_STATE_RUNNING`.
  - Part 9 adds automatic fault retry and reset-only lockout behavior.
  - Part 9 keeps pump and fan on during `HP_STATE_POST_RUN` for `g_pump_post_run_sec`, then turns them off and returns to `HP_STATE_IDLE`.
  - Part 9 adds a minimum compressor run timer using `HP_MIN_COMPRESSOR_RUN_SEC`, default 120 seconds.
  - Part 9 adds slow EEV running superheat control using `SuCTS - EvpTS` compared with `g_superheat_delta_t_setpoint_c`.
  - Part 9 limits heat-pump-controlled EEV running range to 10% through 40% to reduce compressor liquid-return and evaporator starvation risk. The low-level EEV driver still has the absolute 0 to 560 step mechanical limit.
  - Real-test safety layer uses SHWTS as Supply Hot Water Temperature and RHWTS as Return Hot Water Temperature for high-temperature stop protection.
  - Real-test safety layer stops on SHWTS or RHWTS at 75 C or higher during compressor-start/running, EvpTS freeze at 2 C or lower during compressor-start/running, TopDHWTS/BotDHWTS tank over-temperature at 75 C or higher, and EEV below the 10% minimum opening while running.
  - Real-test safety layer adds a compressor minimum-off timer using the EEPROM-backed anti-short-cycle time after every compressor stop.
  - `HP_ENABLE_COMPRESSOR_OUTPUT` controls whether `g_hp_compressor_allowed` is allowed to energize the real compressor relay output on `RB13`.
  - Contains `HP_ENABLE_FAN_OUTPUT`, which is set to `1` in Part 9 so the fan output may turn on after EEV preposition and remain on during post-run.
  - Flow switch handling keeps the existing input polarity: `RC10` high means Flow fault. Because the switch is normally high before the pump runs, the control module ignores Flow during idle/precheck and during pump pre-run, then requires Flow to become low after the pump pre-run/proving time.
  - If Flow is still high after pump pre-run expires, the control module stops the pump, latches `FAULT_INPUT_FLOW_MASK`, and the fault LED blinks from the heat-pump filtered/latched fault state.
  - Reads EEPROM-backed settings globals for tank setpoint, tank hysteresis, pump pre/post-run time, compressor anti-short-cycle time, and LP startup bypass time.
  - Tracks HP/LP/Flow faults from the fault module for the control-state fault decision.
  - Contains `HP_IGNORE_SENSOR_FAULTS_FOR_BENCH_TEST`, a compile-time macro for bench testing with unplugged sensors.
  - When `HP_IGNORE_SENSOR_FAULTS_FOR_BENCH_TEST` is `1`, NTC sensor open/short bits remain visible in `g_sensor_fault_inputs` but are ignored by the heat pump control FAULT decision.
  - When `HP_IGNORE_SENSOR_FAULTS_FOR_BENCH_TEST` is `0`, NTC sensor faults are included in `g_hp_active_fault` and block the control state machine.
  - Uses `TopDHWTS` and `BotDHWTS` as the real DHWT tank demand sensors.
  - Starts heating from bottom tank temperature and stops heating from top tank temperature.
  - Uses `OST` as an outdoor-temperature compensator for the effective DHWT tank setpoint.
  - OST compensation maps `OST <= 5 C` to a 70 C DHWT setpoint and `OST >= 20 C` to a 50 C DHWT setpoint, with linear interpolation between 5 C and 20 C.
  - If OST is invalid, the effective setpoint falls back to the EEPROM-backed `g_tank_temp_setpoint_c`; the code does not fake an outdoor temperature.
  - Invalid TopDHWTS or BotDHWTS keeps heating request off and the state machine safely in `HP_STATE_IDLE`.

- `heatpump_control.h`
  - Public header for the heat pump control module.
  - Declares `HeatPumpControl_State`, state values, debug globals, and init/task APIs.

- `app_config.h`
  - Central constants intended for application-level configuration.
  - Defines `APP_FCY_HZ`, task period, ADC reference values, NTC constants, pressure sensor scaling constants, and EEV timing/position constants.
  - Included by `LCD_I2C.c` for `APP_FCY_HZ`; other modules still define or use their own local constants where already established.

- `Makefile`
  - MPLAB generated make entry point. Includes `nbproject/Makefile-impl.mk` and `nbproject/Makefile-variables.mk`.

- `nbproject/`
  - MPLAB X project configuration, source membership, toolchain/device settings, generated makefiles, and private IDE settings.

Generated/build folders:

- `.generated_files/`, `build/`, `debug/`, `dist/`
  - Generated output folders. Do not edit these by hand unless specifically working on build artifacts.

## Configuration Bits And Clock

Configuration bits are currently in `main.c`:

```c
#pragma config FNOSC = FRC
#pragma config POSCMD = NONE
#pragma config OSCIOFNC = ON
#pragma config ALTI2C1 = OFF
#pragma config ICS = PGD2
#pragma config JTAGEN = OFF
```

Clock-related definitions:

```c
#define FCY 4000000UL
#include <libpic30.h>
```

Rules:

- Do not change working configuration bits unless explicitly asked.
- Do not move, rewrite, or regenerate configuration-bit setup unless necessary for a requested change.
- Keep delay timing consistent with `FCY` when adding timing-dependent code.

## Configured Inputs And Outputs

### Inputs

- `RA0 / AN0`
  - Configured in `ADC_Init_AN0()`.
  - Direction: input via `TRISAbits.TRISA0 = 1`.
  - Analog mode: enabled via `ANSELAbits.ANSELA0 = 1`.
  - ADC channel: `ADC_AN0_CHANNEL` set to `0U`.
  - ADC core: Dedicated ADC Core 0.
  - Intended package note in code: pin 8 on 48-pin `dsPIC33CK256MP505` package.
  - Used for a 10K NTC divider.

- `RC5`
  - Configured in `FaultInputs_Init()`.
  - Direction: digital input via `TRISCbits.TRISC5 = 1`.
  - Fault role: HP alarm.
  - Polarity: active-low. `PORTCbits.RC5 == 0` means HP fault active.

- `RC4`
  - Configured in `FaultInputs_Init()`.
  - Direction: digital input via `TRISCbits.TRISC4 = 1`.
  - Fault role: LP alarm.
  - Polarity: active-low. `PORTCbits.RC4 == 0` means LP fault active.

- `RC10`
  - Configured in `FaultInputs_Init()`.
  - Direction: digital input via `TRISCbits.TRISC10 = 1`.
  - Fault role: Flow Switch alarm.
  - Polarity: active-high. `PORTCbits.RC10 != 0` means Flow Switch fault active.

- Additional NTC ADC channels
  - EvpTS: `AN12 / RC0`.
  - RHWTS: `ANA1 / RA1`.
  - TopDHWTS: `AN9 / RA2`.
  - BotDHWTS: `AN3 / RA3`.
  - OST: `AN4 / RA4`.
  - SuCTS is physically `ANN2 / RD13` and is not read as normal single-ended AN2.


- I2C LCD pins
  - `RB9 / SDA1`: I2C1 data for the PCF8574 LCD backpack.
  - `RB8 / SCL1`: I2C1 clock for the PCF8574 LCD backpack.
  - Both pins are configured digital, open-drain via `ODCBbits`, with external 4.7k pull-up resistors.
  - I2C1 pin selection is controlled by `#pragma config ALTI2C1 = OFF`; do not add normal RPOR/RPINR PPS mapping for SDA1/SCL1 on this device.


- I2C EEPROM
  - Device family: Microchip 24FC04 / 24AA04 / 24LC04B compatible 4-Kbit EEPROM.
  - Total size: 512 bytes, address range `0x000..0x1FF`.
  - Organization: two 256-byte blocks selected by the control-byte B0 bit.
  - `A0`, `A1`, and `A2` are connected to GND but are not used internally for 24FC04 addressing. Do not include them when calculating the device address.
  - `WP` is connected to GND, so writes are allowed.
  - Shares I2C1 with the LCD at 100 kHz on RB9/SDA1 and RB8/SCL1 with external 4.7k pull-ups.

### ADC Configuration

- ADC module enabled by clearing `PMD1bits.ADC1MD` and setting `ADCON1Lbits.ADON`.
- ADC output format: integer, `ADCON1Hbits.FORM = 0`.
- ADC reference: AVDD/AVSS, `ADCON3Lbits.REFSEL = 0`.
- ADC clock: peripheral clock, `ADCON3Hbits.CLKSEL = 0`, divider `ADCON3Hbits.CLKDIV = 3`.
- Dedicated Core 0 enabled with `ADCON3Hbits.C0EN = 1`.
- Core 0 positive input selected with `ADCON4Hbits.C0CHS = ADC_AN0_CHANNEL`.
- Extra sample time enabled with `ADCON4Lbits.SAMC0EN = 1`.
- Core 0 setup: `ADCORE0Hbits.ADCS = 5`, `ADCORE0Hbits.RES = 3`, `ADCORE0Lbits.SAMC = 20`.
- AN0 mode: unsigned, single-ended via `ADMOD0Lbits.SIGN0 = 0`, `ADMOD0Lbits.DIFF0 = 0`.
- AN0 software trigger source is explicitly set with `ADTRIG0Lbits.TRGSRC0 = 1`.
- Warm-up: `ADCON5Hbits.WARMTIME = 15`.
- Core 0 powered by `ADCON5Lbits.C0PWR = 1`, then code waits for `ADCON5Lbits.C0RDY`.
- Additional NTC channels use explicit software trigger sources: AN1, AN3, AN4, AN9, and AN12 are configured in `ntc_loops.c` through their `ADTRIGx` fields.

### Outputs

- `RD8`
  - Configured in `FaultInputs_Init()`.
  - Direction: digital output via `TRISDbits.TRISD8 = 0`.
  - Fault LED output.
  - Toggles every 500 ms while `g_hp_active_fault` or `g_hp_latched_fault` is active.
  - Cleared to off when the heat-pump filtered/latched fault state is clear.

- `RB13`
  - Configured in `Outputs_Init()`.
  - Direction: digital output via `TRISBbits.TRISB13 = 0`.
  - No `ANSELB13` bit exists in the dsPIC33CK256MP505 header; `Outputs_Init()` leaves analog-disable handling out for this pin and drives it through `LATBbits.LATB13`.
  - Output role: Compressor.
  - Polarity: active-high. `LATBbits.LATB13 = 1` means compressor output enabled.

- `RC1`
  - Configured in `Outputs_Init()`.
  - Direction: digital output via `TRISCbits.TRISC1 = 0`.
  - Analog mode disabled via `ANSELCbits.ANSELC1 = 0`.
  - Output role: Heater.
  - Polarity: active-high. `LATCbits.LATC1 = 1` means heater output enabled.

- `RC2`
  - Configured in `Outputs_Init()`.
  - Direction: digital output via `TRISCbits.TRISC2 = 0`.
  - Analog mode disabled via `ANSELCbits.ANSELC2 = 0`.
  - Output role: FAN.
  - Polarity: active-high. `LATCbits.LATC2 = 1` means fan output enabled.

- `RC6`
  - Configured in `Outputs_Init()`.
  - Direction: digital output via `TRISCbits.TRISC6 = 0`.
  - Analog mode disabled via `ANSELCbits.ANSELC6 = 0`.
  - Output role: Pump.
  - Polarity: active-high. `LATCbits.LATC6 = 1` means pump output enabled.

- `RC3`
  - Configured in `Outputs_Init()`.
  - Direction: digital output via `TRISCbits.TRISC3 = 0`.
  - Analog mode disabled via `ANSELCbits.ANSELC3 = 0`.
  - Output role: 4 Way Valve.
  - Polarity: active-high. `LATCbits.LATC3 = 1` means 4 way valve output enabled.

- EEV stepper coil outputs
  - Configured in `EEV_Init()`.
  - `RB0`: EEV A1 coil output.
    - Analog mode disabled via `ANSELBbits.ANSELB0 = 0`.
    - Direction: digital output via `TRISBbits.TRISB0 = 0`.
  - `RB1`: EEV B1 coil output.
    - Analog mode disabled via `ANSELBbits.ANSELB1 = 0`.
    - Direction: digital output via `TRISBbits.TRISB1 = 0`.
  - `RD10`: EEV A2 coil output.
    - Analog mode disabled via `ANSELDbits.ANSELD10 = 0`.
    - Direction: digital output via `TRISDbits.TRISD10 = 0`.
  - `RC7`: EEV B2 coil output.
    - Analog mode disabled via `ANSELCbits.ANSELC7 = 0`.
    - Direction: digital output via `TRISCbits.TRISC7 = 0`.
  - All four outputs are initialized off via `EEV_AllOutputsOff()`.
  - The driver uses an 8-step half-step coil sequence documented in `eev.c`.
  - Timing is 3 ms energized and 3 ms off per step, about 166.7 Hz, inside the 100 to 300 Hz pulse frequency and 2 to 5 ms pulse width requirements.
  - Startup close behavior: `EEV_Init()` sends 620 close pulses, then resets the software position to 0 steps / 0%.
  - Position limits: minimum 0 steps, maximum 560 steps. 560 steps is treated as 100% valve opening.


- I2C LCD display
  - Uses I2C1 at 100 kHz through a PCF8574 backpack.
  - Default address scan order is `0x27`, then `0x3F`.
  - Display pages roll about once per second: SuCTS/EvpTS, TopDHWTS/BotDHWTS, and EEV opening percent/effective compensated DHWT setpoint.
  - HP/LP/Flow, heat-pump software safety faults, and blocking sensor faults override the rolling pages and show the matching fault message until the active/latched fault clears.
  - Sensor fault LCD display reads `g_sensor_fault_inputs` and identifies the first active NTC sensor fault by priority: SHWTS, EvpTS, RHWTS, TopDHWTS, BotDHWTS, then OST.
  - LCD routines have I2C timeouts and return without hanging if no backpack is detected.

Runtime/debug outputs are exposed as volatile globals for MPLAB watch/debugging:

- Raw ADC value: `g_ntc_adc_raw`
- Direct ADC buffer readback: `g_adcbuf0_direct`
- ADC status/debug flags: `g_adc_timeout`, `g_adc_adon`, `g_adc_c0en`, `g_adc_c0rdy`, `g_adc_pmd`
- Program progress: `g_main_state`, `g_loop_counter`
- Calculated values: `g_ntc_resistance`, `g_ntc_temperature_c`
- Fault status: `g_fault_active`, `g_fault_inputs`, `g_sensor_fault_inputs`
- Output status: `g_output_states`, `g_output_init_done`
- EEV status: `g_eev_position_steps`, `g_eev_position_percent`, `g_eev_startup_close_done`, `g_eev_init_done`
- LCD status: `g_lcd_i2c_state`, `g_lcd_i2c_error`, `g_lcd_i2c_updates`, `g_lcd_i2c_address`, `g_lcd_i2c_detected`
- EEPROM status: `g_eeprom_present`, `g_eeprom_error`, `g_eeprom_last_addr`, `g_eeprom_last_data`, `g_eeprom_write_count`, `g_eeprom_read_count`
- Settings: `g_tank_temp_setpoint_c`, `g_superheat_delta_t_setpoint_c`, `g_tank_hysteresis_c`, `g_eev_startup_steps_setting`, `g_eev_max_steps_setting`, `g_compressor_anti_short_cycle_sec`, `g_lp_startup_bypass_sec`, `g_pump_pre_run_sec`, `g_pump_post_run_sec`, `g_heater_enabled_setting`
- Heat pump control status: `g_hp_state`, `g_hp_previous_state`, `g_hp_active_fault`, `g_hp_latched_fault`, `g_hp_heating_request`, `g_hp_compressor_allowed`, `g_hp_pump_command`, `g_hp_fan_command`, `g_hp_heater_command`, `g_hp_eev_target_percent`, `g_hp_eev_preposition_done`, `g_hp_eev_target_steps`, `g_hp_top_dhwt_temp_c`, `g_hp_bottom_dhwt_temp_c`, `g_hp_ost_temp_c`, `g_hp_dhwt_compensated_setpoint_c`, `g_hp_supply_hot_water_temp_c`, `g_hp_return_water_temp_c`, `g_hp_top_dhwt_valid`, `g_hp_bottom_dhwt_valid`, `g_hp_ost_valid`, `g_hp_supply_hot_water_valid`, `g_hp_return_water_valid`, `g_hp_safety_fault`, `g_hp_state_elapsed_ms`, `g_hp_pump_pre_run_remaining_sec`, `g_hp_pump_post_run_remaining_sec`, `g_hp_anti_short_cycle_remaining_sec`, `g_hp_compressor_min_off_remaining_sec`, `g_hp_min_run_remaining_sec`, `g_hp_lp_bypass_remaining_sec`, `g_hp_compressor_start_ready`, `g_hp_flow_proving_active`, `g_hp_flow_proven`, `g_hp_fault_retry_count`, `g_hp_fault_retry_limit`, `g_hp_fault_retry_pending`, `g_hp_fault_lockout`, `g_hp_last_fault`, `g_hp_suction_temp_c`, `g_hp_evap_temp_c`, `g_hp_superheat_delta_t_c`, `g_hp_superheat_target_c`, `g_hp_suction_temp_valid`, `g_hp_evap_temp_valid`, `g_hp_eev_control_enabled`, `g_hp_eev_control_remaining_sec`, `g_hp_eev_last_step_command`, `g_hp_eev_control_max_percent`, `g_hp_eev_control_max_steps`, `g_hp_eev_control_limit_active`, `g_hp_eev_control_min_percent`, `g_hp_eev_control_min_steps`, `g_hp_eev_control_min_limit_active`
- ADC read counters: `g_shwts_adc_read_counter`, `g_evpts_adc_read_counter`, `g_rhwts_adc_read_counter`, `g_topdhwts_adc_read_counter`, `g_botdhwts_adc_read_counter`, `g_ost_adc_read_counter`

### Fault Configuration

Fault bit masks in `faults/fault_inputs.h`:

- `FAULT_INPUT_HP_MASK = 0x0001`
- `FAULT_INPUT_LP_MASK = 0x0002`
- `FAULT_INPUT_FLOW_MASK = 0x0004`
- `FAULT_INPUT_SENSOR_MASK = 0x0008`

Sensor open/short detection in `faults/fault_inputs.c`:

- `0..5` ADC counts means sensor short fault.
- `4090..4095` ADC counts means sensor open fault.
- `0xFFFF` ADC timeout is not treated as an open/short sensor fault by `FaultInputs_IsSensorOpenOrShort()`.

Sensor fault detail bits are stored in `g_sensor_fault_inputs`:

- Bit `0x0001`: SHWTS / main AN0 NTC fault.
- Bit `0x0002`: EvpTS fault.
- Bit `0x0004`: RHWTS fault.
- Bit `0x0008`: TopDHWTS fault.
- Bit `0x0010`: BotDHWTS fault.
- Bit `0x0020`: OST fault.
- Current behavior: these bits are debug/status only. They are not ORed into `g_fault_inputs`, do not set `g_fault_active`, and do not blink `RD8`.

## ADC / NTC 10K Reading Flow

1. `main()` disables the watchdog at runtime with `WDTCONLbits.ON = 0`.
2. `main()` sets `g_main_state = 1` and calls `ADC_Init_AN0()`.
   - Startup also calls `HeatPumpControl_Init()` after EEPROM settings are loaded so application outputs are safe OFF.
   - Startup then calls `EEV_Init()` once, before the main loop, so the EEV performs its 620-pulse startup close and resets position to 0%.
3. `ADC_Init_AN0()`:
   - Sets `g_main_state = 2`.
   - Enables the ADC peripheral by clearing `PMD1bits.ADC1MD`.
   - Configures `RA0` as analog input `AN0`.
   - Disables ADC before changing ADC configuration.
   - Configures ADC format, reference, clock, Dedicated Core 0, sample time, resolution, and AN0 single-ended mode.
   - Enables ADC with `ADCON1Lbits.ADON = 1` and sets `g_main_state = 3`.
   - Powers Core 0 with `ADCON5Lbits.C0PWR = 1` and sets `g_main_state = 4`.
   - Waits until `ADCON5Lbits.C0RDY` becomes ready or timeout expires.
   - On Core 0 ready timeout, sets `g_adc_timeout = 2` and `g_main_state = 0xE002`.
4. The infinite loop in `main()`:
   - Polls `FaultInputs_IsActive()` for HP/LP/Flow status, but does not block ADC/LCD refresh.
   - Increments `g_loop_counter`.
   - Calls `ADC_Read_AN0()` and stores the result in `g_ntc_adc_raw`.
   - Copies `ADCBUF0` into `g_adcbuf0_direct`.
   - Updates ADC debug globals.
   - If the ADC read returns `0xFFFF`, treats it as timeout/error, sets `g_adc_timeout = 1`, and clears calculated resistance/temperature to `0.0f`.
   - Otherwise clears `g_adc_timeout`, calculates resistance with `NTC10K_ADCToResistance()`, and calculates temperature with `NTC10K_ADCToTemperatureC()`.
   - Calls `NTCLoops_Task()` for the additional NTC channels.
   - Calls `LCD_I2C_ShowNTCTemperatures()` to refresh the active LCD page.
   - Calls the local `UpdateSensorFaults()` helper to update NTC open/short debug bits using the fault module thresholds.
   - Calls `HeatPumpControl_Task()` for the Part 5 DHWT heat pump state machine.
   - Calls `DelayAndServiceFaults()`, which polls HP/LP/Flow raw inputs for a 500 ms loop period and toggles `RD8` from the heat-pump filtered/latched fault state.

`ADC_Read_AN0()` flow:

1. Sets `g_main_state = 5`.
2. Checks `ADCON1Lbits.ADON` and `ADCON5Lbits.C0RDY`; if either is not ready, sets `g_main_state = 0xE001` and returns `0xFFFF`.
3. Selects channel AN0 using `ADCON3Lbits.CNVCHSEL = ADC_AN0_CHANNEL`.
4. Reads `ADCBUF0` once to drain any previous AN0 result.
5. Starts conversion with `ADCON3Lbits.CNVRTCH = 1`.
6. Waits for `ADCON3Lbits.CNVRTCH` to clear, then waits for `ADSTATLbits.AN0RDY` or timeout.
7. On conversion timeout, sets `g_main_state = 0xE003` and returns `0xFFFF`.
8. On success, increments `g_shwts_adc_read_counter`, sets `g_main_state = 6`, and returns `ADCBUF0`.

Important note about divider comments:

- `main.c` describes hardware as `3.3V ---- 10k fixed resistor ---- AN0 / RA0 ---- NTC 10k ---- GND`.
- `ntc_10k.c` describes and calculates for `3.3V --- NTC --- ADC pin --- 10k fixed resistor --- GND`.
- Before changing formulas or wiring assumptions, confirm the actual board wiring. Do not silently change this in future work.

## Existing Global Debug Variables

All existing debug variables are `volatile` so MPLAB watch/debug views can observe live changes.

- `g_ntc_adc_raw`
  - Last return value from `ADC_Read_AN0()`.
  - Normal range is 0 to 4095. `0xFFFF` means ADC read timeout/error.

- `g_adcbuf0_direct`
  - Direct copy of `ADCBUF0` after the read call.
  - Useful for comparing the helper return value against the hardware buffer.

- `g_adc_timeout`
  - `0`: last conversion path succeeded.
  - `1`: conversion timeout in the main loop after `ADC_Read_AN0()` returned `0xFFFF`.
  - `2`: Dedicated Core 0 did not become ready during ADC initialization.

- `g_adc_adon`
  - Snapshot of `ADCON1Lbits.ADON`.
  - `1` means ADC module is enabled.

- `g_adc_c0en`
  - Snapshot of `ADCON3Hbits.C0EN`.
  - `1` means Dedicated ADC Core 0 is enabled.

- `g_adc_c0rdy`
  - Snapshot of `ADCON5Lbits.C0RDY`.
  - `1` means Dedicated Core 0 is ready.

- `g_adc_pmd`
  - Snapshot of `PMD1bits.ADC1MD`.
  - `0` means ADC peripheral is not disabled by PMD.

- `g_main_state`
  - Simple state/progress marker.
  - `1`: entered `main()` and watchdog disabled.
  - `2`: entered `ADC_Init_AN0()`.
  - `3`: ADC module enabled.
  - `4`: Core 0 powered and waiting/ready check started.
  - `5`: entered `ADC_Read_AN0()`.
  - `6`: ADC conversion completed successfully.
  - `0xE001`: ADC read attempted while ADC/core not ready.
  - `0xE002`: Core 0 ready timeout during initialization.
  - `0xE003`: AN0 conversion timeout.

- `g_loop_counter`
  - Increments once per main loop iteration.
  - Useful to confirm firmware is alive and not stuck.

- `g_ntc_resistance`
  - Last calculated NTC resistance in ohms.
  - Set to `0.0f` on ADC timeout/error in `main()`.

- `g_ntc_temperature_c`
  - Last calculated NTC temperature in degrees Celsius.
  - Set to `0.0f` on ADC timeout/error in `main()`.

- `g_fault_active`
  - `0`: no HP, LP, or Flow fault is currently active.
  - `1`: at least one HP, LP, or Flow fault is currently active.

- `g_fault_inputs`
  - Bitfield combining active fault categories.
  - `0x0001`: HP input fault.
  - `0x0002`: LP input fault.
  - `0x0004`: Flow Switch input fault.
  - `0x0008`: reserved sensor fault mask, not currently set by `FaultInputs_IsActive()`.

- `g_sensor_fault_inputs`
  - Debug/status bitfield showing which NTC sensor has an open/short condition.
  - These bits do not currently affect `g_fault_active`, `g_fault_inputs`, or `RD8`.
  - `0x0001`: SHWTS / main AN0 NTC.
  - `0x0002`: EvpTS.
  - `0x0004`: RHWTS.
  - `0x0008`: TopDHWTS.
  - `0x0010`: BotDHWTS.
  - `0x0020`: OST.

- `g_output_states`
  - Bitfield showing which application outputs are currently requested on.
  - `0x0001`: Compressor output on `RB13`.
  - `0x0002`: Heater output on `RC1`.
  - `0x0004`: FAN output on `RC2`.
  - `0x0008`: Pump output on `RC6`.
  - `0x0010`: 4 Way Valve output on `RC3`.

- `g_output_init_done`
  - `0`: `Outputs_Init()` has not completed.
  - `1`: output pins have been configured and initialized off.

- `g_eev_position_steps`
  - Current tracked EEV opening in steps.
  - Clamped to `0..560`.
  - `0` means fully closed; `560` means 100% open.

- `g_eev_position_percent`
  - Current tracked EEV opening in percent.
  - Calculated from `g_eev_position_steps`.
  - `EEV_MoveToPercent(0)` moves to fully closed; `EEV_MoveToPercent(100)` moves to 560 steps open.

- `g_eev_startup_close_done`
  - `0`: startup over-close has not completed.
  - `1`: `EEV_Init()` has sent 620 closing pulses and reset position to 0%.

- `g_eev_init_done`
  - `0`: `EEV_Init()` has not completed.
  - `1`: EEV pins are configured, startup close has completed, and all coil outputs are off.

- `g_hp_state`
  - Current Part 5 heat pump control state.
  - Values are from `HeatPumpControl_State`: `HP_STATE_IDLE`, `HP_STATE_PRECHECK`, `HP_STATE_PUMP_PRE_RUN`, `HP_STATE_EEV_PREPOSITION`, `HP_STATE_COMPRESSOR_START`, `HP_STATE_RUNNING`, `HP_STATE_POST_RUN`, and `HP_STATE_FAULT`.

- `g_hp_previous_state`
  - Previous heat pump control state before the most recent state transition.

- `g_hp_active_fault`
  - Current heat pump control fault bitfield.
  - Low bits include HP/LP/Flow masks from `g_fault_inputs`.
  - If `HP_IGNORE_SENSOR_FAULTS_FOR_BENCH_TEST` is `0`, NTC sensor open/short debug bits are shifted into this value and block the control state machine.
  - If `HP_IGNORE_SENSOR_FAULTS_FOR_BENCH_TEST` is `1`, NTC sensor bits remain visible in `g_sensor_fault_inputs` but are not included here.

- `g_hp_latched_fault`
  - Placeholder for future serious fault latching.
  - Part 5 does not implement complicated reset logic.

- `g_hp_heating_request`
  - Latched DHWT heating request.
  - Set to `1` when `g_hp_bottom_dhwt_temp_c <= (g_hp_dhwt_compensated_setpoint_c - g_tank_hysteresis_c)`.
  - Held at `1` until `g_hp_top_dhwt_temp_c >= g_hp_dhwt_compensated_setpoint_c`.
  - Forced to `0` if TopDHWTS or BotDHWTS is invalid.

- `g_hp_top_dhwt_temp_c`
  - Copy of `g_topdhwts_temperature_c`, the top domestic hot water tank temperature.
  - Used to stop heating when it reaches the tank setpoint.

- `g_hp_bottom_dhwt_temp_c`
  - Copy of `g_botdhwts_temperature_c`, the bottom domestic hot water tank temperature.
  - Used to start heating when it falls to tank setpoint minus hysteresis.

- `g_hp_top_dhwt_valid`
  - `1`: TopDHWTS raw ADC is not open/short by the existing fault thresholds.
  - `0`: TopDHWTS is invalid; heat request is forced off.

- `g_hp_bottom_dhwt_valid`
  - `1`: BotDHWTS raw ADC is not open/short by the existing fault thresholds.
  - `0`: BotDHWTS is invalid; heat request is forced off.

- `g_hp_supply_hot_water_temp_c`
  - Copy of `g_shwts_temperature_c`, used as Supply Hot Water Temperature for high-temperature compressor stop protection.

- `g_hp_return_water_temp_c`
  - Copy of `g_rhwts_temperature_c`, used as Return Hot Water Temperature for high-temperature compressor stop protection.

- `g_hp_supply_hot_water_valid`, `g_hp_return_water_valid`
  - `1`: related sensor raw ADC is not open/short by the existing fault thresholds.
  - `0`: related sensor is invalid; with `HP_IGNORE_SENSOR_FAULTS_FOR_BENCH_TEST = 0`, the sensor fault blocks the heat pump control state machine.

- `g_hp_safety_fault`
  - Software safety fault bitfield ORed into `g_hp_active_fault`.
  - `HP_CONTROL_FAULT_HIGH_TEMP_MASK`: SHWTS or RHWTS is 75 C or higher during compressor-start/running.
  - `HP_CONTROL_FAULT_FREEZE_MASK`: EvpTS is 2 C or lower during compressor-start/running.
  - `HP_CONTROL_FAULT_TANK_OVERTEMP_MASK`: TopDHWTS or BotDHWTS is 75 C or higher.
  - `HP_CONTROL_FAULT_EEV_MIN_OPEN_MASK`: EEV is below the 10% running minimum while the compressor is running.

- `g_hp_ost_temp_c`
  - Copy of `g_ost_temperature_c`, the outdoor sensor temperature used for DHWT setpoint compensation.

- `g_hp_dhwt_compensated_setpoint_c`
  - Effective DHWT stop setpoint used by heat-pump control.
  - `70 C` when OST is 5 C or colder.
  - `50 C` when OST is 20 C or warmer.
  - Linearly interpolated between 5 C and 20 C.
  - Falls back to `g_tank_temp_setpoint_c` if OST is invalid.

- `g_hp_ost_valid`
  - `1`: OST raw ADC is not open/short by the existing fault thresholds.
  - `0`: OST is invalid; compensation falls back to the EEPROM tank setpoint.

- `g_hp_compressor_allowed`
  - Part 5 may set this to `1` for state-machine/debug permission after the anti-short-cycle countdown, but the real compressor relay remains off while `HP_ENABLE_COMPRESSOR_OUTPUT = 0`.
  - `HP_ENABLE_COMPRESSOR_OUTPUT` is set to `0`, so the real compressor relay is forced off by `HeatPumpControl_ApplyCommands()`.

- `g_hp_pump_command`, `g_hp_fan_command`, `g_hp_heater_command`
  - In Part 5, `g_hp_pump_command` may become `1` during `HP_STATE_PUMP_PRE_RUN`, `HP_STATE_EEV_PREPOSITION`, `HP_STATE_COMPRESSOR_START`, `HP_STATE_RUNNING`, and `HP_STATE_POST_RUN`.
  - `g_hp_fan_command` and `g_hp_heater_command` remain `0`.

- `g_hp_eev_target_percent`
  - Part 5 sets this to `25` during EEV preposition, compressor-start timing, and running simulation.
  - This is a fixed TODO startup opening because the current EEPROM EEV startup setting is the startup close count, not startup open position.

- `g_hp_eev_preposition_done`
  - `1` after `HP_STATE_EEV_PREPOSITION` has moved the EEV to the startup open position.

- `g_hp_eev_target_steps`
  - Part 5 target EEV opening in steps.
  - Fixed at `140` steps for the 25% startup preposition, where 560 steps is 100%.

- `g_hp_state_elapsed_ms`
  - Non-blocking elapsed time counter for the current heat pump state.
  - Reset to `0` on every state transition and incremented once per `HeatPumpControl_Task()` call using the 500 ms main-loop period.

- `g_hp_pump_pre_run_remaining_sec`
  - Remaining pump pre-run time in seconds while `HP_STATE_PUMP_PRE_RUN` is active.
  - Counts down from `g_pump_pre_run_sec` and reaches `0` before moving to `HP_STATE_EEV_PREPOSITION`.

- `g_hp_anti_short_cycle_remaining_sec`
  - Reserved debug variable for compressor anti-short-cycle timing.
  - Part 5 counts this down in `HP_STATE_COMPRESSOR_START` from `g_compressor_anti_short_cycle_sec`; when it reaches `0`, compressor permission debug state may become ready.

- `g_hp_flow_proving_active`
  - `1` while `HP_STATE_PUMP_PRE_RUN` is giving the pump time to create flow.
  - During this window, raw Flow fault input remains visible in `g_fault_inputs`, but it does not stop the pump.

- `g_hp_flow_proven`
  - Set to `1` when Flow is low at the end of pump pre-run.
  - If Flow remains high after pump pre-run, `g_hp_latched_fault` receives `FAULT_INPUT_FLOW_MASK`, the pump is stopped, and the LED blinks.


- `g_lcd_i2c_updates`
  - Increments every successful LCD refresh call.
  - Useful to confirm the display is not frozen.

- `g_lcd_i2c_detected`
  - `1`: an LCD backpack ACKed during initialization.
  - `0`: no LCD backpack was detected at `0x27` or `0x3F`.

- `g_lcd_i2c_address`
  - Stores the detected LCD I2C address, normally `0x27` or `0x3F`.

- `g_ts*_adc_read_counter`
  - Per-channel counters for EvpTS, SHWTS, RHWTS, TopDHWTS, BotDHWTS, and OST.
  - Useful to prove each ADC read path is still running even during fault conditions.



## I2C EEPROM Behavior

- EEPROM module files: `eeprom_24fc04.c` and `eeprom_24fc04.h`.
- I2C peripheral: shares the already-working I2C1 bus with the LCD.
- Do not change the existing LCD/I2C1 pin configuration for EEPROM work.
- 24FC04 addressing rule:
  - Device code is `1010`.
  - Use 7-bit control address `0x50` for EEPROM memory `0x000..0x0FF`.
  - Use 7-bit control address `0x51` for EEPROM memory `0x100..0x1FF`.
  - Send only the lower 8-bit word address byte after the control byte.
  - A0/A1/A2 package pins are not used for address selection on this EEPROM.
- EEPROM memory map selected by firmware:
  - `0x000..0x01E`: settings record, 31 bytes total.
  - `0x000`: magic low byte.
  - `0x001`: magic high byte.
  - `0x002`: version byte, currently `2`.
  - `0x003`: reserved byte.
  - `0x004..0x007`: water tank temperature setpoint as XC16 `float`.
  - `0x008..0x00B`: superheat delta-T setpoint as XC16 `float`.
  - `0x00C..0x00F`: tank hysteresis as XC16 `float`.
  - `0x010..0x011`: EEV startup close steps as little-endian `uint16_t`.
  - `0x012..0x013`: EEV maximum open steps as little-endian `uint16_t`.
  - `0x014..0x015`: compressor anti-short-cycle time in seconds as little-endian `uint16_t`.
  - `0x016..0x017`: LP startup bypass time in seconds as little-endian `uint16_t`.
  - `0x018..0x019`: pump pre-run time in seconds as little-endian `uint16_t`.
  - `0x01A..0x01B`: pump post-run time in seconds as little-endian `uint16_t`.
  - `0x01C`: heater enabled flag, `0` disabled and nonzero enabled.
  - `0x01D`: reserved byte.
  - `0x01E`: CRC8 over bytes `0x000..0x01D`.
  - `0x1F0..0x1F3`: safe EEPROM test area exposed by `EEPROM24FC04_ADDR_TEST` / `EEPROM24FC04_TEST_LENGTH`.
- Default settings:
  - Water tank temperature: `55.0 C`.
  - Superheat delta-T / target superheat: `3.0 C`.
  - Tank hysteresis: `3.0 C`.
  - EEV startup close steps: `620`.
  - EEV maximum open steps: `560`.
  - Compressor anti-short-cycle time: `180` seconds.
  - LP startup bypass time: `60` seconds.
  - Pump pre-run time: `5` seconds.
  - Pump post-run time: `60` seconds.
  - Heater enabled: disabled.
- Startup behavior:
  - `main.c` calls `EEPROM24FC04_Init()` once after `LCD_I2C_Init(0U)`, so I2C1 is already configured.
  - `main.c` calls `EEPROM24FC04_LoadSettings()` once during startup.
  - If EEPROM data is missing/invalid but EEPROM is present, defaults are loaded into RAM and written once to EEPROM with a fresh CRC8.
  - If EEPROM is missing, defaults are loaded into RAM and firmware continues without freezing.
- Write handling:
  - Page size is 16 bytes.
  - `EEPROM24FC04_WriteData()` automatically splits writes so no page write crosses a 16-byte page boundary.
  - After every page write, the driver performs ACK polling with a timeout. Do not add infinite write-cycle waits.
  - The EEPROM write cycle can take up to about 5 ms; the module polls safely instead of blocking forever.
- Do not write EEPROM continuously in the main loop. Future button/menu code should call `EEPROM24FC04_SaveSettings()` only after the user changes a setting, or `EEPROM24FC04_RestoreDefaults()` only when defaults are explicitly requested.
- MPLAB Watch debug globals:
  - `g_eeprom_present`
  - `g_eeprom_error`
  - `g_eeprom_last_addr`
  - `g_eeprom_last_data`
  - `g_eeprom_write_count`
  - `g_eeprom_read_count`
  - `g_tank_temp_setpoint_c`
  - `g_superheat_delta_t_setpoint_c`
  - `g_tank_hysteresis_c`
  - `g_eev_startup_steps_setting`
  - `g_eev_max_steps_setting`
  - `g_compressor_anti_short_cycle_sec`
  - `g_lp_startup_bypass_sec`
  - `g_pump_pre_run_sec`
  - `g_pump_post_run_sec`
  - `g_heater_enabled_setting`
- Error codes:
  - `0`: no error.
  - `1`: no ACK / EEPROM not detected.
  - `2`: write timeout.
  - `3`: read timeout.
  - `4`: invalid address.
  - `5`: checksum invalid.
  - `6`: page boundary handling error.

## I2C LCD Behavior

- LCD hardware: HD44780-compatible 16x2 display with PCF8574 I2C backpack.
- I2C peripheral: I2C1 at 100 kHz.
- Pins: `RB9 = SDA1`, `RB8 = SCL1`.
- Pull-ups: external 4.7k resistors on SDA/SCL.
- Address handling: scans `0x27` first, then `0x3F`.
- Missing LCD behavior: initialization returns with `g_lcd_i2c_error = 5`, `g_lcd_i2c_state = 0xE005`, and later LCD calls return without hanging.
- Page order:
  - Page 0: SuCTS/Suction temperature on row 0, EvpTS/Evaporator temperature on row 1.
  - Page 1: TopDHWTS/DHWT top temperature on row 0, BotDHWTS/DHWT bottom temperature on row 1.
  - Page 2: EEV opening percentage on row 0, effective compensated DHWT tank setpoint on row 1.
- Fault override:
  - If `g_hp_active_fault` or `g_hp_latched_fault` contains HP, LP, Flow, or heat-pump software safety fault bits, the LCD stops rolling pages.
  - Fault display priority is HP first, LP second, Flow third, then software safety faults.
  - Fault messages are `HP Fault`, `LP Fault`, `FS Fault`, `HIGH TEMP Fault`, `FREEZE Fault`, `TANK TEMP Fault`, `EEV MIN Fault`, and sensor-specific fallback messages.
  - Sensor fault display uses row 0 `SENSOR Fault` and row 1 with the failed sensor name: `SHWTS Fault`, `EvpTS Fault`, `RHWTS Fault`, `TopDHWTS Fault`, `BotDHWTS Fault`, or `OST Fault`.
- Page timing: each page is held for about 1 second by holding two 500 ms main-loop refresh passes.
- The LCD page refresh does not call full clear each cycle; it rewrites line content and pads with spaces to reduce flicker.

## Coding Style Used In This Project

- C source uses Microchip XC16 register names and bitfields directly.
- Fixed-width integer types from `<stdint.h>` are used for ADC values and counters.
- Boolean support is included in some files with `<stdbool.h>`, though current logic mostly uses register bits and integer flags.
- Constants are uppercase macros with aligned names, e.g. `ADC_TIMEOUT_COUNTS`, `ADC_TIMEOUT_RESULT`.
- Module functions use clear prefixes, e.g. `NTC10K_...`.
- The fault module uses the `FaultInputs_...` prefix.
- Private helper functions in `main.c` are declared `static`.
- Debug/watch globals use `volatile` and a `g_` prefix.
- Comments are block-style and practical, describing hardware assumptions and register intent.
- Error paths use simple numeric debug codes rather than complex error frameworks.
- The current main loop is polling-based with a 500 ms delay/service period. Faults are non-blocking for ADC/LCD refresh.

## Rules For Future Changes

### General Project Rules

- Do not change working configuration bits unless explicitly asked.
- Do not change working ADC initialization unless explicitly asked.
- Do not rewrite the full project unless necessary.
- When modifying firmware code, update `AGENTS.md` with the new behavior/pin mapping/debug variables as needed, then commit and push the updated code to `https://github.com/fadiassi411/-Heat_Pump_DHWT`.
- Keep `main.c` clean; it should call functions from modules instead of accumulating full feature implementations.
- Keep generated MPLAB folders and files untouched unless the requested change requires project metadata/build integration.
- Preserve existing debug variables unless there is a requested reason to rename or remove them.
- Keep hardware assumptions visible in comments when adding or changing pin behavior.
- Keep fault handling centralized in `faults/fault_inputs.c/.h` where practical.

### Fault Handling Rules

- HP `RC5` is active-low. Do not invert this polarity unless the hardware wiring changes.
- LP `RC4` is active-low. Do not invert this polarity unless the hardware wiring changes.
- Flow Switch `RC10` is active-high. Do not invert this polarity unless the hardware wiring changes.
- Fault LED `RD8` should blink/toggle while `g_hp_active_fault` or `g_hp_latched_fault` is active.
- Raw Flow high before/during pump proving must not blink `RD8`; Flow blinks the LED only after the pump proving delay fails or after a later proven-flow fault.
- Faults must not freeze ADC sampling or LCD refresh. HP, LP, and Flow are status/alarm conditions in the current firmware, not blocking loop stops.
- NTC sensor open/short status must not stop the loop and must not light/blink `RD8` yet. Keep it as debug/status only in `g_sensor_fault_inputs` until sensor alarm handling is explicitly added later.
- Keep sensor open/short thresholds visible and easy to adjust in `faults/fault_inputs.c`.
- Current NTC open/short thresholds are `adc <= 5` for short and `adc >= 4090` for open.
- Preserve `g_fault_active`, `g_fault_inputs`, and `g_sensor_fault_inputs` as MPLAB watch/debug globals.

### Output Handling Rules

- Compressor output is `RB13` and is currently documented as active-high.
- Heater output is `RC1` and is currently documented as active-high.
- FAN output is `RC2` and is currently documented as active-high.
- Pump output is `RC6` and is currently documented as active-high.
- 4 Way Valve output is `RC3` and is currently documented as active-high.
- Do not invert output polarity unless the hardware wiring changes.
- Keep application output control centralized in `outputs.c/.h` where practical.
- Initialize controlled outputs off before enabling their pins as outputs.
- Preserve `g_output_states` and `g_output_init_done` as MPLAB Watch debug globals.

### EEV Handling Rules

- EEV coil outputs are `RB0`/A1, `RB1`/B1, `RD10`/A2, and `RC7`/B2.
- Keep the EEV driver modular in `eev.c/.h`.
- Use `EEV_Init()` once during startup.
- `EEV_Init()` must configure all EEV pins as digital outputs, keep all four outputs off during initialization, send 620 close pulses, then set position to 0 steps / 0%.
- Use 560 steps as 100% valve opening.
- Clamp tracked position to 0 through 560 steps; do not let the software counter underflow or exceed 560.
- Keep EEV timing inside the valve specification: 100 to 300 Hz pulse frequency and 2 to 5 ms pulse width. Current timing is 3 ms on / 3 ms off, about 166.7 Hz.
- Debug configuration: the project uses `#pragma config ICS = PGD2` because the board is wired to PGC2/PGD2.
- Normal programming can work on PGC2/PGD2 while debugging still fails if external hardware loads or drives the PGC2/PGD2 pins.
- `EEV_DISABLE_PGD2_PINS_DURING_DEBUG` in `eev.c` is set to `1` for PGD2 debug sessions, so `__DEBUG` builds skip EEV pin configuration and EEV startup movement.
- Production programming builds do not define `__DEBUG`, so the real EEV startup close behavior and EEV movement remain active when loading/programming without starting a debug session.
- If real EEV movement must be watched in a debug session, set `EEV_DISABLE_PGD2_PINS_DURING_DEBUG` to `0` only while accepting that MPLAB debug may fail if EEV hardware loads RB0/RB1/PGC2/PGD2.
- Production programming builds do not define `__DEBUG`, so the real EEV startup close behavior remains active.
- Available EEV API:
  - `EEV_Init()`
  - `EEV_AllOutputsOff()`
  - `EEV_StepOpen()`
  - `EEV_StepClose()`
  - `EEV_MoveStepsOpen(uint16_t steps)`
  - `EEV_MoveStepsClose(uint16_t steps)`
  - `EEV_MoveToPercent(uint8_t percent)`
  - `EEV_GetPositionSteps()`
  - `EEV_GetPositionPercent()`

### Heat Pump Control Rules

- Keep real DHWT heat pump control in `heatpump_control.c/.h`.
- Part 9 state list:
  - `HP_STATE_IDLE`
  - `HP_STATE_PRECHECK`
  - `HP_STATE_PUMP_PRE_RUN`
  - `HP_STATE_EEV_PREPOSITION`
  - `HP_STATE_COMPRESSOR_START`
  - `HP_STATE_RUNNING`
  - `HP_STATE_POST_RUN`
  - `HP_STATE_FAULT`
- `HeatPumpControl_Init()` is called once from `main.c` after EEPROM settings are loaded.
- `HeatPumpControl_Task()` is called once from the main polling loop after sensor fault debug bits are updated.
- `HP_ENABLE_COMPRESSOR_OUTPUT` controls whether compressor permission can energize the real compressor relay output on `RB13`; keep it `0` for safe bench logic testing and use `1` only for controlled relay/compressor testing.
- `HP_ENABLE_FAN_OUTPUT` is set to `1` in Part 9, allowing the fan output on `RC2` to turn on after EEV preposition and remain on during post-run.
- Part 9 keeps heater and 4 way valve safely off through the existing `Outputs_...` API.
- Part 9 allows the pump output on `RC6` to turn on during pump pre-run, EEV preposition, compressor-start timing, running, and pump/fan post-run.
- Part 9 moves the EEV to a startup preposition, turns the fan on in `HP_STATE_COMPRESSOR_START`, waits the compressor anti-short-cycle setting, then sets compressor permission and transitions to `HP_STATE_RUNNING`.
- Part 9 adds slow closed-loop EEV superheat control from `SuCTS - EvpTS`; it does not add pressure-based refrigerant tables yet.
- Part 9 limits all heat-pump running EEV movement to `HP_EEV_CONTROL_MIN_PERCENT = 10` through `HP_EEV_CONTROL_MAX_PERCENT = 40`, equal to 56 through 224 steps of the 560-step valve stroke.
- Part 9 allowed transitions:
  - `IDLE` to `PRECHECK` when heating is requested.
  - `PRECHECK` back to `IDLE` when heating is no longer requested.
  - `PRECHECK` to `PUMP_PRE_RUN` when heating is requested and HP/LP faults are clear. Flow may still be high here because the pump is not running yet.
  - `PUMP_PRE_RUN` to `EEV_PREPOSITION` after `g_pump_pre_run_sec` expires only if Flow has become low.
  - `PUMP_PRE_RUN` to `FAULT` if Flow remains high after `g_pump_pre_run_sec` expires.
  - `EEV_PREPOSITION` to `COMPRESSOR_START` after the EEV has moved to the startup open position.
  - `COMPRESSOR_START` to `RUNNING` after `g_compressor_anti_short_cycle_sec` expires.
  - `RUNNING` to `POST_RUN` when the heating request is satisfied.
  - `POST_RUN` to `IDLE` after `g_pump_post_run_sec` expires.
  - Any state to `FAULT` when HP, LP, or Flow status is active.
  - `FAULT` back to `IDLE` when the active fault clears and the reset-only lockout is not active.
  - `FAULT` remains latched permanently when `g_hp_fault_lockout = 1`; MCU reset or power cycle is required to clear it.
- `HP_STATE_IDLE` commands compressor, heater, fan, pump, and 4 way valve off.
- `HP_STATE_PRECHECK` checks heating request and HP/LP faults with the existing bench sensor-fault bypass behavior. Flow is ignored in precheck because the flow switch is expected to be high before the pump runs.
- `HP_STATE_PUMP_PRE_RUN` commands the real pump output on using `Outputs_SetPump()` through the command-apply helper, while compressor, heater, fan, and 4 way valve remain off.
- During `HP_STATE_PUMP_PRE_RUN`, `g_hp_flow_proving_active = 1`; raw Flow status still appears in `g_fault_inputs`, but it is ignored until the pump pre-run/proving time expires.
- At the end of pump pre-run, Flow must be low. If `RC10` is still high, the control module stops the pump, latches `FAULT_INPUT_FLOW_MASK` in `g_hp_latched_fault`, and moves to `HP_STATE_FAULT`.
- `HP_STATE_EEV_PREPOSITION` keeps the pump on and moves the EEV once to the Part 9 startup open position.
- Part 9 startup EEV open position is fixed at 25%, which equals 140 steps from the 560-step full stroke and is below the 40% heat-pump control ceiling.
- The existing EEPROM setting `g_eev_startup_steps_setting` is still the startup close count, default 620, not a startup open position.
- TODO: add a separate EEPROM setting for EEV startup open percent/steps before tuning real compressor operation.
- After EEV preposition completes, set `g_hp_eev_preposition_done = 1` and transition to `HP_STATE_COMPRESSOR_START`.
- The control module may only mark EEV preposition done after `g_eev_init_done == 1` and `EEV_GetPositionSteps()` equals `g_hp_eev_target_steps`.
- `HP_STATE_COMPRESSOR_START` is a timed holding state in Part 9. It keeps pump on, turns fan on through `HP_ENABLE_FAN_OUTPUT = 1`, keeps heater/4 way valve off, and counts down `g_hp_anti_short_cycle_remaining_sec`.
- When the anti-short-cycle countdown reaches zero, `g_hp_compressor_start_ready` becomes `1`, `g_hp_compressor_allowed` becomes `1`, and the state moves to `HP_STATE_RUNNING`. `HP_ENABLE_COMPRESSOR_OUTPUT` decides whether that permission energizes `RB13`.
- `HP_STATE_RUNNING` keeps pump and fan on, keeps heater and 4 way valve off, keeps compressor permission active, observes the minimum compressor run timer, and runs slow EEV superheat control.
- `HP_STATE_POST_RUN` turns compressor permission off first, keeps both pump and fan on for `g_pump_post_run_sec`, then turns pump/fan off and returns to `HP_STATE_IDLE`.
- Fan starts only after EEV preposition is complete. If fan bench testing is not wanted, set `HP_ENABLE_FAN_OUTPUT` to `0`.
- Pump pre-run timing is non-blocking and uses `g_hp_state_elapsed_ms` and `g_hp_pump_pre_run_remaining_sec`.
- Pump/fan post-run timing is non-blocking and uses `g_hp_state_elapsed_ms`, `g_pump_post_run_sec`, and `g_hp_pump_post_run_remaining_sec`.
- Compressor-start timing is non-blocking and uses `g_hp_state_elapsed_ms`, `g_compressor_anti_short_cycle_sec`, and `g_hp_anti_short_cycle_remaining_sec`.
- Minimum compressor run timing is non-blocking and uses `HP_MIN_COMPRESSOR_RUN_SEC`, default `120` seconds, and `g_hp_min_run_remaining_sec`. If the tank reaches setpoint before this timer expires, compressor permission remains active until the minimum run timer reaches zero.
- LP startup bypass timing is exposed through `g_hp_lp_bypass_remaining_sec` and uses `g_lp_startup_bypass_sec` while in compressor-start/running states. HP and Flow faults still stop the state machine.
- EEV running superheat control in Part 9:
  - Uses `g_sucts_temperature_c` as suction temperature and `g_evpts_temperature_c` as evaporator temperature.
  - Calculates `g_hp_superheat_delta_t_c = SuCTS - EvpTS`.
  - Uses `g_superheat_delta_t_setpoint_c` as `g_hp_superheat_target_c`.
  - If superheat is above target by more than `HP_SUPERHEAT_DEADBAND_C`, default `0.5 C`, it opens the EEV by 1 step.
  - If superheat is below target by more than `HP_SUPERHEAT_DEADBAND_C`, default `0.5 C`, it closes the EEV by 1 step.
  - EEV movement is limited to one step every `HP_EEV_CONTROL_INTERVAL_MS`, default `5000 ms`.
  - Heat-pump running control opening is limited by `HP_EEV_CONTROL_MIN_PERCENT`, default `10%`, through `HP_EEV_CONTROL_MAX_PERCENT`, default `40%`.
  - `g_hp_eev_control_min_percent` exposes the 10% minimum running opening.
  - `g_hp_eev_control_min_steps` is 56 steps when the minimum is 10%.
  - `g_hp_eev_control_max_steps` is 224 steps when the maximum is 40%.
  - If the valve is already below the heat-pump control minimum, running control opens it by one step per control interval until it is back at or above the limit.
  - If the valve is already above the heat-pump control maximum, running control closes it by one step per control interval until it is back at or below the limit.
  - `g_hp_eev_control_limit_active` is `1` while the controller is at or above the heat-pump EEV opening ceiling.
  - `g_hp_eev_control_min_limit_active` is `1` while the controller is at or below the heat-pump EEV opening floor.
  - The existing EEV driver still clamps the absolute mechanical position from 0 to 560 steps.
  - If SuCTS or EvpTS is invalid, or if `g_eev_init_done = 0`, EEV running control is disabled and does not move the valve.
  - No pressure-based refrigerant saturation table is implemented yet.
- Fault retry behavior in Part 9:
  - `HP_FAULT_AUTO_RETRY_LIMIT` is `3`.
  - `g_hp_fault_retry_limit` mirrors that limit for MPLAB Watch.
  - The first three fault events may auto-restart after the fault clears.
  - A later fault sets `g_hp_fault_lockout = 1`, keeps all outputs off, keeps the state in `HP_STATE_FAULT`, and requires MCU reset or power cycle to clear.
  - `g_hp_fault_retry_count` increments once per fault event, not once per polling loop.
  - `g_hp_fault_retry_pending` remains `1` while the current fault is active and returns to `0` after the fault clears and the state leaves `HP_STATE_FAULT`.
  - `g_hp_last_fault` stores the most recent heat-pump fault bits for Watch/debug.
  - Retry/lockout counters are RAM-only and are not written to EEPROM.
- The fault LED in `main.c` blinks from `g_hp_active_fault` or `g_hp_latched_fault`, not directly from raw `g_fault_active`, so the normal pre-pump Flow high condition does not blink the LED.
- `HP_IGNORE_SENSOR_FAULTS_FOR_BENCH_TEST` in `heatpump_control.c` controls whether NTC sensor faults block the heat pump control state machine.
- Current real-test setting: `HP_IGNORE_SENSOR_FAULTS_FOR_BENCH_TEST = 0`, so NTC sensor faults block the heat pump control state machine.
- Warning: `HP_IGNORE_SENSOR_FAULTS_FOR_BENCH_TEST = 1` is only for bench testing with unplugged sensors. `g_sensor_fault_inputs` must still show real sensor bits, for example `0x003F`, but those bits do not set `g_hp_active_fault`.
- Before real compressor testing, set `HP_IGNORE_SENSOR_FAULTS_FOR_BENCH_TEST` to `0` so NTC sensor faults again block the heat pump control state machine.
- Real-test compressor safety:
  - SHWTS is treated as Supply Hot Water Temperature.
  - RHWTS is treated as Return Hot Water Temperature.
  - SHWTS or RHWTS at 75 C or higher during compressor-start/running sets `HP_CONTROL_FAULT_HIGH_TEMP_MASK`.
  - EvpTS at 2 C or lower during compressor-start/running sets `HP_CONTROL_FAULT_FREEZE_MASK`.
  - TopDHWTS or BotDHWTS at 75 C or higher sets `HP_CONTROL_FAULT_TANK_OVERTEMP_MASK`.
  - EEV below the 10% minimum running opening while in `HP_STATE_RUNNING` sets `HP_CONTROL_FAULT_EEV_MIN_OPEN_MASK`.
  - Any of these software safety faults stops compressor permission immediately and enters `HP_STATE_FAULT` through the existing retry/lockout logic.
- Compressor minimum-off protection:
  - `g_hp_compressor_min_off_remaining_sec` counts down after every compressor stop.
  - The countdown uses the EEPROM-backed `g_compressor_anti_short_cycle_sec`, default 180 seconds.
  - `HP_STATE_COMPRESSOR_START` will not allow compressor permission until both `g_hp_anti_short_cycle_remaining_sec` and `g_hp_compressor_min_off_remaining_sec` are zero.
- DHWT demand uses existing NTC globals from `ntc_loops.c/.h`:
  - `g_topdhwts_temperature_c` / `g_topdhwts_adc_raw` for `TopDHWTS`.
  - `g_botdhwts_temperature_c` / `g_botdhwts_adc_raw` for `BotDHWTS`.
- OST setpoint compensation uses `g_ost_temperature_c` / `g_ost_adc_raw` for `OST`.
- Compensated DHWT setpoint:
  - `70 C` when `OST <= 5 C`.
  - `50 C` when `OST >= 20 C`.
  - Linear interpolation between 5 C and 20 C.
  - Fall back to `g_tank_temp_setpoint_c` if OST is invalid.
- Heating start condition: set `g_hp_heating_request = 1` when `BotDHWTS <= g_hp_dhwt_compensated_setpoint_c - g_tank_hysteresis_c`.
- Heating stop condition: keep request latched on until `TopDHWTS >= g_hp_dhwt_compensated_setpoint_c`, then set `g_hp_heating_request = 0`.
- If either TopDHWTS or BotDHWTS is invalid by existing open/short thresholds, set the related valid flag to `0`, force `g_hp_heating_request = 0`, and keep the state machine safely in `HP_STATE_IDLE`.
- `HP_IGNORE_SENSOR_FAULTS_FOR_BENCH_TEST = 0` is the current real-test setting. Setting it to `1` may ignore sensor faults for bench FAULT blocking only; it must not fake valid tank temperatures.
- Use EEPROM-backed settings globals for tank setpoint, tank hysteresis, pump pre-run time, pump post-run time, compressor anti-short-cycle time, and LP startup bypass time.
- Do not use long blocking delays in the control module.

### Adding New Loops

- Add a new loop in a modular way using a new `.c` / `.h` pair if possible.
- Prefer a small module API such as:
  - `Module_Init()` for one-time setup.
  - `Module_Task()` or `Module_Update()` for repeated polling work.
  - Optional getters for debug/calculated values.
- Call the module task from `main()` instead of placing the loop body directly in `main.c`.
- Avoid long blocking delays inside new module tasks. Keep ADC/LCD refresh alive even during active faults.
- If timing matters, use `APP_TASK_PERIOD_MS` or another clearly named period macro rather than hidden literal delays.
- Keep state variables module-local where possible. Expose only the debug globals that are useful in MPLAB watch windows.

### Adding New Sensors

- Add each new sensor as its own module where practical, for example `pressure_sensor.c/.h` or `ntc_bank.c/.h`.
- Keep pin setup, ADC channel selection, conversion math, and public API together in that sensor module where possible.
- Document the exact hardware wiring, ADC channel, expected voltage range, and units.
- Reuse existing configuration constants from `app_config.h` where suitable.
- If adding ADC channels, do not disturb the working AN0/Core 0 setup unless required and verified.
- Use timeouts for ADC waits; do not create infinite waits on hardware flags.
- Add new debug globals only when they help diagnose live hardware behavior.
- Name debug globals with the `g_` prefix and mark them `volatile` if they are intended for watch windows.

### ADC/NTC Cautions

- Keep the current AN0 test path working unless specifically asked to change it.
- Confirm real divider orientation before changing `NTC10K_ADCToResistance()`.
- Keep ADC timeout handling visible and simple.
- Avoid mixing old ADC results with new conversions; preserve the existing pattern of draining the relevant `ADCBUFx`, starting `CNVRTCH`, waiting for `CNVRTCH` to clear, and then waiting for the channel ready flag unless a better verified ADC flow is introduced.

## Current Build/Debug Notes

- MPLAB X default configuration builds source files separately (`combine-sourcefiles=false`).
- Warnings are enabled (`enable-all-warnings=true`).
- Optimization level is `0`, suitable for debugging.
- Debug symbols are enabled (`enable-symbols=true`).
- Linker map file generation is configured.
- Programmer/debug voltage setting in metadata is `3.25` V.
