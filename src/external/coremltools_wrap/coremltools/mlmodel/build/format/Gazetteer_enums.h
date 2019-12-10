#ifndef __GAZETTEER_ENUMS_H
#define __GAZETTEER_ENUMS_H
enum MLGazetteerClassLabels: int {
    MLGazetteerClassLabels_stringClassLabels = 200,
    MLGazetteerClassLabels_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLGazetteerClassLabels_Name(MLGazetteerClassLabels x) {
    switch (x) {
        case MLGazetteerClassLabels_stringClassLabels:
            return "MLGazetteerClassLabels_stringClassLabels";
        case MLGazetteerClassLabels_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

#endif
