/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmLocalGhsMultiGenerator.h"

#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmGhsMultiTargetGenerator.h"
#include "cmGlobalGhsMultiGenerator.h"
#include "cmMakefile.h"

cmLocalGhsMultiGenerator::cmLocalGhsMultiGenerator(cmGlobalGenerator* gg,
                                                   cmMakefile* mf)
  : cmLocalGenerator(gg, mf)
{
}

cmLocalGhsMultiGenerator::~cmLocalGhsMultiGenerator()
{
}

void cmLocalGhsMultiGenerator::Generate()
{
  const std::vector<cmGeneratorTarget*>& tgts = this->GetGeneratorTargets();

  for (std::vector<cmGeneratorTarget*>::const_iterator l = tgts.begin();
       l != tgts.end(); ++l) {
    if ((*l)->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
      continue;
    }
    cmGhsMultiTargetGenerator tg(*l);
    tg.Generate();
  }
}
