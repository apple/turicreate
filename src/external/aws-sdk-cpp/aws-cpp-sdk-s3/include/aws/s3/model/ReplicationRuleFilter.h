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
#include <aws/s3/model/Tag.h>
#include <aws/s3/model/ReplicationRuleAndOperator.h>
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
   * <p>A filter that identifies the subset of objects to which the replication rule
   * applies. A <code>Filter</code> must specify exactly one <code>Prefix</code>,
   * <code>Tag</code>, or an <code>And</code> child element.</p><p><h3>See Also:</h3>
   * <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/ReplicationRuleFilter">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API ReplicationRuleFilter
  {
  public:
    ReplicationRuleFilter();
    ReplicationRuleFilter(const Aws::Utils::Xml::XmlNode& xmlNode);
    ReplicationRuleFilter& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

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
    inline ReplicationRuleFilter& WithPrefix(const Aws::String& value) { SetPrefix(value); return *this;}

    /**
     * <p>An object key name prefix that identifies the subset of objects to which the
     * rule applies.</p>
     */
    inline ReplicationRuleFilter& WithPrefix(Aws::String&& value) { SetPrefix(std::move(value)); return *this;}

    /**
     * <p>An object key name prefix that identifies the subset of objects to which the
     * rule applies.</p>
     */
    inline ReplicationRuleFilter& WithPrefix(const char* value) { SetPrefix(value); return *this;}


    /**
     * <p>A container for specifying a tag key and value. </p> <p>The rule applies only
     * to objects that have the tag in their tag set.</p>
     */
    inline const Tag& GetTag() const{ return m_tag; }

    /**
     * <p>A container for specifying a tag key and value. </p> <p>The rule applies only
     * to objects that have the tag in their tag set.</p>
     */
    inline bool TagHasBeenSet() const { return m_tagHasBeenSet; }

    /**
     * <p>A container for specifying a tag key and value. </p> <p>The rule applies only
     * to objects that have the tag in their tag set.</p>
     */
    inline void SetTag(const Tag& value) { m_tagHasBeenSet = true; m_tag = value; }

    /**
     * <p>A container for specifying a tag key and value. </p> <p>The rule applies only
     * to objects that have the tag in their tag set.</p>
     */
    inline void SetTag(Tag&& value) { m_tagHasBeenSet = true; m_tag = std::move(value); }

    /**
     * <p>A container for specifying a tag key and value. </p> <p>The rule applies only
     * to objects that have the tag in their tag set.</p>
     */
    inline ReplicationRuleFilter& WithTag(const Tag& value) { SetTag(value); return *this;}

    /**
     * <p>A container for specifying a tag key and value. </p> <p>The rule applies only
     * to objects that have the tag in their tag set.</p>
     */
    inline ReplicationRuleFilter& WithTag(Tag&& value) { SetTag(std::move(value)); return *this;}


    /**
     * <p>A container for specifying rule filters. The filters determine the subset of
     * objects to which the rule applies. This element is required only if you specify
     * more than one filter. For example: </p> <ul> <li> <p>If you specify both a
     * <code>Prefix</code> and a <code>Tag</code> filter, wrap these filters in an
     * <code>And</code> tag.</p> </li> <li> <p>If you specify a filter based on
     * multiple tags, wrap the <code>Tag</code> elements in an <code>And</code>
     * tag.</p> </li> </ul>
     */
    inline const ReplicationRuleAndOperator& GetAnd() const{ return m_and; }

    /**
     * <p>A container for specifying rule filters. The filters determine the subset of
     * objects to which the rule applies. This element is required only if you specify
     * more than one filter. For example: </p> <ul> <li> <p>If you specify both a
     * <code>Prefix</code> and a <code>Tag</code> filter, wrap these filters in an
     * <code>And</code> tag.</p> </li> <li> <p>If you specify a filter based on
     * multiple tags, wrap the <code>Tag</code> elements in an <code>And</code>
     * tag.</p> </li> </ul>
     */
    inline bool AndHasBeenSet() const { return m_andHasBeenSet; }

    /**
     * <p>A container for specifying rule filters. The filters determine the subset of
     * objects to which the rule applies. This element is required only if you specify
     * more than one filter. For example: </p> <ul> <li> <p>If you specify both a
     * <code>Prefix</code> and a <code>Tag</code> filter, wrap these filters in an
     * <code>And</code> tag.</p> </li> <li> <p>If you specify a filter based on
     * multiple tags, wrap the <code>Tag</code> elements in an <code>And</code>
     * tag.</p> </li> </ul>
     */
    inline void SetAnd(const ReplicationRuleAndOperator& value) { m_andHasBeenSet = true; m_and = value; }

    /**
     * <p>A container for specifying rule filters. The filters determine the subset of
     * objects to which the rule applies. This element is required only if you specify
     * more than one filter. For example: </p> <ul> <li> <p>If you specify both a
     * <code>Prefix</code> and a <code>Tag</code> filter, wrap these filters in an
     * <code>And</code> tag.</p> </li> <li> <p>If you specify a filter based on
     * multiple tags, wrap the <code>Tag</code> elements in an <code>And</code>
     * tag.</p> </li> </ul>
     */
    inline void SetAnd(ReplicationRuleAndOperator&& value) { m_andHasBeenSet = true; m_and = std::move(value); }

    /**
     * <p>A container for specifying rule filters. The filters determine the subset of
     * objects to which the rule applies. This element is required only if you specify
     * more than one filter. For example: </p> <ul> <li> <p>If you specify both a
     * <code>Prefix</code> and a <code>Tag</code> filter, wrap these filters in an
     * <code>And</code> tag.</p> </li> <li> <p>If you specify a filter based on
     * multiple tags, wrap the <code>Tag</code> elements in an <code>And</code>
     * tag.</p> </li> </ul>
     */
    inline ReplicationRuleFilter& WithAnd(const ReplicationRuleAndOperator& value) { SetAnd(value); return *this;}

    /**
     * <p>A container for specifying rule filters. The filters determine the subset of
     * objects to which the rule applies. This element is required only if you specify
     * more than one filter. For example: </p> <ul> <li> <p>If you specify both a
     * <code>Prefix</code> and a <code>Tag</code> filter, wrap these filters in an
     * <code>And</code> tag.</p> </li> <li> <p>If you specify a filter based on
     * multiple tags, wrap the <code>Tag</code> elements in an <code>And</code>
     * tag.</p> </li> </ul>
     */
    inline ReplicationRuleFilter& WithAnd(ReplicationRuleAndOperator&& value) { SetAnd(std::move(value)); return *this;}

  private:

    Aws::String m_prefix;
    bool m_prefixHasBeenSet;

    Tag m_tag;
    bool m_tagHasBeenSet;

    ReplicationRuleAndOperator m_and;
    bool m_andHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
