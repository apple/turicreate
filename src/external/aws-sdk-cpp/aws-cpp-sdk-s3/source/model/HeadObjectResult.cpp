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

#include <aws/s3/model/HeadObjectResult.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <aws/core/AmazonWebServiceResult.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

#include <utility>

using namespace Aws::S3::Model;
using namespace Aws::Utils::Xml;
using namespace Aws::Utils;
using namespace Aws;

HeadObjectResult::HeadObjectResult() : 
    m_deleteMarker(false),
    m_contentLength(0),
    m_missingMeta(0),
    m_serverSideEncryption(ServerSideEncryption::NOT_SET),
    m_storageClass(StorageClass::NOT_SET),
    m_requestCharged(RequestCharged::NOT_SET),
    m_replicationStatus(ReplicationStatus::NOT_SET),
    m_partsCount(0),
    m_objectLockMode(ObjectLockMode::NOT_SET),
    m_objectLockLegalHoldStatus(ObjectLockLegalHoldStatus::NOT_SET)
{
}

HeadObjectResult::HeadObjectResult(const Aws::AmazonWebServiceResult<XmlDocument>& result) : 
    m_deleteMarker(false),
    m_contentLength(0),
    m_missingMeta(0),
    m_serverSideEncryption(ServerSideEncryption::NOT_SET),
    m_storageClass(StorageClass::NOT_SET),
    m_requestCharged(RequestCharged::NOT_SET),
    m_replicationStatus(ReplicationStatus::NOT_SET),
    m_partsCount(0),
    m_objectLockMode(ObjectLockMode::NOT_SET),
    m_objectLockLegalHoldStatus(ObjectLockLegalHoldStatus::NOT_SET)
{
  *this = result;
}

HeadObjectResult& HeadObjectResult::operator =(const Aws::AmazonWebServiceResult<XmlDocument>& result)
{
  const XmlDocument& xmlDocument = result.GetPayload();
  XmlNode resultNode = xmlDocument.GetRootElement();

  if(!resultNode.IsNull())
  {
  }

  const auto& headers = result.GetHeaderValueCollection();
  const auto& deleteMarkerIter = headers.find("x-amz-delete-marker");
  if(deleteMarkerIter != headers.end())
  {
     m_deleteMarker = StringUtils::ConvertToBool(deleteMarkerIter->second.c_str());
  }

  const auto& acceptRangesIter = headers.find("accept-ranges");
  if(acceptRangesIter != headers.end())
  {
    m_acceptRanges = acceptRangesIter->second;
  }

  const auto& expirationIter = headers.find("x-amz-expiration");
  if(expirationIter != headers.end())
  {
    m_expiration = expirationIter->second;
  }

  const auto& restoreIter = headers.find("x-amz-restore");
  if(restoreIter != headers.end())
  {
    m_restore = restoreIter->second;
  }

  const auto& lastModifiedIter = headers.find("last-modified");
  if(lastModifiedIter != headers.end())
  {
    m_lastModified = DateTime(lastModifiedIter->second, DateFormat::RFC822);
  }

  const auto& contentLengthIter = headers.find("content-length");
  if(contentLengthIter != headers.end())
  {
     m_contentLength = StringUtils::ConvertToInt64(contentLengthIter->second.c_str());
  }

  const auto& eTagIter = headers.find("etag");
  if(eTagIter != headers.end())
  {
    m_eTag = eTagIter->second;
  }

  const auto& missingMetaIter = headers.find("x-amz-missing-meta");
  if(missingMetaIter != headers.end())
  {
     m_missingMeta = StringUtils::ConvertToInt32(missingMetaIter->second.c_str());
  }

  const auto& versionIdIter = headers.find("x-amz-version-id");
  if(versionIdIter != headers.end())
  {
    m_versionId = versionIdIter->second;
  }

  const auto& cacheControlIter = headers.find("cache-control");
  if(cacheControlIter != headers.end())
  {
    m_cacheControl = cacheControlIter->second;
  }

  const auto& contentDispositionIter = headers.find("content-disposition");
  if(contentDispositionIter != headers.end())
  {
    m_contentDisposition = contentDispositionIter->second;
  }

  const auto& contentEncodingIter = headers.find("content-encoding");
  if(contentEncodingIter != headers.end())
  {
    m_contentEncoding = contentEncodingIter->second;
  }

  const auto& contentLanguageIter = headers.find("content-language");
  if(contentLanguageIter != headers.end())
  {
    m_contentLanguage = contentLanguageIter->second;
  }

  const auto& contentTypeIter = headers.find("content-type");
  if(contentTypeIter != headers.end())
  {
    m_contentType = contentTypeIter->second;
  }

  const auto& expiresIter = headers.find("expires");
  if(expiresIter != headers.end())
  {
    m_expires = DateTime(expiresIter->second, DateFormat::RFC822);
  }

  const auto& websiteRedirectLocationIter = headers.find("x-amz-website-redirect-location");
  if(websiteRedirectLocationIter != headers.end())
  {
    m_websiteRedirectLocation = websiteRedirectLocationIter->second;
  }

  const auto& serverSideEncryptionIter = headers.find("x-amz-server-side-encryption");
  if(serverSideEncryptionIter != headers.end())
  {
    m_serverSideEncryption = ServerSideEncryptionMapper::GetServerSideEncryptionForName(serverSideEncryptionIter->second);
  }

  std::size_t prefixSize = sizeof("x-amz-meta-") - 1; //subtract the NULL terminator out
  for(const auto& item : headers)
  {
    std::size_t foundPrefix = item.first.find("x-amz-meta-");

    if(foundPrefix != std::string::npos)
    {
      m_metadata[item.first.substr(prefixSize)] = item.second;
    }
  }

  const auto& sSECustomerAlgorithmIter = headers.find("x-amz-server-side-encryption-customer-algorithm");
  if(sSECustomerAlgorithmIter != headers.end())
  {
    m_sSECustomerAlgorithm = sSECustomerAlgorithmIter->second;
  }

  const auto& sSECustomerKeyMD5Iter = headers.find("x-amz-server-side-encryption-customer-key-md5");
  if(sSECustomerKeyMD5Iter != headers.end())
  {
    m_sSECustomerKeyMD5 = sSECustomerKeyMD5Iter->second;
  }

  const auto& sSEKMSKeyIdIter = headers.find("x-amz-server-side-encryption-aws-kms-key-id");
  if(sSEKMSKeyIdIter != headers.end())
  {
    m_sSEKMSKeyId = sSEKMSKeyIdIter->second;
  }

  const auto& storageClassIter = headers.find("x-amz-storage-class");
  if(storageClassIter != headers.end())
  {
    m_storageClass = StorageClassMapper::GetStorageClassForName(storageClassIter->second);
  }

  const auto& requestChargedIter = headers.find("x-amz-request-charged");
  if(requestChargedIter != headers.end())
  {
    m_requestCharged = RequestChargedMapper::GetRequestChargedForName(requestChargedIter->second);
  }

  const auto& replicationStatusIter = headers.find("x-amz-replication-status");
  if(replicationStatusIter != headers.end())
  {
    m_replicationStatus = ReplicationStatusMapper::GetReplicationStatusForName(replicationStatusIter->second);
  }

  const auto& partsCountIter = headers.find("x-amz-mp-parts-count");
  if(partsCountIter != headers.end())
  {
     m_partsCount = StringUtils::ConvertToInt32(partsCountIter->second.c_str());
  }

  const auto& objectLockModeIter = headers.find("x-amz-object-lock-mode");
  if(objectLockModeIter != headers.end())
  {
    m_objectLockMode = ObjectLockModeMapper::GetObjectLockModeForName(objectLockModeIter->second);
  }

  const auto& objectLockRetainUntilDateIter = headers.find("x-amz-object-lock-retain-until-date");
  if(objectLockRetainUntilDateIter != headers.end())
  {
    m_objectLockRetainUntilDate = DateTime(objectLockRetainUntilDateIter->second, DateFormat::RFC822);
  }

  const auto& objectLockLegalHoldStatusIter = headers.find("x-amz-object-lock-legal-hold");
  if(objectLockLegalHoldStatusIter != headers.end())
  {
    m_objectLockLegalHoldStatus = ObjectLockLegalHoldStatusMapper::GetObjectLockLegalHoldStatusForName(objectLockLegalHoldStatusIter->second);
  }

  return *this;
}
