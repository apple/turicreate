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

#include <aws/core/http/HttpRequest.h>

namespace Aws
{
namespace Http
{

const char DATE_HEADER[] = "date";
const char AWS_DATE_HEADER[] = "X-Amz-Date";
const char AWS_SECURITY_TOKEN[] = "X-Amz-Security-Token";
const char ACCEPT_HEADER[] = "accept";
const char ACCEPT_CHAR_SET_HEADER[] = "accept-charset";
const char ACCEPT_ENCODING_HEADER[] = "accept-encoding";
const char AUTHORIZATION_HEADER[] = "authorization";
const char AWS_AUTHORIZATION_HEADER[] = "authorization";
const char COOKIE_HEADER[] = "cookie";
const char CONTENT_LENGTH_HEADER[] = "content-length";
const char CONTENT_TYPE_HEADER[] = "content-type";
const char TRANSFER_ENCODING_HEADER[] = "transfer-encoding";
const char USER_AGENT_HEADER[] = "user-agent";
const char VIA_HEADER[] = "via";
const char HOST_HEADER[] = "host";
const char AMZ_TARGET_HEADER[] = "x-amz-target";
const char X_AMZ_EXPIRES_HEADER[] = "X-Amz-Expires";
const char CONTENT_MD5_HEADER[] = "content-md5";
const char API_VERSION_HEADER[] = "x-amz-api-version";
const char CHUNKED_VALUE[] = "chunked";

} // Http
} // Aws



