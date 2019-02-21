/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmCTestBatchTestHandler_h
#define cmCTestBatchTestHandler_h

#include "cmConfigure.h"

#include "cmCTestMultiProcessHandler.h"
#include "cmsys/FStream.hxx"
#include <string>

/** \class cmCTestBatchTestHandler
 * \brief run parallel ctest
 *
 * cmCTestBatchTestHandler
 */
class cmCTestBatchTestHandler : public cmCTestMultiProcessHandler
{
public:
  ~cmCTestBatchTestHandler() CM_OVERRIDE;
  void RunTests() CM_OVERRIDE;

protected:
  void WriteBatchScript();
  void WriteSrunArgs(int test, std::ostream& fout);
  void WriteTestCommand(int test, std::ostream& fout);

  void SubmitBatchScript();

  std::string Script;
};

#endif
