/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCPackIFWCommon_h
#define cmCPackIFWCommon_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <map>
#include <string>

class cmCPackIFWGenerator;
class cmXMLWriter;

/** \class cmCPackIFWCommon
 * \brief A base class for CPack IFW generator implementation subclasses
 */
class cmCPackIFWCommon
{
public:
  // Constructor

  /**
   * Construct Part
   */
  cmCPackIFWCommon();

public:
  // Internal implementation

  const char* GetOption(const std::string& op) const;
  bool IsOn(const std::string& op) const;
  bool IsSetToOff(const std::string& op) const;
  bool IsSetToEmpty(const std::string& op) const;

  /**
   * Compare \a version with QtIFW framework version
   */
  bool IsVersionLess(const char* version);

  /**
   * Compare \a version with QtIFW framework version
   */
  bool IsVersionGreater(const char* version);

  /**
   * Compare \a version with QtIFW framework version
   */
  bool IsVersionEqual(const char* version);

  /** Expand the list argument containing the map of the key-value pairs.
   *  If the number of elements is odd, then the first value is used as the
   *  default value with an empty key.
   *  Any values with the same keys will be permanently overwritten.
   */
  static void ExpandListArgument(const std::string& arg,
                                 std::map<std::string, std::string>& argsOut);

  /** Expand the list argument containing the multimap of the key-value pairs.
   *  If the number of elements is odd, then the first value is used as the
   *  default value with an empty key.
   */
  static void ExpandListArgument(
    const std::string& arg, std::multimap<std::string, std::string>& argsOut);

  cmCPackIFWGenerator* Generator;

protected:
  void WriteGeneratedByToStrim(cmXMLWriter& xout);
};

#define cmCPackIFWLogger(logType, msg)                                        \
  do {                                                                        \
    std::ostringstream cmCPackLog_msg;                                        \
    cmCPackLog_msg << msg;                                                    \
    if (Generator) {                                                          \
      Generator->Logger->Log(cmCPackLog::LOG_##logType, __FILE__, __LINE__,   \
                             cmCPackLog_msg.str().c_str());                   \
    }                                                                         \
  } while (false)

#endif // cmCPackIFWCommon_h
