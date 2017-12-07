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
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

#include <utility>

using namespace Aws::S3::Model;
using namespace Aws::Utils::Xml;
using namespace Aws::Utils;

CreateBucketRequest::CreateBucketRequest() : 
    m_aCLHasBeenSet(false),
    m_bucketHasBeenSet(false),
    m_createBucketConfigurationHasBeenSet(false),
    m_grantFullControlHasBeenSet(false),
    m_grantReadHasBeenSet(false),
    m_grantReadACPHasBeenSet(false),
    m_grantWriteHasBeenSet(false),
    m_grantWriteACPHasBeenSet(false)
{
}

Aws::String CreateBucketRequest::SerializePayload() const
{
  XmlDocument payloadDoc = XmlDocument::CreateWithRootNode("CreateBucketConfiguration");

  XmlNode parentNode = payloadDoc.GetRootElement();
  parentNode.SetAttributeValue("xmlns", "http://s3.amazonaws.com/doc/2006-03-01/");

  m_createBucketConfiguration.AddToNode(parentNode);
  if(parentNode.HasChildren())
  {
    return payloadDoc.ConvertToString();
  }

  return "";
}


Aws::Http::HeaderValueCollection CreateBucketRequest::GetRequestSpecificHeaders() const
{
  Aws::Http::HeaderValueCollection headers;
  Aws::StringStream ss;
  if(m_aCLHasBeenSet)
  {
    headers.insert(Aws::Http::HeaderValuePair("x-amz-acl", BucketCannedACLMapper::GetNameForBucketCannedACL(m_aCL)));
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

  return headers;
}
