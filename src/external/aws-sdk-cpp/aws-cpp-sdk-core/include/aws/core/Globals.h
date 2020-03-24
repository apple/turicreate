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

#include <aws/core/Core_EXPORTS.h>

namespace Aws
{
    namespace Utils
    {
        class EnumParseOverflowContainer;
    }
    /**
     * This is used to handle the Enum round tripping problem
     * for when a service updates their enumerations, but the user does not
     * have an up to date client. This container will be initialized during Aws::InitAPI
     * and will be cleaned on Aws::ShutdownAPI.
     */
    AWS_CORE_API Utils::EnumParseOverflowContainer* GetEnumOverflowContainer();

    /**
     * Initializes a global overflow container to a new instance.
     * This should only be called once from within Aws::InitAPI
     */
    void InitializeEnumOverflowContainer();

    /**
     * Destroys the global overflow container instance.
     * This should only be called once from within Aws::ShutdownAPI
     */
    void CleanupEnumOverflowContainer();
}
