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

#include <aws/core/client/DefaultRetryStrategy.h>

#include <aws/core/client/AWSError.h>
#include <aws/core/utils/UnreferencedParam.h>

using namespace Aws;
using namespace Aws::Client;

bool DefaultRetryStrategy::ShouldRetry(const AWSError<CoreErrors>& error, long attemptedRetries) const
{    
    if (attemptedRetries >= m_maxRetries)
        return false;

    return error.ShouldRetry();
}

long DefaultRetryStrategy::CalculateDelayBeforeNextRetry(const AWSError<CoreErrors>& error, long attemptedRetries) const
{
    AWS_UNREFERENCED_PARAM(error);

    if (attemptedRetries == 0)
    {
        return 0;
    }

    return (1 << attemptedRetries) * m_scaleFactor;
}
