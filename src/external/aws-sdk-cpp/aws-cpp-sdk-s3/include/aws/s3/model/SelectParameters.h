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
#include <aws/s3/model/InputSerialization.h>
#include <aws/s3/model/ExpressionType.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/s3/model/OutputSerialization.h>
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
   * <p>Describes the parameters for Select job types.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/SelectParameters">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API SelectParameters
  {
  public:
    SelectParameters();
    SelectParameters(const Aws::Utils::Xml::XmlNode& xmlNode);
    SelectParameters& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Describes the serialization format of the object.</p>
     */
    inline const InputSerialization& GetInputSerialization() const{ return m_inputSerialization; }

    /**
     * <p>Describes the serialization format of the object.</p>
     */
    inline bool InputSerializationHasBeenSet() const { return m_inputSerializationHasBeenSet; }

    /**
     * <p>Describes the serialization format of the object.</p>
     */
    inline void SetInputSerialization(const InputSerialization& value) { m_inputSerializationHasBeenSet = true; m_inputSerialization = value; }

    /**
     * <p>Describes the serialization format of the object.</p>
     */
    inline void SetInputSerialization(InputSerialization&& value) { m_inputSerializationHasBeenSet = true; m_inputSerialization = std::move(value); }

    /**
     * <p>Describes the serialization format of the object.</p>
     */
    inline SelectParameters& WithInputSerialization(const InputSerialization& value) { SetInputSerialization(value); return *this;}

    /**
     * <p>Describes the serialization format of the object.</p>
     */
    inline SelectParameters& WithInputSerialization(InputSerialization&& value) { SetInputSerialization(std::move(value)); return *this;}


    /**
     * <p>The type of the provided expression (for example, SQL).</p>
     */
    inline const ExpressionType& GetExpressionType() const{ return m_expressionType; }

    /**
     * <p>The type of the provided expression (for example, SQL).</p>
     */
    inline bool ExpressionTypeHasBeenSet() const { return m_expressionTypeHasBeenSet; }

    /**
     * <p>The type of the provided expression (for example, SQL).</p>
     */
    inline void SetExpressionType(const ExpressionType& value) { m_expressionTypeHasBeenSet = true; m_expressionType = value; }

    /**
     * <p>The type of the provided expression (for example, SQL).</p>
     */
    inline void SetExpressionType(ExpressionType&& value) { m_expressionTypeHasBeenSet = true; m_expressionType = std::move(value); }

    /**
     * <p>The type of the provided expression (for example, SQL).</p>
     */
    inline SelectParameters& WithExpressionType(const ExpressionType& value) { SetExpressionType(value); return *this;}

    /**
     * <p>The type of the provided expression (for example, SQL).</p>
     */
    inline SelectParameters& WithExpressionType(ExpressionType&& value) { SetExpressionType(std::move(value)); return *this;}


    /**
     * <p>The expression that is used to query the object.</p>
     */
    inline const Aws::String& GetExpression() const{ return m_expression; }

    /**
     * <p>The expression that is used to query the object.</p>
     */
    inline bool ExpressionHasBeenSet() const { return m_expressionHasBeenSet; }

    /**
     * <p>The expression that is used to query the object.</p>
     */
    inline void SetExpression(const Aws::String& value) { m_expressionHasBeenSet = true; m_expression = value; }

    /**
     * <p>The expression that is used to query the object.</p>
     */
    inline void SetExpression(Aws::String&& value) { m_expressionHasBeenSet = true; m_expression = std::move(value); }

    /**
     * <p>The expression that is used to query the object.</p>
     */
    inline void SetExpression(const char* value) { m_expressionHasBeenSet = true; m_expression.assign(value); }

    /**
     * <p>The expression that is used to query the object.</p>
     */
    inline SelectParameters& WithExpression(const Aws::String& value) { SetExpression(value); return *this;}

    /**
     * <p>The expression that is used to query the object.</p>
     */
    inline SelectParameters& WithExpression(Aws::String&& value) { SetExpression(std::move(value)); return *this;}

    /**
     * <p>The expression that is used to query the object.</p>
     */
    inline SelectParameters& WithExpression(const char* value) { SetExpression(value); return *this;}


    /**
     * <p>Describes how the results of the Select job are serialized.</p>
     */
    inline const OutputSerialization& GetOutputSerialization() const{ return m_outputSerialization; }

    /**
     * <p>Describes how the results of the Select job are serialized.</p>
     */
    inline bool OutputSerializationHasBeenSet() const { return m_outputSerializationHasBeenSet; }

    /**
     * <p>Describes how the results of the Select job are serialized.</p>
     */
    inline void SetOutputSerialization(const OutputSerialization& value) { m_outputSerializationHasBeenSet = true; m_outputSerialization = value; }

    /**
     * <p>Describes how the results of the Select job are serialized.</p>
     */
    inline void SetOutputSerialization(OutputSerialization&& value) { m_outputSerializationHasBeenSet = true; m_outputSerialization = std::move(value); }

    /**
     * <p>Describes how the results of the Select job are serialized.</p>
     */
    inline SelectParameters& WithOutputSerialization(const OutputSerialization& value) { SetOutputSerialization(value); return *this;}

    /**
     * <p>Describes how the results of the Select job are serialized.</p>
     */
    inline SelectParameters& WithOutputSerialization(OutputSerialization&& value) { SetOutputSerialization(std::move(value)); return *this;}

  private:

    InputSerialization m_inputSerialization;
    bool m_inputSerializationHasBeenSet;

    ExpressionType m_expressionType;
    bool m_expressionTypeHasBeenSet;

    Aws::String m_expression;
    bool m_expressionHasBeenSet;

    OutputSerialization m_outputSerialization;
    bool m_outputSerializationHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
