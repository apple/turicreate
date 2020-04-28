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
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/s3/model/Tag.h>
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
   * <p>A container for specifying rule filters. The filters determine the subset of
   * objects to which the rule applies. This element is required only if you specify
   * more than one filter. </p> <p>For example:</p> <ul> <li> <p>If you specify both
   * a <code>Prefix</code> and a <code>Tag</code> filter, wrap these filters in an
   * <code>And</code> tag. </p> </li> <li> <p>If you specify a filter based on
   * multiple tags, wrap the <code>Tag</code> elements in an <code>And</code> tag</p>
   * </li> </ul><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/ReplicationRuleAndOperator">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API ReplicationRuleAndOperator
  {
  public:
    ReplicationRuleAndOperator();
    ReplicationRuleAndOperator(const Aws::Utils::Xml::XmlNode& xmlNode);
    ReplicationRuleAndOperator& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>An object key name prefix that identifies the subset of objects to which the
     * rule applies.</p>
     */
    inline const Aws::String& GetPrefix() const{ return m_prefix; }

    /**
     * <p>An object key name prefix that identifies the subset of objects to which the
     * rule applies.</p>
     */
    inline bool PrefixHasBeenSet() const { return m_prefixHasBeenSet; }

    /**
     * <p>An object key name prefix that identifies the subset of objects to which the
     * rule applies.</p>
     */
    inline void SetPrefix(const Aws::String& value) { m_prefixHasBeenSet = true; m_prefix = value; }

    /**
     * <p>An object key name prefix that identifies the subset of objects to which the
     * rule applies.</p>
     */
    inline void SetPrefix(Aws::String&& value) { m_prefixHasBeenSet = true; m_prefix = std::move(value); }

    /**
     * <p>An object key name prefix that identifies the subset of objects to which the
     * rule applies.</p>
     */
    inline void SetPrefix(const char* value) { m_prefixHasBeenSet = true; m_prefix.assign(value); }

    /**
     * <p>An object key name prefix that identifies the subset of objects to which the
     * rule applies.</p>
     */
    inline ReplicationRuleAndOperator& WithPrefix(const Aws::String& value) { SetPrefix(value); return *this;}

    /**
     * <p>An object key name prefix that identifies the subset of objects to which the
     * rule applies.</p>
     */
    inline ReplicationRuleAndOperator& WithPrefix(Aws::String&& value) { SetPrefix(std::move(value)); return *this;}

    /**
     * <p>An object key name prefix that identifies the subset of objects to which the
     * rule applies.</p>
     */
    inline ReplicationRuleAndOperator& WithPrefix(const char* value) { SetPrefix(value); return *this;}


    /**
     * <p>An array of tags containing key and value pairs.</p>
     */
    inline const Aws::Vector<Tag>& GetTags() const{ return m_tags; }

    /**
     * <p>An array of tags containing key and value pairs.</p>
     */
    inline bool TagsHasBeenSet() const { return m_tagsHasBeenSet; }

    /**
     * <p>An array of tags containing key and value pairs.</p>
     */
    inline void SetTags(const Aws::Vector<Tag>& value) { m_tagsHasBeenSet = true; m_tags = value; }

    /**
     * <p>An array of tags containing key and value pairs.</p>
     */
    inline void SetTags(Aws::Vector<Tag>&& value) { m_tagsHasBeenSet = true; m_tags = std::move(value); }

    /**
     * <p>An array of tags containing key and value pairs.</p>
     */
    inline ReplicationRuleAndOperator& WithTags(const Aws::Vector<Tag>& value) { SetTags(value); return *this;}

    /**
     * <p>An array of tags containing key and value pairs.</p>
     */
    inline ReplicationRuleAndOperator& WithTags(Aws::Vector<Tag>&& value) { SetTags(std::move(value)); return *this;}

    /**
     * <p>An array of tags containing key and value pairs.</p>
     */
    inline ReplicationRuleAndOperator& AddTags(const Tag& value) { m_tagsHasBeenSet = true; m_tags.push_back(value); return *this; }

    /**
     * <p>An array of tags containing key and value pairs.</p>
     */
    inline ReplicationRuleAndOperator& AddTags(Tag&& value) { m_tagsHasBeenSet = true; m_tags.push_back(std::move(value)); return *this; }

  private:

    Aws::String m_prefix;
    bool m_prefixHasBeenSet;

    Aws::Vector<Tag> m_tags;
    bool m_tagsHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
