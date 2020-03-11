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

#include <aws/s3/model/SelectObjectContentRequest.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/http/URI.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

#include <utility>

using namespace Aws::S3::Model;
using namespace Aws::Utils::Xml;
using namespace Aws::Utils;
using namespace Aws::Http;

SelectObjectContentRequest::SelectObjectContentRequest() : 
    m_bucketHasBeenSet(false),
    m_keyHasBeenSet(false),
    m_sSECustomerAlgorithmHasBeenSet(false),
    m_sSECustomerKeyHasBeenSet(false),
    m_sSECustomerKeyMD5HasBeenSet(false),
    m_expressionHasBeenSet(false),
    m_expressionType(ExpressionType::NOT_SET),
    m_expressionTypeHasBeenSet(false),
    m_requestProgressHasBeenSet(false),
    m_inputSerializationHasBeenSet(false),
    m_outputSerializationHasBeenSet(false),
    m_scanRangeHasBeenSet(false),
    m_customizedAccessLogTagHasBeenSet(false),
    m_decoder(Aws::Utils::Event::EventStreamDecoder(&m_handler))
{
}

Aws::String SelectObjectContentRequest::SerializePayload() const
{
  XmlDocument payloadDoc = XmlDocument::CreateWithRootNode("SelectObjectContentRequest");

  XmlNode parentNode = payloadDoc.GetRootElement();
  parentNode.SetAttributeValue("xmlns", "http://s3.amazonaws.com/doc/2006-03-01/");

  Aws::StringStream ss;
  if(m_expressionHasBeenSet)
  {
   XmlNode expressionNode = parentNode.CreateChildElement("Expression");
   expressionNode.SetText(m_expression);
  }

  if(m_expressionTypeHasBeenSet)
  {
   XmlNode expressionTypeNode = parentNode.CreateChildElement("ExpressionType");
   expressionTypeNode.SetText(ExpressionTypeMapper::GetNameForExpressionType(m_expressionType));
  }

  if(m_requestProgressHasBeenSet)
  {
   XmlNode requestProgressNode = parentNode.CreateChildElement("RequestProgress");
   m_requestProgress.AddToNode(requestProgressNode);
  }

  if(m_inputSerializationHasBeenSet)
  {
   XmlNode inputSerializationNode = parentNode.CreateChildElement("InputSerialization");
   m_inputSerialization.AddToNode(inputSerializationNode);
  }

  if(m_outputSerializationHasBeenSet)
  {
   XmlNode outputSerializationNode = parentNode.CreateChildElement("OutputSerialization");
   m_outputSerialization.AddToNode(outputSerializationNode);
  }

  if(m_scanRangeHasBeenSet)
  {
   XmlNode scanRangeNode = parentNode.CreateChildElement("ScanRange");
   m_scanRange.AddToNode(scanRangeNode);
  }

  return payloadDoc.ConvertToString();
}

void SelectObjectContentRequest::AddQueryStringParameters(URI& uri) const
{
    Aws::StringStream ss;
    if(!m_customizedAccessLogTag.empty())
    {
        // only accept customized LogTag which starts with "x-"
        Aws::Map<Aws::String, Aws::String> collectedLogTags;
        for(const auto& entry: m_customizedAccessLogTag)
        {
            if (!entry.first.empty() && !entry.second.empty() && entry.first.substr(0, 2) == "x-")
            {
                collectedLogTags.emplace(entry.first, entry.second);
            }
        }

        if (!collectedLogTags.empty())
        {
            uri.AddQueryStringParameter(collectedLogTags);
        }
    }
}

Aws::Http::HeaderValueCollection SelectObjectContentRequest::GetRequestSpecificHeaders() const
{
  Aws::Http::HeaderValueCollection headers;
  Aws::StringStream ss;
  if(m_sSECustomerAlgorithmHasBeenSet)
  {
    ss << m_sSECustomerAlgorithm;
    headers.emplace("x-amz-server-side-encryption-customer-algorithm",  ss.str());
    ss.str("");
  }

  if(m_sSECustomerKeyHasBeenSet)
  {
    ss << m_sSECustomerKey;
    headers.emplace("x-amz-server-side-encryption-customer-key",  ss.str());
    ss.str("");
  }

  if(m_sSECustomerKeyMD5HasBeenSet)
  {
    ss << m_sSECustomerKeyMD5;
    headers.emplace("x-amz-server-side-encryption-customer-key-md5",  ss.str());
    ss.str("");
  }

  return headers;
}
