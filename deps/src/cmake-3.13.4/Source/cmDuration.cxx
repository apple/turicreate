/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#define CMDURATION_CPP
#include "cmDuration.h"

template <typename T>
T cmDurationTo(const cmDuration& duration)
{
  /* This works because the comparison operators for duration rely on
   * std::common_type.
   * So for example duration<int>::max() gets promoted to a duration<double>,
   * which can then be safely compared.
   */
  if (duration >= std::chrono::duration<T>::max()) {
    return std::chrono::duration<T>::max().count();
  }
  if (duration <= std::chrono::duration<T>::min()) {
    return std::chrono::duration<T>::min().count();
  }
  // Ensure number of seconds by defining ratio<1>
  return std::chrono::duration_cast<std::chrono::duration<T, std::ratio<1>>>(
           duration)
    .count();
}

template int cmDurationTo<int>(const cmDuration&);
template unsigned int cmDurationTo<unsigned int>(const cmDuration&);
