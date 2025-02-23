#ifndef __PARAMETERS_H__
#define __PARAMETERS_H__

#include <stdbool.h>
#include <stdint.h>

// Sample struct containing some config parameters
typedef struct {
    float float_param_1;
    float float_param_2;
    uint32_t dw_param;
    uint16_t w_param;
    bool bool_param_1;
    bool bool_param_2;
} Param_Data_t;

typedef struct /*__attribute__((__packed__))*/ {
    Param_Data_t data;
    uint32_t reserved;
    // can be expanded to 8KB
} Parameters_t;

extern Parameters_t param;

#endif // __PARAMETERS_H__
