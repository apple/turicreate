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
#include <aws/s3/model/Owner.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/DateTime.h>
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
   * <p>Information about the delete marker.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/DeleteMarkerEntry">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API DeleteMarkerEntry
  {
  public:
    DeleteMarkerEntry();
    DeleteMarkerEntry(const Aws::Utils::Xml::XmlNode& xmlNode);
    DeleteMarkerEntry& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>The account that created the delete marker.&gt;</p>
     */
    inline const Owner& GetOwner() const{ return m_owner; }

    /**
     * <p>The account that created the delete marker.&gt;</p>
     */
    inline bool OwnerHasBeenSet() const { return m_ownerHasBeenSet; }

    /**
     * <p>The account that created the delete marker.&gt;</p>
     */
    inline void SetOwner(const Owner& value) { m_ownerHasBeenSet = true; m_owner = value; }

    /**
     * <p>The account that created the delete marker.&gt;</p>
     */
    inline void SetOwner(Owner&& value) { m_ownerHasBeenSet = true; m_owner = std::move(value); }

    /**
     * <p>The account that created the delete marker.&gt;</p>
     */
    inline DeleteMarkerEntry& WithOwner(const Owner& value) { SetOwner(value); return *this;}

    /**
     * <p>The account that created the delete marker.&gt;</p>
     */
    inline DeleteMarkerEntry& WithOwner(Owner&& value) { SetOwner(std::move(value)); return *this;}


    /**
     * <p>The object key.</p>
     */
    inline const Aws::String& GetKey() const{ return m_key; }

    /**
     * <p>The object key.</p>
     */
    inline bool KeyHasBeenSet() const { return m_keyHasBeenSet; }

    /**
     * <p>The object key.</p>
     */
    inline void SetKey(const Aws::String& value) { m_keyHasBeenSet = true; m_key = value; }

    /**
     * <p>The object key.</p>
     */
    inline void SetKey(Aws::String&& value) { m_keyHasBeenSet = true; m_key = std::move(value); }

    /**
     * <p>The object key.</p>
     */
    inline void SetKey(const char* value) { m_keyHasBeenSet = true; m_key.assign(value); }

    /**
     * <p>The object key.</p>
     */
    inline DeleteMarkerEntry& WithKey(const Aws::String& value) { SetKey(value); return *this;}

    /**
     * <p>The object key.</p>
     */
    inline DeleteMarkerEntry& WithKey(Aws::String&& value) { SetKey(std::move(value)); return *this;}

    /**
     * <p>The object key.</p>
     */
    inline DeleteMarkerEntry& WithKey(const char* value) { SetKey(value); return *this;}


    /**
     * <p>Version ID of an object.</p>
     */
    inline const Aws::String& GetVersionId() const{ return m_versionId; }

    /**
     * <p>Version ID of an object.</p>
     */
    inline bool VersionIdHasBeenSet() const { return m_versionIdHasBeenSet; }

    /**
     * <p>Version ID of an object.</p>
     */
    inline void SetVersionId(const Aws::String& value) { m_versionIdHasBeenSet = true; m_versionId = value; }

    /**
     * <p>Version ID of an object.</p>
     */
    inline void SetVersionId(Aws::String&& value) { m_versionIdHasBeenSet = true; m_versionId = std::move(value); }

    /**
     * <p>Version ID of an object.</p>
     */
    inline void SetVersionId(const char* value) { m_versionIdHasBeenSet = true; m_versionId.assign(value); }

    /**
     * <p>Version ID of an object.</p>
     */
    inline DeleteMarkerEntry& WithVersionId(const Aws::String& value) { SetVersionId(value); return *this;}

    /**
     * <p>Version ID of an object.</p>
     */
    inline DeleteMarkerEntry& WithVersionId(Aws::String&& value) { SetVersionId(std::move(value)); return *this;}

    /**
     * <p>Version ID of an object.</p>
     */
    inline DeleteMarkerEntry& WithVersionId(const char* value) { SetVersionId(value); return *this;}


    /**
     * <p>Specifies whether the object is (true) or is not (false) the latest version
     * of an object.</p>
     */
    inline bool GetIsLatest() const{ return m_isLatest; }

    /**
     * <p>Specifies whether the object is (true) or is not (false) the latest version
     * of an object.</p>
     */
    inline bool IsLatestHasBeenSet() const { return m_isLatestHasBeenSet; }

    /**
     * <p>Specifies whether the object is (true) or is not (false) the latest version
     * of an object.</p>
     */
    inline void SetIsLatest(bool value) { m_isLatestHasBeenSet = true; m_isLatest = value; }

    /**
     * <p>Specifies whether the object is (true) or is not (false) the latest version
     * of an object.</p>
     */
    inline DeleteMarkerEntry& WithIsLatest(bool value) { SetIsLatest(value); return *this;}


    /**
     * <p>Date and time the object was last modified.</p>
     */
    inline const Aws::Utils::DateTime& GetLastModified() const{ return m_lastModified; }

    /**
     * <p>Date and time the object was last modified.</p>
     */
    inline bool LastModifiedHasBeenSet() const { return m_lastModifiedHasBeenSet; }

    /**
     * <p>Date and time the object was last modified.</p>
     */
    inline void SetLastModified(const Aws::Utils::DateTime& value) { m_lastModifiedHasBeenSet = true; m_lastModified = value; }

    /**
     * <p>Date and time the object was last modified.</p>
     */
    inline void SetLastModified(Aws::Utils::DateTime&& value) { m_lastModifiedHasBeenSet = true; m_lastModified = std::move(value); }

    /**
     * <p>Date and time the object was last modified.</p>
     */
    inline DeleteMarkerEntry& WithLastModified(const Aws::Utils::DateTime& value) { SetLastModified(value); return *this;}

    /**
     * <p>Date and time the object was last modified.</p>
     */
    inline DeleteMarkerEntry& WithLastModified(Aws::Utils::DateTime&& value) { SetLastModified(std::move(value)); return *this;}

  private:

    Owner m_owner;
    bool m_ownerHasBeenSet;

    Aws::String m_key;
    bool m_keyHasBeenSet;

    Aws::String m_versionId;
    bool m_versionIdHasBeenSet;

    bool m_isLatest;
    bool m_isLatestHasBeenSet;

    Aws::Utils::DateTime m_lastModified;
    bool m_lastModifiedHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
