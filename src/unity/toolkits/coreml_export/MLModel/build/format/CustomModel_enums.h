#ifndef __CUSTOMMODEL_ENUMS_H
#define __CUSTOMMODEL_ENUMS_H
enum MLCustomModelParamValuevalue: int {
    MLCustomModelParamValuevalue_doubleValue = 10,
    MLCustomModelParamValuevalue_stringValue = 20,
    MLCustomModelParamValuevalue_intValue = 30,
    MLCustomModelParamValuevalue_longValue = 40,
    MLCustomModelParamValuevalue_boolValue = 50,
    MLCustomModelParamValuevalue_bytesValue = 60,
    MLCustomModelParamValuevalue_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLCustomModelParamValuevalue_Name(MLCustomModelParamValuevalue x) {
    switch (x) {
        case MLCustomModelParamValuevalue_doubleValue:
            return "MLCustomModelParamValuevalue_doubleValue";
        case MLCustomModelParamValuevalue_stringValue:
            return "MLCustomModelParamValuevalue_stringValue";
        case MLCustomModelParamValuevalue_intValue:
            return "MLCustomModelParamValuevalue_intValue";
        case MLCustomModelParamValuevalue_longValue:
            return "MLCustomModelParamValuevalue_longValue";
        case MLCustomModelParamValuevalue_boolValue:
            return "MLCustomModelParamValuevalue_boolValue";
        case MLCustomModelParamValuevalue_bytesValue:
            return "MLCustomModelParamValuevalue_bytesValue";
        case MLCustomModelParamValuevalue_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

#endif
