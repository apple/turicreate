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

#include <aws/core/client/AWSErrorMarshaller.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/client/AWSError.h>
#include <aws/core/client/CoreErrors.h>

using namespace Aws::Utils::Logging;
using namespace Aws::Utils::Json;
using namespace Aws::Utils::Xml;
using namespace Aws::Http;
using namespace Aws::Utils;
using namespace Aws::Client;

static const char AWS_ERROR_MARSHALLER_LOG_TAG[] = "AWSErrorMarshaller";
AWS_CORE_API extern const char MESSAGE_LOWER_CASE[]     = "message";
AWS_CORE_API extern const char MESSAGE_CAMEL_CASE[]     = "Message";
AWS_CORE_API extern const char ERROR_TYPE_HEADER[]      = "x-amzn-ErrorType";
AWS_CORE_API extern const char TYPE[]                   = "__type";

AWSError<CoreErrors> JsonErrorMarshaller::Marshall(const Aws::Http::HttpResponse& httpResponse) const
{
    JsonValue exceptionPayload(httpResponse.GetResponseBody());
    JsonView payloadView(exceptionPayload);
    if (!exceptionPayload.WasParseSuccessful())
    {
        return AWSError<CoreErrors>(CoreErrors::UNKNOWN, "", "Failed to parse error payload", false);
    }

    AWS_LOGSTREAM_TRACE(AWS_ERROR_MARSHALLER_LOG_TAG, "Error response is " << payloadView.WriteReadable());

    Aws::String message(payloadView.ValueExists(MESSAGE_CAMEL_CASE) ? payloadView.GetString(MESSAGE_CAMEL_CASE) :
            payloadView.ValueExists(MESSAGE_LOWER_CASE) ? payloadView.GetString(MESSAGE_LOWER_CASE) : "");

    if (httpResponse.HasHeader(ERROR_TYPE_HEADER))
    {
        return Marshall(httpResponse.GetHeader(ERROR_TYPE_HEADER), message);
    }
    else if (payloadView.ValueExists(TYPE))
    {
        return Marshall(payloadView.GetString(TYPE), message);
    }
    else
    {
        return FindErrorByHttpResponseCode(httpResponse.GetResponseCode());
    }
}

AWSError<CoreErrors> XmlErrorMarshaller::Marshall(const Aws::Http::HttpResponse& httpResponse) const
{
    XmlDocument doc = XmlDocument::CreateFromXmlStream(httpResponse.GetResponseBody());
    AWS_LOGSTREAM_TRACE(AWS_ERROR_MARSHALLER_LOG_TAG, "Error response is " << doc.ConvertToString());
    bool errorParsed = false;
    AWSError<CoreErrors> error;
    if (doc.WasParseSuccessful())
    {
        XmlNode errorNode = doc.GetRootElement();
        if (errorNode.GetName() != "Error")
        {
            errorNode = doc.GetRootElement().FirstChild("Error");
        }
        if (errorNode.IsNull())
        {
            errorNode = doc.GetRootElement().FirstChild("Errors");
            if(!errorNode.IsNull())
            {
                errorNode = errorNode.FirstChild("Error");
            }
        }

        if (!errorNode.IsNull())
        {
            XmlNode codeNode = errorNode.FirstChild("Code");
            XmlNode messageNode = errorNode.FirstChild("Message");

            if (!codeNode.IsNull())
            {
                error = Marshall(StringUtils::Trim(codeNode.GetText().c_str()),
                                 StringUtils::Trim(messageNode.GetText().c_str()));
                errorParsed = true;
            }
        }
    }

    if(!errorParsed)
    {
        // An error occurred attempting to parse the httpResponse as an XML stream, so we're just
        // going to dump the XML parsing error and the http response code as a string
        AWS_LOGSTREAM_WARN(AWS_ERROR_MARSHALLER_LOG_TAG, "Unable to generate a proper httpResponse from the response "
                "stream.   Response code: " << static_cast< uint32_t >(httpResponse.GetResponseCode()));
        error = FindErrorByHttpResponseCode(httpResponse.GetResponseCode());
    }

    return error;
}

AWSError<CoreErrors> AWSErrorMarshaller::Marshall(const Aws::String& exceptionName, const Aws::String& message) const
{
    if(exceptionName.empty())
    {
        return AWSError<CoreErrors>(CoreErrors::UNKNOWN, "", message, false);
    }

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
        AWS_LOGSTREAM_WARN(AWS_ERROR_MARSHALLER_LOG_TAG, "Encountered AWSError '" << formalExceptionName.c_str() <<
                "': " << message.c_str());
        error.SetExceptionName(formalExceptionName);
        error.SetMessage(message);
        return error;
    }    

    AWS_LOGSTREAM_WARN(AWS_ERROR_MARSHALLER_LOG_TAG, "Encountered Unknown AWSError '" << exceptionName.c_str() <<
            "': " <<  message.c_str());

    return AWSError<CoreErrors>(CoreErrors::UNKNOWN, exceptionName, "Unable to parse ExceptionName: " + exceptionName + " Message: " + message, false);
}

AWSError<CoreErrors> AWSErrorMarshaller::FindErrorByName(const char* errorName) const
{
    return CoreErrorsMapper::GetErrorForName(errorName);
}

AWSError<CoreErrors> AWSErrorMarshaller::FindErrorByHttpResponseCode(Aws::Http::HttpResponseCode code) const
{
    return CoreErrorsMapper::GetErrorForHttpResponseCode(code);
}
