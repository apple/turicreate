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
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/s3/model/ReplicationRuleStatus.h>
#include <aws/s3/model/Destination.h>

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

  class AWS_S3_API ReplicationRule
  {
  public:
    ReplicationRule();
    ReplicationRule(const Aws::Utils::Xml::XmlNode& xmlNode);
    ReplicationRule& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;

    /**
     * Unique identifier for the rule. The value cannot be longer than 255 characters.
     */
    inline const Aws::String& GetID() const{ return m_iD; }

    /**
     * Unique identifier for the rule. The value cannot be longer than 255 characters.
     */
    inline void SetID(const Aws::String& value) { m_iDHasBeenSet = true; m_iD = value; }

    /**
     * Unique identifier for the rule. The value cannot be longer than 255 characters.
     */
    inline void SetID(Aws::String&& value) { m_iDHasBeenSet = true; m_iD = value; }

    /**
     * Unique identifier for the rule. The value cannot be longer than 255 characters.
     */
    inline void SetID(const char* value) { m_iDHasBeenSet = true; m_iD.assign(value); }

    /**
     * Unique identifier for the rule. The value cannot be longer than 255 characters.
     */
    inline ReplicationRule& WithID(const Aws::String& value) { SetID(value); return *this;}

    /**
     * Unique identifier for the rule. The value cannot be longer than 255 characters.
     */
    inline ReplicationRule& WithID(Aws::String&& value) { SetID(value); return *this;}

    /**
     * Unique identifier for the rule. The value cannot be longer than 255 characters.
     */
    inline ReplicationRule& WithID(const char* value) { SetID(value); return *this;}

    /**
     * Object keyname prefix identifying one or more objects to which the rule applies.
     * Maximum prefix length can be up to 1,024 characters. Overlapping prefixes are
     * not supported.
     */
    inline const Aws::String& GetPrefix() const{ return m_prefix; }

    /**
     * Object keyname prefix identifying one or more objects to which the rule applies.
     * Maximum prefix length can be up to 1,024 characters. Overlapping prefixes are
     * not supported.
     */
    inline void SetPrefix(const Aws::String& value) { m_prefixHasBeenSet = true; m_prefix = value; }

    /**
     * Object keyname prefix identifying one or more objects to which the rule applies.
     * Maximum prefix length can be up to 1,024 characters. Overlapping prefixes are
     * not supported.
     */
    inline void SetPrefix(Aws::String&& value) { m_prefixHasBeenSet = true; m_prefix = value; }

    /**
     * Object keyname prefix identifying one or more objects to which the rule applies.
     * Maximum prefix length can be up to 1,024 characters. Overlapping prefixes are
     * not supported.
     */
    inline void SetPrefix(const char* value) { m_prefixHasBeenSet = true; m_prefix.assign(value); }

    /**
     * Object keyname prefix identifying one or more objects to which the rule applies.
     * Maximum prefix length can be up to 1,024 characters. Overlapping prefixes are
     * not supported.
     */
    inline ReplicationRule& WithPrefix(const Aws::String& value) { SetPrefix(value); return *this;}

    /**
     * Object keyname prefix identifying one or more objects to which the rule applies.
     * Maximum prefix length can be up to 1,024 characters. Overlapping prefixes are
     * not supported.
     */
    inline ReplicationRule& WithPrefix(Aws::String&& value) { SetPrefix(value); return *this;}

    /**
     * Object keyname prefix identifying one or more objects to which the rule applies.
     * Maximum prefix length can be up to 1,024 characters. Overlapping prefixes are
     * not supported.
     */
    inline ReplicationRule& WithPrefix(const char* value) { SetPrefix(value); return *this;}

    /**
     * The rule is ignored if status is not Enabled.
     */
    inline const ReplicationRuleStatus& GetStatus() const{ return m_status; }

    /**
     * The rule is ignored if status is not Enabled.
     */
    inline void SetStatus(const ReplicationRuleStatus& value) { m_statusHasBeenSet = true; m_status = value; }

    /**
     * The rule is ignored if status is not Enabled.
     */
    inline void SetStatus(ReplicationRuleStatus&& value) { m_statusHasBeenSet = true; m_status = value; }

    /**
     * The rule is ignored if status is not Enabled.
     */
    inline ReplicationRule& WithStatus(const ReplicationRuleStatus& value) { SetStatus(value); return *this;}

    /**
     * The rule is ignored if status is not Enabled.
     */
    inline ReplicationRule& WithStatus(ReplicationRuleStatus&& value) { SetStatus(value); return *this;}

    
    inline const Destination& GetDestination() const{ return m_destination; }

    
    inline void SetDestination(const Destination& value) { m_destinationHasBeenSet = true; m_destination = value; }

    
    inline void SetDestination(Destination&& value) { m_destinationHasBeenSet = true; m_destination = value; }

    
    inline ReplicationRule& WithDestination(const Destination& value) { SetDestination(value); return *this;}

    
    inline ReplicationRule& WithDestination(Destination&& value) { SetDestination(value); return *this;}

  private:
    Aws::String m_iD;
    bool m_iDHasBeenSet;
    Aws::String m_prefix;
    bool m_prefixHasBeenSet;
    ReplicationRuleStatus m_status;
    bool m_statusHasBeenSet;
    Destination m_destination;
    bool m_destinationHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
