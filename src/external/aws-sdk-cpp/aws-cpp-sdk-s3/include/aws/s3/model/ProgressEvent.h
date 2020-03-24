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
#include <aws/s3/model/Progress.h>
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
   * <p>This data type contains information about the progress event of an
   * operation.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/ProgressEvent">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API ProgressEvent
  {
  public:
    ProgressEvent();
    ProgressEvent(const Aws::Utils::Xml::XmlNode& xmlNode);
    ProgressEvent& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>The Progress event details.</p>
     */
    inline const Progress& GetDetails() const{ return m_details; }

    /**
     * <p>The Progress event details.</p>
     */
    inline bool DetailsHasBeenSet() const { return m_detailsHasBeenSet; }

    /**
     * <p>The Progress event details.</p>
     */
    inline void SetDetails(const Progress& value) { m_detailsHasBeenSet = true; m_details = value; }

    /**
     * <p>The Progress event details.</p>
     */
    inline void SetDetails(Progress&& value) { m_detailsHasBeenSet = true; m_details = std::move(value); }

    /**
     * <p>The Progress event details.</p>
     */
    inline ProgressEvent& WithDetails(const Progress& value) { SetDetails(value); return *this;}

    /**
     * <p>The Progress event details.</p>
     */
    inline ProgressEvent& WithDetails(Progress&& value) { SetDetails(std::move(value)); return *this;}

  private:

    Progress m_details;
    bool m_detailsHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
