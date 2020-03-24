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
#include <aws/s3/model/TransitionStorageClass.h>
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
   * <p>Container for the transition rule that describes when noncurrent objects
   * transition to the <code>STANDARD_IA</code>, <code>ONEZONE_IA</code>,
   * <code>INTELLIGENT_TIERING</code>, <code>GLACIER</code>, or
   * <code>DEEP_ARCHIVE</code> storage class. If your bucket is versioning-enabled
   * (or versioning is suspended), you can set this action to request that Amazon S3
   * transition noncurrent object versions to the <code>STANDARD_IA</code>,
   * <code>ONEZONE_IA</code>, <code>INTELLIGENT_TIERING</code>, <code>GLACIER</code>,
   * or <code>DEEP_ARCHIVE</code> storage class at a specific period in the object's
   * lifetime.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/NoncurrentVersionTransition">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API NoncurrentVersionTransition
  {
  public:
    NoncurrentVersionTransition();
    NoncurrentVersionTransition(const Aws::Utils::Xml::XmlNode& xmlNode);
    NoncurrentVersionTransition& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Specifies the number of days an object is noncurrent before Amazon S3 can
     * perform the associated action. For information about the noncurrent days
     * calculations, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/intro-lifecycle-rules.html#non-current-days-calculations">How
     * Amazon S3 Calculates How Long an Object Has Been Noncurrent</a> in the <i>Amazon
     * Simple Storage Service Developer Guide</i>.</p>
     */
    inline int GetNoncurrentDays() const{ return m_noncurrentDays; }

    /**
     * <p>Specifies the number of days an object is noncurrent before Amazon S3 can
     * perform the associated action. For information about the noncurrent days
     * calculations, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/intro-lifecycle-rules.html#non-current-days-calculations">How
     * Amazon S3 Calculates How Long an Object Has Been Noncurrent</a> in the <i>Amazon
     * Simple Storage Service Developer Guide</i>.</p>
     */
    inline bool NoncurrentDaysHasBeenSet() const { return m_noncurrentDaysHasBeenSet; }

    /**
     * <p>Specifies the number of days an object is noncurrent before Amazon S3 can
     * perform the associated action. For information about the noncurrent days
     * calculations, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/intro-lifecycle-rules.html#non-current-days-calculations">How
     * Amazon S3 Calculates How Long an Object Has Been Noncurrent</a> in the <i>Amazon
     * Simple Storage Service Developer Guide</i>.</p>
     */
    inline void SetNoncurrentDays(int value) { m_noncurrentDaysHasBeenSet = true; m_noncurrentDays = value; }

    /**
     * <p>Specifies the number of days an object is noncurrent before Amazon S3 can
     * perform the associated action. For information about the noncurrent days
     * calculations, see <a
     * href="https://docs.aws.amazon.com/AmazonS3/latest/dev/intro-lifecycle-rules.html#non-current-days-calculations">How
     * Amazon S3 Calculates How Long an Object Has Been Noncurrent</a> in the <i>Amazon
     * Simple Storage Service Developer Guide</i>.</p>
     */
    inline NoncurrentVersionTransition& WithNoncurrentDays(int value) { SetNoncurrentDays(value); return *this;}


    /**
     * <p>The class of storage used to store the object.</p>
     */
    inline const TransitionStorageClass& GetStorageClass() const{ return m_storageClass; }

    /**
     * <p>The class of storage used to store the object.</p>
     */
    inline bool StorageClassHasBeenSet() const { return m_storageClassHasBeenSet; }

    /**
     * <p>The class of storage used to store the object.</p>
     */
    inline void SetStorageClass(const TransitionStorageClass& value) { m_storageClassHasBeenSet = true; m_storageClass = value; }

    /**
     * <p>The class of storage used to store the object.</p>
     */
    inline void SetStorageClass(TransitionStorageClass&& value) { m_storageClassHasBeenSet = true; m_storageClass = std::move(value); }

    /**
     * <p>The class of storage used to store the object.</p>
     */
    inline NoncurrentVersionTransition& WithStorageClass(const TransitionStorageClass& value) { SetStorageClass(value); return *this;}

    /**
     * <p>The class of storage used to store the object.</p>
     */
    inline NoncurrentVersionTransition& WithStorageClass(TransitionStorageClass&& value) { SetStorageClass(std::move(value)); return *this;}

  private:

    int m_noncurrentDays;
    bool m_noncurrentDaysHasBeenSet;

    TransitionStorageClass m_storageClass;
    bool m_storageClassHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
