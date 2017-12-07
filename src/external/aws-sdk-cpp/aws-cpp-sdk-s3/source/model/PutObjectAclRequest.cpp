/*
* Copyright 2010-2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <aws/s3/model/PutObjectAclRequest.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/http/URI.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

#include <utility>

using namespace Aws::S3::Model;
using namespace Aws::Utils::Xml;
using namespace Aws::Utils;
using namespace Aws::Http;

PutObjectAclRequest::PutObjectAclRequest() : 
    m_aCLHasBeenSet(false),
    m_accessControlPolicyHasBeenSet(false),
    m_bucketHasBeenSet(false),
    m_contentMD5HasBeenSet(false),
    m_grantFullControlHasBeenSet(false),
    m_grantReadHasBeenSet(false),
    m_grantReadACPHasBeenSet(false),
    m_grantWriteHasBeenSet(false),
    m_grantWriteACPHasBeenSet(false),
    m_keyHasBeenSet(false),
    m_requestPayerHasBeenSet(false),
    m_versionIdHasBeenSet(false)
{
}

Aws::String PutObjectAclRequest::SerializePayload() const
{
  XmlDocument payloadDoc = XmlDocument::CreateWithRootNode("AccessControlPolicy");

  XmlNode parentNode = payloadDoc.GetRootElement();
  parentNode.SetAttributeValue("xmlns", "http://s3.amazonaws.com/doc/2006-03-01/");

  m_accessControlPolicy.AddToNode(parentNode);
  if(parentNode.HasChildren())
  {
    return payloadDoc.ConvertToString();
  }

  return "";
}

void PutObjectAclRequest::AddQueryStringParameters(URI& uri) const
{
    Aws::StringStream ss;
    if(m_versionIdHasBeenSet)
    {
      ss << m_versionId;
      uri.AddQueryStringParameter("versionId", ss.str());
      ss.str("");
    }

}

Aws::Http::HeaderValueCollection PutObjectAclRequest::GetRequestSpecificHeaders() const
{
  Aws::Http::HeaderValueCollection headers;
  Aws::StringStream ss;
  if(m_aCLHasBeenSet)
  {
    headers.insert(Aws::Http::HeaderValuePair("x-amz-acl", ObjectCannedACLMapper::GetNameForObjectCannedACL(m_aCL)));
  }

  if(m_contentMD5HasBeenSet)
  {
    ss << m_contentMD5;
    headers.insert(Aws::Http::HeaderValuePair("content-md5", ss.str()));
    ss.str("");
  }

  if(m_grantFullControlHasBeenSet)
  {
    ss << m_grantFullControl;
    headers.insert(Aws::Http::HeaderValuePair("x-amz-grant-full-control", ss.str()));
    ss.str("");
  }

  if(m_grantReadHasBeenSet)
  {
    ss << m_grantRead;
    headers.insert(Aws::Http::HeaderValuePair("x-amz-grant-read", ss.str()));
    ss.str("");
  }

  if(m_grantReadACPHasBeenSet)
  {
    ss << m_grantReadACP;
    headers.insert(Aws::Http::HeaderValuePair("x-amz-grant-read-acp", ss.str()));
    ss.str("");
  }

  if(m_grantWriteHasBeenSet)
  {
    ss << m_grantWrite;
    headers.insert(Aws::Http::HeaderValuePair("x-amz-grant-write", ss.str()));
    ss.str("");
  }

  if(m_grantWriteACPHasBeenSet)
  {
    ss << m_grantWriteACP;
    headers.insert(Aws::Http::HeaderValuePair("x-amz-grant-write-acp", ss.str()));
    ss.str("");
  }

  if(m_requestPayerHasBeenSet)
  {
    headers.insert(Aws::Http::HeaderValuePair("x-amz-request-payer", RequestPayerMapper::GetNameForRequestPayer(m_requestPayer)));
  }

  return headers;
}
