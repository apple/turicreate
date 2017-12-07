/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmWIXFeaturesSourceWriter_h
#define cmWIXFeaturesSourceWriter_h

#include "cmWIXPatch.h"
#include "cmWIXSourceWriter.h"

#include "cmCPackGenerator.h"

/** \class cmWIXFeaturesSourceWriter
 * \brief Helper class to generate features.wxs
 */
class cmWIXFeaturesSourceWriter : public cmWIXSourceWriter
{
public:
  cmWIXFeaturesSourceWriter(cmCPackLog* logger, std::string const& filename,
                            GuidType componentGuidType);

  void CreateCMakePackageRegistryEntry(std::string const& package,
                                       std::string const& upgradeGuid);

  void EmitFeatureForComponentGroup(const cmCPackComponentGroup& group,
                                    cmWIXPatch& patch);

  void EmitFeatureForComponent(const cmCPackComponent& component,
                               cmWIXPatch& patch);

  void EmitComponentRef(std::string const& id);
};

#endif
