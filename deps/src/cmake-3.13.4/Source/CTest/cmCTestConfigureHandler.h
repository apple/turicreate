/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCTestConfigureHandler_h
#define cmCTestConfigureHandler_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmCTestGenericHandler.h"

/** \class cmCTestConfigureHandler
 * \brief A class that handles ctest -S invocations
 *
 */
class cmCTestConfigureHandler : public cmCTestGenericHandler
{
public:
  typedef cmCTestGenericHandler Superclass;

  /*
   * The main entry point for this class
   */
  int ProcessHandler() override;

  cmCTestConfigureHandler();

  void Initialize() override;
};

#endif
