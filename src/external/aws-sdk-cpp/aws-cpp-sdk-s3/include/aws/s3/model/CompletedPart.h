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
   * <p>Details of the parts that were uploaded.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/CompletedPart">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API CompletedPart
  {
  public:
    CompletedPart();
    CompletedPart(const Aws::Utils::Xml::XmlNode& xmlNode);
    CompletedPart& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Entity tag returned when the part was uploaded.</p>
     */
    inline const Aws::String& GetETag() const{ return m_eTag; }

    /**
     * <p>Entity tag returned when the part was uploaded.</p>
     */
    inline bool ETagHasBeenSet() const { return m_eTagHasBeenSet; }

    /**
     * <p>Entity tag returned when the part was uploaded.</p>
     */
    inline void SetETag(const Aws::String& value) { m_eTagHasBeenSet = true; m_eTag = value; }

    /**
     * <p>Entity tag returned when the part was uploaded.</p>
     */
    inline void SetETag(Aws::String&& value) { m_eTagHasBeenSet = true; m_eTag = std::move(value); }

    /**
     * <p>Entity tag returned when the part was uploaded.</p>
     */
    inline void SetETag(const char* value) { m_eTagHasBeenSet = true; m_eTag.assign(value); }

    /**
     * <p>Entity tag returned when the part was uploaded.</p>
     */
    inline CompletedPart& WithETag(const Aws::String& value) { SetETag(value); return *this;}

    /**
     * <p>Entity tag returned when the part was uploaded.</p>
     */
    inline CompletedPart& WithETag(Aws::String&& value) { SetETag(std::move(value)); return *this;}

    /**
     * <p>Entity tag returned when the part was uploaded.</p>
     */
    inline CompletedPart& WithETag(const char* value) { SetETag(value); return *this;}


    /**
     * <p>Part number that identifies the part. This is a positive integer between 1
     * and 10,000.</p>
     */
    inline int GetPartNumber() const{ return m_partNumber; }

    /**
     * <p>Part number that identifies the part. This is a positive integer between 1
     * and 10,000.</p>
     */
    inline bool PartNumberHasBeenSet() const { return m_partNumberHasBeenSet; }

    /**
     * <p>Part number that identifies the part. This is a positive integer between 1
     * and 10,000.</p>
     */
    inline void SetPartNumber(int value) { m_partNumberHasBeenSet = true; m_partNumber = value; }

    /**
     * <p>Part number that identifies the part. This is a positive integer between 1
     * and 10,000.</p>
     */
    inline CompletedPart& WithPartNumber(int value) { SetPartNumber(value); return *this;}

  private:

    Aws::String m_eTag;
    bool m_eTagHasBeenSet;

    int m_partNumber;
    bool m_partNumberHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
