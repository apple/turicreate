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
#include <aws/core/utils/memory/stl/AWSVector.h>
#include <aws/s3/model/ObjectIdentifier.h>
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
   * <p>Container for the objects to delete.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/Delete">AWS API
   * Reference</a></p>
   */
  class AWS_S3_API Delete
  {
  public:
    Delete();
    Delete(const Aws::Utils::Xml::XmlNode& xmlNode);
    Delete& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>The objects to delete.</p>
     */
    inline const Aws::Vector<ObjectIdentifier>& GetObjects() const{ return m_objects; }

    /**
     * <p>The objects to delete.</p>
     */
    inline bool ObjectsHasBeenSet() const { return m_objectsHasBeenSet; }

    /**
     * <p>The objects to delete.</p>
     */
    inline void SetObjects(const Aws::Vector<ObjectIdentifier>& value) { m_objectsHasBeenSet = true; m_objects = value; }

    /**
     * <p>The objects to delete.</p>
     */
    inline void SetObjects(Aws::Vector<ObjectIdentifier>&& value) { m_objectsHasBeenSet = true; m_objects = std::move(value); }

    /**
     * <p>The objects to delete.</p>
     */
    inline Delete& WithObjects(const Aws::Vector<ObjectIdentifier>& value) { SetObjects(value); return *this;}

    /**
     * <p>The objects to delete.</p>
     */
    inline Delete& WithObjects(Aws::Vector<ObjectIdentifier>&& value) { SetObjects(std::move(value)); return *this;}

    /**
     * <p>The objects to delete.</p>
     */
    inline Delete& AddObjects(const ObjectIdentifier& value) { m_objectsHasBeenSet = true; m_objects.push_back(value); return *this; }

    /**
     * <p>The objects to delete.</p>
     */
    inline Delete& AddObjects(ObjectIdentifier&& value) { m_objectsHasBeenSet = true; m_objects.push_back(std::move(value)); return *this; }


    /**
     * <p>Element to enable quiet mode for the request. When you add this element, you
     * must set its value to true.</p>
     */
    inline bool GetQuiet() const{ return m_quiet; }

    /**
     * <p>Element to enable quiet mode for the request. When you add this element, you
     * must set its value to true.</p>
     */
    inline bool QuietHasBeenSet() const { return m_quietHasBeenSet; }

    /**
     * <p>Element to enable quiet mode for the request. When you add this element, you
     * must set its value to true.</p>
     */
    inline void SetQuiet(bool value) { m_quietHasBeenSet = true; m_quiet = value; }

    /**
     * <p>Element to enable quiet mode for the request. When you add this element, you
     * must set its value to true.</p>
     */
    inline Delete& WithQuiet(bool value) { SetQuiet(value); return *this;}

  private:

    Aws::Vector<ObjectIdentifier> m_objects;
    bool m_objectsHasBeenSet;

    bool m_quiet;
    bool m_quietHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
