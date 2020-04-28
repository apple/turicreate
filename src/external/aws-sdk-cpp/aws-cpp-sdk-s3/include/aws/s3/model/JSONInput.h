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
#include <aws/s3/model/JSONType.h>
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
   * <p>Specifies JSON as object's input serialization format.</p><p><h3>See
   * Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/JSONInput">AWS API
   * Reference</a></p>
   */
  class AWS_S3_API JSONInput
  {
  public:
    JSONInput();
    JSONInput(const Aws::Utils::Xml::XmlNode& xmlNode);
    JSONInput& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>The type of JSON. Valid values: Document, Lines.</p>
     */
    inline const JSONType& GetType() const{ return m_type; }

    /**
     * <p>The type of JSON. Valid values: Document, Lines.</p>
     */
    inline bool TypeHasBeenSet() const { return m_typeHasBeenSet; }

    /**
     * <p>The type of JSON. Valid values: Document, Lines.</p>
     */
    inline void SetType(const JSONType& value) { m_typeHasBeenSet = true; m_type = value; }

    /**
     * <p>The type of JSON. Valid values: Document, Lines.</p>
     */
    inline void SetType(JSONType&& value) { m_typeHasBeenSet = true; m_type = std::move(value); }

    /**
     * <p>The type of JSON. Valid values: Document, Lines.</p>
     */
    inline JSONInput& WithType(const JSONType& value) { SetType(value); return *this;}

    /**
     * <p>The type of JSON. Valid values: Document, Lines.</p>
     */
    inline JSONInput& WithType(JSONType&& value) { SetType(std::move(value)); return *this;}

  private:

    JSONType m_type;
    bool m_typeHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
