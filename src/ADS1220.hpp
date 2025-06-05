#pragma once
#ifndef ARDUINO_ADS1220_H
#define ARDUINO_ADS1220_H

#include "ADS1220/Commands.hpp"
#include "ADS1220/Registers.hpp"

#include <Arduino.h>
#include <SPI.h>
#ifndef ARDUINO_ARCH_AVR
#include <functional>
#include <cassert>
#endif

namespace ads1220 {

/// Reference:
/// TI Application Note: A Basic Guide to RTD Measurements
/// https://www.ti.com/lit/an/sbaa275a/sbaa275a.pdf

/// @brief Calcurate 2-wired RTD resistance from read data
/// @param r_ref_ohms Reference resistance [ohms]
/// @param gain Gain value (1, 2, 4, 8, 16, 32, 64, 128)
/// @param data Read data using ADS1220::read()
/// @return RTD resistance [ohms]
/// @ref R_RTD = R_REF * Output Code / (2^23 • Gain)
///      2.1 Two-Wire RTD Measurement With Low-Side Reference - eq (18)
///      2.2 Two-Wire RTD Measurement With High-Side Reference - eq (23)
///      2.12 Universal RTD Measurement Interface With Low-Side Reference
///      2.12.4.1 Two-Wire Measurement - eq (94)
///      2.13 Universal RTD Measurement Interface With High-Side Reference
///      2.13.4.1 Two-Wire Measurement - eq (108)
inline constexpr float get_two_wired_rtd_ohms_from_read_data(float r_ref_ohms, float gain, int32_t data)
{
    return r_ref_ohms * static_cast<float>(data) / (static_cast<float>(1 << 23) * gain);
}

/// @brief Calcurate 3-wired RTD resistance (with Low-Side Reference) from read data
/// @param r_ref_ohms Reference resistance [ohms]
/// @param gain Gain value (1, 2, 4, 8, 16, 32, 64, 128)
/// @param data Read data using ADS1220::read()
/// @return RTD resistance [ohms]
/// @ref R_RTD = R_REF • Output code / (2^22 • Gain)
///      2.3 Three-Wire RTD Measurement, Low-Side Reference - eq (32)
inline constexpr float get_three_wired_rtd_ohms_low_side_ref_from_read_data(float r_ref_ohms, float gain, int32_t data)
{
    return r_ref_ohms * static_cast<float>(data) / (static_cast<float>(1 << 22) * gain);
}

/// @brief Calcurate 3-wired RTD resistance (generic) from read data
/// @param r_ref_ohms Reference resistance [ohms]
/// @param gain Gain value (1, 2, 4, 8, 16, 32, 64, 128)
/// @param data Read data using ADS1220::read()
/// @return RTD resistance [ohms]
/// @ref R_RTD = R_REF • (Output code 1 – Output code 2) / (2^23 • Gain)
///      2.4 Three-Wire RTD Measurement, Low-Side Reference, One IDAC Current Source - eq (54)
///      2.12 Universal RTD Measurement Interface With Low-Side Reference
///      2.12.4.2 Three-Wire Measurement - eq (98)
///      2.13 Universal RTD Measurement Interface With High-Side Reference
///      2.13.4.2 Three-Wire Measurement - eq (112)
inline constexpr float get_three_wired_rtd_ohms_from_read_data(float r_ref_ohms, float gain, int32_t data1, int32_t data2)
{
    return r_ref_ohms * (static_cast<float>(data1) - static_cast<float>(data2)) / (static_cast<float>(1 << 23) * gain);
}

/// @brief Calcurate 3-wired RTD resistance (with High-Side Reference) from read data
/// @param r_ref_ohms Reference resistance [ohms]
/// @param gain Gain value (1, 2, 4, 8, 16, 32, 64, 128)
/// @param data Read data using ADS1220::read()
/// @return RTD resistance [ohms]
/// @ref R_RTD = R_REF • Output code / (2^23 • Gain)
///      2.5 Three-Wire RTD Measurement, High-Side Reference - eq (64)
inline constexpr float get_three_wired_rtd_ohms_high_side_ref_from_read_data(float r_ref_ohms, float gain, int32_t data)
{
    return r_ref_ohms * static_cast<float>(data) / (static_cast<float>(1 << 23) * gain);
}

/// @brief Calcurate 4-wired RTD resistance from read data
/// @param r_ref_ohms Reference resistance [ohms]
/// @param gain Gain value (1, 2, 4, 8, 16, 32, 64, 128)
/// @param data Read data using ADS1220::read()
/// @return RTD resistance [ohms]
/// @ref R_RTD = R_REF • Output code / (2^23 • Gain)
/// 2.6 Four-Wire RTD Measurement, Low-Side Reference - eq (68)
/// 2.12 Universal RTD Measurement Interface With Low-Side Reference
/// 2.12.4.3 Four-Wire Measurement - eq (100)
/// 2.13 Universal RTD Measurement Interface With High-Side Reference
/// 2.13.4.3 Four-Wire Measurement - eq (114)
inline constexpr float get_four_wired_rtd_ohms_from_read_data(float r_ref_ohms, float gain, int32_t data)
{
    return r_ref_ohms * static_cast<float>(data) / (static_cast<float>(1 << 23) * gain);
}

/// Constants for PT100 RTD
namespace {
constexpr float R_RTD_0_DEGREES_CELSIUS = 100.0f; // 100 ohms at 0 degrees celsius
constexpr float TEMPERATURE_COEFFICIENT = 0.003851f; // 0.003851 ohms per ohm per degree celsius
} // namespace

/// @brief Calcurate linear approximation of temperature from RTD resistance (PT100)
/// @param r_rtd_ohms RTD resistance [ohms]
/// @return Approximated temperature [degree celsius]
/// @ref https://blog.beamex.com/pt100-temperature-sensor
inline constexpr float get_temperature_from_rtd_ohms_with_linear_approx(float r_rtd_ohms)
{
    return (r_rtd_ohms - R_RTD_0_DEGREES_CELSIUS) / (TEMPERATURE_COEFFICIENT * R_RTD_0_DEGREES_CELSIUS);
}

namespace helper {

/// @brief Convert read internal temperature sensor data to temperature in deg C
/// @param data_14bit Read 14-bit internal temperature sensor
/// @return Temperature in deg C
/// @note One 14-bit LSB equals 0.03125 [deg C]
inline float get_temperature_from_internal_temperature_sensor_data(int32_t data_14bit)
{
    return static_cast<float>(data_14bit) * 0.03125f;
}

/// @brief Convert right-justified 24-bit twos complement data to 32-bit signed integer
/// @param data_24bit Read righ-justified 24-bit adc data
/// @return 32-bit signed integer data
inline int32_t get_right_justified_24bit_twos_complement_to_int32(uint32_t data_24bit)
{
    return (data_24bit & (1 << 23)) ? (data_24bit | 0xFF000000) : data_24bit;
}

/// @brief Convert right-justified 14-bit twos complement data to 32-bit signed integer
/// @param data_14bit Read righ-justified 14-bit adc data
/// @return 32-bit signed integer data
inline int32_t get_right_justified_14bit_twos_complement_to_int32(uint32_t data_14bit)
{
    return (data_14bit & (1 << 13)) ? (data_14bit | 0xFFFFC000) : data_14bit;
}

} // namespace helper

class ADS1220
{
#ifdef ARDUINO_ARCH_AVR
    using fn_cs_t = void(*)(uint8_t);
    using fn_drdy_t = int(*)(void);
#else
    using fn_cs_t = std::function<void(uint8_t)>;
    using fn_drdy_t = std::function<int(void)>;
#endif

    SPIClass *spi;
    SPISettings spi_settings;
    uint8_t pin_cs;
    uint8_t pin_drdy;
    fn_cs_t fn_cs;
    fn_drdy_t fn_drdy;

    uint8_t config_regs[4];
    bool continuous_converting;

public:
    ADS1220()
    : spi(nullptr)
    , spi_settings()
    , pin_cs(0xFF)
    , pin_drdy(0xFF)
    , fn_cs(nullptr)
    , fn_drdy(nullptr)
    , config_regs {0, 0, 0, 0}
    , continuous_converting(false)
    {
    }

    /// @brief Initialize ADS1220 with CS/DRDY pins
    /// @param spi SPI instance
    /// @param pin_cs CS/SS pin
    /// @param pin_drdy DRDY pin
    /// @param spi_settings SPISettings
    /// @note Call SPI.begin() before calling this function
    void begin(SPIClass &spi, uint8_t pin_cs, uint8_t pin_drdy, const SPISettings &spi_settings)
    {
        this->spi = &spi;
        this->spi_settings = spi_settings;
        this->pin_cs = pin_cs;
        this->pin_drdy = pin_drdy;
        pinMode(this->pin_cs, OUTPUT);
        pinMode(this->pin_drdy, INPUT);
        this->begin_impl();
    }

    /// @brief Initialize ADS1220 with CS/DRDY user-defined functions
    /// @param spi SPI instance
    /// @param fn_cs The user-defined function to write CS/SS pin
    /// @param fn_drdy The user-defined function to read DRDY pin
    /// @param spi_settings SPISettings
    /// @note Call SPI.begin() before calling this function
    void begin(SPIClass &spi, fn_cs_t fn_cs, fn_drdy_t fn_drdy, const SPISettings &spi_settings)
    {
        this->spi = &spi;
        this->spi_settings = spi_settings;
        this->fn_cs = fn_cs;
        this->fn_drdy = fn_drdy;
        this->begin_impl();
    }

    // ==================== Read ADC Data ====================

    /// @brief Read 24-bit twos complement ADC data as int32_t with timeout
    /// @param timeout_ms Timeout in milliseconds
    /// @return ADC data as int32_t
    int32_t read(uint32_t timeout_ms = 60)
    {
        uint32_t right_justified_24bit = this->read_raw(timeout_ms);
        return helper::get_right_justified_24bit_twos_complement_to_int32(right_justified_24bit);
    }

    // ==================== Single-Ended Input Selection ====================

    /// @brief Select single-ended input by writing register MUX
    /// @param value ads1220::config::mux::SingleEnded
    void select_single_ended_input(config::mux::SingleEnded value)
    {
        this->write_config_input_multiplexer(static_cast<config::mux::Mux>(value));
    }

    // ==================== Begin/End Continuous Conversion ====================

    /// @brief Begin continuous conversion mode
    ///        - write ads1220::config::conversion_mode::CONTINUOUS to register MODE
    ///        - drive CS pin LOW
    ///        - begin SPI transaction
    ///        - send START/SYNC command
    ///        - wait for DRDY to go HIGH (first conversion)
    /// @note This function drives CS pin LOW until end_continuous() is called
    void begin_continuous()
    {
        if (!this->is_continuous_conversion_mode()) {
            this->write_config_conversion_mode(config::conversion_mode::CONTINUOUS);
        }
        this->spi->beginTransaction(this->spi_settings);
        this->write_cs_pin(LOW);
        this->send_command_without_cs(commands::START_SYNC, 0, nullptr, nullptr, 0);
        delay(1); // Wait for DRDY to go HIGH for safety
        this->wait_drdy(60);
        this->continuous_converting = true;
    }

    /// @brief End continuous conversion mode
    ///        - drive CS pin HIGH
    ///        - end SPI transaction
    void end_continuous()
    {
        this->write_cs_pin(HIGH);
        this->spi->endTransaction();
        this->continuous_converting = false;
    }

    /// @brief Check if continuous conversion mode is enabled
    /// @return true if continuous conversion mode is enabled, false otherwise
    bool is_continuous_conversion_mode() const
    {
        return (config_regs[1] & config::CM) == config::conversion_mode::CONTINUOUS;
    }

    /// @brief Check if continuous conversion is in progress
    /// @return true if continuous conversion is in progress, false otherwise
    bool is_continuous_converting() const
    {
        return this->is_continuous_conversion_mode() && this->continuous_converting;
    }

    // ==================== Internal Temperature Sensor ====================

    /// @brief Begin internal temperature sensor mode
    /// @note This function disables external adc readings
    void begin_internal_temperature_sensor_mode()
    {
        this->write_config_temperature_sensor(config::temperature_sensor::ENABLE);
    }

    /// @brief Read 14-bit twos complement internal temperature sensor data and convert to temperature in deg C
    /// @param timeout_ms Timeout in milliseconds
    /// @return Temperature in deg C
    float read_internal_temperature(uint32_t timeout_ms = 60)
    {
        uint32_t right_justified_14bit = this->read_raw(timeout_ms) >> 10;
        int32_t read_data = helper::get_right_justified_14bit_twos_complement_to_int32(right_justified_14bit);
        return helper::get_temperature_from_internal_temperature_sensor_data(read_data);
    }

    /// @brief End internal temperature sensor mode
    void end_internal_temperature_sensor_mode()
    {
        this->write_config_temperature_sensor(config::temperature_sensor::DISABLE);
    }

    /// @brief Check if internal temperature sensor mode is enabled
    /// @return true if internal temperature sensor mode is enabled, false otherwise
    bool is_internal_temperature_sensor_mode() const
    {
        return (config_regs[1] & config::TS) == config::temperature_sensor::ENABLE;
    }

    // ==================== Command ====================

    /// @brief Send RESET command
    /// @return true if DRDY goes HIGH (ADS1220 wakeup) after the command, false otherwise
    bool reset()
    {
        this->send_command(commands::RESET, 0, nullptr, nullptr, 0);
        delay(1); // Wait for power-up (DRDY goes HIGH)

        if (!this->wait_drdy(60)) {
            Serial.println("Failed to detect power-up of ADS1220");
            return false;
        }
        return true;
    }

    /// @brief Send START/SYNC command
    void start_sync()
    {
        this->send_command(commands::START_SYNC, 0, nullptr, nullptr, 0);
    }

    /// @brief Send POWERDOWN command
    void power_down()
    {
        this->send_command(commands::POWERDOWN, 0, nullptr, nullptr, 0);
    }

    /// @brief Send RDATA command
    /// @return Read 24-bit right-justified twos complement raw data
    uint32_t rdata()
    {
        this->spi->beginTransaction(this->spi_settings);
        this->write_cs_pin(LOW); // Keep CS low until reading data is complete
        this->send_command_without_cs(commands::RDATA, 0, nullptr, nullptr, 0);
        delay(1); // Wait for DRDY to go HIGH for safety
        uint32_t right_justified_24bit = this->read_raw_impl();
        this->write_cs_pin(HIGH);
        this->spi->endTransaction();
        return right_justified_24bit;
    }

    /// @brief Send WREG command
    /// @param address The address of the register to write
    /// @param tx_data The data to write
    /// @param size The size of the data to write (1 to 4 bytes)
    void wreg(uint8_t address, const uint8_t *tx_data, size_t size)
    {
#ifndef ARDUINO_ARCH_AVR
        assert(tx_data != nullptr);
        assert(size > 0 && size <= 4);
#endif
        uint8_t option = (address << 2) | ((size - 1) & 0x03);
        this->send_command(commands::WREG, option, tx_data, nullptr, size);
    }

    /// @brief Send RREG command
    /// @param address The address of the register to read
    /// @param rx_data The buffer to store the read data
    /// @param size The size of the data to read (1 to 4 bytes)
    void rreg(uint8_t address, uint8_t *rx_data, size_t size)
    {
#ifndef ARDUINO_ARCH_AVR
        assert(rx_data != nullptr);
        assert(size > 0 && size <= 4);
#endif
        uint8_t option = (address << 2) | ((size - 1) & 0x03);
        this->send_command(commands::RREG, option, nullptr, rx_data, size);
    }

    // ==================== Read/Write Configuration Registers ====================

    /// @brief Write a value to the register
    /// @param address The address of the register to write
    /// @param value The value to write
    void write_register(uint8_t address, uint8_t value)
    {
        this->wreg(address, &value, 1);
    }

    /// @brief Write values to the registers
    /// @param address The first address of the register to write
    /// @param tx_data The data to write
    /// @param size The size of the data to write
    void write_registers(uint8_t address, const uint8_t *tx_data, size_t size)
    {
        this->wreg(address, tx_data, size);
    }

    /// @brief Read a value from the register
    /// @param address The address of the register to read
    /// @return The value read
    uint8_t read_register(uint8_t address)
    {
        uint8_t value = 0;
        this->rreg(address, &value, 1);
        return value;
    }

    /// @brief Read values from the registers
    /// @param address The first address of the register to read
    /// @param rx_data The buffer to store the read data
    /// @param size The size of the data to read
    void read_registers(uint8_t address, uint8_t *rx_data, size_t size)
    {
        this->rreg(address, rx_data, size);
    }

    /// @brief Read all configuration registers into the internal buffer
    /// @return The pointer to the internal buffer for the configuration registers
    const uint8_t *read_config_registers()
    {
        this->rreg(config::REG0_ADDR, this->config_regs, 4);
        return this->config_regs;
    }

    /// @brief Print configuration register values to Serial
    void print_config_registers()
    {
        this->read_config_registers();

        Serial.println("Register 0: ");
        Serial.print("MUX: 0b");
        Serial.print(this->get_config_input_multiplexer(), BIN);
        Serial.print(", GAIN: 0b");
        Serial.print(this->get_config_pga_gain(), BIN);
        Serial.print(", PGA_BYPASS: 0b");
        Serial.println(this->get_config_pga_bypass(), BIN);
        Serial.println("Register 1: ");
        Serial.print("DR: 0b");
        Serial.print(this->get_config_data_rate(), BIN);
        Serial.print(", MODE: 0b");
        Serial.print(this->get_config_operation_mode(), BIN);
        Serial.print(", CM: 0b");
        Serial.print(this->get_config_conversion_mode(), BIN);
        Serial.print(", TS: 0b");
        Serial.print(this->get_config_temperature_sensor(), BIN);
        Serial.print(", BCS: 0b");
        Serial.println(this->get_config_burn_out_current_sources(), BIN);
        Serial.println("Register 2: ");
        Serial.print("VREF: 0b");
        Serial.print(this->get_config_vref_selection(), BIN);
        Serial.print(", 50/60: 0b");
        Serial.print(this->get_config_fir_filter(), BIN);
        Serial.print(", PSW: 0b");
        Serial.print(this->get_config_low_side_power_switch(), BIN);
        Serial.print(", IDAC: 0b");
        Serial.println(this->get_config_idac_current(), BIN);
        Serial.println("Register 3: ");
        Serial.print("I1MUX: 0b");
        Serial.print(this->get_config_idac1_routing(), BIN);
        Serial.print(", I2MUX: 0b");
        Serial.print(this->get_config_idac2_routing(), BIN);
        Serial.print(", DRDYM: 0b");
        Serial.println(this->get_config_drdy_mode(), BIN);
        Serial.println();
    }

    // ===== control register 0 =====

    /// @brief Write PGA_BYPASS configuration
    /// @param value ads1220::config::pga_bypass::PgaBypass
    void write_config_pga_bypass(config::pga_bypass::PgaBypass value)
    {
        this->set_config_register(0, config::PGA_BYPASS, value);
        this->write_register(config::REG0_ADDR, this->config_regs[0]);
    }

    /// @brief Get PGA_BYPASS configuration from the internal buffer
    /// @return The value of PGA_BYPASS from the internal buffer
    uint8_t get_config_pga_bypass() const
    {
        return (config_regs[0] & config::PGA_BYPASS) >> 0;
    }

    /// @brief Write PGA_GAIN configuration
    /// @param value ads1220::config::gain::Gain
    void write_config_pga_gain(config::gain::Gain value)
    {
        this->set_config_register(0, config::GAIN, value);
        this->write_register(config::REG0_ADDR, this->config_regs[0]);
    }

    /// @brief Get PGA_GAIN configuration from the internal buffer
    /// @return The value of PGA_GAIN from the internal buffer
    uint8_t get_config_pga_gain() const
    {
        return (config_regs[0] & config::GAIN) >> 1;
    }

    /// @brief Get the gain value from the internal buffer
    /// @return 1, 2, 4, 8, 16, 32, 64, or 128
    uint8_t get_config_pga_gain_value() const
    {
        auto gain = (config_regs[0] & config::GAIN);
        switch (gain) {
        case config::gain::GAIN_1:
            return 1;
        case config::gain::GAIN_2:
            return 2;
        case config::gain::GAIN_4:
            return 4;
        case config::gain::GAIN_8:
            return 8;
        case config::gain::GAIN_16:
            return 16;
        case config::gain::GAIN_32:
            return 32;
        case config::gain::GAIN_64:
            return 64;
        case config::gain::GAIN_128:
            return 128;
        default:
            return 1;
        }
    }

    /// @brief Write MUX configuration
    /// @param value ads1220::config::mux::Mux
    void write_config_input_multiplexer(config::mux::Mux value)
    {
        this->set_config_register(0, config::MUX, value);
        this->write_register(config::REG0_ADDR, this->config_regs[0]);
    }

    /// @brief Get MUX configuration from the internal buffer
    /// @return The value of MUX from the internal buffer
    uint8_t get_config_input_multiplexer() const
    {
        return (config_regs[0] & config::MUX) >> 4;
    }

    // ===== control register 1 =====

    /// @brief Write BCS configuration
    /// @param value ads1220::config::burn_out_current_sources::Bcs
    void write_config_burn_out_current_sources(config::burn_out_current_sources::Bcs value)
    {
        this->set_config_register(1, config::BCS, value);
        this->write_register(config::REG1_ADDR, this->config_regs[1]);
    }

    /// @brief Get BCS configuration from the internal buffer
    /// @return The value of BCS from the internal buffer
    uint8_t get_config_burn_out_current_sources() const
    {
        return (config_regs[1] & config::BCS) >> 0;
    }

    /// @brief Write TS configuration
    /// @param value ads1220::config::temperature_sensor::Ts
    void write_config_temperature_sensor(config::temperature_sensor::Ts value)
    {
        this->set_config_register(1, config::TS, value);
        this->write_register(config::REG1_ADDR, this->config_regs[1]);
    }

    /// @brief Get TS configuration from the internal buffer
    /// @return The value of TS from the internal buffer
    uint8_t get_config_temperature_sensor() const
    {
        return (config_regs[1] & config::TS) >> 1;
    }

    /// @brief Write CM configuration
    /// @param value ads1220::config::conversion_mode::Cm
    void write_config_conversion_mode(config::conversion_mode::Cm value)
    {
        this->set_config_register(1, config::CM, value);
        this->write_register(config::REG1_ADDR, this->config_regs[1]);
    }

    /// @brief Get CM configuration from the internal buffer
    /// @return The value of CM from the internal buffer
    uint8_t get_config_conversion_mode() const
    {
        return (config_regs[1] & config::CM) >> 2;
    }

    /// @brief Write MODE configuration
    /// @param value ads1220::config::mode::Mode
    void write_config_operating_mode(config::mode::Mode value)
    {
        this->set_config_register(1, config::MODE, value);
        this->write_register(config::REG1_ADDR, this->config_regs[1]);
    }

    /// @brief Get MODE configuration from the internal buffer
    /// @return The value of MODE from the internal buffer
    uint8_t get_config_operation_mode() const
    {
        return (config_regs[1] & config::MODE) >> 3;
    }

    /// @brief Write DR configuration
    /// @param value ads1220::config::data_rate::Dr
    void write_config_data_rate_normal(config::data_rate::DrNormal value)
    {
        this->write_config_data_rate_impl(value);
    }

    /// @brief Write DR configuration
    /// @param value ads1220::config::data_rate::DrDutyCycle
    void write_config_data_rate_duty_cycle(config::data_rate::DrDutyCycle value)
    {
        this->write_config_data_rate_impl(value);
    }

    /// @brief Write DR configuration
    /// @param value ads1220::config::data_rate::DrTurbo
    void write_config_data_rate_turbo(config::data_rate::DrTurbo value)
    {
        this->write_config_data_rate_impl(value);
    }

    /// @brief Get DR configuration from the internal buffer
    /// @return The value of DR from the internal buffer
    uint8_t get_config_data_rate() const
    {
        return (config_regs[1] & config::DR) >> 5;
    }

    // ===== control register 2 =====

    /// @brief Write IDAC configuration
    /// @param value ads1220::config::idac::Idac
    void write_config_idac_current(config::idac::Idac value)
    {
        this->set_config_register(2, config::IDAC, value);
        this->write_register(config::REG2_ADDR, this->config_regs[2]);
    }

    /// @brief Get IDAC configuration from the internal buffer
    /// @return The value of IDAC from the internal buffer
    uint8_t get_config_idac_current() const
    {
        return (config_regs[2] & config::IDAC) >> 0;
    }

    /// @brief Write PSW configuration
    /// @param value ads1220::config::low_side_power_switch::Psw
    void write_config_low_side_power_switch(config::low_side_power_switch::Psw value)
    {
        this->set_config_register(2, config::PSW, value);
        this->write_register(config::REG2_ADDR, this->config_regs[2]);
    }

    /// @brief Write PSW configuration
    /// @return The value of PSW from the internal buffer
    uint8_t get_config_low_side_power_switch() const
    {
        return (config_regs[2] & config::PSW) >> 3;
    }

    /// @brief Write 50/60 configuration
    /// @param value ads1220::config::fir_50_60::Fir5060
    void write_config_fir_filter(config::fir_50_60::Fir5060 value)
    {
        this->set_config_register(2, config::FIR_50_60, value);
        this->write_register(config::REG2_ADDR, this->config_regs[2]);
    }

    /// @brief Get 50/60 configuration from the internal buffer
    /// @return The value of 50/60 from the internal buffer
    uint8_t get_config_fir_filter() const
    {
        return (config_regs[2] & config::FIR_50_60) >> 4;
    }

    /// @brief Write VREF configuration
    /// @param value ads1220::config::vref::Vref
    void write_config_vref_selection(config::vref::Vref value)
    {
        this->set_config_register(2, config::VREF, value);
        this->write_register(config::REG2_ADDR, this->config_regs[2]);
    }

    /// @brief Get VREF configuration from the internal buffer
    /// @return The value of VREF from the internal buffer
    uint8_t get_config_vref_selection() const
    {
        return (config_regs[2] & config::VREF) >> 6;
    }

    // ===== control register 3 =====

    /// @brief Write DRDYM configuration
    /// @param value ads1220::config::drdy_mode::DrdyMode
    void write_config_drdy_mode(config::drdy_mode::DrdyMode value)
    {
        this->set_config_register(3, config::DRDYM, value);
        this->write_register(config::REG3_ADDR, this->config_regs[3]);
    }

    /// @brief Get DRDYM configuration from the internal buffer
    /// @return The value of DRDYM from the internal buffer
    uint8_t get_config_drdy_mode() const
    {
        return (config_regs[3] & config::DRDYM) >> 1;
    }

    /// @brief Write I2MUX configuration
    /// @param value ads1220::config::i2mux::I2Mux
    void write_config_idac2_routing(config::i2mux::I2Mux value)
    {
        this->set_config_register(3, config::I2MUX, value);
        this->write_register(config::REG3_ADDR, this->config_regs[3]);
    }

    /// @brief Get I2MUX configuration from the internal buffer
    /// @return The value of I2MUX from the internal buffer
    uint8_t get_config_idac2_routing() const
    {
        return (config_regs[3] & config::I2MUX) >> 2;
    }

    /// @brief Write I1MUX configuration
    /// @param value ads1220::config::i1mux::I1Mux
    void write_config_idac1_routing(config::i1mux::I1Mux value)
    {
        this->set_config_register(3, config::I1MUX, value);
        this->write_register(config::REG3_ADDR, this->config_regs[3]);
    }

    /// @brief Get I1MUX configuration from the internal buffer
    /// @return The value of I1MUX from the internal buffer
    uint8_t get_config_idac1_routing() const
    {
        return (config_regs[3] & config::I1MUX) >> 5;
    }

private:
    /// @brief Internal initialization of ADS1220
    void begin_impl()
    {
        this->write_cs_pin(HIGH);
        this->reset();
        this->read_config_registers();
    }

    /// @brief Write CS/SS pin
    /// @param level HIGH or LOW
    void write_cs_pin(bool level)
    {
        if (this->fn_cs) {
            this->fn_cs(level);
        } else {
            digitalWrite(this->pin_cs, level);
        }
    }

    /// @brief Read DRDY pin
    /// @return HIGH or LOW
    bool read_drdy_pin()
    {
        if (this->fn_drdy) {
            return this->fn_drdy();
        } else {
            return digitalRead(this->pin_drdy);
        }
    }

    /// @brief Send command without CS/SS pin control
    /// @param command The command
    /// @param option The option byte for the command
    /// @param tx_data The data to send
    /// @param rx_data The buffer to store the read data
    /// @param size The size of the data to send/read
    void send_command_without_cs(commands::Command command, uint8_t option, const uint8_t *tx_data, uint8_t *rx_data, size_t size)
    {
        this->spi->transfer(command | option);
        for (size_t i = 0; i < size; i++) {
            uint8_t rx = this->spi->transfer(tx_data ? tx_data[i] : 0x00);
            if (rx_data) {
                rx_data[i] = rx;
            }
        }
    }

    /// @brief Send command with CS/SS pin control
    /// @param command The command
    /// @param option The option byte for the command
    /// @param tx_data The data to send
    /// @param rx_data The buffer to store the read data
    /// @param size The size of the data to send/read
    void send_command(commands::Command command, uint8_t option, const uint8_t *tx_data, uint8_t *rx_data, size_t size)
    {
        this->spi->beginTransaction(this->spi_settings);
        this->write_cs_pin(LOW);
        delayMicroseconds(1);
        this->send_command_without_cs(command, option, tx_data, rx_data, size);
        delayMicroseconds(1);
        this->write_cs_pin(HIGH);
        this->spi->endTransaction();
    }

    /// @brief Read 3 bytes from ADS1220 and return as 24-bit right-justified raw data without CS/SS pin control
    /// @return 24-bit right-justified raw data
    uint32_t read_raw_impl()
    {
        uint8_t adc_data[3];
        for (int i = 0; i < 3; i++) {
            adc_data[i] = this->spi->transfer(0x00);
        }
        // To make sure DOUT/DRDY is taken high, send 8 additional SCLKs with DIN held low
        // after each data read operation
        this->spi->transfer(0x00);

        uint32_t right_justified_24bit = adc_data[0] << 16 | adc_data[1] << 8 | adc_data[2];
        return right_justified_24bit;
    }

    /// @brief Read 24-bit raw data from ADS1220 with CS/SS control and timeout
    /// @param timeout_ms Timeout in milliseconds
    /// @return 24-bit right-justified raw data
    uint32_t read_raw(uint32_t timeout_ms)
    {
        if (!this->is_continuous_converting()) {
            this->spi->beginTransaction(this->spi_settings);
            this->write_cs_pin(LOW); // Keep CS low until reading data is complete
            this->send_command_without_cs(commands::START_SYNC, 0, nullptr, nullptr, 0);
            delay(1); // Wait for DRDY to go HIGH for safety
            if(!this->wait_drdy(timeout_ms)){
                return 0;
            }
        }

        uint32_t right_justified_24bit = this->read_raw_impl();

        if (!this->is_continuous_converting()) {
            this->write_cs_pin(HIGH);
            this->spi->endTransaction();
        }

        return right_justified_24bit;
    }

    /// @brief Wait for DRDY to go HIGH with timeout
    /// @param timeout_ms Timeout in milliseconds
    /// @return true if DRDY goes HIGH, false if timeout
    bool wait_drdy(uint32_t timeout_ms)
    {
        uint32_t start_ms = millis();
        while (this->read_drdy_pin()) {
            if (millis() > start_ms + timeout_ms) {
                return false;
            }
            delay(1);
        }
        return true;
    }

    /// @brief Set the value of the register to the internal buffer
    /// @param reg Config resister index (0 to 3)
    /// @param mask Register mask
    /// @param value The value to set
    void set_config_register(uint8_t reg, uint8_t mask, uint8_t value)
    {
        this->config_regs[reg] &= ~mask;
        this->config_regs[reg] |= value;
    }

    /// @brief Write DR configuration
    /// @param value
    void write_config_data_rate_impl(uint8_t value)
    {
        this->set_config_register(1, config::DR, value);
        this->write_register(config::REG1_ADDR, this->config_regs[1]);
    }
};

} // namespace ads1220

#endif // ARDUINO_ADS1220_H
