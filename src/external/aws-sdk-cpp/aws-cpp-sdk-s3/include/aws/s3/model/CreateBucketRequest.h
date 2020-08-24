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

#pragma once
#include <aws/s3/S3_EXPORTS.h>
#include <aws/s3/S3Request.h>
#include <aws/s3/model/BucketCannedACL.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/s3/model/CreateBucketConfiguration.h>
#include <aws/core/utils/memory/stl/AWSMap.h>
#include <utility>

namespace Aws
{
namespace Http
{
    class URI;
} //namespace Http
namespace S3
{
namespace Model
{

  /**
   */
  class AWS_S3_API CreateBucketRequest : public S3Request
  {
  public:
    CreateBucketRequest();

    // Service request name is the Operation name which will send this request out,
    // each operation should has unique request name, so that we can get operation's name from this request.
    // Note: this is not true for response, multiple operations may have the same response name,
    // so we can not get operation's name from response.
    inline virtual const char* GetServiceRequestName() const override { return "CreateBucket"; }

    Aws::String SerializePayload() const override;

    void AddQueryStringParameters(Aws::Http::URI& uri) const override;

    Aws::Http::HeaderValueCollection GetRequestSpecificHeaders() const override;


    /**
     * <p>The canned ACL to apply to the bucket.</p>
     */
    inline const BucketCannedACL& GetACL() const{ return m_aCL; }

    /**
     * <p>The canned ACL to apply to the bucket.</p>
     */
    inline bool ACLHasBeenSet() const { return m_aCLHasBeenSet; }

    /**
     * <p>The canned ACL to apply to the bucket.</p>
     */
    inline void SetACL(const BucketCannedACL& value) { m_aCLHasBeenSet = true; m_aCL = value; }

    /**
     * <p>The canned ACL to apply to the bucket.</p>
     */
    inline void SetACL(BucketCannedACL&& value) { m_aCLHasBeenSet = true; m_aCL = std::move(value); }

    /**
     * <p>The canned ACL to apply to the bucket.</p>
     */
    inline CreateBucketRequest& WithACL(const BucketCannedACL& value) { SetACL(value); return *this;}

    /**
     * <p>The canned ACL to apply to the bucket.</p>
     */
    inline CreateBucketRequest& WithACL(BucketCannedACL&& value) { SetACL(std::move(value)); return *this;}


    /**
     * <p>The name of the bucket to create.</p>
     */
    inline const Aws::String& GetBucket() const{ return m_bucket; }

    /**
     * <p>The name of the bucket to create.</p>
     */
    inline bool BucketHasBeenSet() const { return m_bucketHasBeenSet; }

    /**
     * <p>The name of the bucket to create.</p>
     */
    inline void SetBucket(const Aws::String& value) { m_bucketHasBeenSet = true; m_bucket = value; }

    /**
     * <p>The name of the bucket to create.</p>
     */
    inline void SetBucket(Aws::String&& value) { m_bucketHasBeenSet = true; m_bucket = std::move(value); }

    /**
     * <p>The name of the bucket to create.</p>
     */
    inline void SetBucket(const char* value) { m_bucketHasBeenSet = true; m_bucket.assign(value); }

    /**
     * <p>The name of the bucket to create.</p>
     */
    inline CreateBucketRequest& WithBucket(const Aws::String& value) { SetBucket(value); return *this;}

    /**
     * <p>The name of the bucket to create.</p>
     */
    inline CreateBucketRequest& WithBucket(Aws::String&& value) { SetBucket(std::move(value)); return *this;}

    /**
     * <p>The name of the bucket to create.</p>
     */
    inline CreateBucketRequest& WithBucket(const char* value) { SetBucket(value); return *this;}


    /**
     * <p>The configuration information for the bucket.</p>
     */
    inline const CreateBucketConfiguration& GetCreateBucketConfiguration() const{ return m_createBucketConfiguration; }

    /**
     * <p>The configuration information for the bucket.</p>
     */
    inline bool CreateBucketConfigurationHasBeenSet() const { return m_createBucketConfigurationHasBeenSet; }

    /**
     * <p>The configuration information for the bucket.</p>
     */
    inline void SetCreateBucketConfiguration(const CreateBucketConfiguration& value) { m_createBucketConfigurationHasBeenSet = true; m_createBucketConfiguration = value; }

    /**
     * <p>The configuration information for the bucket.</p>
     */
    inline void SetCreateBucketConfiguration(CreateBucketConfiguration&& value) { m_createBucketConfigurationHasBeenSet = true; m_createBucketConfiguration = std::move(value); }

    /**
     * <p>The configuration information for the bucket.</p>
     */
    inline CreateBucketRequest& WithCreateBucketConfiguration(const CreateBucketConfiguration& value) { SetCreateBucketConfiguration(value); return *this;}

    /**
     * <p>The configuration information for the bucket.</p>
     */
    inline CreateBucketRequest& WithCreateBucketConfiguration(CreateBucketConfiguration&& value) { SetCreateBucketConfiguration(std::move(value)); return *this;}


    /**
     * <p>Allows grantee the read, write, read ACP, and write ACP permissions on the
     * bucket.</p>
     */
    inline const Aws::String& GetGrantFullControl() const{ return m_grantFullControl; }

    /**
     * <p>Allows grantee the read, write, read ACP, and write ACP permissions on the
     * bucket.</p>
     */
    inline bool GrantFullControlHasBeenSet() const { return m_grantFullControlHasBeenSet; }

    /**
     * <p>Allows grantee the read, write, read ACP, and write ACP permissions on the
     * bucket.</p>
     */
    inline void SetGrantFullControl(const Aws::String& value) { m_grantFullControlHasBeenSet = true; m_grantFullControl = value; }

    /**
     * <p>Allows grantee the read, write, read ACP, and write ACP permissions on the
     * bucket.</p>
     */
    inline void SetGrantFullControl(Aws::String&& value) { m_grantFullControlHasBeenSet = true; m_grantFullControl = std::move(value); }

    /**
     * <p>Allows grantee the read, write, read ACP, and write ACP permissions on the
     * bucket.</p>
     */
    inline void SetGrantFullControl(const char* value) { m_grantFullControlHasBeenSet = true; m_grantFullControl.assign(value); }

    /**
     * <p>Allows grantee the read, write, read ACP, and write ACP permissions on the
     * bucket.</p>
     */
    inline CreateBucketRequest& WithGrantFullControl(const Aws::String& value) { SetGrantFullControl(value); return *this;}

    /**
     * <p>Allows grantee the read, write, read ACP, and write ACP permissions on the
     * bucket.</p>
     */
    inline CreateBucketRequest& WithGrantFullControl(Aws::String&& value) { SetGrantFullControl(std::move(value)); return *this;}

    /**
     * <p>Allows grantee the read, write, read ACP, and write ACP permissions on the
     * bucket.</p>
     */
    inline CreateBucketRequest& WithGrantFullControl(const char* value) { SetGrantFullControl(value); return *this;}


    /**
     * <p>Allows grantee to list the objects in the bucket.</p>
     */
    inline const Aws::String& GetGrantRead() const{ return m_grantRead; }

    /**
     * <p>Allows grantee to list the objects in the bucket.</p>
     */
    inline bool GrantReadHasBeenSet() const { return m_grantReadHasBeenSet; }

    /**
     * <p>Allows grantee to list the objects in the bucket.</p>
     */
    inline void SetGrantRead(const Aws::String& value) { m_grantReadHasBeenSet = true; m_grantRead = value; }

    /**
     * <p>Allows grantee to list the objects in the bucket.</p>
     */
    inline void SetGrantRead(Aws::String&& value) { m_grantReadHasBeenSet = true; m_grantRead = std::move(value); }

    /**
     * <p>Allows grantee to list the objects in the bucket.</p>
     */
    inline void SetGrantRead(const char* value) { m_grantReadHasBeenSet = true; m_grantRead.assign(value); }

    /**
     * <p>Allows grantee to list the objects in the bucket.</p>
     */
    inline CreateBucketRequest& WithGrantRead(const Aws::String& value) { SetGrantRead(value); return *this;}

    /**
     * <p>Allows grantee to list the objects in the bucket.</p>
     */
    inline CreateBucketRequest& WithGrantRead(Aws::String&& value) { SetGrantRead(std::move(value)); return *this;}

    /**
     * <p>Allows grantee to list the objects in the bucket.</p>
     */
    inline CreateBucketRequest& WithGrantRead(const char* value) { SetGrantRead(value); return *this;}


    /**
     * <p>Allows grantee to read the bucket ACL.</p>
     */
    inline const Aws::String& GetGrantReadACP() const{ return m_grantReadACP; }

    /**
     * <p>Allows grantee to read the bucket ACL.</p>
     */
    inline bool GrantReadACPHasBeenSet() const { return m_grantReadACPHasBeenSet; }

    /**
     * <p>Allows grantee to read the bucket ACL.</p>
     */
    inline void SetGrantReadACP(const Aws::String& value) { m_grantReadACPHasBeenSet = true; m_grantReadACP = value; }

    /**
     * <p>Allows grantee to read the bucket ACL.</p>
     */
    inline void SetGrantReadACP(Aws::String&& value) { m_grantReadACPHasBeenSet = true; m_grantReadACP = std::move(value); }

    /**
     * <p>Allows grantee to read the bucket ACL.</p>
     */
    inline void SetGrantReadACP(const char* value) { m_grantReadACPHasBeenSet = true; m_grantReadACP.assign(value); }

    /**
     * <p>Allows grantee to read the bucket ACL.</p>
     */
    inline CreateBucketRequest& WithGrantReadACP(const Aws::String& value) { SetGrantReadACP(value); return *this;}

    /**
     * <p>Allows grantee to read the bucket ACL.</p>
     */
    inline CreateBucketRequest& WithGrantReadACP(Aws::String&& value) { SetGrantReadACP(std::move(value)); return *this;}

    /**
     * <p>Allows grantee to read the bucket ACL.</p>
     */
    inline CreateBucketRequest& WithGrantReadACP(const char* value) { SetGrantReadACP(value); return *this;}


    /**
     * <p>Allows grantee to create, overwrite, and delete any object in the bucket.</p>
     */
    inline const Aws::String& GetGrantWrite() const{ return m_grantWrite; }

    /**
     * <p>Allows grantee to create, overwrite, and delete any object in the bucket.</p>
     */
    inline bool GrantWriteHasBeenSet() const { return m_grantWriteHasBeenSet; }

    /**
     * <p>Allows grantee to create, overwrite, and delete any object in the bucket.</p>
     */
    inline void SetGrantWrite(const Aws::String& value) { m_grantWriteHasBeenSet = true; m_grantWrite = value; }

    /**
     * <p>Allows grantee to create, overwrite, and delete any object in the bucket.</p>
     */
    inline void SetGrantWrite(Aws::String&& value) { m_grantWriteHasBeenSet = true; m_grantWrite = std::move(value); }

    /**
     * <p>Allows grantee to create, overwrite, and delete any object in the bucket.</p>
     */
    inline void SetGrantWrite(const char* value) { m_grantWriteHasBeenSet = true; m_grantWrite.assign(value); }

    /**
     * <p>Allows grantee to create, overwrite, and delete any object in the bucket.</p>
     */
    inline CreateBucketRequest& WithGrantWrite(const Aws::String& value) { SetGrantWrite(value); return *this;}

    /**
     * <p>Allows grantee to create, overwrite, and delete any object in the bucket.</p>
     */
    inline CreateBucketRequest& WithGrantWrite(Aws::String&& value) { SetGrantWrite(std::move(value)); return *this;}

    /**
     * <p>Allows grantee to create, overwrite, and delete any object in the bucket.</p>
     */
    inline CreateBucketRequest& WithGrantWrite(const char* value) { SetGrantWrite(value); return *this;}


    /**
     * <p>Allows grantee to write the ACL for the applicable bucket.</p>
     */
    inline const Aws::String& GetGrantWriteACP() const{ return m_grantWriteACP; }

    /**
     * <p>Allows grantee to write the ACL for the applicable bucket.</p>
     */
    inline bool GrantWriteACPHasBeenSet() const { return m_grantWriteACPHasBeenSet; }

    /**
     * <p>Allows grantee to write the ACL for the applicable bucket.</p>
     */
    inline void SetGrantWriteACP(const Aws::String& value) { m_grantWriteACPHasBeenSet = true; m_grantWriteACP = value; }

    /**
     * <p>Allows grantee to write the ACL for the applicable bucket.</p>
     */
    inline void SetGrantWriteACP(Aws::String&& value) { m_grantWriteACPHasBeenSet = true; m_grantWriteACP = std::move(value); }

    /**
     * <p>Allows grantee to write the ACL for the applicable bucket.</p>
     */
    inline void SetGrantWriteACP(const char* value) { m_grantWriteACPHasBeenSet = true; m_grantWriteACP.assign(value); }

    /**
     * <p>Allows grantee to write the ACL for the applicable bucket.</p>
     */
    inline CreateBucketRequest& WithGrantWriteACP(const Aws::String& value) { SetGrantWriteACP(value); return *this;}

    /**
     * <p>Allows grantee to write the ACL for the applicable bucket.</p>
     */
    inline CreateBucketRequest& WithGrantWriteACP(Aws::String&& value) { SetGrantWriteACP(std::move(value)); return *this;}

    /**
     * <p>Allows grantee to write the ACL for the applicable bucket.</p>
     */
    inline CreateBucketRequest& WithGrantWriteACP(const char* value) { SetGrantWriteACP(value); return *this;}


    /**
     * <p>Specifies whether you want S3 Object Lock to be enabled for the new
     * bucket.</p>
     */
    inline bool GetObjectLockEnabledForBucket() const{ return m_objectLockEnabledForBucket; }

    /**
     * <p>Specifies whether you want S3 Object Lock to be enabled for the new
     * bucket.</p>
     */
    inline bool ObjectLockEnabledForBucketHasBeenSet() const { return m_objectLockEnabledForBucketHasBeenSet; }

    /**
     * <p>Specifies whether you want S3 Object Lock to be enabled for the new
     * bucket.</p>
     */
    inline void SetObjectLockEnabledForBucket(bool value) { m_objectLockEnabledForBucketHasBeenSet = true; m_objectLockEnabledForBucket = value; }

    /**
     * <p>Specifies whether you want S3 Object Lock to be enabled for the new
     * bucket.</p>
     */
    inline CreateBucketRequest& WithObjectLockEnabledForBucket(bool value) { SetObjectLockEnabledForBucket(value); return *this;}


    
    inline const Aws::Map<Aws::String, Aws::String>& GetCustomizedAccessLogTag() const{ return m_customizedAccessLogTag; }

    
    inline bool CustomizedAccessLogTagHasBeenSet() const { return m_customizedAccessLogTagHasBeenSet; }

    
    inline void SetCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = value; }

    
    inline void SetCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag = std::move(value); }

    
    inline CreateBucketRequest& WithCustomizedAccessLogTag(const Aws::Map<Aws::String, Aws::String>& value) { SetCustomizedAccessLogTag(value); return *this;}

    
    inline CreateBucketRequest& WithCustomizedAccessLogTag(Aws::Map<Aws::String, Aws::String>&& value) { SetCustomizedAccessLogTag(std::move(value)); return *this;}

    
    inline CreateBucketRequest& AddCustomizedAccessLogTag(const Aws::String& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

    
    inline CreateBucketRequest& AddCustomizedAccessLogTag(Aws::String&& key, const Aws::String& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline CreateBucketRequest& AddCustomizedAccessLogTag(const Aws::String& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline CreateBucketRequest& AddCustomizedAccessLogTag(Aws::String&& key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), std::move(value)); return *this; }

    
    inline CreateBucketRequest& AddCustomizedAccessLogTag(const char* key, Aws::String&& value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, std::move(value)); return *this; }

    
    inline CreateBucketRequest& AddCustomizedAccessLogTag(Aws::String&& key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(std::move(key), value); return *this; }

    
    inline CreateBucketRequest& AddCustomizedAccessLogTag(const char* key, const char* value) { m_customizedAccessLogTagHasBeenSet = true; m_customizedAccessLogTag.emplace(key, value); return *this; }

  private:

    BucketCannedACL m_aCL;
    bool m_aCLHasBeenSet;

    Aws::String m_bucket;
    bool m_bucketHasBeenSet;

    CreateBucketConfiguration m_createBucketConfiguration;
    bool m_createBucketConfigurationHasBeenSet;

    Aws::String m_grantFullControl;
    bool m_grantFullControlHasBeenSet;

    Aws::String m_grantRead;
    bool m_grantReadHasBeenSet;

    Aws::String m_grantReadACP;
    bool m_grantReadACPHasBeenSet;

    Aws::String m_grantWrite;
    bool m_grantWriteHasBeenSet;

    Aws::String m_grantWriteACP;
    bool m_grantWriteACPHasBeenSet;

    bool m_objectLockEnabledForBucket;
    bool m_objectLockEnabledForBucketHasBeenSet;

    Aws::Map<Aws::String, Aws::String> m_customizedAccessLogTag;
    bool m_customizedAccessLogTagHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
