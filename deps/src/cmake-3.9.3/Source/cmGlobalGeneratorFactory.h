/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmGlobalGeneratorFactory_h
#define cmGlobalGeneratorFactory_h

#include "cmConfigure.h"

#include <string>
#include <vector>

class cmGlobalGenerator;
class cmake;
struct cmDocumentationEntry;

/** \class cmGlobalGeneratorFactory
 * \brief Responable for creating cmGlobalGenerator instances
 *
 * Subclasses of this class generate instances of cmGlobalGenerator.
 */
class cmGlobalGeneratorFactory
{
public:
  virtual ~cmGlobalGeneratorFactory() {}

  /** Create a GlobalGenerator */
  virtual cmGlobalGenerator* CreateGlobalGenerator(const std::string& n,
                                                   cmake* cm) const = 0;

  /** Get the documentation entry for this factory */
  virtual void GetDocumentation(cmDocumentationEntry& entry) const = 0;

  /** Get the names of the current registered generators */
  virtual void GetGenerators(std::vector<std::string>& names) const = 0;

  /** Determine whether or not this generator supports toolsets */
  virtual bool SupportsToolset() const = 0;

  /** Determine whether or not this generator supports platforms */
  virtual bool SupportsPlatform() const = 0;
};

template <class T>
class cmGlobalGeneratorSimpleFactory : public cmGlobalGeneratorFactory
{
public:
  /** Create a GlobalGenerator */
  cmGlobalGenerator* CreateGlobalGenerator(const std::string& name,
                                           cmake* cm) const CM_OVERRIDE
  {
    if (name != T::GetActualName()) {
      return CM_NULLPTR;
    }
    return new T(cm);
  }

  /** Get the documentation entry for this factory */
  void GetDocumentation(cmDocumentationEntry& entry) const CM_OVERRIDE
  {
    T::GetDocumentation(entry);
  }

  /** Get the names of the current registered generators */
  void GetGenerators(std::vector<std::string>& names) const CM_OVERRIDE
  {
    names.push_back(T::GetActualName());
  }

  /** Determine whether or not this generator supports toolsets */
  bool SupportsToolset() const CM_OVERRIDE { return T::SupportsToolset(); }

  /** Determine whether or not this generator supports platforms */
  bool SupportsPlatform() const CM_OVERRIDE { return T::SupportsPlatform(); }
};

#endif
