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

#include <aws/s3/model/CopyObjectRequest.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/http/URI.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

#include <utility>

using namespace Aws::S3::Model;
using namespace Aws::Utils::Xml;
using namespace Aws::Utils;
using namespace Aws::Http;

CopyObjectRequest::CopyObjectRequest() : 
    m_aCL(ObjectCannedACL::NOT_SET),
    m_aCLHasBeenSet(false),
    m_bucketHasBeenSet(false),
    m_cacheControlHasBeenSet(false),
    m_contentDispositionHasBeenSet(false),
    m_contentEncodingHasBeenSet(false),
    m_contentLanguageHasBeenSet(false),
    m_contentTypeHasBeenSet(false),
    m_copySourceHasBeenSet(false),
    m_copySourceIfMatchHasBeenSet(false),
    m_copySourceIfModifiedSinceHasBeenSet(false),
    m_copySourceIfNoneMatchHasBeenSet(false),
    m_copySourceIfUnmodifiedSinceHasBeenSet(false),
    m_expiresHasBeenSet(false),
    m_grantFullControlHasBeenSet(false),
    m_grantReadHasBeenSet(false),
    m_grantReadACPHasBeenSet(false),
    m_grantWriteACPHasBeenSet(false),
    m_keyHasBeenSet(false),
    m_metadataHasBeenSet(false),
    m_metadataDirective(MetadataDirective::NOT_SET),
    m_metadataDirectiveHasBeenSet(false),
    m_taggingDirective(TaggingDirective::NOT_SET),
    m_taggingDirectiveHasBeenSet(false),
    m_serverSideEncryption(ServerSideEncryption::NOT_SET),
    m_serverSideEncryptionHasBeenSet(false),
    m_storageClass(StorageClass::NOT_SET),
    m_storageClassHasBeenSet(false),
    m_websiteRedirectLocationHasBeenSet(false),
    m_sSECustomerAlgorithmHasBeenSet(false),
    m_sSECustomerKeyHasBeenSet(false),
    m_sSECustomerKeyMD5HasBeenSet(false),
    m_sSEKMSKeyIdHasBeenSet(false),
    m_sSEKMSEncryptionContextHasBeenSet(false),
    m_copySourceSSECustomerAlgorithmHasBeenSet(false),
    m_copySourceSSECustomerKeyHasBeenSet(false),
    m_copySourceSSECustomerKeyMD5HasBeenSet(false),
    m_requestPayer(RequestPayer::NOT_SET),
    m_requestPayerHasBeenSet(false),
    m_taggingHasBeenSet(false),
    m_objectLockMode(ObjectLockMode::NOT_SET),
    m_objectLockModeHasBeenSet(false),
    m_objectLockRetainUntilDateHasBeenSet(false),
    m_objectLockLegalHoldStatus(ObjectLockLegalHoldStatus::NOT_SET),
    m_objectLockLegalHoldStatusHasBeenSet(false),
    m_customizedAccessLogTagHasBeenSet(false)
{
}

Aws::String CopyObjectRequest::SerializePayload() const
{
  return {};
}

void CopyObjectRequest::AddQueryStringParameters(URI& uri) const
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

Aws::Http::HeaderValueCollection CopyObjectRequest::GetRequestSpecificHeaders() const
{
  Aws::Http::HeaderValueCollection headers;
  Aws::StringStream ss;
  if(m_aCLHasBeenSet)
  {
    headers.emplace("x-amz-acl", ObjectCannedACLMapper::GetNameForObjectCannedACL(m_aCL));
  }

  if(m_cacheControlHasBeenSet)
  {
    ss << m_cacheControl;
    headers.emplace("cache-control",  ss.str());
    ss.str("");
  }

  if(m_contentDispositionHasBeenSet)
  {
    ss << m_contentDisposition;
    headers.emplace("content-disposition",  ss.str());
    ss.str("");
  }

  if(m_contentEncodingHasBeenSet)
  {
    ss << m_contentEncoding;
    headers.emplace("content-encoding",  ss.str());
    ss.str("");
  }

  if(m_contentLanguageHasBeenSet)
  {
    ss << m_contentLanguage;
    headers.emplace("content-language",  ss.str());
    ss.str("");
  }

  if(m_contentTypeHasBeenSet)
  {
    ss << m_contentType;
    headers.emplace("content-type",  ss.str());
    ss.str("");
  }

  if(m_copySourceHasBeenSet)
  {
    ss << m_copySource;
    headers.emplace("x-amz-copy-source", URI::URLEncodePath(ss.str()));
    ss.str("");
  }

  if(m_copySourceIfMatchHasBeenSet)
  {
    ss << m_copySourceIfMatch;
    headers.emplace("x-amz-copy-source-if-match",  ss.str());
    ss.str("");
  }

  if(m_copySourceIfModifiedSinceHasBeenSet)
  {
    headers.emplace("x-amz-copy-source-if-modified-since", m_copySourceIfModifiedSince.ToGmtString(DateFormat::RFC822));
  }

  if(m_copySourceIfNoneMatchHasBeenSet)
  {
    ss << m_copySourceIfNoneMatch;
    headers.emplace("x-amz-copy-source-if-none-match",  ss.str());
    ss.str("");
  }

  if(m_copySourceIfUnmodifiedSinceHasBeenSet)
  {
    headers.emplace("x-amz-copy-source-if-unmodified-since", m_copySourceIfUnmodifiedSince.ToGmtString(DateFormat::RFC822));
  }

  if(m_expiresHasBeenSet)
  {
    headers.emplace("expires", m_expires.ToGmtString(DateFormat::RFC822));
  }

  if(m_grantFullControlHasBeenSet)
  {
    ss << m_grantFullControl;
    headers.emplace("x-amz-grant-full-control",  ss.str());
    ss.str("");
  }

  if(m_grantReadHasBeenSet)
  {
    ss << m_grantRead;
    headers.emplace("x-amz-grant-read",  ss.str());
    ss.str("");
  }

  if(m_grantReadACPHasBeenSet)
  {
    ss << m_grantReadACP;
    headers.emplace("x-amz-grant-read-acp",  ss.str());
    ss.str("");
  }

  if(m_grantWriteACPHasBeenSet)
  {
    ss << m_grantWriteACP;
    headers.emplace("x-amz-grant-write-acp",  ss.str());
    ss.str("");
  }

  if(m_metadataHasBeenSet)
  {
    for(const auto& item : m_metadata)
    {
      ss << "x-amz-meta-" << item.first;
      headers.emplace(ss.str(), item.second);
      ss.str("");
    }
  }

  if(m_metadataDirectiveHasBeenSet)
  {
    headers.emplace("x-amz-metadata-directive", MetadataDirectiveMapper::GetNameForMetadataDirective(m_metadataDirective));
  }

  if(m_taggingDirectiveHasBeenSet)
  {
    headers.emplace("x-amz-tagging-directive", TaggingDirectiveMapper::GetNameForTaggingDirective(m_taggingDirective));
  }

  if(m_serverSideEncryptionHasBeenSet)
  {
    headers.emplace("x-amz-server-side-encryption", ServerSideEncryptionMapper::GetNameForServerSideEncryption(m_serverSideEncryption));
  }

  if(m_storageClassHasBeenSet)
  {
    headers.emplace("x-amz-storage-class", StorageClassMapper::GetNameForStorageClass(m_storageClass));
  }

  if(m_websiteRedirectLocationHasBeenSet)
  {
    ss << m_websiteRedirectLocation;
    headers.emplace("x-amz-website-redirect-location",  ss.str());
    ss.str("");
  }

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

  if(m_sSEKMSKeyIdHasBeenSet)
  {
    ss << m_sSEKMSKeyId;
    headers.emplace("x-amz-server-side-encryption-aws-kms-key-id",  ss.str());
    ss.str("");
  }

  if(m_sSEKMSEncryptionContextHasBeenSet)
  {
    ss << m_sSEKMSEncryptionContext;
    headers.emplace("x-amz-server-side-encryption-context",  ss.str());
    ss.str("");
  }

  if(m_copySourceSSECustomerAlgorithmHasBeenSet)
  {
    ss << m_copySourceSSECustomerAlgorithm;
    headers.emplace("x-amz-copy-source-server-side-encryption-customer-algorithm",  ss.str());
    ss.str("");
  }

  if(m_copySourceSSECustomerKeyHasBeenSet)
  {
    ss << m_copySourceSSECustomerKey;
    headers.emplace("x-amz-copy-source-server-side-encryption-customer-key",  ss.str());
    ss.str("");
  }

  if(m_copySourceSSECustomerKeyMD5HasBeenSet)
  {
    ss << m_copySourceSSECustomerKeyMD5;
    headers.emplace("x-amz-copy-source-server-side-encryption-customer-key-md5",  ss.str());
    ss.str("");
  }

  if(m_requestPayerHasBeenSet)
  {
    headers.emplace("x-amz-request-payer", RequestPayerMapper::GetNameForRequestPayer(m_requestPayer));
  }

  if(m_taggingHasBeenSet)
  {
    ss << m_tagging;
    headers.emplace("x-amz-tagging",  ss.str());
    ss.str("");
  }

  if(m_objectLockModeHasBeenSet)
  {
    headers.emplace("x-amz-object-lock-mode", ObjectLockModeMapper::GetNameForObjectLockMode(m_objectLockMode));
  }

  if(m_objectLockRetainUntilDateHasBeenSet)
  {
    headers.emplace("x-amz-object-lock-retain-until-date", m_objectLockRetainUntilDate.ToGmtString(DateFormat::RFC822));
  }

  if(m_objectLockLegalHoldStatusHasBeenSet)
  {
    headers.emplace("x-amz-object-lock-legal-hold", ObjectLockLegalHoldStatusMapper::GetNameForObjectLockLegalHoldStatus(m_objectLockLegalHoldStatus));
  }

  return headers;
}
