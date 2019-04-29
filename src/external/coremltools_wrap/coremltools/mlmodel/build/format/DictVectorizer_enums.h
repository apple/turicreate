#ifndef __DICTVECTORIZER_ENUMS_H
#define __DICTVECTORIZER_ENUMS_H
enum MLDictVectorizerMap: int {
    MLDictVectorizerMap_stringToIndex = 1,
    MLDictVectorizerMap_int64ToIndex = 2,
    MLDictVectorizerMap_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLDictVectorizerMap_Name(MLDictVectorizerMap x) {
    switch (x) {
        case MLDictVectorizerMap_stringToIndex:
            return "MLDictVectorizerMap_stringToIndex";
        case MLDictVectorizerMap_int64ToIndex:
            return "MLDictVectorizerMap_int64ToIndex";
        case MLDictVectorizerMap_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

#endif
