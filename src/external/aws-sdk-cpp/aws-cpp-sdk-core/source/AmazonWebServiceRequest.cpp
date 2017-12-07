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

#include <aws/core/AmazonWebServiceRequest.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/utils/memory/stl/AWSFunction.h>

using namespace Aws;

AmazonWebServiceRequest::AmazonWebServiceRequest() :
    m_responseStreamFactory(AWS_BUILD_FUNCTION(Aws::Utils::Stream::DefaultResponseStreamFactoryMethod)),
    m_onDataReceived(nullptr),
    m_onDataSent(nullptr)
{
}

