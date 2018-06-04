#ifndef __VISIONFEATUREPRINT_ENUMS_H
#define __VISIONFEATUREPRINT_ENUMS_H
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
enum MLVisionFeaturePrintVisionFeaturePrintType: int {
    MLVisionFeaturePrintVisionFeaturePrintType_scene = 20,
    MLVisionFeaturePrintVisionFeaturePrintType_NOT_SET = 0,
};

static const char * MLVisionFeaturePrintVisionFeaturePrintType_Name(MLVisionFeaturePrintVisionFeaturePrintType x) {
    switch (x) {
        case MLVisionFeaturePrintVisionFeaturePrintType_scene:
            return "MLVisionFeaturePrintVisionFeaturePrintType_scene";
        case MLVisionFeaturePrintVisionFeaturePrintType_NOT_SET:
            return "INVALID";
    }
}

enum MLSceneVersion: int {
    MLSceneVersionSCENE_VERSION_INVALID = 0,
    MLSceneVersionSCENE_VERSION_1 = 1,
};

#pragma clang diagnostic pop
#endif
