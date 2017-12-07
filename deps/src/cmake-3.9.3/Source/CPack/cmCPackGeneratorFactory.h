/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackGeneratorFactory_h
#define cmCPackGeneratorFactory_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <map>
#include <string>
#include <vector>

class cmCPackGenerator;
class cmCPackLog;

/** \class cmCPackGeneratorFactory
 * \brief A container for CPack generators
 *
 */
class cmCPackGeneratorFactory
{
public:
  cmCPackGeneratorFactory();
  ~cmCPackGeneratorFactory();

  //! Get the generator
  cmCPackGenerator* NewGenerator(const std::string& name);
  void DeleteGenerator(cmCPackGenerator* gen);

  typedef cmCPackGenerator* CreateGeneratorCall();

  void RegisterGenerator(const std::string& name,
                         const char* generatorDescription,
                         CreateGeneratorCall* createGenerator);

  void SetLogger(cmCPackLog* logger) { this->Logger = logger; }

  typedef std::map<std::string, std::string> DescriptionsMap;
  const DescriptionsMap& GetGeneratorsList() const
  {
    return this->GeneratorDescriptions;
  }

private:
  cmCPackGenerator* NewGeneratorInternal(const std::string& name);
  std::vector<cmCPackGenerator*> Generators;

  typedef std::map<std::string, CreateGeneratorCall*> t_GeneratorCreatorsMap;
  t_GeneratorCreatorsMap GeneratorCreators;
  DescriptionsMap GeneratorDescriptions;
  cmCPackLog* Logger;
};

#endif
