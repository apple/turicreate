#ifndef __SOUNDANALYSISPREPROCESSING_ENUMS_H
#define __SOUNDANALYSISPREPROCESSING_ENUMS_H
enum MLSoundAnalysisPreprocessingSoundAnalysisPreprocessingType: int {
    MLSoundAnalysisPreprocessingSoundAnalysisPreprocessingType_vggish = 20,
    MLSoundAnalysisPreprocessingSoundAnalysisPreprocessingType_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLSoundAnalysisPreprocessingSoundAnalysisPreprocessingType_Name(MLSoundAnalysisPreprocessingSoundAnalysisPreprocessingType x) {
    switch (x) {
        case MLSoundAnalysisPreprocessingSoundAnalysisPreprocessingType_vggish:
            return "MLSoundAnalysisPreprocessingSoundAnalysisPreprocessingType_vggish";
        case MLSoundAnalysisPreprocessingSoundAnalysisPreprocessingType_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

#endif
