/*
  * Copyright 2010-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#pragma once

#include <aws/core/client/DefaultRetryStrategy.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/memory/stl/AWSVector.h>
namespace Aws
{
namespace Client
{

/**
 * @brief This retry strategy is almost identical to DefaultRetryStrategy, except it accepts a vector of error or exception names
 * that you want to retry anyway (bypass the retryable definition of the error instance itself) if the retry attempts is less than maxRetries.
 */
class AWS_CORE_API SpecifiedRetryableErrorsRetryStrategy : public DefaultRetryStrategy
{
public:
    SpecifiedRetryableErrorsRetryStrategy(const Aws::Vector<Aws::String>& specifiedRetryableErrors, long maxRetries = 10, long scaleFactor = 25) :
        DefaultRetryStrategy(maxRetries, scaleFactor), m_specifiedRetryableErrors(specifiedRetryableErrors)
    {}

    bool ShouldRetry(const AWSError<CoreErrors>& error, long attemptedRetries) const override;

private:
    Aws::Vector<Aws::String> m_specifiedRetryableErrors;
};

} // namespace Client
} // namespace Aws
