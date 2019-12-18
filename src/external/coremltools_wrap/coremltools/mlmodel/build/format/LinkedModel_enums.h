#ifndef __LINKEDMODEL_ENUMS_H
#define __LINKEDMODEL_ENUMS_H
enum MLLinkedModelLinkType: int {
    MLLinkedModelLinkType_linkedModelFile = 1,
    MLLinkedModelLinkType_NOT_SET = 0,
};

__attribute__((__unused__))
static const char * MLLinkedModelLinkType_Name(MLLinkedModelLinkType x) {
    switch (x) {
        case MLLinkedModelLinkType_linkedModelFile:
            return "MLLinkedModelLinkType_linkedModelFile";
        case MLLinkedModelLinkType_NOT_SET:
            return "INVALID";
    }
    return "INVALID";
}

#endif
