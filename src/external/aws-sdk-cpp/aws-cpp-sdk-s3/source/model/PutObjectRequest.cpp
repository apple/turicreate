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
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/core/AmazonWebServiceResult.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/utils/HashingUtils.h>

#include <utility>

using namespace Aws::S3::Model;
using namespace Aws::Utils::Stream;
using namespace Aws::Utils;
using namespace Aws;

PutObjectRequest::PutObjectRequest() : 
    m_aCLHasBeenSet(false),
    m_bucketHasBeenSet(false),
    m_cacheControlHasBeenSet(false),
    m_contentDispositionHasBeenSet(false),
    m_contentEncodingHasBeenSet(false),
    m_contentLanguageHasBeenSet(false),
    m_contentLength(0),
    m_contentLengthHasBeenSet(false),
    m_contentMD5HasBeenSet(false),
    m_expiresHasBeenSet(false),
    m_grantFullControlHasBeenSet(false),
    m_grantReadHasBeenSet(false),
    m_grantReadACPHasBeenSet(false),
    m_grantWriteACPHasBeenSet(false),
    m_keyHasBeenSet(false),
    m_metadataHasBeenSet(false),
    m_serverSideEncryptionHasBeenSet(false),
    m_storageClassHasBeenSet(false),
    m_websiteRedirectLocationHasBeenSet(false),
    m_sSECustomerAlgorithmHasBeenSet(false),
    m_sSECustomerKeyHasBeenSet(false),
    m_sSECustomerKeyMD5HasBeenSet(false),
    m_sSEKMSKeyIdHasBeenSet(false),
    m_requestPayerHasBeenSet(false)
{
}


Aws::Http::HeaderValueCollection PutObjectRequest::GetRequestSpecificHeaders() const
{
  Aws::Http::HeaderValueCollection headers;
  Aws::StringStream ss;
  if(m_aCLHasBeenSet)
  {
    headers.insert(Aws::Http::HeaderValuePair("x-amz-acl", ObjectCannedACLMapper::GetNameForObjectCannedACL(m_aCL)));
  }

  if(m_cacheControlHasBeenSet)
  {
    ss << m_cacheControl;
    headers.insert(Aws::Http::HeaderValuePair("cache-control", ss.str()));
    ss.str("");
  }

  if(m_contentDispositionHasBeenSet)
  {
    ss << m_contentDisposition;
    headers.insert(Aws::Http::HeaderValuePair("content-disposition", ss.str()));
    ss.str("");
  }

  if(m_contentEncodingHasBeenSet)
  {
    ss << m_contentEncoding;
    headers.insert(Aws::Http::HeaderValuePair("content-encoding", ss.str()));
    ss.str("");
  }

  if(m_contentLanguageHasBeenSet)
  {
    ss << m_contentLanguage;
    headers.insert(Aws::Http::HeaderValuePair("content-language", ss.str()));
    ss.str("");
  }

  if(m_contentLengthHasBeenSet)
  {
    ss << m_contentLength;
    headers.insert(Aws::Http::HeaderValuePair("content-length", ss.str()));
    ss.str("");
  }

  if(m_contentMD5HasBeenSet)
  {
    ss << m_contentMD5;
    headers.insert(Aws::Http::HeaderValuePair("content-md5", ss.str()));
    ss.str("");
  }

  if(m_expiresHasBeenSet)
  {
    headers.insert(Aws::Http::HeaderValuePair("expires", m_expires.ToGmtString(DateFormat::RFC822)));
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

  if(m_grantWriteACPHasBeenSet)
  {
    ss << m_grantWriteACP;
    headers.insert(Aws::Http::HeaderValuePair("x-amz-grant-write-acp", ss.str()));
    ss.str("");
  }

  if(m_metadataHasBeenSet)
  {
    for(const auto& item : m_metadata)
    {
      ss << "x-amz-meta-" << item.first;
      headers.insert(Aws::Http::HeaderValuePair(ss.str(), item.second));
      ss.str("");
    }
  }

  if(m_serverSideEncryptionHasBeenSet)
  {
    headers.insert(Aws::Http::HeaderValuePair("x-amz-server-side-encryption", ServerSideEncryptionMapper::GetNameForServerSideEncryption(m_serverSideEncryption)));
  }

  if(m_storageClassHasBeenSet)
  {
    headers.insert(Aws::Http::HeaderValuePair("x-amz-storage-class", StorageClassMapper::GetNameForStorageClass(m_storageClass)));
  }

  if(m_websiteRedirectLocationHasBeenSet)
  {
    ss << m_websiteRedirectLocation;
    headers.insert(Aws::Http::HeaderValuePair("x-amz-website-redirect-location", ss.str()));
    ss.str("");
  }

  if(m_sSECustomerAlgorithmHasBeenSet)
  {
    ss << m_sSECustomerAlgorithm;
    headers.insert(Aws::Http::HeaderValuePair("x-amz-server-side-encryption-customer-algorithm", ss.str()));
    ss.str("");
  }

  if(m_sSECustomerKeyHasBeenSet)
  {
    ss << m_sSECustomerKey;
    headers.insert(Aws::Http::HeaderValuePair("x-amz-server-side-encryption-customer-key", ss.str()));
    ss.str("");
  }

  if(m_sSECustomerKeyMD5HasBeenSet)
  {
    ss << m_sSECustomerKeyMD5;
    headers.insert(Aws::Http::HeaderValuePair("x-amz-server-side-encryption-customer-key-md5", ss.str()));
    ss.str("");
  }

  if(m_sSEKMSKeyIdHasBeenSet)
  {
    ss << m_sSEKMSKeyId;
    headers.insert(Aws::Http::HeaderValuePair("x-amz-server-side-encryption-aws-kms-key-id", ss.str()));
    ss.str("");
  }

  if(m_requestPayerHasBeenSet)
  {
    headers.insert(Aws::Http::HeaderValuePair("x-amz-request-payer", RequestPayerMapper::GetNameForRequestPayer(m_requestPayer)));
  }

  return headers;

}
