/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmVariableWatch_h
#define cmVariableWatch_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <map>
#include <string>
#include <vector>

class cmMakefile;

/** \class cmVariableWatch
 * \brief Helper class for watching of variable accesses.
 *
 * Calls function when variable is accessed
 */
class cmVariableWatch
{
public:
  typedef void (*WatchMethod)(const std::string& variable, int access_type,
                              void* client_data, const char* newValue,
                              const cmMakefile* mf);
  typedef void (*DeleteData)(void* client_data);

  cmVariableWatch();
  ~cmVariableWatch();

  /**
   * Add watch to the variable
   */
  bool AddWatch(const std::string& variable, WatchMethod method,
                void* client_data = CM_NULLPTR,
                DeleteData delete_data = CM_NULLPTR);
  void RemoveWatch(const std::string& variable, WatchMethod method,
                   void* client_data = CM_NULLPTR);

  /**
   * This method is called when variable is accessed
   */
  bool VariableAccessed(const std::string& variable, int access_type,
                        const char* newValue, const cmMakefile* mf) const;

  /**
   * Different access types.
   */
  enum
  {
    VARIABLE_READ_ACCESS = 0,
    UNKNOWN_VARIABLE_READ_ACCESS,
    UNKNOWN_VARIABLE_DEFINED_ACCESS,
    VARIABLE_MODIFIED_ACCESS,
    VARIABLE_REMOVED_ACCESS,
    NO_ACCESS
  };

  /**
   * Return the access as string
   */
  static const char* GetAccessAsString(int access_type);

protected:
  struct Pair
  {
    WatchMethod Method;
    void* ClientData;
    DeleteData DeleteDataCall;
    Pair()
      : Method(CM_NULLPTR)
      , ClientData(CM_NULLPTR)
      , DeleteDataCall(CM_NULLPTR)
    {
    }
    ~Pair()
    {
      if (this->DeleteDataCall && this->ClientData) {
        this->DeleteDataCall(this->ClientData);
      }
    }
  };

  typedef std::vector<Pair*> VectorOfPairs;
  typedef std::map<std::string, VectorOfPairs> StringToVectorOfPairs;

  StringToVectorOfPairs WatchMap;
};

#endif
