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
   * <p>Container for all (if there are any) keys between Prefix and the next
   * occurrence of the string specified by a delimiter. CommonPrefixes lists keys
   * that act like subdirectories in the directory specified by Prefix. For example,
   * if the prefix is notes/ and the delimiter is a slash (/) as in
   * notes/summer/july, the common prefix is notes/summer/. </p><p><h3>See Also:</h3>
   * <a href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/CommonPrefix">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API CommonPrefix
  {
  public:
    CommonPrefix();
    CommonPrefix(const Aws::Utils::Xml::XmlNode& xmlNode);
    CommonPrefix& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Container for the specified common prefix.</p>
     */
    inline const Aws::String& GetPrefix() const{ return m_prefix; }

    /**
     * <p>Container for the specified common prefix.</p>
     */
    inline bool PrefixHasBeenSet() const { return m_prefixHasBeenSet; }

    /**
     * <p>Container for the specified common prefix.</p>
     */
    inline void SetPrefix(const Aws::String& value) { m_prefixHasBeenSet = true; m_prefix = value; }

    /**
     * <p>Container for the specified common prefix.</p>
     */
    inline void SetPrefix(Aws::String&& value) { m_prefixHasBeenSet = true; m_prefix = std::move(value); }

    /**
     * <p>Container for the specified common prefix.</p>
     */
    inline void SetPrefix(const char* value) { m_prefixHasBeenSet = true; m_prefix.assign(value); }

    /**
     * <p>Container for the specified common prefix.</p>
     */
    inline CommonPrefix& WithPrefix(const Aws::String& value) { SetPrefix(value); return *this;}

    /**
     * <p>Container for the specified common prefix.</p>
     */
    inline CommonPrefix& WithPrefix(Aws::String&& value) { SetPrefix(std::move(value)); return *this;}

    /**
     * <p>Container for the specified common prefix.</p>
     */
    inline CommonPrefix& WithPrefix(const char* value) { SetPrefix(value); return *this;}

  private:

    Aws::String m_prefix;
    bool m_prefixHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
