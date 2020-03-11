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
#include <aws/s3/model/LifecycleExpiration.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/s3/model/ExpirationStatus.h>
#include <aws/s3/model/Transition.h>
#include <aws/s3/model/NoncurrentVersionTransition.h>
#include <aws/s3/model/NoncurrentVersionExpiration.h>
#include <aws/s3/model/AbortIncompleteMultipartUpload.h>
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
   * <p>Specifies lifecycle rules for an Amazon S3 bucket. For more information, see
   * <a
   * href="https://docs.aws.amazon.com/AmazonS3/latest/API/RESTBucketPUTlifecycle.html">PUT
   * Bucket lifecycle</a> in the <i>Amazon Simple Storage Service API
   * Reference</i>.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/Rule">AWS API
   * Reference</a></p>
   */
  class AWS_S3_API Rule
  {
  public:
    Rule();
    Rule(const Aws::Utils::Xml::XmlNode& xmlNode);
    Rule& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Specifies the expiration for the lifecycle of the object.</p>
     */
    inline const LifecycleExpiration& GetExpiration() const{ return m_expiration; }

    /**
     * <p>Specifies the expiration for the lifecycle of the object.</p>
     */
    inline bool ExpirationHasBeenSet() const { return m_expirationHasBeenSet; }

    /**
     * <p>Specifies the expiration for the lifecycle of the object.</p>
     */
    inline void SetExpiration(const LifecycleExpiration& value) { m_expirationHasBeenSet = true; m_expiration = value; }

    /**
     * <p>Specifies the expiration for the lifecycle of the object.</p>
     */
    inline void SetExpiration(LifecycleExpiration&& value) { m_expirationHasBeenSet = true; m_expiration = std::move(value); }

    /**
     * <p>Specifies the expiration for the lifecycle of the object.</p>
     */
    inline Rule& WithExpiration(const LifecycleExpiration& value) { SetExpiration(value); return *this;}

    /**
     * <p>Specifies the expiration for the lifecycle of the object.</p>
     */
    inline Rule& WithExpiration(LifecycleExpiration&& value) { SetExpiration(std::move(value)); return *this;}


    /**
     * <p>Unique identifier for the rule. The value can't be longer than 255
     * characters.</p>
     */
    inline const Aws::String& GetID() const{ return m_iD; }

    /**
     * <p>Unique identifier for the rule. The value can't be longer than 255
     * characters.</p>
     */
    inline bool IDHasBeenSet() const { return m_iDHasBeenSet; }

    /**
     * <p>Unique identifier for the rule. The value can't be longer than 255
     * characters.</p>
     */
    inline void SetID(const Aws::String& value) { m_iDHasBeenSet = true; m_iD = value; }

    /**
     * <p>Unique identifier for the rule. The value can't be longer than 255
     * characters.</p>
     */
    inline void SetID(Aws::String&& value) { m_iDHasBeenSet = true; m_iD = std::move(value); }

    /**
     * <p>Unique identifier for the rule. The value can't be longer than 255
     * characters.</p>
     */
    inline void SetID(const char* value) { m_iDHasBeenSet = true; m_iD.assign(value); }

    /**
     * <p>Unique identifier for the rule. The value can't be longer than 255
     * characters.</p>
     */
    inline Rule& WithID(const Aws::String& value) { SetID(value); return *this;}

    /**
     * <p>Unique identifier for the rule. The value can't be longer than 255
     * characters.</p>
     */
    inline Rule& WithID(Aws::String&& value) { SetID(std::move(value)); return *this;}

    /**
     * <p>Unique identifier for the rule. The value can't be longer than 255
     * characters.</p>
     */
    inline Rule& WithID(const char* value) { SetID(value); return *this;}


    /**
     * <p>Object key prefix that identifies one or more objects to which this rule
     * applies.</p>
     */
    inline const Aws::String& GetPrefix() const{ return m_prefix; }

    /**
     * <p>Object key prefix that identifies one or more objects to which this rule
     * applies.</p>
     */
    inline bool PrefixHasBeenSet() const { return m_prefixHasBeenSet; }

    /**
     * <p>Object key prefix that identifies one or more objects to which this rule
     * applies.</p>
     */
    inline void SetPrefix(const Aws::String& value) { m_prefixHasBeenSet = true; m_prefix = value; }

    /**
     * <p>Object key prefix that identifies one or more objects to which this rule
     * applies.</p>
     */
    inline void SetPrefix(Aws::String&& value) { m_prefixHasBeenSet = true; m_prefix = std::move(value); }

    /**
     * <p>Object key prefix that identifies one or more objects to which this rule
     * applies.</p>
     */
    inline void SetPrefix(const char* value) { m_prefixHasBeenSet = true; m_prefix.assign(value); }

    /**
     * <p>Object key prefix that identifies one or more objects to which this rule
     * applies.</p>
     */
    inline Rule& WithPrefix(const Aws::String& value) { SetPrefix(value); return *this;}

    /**
     * <p>Object key prefix that identifies one or more objects to which this rule
     * applies.</p>
     */
    inline Rule& WithPrefix(Aws::String&& value) { SetPrefix(std::move(value)); return *this;}

    /**
     * <p>Object key prefix that identifies one or more objects to which this rule
     * applies.</p>
     */
    inline Rule& WithPrefix(const char* value) { SetPrefix(value); return *this;}


    /**
     * <p>If <code>Enabled</code>, the rule is currently being applied. If
     * <code>Disabled</code>, the rule is not currently being applied.</p>
     */
    inline const ExpirationStatus& GetStatus() const{ return m_status; }

    /**
     * <p>If <code>Enabled</code>, the rule is currently being applied. If
     * <code>Disabled</code>, the rule is not currently being applied.</p>
     */
    inline bool StatusHasBeenSet() const { return m_statusHasBeenSet; }

    /**
     * <p>If <code>Enabled</code>, the rule is currently being applied. If
     * <code>Disabled</code>, the rule is not currently being applied.</p>
     */
    inline void SetStatus(const ExpirationStatus& value) { m_statusHasBeenSet = true; m_status = value; }

    /**
     * <p>If <code>Enabled</code>, the rule is currently being applied. If
     * <code>Disabled</code>, the rule is not currently being applied.</p>
     */
    inline void SetStatus(ExpirationStatus&& value) { m_statusHasBeenSet = true; m_status = std::move(value); }

    /**
     * <p>If <code>Enabled</code>, the rule is currently being applied. If
     * <code>Disabled</code>, the rule is not currently being applied.</p>
     */
    inline Rule& WithStatus(const ExpirationStatus& value) { SetStatus(value); return *this;}

    /**
     * <p>If <code>Enabled</code>, the rule is currently being applied. If
     * <code>Disabled</code>, the rule is not currently being applied.</p>
     */
    inline Rule& WithStatus(ExpirationStatus&& value) { SetStatus(std::move(value)); return *this;}


    /**
     * <p>Specifies when an object transitions to a specified storage class.</p>
     */
    inline const Transition& GetTransition() const{ return m_transition; }

    /**
     * <p>Specifies when an object transitions to a specified storage class.</p>
     */
    inline bool TransitionHasBeenSet() const { return m_transitionHasBeenSet; }

    /**
     * <p>Specifies when an object transitions to a specified storage class.</p>
     */
    inline void SetTransition(const Transition& value) { m_transitionHasBeenSet = true; m_transition = value; }

    /**
     * <p>Specifies when an object transitions to a specified storage class.</p>
     */
    inline void SetTransition(Transition&& value) { m_transitionHasBeenSet = true; m_transition = std::move(value); }

    /**
     * <p>Specifies when an object transitions to a specified storage class.</p>
     */
    inline Rule& WithTransition(const Transition& value) { SetTransition(value); return *this;}

    /**
     * <p>Specifies when an object transitions to a specified storage class.</p>
     */
    inline Rule& WithTransition(Transition&& value) { SetTransition(std::move(value)); return *this;}


    
    inline const NoncurrentVersionTransition& GetNoncurrentVersionTransition() const{ return m_noncurrentVersionTransition; }

    
    inline bool NoncurrentVersionTransitionHasBeenSet() const { return m_noncurrentVersionTransitionHasBeenSet; }

    
    inline void SetNoncurrentVersionTransition(const NoncurrentVersionTransition& value) { m_noncurrentVersionTransitionHasBeenSet = true; m_noncurrentVersionTransition = value; }

    
    inline void SetNoncurrentVersionTransition(NoncurrentVersionTransition&& value) { m_noncurrentVersionTransitionHasBeenSet = true; m_noncurrentVersionTransition = std::move(value); }

    
    inline Rule& WithNoncurrentVersionTransition(const NoncurrentVersionTransition& value) { SetNoncurrentVersionTransition(value); return *this;}

    
    inline Rule& WithNoncurrentVersionTransition(NoncurrentVersionTransition&& value) { SetNoncurrentVersionTransition(std::move(value)); return *this;}


    
    inline const NoncurrentVersionExpiration& GetNoncurrentVersionExpiration() const{ return m_noncurrentVersionExpiration; }

    
    inline bool NoncurrentVersionExpirationHasBeenSet() const { return m_noncurrentVersionExpirationHasBeenSet; }

    
    inline void SetNoncurrentVersionExpiration(const NoncurrentVersionExpiration& value) { m_noncurrentVersionExpirationHasBeenSet = true; m_noncurrentVersionExpiration = value; }

    
    inline void SetNoncurrentVersionExpiration(NoncurrentVersionExpiration&& value) { m_noncurrentVersionExpirationHasBeenSet = true; m_noncurrentVersionExpiration = std::move(value); }

    
    inline Rule& WithNoncurrentVersionExpiration(const NoncurrentVersionExpiration& value) { SetNoncurrentVersionExpiration(value); return *this;}

    
    inline Rule& WithNoncurrentVersionExpiration(NoncurrentVersionExpiration&& value) { SetNoncurrentVersionExpiration(std::move(value)); return *this;}


    
    inline const AbortIncompleteMultipartUpload& GetAbortIncompleteMultipartUpload() const{ return m_abortIncompleteMultipartUpload; }

    
    inline bool AbortIncompleteMultipartUploadHasBeenSet() const { return m_abortIncompleteMultipartUploadHasBeenSet; }

    
    inline void SetAbortIncompleteMultipartUpload(const AbortIncompleteMultipartUpload& value) { m_abortIncompleteMultipartUploadHasBeenSet = true; m_abortIncompleteMultipartUpload = value; }

    
    inline void SetAbortIncompleteMultipartUpload(AbortIncompleteMultipartUpload&& value) { m_abortIncompleteMultipartUploadHasBeenSet = true; m_abortIncompleteMultipartUpload = std::move(value); }

    
    inline Rule& WithAbortIncompleteMultipartUpload(const AbortIncompleteMultipartUpload& value) { SetAbortIncompleteMultipartUpload(value); return *this;}

    
    inline Rule& WithAbortIncompleteMultipartUpload(AbortIncompleteMultipartUpload&& value) { SetAbortIncompleteMultipartUpload(std::move(value)); return *this;}

  private:

    LifecycleExpiration m_expiration;
    bool m_expirationHasBeenSet;

    Aws::String m_iD;
    bool m_iDHasBeenSet;

    Aws::String m_prefix;
    bool m_prefixHasBeenSet;

    ExpirationStatus m_status;
    bool m_statusHasBeenSet;

    Transition m_transition;
    bool m_transitionHasBeenSet;

    NoncurrentVersionTransition m_noncurrentVersionTransition;
    bool m_noncurrentVersionTransitionHasBeenSet;

    NoncurrentVersionExpiration m_noncurrentVersionExpiration;
    bool m_noncurrentVersionExpirationHasBeenSet;

    AbortIncompleteMultipartUpload m_abortIncompleteMultipartUpload;
    bool m_abortIncompleteMultipartUploadHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
