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
#include <aws/core/utils/Array.h>
#include <utility>

namespace Aws
{
namespace S3
{
namespace Model
{
  /**
   * <p>The container for the records event.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/RecordsEvent">AWS API
   * Reference</a></p>
   */
  class AWS_S3_API RecordsEvent
  {
  public:
    RecordsEvent() = default;
    RecordsEvent(Aws::Vector<unsigned char>&& value) { m_payload = std::move(value); }

    /**
     * <p>The byte array of partial, one or more result records.</p>
     */
    inline const Aws::Vector<unsigned char>& GetPayload() const { return m_payload; }

    /**
     * <p>The byte array of partial, one or more result records.</p>
     */
    inline Aws::Vector<unsigned char>&& GetPayloadWithOwnership() { return std::move(m_payload); }

    /**
     * <p>The byte array of partial, one or more result records.</p>
     */
    inline void SetPayload(const Aws::Vector<unsigned char>& value) { m_payloadHasBeenSet = true; m_payload = value; }

    /**
     * <p>The byte array of partial, one or more result records.</p>
     */
    inline void SetPayload(Aws::Vector<unsigned char>&& value) { m_payloadHasBeenSet = true; m_payload = std::move(value); }

    /**
     * <p>The byte array of partial, one or more result records.</p>
     */
    inline RecordsEvent& WithPayload(const Aws::Vector<unsigned char>& value) { SetPayload(value); return *this;}

    /**
     * <p>The byte array of partial, one or more result records.</p>
     */
    inline RecordsEvent& WithPayload(Aws::Vector<unsigned char>&& value) { SetPayload(std::move(value)); return *this;}

  private:

    Aws::Vector<unsigned char> m_payload;
    bool m_payloadHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
