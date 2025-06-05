#pragma once
#ifndef ARDUINO_ADS1220_COMMANDS_H
#define ARDUINO_ADS1220_COMMANDS_H

#include <Arduino.h>

namespace ads1220::commands {

enum Command : uint8_t
{
    POWERDOWN  = 0b00000010,
    RESET      = 0b00000110,
    START_SYNC = 0b00001000,
    RDATA      = 0b00010000,
    RREG       = 0b00100000,
    WREG       = 0b01000000,
};

} // namespace ads1220::commands

#endif // ARDUINO_ADS1220_COMMANDS_H
