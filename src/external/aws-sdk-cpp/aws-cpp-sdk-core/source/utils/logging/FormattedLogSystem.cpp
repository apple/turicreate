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


#include <aws/core/utils/logging/FormattedLogSystem.h>

#include <aws/core/utils/DateTime.h>
#include <aws/core/utils/Array.h>

#include <fstream>
#include <cstdarg>
#include <stdio.h>
#include <thread>

using namespace Aws::Utils;
using namespace Aws::Utils::Logging;

static Aws::String CreateLogPrefixLine(LogLevel logLevel, const char* tag)
{
    Aws::StringStream ss;

    switch(logLevel)
    {
        case LogLevel::Error:
            ss << "[ERROR] ";
            break;

        case LogLevel::Fatal:
            ss << "[FATAL] ";
            break;

        case LogLevel::Warn:
            ss << "[WARN] ";
            break;

        case LogLevel::Info:
            ss << "[INFO] ";
            break;

        case LogLevel::Debug:
            ss << "[DEBUG] ";
            break;

        case LogLevel::Trace:
            ss << "[TRACE] ";
            break;

        default:
            ss << "[UNKOWN] ";
            break;
    }

    ss << DateTime::Now().CalculateGmtTimeWithMsPrecision() << " " << tag << " [" << std::this_thread::get_id() << "] ";

    return ss.str();
}

FormattedLogSystem::FormattedLogSystem(LogLevel logLevel) :
    m_logLevel(logLevel)
{
}

void FormattedLogSystem::Log(LogLevel logLevel, const char* tag, const char* formatStr, ...)
{
    Aws::StringStream ss;
    ss << CreateLogPrefixLine(logLevel, tag);

    std::va_list args;
    va_start(args, formatStr);

    va_list tmp_args; //unfortunately you cannot consume a va_list twice
    va_copy(tmp_args, args); //so we have to copy it
    #ifdef WIN32
        const int requiredLength = _vscprintf(formatStr, tmp_args) + 1;
    #else
        const int requiredLength = vsnprintf(nullptr, 0, formatStr, tmp_args) + 1;
    #endif
    va_end(tmp_args);

    Array<char> outputBuff(requiredLength);
    #ifdef WIN32
        vsnprintf_s(outputBuff.GetUnderlyingData(), requiredLength, _TRUNCATE, formatStr, args);
    #else
        vsnprintf(outputBuff.GetUnderlyingData(), requiredLength, formatStr, args);
    #endif // WIN32

    ss << outputBuff.GetUnderlyingData() << std::endl;  
  
    ProcessFormattedStatement(ss.str());

    va_end(args);
}

void FormattedLogSystem::LogStream(LogLevel logLevel, const char* tag, const Aws::OStringStream &message_stream)
{
    ProcessFormattedStatement(CreateLogPrefixLine(logLevel, tag) + message_stream.str() + "\n");
}
