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
#pragma once
#include <aws/s3/S3_EXPORTS.h>

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

  class AWS_S3_API RestoreRequest
  {
  public:
    RestoreRequest();
    RestoreRequest(const Aws::Utils::Xml::XmlNode& xmlNode);
    RestoreRequest& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;

    /**
     * Lifetime of the active copy in days
     */
    inline int GetDays() const{ return m_days; }

    /**
     * Lifetime of the active copy in days
     */
    inline void SetDays(int value) { m_daysHasBeenSet = true; m_days = value; }

    /**
     * Lifetime of the active copy in days
     */
    inline RestoreRequest& WithDays(int value) { SetDays(value); return *this;}

  private:
    int m_days;
    bool m_daysHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
