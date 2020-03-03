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
#include <aws/s3/model/Payer.h>
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
   * <p>Container for Payer.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/RequestPaymentConfiguration">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API RequestPaymentConfiguration
  {
  public:
    RequestPaymentConfiguration();
    RequestPaymentConfiguration(const Aws::Utils::Xml::XmlNode& xmlNode);
    RequestPaymentConfiguration& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Specifies who pays for the download and request fees.</p>
     */
    inline const Payer& GetPayer() const{ return m_payer; }

    /**
     * <p>Specifies who pays for the download and request fees.</p>
     */
    inline bool PayerHasBeenSet() const { return m_payerHasBeenSet; }

    /**
     * <p>Specifies who pays for the download and request fees.</p>
     */
    inline void SetPayer(const Payer& value) { m_payerHasBeenSet = true; m_payer = value; }

    /**
     * <p>Specifies who pays for the download and request fees.</p>
     */
    inline void SetPayer(Payer&& value) { m_payerHasBeenSet = true; m_payer = std::move(value); }

    /**
     * <p>Specifies who pays for the download and request fees.</p>
     */
    inline RequestPaymentConfiguration& WithPayer(const Payer& value) { SetPayer(value); return *this;}

    /**
     * <p>Specifies who pays for the download and request fees.</p>
     */
    inline RequestPaymentConfiguration& WithPayer(Payer&& value) { SetPayer(std::move(value)); return *this;}

  private:

    Payer m_payer;
    bool m_payerHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
