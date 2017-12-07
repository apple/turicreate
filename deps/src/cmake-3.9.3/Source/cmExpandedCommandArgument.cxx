/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmExpandedCommandArgument.h"

cmExpandedCommandArgument::cmExpandedCommandArgument()
  : Quoted(false)
{
}

cmExpandedCommandArgument::cmExpandedCommandArgument(std::string const& value,
                                                     bool quoted)
  : Value(value)
  , Quoted(quoted)
{
}

std::string const& cmExpandedCommandArgument::GetValue() const
{
  return this->Value;
}

bool cmExpandedCommandArgument::WasQuoted() const
{
  return this->Quoted;
}

bool cmExpandedCommandArgument::operator==(std::string const& value) const
{
  return this->Value == value;
}

bool cmExpandedCommandArgument::empty() const
{
  return this->Value.empty();
}

const char* cmExpandedCommandArgument::c_str() const
{
  return this->Value.c_str();
}
