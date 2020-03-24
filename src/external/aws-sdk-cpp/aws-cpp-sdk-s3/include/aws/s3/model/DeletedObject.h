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
   * <p>Information about the deleted object.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/DeletedObject">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API DeletedObject
  {
  public:
    DeletedObject();
    DeletedObject(const Aws::Utils::Xml::XmlNode& xmlNode);
    DeletedObject& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>The name of the deleted object.</p>
     */
    inline const Aws::String& GetKey() const{ return m_key; }

    /**
     * <p>The name of the deleted object.</p>
     */
    inline bool KeyHasBeenSet() const { return m_keyHasBeenSet; }

    /**
     * <p>The name of the deleted object.</p>
     */
    inline void SetKey(const Aws::String& value) { m_keyHasBeenSet = true; m_key = value; }

    /**
     * <p>The name of the deleted object.</p>
     */
    inline void SetKey(Aws::String&& value) { m_keyHasBeenSet = true; m_key = std::move(value); }

    /**
     * <p>The name of the deleted object.</p>
     */
    inline void SetKey(const char* value) { m_keyHasBeenSet = true; m_key.assign(value); }

    /**
     * <p>The name of the deleted object.</p>
     */
    inline DeletedObject& WithKey(const Aws::String& value) { SetKey(value); return *this;}

    /**
     * <p>The name of the deleted object.</p>
     */
    inline DeletedObject& WithKey(Aws::String&& value) { SetKey(std::move(value)); return *this;}

    /**
     * <p>The name of the deleted object.</p>
     */
    inline DeletedObject& WithKey(const char* value) { SetKey(value); return *this;}


    /**
     * <p>The version ID of the deleted object.</p>
     */
    inline const Aws::String& GetVersionId() const{ return m_versionId; }

    /**
     * <p>The version ID of the deleted object.</p>
     */
    inline bool VersionIdHasBeenSet() const { return m_versionIdHasBeenSet; }

    /**
     * <p>The version ID of the deleted object.</p>
     */
    inline void SetVersionId(const Aws::String& value) { m_versionIdHasBeenSet = true; m_versionId = value; }

    /**
     * <p>The version ID of the deleted object.</p>
     */
    inline void SetVersionId(Aws::String&& value) { m_versionIdHasBeenSet = true; m_versionId = std::move(value); }

    /**
     * <p>The version ID of the deleted object.</p>
     */
    inline void SetVersionId(const char* value) { m_versionIdHasBeenSet = true; m_versionId.assign(value); }

    /**
     * <p>The version ID of the deleted object.</p>
     */
    inline DeletedObject& WithVersionId(const Aws::String& value) { SetVersionId(value); return *this;}

    /**
     * <p>The version ID of the deleted object.</p>
     */
    inline DeletedObject& WithVersionId(Aws::String&& value) { SetVersionId(std::move(value)); return *this;}

    /**
     * <p>The version ID of the deleted object.</p>
     */
    inline DeletedObject& WithVersionId(const char* value) { SetVersionId(value); return *this;}


    /**
     * <p>Specifies whether the versioned object that was permanently deleted was
     * (true) or was not (false) a delete marker. In a simple DELETE, this header
     * indicates whether (true) or not (false) a delete marker was created.</p>
     */
    inline bool GetDeleteMarker() const{ return m_deleteMarker; }

    /**
     * <p>Specifies whether the versioned object that was permanently deleted was
     * (true) or was not (false) a delete marker. In a simple DELETE, this header
     * indicates whether (true) or not (false) a delete marker was created.</p>
     */
    inline bool DeleteMarkerHasBeenSet() const { return m_deleteMarkerHasBeenSet; }

    /**
     * <p>Specifies whether the versioned object that was permanently deleted was
     * (true) or was not (false) a delete marker. In a simple DELETE, this header
     * indicates whether (true) or not (false) a delete marker was created.</p>
     */
    inline void SetDeleteMarker(bool value) { m_deleteMarkerHasBeenSet = true; m_deleteMarker = value; }

    /**
     * <p>Specifies whether the versioned object that was permanently deleted was
     * (true) or was not (false) a delete marker. In a simple DELETE, this header
     * indicates whether (true) or not (false) a delete marker was created.</p>
     */
    inline DeletedObject& WithDeleteMarker(bool value) { SetDeleteMarker(value); return *this;}


    /**
     * <p>The version ID of the delete marker created as a result of the DELETE
     * operation. If you delete a specific object version, the value returned by this
     * header is the version ID of the object version deleted.</p>
     */
    inline const Aws::String& GetDeleteMarkerVersionId() const{ return m_deleteMarkerVersionId; }

    /**
     * <p>The version ID of the delete marker created as a result of the DELETE
     * operation. If you delete a specific object version, the value returned by this
     * header is the version ID of the object version deleted.</p>
     */
    inline bool DeleteMarkerVersionIdHasBeenSet() const { return m_deleteMarkerVersionIdHasBeenSet; }

    /**
     * <p>The version ID of the delete marker created as a result of the DELETE
     * operation. If you delete a specific object version, the value returned by this
     * header is the version ID of the object version deleted.</p>
     */
    inline void SetDeleteMarkerVersionId(const Aws::String& value) { m_deleteMarkerVersionIdHasBeenSet = true; m_deleteMarkerVersionId = value; }

    /**
     * <p>The version ID of the delete marker created as a result of the DELETE
     * operation. If you delete a specific object version, the value returned by this
     * header is the version ID of the object version deleted.</p>
     */
    inline void SetDeleteMarkerVersionId(Aws::String&& value) { m_deleteMarkerVersionIdHasBeenSet = true; m_deleteMarkerVersionId = std::move(value); }

    /**
     * <p>The version ID of the delete marker created as a result of the DELETE
     * operation. If you delete a specific object version, the value returned by this
     * header is the version ID of the object version deleted.</p>
     */
    inline void SetDeleteMarkerVersionId(const char* value) { m_deleteMarkerVersionIdHasBeenSet = true; m_deleteMarkerVersionId.assign(value); }

    /**
     * <p>The version ID of the delete marker created as a result of the DELETE
     * operation. If you delete a specific object version, the value returned by this
     * header is the version ID of the object version deleted.</p>
     */
    inline DeletedObject& WithDeleteMarkerVersionId(const Aws::String& value) { SetDeleteMarkerVersionId(value); return *this;}

    /**
     * <p>The version ID of the delete marker created as a result of the DELETE
     * operation. If you delete a specific object version, the value returned by this
     * header is the version ID of the object version deleted.</p>
     */
    inline DeletedObject& WithDeleteMarkerVersionId(Aws::String&& value) { SetDeleteMarkerVersionId(std::move(value)); return *this;}

    /**
     * <p>The version ID of the delete marker created as a result of the DELETE
     * operation. If you delete a specific object version, the value returned by this
     * header is the version ID of the object version deleted.</p>
     */
    inline DeletedObject& WithDeleteMarkerVersionId(const char* value) { SetDeleteMarkerVersionId(value); return *this;}

  private:

    Aws::String m_key;
    bool m_keyHasBeenSet;

    Aws::String m_versionId;
    bool m_versionIdHasBeenSet;

    bool m_deleteMarker;
    bool m_deleteMarkerHasBeenSet;

    Aws::String m_deleteMarkerVersionId;
    bool m_deleteMarkerVersionIdHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
