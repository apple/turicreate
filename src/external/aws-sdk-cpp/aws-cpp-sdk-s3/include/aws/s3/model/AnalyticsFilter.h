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
#include <aws/s3/model/AnalyticsAndOperator.h>
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
   * <p>The filter used to describe a set of objects for analyses. A filter must have
   * exactly one prefix, one tag, or one conjunction (AnalyticsAndOperator). If no
   * filter is provided, all objects will be considered in any
   * analysis.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/AnalyticsFilter">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API AnalyticsFilter
  {
  public:
    AnalyticsFilter();
    AnalyticsFilter(const Aws::Utils::Xml::XmlNode& xmlNode);
    AnalyticsFilter& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>The prefix to use when evaluating an analytics filter.</p>
     */
    inline const Aws::String& GetPrefix() const{ return m_prefix; }

    /**
     * <p>The prefix to use when evaluating an analytics filter.</p>
     */
    inline bool PrefixHasBeenSet() const { return m_prefixHasBeenSet; }

    /**
     * <p>The prefix to use when evaluating an analytics filter.</p>
     */
    inline void SetPrefix(const Aws::String& value) { m_prefixHasBeenSet = true; m_prefix = value; }

    /**
     * <p>The prefix to use when evaluating an analytics filter.</p>
     */
    inline void SetPrefix(Aws::String&& value) { m_prefixHasBeenSet = true; m_prefix = std::move(value); }

    /**
     * <p>The prefix to use when evaluating an analytics filter.</p>
     */
    inline void SetPrefix(const char* value) { m_prefixHasBeenSet = true; m_prefix.assign(value); }

    /**
     * <p>The prefix to use when evaluating an analytics filter.</p>
     */
    inline AnalyticsFilter& WithPrefix(const Aws::String& value) { SetPrefix(value); return *this;}

    /**
     * <p>The prefix to use when evaluating an analytics filter.</p>
     */
    inline AnalyticsFilter& WithPrefix(Aws::String&& value) { SetPrefix(std::move(value)); return *this;}

    /**
     * <p>The prefix to use when evaluating an analytics filter.</p>
     */
    inline AnalyticsFilter& WithPrefix(const char* value) { SetPrefix(value); return *this;}


    /**
     * <p>The tag to use when evaluating an analytics filter.</p>
     */
    inline const Tag& GetTag() const{ return m_tag; }

    /**
     * <p>The tag to use when evaluating an analytics filter.</p>
     */
    inline bool TagHasBeenSet() const { return m_tagHasBeenSet; }

    /**
     * <p>The tag to use when evaluating an analytics filter.</p>
     */
    inline void SetTag(const Tag& value) { m_tagHasBeenSet = true; m_tag = value; }

    /**
     * <p>The tag to use when evaluating an analytics filter.</p>
     */
    inline void SetTag(Tag&& value) { m_tagHasBeenSet = true; m_tag = std::move(value); }

    /**
     * <p>The tag to use when evaluating an analytics filter.</p>
     */
    inline AnalyticsFilter& WithTag(const Tag& value) { SetTag(value); return *this;}

    /**
     * <p>The tag to use when evaluating an analytics filter.</p>
     */
    inline AnalyticsFilter& WithTag(Tag&& value) { SetTag(std::move(value)); return *this;}


    /**
     * <p>A conjunction (logical AND) of predicates, which is used in evaluating an
     * analytics filter. The operator must have at least two predicates.</p>
     */
    inline const AnalyticsAndOperator& GetAnd() const{ return m_and; }

    /**
     * <p>A conjunction (logical AND) of predicates, which is used in evaluating an
     * analytics filter. The operator must have at least two predicates.</p>
     */
    inline bool AndHasBeenSet() const { return m_andHasBeenSet; }

    /**
     * <p>A conjunction (logical AND) of predicates, which is used in evaluating an
     * analytics filter. The operator must have at least two predicates.</p>
     */
    inline void SetAnd(const AnalyticsAndOperator& value) { m_andHasBeenSet = true; m_and = value; }

    /**
     * <p>A conjunction (logical AND) of predicates, which is used in evaluating an
     * analytics filter. The operator must have at least two predicates.</p>
     */
    inline void SetAnd(AnalyticsAndOperator&& value) { m_andHasBeenSet = true; m_and = std::move(value); }

    /**
     * <p>A conjunction (logical AND) of predicates, which is used in evaluating an
     * analytics filter. The operator must have at least two predicates.</p>
     */
    inline AnalyticsFilter& WithAnd(const AnalyticsAndOperator& value) { SetAnd(value); return *this;}

    /**
     * <p>A conjunction (logical AND) of predicates, which is used in evaluating an
     * analytics filter. The operator must have at least two predicates.</p>
     */
    inline AnalyticsFilter& WithAnd(AnalyticsAndOperator&& value) { SetAnd(std::move(value)); return *this;}

  private:

    Aws::String m_prefix;
    bool m_prefixHasBeenSet;

    Tag m_tag;
    bool m_tagHasBeenSet;

    AnalyticsAndOperator m_and;
    bool m_andHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
