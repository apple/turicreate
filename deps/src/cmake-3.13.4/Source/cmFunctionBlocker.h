/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmFunctionBlocker_h
#define cmFunctionBlocker_h

#include "cmListFileCache.h"

class cmExecutionStatus;
class cmMakefile;

class cmFunctionBlocker
{
public:
  /**
   * should a function be blocked
   */
  virtual bool IsFunctionBlocked(const cmListFileFunction& lff, cmMakefile& mf,
                                 cmExecutionStatus& status) = 0;

  /**
   * should this function blocker be removed, useful when one function adds a
   * blocker and another must remove it
   */
  virtual bool ShouldRemove(const cmListFileFunction&, cmMakefile&)
  {
    return false;
  }

  virtual ~cmFunctionBlocker() {}

  /** Set/Get the context in which this blocker is created.  */
  void SetStartingContext(cmListFileContext const& lfc)
  {
    this->StartingContext = lfc;
  }
  cmListFileContext const& GetStartingContext() const
  {
    return this->StartingContext;
  }

private:
  cmListFileContext StartingContext;
};

#endif
