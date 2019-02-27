/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmUseMangledMesaCommand_h
#define cmUseMangledMesaCommand_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;

class cmUseMangledMesaCommand : public cmCommand
{
public:
  cmCommand* Clone() override { return new cmUseMangledMesaCommand; }
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) override;

protected:
  void CopyAndFullPathMesaHeader(const char* source, const char* outdir);
};

#endif
