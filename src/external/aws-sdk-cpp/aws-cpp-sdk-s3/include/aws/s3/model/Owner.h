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
   * <p>Container for the owner's display name and ID.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/Owner">AWS API
   * Reference</a></p>
   */
  class AWS_S3_API Owner
  {
  public:
    Owner();
    Owner(const Aws::Utils::Xml::XmlNode& xmlNode);
    Owner& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Container for the display name of the owner.</p>
     */
    inline const Aws::String& GetDisplayName() const{ return m_displayName; }

    /**
     * <p>Container for the display name of the owner.</p>
     */
    inline bool DisplayNameHasBeenSet() const { return m_displayNameHasBeenSet; }

    /**
     * <p>Container for the display name of the owner.</p>
     */
    inline void SetDisplayName(const Aws::String& value) { m_displayNameHasBeenSet = true; m_displayName = value; }

    /**
     * <p>Container for the display name of the owner.</p>
     */
    inline void SetDisplayName(Aws::String&& value) { m_displayNameHasBeenSet = true; m_displayName = std::move(value); }

    /**
     * <p>Container for the display name of the owner.</p>
     */
    inline void SetDisplayName(const char* value) { m_displayNameHasBeenSet = true; m_displayName.assign(value); }

    /**
     * <p>Container for the display name of the owner.</p>
     */
    inline Owner& WithDisplayName(const Aws::String& value) { SetDisplayName(value); return *this;}

    /**
     * <p>Container for the display name of the owner.</p>
     */
    inline Owner& WithDisplayName(Aws::String&& value) { SetDisplayName(std::move(value)); return *this;}

    /**
     * <p>Container for the display name of the owner.</p>
     */
    inline Owner& WithDisplayName(const char* value) { SetDisplayName(value); return *this;}


    /**
     * <p>Container for the ID of the owner.</p>
     */
    inline const Aws::String& GetID() const{ return m_iD; }

    /**
     * <p>Container for the ID of the owner.</p>
     */
    inline bool IDHasBeenSet() const { return m_iDHasBeenSet; }

    /**
     * <p>Container for the ID of the owner.</p>
     */
    inline void SetID(const Aws::String& value) { m_iDHasBeenSet = true; m_iD = value; }

    /**
     * <p>Container for the ID of the owner.</p>
     */
    inline void SetID(Aws::String&& value) { m_iDHasBeenSet = true; m_iD = std::move(value); }

    /**
     * <p>Container for the ID of the owner.</p>
     */
    inline void SetID(const char* value) { m_iDHasBeenSet = true; m_iD.assign(value); }

    /**
     * <p>Container for the ID of the owner.</p>
     */
    inline Owner& WithID(const Aws::String& value) { SetID(value); return *this;}

    /**
     * <p>Container for the ID of the owner.</p>
     */
    inline Owner& WithID(Aws::String&& value) { SetID(std::move(value)); return *this;}

    /**
     * <p>Container for the ID of the owner.</p>
     */
    inline Owner& WithID(const char* value) { SetID(value); return *this;}

  private:

    Aws::String m_displayName;
    bool m_displayNameHasBeenSet;

    Aws::String m_iD;
    bool m_iDHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
