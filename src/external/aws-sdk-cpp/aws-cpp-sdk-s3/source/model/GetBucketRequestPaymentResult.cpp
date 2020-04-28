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

#include <aws/s3/model/GetBucketRequestPaymentResult.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <aws/core/AmazonWebServiceResult.h>
#include <aws/core/utils/StringUtils.h>

#include <utility>

using namespace Aws::S3::Model;
using namespace Aws::Utils::Xml;
using namespace Aws::Utils;
using namespace Aws;

GetBucketRequestPaymentResult::GetBucketRequestPaymentResult() : 
    m_payer(Payer::NOT_SET)
{
}

GetBucketRequestPaymentResult::GetBucketRequestPaymentResult(const Aws::AmazonWebServiceResult<XmlDocument>& result) : 
    m_payer(Payer::NOT_SET)
{
  *this = result;
}

GetBucketRequestPaymentResult& GetBucketRequestPaymentResult::operator =(const Aws::AmazonWebServiceResult<XmlDocument>& result)
{
  const XmlDocument& xmlDocument = result.GetPayload();
  XmlNode resultNode = xmlDocument.GetRootElement();

  if(!resultNode.IsNull())
  {
    XmlNode payerNode = resultNode.FirstChild("Payer");
    if(!payerNode.IsNull())
    {
      m_payer = PayerMapper::GetPayerForName(StringUtils::Trim(Aws::Utils::Xml::DecodeEscapedXmlText(payerNode.GetText()).c_str()).c_str());
    }
  }

  return *this;
}
