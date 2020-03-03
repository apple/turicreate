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

#include <aws/s3/model/AnalyticsConfiguration.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

#include <utility>

using namespace Aws::Utils::Xml;
using namespace Aws::Utils;

namespace Aws
{
namespace S3
{
namespace Model
{

AnalyticsConfiguration::AnalyticsConfiguration() : 
    m_idHasBeenSet(false),
    m_filterHasBeenSet(false),
    m_storageClassAnalysisHasBeenSet(false)
{
}

AnalyticsConfiguration::AnalyticsConfiguration(const XmlNode& xmlNode) : 
    m_idHasBeenSet(false),
    m_filterHasBeenSet(false),
    m_storageClassAnalysisHasBeenSet(false)
{
  *this = xmlNode;
}

AnalyticsConfiguration& AnalyticsConfiguration::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode idNode = resultNode.FirstChild("Id");
    if(!idNode.IsNull())
    {
      m_id = Aws::Utils::Xml::DecodeEscapedXmlText(idNode.GetText());
      m_idHasBeenSet = true;
    }
    XmlNode filterNode = resultNode.FirstChild("Filter");
    if(!filterNode.IsNull())
    {
      m_filter = filterNode;
      m_filterHasBeenSet = true;
    }
    XmlNode storageClassAnalysisNode = resultNode.FirstChild("StorageClassAnalysis");
    if(!storageClassAnalysisNode.IsNull())
    {
      m_storageClassAnalysis = storageClassAnalysisNode;
      m_storageClassAnalysisHasBeenSet = true;
    }
  }

  return *this;
}

void AnalyticsConfiguration::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_idHasBeenSet)
  {
   XmlNode idNode = parentNode.CreateChildElement("Id");
   idNode.SetText(m_id);
  }

  if(m_filterHasBeenSet)
  {
   XmlNode filterNode = parentNode.CreateChildElement("Filter");
   m_filter.AddToNode(filterNode);
  }

  if(m_storageClassAnalysisHasBeenSet)
  {
   XmlNode storageClassAnalysisNode = parentNode.CreateChildElement("StorageClassAnalysis");
   m_storageClassAnalysis.AddToNode(storageClassAnalysisNode);
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
