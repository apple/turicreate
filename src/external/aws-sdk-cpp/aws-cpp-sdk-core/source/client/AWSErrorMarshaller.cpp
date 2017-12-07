/*
  * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <aws/core/client/AWSErrorMarshaller.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/client/AWSError.h>
#include <aws/core/client/CoreErrors.h>

using namespace Aws::Utils::Logging;
using namespace Aws::Client;

static const char* AWS_ERROR_MARSHALLER_LOG_TAG = "AWSErrorMarshaller";

AWSError<CoreErrors> AWSErrorMarshaller::Marshall(const Aws::String& exceptionName, const Aws::String& message) const
{
    auto locationOfPound = exceptionName.find_first_of('#');
    auto locationOfColon = exceptionName.find_first_of(':');
    Aws::String formalExceptionName;

    if (locationOfPound != Aws::String::npos)
    {
        formalExceptionName = exceptionName.substr(locationOfPound + 1);       
    }
    else if (locationOfColon != Aws::String::npos)
    {
        formalExceptionName = exceptionName.substr(0, locationOfColon);       
    }
    else
    {
        formalExceptionName = exceptionName;
    }

    AWSError<CoreErrors> error = FindErrorByName(formalExceptionName.c_str());
    if (error.GetErrorType() != CoreErrors::UNKNOWN)
    {
        AWS_LOG_WARN(AWS_ERROR_MARSHALLER_LOG_TAG, "Encountered AWSError\n%s\n%s:", formalExceptionName.c_str(), message.c_str());
        error.SetExceptionName(formalExceptionName);
        error.SetMessage(message);
        return error;
    }    

    AWS_LOG_WARN(AWS_ERROR_MARSHALLER_LOG_TAG, "Encountered Unknown AWSError\n%s\n%s:", exceptionName.c_str(), message.c_str());

    return AWSError<CoreErrors>(CoreErrors::UNKNOWN, exceptionName, "Unable to parse ExceptionName: " + exceptionName + " Message: " + message, false);
}

AWSError<CoreErrors> AWSErrorMarshaller::FindErrorByName(const char* errorName) const
{
    return CoreErrorsMapper::GetErrorForName(errorName);
}
