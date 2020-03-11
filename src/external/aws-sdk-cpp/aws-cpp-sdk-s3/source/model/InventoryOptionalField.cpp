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

#include <aws/s3/model/InventoryOptionalField.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/Globals.h>
#include <aws/core/utils/EnumParseOverflowContainer.h>

using namespace Aws::Utils;


namespace Aws
{
  namespace S3
  {
    namespace Model
    {
      namespace InventoryOptionalFieldMapper
      {

        static const int Size_HASH = HashingUtils::HashString("Size");
        static const int LastModifiedDate_HASH = HashingUtils::HashString("LastModifiedDate");
        static const int StorageClass_HASH = HashingUtils::HashString("StorageClass");
        static const int ETag_HASH = HashingUtils::HashString("ETag");
        static const int IsMultipartUploaded_HASH = HashingUtils::HashString("IsMultipartUploaded");
        static const int ReplicationStatus_HASH = HashingUtils::HashString("ReplicationStatus");
        static const int EncryptionStatus_HASH = HashingUtils::HashString("EncryptionStatus");
        static const int ObjectLockRetainUntilDate_HASH = HashingUtils::HashString("ObjectLockRetainUntilDate");
        static const int ObjectLockMode_HASH = HashingUtils::HashString("ObjectLockMode");
        static const int ObjectLockLegalHoldStatus_HASH = HashingUtils::HashString("ObjectLockLegalHoldStatus");
        static const int IntelligentTieringAccessTier_HASH = HashingUtils::HashString("IntelligentTieringAccessTier");


        InventoryOptionalField GetInventoryOptionalFieldForName(const Aws::String& name)
        {
          int hashCode = HashingUtils::HashString(name.c_str());
          if (hashCode == Size_HASH)
          {
            return InventoryOptionalField::Size;
          }
          else if (hashCode == LastModifiedDate_HASH)
          {
            return InventoryOptionalField::LastModifiedDate;
          }
          else if (hashCode == StorageClass_HASH)
          {
            return InventoryOptionalField::StorageClass;
          }
          else if (hashCode == ETag_HASH)
          {
            return InventoryOptionalField::ETag;
          }
          else if (hashCode == IsMultipartUploaded_HASH)
          {
            return InventoryOptionalField::IsMultipartUploaded;
          }
          else if (hashCode == ReplicationStatus_HASH)
          {
            return InventoryOptionalField::ReplicationStatus;
          }
          else if (hashCode == EncryptionStatus_HASH)
          {
            return InventoryOptionalField::EncryptionStatus;
          }
          else if (hashCode == ObjectLockRetainUntilDate_HASH)
          {
            return InventoryOptionalField::ObjectLockRetainUntilDate;
          }
          else if (hashCode == ObjectLockMode_HASH)
          {
            return InventoryOptionalField::ObjectLockMode;
          }
          else if (hashCode == ObjectLockLegalHoldStatus_HASH)
          {
            return InventoryOptionalField::ObjectLockLegalHoldStatus;
          }
          else if (hashCode == IntelligentTieringAccessTier_HASH)
          {
            return InventoryOptionalField::IntelligentTieringAccessTier;
          }
          EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
          if(overflowContainer)
          {
            overflowContainer->StoreOverflow(hashCode, name);
            return static_cast<InventoryOptionalField>(hashCode);
          }

          return InventoryOptionalField::NOT_SET;
        }

        Aws::String GetNameForInventoryOptionalField(InventoryOptionalField enumValue)
        {
          switch(enumValue)
          {
          case InventoryOptionalField::Size:
            return "Size";
          case InventoryOptionalField::LastModifiedDate:
            return "LastModifiedDate";
          case InventoryOptionalField::StorageClass:
            return "StorageClass";
          case InventoryOptionalField::ETag:
            return "ETag";
          case InventoryOptionalField::IsMultipartUploaded:
            return "IsMultipartUploaded";
          case InventoryOptionalField::ReplicationStatus:
            return "ReplicationStatus";
          case InventoryOptionalField::EncryptionStatus:
            return "EncryptionStatus";
          case InventoryOptionalField::ObjectLockRetainUntilDate:
            return "ObjectLockRetainUntilDate";
          case InventoryOptionalField::ObjectLockMode:
            return "ObjectLockMode";
          case InventoryOptionalField::ObjectLockLegalHoldStatus:
            return "ObjectLockLegalHoldStatus";
          case InventoryOptionalField::IntelligentTieringAccessTier:
            return "IntelligentTieringAccessTier";
          default:
            EnumParseOverflowContainer* overflowContainer = Aws::GetEnumOverflowContainer();
            if(overflowContainer)
            {
              return overflowContainer->RetrieveOverflow(static_cast<int>(enumValue));
            }

            return {};
          }
        }

      } // namespace InventoryOptionalFieldMapper
    } // namespace Model
  } // namespace S3
} // namespace Aws
