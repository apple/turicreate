/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmProperty.h"

void cmProperty::Set(const char* value)
{
  this->Value = value;
  this->ValueHasBeenSet = true;
}

void cmProperty::Append(const char* value, bool asString)
{
  if (!this->Value.empty() && *value && !asString) {
    this->Value += ";";
  }
  this->Value += value;
  this->ValueHasBeenSet = true;
}

const char* cmProperty::GetValue() const
{
  if (this->ValueHasBeenSet) {
    return this->Value.c_str();
  }
  return nullptr;
}
