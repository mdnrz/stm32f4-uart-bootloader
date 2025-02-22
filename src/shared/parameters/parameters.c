#include "shared/parameters/parameters.h"
// #include <app/detector/detector.h>

__attribute__((section(".parameters_section"))) Parameters_t param = {
    .currentInstance = BOOTLOADER,
    .thresholds.entranceTh = 300,
    .thresholds.exitTh = 150,
    .thresholds.entranceDebounceTime = 3,
    .thresholds.exitDebounceTime = 150,
    .layout[0] = {.active = true, .ch = {1, 3}},
    .layout[1] = {.active = false, .ch = {2, 3}},
    .layout[2] = {.active = false, .ch = {4, 5}},
    .layout[3] = {.active = false, .ch = {6, 7}},
    .captureConfig.switchingActive = false,
    .captureConfig.TB_SW_frequency = 800,
    .filterAvgWindow = 8,
    .reserved12 = 0x33333333,
    .reserved13 = 0x44444444,
    .reserved14 = 0x55555555,
    .reserved15 = 0x66666666,
    .reserved16 = 0x77777777,
    .reserved17 = 0x88888888,
    .reserved18 = 0x99999999,
    .reserved19 = 0xaaaaaaaa,
};
