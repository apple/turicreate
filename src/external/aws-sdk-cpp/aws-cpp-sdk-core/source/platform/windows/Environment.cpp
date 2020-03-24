/*
  * Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
  *
  * Licensed under the Apache License, Version 2.0 (the "License").
  * You may not use this file except in compliance with the License.
  * A copy of the License is located at
  *
  *  http://aws.amazon.com/apache2.0
  *
  * or in the "license" file accompanying this file. This file is distributed
  * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
  * express or implied. See the License for the specific language governing
  * permissions and limitations under the License.
  */

#include <aws/core/platform/Environment.h>

#include <stdio.h>
#include <utility>

namespace Aws
{
namespace Environment
{

/*
using std::getenv generates a warning on windows so we use _dupenv_s instead.  The character array returned by this function is our responsibility to clean up, so rather than returning raw strings
that would need to be manually freed in all the client functions, just copy it into a Aws::String instead, freeing it here.
*/
Aws::String GetEnv(const char *variableName)
{
    char* variableValue = nullptr;
    std::size_t valueSize = 0;
    auto queryResult = _dupenv_s(&variableValue, &valueSize, variableName);

    Aws::String result;
    if(queryResult == 0 && variableValue != nullptr && valueSize > 0)
    {
        result.assign(variableValue, valueSize - 1);  // don't copy the c-string terminator byte
        free(variableValue);
    }

    return result;
}

} // namespace Environment
} // namespace Aws
