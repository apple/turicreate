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

#include <aws/s3/model/GetBucketLocationResult.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <aws/core/AmazonWebServiceResult.h>
#include <aws/core/utils/StringUtils.h>

#include <utility>

using namespace Aws::S3::Model;
using namespace Aws::Utils::Xml;
using namespace Aws::Utils;
using namespace Aws;

GetBucketLocationResult::GetBucketLocationResult():
    m_locationConstraint(BucketLocationConstraint::NOT_SET)
{
}

GetBucketLocationResult::GetBucketLocationResult(const AmazonWebServiceResult<XmlDocument>& result):
    m_locationConstraint(BucketLocationConstraint::NOT_SET)
{
    *this = result;
}

GetBucketLocationResult& GetBucketLocationResult::operator =(const AmazonWebServiceResult<XmlDocument>& result)
{
    const XmlDocument& xmlDocument = result.GetPayload();
    XmlNode resultNode = xmlDocument.GetRootElement();

    if(!resultNode.IsNull())
    {
        m_locationConstraint = BucketLocationConstraintMapper::GetBucketLocationConstraintForName(StringUtils::Trim(resultNode.GetText().c_str()).c_str());
    }

    return *this; 
}

