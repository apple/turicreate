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
#include <aws/s3/model/S3Location.h>
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
   * <p>Describes the location where the restore job's output is
   * stored.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/OutputLocation">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API OutputLocation
  {
  public:
    OutputLocation();
    OutputLocation(const Aws::Utils::Xml::XmlNode& xmlNode);
    OutputLocation& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Describes an S3 location that will receive the results of the restore
     * request.</p>
     */
    inline const S3Location& GetS3() const{ return m_s3; }

    /**
     * <p>Describes an S3 location that will receive the results of the restore
     * request.</p>
     */
    inline bool S3HasBeenSet() const { return m_s3HasBeenSet; }

    /**
     * <p>Describes an S3 location that will receive the results of the restore
     * request.</p>
     */
    inline void SetS3(const S3Location& value) { m_s3HasBeenSet = true; m_s3 = value; }

    /**
     * <p>Describes an S3 location that will receive the results of the restore
     * request.</p>
     */
    inline void SetS3(S3Location&& value) { m_s3HasBeenSet = true; m_s3 = std::move(value); }

    /**
     * <p>Describes an S3 location that will receive the results of the restore
     * request.</p>
     */
    inline OutputLocation& WithS3(const S3Location& value) { SetS3(value); return *this;}

    /**
     * <p>Describes an S3 location that will receive the results of the restore
     * request.</p>
     */
    inline OutputLocation& WithS3(S3Location&& value) { SetS3(std::move(value)); return *this;}

  private:

    S3Location m_s3;
    bool m_s3HasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
