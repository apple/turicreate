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

#include <aws/s3/model/StorageClassAnalysisDataExport.h>
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

StorageClassAnalysisDataExport::StorageClassAnalysisDataExport() : 
    m_outputSchemaVersion(StorageClassAnalysisSchemaVersion::NOT_SET),
    m_outputSchemaVersionHasBeenSet(false),
    m_destinationHasBeenSet(false)
{
}

StorageClassAnalysisDataExport::StorageClassAnalysisDataExport(const XmlNode& xmlNode) : 
    m_outputSchemaVersion(StorageClassAnalysisSchemaVersion::NOT_SET),
    m_outputSchemaVersionHasBeenSet(false),
    m_destinationHasBeenSet(false)
{
  *this = xmlNode;
}

StorageClassAnalysisDataExport& StorageClassAnalysisDataExport::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode outputSchemaVersionNode = resultNode.FirstChild("OutputSchemaVersion");
    if(!outputSchemaVersionNode.IsNull())
    {
      m_outputSchemaVersion = StorageClassAnalysisSchemaVersionMapper::GetStorageClassAnalysisSchemaVersionForName(StringUtils::Trim(Aws::Utils::Xml::DecodeEscapedXmlText(outputSchemaVersionNode.GetText()).c_str()).c_str());
      m_outputSchemaVersionHasBeenSet = true;
    }
    XmlNode destinationNode = resultNode.FirstChild("Destination");
    if(!destinationNode.IsNull())
    {
      m_destination = destinationNode;
      m_destinationHasBeenSet = true;
    }
  }

  return *this;
}

void StorageClassAnalysisDataExport::AddToNode(XmlNode& parentNode) const
{
  Aws::StringStream ss;
  if(m_outputSchemaVersionHasBeenSet)
  {
   XmlNode outputSchemaVersionNode = parentNode.CreateChildElement("OutputSchemaVersion");
   outputSchemaVersionNode.SetText(StorageClassAnalysisSchemaVersionMapper::GetNameForStorageClassAnalysisSchemaVersion(m_outputSchemaVersion));
  }

  if(m_destinationHasBeenSet)
  {
   XmlNode destinationNode = parentNode.CreateChildElement("Destination");
   m_destination.AddToNode(destinationNode);
  }

}

} // namespace Model
} // namespace S3
} // namespace Aws
