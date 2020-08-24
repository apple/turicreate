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
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/s3/model/InventoryFormat.h>
#include <aws/s3/model/InventoryEncryption.h>
#include <utility>

namespace Aws
{
namespace Utils
{
namespace Xml
{
  class XmlNode;
} // namespace Xml
} // namespace Utils
namespace S3
{
namespace Model
{

  /**
   * <p>Contains the bucket name, file format, bucket owner (optional), and prefix
   * (optional) where inventory results are published.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/InventoryS3BucketDestination">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API InventoryS3BucketDestination
  {
  public:
    InventoryS3BucketDestination();
    InventoryS3BucketDestination(const Aws::Utils::Xml::XmlNode& xmlNode);
    InventoryS3BucketDestination& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>The ID of the account that owns the destination bucket.</p>
     */
    inline const Aws::String& GetAccountId() const{ return m_accountId; }

    /**
     * <p>The ID of the account that owns the destination bucket.</p>
     */
    inline bool AccountIdHasBeenSet() const { return m_accountIdHasBeenSet; }

    /**
     * <p>The ID of the account that owns the destination bucket.</p>
     */
    inline void SetAccountId(const Aws::String& value) { m_accountIdHasBeenSet = true; m_accountId = value; }

    /**
     * <p>The ID of the account that owns the destination bucket.</p>
     */
    inline void SetAccountId(Aws::String&& value) { m_accountIdHasBeenSet = true; m_accountId = std::move(value); }

    /**
     * <p>The ID of the account that owns the destination bucket.</p>
     */
    inline void SetAccountId(const char* value) { m_accountIdHasBeenSet = true; m_accountId.assign(value); }

    /**
     * <p>The ID of the account that owns the destination bucket.</p>
     */
    inline InventoryS3BucketDestination& WithAccountId(const Aws::String& value) { SetAccountId(value); return *this;}

    /**
     * <p>The ID of the account that owns the destination bucket.</p>
     */
    inline InventoryS3BucketDestination& WithAccountId(Aws::String&& value) { SetAccountId(std::move(value)); return *this;}

    /**
     * <p>The ID of the account that owns the destination bucket.</p>
     */
    inline InventoryS3BucketDestination& WithAccountId(const char* value) { SetAccountId(value); return *this;}


    /**
     * <p>The Amazon Resource Name (ARN) of the bucket where inventory results will be
     * published.</p>
     */
    inline const Aws::String& GetBucket() const{ return m_bucket; }

    /**
     * <p>The Amazon Resource Name (ARN) of the bucket where inventory results will be
     * published.</p>
     */
    inline bool BucketHasBeenSet() const { return m_bucketHasBeenSet; }

    /**
     * <p>The Amazon Resource Name (ARN) of the bucket where inventory results will be
     * published.</p>
     */
    inline void SetBucket(const Aws::String& value) { m_bucketHasBeenSet = true; m_bucket = value; }

    /**
     * <p>The Amazon Resource Name (ARN) of the bucket where inventory results will be
     * published.</p>
     */
    inline void SetBucket(Aws::String&& value) { m_bucketHasBeenSet = true; m_bucket = std::move(value); }

    /**
     * <p>The Amazon Resource Name (ARN) of the bucket where inventory results will be
     * published.</p>
     */
    inline void SetBucket(const char* value) { m_bucketHasBeenSet = true; m_bucket.assign(value); }

    /**
     * <p>The Amazon Resource Name (ARN) of the bucket where inventory results will be
     * published.</p>
     */
    inline InventoryS3BucketDestination& WithBucket(const Aws::String& value) { SetBucket(value); return *this;}

    /**
     * <p>The Amazon Resource Name (ARN) of the bucket where inventory results will be
     * published.</p>
     */
    inline InventoryS3BucketDestination& WithBucket(Aws::String&& value) { SetBucket(std::move(value)); return *this;}

    /**
     * <p>The Amazon Resource Name (ARN) of the bucket where inventory results will be
     * published.</p>
     */
    inline InventoryS3BucketDestination& WithBucket(const char* value) { SetBucket(value); return *this;}


    /**
     * <p>Specifies the output format of the inventory results.</p>
     */
    inline const InventoryFormat& GetFormat() const{ return m_format; }

    /**
     * <p>Specifies the output format of the inventory results.</p>
     */
    inline bool FormatHasBeenSet() const { return m_formatHasBeenSet; }

    /**
     * <p>Specifies the output format of the inventory results.</p>
     */
    inline void SetFormat(const InventoryFormat& value) { m_formatHasBeenSet = true; m_format = value; }

    /**
     * <p>Specifies the output format of the inventory results.</p>
     */
    inline void SetFormat(InventoryFormat&& value) { m_formatHasBeenSet = true; m_format = std::move(value); }

    /**
     * <p>Specifies the output format of the inventory results.</p>
     */
    inline InventoryS3BucketDestination& WithFormat(const InventoryFormat& value) { SetFormat(value); return *this;}

    /**
     * <p>Specifies the output format of the inventory results.</p>
     */
    inline InventoryS3BucketDestination& WithFormat(InventoryFormat&& value) { SetFormat(std::move(value)); return *this;}


    /**
     * <p>The prefix that is prepended to all inventory results.</p>
     */
    inline const Aws::String& GetPrefix() const{ return m_prefix; }

    /**
     * <p>The prefix that is prepended to all inventory results.</p>
     */
    inline bool PrefixHasBeenSet() const { return m_prefixHasBeenSet; }

    /**
     * <p>The prefix that is prepended to all inventory results.</p>
     */
    inline void SetPrefix(const Aws::String& value) { m_prefixHasBeenSet = true; m_prefix = value; }

    /**
     * <p>The prefix that is prepended to all inventory results.</p>
     */
    inline void SetPrefix(Aws::String&& value) { m_prefixHasBeenSet = true; m_prefix = std::move(value); }

    /**
     * <p>The prefix that is prepended to all inventory results.</p>
     */
    inline void SetPrefix(const char* value) { m_prefixHasBeenSet = true; m_prefix.assign(value); }

    /**
     * <p>The prefix that is prepended to all inventory results.</p>
     */
    inline InventoryS3BucketDestination& WithPrefix(const Aws::String& value) { SetPrefix(value); return *this;}

    /**
     * <p>The prefix that is prepended to all inventory results.</p>
     */
    inline InventoryS3BucketDestination& WithPrefix(Aws::String&& value) { SetPrefix(std::move(value)); return *this;}

    /**
     * <p>The prefix that is prepended to all inventory results.</p>
     */
    inline InventoryS3BucketDestination& WithPrefix(const char* value) { SetPrefix(value); return *this;}


    /**
     * <p>Contains the type of server-side encryption used to encrypt the inventory
     * results.</p>
     */
    inline const InventoryEncryption& GetEncryption() const{ return m_encryption; }

    /**
     * <p>Contains the type of server-side encryption used to encrypt the inventory
     * results.</p>
     */
    inline bool EncryptionHasBeenSet() const { return m_encryptionHasBeenSet; }

    /**
     * <p>Contains the type of server-side encryption used to encrypt the inventory
     * results.</p>
     */
    inline void SetEncryption(const InventoryEncryption& value) { m_encryptionHasBeenSet = true; m_encryption = value; }

    /**
     * <p>Contains the type of server-side encryption used to encrypt the inventory
     * results.</p>
     */
    inline void SetEncryption(InventoryEncryption&& value) { m_encryptionHasBeenSet = true; m_encryption = std::move(value); }

    /**
     * <p>Contains the type of server-side encryption used to encrypt the inventory
     * results.</p>
     */
    inline InventoryS3BucketDestination& WithEncryption(const InventoryEncryption& value) { SetEncryption(value); return *this;}

    /**
     * <p>Contains the type of server-side encryption used to encrypt the inventory
     * results.</p>
     */
    inline InventoryS3BucketDestination& WithEncryption(InventoryEncryption&& value) { SetEncryption(std::move(value)); return *this;}

  private:

    Aws::String m_accountId;
    bool m_accountIdHasBeenSet;

    Aws::String m_bucket;
    bool m_bucketHasBeenSet;

    InventoryFormat m_format;
    bool m_formatHasBeenSet;

    Aws::String m_prefix;
    bool m_prefixHasBeenSet;

    InventoryEncryption m_encryption;
    bool m_encryptionHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
