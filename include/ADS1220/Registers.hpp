#pragma once
#ifndef ARDUINO_ADS1220_REGISTERS_H
#define ARDUINO_ADS1220_REGISTERS_H

#include <Arduino.h>

namespace ads1220 {

namespace config {

enum RegisterAddress
{
    REG0_ADDR = 0x00,
    REG1_ADDR = 0x01,
    REG2_ADDR = 0x02,
    REG3_ADDR = 0x03,
};

// ========== Register 0 ==========

enum Reg0Masks : uint8_t
{
    PGA_BYPASS = 0x00000001,
    GAIN       = 0b00001110,
    MUX        = 0b11110000,
};

namespace pga_bypass {

enum PgaBypass : uint8_t
{
    ENABLE  = 0b00000000, // default
    DISABLE = 0b00000001,
};

} // namespace pga_bypass

namespace gain {

enum Gain : uint8_t
{
    GAIN_1   = 0b00000000, // default
    GAIN_2   = 0b00000010,
    GAIN_4   = 0b00000100,
    GAIN_8   = 0b00000110,
    GAIN_16  = 0b00001000,
    GAIN_32  = 0b00001010,
    GAIN_64  = 0b00001100,
    GAIN_128 = 0b00001110,
};

} // namespace gain

namespace mux {

enum Mux : uint8_t
{
    AIN0_AIN1                 = 0b00000000, // default
    AIN0_AIN2                 = 0b00010000,
    AIN0_AIN3                 = 0b00100000,
    AIN1_AIN2                 = 0b00110000,
    AIN1_AIN3                 = 0b01000000,
    AIN2_AIN3                 = 0b01010000,
    AIN1_AIN0                 = 0b01100000,
    AIN3_AIN2                 = 0b01110000,
    AIN0_AVSS                 = 0b10000000,
    AIN1_AVSS                 = 0b10010000,
    AIN2_AVSS                 = 0b10100000,
    AIN3_AVSS                 = 0b10110000,
    REFPX_REFNX_DIV_4_MONITOR = 0b11000000,
    AVDD_AVSS_DIV_4_MONITOR   = 0b11010000,
    AVDD_AVSS_DIV_2_SHORTED   = 0b11100000,
    RESERVED                  = 0b11110000,
};

enum SingleEnded : uint8_t
{
    SINGLE_ENDED_AIN0 = AIN0_AVSS,
    SINGLE_ENDED_AIN1 = AIN1_AVSS,
    SINGLE_ENDED_AIN2 = AIN2_AVSS,
    SINGLE_ENDED_AIN3 = AIN3_AVSS,
};

} // namespace mux

// ========== Register 1 ==========

enum Reg1Masks : uint8_t
{
    BCS  = 0b00000000,
    TS   = 0b00000010,
    CM   = 0b00000100,
    MODE = 0b00011000,
    DR   = 0b11100000,
};

namespace burn_out_current_sources {

enum Bcs : uint8_t
{
    OFF = 0b00000000, // default
    ON  = 0b00000001,
};

} // namespace burn_out_current_sources

namespace temperature_sensor {

enum Ts : uint8_t
{
    DISABLE = 0b00000000, // default
    ENABLE  = 0b00000010,
};

} // namespace temperature_sensor

namespace conversion_mode {

enum Cm : uint8_t
{
    SINGLE_SHOT = 0b00000000, // default
    CONTINUOUS  = 0b00000100,
};

} // namespace conversion_mode

namespace mode {

enum Mode : uint8_t
{
    NORMAL     = 0b00000000, // default
    DUTY_CYCLE = 0b00001000,
    TURBO      = 0b00010000,
    RESERVED   = 0b00011000,
};

} // namespace mode

namespace data_rate {

enum DrNormal : uint8_t
{
    NORMAL_20_SPS   = 0b00000000, // default
    NORMAL_45_SPS   = 0b00100000,
    NORMAL_90_SPS   = 0b01000000,
    NORMAL_17_5SPS  = 0b01100000,
    NORMAL_330_SPS  = 0b10000000,
    NORMAL_600_SPS  = 0b10100000,
    NORMAL_1000_SPS = 0b11000000,
    NORMAL_RESERVED = 0b11100000,
};

enum DrDutyCycle : uint8_t
{
    DUTY_CYCLE_5_SPS     = 0b00000000, // default
    DUTY_CYCLE_11_25_SPS = 0b00100000,
    DUTY_CYCLE_22_5_SPS  = 0b01000000,
    DUTY_CYCLE_44_SPS    = 0b01100000,
    DUTY_CYCLE_82_5_SPS  = 0b10000000,
    DUTY_CYCLE_150_SPS   = 0b10100000,
    DUTY_CYCLE_250_SPS   = 0b11000000,
    DUTY_CYCLE_RESERVED  = 0b11100000,
};

enum DrTurbo : uint8_t
{
    TURBO_40_SPS   = 0b00000000, // default
    TURBO_90_SPS   = 0b00100000,
    TURBO_180_SPS  = 0b01000000,
    TURBO_350_SPS  = 0b01100000,
    TURBO_660_SPS  = 0b10000000,
    TURBO_1200_SPS = 0b10100000,
    TURBO_2000_SPS = 0b11000000,
    TURBO_RESERVED = 0b11100000,
};

} // namespace data_rate

// ========== Register 2 ==========

enum Reg2Masks : uint8_t
{
    IDAC      = 0b00000111,
    PSW       = 0b00001000,
    FIR_50_60 = 0b00110000,
    VREF      = 0b11000000,
};

namespace idac {

enum Idac : uint8_t
{
    IDAC_OFF     = 0b00000000, // default
    IDAC_10_UA   = 0b00000001,
    IDAC_50_UA   = 0b00000010,
    IDAC_100_UA  = 0b00000011,
    IDAC_250_UA  = 0b00000100,
    IDAC_500_UA  = 0b00000101,
    IDAC_1000_UA = 0b00000110,
    IDAC_1500_UA = 0b00000111,
};

} // namespace idac

namespace low_side_power_switch {

enum Psw : uint8_t
{
    ALWAYS_OPEN = 0b00000000, // default
    AUTO_CLOSE  = 0b00001000,
};

} // namespace psw

namespace fir_50_60 {

enum Fir5060 : uint8_t
{
    FIR_OFF       = 0b00000000, // default
    FIR_50HZ_60HZ = 0b00010000,
    FIR_50HZ      = 0b00100000,
    FIR_60HZ      = 0b00110000,
};

} // namespace fir_50_60

namespace vref {

enum Vref : uint8_t
{
    INTERNAL_2_048_V     = 0b00000000, // default
    EXTERNAL_REFP0_REFN0 = 0b01000000,
    EXTERNAL_AIN0_AIN3   = 0b10000000,
    ANALOG_AVDD_AVSS     = 0b11000000,
};

} // namespace vref

// ========== Register 3 ==========

enum Reg3Masks : uint8_t
{
    RESERVED = 0b00000000,
    DRDYM    = 0b00000010,
    I2MUX    = 0b00011100,
    I1MUX    = 0b11100000,
};

namespace drdy_mode {

enum DrdyMode : uint8_t
{
    DRDY_ONLY     = 0b00000000, // default
    DRDY_AND_DOUT = 0b00000010,
};

} // namespace drdy_mode

namespace i2mux {

enum I2Mux : uint8_t
{
    DISABLE  = 0b00000000, // default
    AIN0     = 0b00000100,
    AIN1     = 0b00001000,
    AIN2     = 0b00001100,
    AIN3     = 0b00010000,
    REFP0    = 0b00010100,
    REFN0    = 0b00011000,
    RESERVED = 0b00011100,
};

} // namespace i2mux

namespace i1mux {

enum I1Mux : uint8_t
{
    DISABLE  = 0b00000000, // default
    AIN0     = 0b00100000,
    AIN1     = 0b01000000,
    AIN2     = 0b01100000,
    AIN3     = 0b10000000,
    REFP0    = 0b10100000,
    REFN0    = 0b11000000,
    RESERVED = 0b11100000,
};

} // namespace i1mux

} // namespace config

} // namespace ads1220

#endif // ARDUINO_ADS1220_REGISTERS_H
