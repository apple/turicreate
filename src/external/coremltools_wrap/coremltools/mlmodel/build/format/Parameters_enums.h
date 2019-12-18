#ifndef __PARAMETERS_ENUMS_H
#define __PARAMETERS_ENUMS_H
enum MLInt64ParameterAllowedValues: int {
    MLInt64ParameterAllowedValues_range = 10,
    MLInt64ParameterAllowedValues_set = 11,
    MLInt64ParameterAllowedValues_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLInt64ParameterAllowedValues_Name(MLInt64ParameterAllowedValues x) {
    switch (x) {
        case MLInt64ParameterAllowedValues_range:
            return "MLInt64ParameterAllowedValues_range";
        case MLInt64ParameterAllowedValues_set:
            return "MLInt64ParameterAllowedValues_set";
        case MLInt64ParameterAllowedValues_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

enum MLDoubleParameterAllowedValues: int {
    MLDoubleParameterAllowedValues_range = 10,
    MLDoubleParameterAllowedValues_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLDoubleParameterAllowedValues_Name(MLDoubleParameterAllowedValues x) {
    switch (x) {
        case MLDoubleParameterAllowedValues_range:
            return "MLDoubleParameterAllowedValues_range";
        case MLDoubleParameterAllowedValues_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

#endif
