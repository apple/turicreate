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
#include <aws/core/utils/DateTime.h>
#include <aws/s3/model/StorageClass.h>
#include <aws/s3/model/Owner.h>
#include <aws/s3/model/Initiator.h>
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
   * <p>Container for the <code>MultipartUpload</code> for the Amazon S3
   * object.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/MultipartUpload">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API MultipartUpload
  {
  public:
    MultipartUpload();
    MultipartUpload(const Aws::Utils::Xml::XmlNode& xmlNode);
    MultipartUpload& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Upload ID that identifies the multipart upload.</p>
     */
    inline const Aws::String& GetUploadId() const{ return m_uploadId; }

    /**
     * <p>Upload ID that identifies the multipart upload.</p>
     */
    inline bool UploadIdHasBeenSet() const { return m_uploadIdHasBeenSet; }

    /**
     * <p>Upload ID that identifies the multipart upload.</p>
     */
    inline void SetUploadId(const Aws::String& value) { m_uploadIdHasBeenSet = true; m_uploadId = value; }

    /**
     * <p>Upload ID that identifies the multipart upload.</p>
     */
    inline void SetUploadId(Aws::String&& value) { m_uploadIdHasBeenSet = true; m_uploadId = std::move(value); }

    /**
     * <p>Upload ID that identifies the multipart upload.</p>
     */
    inline void SetUploadId(const char* value) { m_uploadIdHasBeenSet = true; m_uploadId.assign(value); }

    /**
     * <p>Upload ID that identifies the multipart upload.</p>
     */
    inline MultipartUpload& WithUploadId(const Aws::String& value) { SetUploadId(value); return *this;}

    /**
     * <p>Upload ID that identifies the multipart upload.</p>
     */
    inline MultipartUpload& WithUploadId(Aws::String&& value) { SetUploadId(std::move(value)); return *this;}

    /**
     * <p>Upload ID that identifies the multipart upload.</p>
     */
    inline MultipartUpload& WithUploadId(const char* value) { SetUploadId(value); return *this;}


    /**
     * <p>Key of the object for which the multipart upload was initiated.</p>
     */
    inline const Aws::String& GetKey() const{ return m_key; }

    /**
     * <p>Key of the object for which the multipart upload was initiated.</p>
     */
    inline bool KeyHasBeenSet() const { return m_keyHasBeenSet; }

    /**
     * <p>Key of the object for which the multipart upload was initiated.</p>
     */
    inline void SetKey(const Aws::String& value) { m_keyHasBeenSet = true; m_key = value; }

    /**
     * <p>Key of the object for which the multipart upload was initiated.</p>
     */
    inline void SetKey(Aws::String&& value) { m_keyHasBeenSet = true; m_key = std::move(value); }

    /**
     * <p>Key of the object for which the multipart upload was initiated.</p>
     */
    inline void SetKey(const char* value) { m_keyHasBeenSet = true; m_key.assign(value); }

    /**
     * <p>Key of the object for which the multipart upload was initiated.</p>
     */
    inline MultipartUpload& WithKey(const Aws::String& value) { SetKey(value); return *this;}

    /**
     * <p>Key of the object for which the multipart upload was initiated.</p>
     */
    inline MultipartUpload& WithKey(Aws::String&& value) { SetKey(std::move(value)); return *this;}

    /**
     * <p>Key of the object for which the multipart upload was initiated.</p>
     */
    inline MultipartUpload& WithKey(const char* value) { SetKey(value); return *this;}


    /**
     * <p>Date and time at which the multipart upload was initiated.</p>
     */
    inline const Aws::Utils::DateTime& GetInitiated() const{ return m_initiated; }

    /**
     * <p>Date and time at which the multipart upload was initiated.</p>
     */
    inline bool InitiatedHasBeenSet() const { return m_initiatedHasBeenSet; }

    /**
     * <p>Date and time at which the multipart upload was initiated.</p>
     */
    inline void SetInitiated(const Aws::Utils::DateTime& value) { m_initiatedHasBeenSet = true; m_initiated = value; }

    /**
     * <p>Date and time at which the multipart upload was initiated.</p>
     */
    inline void SetInitiated(Aws::Utils::DateTime&& value) { m_initiatedHasBeenSet = true; m_initiated = std::move(value); }

    /**
     * <p>Date and time at which the multipart upload was initiated.</p>
     */
    inline MultipartUpload& WithInitiated(const Aws::Utils::DateTime& value) { SetInitiated(value); return *this;}

    /**
     * <p>Date and time at which the multipart upload was initiated.</p>
     */
    inline MultipartUpload& WithInitiated(Aws::Utils::DateTime&& value) { SetInitiated(std::move(value)); return *this;}


    /**
     * <p>The class of storage used to store the object.</p>
     */
    inline const StorageClass& GetStorageClass() const{ return m_storageClass; }

    /**
     * <p>The class of storage used to store the object.</p>
     */
    inline bool StorageClassHasBeenSet() const { return m_storageClassHasBeenSet; }

    /**
     * <p>The class of storage used to store the object.</p>
     */
    inline void SetStorageClass(const StorageClass& value) { m_storageClassHasBeenSet = true; m_storageClass = value; }

    /**
     * <p>The class of storage used to store the object.</p>
     */
    inline void SetStorageClass(StorageClass&& value) { m_storageClassHasBeenSet = true; m_storageClass = std::move(value); }

    /**
     * <p>The class of storage used to store the object.</p>
     */
    inline MultipartUpload& WithStorageClass(const StorageClass& value) { SetStorageClass(value); return *this;}

    /**
     * <p>The class of storage used to store the object.</p>
     */
    inline MultipartUpload& WithStorageClass(StorageClass&& value) { SetStorageClass(std::move(value)); return *this;}


    /**
     * <p>Specifies the owner of the object that is part of the multipart upload. </p>
     */
    inline const Owner& GetOwner() const{ return m_owner; }

    /**
     * <p>Specifies the owner of the object that is part of the multipart upload. </p>
     */
    inline bool OwnerHasBeenSet() const { return m_ownerHasBeenSet; }

    /**
     * <p>Specifies the owner of the object that is part of the multipart upload. </p>
     */
    inline void SetOwner(const Owner& value) { m_ownerHasBeenSet = true; m_owner = value; }

    /**
     * <p>Specifies the owner of the object that is part of the multipart upload. </p>
     */
    inline void SetOwner(Owner&& value) { m_ownerHasBeenSet = true; m_owner = std::move(value); }

    /**
     * <p>Specifies the owner of the object that is part of the multipart upload. </p>
     */
    inline MultipartUpload& WithOwner(const Owner& value) { SetOwner(value); return *this;}

    /**
     * <p>Specifies the owner of the object that is part of the multipart upload. </p>
     */
    inline MultipartUpload& WithOwner(Owner&& value) { SetOwner(std::move(value)); return *this;}


    /**
     * <p>Identifies who initiated the multipart upload.</p>
     */
    inline const Initiator& GetInitiator() const{ return m_initiator; }

    /**
     * <p>Identifies who initiated the multipart upload.</p>
     */
    inline bool InitiatorHasBeenSet() const { return m_initiatorHasBeenSet; }

    /**
     * <p>Identifies who initiated the multipart upload.</p>
     */
    inline void SetInitiator(const Initiator& value) { m_initiatorHasBeenSet = true; m_initiator = value; }

    /**
     * <p>Identifies who initiated the multipart upload.</p>
     */
    inline void SetInitiator(Initiator&& value) { m_initiatorHasBeenSet = true; m_initiator = std::move(value); }

    /**
     * <p>Identifies who initiated the multipart upload.</p>
     */
    inline MultipartUpload& WithInitiator(const Initiator& value) { SetInitiator(value); return *this;}

    /**
     * <p>Identifies who initiated the multipart upload.</p>
     */
    inline MultipartUpload& WithInitiator(Initiator&& value) { SetInitiator(std::move(value)); return *this;}

  private:

    Aws::String m_uploadId;
    bool m_uploadIdHasBeenSet;

    Aws::String m_key;
    bool m_keyHasBeenSet;

    Aws::Utils::DateTime m_initiated;
    bool m_initiatedHasBeenSet;

    StorageClass m_storageClass;
    bool m_storageClassHasBeenSet;

    Owner m_owner;
    bool m_ownerHasBeenSet;

    Initiator m_initiator;
    bool m_initiatorHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
