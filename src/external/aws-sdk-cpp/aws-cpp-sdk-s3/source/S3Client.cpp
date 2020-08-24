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

#include <aws/core/utils/Outcome.h>
#include <aws/core/auth/AWSAuthSigner.h>
#include <aws/core/client/CoreErrors.h>
#include <aws/core/client/RetryStrategy.h>
#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/utils/threading/Executor.h>
#include <aws/core/utils/DNS.h>
#include <aws/core/utils/logging/LogMacros.h>

#include <aws/core/utils/event/EventStream.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/S3Endpoint.h>
#include <aws/s3/S3ErrorMarshaller.h>
#include <aws/s3/S3ARN.h>
#include <aws/s3/model/AbortMultipartUploadRequest.h>
#include <aws/s3/model/CompleteMultipartUploadRequest.h>
#include <aws/s3/model/CopyObjectRequest.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/CreateMultipartUploadRequest.h>
#include <aws/s3/model/DeleteBucketRequest.h>
#include <aws/s3/model/DeleteBucketAnalyticsConfigurationRequest.h>
#include <aws/s3/model/DeleteBucketCorsRequest.h>
#include <aws/s3/model/DeleteBucketEncryptionRequest.h>
#include <aws/s3/model/DeleteBucketInventoryConfigurationRequest.h>
#include <aws/s3/model/DeleteBucketLifecycleRequest.h>
#include <aws/s3/model/DeleteBucketMetricsConfigurationRequest.h>
#include <aws/s3/model/DeleteBucketPolicyRequest.h>
#include <aws/s3/model/DeleteBucketReplicationRequest.h>
#include <aws/s3/model/DeleteBucketTaggingRequest.h>
#include <aws/s3/model/DeleteBucketWebsiteRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/DeleteObjectTaggingRequest.h>
#include <aws/s3/model/DeleteObjectsRequest.h>
#include <aws/s3/model/DeletePublicAccessBlockRequest.h>
#include <aws/s3/model/GetBucketAccelerateConfigurationRequest.h>
#include <aws/s3/model/GetBucketAclRequest.h>
#include <aws/s3/model/GetBucketAnalyticsConfigurationRequest.h>
#include <aws/s3/model/GetBucketCorsRequest.h>
#include <aws/s3/model/GetBucketEncryptionRequest.h>
#include <aws/s3/model/GetBucketInventoryConfigurationRequest.h>
#include <aws/s3/model/GetBucketLifecycleConfigurationRequest.h>
#include <aws/s3/model/GetBucketLocationRequest.h>
#include <aws/s3/model/GetBucketLoggingRequest.h>
#include <aws/s3/model/GetBucketMetricsConfigurationRequest.h>
#include <aws/s3/model/GetBucketNotificationConfigurationRequest.h>
#include <aws/s3/model/GetBucketPolicyRequest.h>
#include <aws/s3/model/GetBucketPolicyStatusRequest.h>
#include <aws/s3/model/GetBucketReplicationRequest.h>
#include <aws/s3/model/GetBucketRequestPaymentRequest.h>
#include <aws/s3/model/GetBucketTaggingRequest.h>
#include <aws/s3/model/GetBucketVersioningRequest.h>
#include <aws/s3/model/GetBucketWebsiteRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/GetObjectAclRequest.h>
#include <aws/s3/model/GetObjectLegalHoldRequest.h>
#include <aws/s3/model/GetObjectLockConfigurationRequest.h>
#include <aws/s3/model/GetObjectRetentionRequest.h>
#include <aws/s3/model/GetObjectTaggingRequest.h>
#include <aws/s3/model/GetObjectTorrentRequest.h>
#include <aws/s3/model/GetPublicAccessBlockRequest.h>
#include <aws/s3/model/HeadBucketRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/ListBucketAnalyticsConfigurationsRequest.h>
#include <aws/s3/model/ListBucketInventoryConfigurationsRequest.h>
#include <aws/s3/model/ListBucketMetricsConfigurationsRequest.h>
#include <aws/s3/model/ListMultipartUploadsRequest.h>
#include <aws/s3/model/ListObjectVersionsRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <aws/s3/model/ListPartsRequest.h>
#include <aws/s3/model/PutBucketAccelerateConfigurationRequest.h>
#include <aws/s3/model/PutBucketAclRequest.h>
#include <aws/s3/model/PutBucketAnalyticsConfigurationRequest.h>
#include <aws/s3/model/PutBucketCorsRequest.h>
#include <aws/s3/model/PutBucketEncryptionRequest.h>
#include <aws/s3/model/PutBucketInventoryConfigurationRequest.h>
#include <aws/s3/model/PutBucketLifecycleConfigurationRequest.h>
#include <aws/s3/model/PutBucketLoggingRequest.h>
#include <aws/s3/model/PutBucketMetricsConfigurationRequest.h>
#include <aws/s3/model/PutBucketNotificationConfigurationRequest.h>
#include <aws/s3/model/PutBucketPolicyRequest.h>
#include <aws/s3/model/PutBucketReplicationRequest.h>
#include <aws/s3/model/PutBucketRequestPaymentRequest.h>
#include <aws/s3/model/PutBucketTaggingRequest.h>
#include <aws/s3/model/PutBucketVersioningRequest.h>
#include <aws/s3/model/PutBucketWebsiteRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/PutObjectAclRequest.h>
#include <aws/s3/model/PutObjectLegalHoldRequest.h>
#include <aws/s3/model/PutObjectLockConfigurationRequest.h>
#include <aws/s3/model/PutObjectRetentionRequest.h>
#include <aws/s3/model/PutObjectTaggingRequest.h>
#include <aws/s3/model/PutPublicAccessBlockRequest.h>
#include <aws/s3/model/RestoreObjectRequest.h>
#include <aws/s3/model/SelectObjectContentRequest.h>
#include <aws/s3/model/UploadPartRequest.h>
#include <aws/s3/model/UploadPartCopyRequest.h>

using namespace Aws;
using namespace Aws::Auth;
using namespace Aws::Client;
using namespace Aws::S3;
using namespace Aws::S3::Model;
using namespace Aws::Http;
using namespace Aws::Utils::Xml;


static const char* SERVICE_NAME = "s3";
static const char* ALLOCATION_TAG = "S3Client";


S3Client::S3Client(const Client::ClientConfiguration& clientConfiguration, Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy signPayloads, bool useVirtualAddressing, Aws::S3::US_EAST_1_REGIONAL_ENDPOINT_OPTION USEast1RegionalEndPointOption) :
  BASECLASS(clientConfiguration,
    Aws::MakeShared<AWSAuthV4Signer>(ALLOCATION_TAG, Aws::MakeShared<DefaultAWSCredentialsProviderChain>(ALLOCATION_TAG),
        SERVICE_NAME, clientConfiguration.region, signPayloads, false),
    Aws::MakeShared<S3ErrorMarshaller>(ALLOCATION_TAG)),
    m_executor(clientConfiguration.executor), m_useVirtualAddressing(useVirtualAddressing), m_USEast1RegionalEndpointOption(USEast1RegionalEndPointOption)
{
  init(clientConfiguration);
}

S3Client::S3Client(const AWSCredentials& credentials, const Client::ClientConfiguration& clientConfiguration, Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy signPayloads, bool useVirtualAddressing, Aws::S3::US_EAST_1_REGIONAL_ENDPOINT_OPTION USEast1RegionalEndPointOption) :
  BASECLASS(clientConfiguration,
    Aws::MakeShared<AWSAuthV4Signer>(ALLOCATION_TAG, Aws::MakeShared<SimpleAWSCredentialsProvider>(ALLOCATION_TAG, credentials),
         SERVICE_NAME, clientConfiguration.region, signPayloads, false),
    Aws::MakeShared<S3ErrorMarshaller>(ALLOCATION_TAG)),
    m_executor(clientConfiguration.executor), m_useVirtualAddressing(useVirtualAddressing), m_USEast1RegionalEndpointOption(USEast1RegionalEndPointOption)
{
  init(clientConfiguration);
}

S3Client::S3Client(const std::shared_ptr<AWSCredentialsProvider>& credentialsProvider,
  const Client::ClientConfiguration& clientConfiguration, Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy signPayloads, bool useVirtualAddressing, Aws::S3::US_EAST_1_REGIONAL_ENDPOINT_OPTION USEast1RegionalEndPointOption) :
  BASECLASS(clientConfiguration,
    Aws::MakeShared<AWSAuthV4Signer>(ALLOCATION_TAG, credentialsProvider,
         SERVICE_NAME, clientConfiguration.region, signPayloads, false),
    Aws::MakeShared<S3ErrorMarshaller>(ALLOCATION_TAG)),
    m_executor(clientConfiguration.executor), m_useVirtualAddressing(useVirtualAddressing), m_USEast1RegionalEndpointOption(USEast1RegionalEndPointOption)
{
  init(clientConfiguration);
}

S3Client::~S3Client()
{
}

void S3Client::init(const ClientConfiguration& config)
{
  LoadS3SpecificConfig(config.profileName);
  m_configScheme = SchemeMapper::ToString(config.scheme);
  m_scheme = m_configScheme;
  m_region = config.region;
  m_useDualStack = config.useDualStack;
  if (config.endpointOverride.empty())
  {
      m_useCustomEndpoint = false;
      m_baseUri = S3Endpoint::ForRegion(config.region, config.useDualStack, m_USEast1RegionalEndpointOption == Aws::S3::US_EAST_1_REGIONAL_ENDPOINT_OPTION::REGIONAL);
  }
  else
  {
      m_useCustomEndpoint = true;
      OverrideEndpoint(config.endpointOverride);
  }
}

void S3Client::OverrideEndpoint(const Aws::String& endpoint)
{
  if (endpoint.compare(0, 7, "http://") == 0)
  {
      m_scheme = "http";
      m_baseUri = endpoint.substr(7);
  }
  else if (endpoint.compare(0, 8, "https://") == 0)
  {
      m_scheme = "https";
      m_baseUri = endpoint.substr(8);
  }
  else
  {
      m_scheme = m_configScheme;
      m_baseUri = endpoint;
  }
}

AbortMultipartUploadOutcome S3Client::AbortMultipartUpload(const AbortMultipartUploadRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("AbortMultipartUpload", "Required field: Bucket, is not set");
    return AbortMultipartUploadOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.KeyHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("AbortMultipartUpload", "Required field: Key, is not set");
    return AbortMultipartUploadOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Key]", false));
  }
  if (!request.UploadIdHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("AbortMultipartUpload", "Required field: UploadId, is not set");
    return AbortMultipartUploadOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [UploadId]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return AbortMultipartUploadOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetKey();
  uri.SetPath(uri.GetPath() + ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_DELETE, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return AbortMultipartUploadOutcome(AbortMultipartUploadResult(outcome.GetResult()));
  }
  else
  {
    return AbortMultipartUploadOutcome(outcome.GetError());
  }
}

AbortMultipartUploadOutcomeCallable S3Client::AbortMultipartUploadCallable(const AbortMultipartUploadRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< AbortMultipartUploadOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->AbortMultipartUpload(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::AbortMultipartUploadAsync(const AbortMultipartUploadRequest& request, const AbortMultipartUploadResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->AbortMultipartUploadAsyncHelper( request, handler, context ); } );
}

void S3Client::AbortMultipartUploadAsyncHelper(const AbortMultipartUploadRequest& request, const AbortMultipartUploadResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, AbortMultipartUpload(request), context);
}

CompleteMultipartUploadOutcome S3Client::CompleteMultipartUpload(const CompleteMultipartUploadRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("CompleteMultipartUpload", "Required field: Bucket, is not set");
    return CompleteMultipartUploadOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.KeyHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("CompleteMultipartUpload", "Required field: Key, is not set");
    return CompleteMultipartUploadOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Key]", false));
  }
  if (!request.UploadIdHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("CompleteMultipartUpload", "Required field: UploadId, is not set");
    return CompleteMultipartUploadOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [UploadId]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return CompleteMultipartUploadOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetKey();
  uri.SetPath(uri.GetPath() + ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_POST, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return CompleteMultipartUploadOutcome(CompleteMultipartUploadResult(outcome.GetResult()));
  }
  else
  {
    return CompleteMultipartUploadOutcome(outcome.GetError());
  }
}

CompleteMultipartUploadOutcomeCallable S3Client::CompleteMultipartUploadCallable(const CompleteMultipartUploadRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< CompleteMultipartUploadOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->CompleteMultipartUpload(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::CompleteMultipartUploadAsync(const CompleteMultipartUploadRequest& request, const CompleteMultipartUploadResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->CompleteMultipartUploadAsyncHelper( request, handler, context ); } );
}

void S3Client::CompleteMultipartUploadAsyncHelper(const CompleteMultipartUploadRequest& request, const CompleteMultipartUploadResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, CompleteMultipartUpload(request), context);
}

CopyObjectOutcome S3Client::CopyObject(const CopyObjectRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("CopyObject", "Required field: Bucket, is not set");
    return CopyObjectOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.CopySourceHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("CopyObject", "Required field: CopySource, is not set");
    return CopyObjectOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [CopySource]", false));
  }
  if (!request.KeyHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("CopyObject", "Required field: Key, is not set");
    return CopyObjectOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Key]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return CopyObjectOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetKey();
  uri.SetPath(uri.GetPath() + ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return CopyObjectOutcome(CopyObjectResult(outcome.GetResult()));
  }
  else
  {
    return CopyObjectOutcome(outcome.GetError());
  }
}

CopyObjectOutcomeCallable S3Client::CopyObjectCallable(const CopyObjectRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< CopyObjectOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->CopyObject(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::CopyObjectAsync(const CopyObjectRequest& request, const CopyObjectResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->CopyObjectAsyncHelper( request, handler, context ); } );
}

void S3Client::CopyObjectAsyncHelper(const CopyObjectRequest& request, const CopyObjectResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, CopyObject(request), context);
}

CreateBucketOutcome S3Client::CreateBucket(const CreateBucketRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("CreateBucket", "Required field: Bucket, is not set");
    return CreateBucketOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString();
  if (!computeEndpointOutcome.IsSuccess())
  {
    return CreateBucketOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetBucket();
  uri.SetPath(uri.GetPath() + ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return CreateBucketOutcome(CreateBucketResult(outcome.GetResult()));
  }
  else
  {
    return CreateBucketOutcome(outcome.GetError());
  }
}

CreateBucketOutcomeCallable S3Client::CreateBucketCallable(const CreateBucketRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< CreateBucketOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->CreateBucket(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::CreateBucketAsync(const CreateBucketRequest& request, const CreateBucketResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->CreateBucketAsyncHelper( request, handler, context ); } );
}

void S3Client::CreateBucketAsyncHelper(const CreateBucketRequest& request, const CreateBucketResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, CreateBucket(request), context);
}

CreateMultipartUploadOutcome S3Client::CreateMultipartUpload(const CreateMultipartUploadRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("CreateMultipartUpload", "Required field: Bucket, is not set");
    return CreateMultipartUploadOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.KeyHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("CreateMultipartUpload", "Required field: Key, is not set");
    return CreateMultipartUploadOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Key]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return CreateMultipartUploadOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetKey();
  uri.SetPath(uri.GetPath() + ss.str());
  ss.str("?uploads");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_POST, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return CreateMultipartUploadOutcome(CreateMultipartUploadResult(outcome.GetResult()));
  }
  else
  {
    return CreateMultipartUploadOutcome(outcome.GetError());
  }
}

CreateMultipartUploadOutcomeCallable S3Client::CreateMultipartUploadCallable(const CreateMultipartUploadRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< CreateMultipartUploadOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->CreateMultipartUpload(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::CreateMultipartUploadAsync(const CreateMultipartUploadRequest& request, const CreateMultipartUploadResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->CreateMultipartUploadAsyncHelper( request, handler, context ); } );
}

void S3Client::CreateMultipartUploadAsyncHelper(const CreateMultipartUploadRequest& request, const CreateMultipartUploadResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, CreateMultipartUpload(request), context);
}

DeleteBucketOutcome S3Client::DeleteBucket(const DeleteBucketRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("DeleteBucket", "Required field: Bucket, is not set");
    return DeleteBucketOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return DeleteBucketOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  uri.SetPath(uri.GetPath() + ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_DELETE, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return DeleteBucketOutcome(NoResult());
  }
  else
  {
    return DeleteBucketOutcome(outcome.GetError());
  }
}

DeleteBucketOutcomeCallable S3Client::DeleteBucketCallable(const DeleteBucketRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< DeleteBucketOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->DeleteBucket(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::DeleteBucketAsync(const DeleteBucketRequest& request, const DeleteBucketResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->DeleteBucketAsyncHelper( request, handler, context ); } );
}

void S3Client::DeleteBucketAsyncHelper(const DeleteBucketRequest& request, const DeleteBucketResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, DeleteBucket(request), context);
}

DeleteBucketAnalyticsConfigurationOutcome S3Client::DeleteBucketAnalyticsConfiguration(const DeleteBucketAnalyticsConfigurationRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("DeleteBucketAnalyticsConfiguration", "Required field: Bucket, is not set");
    return DeleteBucketAnalyticsConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.IdHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("DeleteBucketAnalyticsConfiguration", "Required field: Id, is not set");
    return DeleteBucketAnalyticsConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Id]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return DeleteBucketAnalyticsConfigurationOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?analytics");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_DELETE, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return DeleteBucketAnalyticsConfigurationOutcome(NoResult());
  }
  else
  {
    return DeleteBucketAnalyticsConfigurationOutcome(outcome.GetError());
  }
}

DeleteBucketAnalyticsConfigurationOutcomeCallable S3Client::DeleteBucketAnalyticsConfigurationCallable(const DeleteBucketAnalyticsConfigurationRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< DeleteBucketAnalyticsConfigurationOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->DeleteBucketAnalyticsConfiguration(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::DeleteBucketAnalyticsConfigurationAsync(const DeleteBucketAnalyticsConfigurationRequest& request, const DeleteBucketAnalyticsConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->DeleteBucketAnalyticsConfigurationAsyncHelper( request, handler, context ); } );
}

void S3Client::DeleteBucketAnalyticsConfigurationAsyncHelper(const DeleteBucketAnalyticsConfigurationRequest& request, const DeleteBucketAnalyticsConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, DeleteBucketAnalyticsConfiguration(request), context);
}

DeleteBucketCorsOutcome S3Client::DeleteBucketCors(const DeleteBucketCorsRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("DeleteBucketCors", "Required field: Bucket, is not set");
    return DeleteBucketCorsOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return DeleteBucketCorsOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?cors");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_DELETE, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return DeleteBucketCorsOutcome(NoResult());
  }
  else
  {
    return DeleteBucketCorsOutcome(outcome.GetError());
  }
}

DeleteBucketCorsOutcomeCallable S3Client::DeleteBucketCorsCallable(const DeleteBucketCorsRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< DeleteBucketCorsOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->DeleteBucketCors(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::DeleteBucketCorsAsync(const DeleteBucketCorsRequest& request, const DeleteBucketCorsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->DeleteBucketCorsAsyncHelper( request, handler, context ); } );
}

void S3Client::DeleteBucketCorsAsyncHelper(const DeleteBucketCorsRequest& request, const DeleteBucketCorsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, DeleteBucketCors(request), context);
}

DeleteBucketEncryptionOutcome S3Client::DeleteBucketEncryption(const DeleteBucketEncryptionRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("DeleteBucketEncryption", "Required field: Bucket, is not set");
    return DeleteBucketEncryptionOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return DeleteBucketEncryptionOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?encryption");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_DELETE, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return DeleteBucketEncryptionOutcome(NoResult());
  }
  else
  {
    return DeleteBucketEncryptionOutcome(outcome.GetError());
  }
}

DeleteBucketEncryptionOutcomeCallable S3Client::DeleteBucketEncryptionCallable(const DeleteBucketEncryptionRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< DeleteBucketEncryptionOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->DeleteBucketEncryption(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::DeleteBucketEncryptionAsync(const DeleteBucketEncryptionRequest& request, const DeleteBucketEncryptionResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->DeleteBucketEncryptionAsyncHelper( request, handler, context ); } );
}

void S3Client::DeleteBucketEncryptionAsyncHelper(const DeleteBucketEncryptionRequest& request, const DeleteBucketEncryptionResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, DeleteBucketEncryption(request), context);
}

DeleteBucketInventoryConfigurationOutcome S3Client::DeleteBucketInventoryConfiguration(const DeleteBucketInventoryConfigurationRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("DeleteBucketInventoryConfiguration", "Required field: Bucket, is not set");
    return DeleteBucketInventoryConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.IdHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("DeleteBucketInventoryConfiguration", "Required field: Id, is not set");
    return DeleteBucketInventoryConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Id]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return DeleteBucketInventoryConfigurationOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?inventory");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_DELETE, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return DeleteBucketInventoryConfigurationOutcome(NoResult());
  }
  else
  {
    return DeleteBucketInventoryConfigurationOutcome(outcome.GetError());
  }
}

DeleteBucketInventoryConfigurationOutcomeCallable S3Client::DeleteBucketInventoryConfigurationCallable(const DeleteBucketInventoryConfigurationRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< DeleteBucketInventoryConfigurationOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->DeleteBucketInventoryConfiguration(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::DeleteBucketInventoryConfigurationAsync(const DeleteBucketInventoryConfigurationRequest& request, const DeleteBucketInventoryConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->DeleteBucketInventoryConfigurationAsyncHelper( request, handler, context ); } );
}

void S3Client::DeleteBucketInventoryConfigurationAsyncHelper(const DeleteBucketInventoryConfigurationRequest& request, const DeleteBucketInventoryConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, DeleteBucketInventoryConfiguration(request), context);
}

DeleteBucketLifecycleOutcome S3Client::DeleteBucketLifecycle(const DeleteBucketLifecycleRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("DeleteBucketLifecycle", "Required field: Bucket, is not set");
    return DeleteBucketLifecycleOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return DeleteBucketLifecycleOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?lifecycle");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_DELETE, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return DeleteBucketLifecycleOutcome(NoResult());
  }
  else
  {
    return DeleteBucketLifecycleOutcome(outcome.GetError());
  }
}

DeleteBucketLifecycleOutcomeCallable S3Client::DeleteBucketLifecycleCallable(const DeleteBucketLifecycleRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< DeleteBucketLifecycleOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->DeleteBucketLifecycle(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::DeleteBucketLifecycleAsync(const DeleteBucketLifecycleRequest& request, const DeleteBucketLifecycleResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->DeleteBucketLifecycleAsyncHelper( request, handler, context ); } );
}

void S3Client::DeleteBucketLifecycleAsyncHelper(const DeleteBucketLifecycleRequest& request, const DeleteBucketLifecycleResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, DeleteBucketLifecycle(request), context);
}

DeleteBucketMetricsConfigurationOutcome S3Client::DeleteBucketMetricsConfiguration(const DeleteBucketMetricsConfigurationRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("DeleteBucketMetricsConfiguration", "Required field: Bucket, is not set");
    return DeleteBucketMetricsConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.IdHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("DeleteBucketMetricsConfiguration", "Required field: Id, is not set");
    return DeleteBucketMetricsConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Id]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return DeleteBucketMetricsConfigurationOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?metrics");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_DELETE, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return DeleteBucketMetricsConfigurationOutcome(NoResult());
  }
  else
  {
    return DeleteBucketMetricsConfigurationOutcome(outcome.GetError());
  }
}

DeleteBucketMetricsConfigurationOutcomeCallable S3Client::DeleteBucketMetricsConfigurationCallable(const DeleteBucketMetricsConfigurationRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< DeleteBucketMetricsConfigurationOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->DeleteBucketMetricsConfiguration(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::DeleteBucketMetricsConfigurationAsync(const DeleteBucketMetricsConfigurationRequest& request, const DeleteBucketMetricsConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->DeleteBucketMetricsConfigurationAsyncHelper( request, handler, context ); } );
}

void S3Client::DeleteBucketMetricsConfigurationAsyncHelper(const DeleteBucketMetricsConfigurationRequest& request, const DeleteBucketMetricsConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, DeleteBucketMetricsConfiguration(request), context);
}

DeleteBucketPolicyOutcome S3Client::DeleteBucketPolicy(const DeleteBucketPolicyRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("DeleteBucketPolicy", "Required field: Bucket, is not set");
    return DeleteBucketPolicyOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return DeleteBucketPolicyOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?policy");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_DELETE, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return DeleteBucketPolicyOutcome(NoResult());
  }
  else
  {
    return DeleteBucketPolicyOutcome(outcome.GetError());
  }
}

DeleteBucketPolicyOutcomeCallable S3Client::DeleteBucketPolicyCallable(const DeleteBucketPolicyRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< DeleteBucketPolicyOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->DeleteBucketPolicy(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::DeleteBucketPolicyAsync(const DeleteBucketPolicyRequest& request, const DeleteBucketPolicyResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->DeleteBucketPolicyAsyncHelper( request, handler, context ); } );
}

void S3Client::DeleteBucketPolicyAsyncHelper(const DeleteBucketPolicyRequest& request, const DeleteBucketPolicyResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, DeleteBucketPolicy(request), context);
}

DeleteBucketReplicationOutcome S3Client::DeleteBucketReplication(const DeleteBucketReplicationRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("DeleteBucketReplication", "Required field: Bucket, is not set");
    return DeleteBucketReplicationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return DeleteBucketReplicationOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?replication");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_DELETE, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return DeleteBucketReplicationOutcome(NoResult());
  }
  else
  {
    return DeleteBucketReplicationOutcome(outcome.GetError());
  }
}

DeleteBucketReplicationOutcomeCallable S3Client::DeleteBucketReplicationCallable(const DeleteBucketReplicationRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< DeleteBucketReplicationOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->DeleteBucketReplication(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::DeleteBucketReplicationAsync(const DeleteBucketReplicationRequest& request, const DeleteBucketReplicationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->DeleteBucketReplicationAsyncHelper( request, handler, context ); } );
}

void S3Client::DeleteBucketReplicationAsyncHelper(const DeleteBucketReplicationRequest& request, const DeleteBucketReplicationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, DeleteBucketReplication(request), context);
}

DeleteBucketTaggingOutcome S3Client::DeleteBucketTagging(const DeleteBucketTaggingRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("DeleteBucketTagging", "Required field: Bucket, is not set");
    return DeleteBucketTaggingOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return DeleteBucketTaggingOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?tagging");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_DELETE, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return DeleteBucketTaggingOutcome(NoResult());
  }
  else
  {
    return DeleteBucketTaggingOutcome(outcome.GetError());
  }
}

DeleteBucketTaggingOutcomeCallable S3Client::DeleteBucketTaggingCallable(const DeleteBucketTaggingRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< DeleteBucketTaggingOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->DeleteBucketTagging(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::DeleteBucketTaggingAsync(const DeleteBucketTaggingRequest& request, const DeleteBucketTaggingResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->DeleteBucketTaggingAsyncHelper( request, handler, context ); } );
}

void S3Client::DeleteBucketTaggingAsyncHelper(const DeleteBucketTaggingRequest& request, const DeleteBucketTaggingResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, DeleteBucketTagging(request), context);
}

DeleteBucketWebsiteOutcome S3Client::DeleteBucketWebsite(const DeleteBucketWebsiteRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("DeleteBucketWebsite", "Required field: Bucket, is not set");
    return DeleteBucketWebsiteOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return DeleteBucketWebsiteOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?website");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_DELETE, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return DeleteBucketWebsiteOutcome(NoResult());
  }
  else
  {
    return DeleteBucketWebsiteOutcome(outcome.GetError());
  }
}

DeleteBucketWebsiteOutcomeCallable S3Client::DeleteBucketWebsiteCallable(const DeleteBucketWebsiteRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< DeleteBucketWebsiteOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->DeleteBucketWebsite(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::DeleteBucketWebsiteAsync(const DeleteBucketWebsiteRequest& request, const DeleteBucketWebsiteResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->DeleteBucketWebsiteAsyncHelper( request, handler, context ); } );
}

void S3Client::DeleteBucketWebsiteAsyncHelper(const DeleteBucketWebsiteRequest& request, const DeleteBucketWebsiteResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, DeleteBucketWebsite(request), context);
}

DeleteObjectOutcome S3Client::DeleteObject(const DeleteObjectRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("DeleteObject", "Required field: Bucket, is not set");
    return DeleteObjectOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.KeyHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("DeleteObject", "Required field: Key, is not set");
    return DeleteObjectOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Key]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return DeleteObjectOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetKey();
  uri.SetPath(uri.GetPath() + ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_DELETE, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return DeleteObjectOutcome(DeleteObjectResult(outcome.GetResult()));
  }
  else
  {
    return DeleteObjectOutcome(outcome.GetError());
  }
}

DeleteObjectOutcomeCallable S3Client::DeleteObjectCallable(const DeleteObjectRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< DeleteObjectOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->DeleteObject(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::DeleteObjectAsync(const DeleteObjectRequest& request, const DeleteObjectResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->DeleteObjectAsyncHelper( request, handler, context ); } );
}

void S3Client::DeleteObjectAsyncHelper(const DeleteObjectRequest& request, const DeleteObjectResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, DeleteObject(request), context);
}

DeleteObjectTaggingOutcome S3Client::DeleteObjectTagging(const DeleteObjectTaggingRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("DeleteObjectTagging", "Required field: Bucket, is not set");
    return DeleteObjectTaggingOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.KeyHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("DeleteObjectTagging", "Required field: Key, is not set");
    return DeleteObjectTaggingOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Key]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return DeleteObjectTaggingOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetKey();
  uri.SetPath(uri.GetPath() + ss.str());
  ss.str("?tagging");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_DELETE, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return DeleteObjectTaggingOutcome(DeleteObjectTaggingResult(outcome.GetResult()));
  }
  else
  {
    return DeleteObjectTaggingOutcome(outcome.GetError());
  }
}

DeleteObjectTaggingOutcomeCallable S3Client::DeleteObjectTaggingCallable(const DeleteObjectTaggingRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< DeleteObjectTaggingOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->DeleteObjectTagging(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::DeleteObjectTaggingAsync(const DeleteObjectTaggingRequest& request, const DeleteObjectTaggingResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->DeleteObjectTaggingAsyncHelper( request, handler, context ); } );
}

void S3Client::DeleteObjectTaggingAsyncHelper(const DeleteObjectTaggingRequest& request, const DeleteObjectTaggingResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, DeleteObjectTagging(request), context);
}

DeleteObjectsOutcome S3Client::DeleteObjects(const DeleteObjectsRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("DeleteObjects", "Required field: Bucket, is not set");
    return DeleteObjectsOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return DeleteObjectsOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?delete");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_POST, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return DeleteObjectsOutcome(DeleteObjectsResult(outcome.GetResult()));
  }
  else
  {
    return DeleteObjectsOutcome(outcome.GetError());
  }
}

DeleteObjectsOutcomeCallable S3Client::DeleteObjectsCallable(const DeleteObjectsRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< DeleteObjectsOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->DeleteObjects(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::DeleteObjectsAsync(const DeleteObjectsRequest& request, const DeleteObjectsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->DeleteObjectsAsyncHelper( request, handler, context ); } );
}

void S3Client::DeleteObjectsAsyncHelper(const DeleteObjectsRequest& request, const DeleteObjectsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, DeleteObjects(request), context);
}

DeletePublicAccessBlockOutcome S3Client::DeletePublicAccessBlock(const DeletePublicAccessBlockRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("DeletePublicAccessBlock", "Required field: Bucket, is not set");
    return DeletePublicAccessBlockOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return DeletePublicAccessBlockOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?publicAccessBlock");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_DELETE, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return DeletePublicAccessBlockOutcome(NoResult());
  }
  else
  {
    return DeletePublicAccessBlockOutcome(outcome.GetError());
  }
}

DeletePublicAccessBlockOutcomeCallable S3Client::DeletePublicAccessBlockCallable(const DeletePublicAccessBlockRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< DeletePublicAccessBlockOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->DeletePublicAccessBlock(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::DeletePublicAccessBlockAsync(const DeletePublicAccessBlockRequest& request, const DeletePublicAccessBlockResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->DeletePublicAccessBlockAsyncHelper( request, handler, context ); } );
}

void S3Client::DeletePublicAccessBlockAsyncHelper(const DeletePublicAccessBlockRequest& request, const DeletePublicAccessBlockResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, DeletePublicAccessBlock(request), context);
}

GetBucketAccelerateConfigurationOutcome S3Client::GetBucketAccelerateConfiguration(const GetBucketAccelerateConfigurationRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetBucketAccelerateConfiguration", "Required field: Bucket, is not set");
    return GetBucketAccelerateConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetBucketAccelerateConfigurationOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?accelerate");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetBucketAccelerateConfigurationOutcome(GetBucketAccelerateConfigurationResult(outcome.GetResult()));
  }
  else
  {
    return GetBucketAccelerateConfigurationOutcome(outcome.GetError());
  }
}

GetBucketAccelerateConfigurationOutcomeCallable S3Client::GetBucketAccelerateConfigurationCallable(const GetBucketAccelerateConfigurationRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetBucketAccelerateConfigurationOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetBucketAccelerateConfiguration(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetBucketAccelerateConfigurationAsync(const GetBucketAccelerateConfigurationRequest& request, const GetBucketAccelerateConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetBucketAccelerateConfigurationAsyncHelper( request, handler, context ); } );
}

void S3Client::GetBucketAccelerateConfigurationAsyncHelper(const GetBucketAccelerateConfigurationRequest& request, const GetBucketAccelerateConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetBucketAccelerateConfiguration(request), context);
}

GetBucketAclOutcome S3Client::GetBucketAcl(const GetBucketAclRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetBucketAcl", "Required field: Bucket, is not set");
    return GetBucketAclOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetBucketAclOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?acl");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetBucketAclOutcome(GetBucketAclResult(outcome.GetResult()));
  }
  else
  {
    return GetBucketAclOutcome(outcome.GetError());
  }
}

GetBucketAclOutcomeCallable S3Client::GetBucketAclCallable(const GetBucketAclRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetBucketAclOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetBucketAcl(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetBucketAclAsync(const GetBucketAclRequest& request, const GetBucketAclResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetBucketAclAsyncHelper( request, handler, context ); } );
}

void S3Client::GetBucketAclAsyncHelper(const GetBucketAclRequest& request, const GetBucketAclResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetBucketAcl(request), context);
}

GetBucketAnalyticsConfigurationOutcome S3Client::GetBucketAnalyticsConfiguration(const GetBucketAnalyticsConfigurationRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetBucketAnalyticsConfiguration", "Required field: Bucket, is not set");
    return GetBucketAnalyticsConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.IdHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetBucketAnalyticsConfiguration", "Required field: Id, is not set");
    return GetBucketAnalyticsConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Id]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetBucketAnalyticsConfigurationOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?analytics");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetBucketAnalyticsConfigurationOutcome(GetBucketAnalyticsConfigurationResult(outcome.GetResult()));
  }
  else
  {
    return GetBucketAnalyticsConfigurationOutcome(outcome.GetError());
  }
}

GetBucketAnalyticsConfigurationOutcomeCallable S3Client::GetBucketAnalyticsConfigurationCallable(const GetBucketAnalyticsConfigurationRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetBucketAnalyticsConfigurationOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetBucketAnalyticsConfiguration(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetBucketAnalyticsConfigurationAsync(const GetBucketAnalyticsConfigurationRequest& request, const GetBucketAnalyticsConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetBucketAnalyticsConfigurationAsyncHelper( request, handler, context ); } );
}

void S3Client::GetBucketAnalyticsConfigurationAsyncHelper(const GetBucketAnalyticsConfigurationRequest& request, const GetBucketAnalyticsConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetBucketAnalyticsConfiguration(request), context);
}

GetBucketCorsOutcome S3Client::GetBucketCors(const GetBucketCorsRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetBucketCors", "Required field: Bucket, is not set");
    return GetBucketCorsOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetBucketCorsOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?cors");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetBucketCorsOutcome(GetBucketCorsResult(outcome.GetResult()));
  }
  else
  {
    return GetBucketCorsOutcome(outcome.GetError());
  }
}

GetBucketCorsOutcomeCallable S3Client::GetBucketCorsCallable(const GetBucketCorsRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetBucketCorsOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetBucketCors(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetBucketCorsAsync(const GetBucketCorsRequest& request, const GetBucketCorsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetBucketCorsAsyncHelper( request, handler, context ); } );
}

void S3Client::GetBucketCorsAsyncHelper(const GetBucketCorsRequest& request, const GetBucketCorsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetBucketCors(request), context);
}

GetBucketEncryptionOutcome S3Client::GetBucketEncryption(const GetBucketEncryptionRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetBucketEncryption", "Required field: Bucket, is not set");
    return GetBucketEncryptionOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetBucketEncryptionOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?encryption");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetBucketEncryptionOutcome(GetBucketEncryptionResult(outcome.GetResult()));
  }
  else
  {
    return GetBucketEncryptionOutcome(outcome.GetError());
  }
}

GetBucketEncryptionOutcomeCallable S3Client::GetBucketEncryptionCallable(const GetBucketEncryptionRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetBucketEncryptionOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetBucketEncryption(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetBucketEncryptionAsync(const GetBucketEncryptionRequest& request, const GetBucketEncryptionResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetBucketEncryptionAsyncHelper( request, handler, context ); } );
}

void S3Client::GetBucketEncryptionAsyncHelper(const GetBucketEncryptionRequest& request, const GetBucketEncryptionResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetBucketEncryption(request), context);
}

GetBucketInventoryConfigurationOutcome S3Client::GetBucketInventoryConfiguration(const GetBucketInventoryConfigurationRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetBucketInventoryConfiguration", "Required field: Bucket, is not set");
    return GetBucketInventoryConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.IdHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetBucketInventoryConfiguration", "Required field: Id, is not set");
    return GetBucketInventoryConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Id]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetBucketInventoryConfigurationOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?inventory");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetBucketInventoryConfigurationOutcome(GetBucketInventoryConfigurationResult(outcome.GetResult()));
  }
  else
  {
    return GetBucketInventoryConfigurationOutcome(outcome.GetError());
  }
}

GetBucketInventoryConfigurationOutcomeCallable S3Client::GetBucketInventoryConfigurationCallable(const GetBucketInventoryConfigurationRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetBucketInventoryConfigurationOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetBucketInventoryConfiguration(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetBucketInventoryConfigurationAsync(const GetBucketInventoryConfigurationRequest& request, const GetBucketInventoryConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetBucketInventoryConfigurationAsyncHelper( request, handler, context ); } );
}

void S3Client::GetBucketInventoryConfigurationAsyncHelper(const GetBucketInventoryConfigurationRequest& request, const GetBucketInventoryConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetBucketInventoryConfiguration(request), context);
}

GetBucketLifecycleConfigurationOutcome S3Client::GetBucketLifecycleConfiguration(const GetBucketLifecycleConfigurationRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetBucketLifecycleConfiguration", "Required field: Bucket, is not set");
    return GetBucketLifecycleConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetBucketLifecycleConfigurationOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?lifecycle");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetBucketLifecycleConfigurationOutcome(GetBucketLifecycleConfigurationResult(outcome.GetResult()));
  }
  else
  {
    return GetBucketLifecycleConfigurationOutcome(outcome.GetError());
  }
}

GetBucketLifecycleConfigurationOutcomeCallable S3Client::GetBucketLifecycleConfigurationCallable(const GetBucketLifecycleConfigurationRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetBucketLifecycleConfigurationOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetBucketLifecycleConfiguration(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetBucketLifecycleConfigurationAsync(const GetBucketLifecycleConfigurationRequest& request, const GetBucketLifecycleConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetBucketLifecycleConfigurationAsyncHelper( request, handler, context ); } );
}

void S3Client::GetBucketLifecycleConfigurationAsyncHelper(const GetBucketLifecycleConfigurationRequest& request, const GetBucketLifecycleConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetBucketLifecycleConfiguration(request), context);
}

GetBucketLocationOutcome S3Client::GetBucketLocation(const GetBucketLocationRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetBucketLocation", "Required field: Bucket, is not set");
    return GetBucketLocationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetBucketLocationOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?location");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetBucketLocationOutcome(GetBucketLocationResult(outcome.GetResult()));
  }
  else
  {
    return GetBucketLocationOutcome(outcome.GetError());
  }
}

GetBucketLocationOutcomeCallable S3Client::GetBucketLocationCallable(const GetBucketLocationRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetBucketLocationOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetBucketLocation(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetBucketLocationAsync(const GetBucketLocationRequest& request, const GetBucketLocationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetBucketLocationAsyncHelper( request, handler, context ); } );
}

void S3Client::GetBucketLocationAsyncHelper(const GetBucketLocationRequest& request, const GetBucketLocationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetBucketLocation(request), context);
}

GetBucketLoggingOutcome S3Client::GetBucketLogging(const GetBucketLoggingRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetBucketLogging", "Required field: Bucket, is not set");
    return GetBucketLoggingOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetBucketLoggingOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?logging");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetBucketLoggingOutcome(GetBucketLoggingResult(outcome.GetResult()));
  }
  else
  {
    return GetBucketLoggingOutcome(outcome.GetError());
  }
}

GetBucketLoggingOutcomeCallable S3Client::GetBucketLoggingCallable(const GetBucketLoggingRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetBucketLoggingOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetBucketLogging(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetBucketLoggingAsync(const GetBucketLoggingRequest& request, const GetBucketLoggingResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetBucketLoggingAsyncHelper( request, handler, context ); } );
}

void S3Client::GetBucketLoggingAsyncHelper(const GetBucketLoggingRequest& request, const GetBucketLoggingResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetBucketLogging(request), context);
}

GetBucketMetricsConfigurationOutcome S3Client::GetBucketMetricsConfiguration(const GetBucketMetricsConfigurationRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetBucketMetricsConfiguration", "Required field: Bucket, is not set");
    return GetBucketMetricsConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.IdHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetBucketMetricsConfiguration", "Required field: Id, is not set");
    return GetBucketMetricsConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Id]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetBucketMetricsConfigurationOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?metrics");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetBucketMetricsConfigurationOutcome(GetBucketMetricsConfigurationResult(outcome.GetResult()));
  }
  else
  {
    return GetBucketMetricsConfigurationOutcome(outcome.GetError());
  }
}

GetBucketMetricsConfigurationOutcomeCallable S3Client::GetBucketMetricsConfigurationCallable(const GetBucketMetricsConfigurationRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetBucketMetricsConfigurationOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetBucketMetricsConfiguration(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetBucketMetricsConfigurationAsync(const GetBucketMetricsConfigurationRequest& request, const GetBucketMetricsConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetBucketMetricsConfigurationAsyncHelper( request, handler, context ); } );
}

void S3Client::GetBucketMetricsConfigurationAsyncHelper(const GetBucketMetricsConfigurationRequest& request, const GetBucketMetricsConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetBucketMetricsConfiguration(request), context);
}

GetBucketNotificationConfigurationOutcome S3Client::GetBucketNotificationConfiguration(const GetBucketNotificationConfigurationRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetBucketNotificationConfiguration", "Required field: Bucket, is not set");
    return GetBucketNotificationConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetBucketNotificationConfigurationOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?notification");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetBucketNotificationConfigurationOutcome(GetBucketNotificationConfigurationResult(outcome.GetResult()));
  }
  else
  {
    return GetBucketNotificationConfigurationOutcome(outcome.GetError());
  }
}

GetBucketNotificationConfigurationOutcomeCallable S3Client::GetBucketNotificationConfigurationCallable(const GetBucketNotificationConfigurationRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetBucketNotificationConfigurationOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetBucketNotificationConfiguration(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetBucketNotificationConfigurationAsync(const GetBucketNotificationConfigurationRequest& request, const GetBucketNotificationConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetBucketNotificationConfigurationAsyncHelper( request, handler, context ); } );
}

void S3Client::GetBucketNotificationConfigurationAsyncHelper(const GetBucketNotificationConfigurationRequest& request, const GetBucketNotificationConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetBucketNotificationConfiguration(request), context);
}

GetBucketPolicyOutcome S3Client::GetBucketPolicy(const GetBucketPolicyRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetBucketPolicy", "Required field: Bucket, is not set");
    return GetBucketPolicyOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetBucketPolicyOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?policy");
  uri.SetQueryString(ss.str());
  StreamOutcome outcome = MakeRequestWithUnparsedResponse(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetBucketPolicyOutcome(GetBucketPolicyResult(outcome.GetResultWithOwnership()));
  }
  else
  {
    return GetBucketPolicyOutcome(outcome.GetError());
  }
}

GetBucketPolicyOutcomeCallable S3Client::GetBucketPolicyCallable(const GetBucketPolicyRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetBucketPolicyOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetBucketPolicy(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetBucketPolicyAsync(const GetBucketPolicyRequest& request, const GetBucketPolicyResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetBucketPolicyAsyncHelper( request, handler, context ); } );
}

void S3Client::GetBucketPolicyAsyncHelper(const GetBucketPolicyRequest& request, const GetBucketPolicyResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetBucketPolicy(request), context);
}

GetBucketPolicyStatusOutcome S3Client::GetBucketPolicyStatus(const GetBucketPolicyStatusRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetBucketPolicyStatus", "Required field: Bucket, is not set");
    return GetBucketPolicyStatusOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetBucketPolicyStatusOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?policyStatus");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetBucketPolicyStatusOutcome(GetBucketPolicyStatusResult(outcome.GetResult()));
  }
  else
  {
    return GetBucketPolicyStatusOutcome(outcome.GetError());
  }
}

GetBucketPolicyStatusOutcomeCallable S3Client::GetBucketPolicyStatusCallable(const GetBucketPolicyStatusRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetBucketPolicyStatusOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetBucketPolicyStatus(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetBucketPolicyStatusAsync(const GetBucketPolicyStatusRequest& request, const GetBucketPolicyStatusResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetBucketPolicyStatusAsyncHelper( request, handler, context ); } );
}

void S3Client::GetBucketPolicyStatusAsyncHelper(const GetBucketPolicyStatusRequest& request, const GetBucketPolicyStatusResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetBucketPolicyStatus(request), context);
}

GetBucketReplicationOutcome S3Client::GetBucketReplication(const GetBucketReplicationRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetBucketReplication", "Required field: Bucket, is not set");
    return GetBucketReplicationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetBucketReplicationOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?replication");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetBucketReplicationOutcome(GetBucketReplicationResult(outcome.GetResult()));
  }
  else
  {
    return GetBucketReplicationOutcome(outcome.GetError());
  }
}

GetBucketReplicationOutcomeCallable S3Client::GetBucketReplicationCallable(const GetBucketReplicationRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetBucketReplicationOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetBucketReplication(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetBucketReplicationAsync(const GetBucketReplicationRequest& request, const GetBucketReplicationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetBucketReplicationAsyncHelper( request, handler, context ); } );
}

void S3Client::GetBucketReplicationAsyncHelper(const GetBucketReplicationRequest& request, const GetBucketReplicationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetBucketReplication(request), context);
}

GetBucketRequestPaymentOutcome S3Client::GetBucketRequestPayment(const GetBucketRequestPaymentRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetBucketRequestPayment", "Required field: Bucket, is not set");
    return GetBucketRequestPaymentOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetBucketRequestPaymentOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?requestPayment");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetBucketRequestPaymentOutcome(GetBucketRequestPaymentResult(outcome.GetResult()));
  }
  else
  {
    return GetBucketRequestPaymentOutcome(outcome.GetError());
  }
}

GetBucketRequestPaymentOutcomeCallable S3Client::GetBucketRequestPaymentCallable(const GetBucketRequestPaymentRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetBucketRequestPaymentOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetBucketRequestPayment(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetBucketRequestPaymentAsync(const GetBucketRequestPaymentRequest& request, const GetBucketRequestPaymentResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetBucketRequestPaymentAsyncHelper( request, handler, context ); } );
}

void S3Client::GetBucketRequestPaymentAsyncHelper(const GetBucketRequestPaymentRequest& request, const GetBucketRequestPaymentResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetBucketRequestPayment(request), context);
}

GetBucketTaggingOutcome S3Client::GetBucketTagging(const GetBucketTaggingRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetBucketTagging", "Required field: Bucket, is not set");
    return GetBucketTaggingOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetBucketTaggingOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?tagging");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetBucketTaggingOutcome(GetBucketTaggingResult(outcome.GetResult()));
  }
  else
  {
    return GetBucketTaggingOutcome(outcome.GetError());
  }
}

GetBucketTaggingOutcomeCallable S3Client::GetBucketTaggingCallable(const GetBucketTaggingRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetBucketTaggingOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetBucketTagging(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetBucketTaggingAsync(const GetBucketTaggingRequest& request, const GetBucketTaggingResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetBucketTaggingAsyncHelper( request, handler, context ); } );
}

void S3Client::GetBucketTaggingAsyncHelper(const GetBucketTaggingRequest& request, const GetBucketTaggingResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetBucketTagging(request), context);
}

GetBucketVersioningOutcome S3Client::GetBucketVersioning(const GetBucketVersioningRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetBucketVersioning", "Required field: Bucket, is not set");
    return GetBucketVersioningOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetBucketVersioningOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?versioning");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetBucketVersioningOutcome(GetBucketVersioningResult(outcome.GetResult()));
  }
  else
  {
    return GetBucketVersioningOutcome(outcome.GetError());
  }
}

GetBucketVersioningOutcomeCallable S3Client::GetBucketVersioningCallable(const GetBucketVersioningRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetBucketVersioningOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetBucketVersioning(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetBucketVersioningAsync(const GetBucketVersioningRequest& request, const GetBucketVersioningResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetBucketVersioningAsyncHelper( request, handler, context ); } );
}

void S3Client::GetBucketVersioningAsyncHelper(const GetBucketVersioningRequest& request, const GetBucketVersioningResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetBucketVersioning(request), context);
}

GetBucketWebsiteOutcome S3Client::GetBucketWebsite(const GetBucketWebsiteRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetBucketWebsite", "Required field: Bucket, is not set");
    return GetBucketWebsiteOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetBucketWebsiteOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?website");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetBucketWebsiteOutcome(GetBucketWebsiteResult(outcome.GetResult()));
  }
  else
  {
    return GetBucketWebsiteOutcome(outcome.GetError());
  }
}

GetBucketWebsiteOutcomeCallable S3Client::GetBucketWebsiteCallable(const GetBucketWebsiteRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetBucketWebsiteOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetBucketWebsite(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetBucketWebsiteAsync(const GetBucketWebsiteRequest& request, const GetBucketWebsiteResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetBucketWebsiteAsyncHelper( request, handler, context ); } );
}

void S3Client::GetBucketWebsiteAsyncHelper(const GetBucketWebsiteRequest& request, const GetBucketWebsiteResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetBucketWebsite(request), context);
}

GetObjectOutcome S3Client::GetObject(const GetObjectRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetObject", "Required field: Bucket, is not set");
    return GetObjectOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.KeyHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetObject", "Required field: Key, is not set");
    return GetObjectOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Key]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetObjectOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetKey();
  uri.SetPath(uri.GetPath() + ss.str());
  StreamOutcome outcome = MakeRequestWithUnparsedResponse(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetObjectOutcome(GetObjectResult(outcome.GetResultWithOwnership()));
  }
  else
  {
    return GetObjectOutcome(outcome.GetError());
  }
}

GetObjectOutcomeCallable S3Client::GetObjectCallable(const GetObjectRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetObjectOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetObject(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetObjectAsync(const GetObjectRequest& request, const GetObjectResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetObjectAsyncHelper( request, handler, context ); } );
}

void S3Client::GetObjectAsyncHelper(const GetObjectRequest& request, const GetObjectResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetObject(request), context);
}

GetObjectAclOutcome S3Client::GetObjectAcl(const GetObjectAclRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetObjectAcl", "Required field: Bucket, is not set");
    return GetObjectAclOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.KeyHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetObjectAcl", "Required field: Key, is not set");
    return GetObjectAclOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Key]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetObjectAclOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetKey();
  uri.SetPath(uri.GetPath() + ss.str());
  ss.str("?acl");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetObjectAclOutcome(GetObjectAclResult(outcome.GetResult()));
  }
  else
  {
    return GetObjectAclOutcome(outcome.GetError());
  }
}

GetObjectAclOutcomeCallable S3Client::GetObjectAclCallable(const GetObjectAclRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetObjectAclOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetObjectAcl(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetObjectAclAsync(const GetObjectAclRequest& request, const GetObjectAclResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetObjectAclAsyncHelper( request, handler, context ); } );
}

void S3Client::GetObjectAclAsyncHelper(const GetObjectAclRequest& request, const GetObjectAclResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetObjectAcl(request), context);
}

GetObjectLegalHoldOutcome S3Client::GetObjectLegalHold(const GetObjectLegalHoldRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetObjectLegalHold", "Required field: Bucket, is not set");
    return GetObjectLegalHoldOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.KeyHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetObjectLegalHold", "Required field: Key, is not set");
    return GetObjectLegalHoldOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Key]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetObjectLegalHoldOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetKey();
  uri.SetPath(uri.GetPath() + ss.str());
  ss.str("?legal-hold");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetObjectLegalHoldOutcome(GetObjectLegalHoldResult(outcome.GetResult()));
  }
  else
  {
    return GetObjectLegalHoldOutcome(outcome.GetError());
  }
}

GetObjectLegalHoldOutcomeCallable S3Client::GetObjectLegalHoldCallable(const GetObjectLegalHoldRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetObjectLegalHoldOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetObjectLegalHold(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetObjectLegalHoldAsync(const GetObjectLegalHoldRequest& request, const GetObjectLegalHoldResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetObjectLegalHoldAsyncHelper( request, handler, context ); } );
}

void S3Client::GetObjectLegalHoldAsyncHelper(const GetObjectLegalHoldRequest& request, const GetObjectLegalHoldResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetObjectLegalHold(request), context);
}

GetObjectLockConfigurationOutcome S3Client::GetObjectLockConfiguration(const GetObjectLockConfigurationRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetObjectLockConfiguration", "Required field: Bucket, is not set");
    return GetObjectLockConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetObjectLockConfigurationOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?object-lock");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetObjectLockConfigurationOutcome(GetObjectLockConfigurationResult(outcome.GetResult()));
  }
  else
  {
    return GetObjectLockConfigurationOutcome(outcome.GetError());
  }
}

GetObjectLockConfigurationOutcomeCallable S3Client::GetObjectLockConfigurationCallable(const GetObjectLockConfigurationRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetObjectLockConfigurationOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetObjectLockConfiguration(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetObjectLockConfigurationAsync(const GetObjectLockConfigurationRequest& request, const GetObjectLockConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetObjectLockConfigurationAsyncHelper( request, handler, context ); } );
}

void S3Client::GetObjectLockConfigurationAsyncHelper(const GetObjectLockConfigurationRequest& request, const GetObjectLockConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetObjectLockConfiguration(request), context);
}

GetObjectRetentionOutcome S3Client::GetObjectRetention(const GetObjectRetentionRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetObjectRetention", "Required field: Bucket, is not set");
    return GetObjectRetentionOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.KeyHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetObjectRetention", "Required field: Key, is not set");
    return GetObjectRetentionOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Key]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetObjectRetentionOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetKey();
  uri.SetPath(uri.GetPath() + ss.str());
  ss.str("?retention");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetObjectRetentionOutcome(GetObjectRetentionResult(outcome.GetResult()));
  }
  else
  {
    return GetObjectRetentionOutcome(outcome.GetError());
  }
}

GetObjectRetentionOutcomeCallable S3Client::GetObjectRetentionCallable(const GetObjectRetentionRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetObjectRetentionOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetObjectRetention(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetObjectRetentionAsync(const GetObjectRetentionRequest& request, const GetObjectRetentionResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetObjectRetentionAsyncHelper( request, handler, context ); } );
}

void S3Client::GetObjectRetentionAsyncHelper(const GetObjectRetentionRequest& request, const GetObjectRetentionResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetObjectRetention(request), context);
}

GetObjectTaggingOutcome S3Client::GetObjectTagging(const GetObjectTaggingRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetObjectTagging", "Required field: Bucket, is not set");
    return GetObjectTaggingOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.KeyHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetObjectTagging", "Required field: Key, is not set");
    return GetObjectTaggingOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Key]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetObjectTaggingOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetKey();
  uri.SetPath(uri.GetPath() + ss.str());
  ss.str("?tagging");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetObjectTaggingOutcome(GetObjectTaggingResult(outcome.GetResult()));
  }
  else
  {
    return GetObjectTaggingOutcome(outcome.GetError());
  }
}

GetObjectTaggingOutcomeCallable S3Client::GetObjectTaggingCallable(const GetObjectTaggingRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetObjectTaggingOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetObjectTagging(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetObjectTaggingAsync(const GetObjectTaggingRequest& request, const GetObjectTaggingResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetObjectTaggingAsyncHelper( request, handler, context ); } );
}

void S3Client::GetObjectTaggingAsyncHelper(const GetObjectTaggingRequest& request, const GetObjectTaggingResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetObjectTagging(request), context);
}

GetObjectTorrentOutcome S3Client::GetObjectTorrent(const GetObjectTorrentRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetObjectTorrent", "Required field: Bucket, is not set");
    return GetObjectTorrentOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.KeyHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetObjectTorrent", "Required field: Key, is not set");
    return GetObjectTorrentOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Key]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetObjectTorrentOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetKey();
  uri.SetPath(uri.GetPath() + ss.str());
  ss.str("?torrent");
  uri.SetQueryString(ss.str());
  StreamOutcome outcome = MakeRequestWithUnparsedResponse(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetObjectTorrentOutcome(GetObjectTorrentResult(outcome.GetResultWithOwnership()));
  }
  else
  {
    return GetObjectTorrentOutcome(outcome.GetError());
  }
}

GetObjectTorrentOutcomeCallable S3Client::GetObjectTorrentCallable(const GetObjectTorrentRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetObjectTorrentOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetObjectTorrent(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetObjectTorrentAsync(const GetObjectTorrentRequest& request, const GetObjectTorrentResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetObjectTorrentAsyncHelper( request, handler, context ); } );
}

void S3Client::GetObjectTorrentAsyncHelper(const GetObjectTorrentRequest& request, const GetObjectTorrentResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetObjectTorrent(request), context);
}

GetPublicAccessBlockOutcome S3Client::GetPublicAccessBlock(const GetPublicAccessBlockRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("GetPublicAccessBlock", "Required field: Bucket, is not set");
    return GetPublicAccessBlockOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return GetPublicAccessBlockOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?publicAccessBlock");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return GetPublicAccessBlockOutcome(GetPublicAccessBlockResult(outcome.GetResult()));
  }
  else
  {
    return GetPublicAccessBlockOutcome(outcome.GetError());
  }
}

GetPublicAccessBlockOutcomeCallable S3Client::GetPublicAccessBlockCallable(const GetPublicAccessBlockRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< GetPublicAccessBlockOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->GetPublicAccessBlock(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::GetPublicAccessBlockAsync(const GetPublicAccessBlockRequest& request, const GetPublicAccessBlockResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetPublicAccessBlockAsyncHelper( request, handler, context ); } );
}

void S3Client::GetPublicAccessBlockAsyncHelper(const GetPublicAccessBlockRequest& request, const GetPublicAccessBlockResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetPublicAccessBlock(request), context);
}

HeadBucketOutcome S3Client::HeadBucket(const HeadBucketRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("HeadBucket", "Required field: Bucket, is not set");
    return HeadBucketOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return HeadBucketOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  uri.SetPath(uri.GetPath() + ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_HEAD, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return HeadBucketOutcome(NoResult());
  }
  else
  {
    return HeadBucketOutcome(outcome.GetError());
  }
}

HeadBucketOutcomeCallable S3Client::HeadBucketCallable(const HeadBucketRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< HeadBucketOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->HeadBucket(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::HeadBucketAsync(const HeadBucketRequest& request, const HeadBucketResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->HeadBucketAsyncHelper( request, handler, context ); } );
}

void S3Client::HeadBucketAsyncHelper(const HeadBucketRequest& request, const HeadBucketResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, HeadBucket(request), context);
}

HeadObjectOutcome S3Client::HeadObject(const HeadObjectRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("HeadObject", "Required field: Bucket, is not set");
    return HeadObjectOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.KeyHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("HeadObject", "Required field: Key, is not set");
    return HeadObjectOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Key]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return HeadObjectOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetKey();
  uri.SetPath(uri.GetPath() + ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_HEAD, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return HeadObjectOutcome(HeadObjectResult(outcome.GetResult()));
  }
  else
  {
    return HeadObjectOutcome(outcome.GetError());
  }
}

HeadObjectOutcomeCallable S3Client::HeadObjectCallable(const HeadObjectRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< HeadObjectOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->HeadObject(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::HeadObjectAsync(const HeadObjectRequest& request, const HeadObjectResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->HeadObjectAsyncHelper( request, handler, context ); } );
}

void S3Client::HeadObjectAsyncHelper(const HeadObjectRequest& request, const HeadObjectResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, HeadObject(request), context);
}

ListBucketAnalyticsConfigurationsOutcome S3Client::ListBucketAnalyticsConfigurations(const ListBucketAnalyticsConfigurationsRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("ListBucketAnalyticsConfigurations", "Required field: Bucket, is not set");
    return ListBucketAnalyticsConfigurationsOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return ListBucketAnalyticsConfigurationsOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?analytics");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return ListBucketAnalyticsConfigurationsOutcome(ListBucketAnalyticsConfigurationsResult(outcome.GetResult()));
  }
  else
  {
    return ListBucketAnalyticsConfigurationsOutcome(outcome.GetError());
  }
}

ListBucketAnalyticsConfigurationsOutcomeCallable S3Client::ListBucketAnalyticsConfigurationsCallable(const ListBucketAnalyticsConfigurationsRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< ListBucketAnalyticsConfigurationsOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->ListBucketAnalyticsConfigurations(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::ListBucketAnalyticsConfigurationsAsync(const ListBucketAnalyticsConfigurationsRequest& request, const ListBucketAnalyticsConfigurationsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->ListBucketAnalyticsConfigurationsAsyncHelper( request, handler, context ); } );
}

void S3Client::ListBucketAnalyticsConfigurationsAsyncHelper(const ListBucketAnalyticsConfigurationsRequest& request, const ListBucketAnalyticsConfigurationsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, ListBucketAnalyticsConfigurations(request), context);
}

ListBucketInventoryConfigurationsOutcome S3Client::ListBucketInventoryConfigurations(const ListBucketInventoryConfigurationsRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("ListBucketInventoryConfigurations", "Required field: Bucket, is not set");
    return ListBucketInventoryConfigurationsOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return ListBucketInventoryConfigurationsOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?inventory");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return ListBucketInventoryConfigurationsOutcome(ListBucketInventoryConfigurationsResult(outcome.GetResult()));
  }
  else
  {
    return ListBucketInventoryConfigurationsOutcome(outcome.GetError());
  }
}

ListBucketInventoryConfigurationsOutcomeCallable S3Client::ListBucketInventoryConfigurationsCallable(const ListBucketInventoryConfigurationsRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< ListBucketInventoryConfigurationsOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->ListBucketInventoryConfigurations(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::ListBucketInventoryConfigurationsAsync(const ListBucketInventoryConfigurationsRequest& request, const ListBucketInventoryConfigurationsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->ListBucketInventoryConfigurationsAsyncHelper( request, handler, context ); } );
}

void S3Client::ListBucketInventoryConfigurationsAsyncHelper(const ListBucketInventoryConfigurationsRequest& request, const ListBucketInventoryConfigurationsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, ListBucketInventoryConfigurations(request), context);
}

ListBucketMetricsConfigurationsOutcome S3Client::ListBucketMetricsConfigurations(const ListBucketMetricsConfigurationsRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("ListBucketMetricsConfigurations", "Required field: Bucket, is not set");
    return ListBucketMetricsConfigurationsOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return ListBucketMetricsConfigurationsOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?metrics");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return ListBucketMetricsConfigurationsOutcome(ListBucketMetricsConfigurationsResult(outcome.GetResult()));
  }
  else
  {
    return ListBucketMetricsConfigurationsOutcome(outcome.GetError());
  }
}

ListBucketMetricsConfigurationsOutcomeCallable S3Client::ListBucketMetricsConfigurationsCallable(const ListBucketMetricsConfigurationsRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< ListBucketMetricsConfigurationsOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->ListBucketMetricsConfigurations(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::ListBucketMetricsConfigurationsAsync(const ListBucketMetricsConfigurationsRequest& request, const ListBucketMetricsConfigurationsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->ListBucketMetricsConfigurationsAsyncHelper( request, handler, context ); } );
}

void S3Client::ListBucketMetricsConfigurationsAsyncHelper(const ListBucketMetricsConfigurationsRequest& request, const ListBucketMetricsConfigurationsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, ListBucketMetricsConfigurations(request), context);
}

ListBucketsOutcome S3Client::ListBuckets() const
{
  Aws::StringStream ss;
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString();
  if (!computeEndpointOutcome.IsSuccess())
  {
    return ListBucketsOutcome(computeEndpointOutcome.GetError());
  }
  ss << computeEndpointOutcome.GetResult().first;
  XmlOutcome outcome = MakeRequest(ss.str(), Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, "ListBuckets", computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return ListBucketsOutcome(ListBucketsResult(outcome.GetResult()));
  }
  else
  {
    return ListBucketsOutcome(outcome.GetError());
  }
}

ListBucketsOutcomeCallable S3Client::ListBucketsCallable() const
{
  auto task = Aws::MakeShared< std::packaged_task< ListBucketsOutcome() > >(ALLOCATION_TAG, [this](){ return this->ListBuckets(); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::ListBucketsAsync(const ListBucketsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, handler, context](){ this->ListBucketsAsyncHelper( handler, context ); } );
}

void S3Client::ListBucketsAsyncHelper(const ListBucketsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, ListBuckets(), context);
}

ListMultipartUploadsOutcome S3Client::ListMultipartUploads(const ListMultipartUploadsRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("ListMultipartUploads", "Required field: Bucket, is not set");
    return ListMultipartUploadsOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return ListMultipartUploadsOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?uploads");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return ListMultipartUploadsOutcome(ListMultipartUploadsResult(outcome.GetResult()));
  }
  else
  {
    return ListMultipartUploadsOutcome(outcome.GetError());
  }
}

ListMultipartUploadsOutcomeCallable S3Client::ListMultipartUploadsCallable(const ListMultipartUploadsRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< ListMultipartUploadsOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->ListMultipartUploads(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::ListMultipartUploadsAsync(const ListMultipartUploadsRequest& request, const ListMultipartUploadsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->ListMultipartUploadsAsyncHelper( request, handler, context ); } );
}

void S3Client::ListMultipartUploadsAsyncHelper(const ListMultipartUploadsRequest& request, const ListMultipartUploadsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, ListMultipartUploads(request), context);
}

ListObjectVersionsOutcome S3Client::ListObjectVersions(const ListObjectVersionsRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("ListObjectVersions", "Required field: Bucket, is not set");
    return ListObjectVersionsOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return ListObjectVersionsOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?versions");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return ListObjectVersionsOutcome(ListObjectVersionsResult(outcome.GetResult()));
  }
  else
  {
    return ListObjectVersionsOutcome(outcome.GetError());
  }
}

ListObjectVersionsOutcomeCallable S3Client::ListObjectVersionsCallable(const ListObjectVersionsRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< ListObjectVersionsOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->ListObjectVersions(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::ListObjectVersionsAsync(const ListObjectVersionsRequest& request, const ListObjectVersionsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->ListObjectVersionsAsyncHelper( request, handler, context ); } );
}

void S3Client::ListObjectVersionsAsyncHelper(const ListObjectVersionsRequest& request, const ListObjectVersionsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, ListObjectVersions(request), context);
}

ListObjectsOutcome S3Client::ListObjects(const ListObjectsRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("ListObjects", "Required field: Bucket, is not set");
    return ListObjectsOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return ListObjectsOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  uri.SetPath(uri.GetPath() + ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return ListObjectsOutcome(ListObjectsResult(outcome.GetResult()));
  }
  else
  {
    return ListObjectsOutcome(outcome.GetError());
  }
}

ListObjectsOutcomeCallable S3Client::ListObjectsCallable(const ListObjectsRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< ListObjectsOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->ListObjects(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::ListObjectsAsync(const ListObjectsRequest& request, const ListObjectsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->ListObjectsAsyncHelper( request, handler, context ); } );
}

void S3Client::ListObjectsAsyncHelper(const ListObjectsRequest& request, const ListObjectsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, ListObjects(request), context);
}

ListObjectsV2Outcome S3Client::ListObjectsV2(const ListObjectsV2Request& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("ListObjectsV2", "Required field: Bucket, is not set");
    return ListObjectsV2Outcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return ListObjectsV2Outcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?list-type=2");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return ListObjectsV2Outcome(ListObjectsV2Result(outcome.GetResult()));
  }
  else
  {
    return ListObjectsV2Outcome(outcome.GetError());
  }
}

ListObjectsV2OutcomeCallable S3Client::ListObjectsV2Callable(const ListObjectsV2Request& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< ListObjectsV2Outcome() > >(ALLOCATION_TAG, [this, request](){ return this->ListObjectsV2(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::ListObjectsV2Async(const ListObjectsV2Request& request, const ListObjectsV2ResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->ListObjectsV2AsyncHelper( request, handler, context ); } );
}

void S3Client::ListObjectsV2AsyncHelper(const ListObjectsV2Request& request, const ListObjectsV2ResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, ListObjectsV2(request), context);
}

ListPartsOutcome S3Client::ListParts(const ListPartsRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("ListParts", "Required field: Bucket, is not set");
    return ListPartsOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.KeyHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("ListParts", "Required field: Key, is not set");
    return ListPartsOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Key]", false));
  }
  if (!request.UploadIdHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("ListParts", "Required field: UploadId, is not set");
    return ListPartsOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [UploadId]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return ListPartsOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetKey();
  uri.SetPath(uri.GetPath() + ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_GET, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return ListPartsOutcome(ListPartsResult(outcome.GetResult()));
  }
  else
  {
    return ListPartsOutcome(outcome.GetError());
  }
}

ListPartsOutcomeCallable S3Client::ListPartsCallable(const ListPartsRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< ListPartsOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->ListParts(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::ListPartsAsync(const ListPartsRequest& request, const ListPartsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->ListPartsAsyncHelper( request, handler, context ); } );
}

void S3Client::ListPartsAsyncHelper(const ListPartsRequest& request, const ListPartsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, ListParts(request), context);
}

PutBucketAccelerateConfigurationOutcome S3Client::PutBucketAccelerateConfiguration(const PutBucketAccelerateConfigurationRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutBucketAccelerateConfiguration", "Required field: Bucket, is not set");
    return PutBucketAccelerateConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return PutBucketAccelerateConfigurationOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?accelerate");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return PutBucketAccelerateConfigurationOutcome(NoResult());
  }
  else
  {
    return PutBucketAccelerateConfigurationOutcome(outcome.GetError());
  }
}

PutBucketAccelerateConfigurationOutcomeCallable S3Client::PutBucketAccelerateConfigurationCallable(const PutBucketAccelerateConfigurationRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutBucketAccelerateConfigurationOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutBucketAccelerateConfiguration(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::PutBucketAccelerateConfigurationAsync(const PutBucketAccelerateConfigurationRequest& request, const PutBucketAccelerateConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutBucketAccelerateConfigurationAsyncHelper( request, handler, context ); } );
}

void S3Client::PutBucketAccelerateConfigurationAsyncHelper(const PutBucketAccelerateConfigurationRequest& request, const PutBucketAccelerateConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutBucketAccelerateConfiguration(request), context);
}

PutBucketAclOutcome S3Client::PutBucketAcl(const PutBucketAclRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutBucketAcl", "Required field: Bucket, is not set");
    return PutBucketAclOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return PutBucketAclOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?acl");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return PutBucketAclOutcome(NoResult());
  }
  else
  {
    return PutBucketAclOutcome(outcome.GetError());
  }
}

PutBucketAclOutcomeCallable S3Client::PutBucketAclCallable(const PutBucketAclRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutBucketAclOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutBucketAcl(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::PutBucketAclAsync(const PutBucketAclRequest& request, const PutBucketAclResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutBucketAclAsyncHelper( request, handler, context ); } );
}

void S3Client::PutBucketAclAsyncHelper(const PutBucketAclRequest& request, const PutBucketAclResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutBucketAcl(request), context);
}

PutBucketAnalyticsConfigurationOutcome S3Client::PutBucketAnalyticsConfiguration(const PutBucketAnalyticsConfigurationRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutBucketAnalyticsConfiguration", "Required field: Bucket, is not set");
    return PutBucketAnalyticsConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.IdHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutBucketAnalyticsConfiguration", "Required field: Id, is not set");
    return PutBucketAnalyticsConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Id]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return PutBucketAnalyticsConfigurationOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?analytics");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return PutBucketAnalyticsConfigurationOutcome(NoResult());
  }
  else
  {
    return PutBucketAnalyticsConfigurationOutcome(outcome.GetError());
  }
}

PutBucketAnalyticsConfigurationOutcomeCallable S3Client::PutBucketAnalyticsConfigurationCallable(const PutBucketAnalyticsConfigurationRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutBucketAnalyticsConfigurationOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutBucketAnalyticsConfiguration(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::PutBucketAnalyticsConfigurationAsync(const PutBucketAnalyticsConfigurationRequest& request, const PutBucketAnalyticsConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutBucketAnalyticsConfigurationAsyncHelper( request, handler, context ); } );
}

void S3Client::PutBucketAnalyticsConfigurationAsyncHelper(const PutBucketAnalyticsConfigurationRequest& request, const PutBucketAnalyticsConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutBucketAnalyticsConfiguration(request), context);
}

PutBucketCorsOutcome S3Client::PutBucketCors(const PutBucketCorsRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutBucketCors", "Required field: Bucket, is not set");
    return PutBucketCorsOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return PutBucketCorsOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?cors");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return PutBucketCorsOutcome(NoResult());
  }
  else
  {
    return PutBucketCorsOutcome(outcome.GetError());
  }
}

PutBucketCorsOutcomeCallable S3Client::PutBucketCorsCallable(const PutBucketCorsRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutBucketCorsOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutBucketCors(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::PutBucketCorsAsync(const PutBucketCorsRequest& request, const PutBucketCorsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutBucketCorsAsyncHelper( request, handler, context ); } );
}

void S3Client::PutBucketCorsAsyncHelper(const PutBucketCorsRequest& request, const PutBucketCorsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutBucketCors(request), context);
}

PutBucketEncryptionOutcome S3Client::PutBucketEncryption(const PutBucketEncryptionRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutBucketEncryption", "Required field: Bucket, is not set");
    return PutBucketEncryptionOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return PutBucketEncryptionOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?encryption");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return PutBucketEncryptionOutcome(NoResult());
  }
  else
  {
    return PutBucketEncryptionOutcome(outcome.GetError());
  }
}

PutBucketEncryptionOutcomeCallable S3Client::PutBucketEncryptionCallable(const PutBucketEncryptionRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutBucketEncryptionOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutBucketEncryption(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::PutBucketEncryptionAsync(const PutBucketEncryptionRequest& request, const PutBucketEncryptionResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutBucketEncryptionAsyncHelper( request, handler, context ); } );
}

void S3Client::PutBucketEncryptionAsyncHelper(const PutBucketEncryptionRequest& request, const PutBucketEncryptionResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutBucketEncryption(request), context);
}

PutBucketInventoryConfigurationOutcome S3Client::PutBucketInventoryConfiguration(const PutBucketInventoryConfigurationRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutBucketInventoryConfiguration", "Required field: Bucket, is not set");
    return PutBucketInventoryConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.IdHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutBucketInventoryConfiguration", "Required field: Id, is not set");
    return PutBucketInventoryConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Id]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return PutBucketInventoryConfigurationOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?inventory");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return PutBucketInventoryConfigurationOutcome(NoResult());
  }
  else
  {
    return PutBucketInventoryConfigurationOutcome(outcome.GetError());
  }
}

PutBucketInventoryConfigurationOutcomeCallable S3Client::PutBucketInventoryConfigurationCallable(const PutBucketInventoryConfigurationRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutBucketInventoryConfigurationOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutBucketInventoryConfiguration(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::PutBucketInventoryConfigurationAsync(const PutBucketInventoryConfigurationRequest& request, const PutBucketInventoryConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutBucketInventoryConfigurationAsyncHelper( request, handler, context ); } );
}

void S3Client::PutBucketInventoryConfigurationAsyncHelper(const PutBucketInventoryConfigurationRequest& request, const PutBucketInventoryConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutBucketInventoryConfiguration(request), context);
}

PutBucketLifecycleConfigurationOutcome S3Client::PutBucketLifecycleConfiguration(const PutBucketLifecycleConfigurationRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutBucketLifecycleConfiguration", "Required field: Bucket, is not set");
    return PutBucketLifecycleConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return PutBucketLifecycleConfigurationOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?lifecycle");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return PutBucketLifecycleConfigurationOutcome(NoResult());
  }
  else
  {
    return PutBucketLifecycleConfigurationOutcome(outcome.GetError());
  }
}

PutBucketLifecycleConfigurationOutcomeCallable S3Client::PutBucketLifecycleConfigurationCallable(const PutBucketLifecycleConfigurationRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutBucketLifecycleConfigurationOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutBucketLifecycleConfiguration(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::PutBucketLifecycleConfigurationAsync(const PutBucketLifecycleConfigurationRequest& request, const PutBucketLifecycleConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutBucketLifecycleConfigurationAsyncHelper( request, handler, context ); } );
}

void S3Client::PutBucketLifecycleConfigurationAsyncHelper(const PutBucketLifecycleConfigurationRequest& request, const PutBucketLifecycleConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutBucketLifecycleConfiguration(request), context);
}

PutBucketLoggingOutcome S3Client::PutBucketLogging(const PutBucketLoggingRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutBucketLogging", "Required field: Bucket, is not set");
    return PutBucketLoggingOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return PutBucketLoggingOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?logging");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return PutBucketLoggingOutcome(NoResult());
  }
  else
  {
    return PutBucketLoggingOutcome(outcome.GetError());
  }
}

PutBucketLoggingOutcomeCallable S3Client::PutBucketLoggingCallable(const PutBucketLoggingRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutBucketLoggingOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutBucketLogging(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::PutBucketLoggingAsync(const PutBucketLoggingRequest& request, const PutBucketLoggingResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutBucketLoggingAsyncHelper( request, handler, context ); } );
}

void S3Client::PutBucketLoggingAsyncHelper(const PutBucketLoggingRequest& request, const PutBucketLoggingResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutBucketLogging(request), context);
}

PutBucketMetricsConfigurationOutcome S3Client::PutBucketMetricsConfiguration(const PutBucketMetricsConfigurationRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutBucketMetricsConfiguration", "Required field: Bucket, is not set");
    return PutBucketMetricsConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.IdHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutBucketMetricsConfiguration", "Required field: Id, is not set");
    return PutBucketMetricsConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Id]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return PutBucketMetricsConfigurationOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?metrics");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return PutBucketMetricsConfigurationOutcome(NoResult());
  }
  else
  {
    return PutBucketMetricsConfigurationOutcome(outcome.GetError());
  }
}

PutBucketMetricsConfigurationOutcomeCallable S3Client::PutBucketMetricsConfigurationCallable(const PutBucketMetricsConfigurationRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutBucketMetricsConfigurationOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutBucketMetricsConfiguration(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::PutBucketMetricsConfigurationAsync(const PutBucketMetricsConfigurationRequest& request, const PutBucketMetricsConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutBucketMetricsConfigurationAsyncHelper( request, handler, context ); } );
}

void S3Client::PutBucketMetricsConfigurationAsyncHelper(const PutBucketMetricsConfigurationRequest& request, const PutBucketMetricsConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutBucketMetricsConfiguration(request), context);
}

PutBucketNotificationConfigurationOutcome S3Client::PutBucketNotificationConfiguration(const PutBucketNotificationConfigurationRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutBucketNotificationConfiguration", "Required field: Bucket, is not set");
    return PutBucketNotificationConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return PutBucketNotificationConfigurationOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?notification");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return PutBucketNotificationConfigurationOutcome(NoResult());
  }
  else
  {
    return PutBucketNotificationConfigurationOutcome(outcome.GetError());
  }
}

PutBucketNotificationConfigurationOutcomeCallable S3Client::PutBucketNotificationConfigurationCallable(const PutBucketNotificationConfigurationRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutBucketNotificationConfigurationOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutBucketNotificationConfiguration(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::PutBucketNotificationConfigurationAsync(const PutBucketNotificationConfigurationRequest& request, const PutBucketNotificationConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutBucketNotificationConfigurationAsyncHelper( request, handler, context ); } );
}

void S3Client::PutBucketNotificationConfigurationAsyncHelper(const PutBucketNotificationConfigurationRequest& request, const PutBucketNotificationConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutBucketNotificationConfiguration(request), context);
}

PutBucketPolicyOutcome S3Client::PutBucketPolicy(const PutBucketPolicyRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutBucketPolicy", "Required field: Bucket, is not set");
    return PutBucketPolicyOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return PutBucketPolicyOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?policy");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return PutBucketPolicyOutcome(NoResult());
  }
  else
  {
    return PutBucketPolicyOutcome(outcome.GetError());
  }
}

PutBucketPolicyOutcomeCallable S3Client::PutBucketPolicyCallable(const PutBucketPolicyRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutBucketPolicyOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutBucketPolicy(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::PutBucketPolicyAsync(const PutBucketPolicyRequest& request, const PutBucketPolicyResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutBucketPolicyAsyncHelper( request, handler, context ); } );
}

void S3Client::PutBucketPolicyAsyncHelper(const PutBucketPolicyRequest& request, const PutBucketPolicyResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutBucketPolicy(request), context);
}

PutBucketReplicationOutcome S3Client::PutBucketReplication(const PutBucketReplicationRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutBucketReplication", "Required field: Bucket, is not set");
    return PutBucketReplicationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return PutBucketReplicationOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?replication");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return PutBucketReplicationOutcome(NoResult());
  }
  else
  {
    return PutBucketReplicationOutcome(outcome.GetError());
  }
}

PutBucketReplicationOutcomeCallable S3Client::PutBucketReplicationCallable(const PutBucketReplicationRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutBucketReplicationOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutBucketReplication(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::PutBucketReplicationAsync(const PutBucketReplicationRequest& request, const PutBucketReplicationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutBucketReplicationAsyncHelper( request, handler, context ); } );
}

void S3Client::PutBucketReplicationAsyncHelper(const PutBucketReplicationRequest& request, const PutBucketReplicationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutBucketReplication(request), context);
}

PutBucketRequestPaymentOutcome S3Client::PutBucketRequestPayment(const PutBucketRequestPaymentRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutBucketRequestPayment", "Required field: Bucket, is not set");
    return PutBucketRequestPaymentOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return PutBucketRequestPaymentOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?requestPayment");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return PutBucketRequestPaymentOutcome(NoResult());
  }
  else
  {
    return PutBucketRequestPaymentOutcome(outcome.GetError());
  }
}

PutBucketRequestPaymentOutcomeCallable S3Client::PutBucketRequestPaymentCallable(const PutBucketRequestPaymentRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutBucketRequestPaymentOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutBucketRequestPayment(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::PutBucketRequestPaymentAsync(const PutBucketRequestPaymentRequest& request, const PutBucketRequestPaymentResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutBucketRequestPaymentAsyncHelper( request, handler, context ); } );
}

void S3Client::PutBucketRequestPaymentAsyncHelper(const PutBucketRequestPaymentRequest& request, const PutBucketRequestPaymentResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutBucketRequestPayment(request), context);
}

PutBucketTaggingOutcome S3Client::PutBucketTagging(const PutBucketTaggingRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutBucketTagging", "Required field: Bucket, is not set");
    return PutBucketTaggingOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return PutBucketTaggingOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?tagging");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return PutBucketTaggingOutcome(NoResult());
  }
  else
  {
    return PutBucketTaggingOutcome(outcome.GetError());
  }
}

PutBucketTaggingOutcomeCallable S3Client::PutBucketTaggingCallable(const PutBucketTaggingRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutBucketTaggingOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutBucketTagging(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::PutBucketTaggingAsync(const PutBucketTaggingRequest& request, const PutBucketTaggingResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutBucketTaggingAsyncHelper( request, handler, context ); } );
}

void S3Client::PutBucketTaggingAsyncHelper(const PutBucketTaggingRequest& request, const PutBucketTaggingResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutBucketTagging(request), context);
}

PutBucketVersioningOutcome S3Client::PutBucketVersioning(const PutBucketVersioningRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutBucketVersioning", "Required field: Bucket, is not set");
    return PutBucketVersioningOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return PutBucketVersioningOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?versioning");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return PutBucketVersioningOutcome(NoResult());
  }
  else
  {
    return PutBucketVersioningOutcome(outcome.GetError());
  }
}

PutBucketVersioningOutcomeCallable S3Client::PutBucketVersioningCallable(const PutBucketVersioningRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutBucketVersioningOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutBucketVersioning(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::PutBucketVersioningAsync(const PutBucketVersioningRequest& request, const PutBucketVersioningResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutBucketVersioningAsyncHelper( request, handler, context ); } );
}

void S3Client::PutBucketVersioningAsyncHelper(const PutBucketVersioningRequest& request, const PutBucketVersioningResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutBucketVersioning(request), context);
}

PutBucketWebsiteOutcome S3Client::PutBucketWebsite(const PutBucketWebsiteRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutBucketWebsite", "Required field: Bucket, is not set");
    return PutBucketWebsiteOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return PutBucketWebsiteOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?website");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return PutBucketWebsiteOutcome(NoResult());
  }
  else
  {
    return PutBucketWebsiteOutcome(outcome.GetError());
  }
}

PutBucketWebsiteOutcomeCallable S3Client::PutBucketWebsiteCallable(const PutBucketWebsiteRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutBucketWebsiteOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutBucketWebsite(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::PutBucketWebsiteAsync(const PutBucketWebsiteRequest& request, const PutBucketWebsiteResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutBucketWebsiteAsyncHelper( request, handler, context ); } );
}

void S3Client::PutBucketWebsiteAsyncHelper(const PutBucketWebsiteRequest& request, const PutBucketWebsiteResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutBucketWebsite(request), context);
}

PutObjectOutcome S3Client::PutObject(const PutObjectRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutObject", "Required field: Bucket, is not set");
    return PutObjectOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.KeyHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutObject", "Required field: Key, is not set");
    return PutObjectOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Key]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return PutObjectOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetKey();
  uri.SetPath(uri.GetPath() + ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return PutObjectOutcome(PutObjectResult(outcome.GetResult()));
  }
  else
  {
    return PutObjectOutcome(outcome.GetError());
  }
}

PutObjectOutcomeCallable S3Client::PutObjectCallable(const PutObjectRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutObjectOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutObject(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::PutObjectAsync(const PutObjectRequest& request, const PutObjectResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutObjectAsyncHelper( request, handler, context ); } );
}

void S3Client::PutObjectAsyncHelper(const PutObjectRequest& request, const PutObjectResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutObject(request), context);
}

PutObjectAclOutcome S3Client::PutObjectAcl(const PutObjectAclRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutObjectAcl", "Required field: Bucket, is not set");
    return PutObjectAclOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.KeyHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutObjectAcl", "Required field: Key, is not set");
    return PutObjectAclOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Key]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return PutObjectAclOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetKey();
  uri.SetPath(uri.GetPath() + ss.str());
  ss.str("?acl");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return PutObjectAclOutcome(PutObjectAclResult(outcome.GetResult()));
  }
  else
  {
    return PutObjectAclOutcome(outcome.GetError());
  }
}

PutObjectAclOutcomeCallable S3Client::PutObjectAclCallable(const PutObjectAclRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutObjectAclOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutObjectAcl(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::PutObjectAclAsync(const PutObjectAclRequest& request, const PutObjectAclResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutObjectAclAsyncHelper( request, handler, context ); } );
}

void S3Client::PutObjectAclAsyncHelper(const PutObjectAclRequest& request, const PutObjectAclResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutObjectAcl(request), context);
}

PutObjectLegalHoldOutcome S3Client::PutObjectLegalHold(const PutObjectLegalHoldRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutObjectLegalHold", "Required field: Bucket, is not set");
    return PutObjectLegalHoldOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.KeyHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutObjectLegalHold", "Required field: Key, is not set");
    return PutObjectLegalHoldOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Key]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return PutObjectLegalHoldOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetKey();
  uri.SetPath(uri.GetPath() + ss.str());
  ss.str("?legal-hold");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return PutObjectLegalHoldOutcome(PutObjectLegalHoldResult(outcome.GetResult()));
  }
  else
  {
    return PutObjectLegalHoldOutcome(outcome.GetError());
  }
}

PutObjectLegalHoldOutcomeCallable S3Client::PutObjectLegalHoldCallable(const PutObjectLegalHoldRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutObjectLegalHoldOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutObjectLegalHold(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::PutObjectLegalHoldAsync(const PutObjectLegalHoldRequest& request, const PutObjectLegalHoldResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutObjectLegalHoldAsyncHelper( request, handler, context ); } );
}

void S3Client::PutObjectLegalHoldAsyncHelper(const PutObjectLegalHoldRequest& request, const PutObjectLegalHoldResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutObjectLegalHold(request), context);
}

PutObjectLockConfigurationOutcome S3Client::PutObjectLockConfiguration(const PutObjectLockConfigurationRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutObjectLockConfiguration", "Required field: Bucket, is not set");
    return PutObjectLockConfigurationOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return PutObjectLockConfigurationOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?object-lock");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return PutObjectLockConfigurationOutcome(PutObjectLockConfigurationResult(outcome.GetResult()));
  }
  else
  {
    return PutObjectLockConfigurationOutcome(outcome.GetError());
  }
}

PutObjectLockConfigurationOutcomeCallable S3Client::PutObjectLockConfigurationCallable(const PutObjectLockConfigurationRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutObjectLockConfigurationOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutObjectLockConfiguration(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::PutObjectLockConfigurationAsync(const PutObjectLockConfigurationRequest& request, const PutObjectLockConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutObjectLockConfigurationAsyncHelper( request, handler, context ); } );
}

void S3Client::PutObjectLockConfigurationAsyncHelper(const PutObjectLockConfigurationRequest& request, const PutObjectLockConfigurationResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutObjectLockConfiguration(request), context);
}

PutObjectRetentionOutcome S3Client::PutObjectRetention(const PutObjectRetentionRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutObjectRetention", "Required field: Bucket, is not set");
    return PutObjectRetentionOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.KeyHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutObjectRetention", "Required field: Key, is not set");
    return PutObjectRetentionOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Key]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return PutObjectRetentionOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetKey();
  uri.SetPath(uri.GetPath() + ss.str());
  ss.str("?retention");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return PutObjectRetentionOutcome(PutObjectRetentionResult(outcome.GetResult()));
  }
  else
  {
    return PutObjectRetentionOutcome(outcome.GetError());
  }
}

PutObjectRetentionOutcomeCallable S3Client::PutObjectRetentionCallable(const PutObjectRetentionRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutObjectRetentionOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutObjectRetention(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::PutObjectRetentionAsync(const PutObjectRetentionRequest& request, const PutObjectRetentionResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutObjectRetentionAsyncHelper( request, handler, context ); } );
}

void S3Client::PutObjectRetentionAsyncHelper(const PutObjectRetentionRequest& request, const PutObjectRetentionResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutObjectRetention(request), context);
}

PutObjectTaggingOutcome S3Client::PutObjectTagging(const PutObjectTaggingRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutObjectTagging", "Required field: Bucket, is not set");
    return PutObjectTaggingOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.KeyHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutObjectTagging", "Required field: Key, is not set");
    return PutObjectTaggingOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Key]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return PutObjectTaggingOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetKey();
  uri.SetPath(uri.GetPath() + ss.str());
  ss.str("?tagging");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return PutObjectTaggingOutcome(PutObjectTaggingResult(outcome.GetResult()));
  }
  else
  {
    return PutObjectTaggingOutcome(outcome.GetError());
  }
}

PutObjectTaggingOutcomeCallable S3Client::PutObjectTaggingCallable(const PutObjectTaggingRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutObjectTaggingOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutObjectTagging(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::PutObjectTaggingAsync(const PutObjectTaggingRequest& request, const PutObjectTaggingResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutObjectTaggingAsyncHelper( request, handler, context ); } );
}

void S3Client::PutObjectTaggingAsyncHelper(const PutObjectTaggingRequest& request, const PutObjectTaggingResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutObjectTagging(request), context);
}

PutPublicAccessBlockOutcome S3Client::PutPublicAccessBlock(const PutPublicAccessBlockRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("PutPublicAccessBlock", "Required field: Bucket, is not set");
    return PutPublicAccessBlockOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return PutPublicAccessBlockOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss.str("?publicAccessBlock");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return PutPublicAccessBlockOutcome(NoResult());
  }
  else
  {
    return PutPublicAccessBlockOutcome(outcome.GetError());
  }
}

PutPublicAccessBlockOutcomeCallable S3Client::PutPublicAccessBlockCallable(const PutPublicAccessBlockRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< PutPublicAccessBlockOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->PutPublicAccessBlock(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::PutPublicAccessBlockAsync(const PutPublicAccessBlockRequest& request, const PutPublicAccessBlockResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutPublicAccessBlockAsyncHelper( request, handler, context ); } );
}

void S3Client::PutPublicAccessBlockAsyncHelper(const PutPublicAccessBlockRequest& request, const PutPublicAccessBlockResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutPublicAccessBlock(request), context);
}

RestoreObjectOutcome S3Client::RestoreObject(const RestoreObjectRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("RestoreObject", "Required field: Bucket, is not set");
    return RestoreObjectOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.KeyHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("RestoreObject", "Required field: Key, is not set");
    return RestoreObjectOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Key]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return RestoreObjectOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetKey();
  uri.SetPath(uri.GetPath() + ss.str());
  ss.str("?restore");
  uri.SetQueryString(ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_POST, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return RestoreObjectOutcome(RestoreObjectResult(outcome.GetResult()));
  }
  else
  {
    return RestoreObjectOutcome(outcome.GetError());
  }
}

RestoreObjectOutcomeCallable S3Client::RestoreObjectCallable(const RestoreObjectRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< RestoreObjectOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->RestoreObject(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::RestoreObjectAsync(const RestoreObjectRequest& request, const RestoreObjectResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->RestoreObjectAsyncHelper( request, handler, context ); } );
}

void S3Client::RestoreObjectAsyncHelper(const RestoreObjectRequest& request, const RestoreObjectResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, RestoreObject(request), context);
}

SelectObjectContentOutcome S3Client::SelectObjectContent(SelectObjectContentRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("SelectObjectContent", "Required field: Bucket, is not set");
    return SelectObjectContentOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.KeyHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("SelectObjectContent", "Required field: Key, is not set");
    return SelectObjectContentOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Key]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return SelectObjectContentOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetKey();
  uri.SetPath(uri.GetPath() + ss.str());
  ss.str("?select&select-type=2");
  uri.SetQueryString(ss.str());
  request.SetResponseStreamFactory(
      [&] { request.GetEventStreamDecoder().Reset(); return Aws::New<Aws::Utils::Event::EventDecoderStream>(ALLOCATION_TAG, request.GetEventStreamDecoder()); }
  );
  XmlOutcome outcome = MakeRequestWithEventStream(uri, request, Aws::Http::HttpMethod::HTTP_POST, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return SelectObjectContentOutcome(NoResult());
  }
  else
  {
    return SelectObjectContentOutcome(outcome.GetError());
  }
}

SelectObjectContentOutcomeCallable S3Client::SelectObjectContentCallable(SelectObjectContentRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< SelectObjectContentOutcome() > >(ALLOCATION_TAG, [this, &request](){ return this->SelectObjectContent(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::SelectObjectContentAsync(SelectObjectContentRequest& request, const SelectObjectContentResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, &request, handler, context](){ this->SelectObjectContentAsyncHelper( request, handler, context ); } );
}

void S3Client::SelectObjectContentAsyncHelper(SelectObjectContentRequest& request, const SelectObjectContentResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, SelectObjectContent(request), context);
}

UploadPartOutcome S3Client::UploadPart(const UploadPartRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("UploadPart", "Required field: Bucket, is not set");
    return UploadPartOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.KeyHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("UploadPart", "Required field: Key, is not set");
    return UploadPartOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Key]", false));
  }
  if (!request.PartNumberHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("UploadPart", "Required field: PartNumber, is not set");
    return UploadPartOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [PartNumber]", false));
  }
  if (!request.UploadIdHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("UploadPart", "Required field: UploadId, is not set");
    return UploadPartOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [UploadId]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return UploadPartOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetKey();
  uri.SetPath(uri.GetPath() + ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return UploadPartOutcome(UploadPartResult(outcome.GetResult()));
  }
  else
  {
    return UploadPartOutcome(outcome.GetError());
  }
}

UploadPartOutcomeCallable S3Client::UploadPartCallable(const UploadPartRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< UploadPartOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->UploadPart(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::UploadPartAsync(const UploadPartRequest& request, const UploadPartResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->UploadPartAsyncHelper( request, handler, context ); } );
}

void S3Client::UploadPartAsyncHelper(const UploadPartRequest& request, const UploadPartResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, UploadPart(request), context);
}

UploadPartCopyOutcome S3Client::UploadPartCopy(const UploadPartCopyRequest& request) const
{
  if (!request.BucketHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("UploadPartCopy", "Required field: Bucket, is not set");
    return UploadPartCopyOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Bucket]", false));
  }
  if (!request.CopySourceHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("UploadPartCopy", "Required field: CopySource, is not set");
    return UploadPartCopyOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [CopySource]", false));
  }
  if (!request.KeyHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("UploadPartCopy", "Required field: Key, is not set");
    return UploadPartCopyOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [Key]", false));
  }
  if (!request.PartNumberHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("UploadPartCopy", "Required field: PartNumber, is not set");
    return UploadPartCopyOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [PartNumber]", false));
  }
  if (!request.UploadIdHasBeenSet())
  {
    AWS_LOGSTREAM_ERROR("UploadPartCopy", "Required field: UploadId, is not set");
    return UploadPartCopyOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::MISSING_PARAMETER, "MISSING_PARAMETER", "Missing required field [UploadId]", false));
  }
  ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(request.GetBucket());
  if (!computeEndpointOutcome.IsSuccess())
  {
    return UploadPartCopyOutcome(computeEndpointOutcome.GetError());
  }
  Aws::Http::URI uri = computeEndpointOutcome.GetResult().first;
  Aws::StringStream ss;
  ss << "/";
  ss << request.GetKey();
  uri.SetPath(uri.GetPath() + ss.str());
  XmlOutcome outcome = MakeRequest(uri, request, Aws::Http::HttpMethod::HTTP_PUT, Aws::Auth::SIGV4_SIGNER, computeEndpointOutcome.GetResult().second.c_str() /*signerRegionOverride*/);
  if(outcome.IsSuccess())
  {
    return UploadPartCopyOutcome(UploadPartCopyResult(outcome.GetResult()));
  }
  else
  {
    return UploadPartCopyOutcome(outcome.GetError());
  }
}

UploadPartCopyOutcomeCallable S3Client::UploadPartCopyCallable(const UploadPartCopyRequest& request) const
{
  auto task = Aws::MakeShared< std::packaged_task< UploadPartCopyOutcome() > >(ALLOCATION_TAG, [this, request](){ return this->UploadPartCopy(request); } );
  auto packagedFunction = [task]() { (*task)(); };
  m_executor->Submit(packagedFunction);
  return task->get_future();
}

void S3Client::UploadPartCopyAsync(const UploadPartCopyRequest& request, const UploadPartCopyResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->UploadPartCopyAsyncHelper( request, handler, context ); } );
}

void S3Client::UploadPartCopyAsyncHelper(const UploadPartCopyRequest& request, const UploadPartCopyResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, UploadPartCopy(request), context);
}




#include<aws/core/platform/Environment.h>

static const char US_EAST_1_REGIONAL_ENDPOINT_ENV_VAR[] = "AWS_S3_US_EAST_1_REGIONAL_ENDPOINT";
static const char US_EAST_1_REGIONAL_ENDPOINT_CONFIG_VAR[] = "s3_us_east_1_regional_endpoint";
static const char S3_USE_ARN_REGION_ENVIRONMENT_VARIABLE[] = "AWS_S3_USE_ARN_REGION";
static const char S3_USE_ARN_REGION_CONFIG_FILE_OPTION[] = "s3_use_arn_region";

void S3Client::LoadS3SpecificConfig(const Aws::String& profile)
{
  if (m_USEast1RegionalEndpointOption == Aws::S3::US_EAST_1_REGIONAL_ENDPOINT_OPTION::NOT_SET)
  {
    Aws::String option = Aws::Environment::GetEnv(US_EAST_1_REGIONAL_ENDPOINT_ENV_VAR);
    if (option.empty())
    {
      option = Aws::Config::GetCachedConfigValue(profile, US_EAST_1_REGIONAL_ENDPOINT_CONFIG_VAR);
    }

    if (Aws::Utils::StringUtils::ToLower(option.c_str()) == "regional")
    {
      m_USEast1RegionalEndpointOption = Aws::S3::US_EAST_1_REGIONAL_ENDPOINT_OPTION::REGIONAL;
    }
    else // defaults is legacy
    {
      m_USEast1RegionalEndpointOption = Aws::S3::US_EAST_1_REGIONAL_ENDPOINT_OPTION::LEGACY;
    }
  }

  Aws::String s3UseArnRegion = Aws::Environment::GetEnv(S3_USE_ARN_REGION_ENVIRONMENT_VARIABLE);
  if (s3UseArnRegion.empty())
  {
    s3UseArnRegion = Aws::Config::GetCachedConfigValue(profile, S3_USE_ARN_REGION_CONFIG_FILE_OPTION);
  }

  if (s3UseArnRegion == "true")
  {
    m_useArnRegion = true;
  }
  else
  {
    if (!s3UseArnRegion.empty() && s3UseArnRegion != "false")
    {
      AWS_LOGSTREAM_WARN("S3Client", "AWS_S3_USE_ARN_REGION in environment variables or s3_use_arn_region in config file"
                                  << "should either be true of false if specified, otherwise turn off this flag by default.");
    }
    m_useArnRegion = false;
  }
}

#include<aws/core/utils/HashingUtils.h>
Aws::String S3Client::GeneratePresignedUrl(const Aws::String& bucket, const Aws::String& key, Aws::Http::HttpMethod method, long long expirationInSeconds)
{
    ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(bucket);
    if (!computeEndpointOutcome.IsSuccess())
    {
        AWS_LOGSTREAM_ERROR(ALLOCATION_TAG, "Presigned URL generating failed. Encountered error: " << computeEndpointOutcome.GetError());
        return {};
    }
    Aws::StringStream ss;
    ss << computeEndpointOutcome.GetResult().first << "/" << key;
    URI uri(ss.str());
    return AWSClient::GeneratePresignedUrl(uri, method, computeEndpointOutcome.GetResult().second.c_str(), expirationInSeconds);
}

Aws::String S3Client::GeneratePresignedUrl(const Aws::String& bucket, const Aws::String& key, Aws::Http::HttpMethod method, const Http::HeaderValueCollection& customizedHeaders, long long expirationInSeconds)
{
    ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(bucket);
    if (!computeEndpointOutcome.IsSuccess())
    {
        AWS_LOGSTREAM_ERROR(ALLOCATION_TAG, "Presigned URL generating failed. Encountered error: " << computeEndpointOutcome.GetError());
        return {};
    }
    Aws::StringStream ss;
    ss << computeEndpointOutcome.GetResult().first << "/" << key;
    URI uri(ss.str());
    return AWSClient::GeneratePresignedUrl(uri, method, computeEndpointOutcome.GetResult().second.c_str(), customizedHeaders, expirationInSeconds);
}

Aws::String S3Client::GeneratePresignedUrlWithSSES3(const Aws::String& bucket, const Aws::String& key, Aws::Http::HttpMethod method, long long expirationInSeconds)
{
    ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(bucket);
    if (!computeEndpointOutcome.IsSuccess())
    {
        AWS_LOGSTREAM_ERROR(ALLOCATION_TAG, "Presigned URL generating failed. Encountered error: " << computeEndpointOutcome.GetError());
        return {};
    }
    Aws::StringStream ss;
    ss << computeEndpointOutcome.GetResult().first << "/" << key;
    URI uri(ss.str());
    Aws::Http::HeaderValueCollection headers;
    headers.emplace(Aws::S3::SSEHeaders::SERVER_SIDE_ENCRYPTION, Aws::S3::Model::ServerSideEncryptionMapper::GetNameForServerSideEncryption(Aws::S3::Model::ServerSideEncryption::AES256));
    return AWSClient::GeneratePresignedUrl(uri, method, computeEndpointOutcome.GetResult().second.c_str(), headers, expirationInSeconds);
}

Aws::String S3Client::GeneratePresignedUrlWithSSES3(const Aws::String& bucket, const Aws::String& key, Aws::Http::HttpMethod method, Http::HeaderValueCollection customizedHeaders, long long expirationInSeconds)
{
    ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(bucket);
    if (!computeEndpointOutcome.IsSuccess())
    {
        AWS_LOGSTREAM_ERROR(ALLOCATION_TAG, "Presigned URL generating failed. Encountered error: " << computeEndpointOutcome.GetError());
        return {};
    }
    Aws::StringStream ss;
    ss << computeEndpointOutcome.GetResult().first << "/" << key;
    URI uri(ss.str());
    customizedHeaders.emplace(Aws::S3::SSEHeaders::SERVER_SIDE_ENCRYPTION, Aws::S3::Model::ServerSideEncryptionMapper::GetNameForServerSideEncryption(Aws::S3::Model::ServerSideEncryption::AES256));
    return AWSClient::GeneratePresignedUrl(uri, method, computeEndpointOutcome.GetResult().second.c_str(), customizedHeaders, expirationInSeconds);
}

Aws::String S3Client::GeneratePresignedUrlWithSSEKMS(const Aws::String& bucket, const Aws::String& key, Aws::Http::HttpMethod method, const Aws::String& kmsMasterKeyId, long long expirationInSeconds)
{
    ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(bucket);
    if (!computeEndpointOutcome.IsSuccess())
    {
        AWS_LOGSTREAM_ERROR(ALLOCATION_TAG, "Presigned URL generating failed. Encountered error: " << computeEndpointOutcome.GetError());
        return {};
    }
    Aws::StringStream ss;
    ss << computeEndpointOutcome.GetResult().first << "/" << key;
    URI uri(ss.str());
    Aws::Http::HeaderValueCollection headers;
    headers.emplace(Aws::S3::SSEHeaders::SERVER_SIDE_ENCRYPTION, Aws::S3::Model::ServerSideEncryptionMapper::GetNameForServerSideEncryption(Aws::S3::Model::ServerSideEncryption::aws_kms));
    headers.emplace(Aws::S3::SSEHeaders::SERVER_SIDE_ENCRYPTION_AWS_KMS_KEY_ID, kmsMasterKeyId);
    return AWSClient::GeneratePresignedUrl(uri, method, computeEndpointOutcome.GetResult().second.c_str(), headers, expirationInSeconds);
}

Aws::String S3Client::GeneratePresignedUrlWithSSEKMS(const Aws::String& bucket, const Aws::String& key, Aws::Http::HttpMethod method, Http::HeaderValueCollection customizedHeaders, const Aws::String& kmsMasterKeyId, long long expirationInSeconds)
{
    ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(bucket);
    if (!computeEndpointOutcome.IsSuccess())
    {
        AWS_LOGSTREAM_ERROR(ALLOCATION_TAG, "Presigned URL generating failed. Encountered error: " << computeEndpointOutcome.GetError());
        return {};
    }
    Aws::StringStream ss;
    ss << computeEndpointOutcome.GetResult().first << "/" << key;
    URI uri(ss.str());
    customizedHeaders.emplace(Aws::S3::SSEHeaders::SERVER_SIDE_ENCRYPTION, Aws::S3::Model::ServerSideEncryptionMapper::GetNameForServerSideEncryption(Aws::S3::Model::ServerSideEncryption::aws_kms));
    customizedHeaders.emplace(Aws::S3::SSEHeaders::SERVER_SIDE_ENCRYPTION_AWS_KMS_KEY_ID, kmsMasterKeyId);
    return AWSClient::GeneratePresignedUrl(uri, method, computeEndpointOutcome.GetResult().second.c_str(), customizedHeaders, expirationInSeconds);
}

Aws::String S3Client::GeneratePresignedUrlWithSSEC(const Aws::String& bucket, const Aws::String& key, Aws::Http::HttpMethod method, const Aws::String& base64EncodedAES256Key, long long expirationInSeconds)
{
    ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(bucket);
    if (!computeEndpointOutcome.IsSuccess())
    {
        AWS_LOGSTREAM_ERROR(ALLOCATION_TAG, "Presigned URL generating failed. Encountered error: " << computeEndpointOutcome.GetError());
        return {};
    }
    Aws::StringStream ss;
    ss << computeEndpointOutcome.GetResult().first << "/" << key;
    URI uri(ss.str());
    Aws::Http::HeaderValueCollection headers;
    headers.emplace(Aws::S3::SSEHeaders::SERVER_SIDE_ENCRYPTION_CUSTOMER_ALGORITHM, Aws::S3::Model::ServerSideEncryptionMapper::GetNameForServerSideEncryption(Aws::S3::Model::ServerSideEncryption::AES256));
    headers.emplace(Aws::S3::SSEHeaders::SERVER_SIDE_ENCRYPTION_CUSTOMER_KEY, base64EncodedAES256Key);
    Aws::Utils::ByteBuffer buffer = Aws::Utils::HashingUtils::Base64Decode(base64EncodedAES256Key);
    Aws::String strBuffer(reinterpret_cast<char*>(buffer.GetUnderlyingData()), buffer.GetLength());
    headers.emplace(Aws::S3::SSEHeaders::SERVER_SIDE_ENCRYPTION_CUSTOMER_KEY_MD5, Aws::Utils::HashingUtils::Base64Encode(Aws::Utils::HashingUtils::CalculateMD5(strBuffer)));
    return AWSClient::GeneratePresignedUrl(uri, method, computeEndpointOutcome.GetResult().second.c_str(), headers, expirationInSeconds);
}

Aws::String S3Client::GeneratePresignedUrlWithSSEC(const Aws::String& bucket, const Aws::String& key, Aws::Http::HttpMethod method, Http::HeaderValueCollection customizedHeaders, const Aws::String& base64EncodedAES256Key, long long expirationInSeconds)
{
    ComputeEndpointOutcome computeEndpointOutcome = ComputeEndpointString(bucket);
    if (!computeEndpointOutcome.IsSuccess())
    {
        AWS_LOGSTREAM_ERROR(ALLOCATION_TAG, "Presigned URL generating failed. Encountered error: " << computeEndpointOutcome.GetError());
        return {};
    }
    Aws::StringStream ss;
    ss << computeEndpointOutcome.GetResult().first << "/" << key;
    URI uri(ss.str());
    customizedHeaders.emplace(Aws::S3::SSEHeaders::SERVER_SIDE_ENCRYPTION_CUSTOMER_ALGORITHM, Aws::S3::Model::ServerSideEncryptionMapper::GetNameForServerSideEncryption(Aws::S3::Model::ServerSideEncryption::AES256));
    customizedHeaders.emplace(Aws::S3::SSEHeaders::SERVER_SIDE_ENCRYPTION_CUSTOMER_KEY, base64EncodedAES256Key);
    Aws::Utils::ByteBuffer buffer = Aws::Utils::HashingUtils::Base64Decode(base64EncodedAES256Key);
    Aws::String strBuffer(reinterpret_cast<char*>(buffer.GetUnderlyingData()), buffer.GetLength());
    customizedHeaders.emplace(Aws::S3::SSEHeaders::SERVER_SIDE_ENCRYPTION_CUSTOMER_KEY_MD5, Aws::Utils::HashingUtils::Base64Encode(Aws::Utils::HashingUtils::CalculateMD5(strBuffer)));
    return AWSClient::GeneratePresignedUrl(uri, method, computeEndpointOutcome.GetResult().second.c_str(), customizedHeaders, expirationInSeconds);
}

ComputeEndpointOutcome S3Client::ComputeEndpointString(const Aws::String& bucketOrArn) const
{
    Aws::StringStream ss;
    Aws::String bucket = bucketOrArn;
    Aws::String signerRegion = m_region;
    S3ARN arn(bucketOrArn);

    if (arn)
    {
        if (m_useCustomEndpoint)
        {
            return ComputeEndpointOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::VALIDATION, "VALIDATION",
                "Custom endpoint is not compatible with Access Point ARN in Bucket field.", false));
        }

        if (!m_useVirtualAddressing)
        {
            return ComputeEndpointOutcome(Aws::Client::AWSError<S3Errors>(S3Errors::VALIDATION, "VALIDATION",
                "Path style addressing is not compatible with Access Point ARN in Bucket field, please consider using virtual addressing for this client instead.", false));
        }

        S3ARNOutcome s3ArnOutcome = m_useArnRegion ? arn.Validate() : arn.Validate(m_region.c_str());
        if (!s3ArnOutcome.IsSuccess())
        {
            return ComputeEndpointOutcome(s3ArnOutcome.GetError());
        }
        if (arn.GetResourceType() == ARNResourceType::ACCESSPOINT)
        {
            ss << m_scheme << "://" << S3Endpoint::ForAccessPointArn(arn, m_useArnRegion ? "" : m_region, m_useDualStack);
            signerRegion = m_useArnRegion ? arn.GetRegion() : m_region;
            if (signerRegion == "fips-us-gov-west-1")
            {
                signerRegion = "us-gov-west-1";
            }
            return ComputeEndpointOutcome(EndpointRegionPair(ss.str(), signerRegion));
        }
    }

    // when using virtual hosting of buckets, the bucket name has to follow some rules.
    // Mainly, it has to be a valid DNS label, and it must be lowercase.
    // For more information see http://docs.aws.amazon.com/AmazonS3/latest/dev/VirtualHosting.html#VirtualHostingSpecifyBucket
    if(m_useVirtualAddressing && Aws::Utils::IsValidDnsLabel(bucket) &&
        bucket == Aws::Utils::StringUtils::ToLower(bucket.c_str()))
    {
        ss << m_scheme << "://" << bucket << "." << m_baseUri;
    }
    else
    {
        ss << m_scheme << "://" << m_baseUri << "/" << bucket;
    }

    if (m_region == "fips-us-gov-west-1")
    {
      signerRegion = "us-gov-west-1";
    }

    return ComputeEndpointOutcome(EndpointRegionPair(ss.str(), signerRegion));
}

ComputeEndpointOutcome S3Client::ComputeEndpointString() const
{
    Aws::String signerRegion = m_region;
    Aws::StringStream ss;
    ss << m_scheme << "://" << m_baseUri;
    if (m_region == "fips-us-gov-west-1")
    {
      signerRegion = "us-gov-west-1";
    }
    return ComputeEndpointOutcome(EndpointRegionPair(ss.str(), signerRegion));
}

bool S3Client::MultipartUploadSupported() const
{
    return true;
}
