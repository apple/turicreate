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

#include <aws/core/utils/memory/AWSMemory.h>

#include <aws/core/utils/memory/MemorySystemInterface.h>
#include <aws/common/common.h>

#include <atomic>

using namespace Aws::Utils;
using namespace Aws::Utils::Memory;

#ifdef USE_AWS_MEMORY_MANAGEMENT
  static MemorySystemInterface* AWSMemorySystem(nullptr);
#endif // USE_AWS_MEMORY_MANAGEMENT

namespace Aws
{
namespace Utils
{
namespace Memory
{

void InitializeAWSMemorySystem(MemorySystemInterface& memorySystem)
{
    #ifdef USE_AWS_MEMORY_MANAGEMENT
        if(AWSMemorySystem != nullptr)
        {
            AWSMemorySystem->End();
        }

        AWSMemorySystem = &memorySystem;
        AWSMemorySystem->Begin();
    #else
        AWS_UNREFERENCED_PARAM(memorySystem);
    #endif // USE_AWS_MEMORY_MANAGEMENT
}

void ShutdownAWSMemorySystem(void)
{
    #ifdef USE_AWS_MEMORY_MANAGEMENT
        if(AWSMemorySystem != nullptr)
        {
            AWSMemorySystem->End();
        }
        AWSMemorySystem = nullptr;
    #endif // USE_AWS_MEMORY_MANAGEMENT
}

MemorySystemInterface* GetMemorySystem()
{
    #ifdef USE_AWS_MEMORY_MANAGEMENT
        return AWSMemorySystem;
    #else
        return nullptr;
    #endif // USE_AWS_MEMORY_MANAGEMENT
}

} // namespace Memory
} // namespace Utils

void* Malloc(const char* allocationTag, size_t allocationSize)
{
    Aws::Utils::Memory::MemorySystemInterface* memorySystem = Aws::Utils::Memory::GetMemorySystem();

    void* rawMemory = nullptr;
    if(memorySystem != nullptr)
    {
        rawMemory = memorySystem->AllocateMemory(allocationSize, 1, allocationTag);
    }
    else
    {
        rawMemory = malloc(allocationSize);
    }

    return rawMemory;
}


void Free(void* memoryPtr)
{
    if(memoryPtr == nullptr)
    {
        return;
    }

    Aws::Utils::Memory::MemorySystemInterface* memorySystem = Aws::Utils::Memory::GetMemorySystem();
    if(memorySystem != nullptr)
    {
        memorySystem->FreeMemory(memoryPtr);
    }
    else
    {
        free(memoryPtr);
    }
}

static void* MemAcquire(aws_allocator* allocator, size_t size)
{
    (void)allocator; // unused;
    return Aws::Malloc("MemAcquire", size);
}

static void MemRelease(aws_allocator* allocator, void* ptr)
{
    (void)allocator; // unused;
    return Aws::Free(ptr);
}

static aws_allocator create_aws_allocator()
{
#if (__GNUC__ == 4) && !defined(__clang__)
    AWS_SUPPRESS_WARNING("-Wmissing-field-initializers", aws_allocator wrapper{};);
#else
    aws_allocator wrapper{};
#endif
    wrapper.mem_acquire = MemAcquire;
    wrapper.mem_release = MemRelease;
    wrapper.mem_realloc = nullptr;
    return wrapper;
}

aws_allocator* get_aws_allocator()
{
    static aws_allocator wrapper = create_aws_allocator();
    return &wrapper;
}

} // namespace Aws


