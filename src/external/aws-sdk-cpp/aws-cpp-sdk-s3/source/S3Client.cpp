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
#include <aws/s3/S3Client.h>
#include <aws/s3/S3Endpoint.h>
#include <aws/s3/S3ErrorMarshaller.h>
#include <aws/s3/model/AbortMultipartUploadRequest.h>
#include <aws/s3/model/CompleteMultipartUploadRequest.h>
#include <aws/s3/model/CopyObjectRequest.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/CreateMultipartUploadRequest.h>
#include <aws/s3/model/DeleteBucketRequest.h>
#include <aws/s3/model/DeleteBucketCorsRequest.h>
#include <aws/s3/model/DeleteBucketLifecycleRequest.h>
#include <aws/s3/model/DeleteBucketPolicyRequest.h>
#include <aws/s3/model/DeleteBucketReplicationRequest.h>
#include <aws/s3/model/DeleteBucketTaggingRequest.h>
#include <aws/s3/model/DeleteBucketWebsiteRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/DeleteObjectsRequest.h>
#include <aws/s3/model/GetBucketAccelerateConfigurationRequest.h>
#include <aws/s3/model/GetBucketAclRequest.h>
#include <aws/s3/model/GetBucketCorsRequest.h>
#include <aws/s3/model/GetBucketLifecycleConfigurationRequest.h>
#include <aws/s3/model/GetBucketLocationRequest.h>
#include <aws/s3/model/GetBucketLoggingRequest.h>
#include <aws/s3/model/GetBucketNotificationConfigurationRequest.h>
#include <aws/s3/model/GetBucketPolicyRequest.h>
#include <aws/s3/model/GetBucketReplicationRequest.h>
#include <aws/s3/model/GetBucketRequestPaymentRequest.h>
#include <aws/s3/model/GetBucketTaggingRequest.h>
#include <aws/s3/model/GetBucketVersioningRequest.h>
#include <aws/s3/model/GetBucketWebsiteRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/GetObjectAclRequest.h>
#include <aws/s3/model/GetObjectTorrentRequest.h>
#include <aws/s3/model/HeadBucketRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/ListMultipartUploadsRequest.h>
#include <aws/s3/model/ListObjectVersionsRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <aws/s3/model/ListPartsRequest.h>
#include <aws/s3/model/PutBucketAccelerateConfigurationRequest.h>
#include <aws/s3/model/PutBucketAclRequest.h>
#include <aws/s3/model/PutBucketCorsRequest.h>
#include <aws/s3/model/PutBucketLifecycleConfigurationRequest.h>
#include <aws/s3/model/PutBucketLoggingRequest.h>
#include <aws/s3/model/PutBucketNotificationConfigurationRequest.h>
#include <aws/s3/model/PutBucketPolicyRequest.h>
#include <aws/s3/model/PutBucketReplicationRequest.h>
#include <aws/s3/model/PutBucketRequestPaymentRequest.h>
#include <aws/s3/model/PutBucketTaggingRequest.h>
#include <aws/s3/model/PutBucketVersioningRequest.h>
#include <aws/s3/model/PutBucketWebsiteRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/PutObjectAclRequest.h>
#include <aws/s3/model/RestoreObjectRequest.h>
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


S3Client::S3Client(const Client::ClientConfiguration& clientConfiguration, bool signPayloads) :
  BASECLASS(clientConfiguration,
    Aws::MakeShared<AWSAuthV4Signer>(ALLOCATION_TAG, Aws::MakeShared<DefaultAWSCredentialsProviderChain>(ALLOCATION_TAG),
        SERVICE_NAME, clientConfiguration.region, signPayloads, false),
    Aws::MakeShared<S3ErrorMarshaller>(ALLOCATION_TAG)),
    m_executor(clientConfiguration.executor)
{
  init(clientConfiguration);
}

S3Client::S3Client(const AWSCredentials& credentials, const Client::ClientConfiguration& clientConfiguration, bool signPayloads) :
  BASECLASS(clientConfiguration,
    Aws::MakeShared<AWSAuthV4Signer>(ALLOCATION_TAG, Aws::MakeShared<SimpleAWSCredentialsProvider>(ALLOCATION_TAG, credentials),
         SERVICE_NAME, clientConfiguration.region, signPayloads, false),
    Aws::MakeShared<S3ErrorMarshaller>(ALLOCATION_TAG)),
    m_executor(clientConfiguration.executor)
{
  init(clientConfiguration);
}

S3Client::S3Client(const std::shared_ptr<AWSCredentialsProvider>& credentialsProvider,
  const Client::ClientConfiguration& clientConfiguration, bool signPayloads) :
  BASECLASS(clientConfiguration,
    Aws::MakeShared<AWSAuthV4Signer>(ALLOCATION_TAG, credentialsProvider,
         SERVICE_NAME, clientConfiguration.region, signPayloads, false),
    Aws::MakeShared<S3ErrorMarshaller>(ALLOCATION_TAG)),
    m_executor(clientConfiguration.executor)
{
  init(clientConfiguration);
}

S3Client::~S3Client()
{
}

void S3Client::init(const ClientConfiguration& config)
{
  Aws::StringStream ss;
  ss << SchemeMapper::ToString(config.scheme) << "://";

  if(config.endpointOverride.empty())
  {
    ss << S3Endpoint::ForRegion(config.region, config.useDualStack);
  }
  else
  {
    ss << config.endpointOverride;
  }

  m_uri = ss.str();
}

AbortMultipartUploadOutcome S3Client::AbortMultipartUpload(const AbortMultipartUploadRequest& request) const
{
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "/";
  ss << request.GetKey();
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_DELETE);
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
  return std::async(std::launch::async, [this, request](){ return this->AbortMultipartUpload( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "/";
  ss << request.GetKey();
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_POST);
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
  return std::async(std::launch::async, [this, request](){ return this->CompleteMultipartUpload( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "/";
  ss << request.GetKey();
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_PUT);
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
  return std::async(std::launch::async, [this, request](){ return this->CopyObject( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_PUT);
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
  return std::async(std::launch::async, [this, request](){ return this->CreateBucket( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "/";
  ss << request.GetKey();
  ss << "?uploads";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_POST);
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
  return std::async(std::launch::async, [this, request](){ return this->CreateMultipartUpload( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_DELETE);
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
  return std::async(std::launch::async, [this, request](){ return this->DeleteBucket( request ); } );
}

void S3Client::DeleteBucketAsync(const DeleteBucketRequest& request, const DeleteBucketResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->DeleteBucketAsyncHelper( request, handler, context ); } );
}

void S3Client::DeleteBucketAsyncHelper(const DeleteBucketRequest& request, const DeleteBucketResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, DeleteBucket(request), context);
}

DeleteBucketCorsOutcome S3Client::DeleteBucketCors(const DeleteBucketCorsRequest& request) const
{
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?cors";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_DELETE);
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
  return std::async(std::launch::async, [this, request](){ return this->DeleteBucketCors( request ); } );
}

void S3Client::DeleteBucketCorsAsync(const DeleteBucketCorsRequest& request, const DeleteBucketCorsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->DeleteBucketCorsAsyncHelper( request, handler, context ); } );
}

void S3Client::DeleteBucketCorsAsyncHelper(const DeleteBucketCorsRequest& request, const DeleteBucketCorsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, DeleteBucketCors(request), context);
}

DeleteBucketLifecycleOutcome S3Client::DeleteBucketLifecycle(const DeleteBucketLifecycleRequest& request) const
{
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?lifecycle";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_DELETE);
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
  return std::async(std::launch::async, [this, request](){ return this->DeleteBucketLifecycle( request ); } );
}

void S3Client::DeleteBucketLifecycleAsync(const DeleteBucketLifecycleRequest& request, const DeleteBucketLifecycleResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->DeleteBucketLifecycleAsyncHelper( request, handler, context ); } );
}

void S3Client::DeleteBucketLifecycleAsyncHelper(const DeleteBucketLifecycleRequest& request, const DeleteBucketLifecycleResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, DeleteBucketLifecycle(request), context);
}

DeleteBucketPolicyOutcome S3Client::DeleteBucketPolicy(const DeleteBucketPolicyRequest& request) const
{
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?policy";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_DELETE);
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
  return std::async(std::launch::async, [this, request](){ return this->DeleteBucketPolicy( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?replication";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_DELETE);
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
  return std::async(std::launch::async, [this, request](){ return this->DeleteBucketReplication( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?tagging";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_DELETE);
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
  return std::async(std::launch::async, [this, request](){ return this->DeleteBucketTagging( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?website";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_DELETE);
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
  return std::async(std::launch::async, [this, request](){ return this->DeleteBucketWebsite( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "/";
  ss << request.GetKey();
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_DELETE);
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
  return std::async(std::launch::async, [this, request](){ return this->DeleteObject( request ); } );
}

void S3Client::DeleteObjectAsync(const DeleteObjectRequest& request, const DeleteObjectResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->DeleteObjectAsyncHelper( request, handler, context ); } );
}

void S3Client::DeleteObjectAsyncHelper(const DeleteObjectRequest& request, const DeleteObjectResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, DeleteObject(request), context);
}

DeleteObjectsOutcome S3Client::DeleteObjects(const DeleteObjectsRequest& request) const
{
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?delete";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_POST);
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
  return std::async(std::launch::async, [this, request](){ return this->DeleteObjects( request ); } );
}

void S3Client::DeleteObjectsAsync(const DeleteObjectsRequest& request, const DeleteObjectsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->DeleteObjectsAsyncHelper( request, handler, context ); } );
}

void S3Client::DeleteObjectsAsyncHelper(const DeleteObjectsRequest& request, const DeleteObjectsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, DeleteObjects(request), context);
}

GetBucketAccelerateConfigurationOutcome S3Client::GetBucketAccelerateConfiguration(const GetBucketAccelerateConfigurationRequest& request) const
{
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?accelerate";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_GET);
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
  return std::async(std::launch::async, [this, request](){ return this->GetBucketAccelerateConfiguration( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?acl";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_GET);
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
  return std::async(std::launch::async, [this, request](){ return this->GetBucketAcl( request ); } );
}

void S3Client::GetBucketAclAsync(const GetBucketAclRequest& request, const GetBucketAclResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetBucketAclAsyncHelper( request, handler, context ); } );
}

void S3Client::GetBucketAclAsyncHelper(const GetBucketAclRequest& request, const GetBucketAclResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetBucketAcl(request), context);
}

GetBucketCorsOutcome S3Client::GetBucketCors(const GetBucketCorsRequest& request) const
{
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?cors";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_GET);
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
  return std::async(std::launch::async, [this, request](){ return this->GetBucketCors( request ); } );
}

void S3Client::GetBucketCorsAsync(const GetBucketCorsRequest& request, const GetBucketCorsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetBucketCorsAsyncHelper( request, handler, context ); } );
}

void S3Client::GetBucketCorsAsyncHelper(const GetBucketCorsRequest& request, const GetBucketCorsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetBucketCors(request), context);
}

GetBucketLifecycleConfigurationOutcome S3Client::GetBucketLifecycleConfiguration(const GetBucketLifecycleConfigurationRequest& request) const
{
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?lifecycle";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_GET);
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
  return std::async(std::launch::async, [this, request](){ return this->GetBucketLifecycleConfiguration( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?location";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_GET);
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
  return std::async(std::launch::async, [this, request](){ return this->GetBucketLocation( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?logging";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_GET);
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
  return std::async(std::launch::async, [this, request](){ return this->GetBucketLogging( request ); } );
}

void S3Client::GetBucketLoggingAsync(const GetBucketLoggingRequest& request, const GetBucketLoggingResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetBucketLoggingAsyncHelper( request, handler, context ); } );
}

void S3Client::GetBucketLoggingAsyncHelper(const GetBucketLoggingRequest& request, const GetBucketLoggingResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetBucketLogging(request), context);
}

GetBucketNotificationConfigurationOutcome S3Client::GetBucketNotificationConfiguration(const GetBucketNotificationConfigurationRequest& request) const
{
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?notification";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_GET);
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
  return std::async(std::launch::async, [this, request](){ return this->GetBucketNotificationConfiguration( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?policy";
  StreamOutcome outcome = MakeRequestWithUnparsedResponse(ss.str(), request, HttpMethod::HTTP_GET);
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
  return std::async(std::launch::async, [this, request](){ return this->GetBucketPolicy( request ); } );
}

void S3Client::GetBucketPolicyAsync(const GetBucketPolicyRequest& request, const GetBucketPolicyResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetBucketPolicyAsyncHelper( request, handler, context ); } );
}

void S3Client::GetBucketPolicyAsyncHelper(const GetBucketPolicyRequest& request, const GetBucketPolicyResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetBucketPolicy(request), context);
}

GetBucketReplicationOutcome S3Client::GetBucketReplication(const GetBucketReplicationRequest& request) const
{
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?replication";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_GET);
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
  return std::async(std::launch::async, [this, request](){ return this->GetBucketReplication( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?requestPayment";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_GET);
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
  return std::async(std::launch::async, [this, request](){ return this->GetBucketRequestPayment( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?tagging";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_GET);
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
  return std::async(std::launch::async, [this, request](){ return this->GetBucketTagging( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?versioning";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_GET);
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
  return std::async(std::launch::async, [this, request](){ return this->GetBucketVersioning( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?website";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_GET);
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
  return std::async(std::launch::async, [this, request](){ return this->GetBucketWebsite( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "/";
  ss << request.GetKey();
  StreamOutcome outcome = MakeRequestWithUnparsedResponse(ss.str(), request, HttpMethod::HTTP_GET);
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
  return std::async(std::launch::async, [this, request](){ return this->GetObject( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "/";
  ss << request.GetKey();
  ss << "?acl";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_GET);
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
  return std::async(std::launch::async, [this, request](){ return this->GetObjectAcl( request ); } );
}

void S3Client::GetObjectAclAsync(const GetObjectAclRequest& request, const GetObjectAclResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetObjectAclAsyncHelper( request, handler, context ); } );
}

void S3Client::GetObjectAclAsyncHelper(const GetObjectAclRequest& request, const GetObjectAclResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetObjectAcl(request), context);
}

GetObjectTorrentOutcome S3Client::GetObjectTorrent(const GetObjectTorrentRequest& request) const
{
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "/";
  ss << request.GetKey();
  ss << "?torrent";
  StreamOutcome outcome = MakeRequestWithUnparsedResponse(ss.str(), request, HttpMethod::HTTP_GET);
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
  return std::async(std::launch::async, [this, request](){ return this->GetObjectTorrent( request ); } );
}

void S3Client::GetObjectTorrentAsync(const GetObjectTorrentRequest& request, const GetObjectTorrentResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->GetObjectTorrentAsyncHelper( request, handler, context ); } );
}

void S3Client::GetObjectTorrentAsyncHelper(const GetObjectTorrentRequest& request, const GetObjectTorrentResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, GetObjectTorrent(request), context);
}

HeadBucketOutcome S3Client::HeadBucket(const HeadBucketRequest& request) const
{
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_HEAD);
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
  return std::async(std::launch::async, [this, request](){ return this->HeadBucket( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "/";
  ss << request.GetKey();
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_HEAD);
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
  return std::async(std::launch::async, [this, request](){ return this->HeadObject( request ); } );
}

void S3Client::HeadObjectAsync(const HeadObjectRequest& request, const HeadObjectResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->HeadObjectAsyncHelper( request, handler, context ); } );
}

void S3Client::HeadObjectAsyncHelper(const HeadObjectRequest& request, const HeadObjectResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, HeadObject(request), context);
}

ListBucketsOutcome S3Client::ListBuckets() const
{
  Aws::StringStream ss;
  ss << m_uri << "/";
  XmlOutcome outcome = MakeRequest(ss.str(), HttpMethod::HTTP_GET);
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
  return std::async(std::launch::async, [this](){ return this->ListBuckets(); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?uploads";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_GET);
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
  return std::async(std::launch::async, [this, request](){ return this->ListMultipartUploads( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?versions";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_GET);
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
  return std::async(std::launch::async, [this, request](){ return this->ListObjectVersions( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_GET);
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
  return std::async(std::launch::async, [this, request](){ return this->ListObjects( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?list-type=2";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_GET);
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
  return std::async(std::launch::async, [this, request](){ return this->ListObjectsV2( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "/";
  ss << request.GetKey();
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_GET);
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
  return std::async(std::launch::async, [this, request](){ return this->ListParts( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?accelerate";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_PUT);
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
  return std::async(std::launch::async, [this, request](){ return this->PutBucketAccelerateConfiguration( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?acl";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_PUT);
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
  return std::async(std::launch::async, [this, request](){ return this->PutBucketAcl( request ); } );
}

void S3Client::PutBucketAclAsync(const PutBucketAclRequest& request, const PutBucketAclResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutBucketAclAsyncHelper( request, handler, context ); } );
}

void S3Client::PutBucketAclAsyncHelper(const PutBucketAclRequest& request, const PutBucketAclResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutBucketAcl(request), context);
}

PutBucketCorsOutcome S3Client::PutBucketCors(const PutBucketCorsRequest& request) const
{
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?cors";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_PUT);
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
  return std::async(std::launch::async, [this, request](){ return this->PutBucketCors( request ); } );
}

void S3Client::PutBucketCorsAsync(const PutBucketCorsRequest& request, const PutBucketCorsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutBucketCorsAsyncHelper( request, handler, context ); } );
}

void S3Client::PutBucketCorsAsyncHelper(const PutBucketCorsRequest& request, const PutBucketCorsResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutBucketCors(request), context);
}

PutBucketLifecycleConfigurationOutcome S3Client::PutBucketLifecycleConfiguration(const PutBucketLifecycleConfigurationRequest& request) const
{
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?lifecycle";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_PUT);
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
  return std::async(std::launch::async, [this, request](){ return this->PutBucketLifecycleConfiguration( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?logging";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_PUT);
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
  return std::async(std::launch::async, [this, request](){ return this->PutBucketLogging( request ); } );
}

void S3Client::PutBucketLoggingAsync(const PutBucketLoggingRequest& request, const PutBucketLoggingResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutBucketLoggingAsyncHelper( request, handler, context ); } );
}

void S3Client::PutBucketLoggingAsyncHelper(const PutBucketLoggingRequest& request, const PutBucketLoggingResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutBucketLogging(request), context);
}

PutBucketNotificationConfigurationOutcome S3Client::PutBucketNotificationConfiguration(const PutBucketNotificationConfigurationRequest& request) const
{
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?notification";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_PUT);
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
  return std::async(std::launch::async, [this, request](){ return this->PutBucketNotificationConfiguration( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?policy";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_PUT);
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
  return std::async(std::launch::async, [this, request](){ return this->PutBucketPolicy( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?replication";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_PUT);
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
  return std::async(std::launch::async, [this, request](){ return this->PutBucketReplication( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?requestPayment";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_PUT);
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
  return std::async(std::launch::async, [this, request](){ return this->PutBucketRequestPayment( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?tagging";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_PUT);
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
  return std::async(std::launch::async, [this, request](){ return this->PutBucketTagging( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?versioning";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_PUT);
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
  return std::async(std::launch::async, [this, request](){ return this->PutBucketVersioning( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "?website";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_PUT);
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
  return std::async(std::launch::async, [this, request](){ return this->PutBucketWebsite( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "/";
  ss << request.GetKey();
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_PUT);
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
  return std::async(std::launch::async, [this, request](){ return this->PutObject( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "/";
  ss << request.GetKey();
  ss << "?acl";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_PUT);
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
  return std::async(std::launch::async, [this, request](){ return this->PutObjectAcl( request ); } );
}

void S3Client::PutObjectAclAsync(const PutObjectAclRequest& request, const PutObjectAclResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->PutObjectAclAsyncHelper( request, handler, context ); } );
}

void S3Client::PutObjectAclAsyncHelper(const PutObjectAclRequest& request, const PutObjectAclResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, PutObjectAcl(request), context);
}

RestoreObjectOutcome S3Client::RestoreObject(const RestoreObjectRequest& request) const
{
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "/";
  ss << request.GetKey();
  ss << "?restore";
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_POST);
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
  return std::async(std::launch::async, [this, request](){ return this->RestoreObject( request ); } );
}

void S3Client::RestoreObjectAsync(const RestoreObjectRequest& request, const RestoreObjectResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->RestoreObjectAsyncHelper( request, handler, context ); } );
}

void S3Client::RestoreObjectAsyncHelper(const RestoreObjectRequest& request, const RestoreObjectResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, RestoreObject(request), context);
}

UploadPartOutcome S3Client::UploadPart(const UploadPartRequest& request) const
{
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "/";
  ss << request.GetKey();
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_PUT);
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
  return std::async(std::launch::async, [this, request](){ return this->UploadPart( request ); } );
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
  Aws::StringStream ss;
  ss << m_uri << "/";
  ss << request.GetBucket();
  ss << "/";
  ss << request.GetKey();
  XmlOutcome outcome = MakeRequest(ss.str(), request, HttpMethod::HTTP_PUT);
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
  return std::async(std::launch::async, [this, request](){ return this->UploadPartCopy( request ); } );
}

void S3Client::UploadPartCopyAsync(const UploadPartCopyRequest& request, const UploadPartCopyResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  m_executor->Submit( [this, request, handler, context](){ this->UploadPartCopyAsyncHelper( request, handler, context ); } );
}

void S3Client::UploadPartCopyAsyncHelper(const UploadPartCopyRequest& request, const UploadPartCopyResponseReceivedHandler& handler, const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) const
{
  handler(this, request, UploadPartCopy(request), context);
}


Aws::String S3Client::GeneratePresignedUrl(const Aws::String& bucketName, const Aws::String& key, Http::HttpMethod method, long long expirationInSeconds)
{
    Aws::StringStream ss;
    ss << m_uri << "/" << bucketName << "/" << key;
    URI uri(ss.str());
    return AWSClient::GeneratePresignedUrl(uri, method, expirationInSeconds);
}
