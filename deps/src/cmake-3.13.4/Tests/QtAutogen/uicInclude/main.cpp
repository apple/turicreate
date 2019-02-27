
#include "main.hpp"

int main(int argv, char** args)
{
  return 0;
}

// .ui files in CMAKE_AUTOUIC_SEARCH_PATHS
#include "ui_PageA.h"
// .ui files in AUTOUIC_SEARCH_PATHS
#include "sub/gen/deep/ui_PageB2.h"
#include "subB/ui_PageBsub.h"
#include "ui_PageB.h"
// .ui files in source's vicinity
#include "sub/gen/deep/ui_PageC2.h"
#include "subC/ui_PageCsub.h"
#include "ui_PageC.h"
