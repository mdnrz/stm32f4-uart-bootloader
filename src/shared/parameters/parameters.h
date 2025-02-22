#ifndef __PARAMETERS_H__
#define __PARAMETERS_H__

#include <stdbool.h>
#include <stdint.h>

// This is bullshit porbably
enum CURRENT_STATUS {
    BOOTLOADER,
    APPLICATION,
};

enum Param_CapturePSC_t {
    PSC_OFF,
    PSC_2,
    PSC_4,
    PSC_8,
};

typedef struct {
    float entranceTh;
    float exitTh;
    uint32_t entranceDebounceTime;
    uint32_t exitDebounceTime;
} Param_DetectorThCoef_t;

typedef struct {
    bool active;
    uint8_t ch[2];
} Param_LaneAttr_t;

typedef struct __attribute__((__packed__)) {
    bool switchingActive;
    uint32_t TB_SW_frequency;
} Param_CaptureConfig_t;

typedef struct /*__attribute__((__packed__))*/ {
    uint32_t currentInstance;
    Param_DetectorThCoef_t thresholds;
    Param_LaneAttr_t layout[4];
    Param_CaptureConfig_t captureConfig;
    uint8_t filterAvgWindow;
    uint32_t reserved12;
    uint32_t reserved13;
    uint32_t reserved14;
    uint32_t reserved15;
    uint32_t reserved16;
    uint32_t reserved17;
    uint32_t reserved18;
    uint32_t reserved19;
} Parameters_t;

extern Parameters_t param;

#endif // __PARAMETERS_H__
