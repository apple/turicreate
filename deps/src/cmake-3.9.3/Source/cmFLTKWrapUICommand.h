/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmFLTKWrapUICommand_h
#define cmFLTKWrapUICommand_h

#include "cmConfigure.h"

#include <string>
#include <vector>

#include "cmCommand.h"

class cmExecutionStatus;
class cmSourceFile;

/** \class cmFLTKWrapUICommand
 * \brief Create .h and .cxx files rules for FLTK user interfaces files
 *
 * cmFLTKWrapUICommand is used to create wrappers for FLTK classes into
 * normal C++
 */
class cmFLTKWrapUICommand : public cmCommand
{
public:
  /**
   * This is a virtual constructor for the command.
   */
  cmCommand* Clone() CM_OVERRIDE { return new cmFLTKWrapUICommand; }

  /**
   * This is called when the command is first encountered in
   * the CMakeLists.txt file.
   */
  bool InitialPass(std::vector<std::string> const& args,
                   cmExecutionStatus& status) CM_OVERRIDE;

  /**
   * This is called at the end after all the information
   * specified by the command is accumulated. Most commands do
   * not implement this method.  At this point, reading and
   * writing to the cache can be done.
   */
  void FinalPass() CM_OVERRIDE;
  bool HasFinalPass() const CM_OVERRIDE { return true; }

private:
  /**
   * List of produced files.
   */
  std::vector<cmSourceFile*> GeneratedSourcesClasses;

  /**
   * List of Fluid files that provide the source
   * generating .cxx and .h files
   */
  std::string Target;
};

#endif
