#ifndef COMPILE_CHECKS_H
#define COMPILE_CHECKS_H

// Compile-time checks to validate configuration arrays
static_assert(sizeof(filamentPositions) / sizeof(filamentPositions[0]) == NUMBER_OF_FILAMENTS, "filamentPositions length must equal NUMBER_OF_FILAMENTS");
static_assert(sizeof(FILAMENT_SENSOR_PINS) / sizeof(FILAMENT_SENSOR_PINS[0]) == NUMBER_OF_FILAMENTS, "FILAMENT_SENSOR_PINS length must equal NUMBER_OF_FILAMENTS");
static_assert(sizeof(FILAMENT_LEDS) / sizeof(FILAMENT_LEDS[0]) == NUMBER_OF_FILAMENTS, "FILAMENT_LEDS length must equal NUMBER_OF_FILAMENTS");
static_assert(NUMBER_OF_FILAMENTS > 0, "NUMBER_OF_FILAMENTS must be positive");

#endif
