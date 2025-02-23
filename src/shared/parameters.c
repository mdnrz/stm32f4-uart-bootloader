#include "shared/parameters.h"

// Default values for parameters
__attribute__((section(".parameters_section"))) Parameters_t param = {
    .data.float_param_1 = 0.1,
    .data.float_param_2 = 0.2,
    .data.dw_param = 0xD00DE,
    .data.w_param = 0x0DE5,
    .data.bool_param_1 = false,
    .data.bool_param_2 = true,
    .reserved = 0x33333333,
};
