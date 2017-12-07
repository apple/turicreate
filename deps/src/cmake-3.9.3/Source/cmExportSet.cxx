/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmExportSet.h"

#include "cmAlgorithms.h"
#include "cmLocalGenerator.h"
#include "cmTargetExport.h"

cmExportSet::~cmExportSet()
{
  cmDeleteAll(this->TargetExports);
}

void cmExportSet::Compute(cmLocalGenerator* lg)
{
  for (std::vector<cmTargetExport*>::iterator it = this->TargetExports.begin();
       it != this->TargetExports.end(); ++it) {
    (*it)->Target = lg->FindGeneratorTargetToUse((*it)->TargetName);
  }
}

void cmExportSet::AddTargetExport(cmTargetExport* te)
{
  this->TargetExports.push_back(te);
}

void cmExportSet::AddInstallation(cmInstallExportGenerator const* installation)
{
  this->Installations.push_back(installation);
}
