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

#include <aws/core/http/standard/StandardHttpResponse.h>

#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/memory/AWSMemory.h>

#include <istream>

using namespace Aws::Http;
using namespace Aws::Http::Standard;
using namespace Aws::Utils;


HeaderValueCollection StandardHttpResponse::GetHeaders() const
{
    HeaderValueCollection headerValueCollection;

    for (Aws::Map<Aws::String, Aws::String>::const_iterator iter = headerMap.begin(); iter != headerMap.end(); ++iter)
    {
        headerValueCollection.emplace(HeaderValuePair(iter->first, iter->second));
    }

    return headerValueCollection;
}

bool StandardHttpResponse::HasHeader(const char* headerName) const
{
    return headerMap.find(StringUtils::ToLower(headerName)) != headerMap.end();
}

const Aws::String& StandardHttpResponse::GetHeader(const Aws::String& headerName) const
{
    Aws::Map<Aws::String, Aws::String>::const_iterator foundValue = headerMap.find(StringUtils::ToLower(headerName.c_str()));
    return foundValue->second;
}

void StandardHttpResponse::AddHeader(const Aws::String& headerName, const Aws::String& headerValue)
{
    headerMap[StringUtils::ToLower(headerName.c_str())] = headerValue;
}


