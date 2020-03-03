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
#include <aws/s3/model/LifecycleRuleAndOperator.h>
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
   * <p>The <code>Filter</code> is used to identify objects that a Lifecycle Rule
   * applies to. A <code>Filter</code> must have exactly one of <code>Prefix</code>,
   * <code>Tag</code>, or <code>And</code> specified.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/LifecycleRuleFilter">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API LifecycleRuleFilter
  {
  public:
    LifecycleRuleFilter();
    LifecycleRuleFilter(const Aws::Utils::Xml::XmlNode& xmlNode);
    LifecycleRuleFilter& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Prefix identifying one or more objects to which the rule applies.</p>
     */
    inline const Aws::String& GetPrefix() const{ return m_prefix; }

    /**
     * <p>Prefix identifying one or more objects to which the rule applies.</p>
     */
    inline bool PrefixHasBeenSet() const { return m_prefixHasBeenSet; }

    /**
     * <p>Prefix identifying one or more objects to which the rule applies.</p>
     */
    inline void SetPrefix(const Aws::String& value) { m_prefixHasBeenSet = true; m_prefix = value; }

    /**
     * <p>Prefix identifying one or more objects to which the rule applies.</p>
     */
    inline void SetPrefix(Aws::String&& value) { m_prefixHasBeenSet = true; m_prefix = std::move(value); }

    /**
     * <p>Prefix identifying one or more objects to which the rule applies.</p>
     */
    inline void SetPrefix(const char* value) { m_prefixHasBeenSet = true; m_prefix.assign(value); }

    /**
     * <p>Prefix identifying one or more objects to which the rule applies.</p>
     */
    inline LifecycleRuleFilter& WithPrefix(const Aws::String& value) { SetPrefix(value); return *this;}

    /**
     * <p>Prefix identifying one or more objects to which the rule applies.</p>
     */
    inline LifecycleRuleFilter& WithPrefix(Aws::String&& value) { SetPrefix(std::move(value)); return *this;}

    /**
     * <p>Prefix identifying one or more objects to which the rule applies.</p>
     */
    inline LifecycleRuleFilter& WithPrefix(const char* value) { SetPrefix(value); return *this;}


    /**
     * <p>This tag must exist in the object's tag set in order for the rule to
     * apply.</p>
     */
    inline const Tag& GetTag() const{ return m_tag; }

    /**
     * <p>This tag must exist in the object's tag set in order for the rule to
     * apply.</p>
     */
    inline bool TagHasBeenSet() const { return m_tagHasBeenSet; }

    /**
     * <p>This tag must exist in the object's tag set in order for the rule to
     * apply.</p>
     */
    inline void SetTag(const Tag& value) { m_tagHasBeenSet = true; m_tag = value; }

    /**
     * <p>This tag must exist in the object's tag set in order for the rule to
     * apply.</p>
     */
    inline void SetTag(Tag&& value) { m_tagHasBeenSet = true; m_tag = std::move(value); }

    /**
     * <p>This tag must exist in the object's tag set in order for the rule to
     * apply.</p>
     */
    inline LifecycleRuleFilter& WithTag(const Tag& value) { SetTag(value); return *this;}

    /**
     * <p>This tag must exist in the object's tag set in order for the rule to
     * apply.</p>
     */
    inline LifecycleRuleFilter& WithTag(Tag&& value) { SetTag(std::move(value)); return *this;}


    
    inline const LifecycleRuleAndOperator& GetAnd() const{ return m_and; }

    
    inline bool AndHasBeenSet() const { return m_andHasBeenSet; }

    
    inline void SetAnd(const LifecycleRuleAndOperator& value) { m_andHasBeenSet = true; m_and = value; }

    
    inline void SetAnd(LifecycleRuleAndOperator&& value) { m_andHasBeenSet = true; m_and = std::move(value); }

    
    inline LifecycleRuleFilter& WithAnd(const LifecycleRuleAndOperator& value) { SetAnd(value); return *this;}

    
    inline LifecycleRuleFilter& WithAnd(LifecycleRuleAndOperator&& value) { SetAnd(std::move(value)); return *this;}

  private:

    Aws::String m_prefix;
    bool m_prefixHasBeenSet;

    Tag m_tag;
    bool m_tagHasBeenSet;

    LifecycleRuleAndOperator m_and;
    bool m_andHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
