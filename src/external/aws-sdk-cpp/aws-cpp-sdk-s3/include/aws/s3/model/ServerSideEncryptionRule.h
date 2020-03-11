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
#include <aws/s3/model/ServerSideEncryptionByDefault.h>
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
   * <p>Specifies the default server-side encryption configuration.</p><p><h3>See
   * Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/ServerSideEncryptionRule">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API ServerSideEncryptionRule
  {
  public:
    ServerSideEncryptionRule();
    ServerSideEncryptionRule(const Aws::Utils::Xml::XmlNode& xmlNode);
    ServerSideEncryptionRule& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Specifies the default server-side encryption to apply to new objects in the
     * bucket. If a PUT Object request doesn't specify any server-side encryption, this
     * default encryption will be applied.</p>
     */
    inline const ServerSideEncryptionByDefault& GetApplyServerSideEncryptionByDefault() const{ return m_applyServerSideEncryptionByDefault; }

    /**
     * <p>Specifies the default server-side encryption to apply to new objects in the
     * bucket. If a PUT Object request doesn't specify any server-side encryption, this
     * default encryption will be applied.</p>
     */
    inline bool ApplyServerSideEncryptionByDefaultHasBeenSet() const { return m_applyServerSideEncryptionByDefaultHasBeenSet; }

    /**
     * <p>Specifies the default server-side encryption to apply to new objects in the
     * bucket. If a PUT Object request doesn't specify any server-side encryption, this
     * default encryption will be applied.</p>
     */
    inline void SetApplyServerSideEncryptionByDefault(const ServerSideEncryptionByDefault& value) { m_applyServerSideEncryptionByDefaultHasBeenSet = true; m_applyServerSideEncryptionByDefault = value; }

    /**
     * <p>Specifies the default server-side encryption to apply to new objects in the
     * bucket. If a PUT Object request doesn't specify any server-side encryption, this
     * default encryption will be applied.</p>
     */
    inline void SetApplyServerSideEncryptionByDefault(ServerSideEncryptionByDefault&& value) { m_applyServerSideEncryptionByDefaultHasBeenSet = true; m_applyServerSideEncryptionByDefault = std::move(value); }

    /**
     * <p>Specifies the default server-side encryption to apply to new objects in the
     * bucket. If a PUT Object request doesn't specify any server-side encryption, this
     * default encryption will be applied.</p>
     */
    inline ServerSideEncryptionRule& WithApplyServerSideEncryptionByDefault(const ServerSideEncryptionByDefault& value) { SetApplyServerSideEncryptionByDefault(value); return *this;}

    /**
     * <p>Specifies the default server-side encryption to apply to new objects in the
     * bucket. If a PUT Object request doesn't specify any server-side encryption, this
     * default encryption will be applied.</p>
     */
    inline ServerSideEncryptionRule& WithApplyServerSideEncryptionByDefault(ServerSideEncryptionByDefault&& value) { SetApplyServerSideEncryptionByDefault(std::move(value)); return *this;}

  private:

    ServerSideEncryptionByDefault m_applyServerSideEncryptionByDefault;
    bool m_applyServerSideEncryptionByDefaultHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
