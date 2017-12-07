/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmTimestamp_h
#define cmTimestamp_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <time.h>

/** \class cmTimestamp
 * \brief Utility class to generate string representation of a timestamp
 *
 */
class cmTimestamp
{
public:
  cmTimestamp() {}

  std::string CurrentTime(const std::string& formatString, bool utcFlag);

  std::string FileModificationTime(const char* path,
                                   const std::string& formatString,
                                   bool utcFlag);

private:
  time_t CreateUtcTimeTFromTm(struct tm& timeStruct) const;

  std::string CreateTimestampFromTimeT(time_t timeT, std::string formatString,
                                       bool utcFlag) const;

  std::string AddTimestampComponent(char flag, struct tm& timeStruct,
                                    time_t timeT) const;
};

#endif
