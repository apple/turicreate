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

#include <aws/core/http/HttpTypes.h>
#include <cassert>

using namespace Aws::Http;

namespace Aws
{
namespace Http
{

namespace HttpMethodMapper
{
const char* GetNameForHttpMethod(HttpMethod httpMethod)
{
    switch (httpMethod)
    {
        case HttpMethod::HTTP_GET:
            return "GET";
        case HttpMethod::HTTP_POST:
            return "POST";
        case HttpMethod::HTTP_DELETE:
            return "DELETE";
        case HttpMethod::HTTP_PUT:
            return "PUT";
        case HttpMethod::HTTP_HEAD:
            return "HEAD";
        case HttpMethod::HTTP_PATCH:
            return "PATCH";
        default:
            assert(0);
            return "GET";
    }
}

} // namespace HttpMethodMapper
} // namespace Http
} // namespace Aws
