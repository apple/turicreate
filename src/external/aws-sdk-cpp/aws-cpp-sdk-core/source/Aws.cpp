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
#include <aws/core/Version.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/Aws.h>
#include <aws/core/client/CoreErrors.h>
#include <aws/core/utils/logging/AWSLogging.h>
#include <aws/core/utils/logging/DefaultLogSystem.h>
#include <aws/core/Globals.h>
#include <aws/core/external/cjson/cJSON.h>
#include <aws/core/monitoring/MonitoringManager.h>
#include <aws/core/net/Net.h>
#include <aws/core/config/AWSProfileConfigLoader.h>

namespace Aws
{
    static const char* ALLOCATION_TAG = "Aws_Init_Cleanup";

    void InitAPI(const SDKOptions &options)
    {
#ifdef USE_AWS_MEMORY_MANAGEMENT
        if(options.memoryManagementOptions.memoryManager)
        {
            Aws::Utils::Memory::InitializeAWSMemorySystem(*options.memoryManagementOptions.memoryManager);
        }
#endif // USE_AWS_MEMORY_MANAGEMENT
        Aws::Client::CoreErrorsMapper::InitCoreErrorsMapper();
        if(options.loggingOptions.logLevel != Aws::Utils::Logging::LogLevel::Off)
        {
            if(options.loggingOptions.logger_create_fn)
            {
                Aws::Utils::Logging::InitializeAWSLogging(options.loggingOptions.logger_create_fn());
            }
            else
            {
                Aws::Utils::Logging::InitializeAWSLogging(
                        Aws::MakeShared<Aws::Utils::Logging::DefaultLogSystem>(ALLOCATION_TAG, options.loggingOptions.logLevel, options.loggingOptions.defaultLogPrefix));
            }
            // For users to better debugging in case multiple versions of SDK installed
            AWS_LOGSTREAM_INFO(ALLOCATION_TAG, "Initiate AWS SDK for C++ with Version:" << Aws::String(Aws::Version::GetVersionString()));
        }

        Aws::Config::InitConfigAndCredentialsCacheManager();

        if (options.cryptoOptions.aes_CBCFactory_create_fn)
        {
            Aws::Utils::Crypto::SetAES_CBCFactory(options.cryptoOptions.aes_CBCFactory_create_fn());
        }

        if(options.cryptoOptions.aes_CTRFactory_create_fn)
        {
            Aws::Utils::Crypto::SetAES_CTRFactory(options.cryptoOptions.aes_CTRFactory_create_fn());
        }

        if(options.cryptoOptions.aes_GCMFactory_create_fn)
        {
            Aws::Utils::Crypto::SetAES_GCMFactory(options.cryptoOptions.aes_GCMFactory_create_fn());
        }

        if(options.cryptoOptions.md5Factory_create_fn)
        {
            Aws::Utils::Crypto::SetMD5Factory(options.cryptoOptions.md5Factory_create_fn());
        }

        if(options.cryptoOptions.sha256Factory_create_fn)
        {
            Aws::Utils::Crypto::SetSha256Factory(options.cryptoOptions.sha256Factory_create_fn());
        }

        if(options.cryptoOptions.sha256HMACFactory_create_fn)
        {
            Aws::Utils::Crypto::SetSha256HMACFactory(options.cryptoOptions.sha256HMACFactory_create_fn());
        }

        if (options.cryptoOptions.aes_KeyWrapFactory_create_fn)
        {
            Aws::Utils::Crypto::SetAES_KeyWrapFactory(options.cryptoOptions.aes_KeyWrapFactory_create_fn());
        }

        if(options.cryptoOptions.secureRandomFactory_create_fn)
        {
            Aws::Utils::Crypto::SetSecureRandomFactory(options.cryptoOptions.secureRandomFactory_create_fn());
        }

        Aws::Utils::Crypto::SetInitCleanupOpenSSLFlag(options.cryptoOptions.initAndCleanupOpenSSL);
        Aws::Utils::Crypto::InitCrypto();

        if(options.httpOptions.httpClientFactory_create_fn)
        {
            Aws::Http::SetHttpClientFactory(options.httpOptions.httpClientFactory_create_fn());
        }

        Aws::Http::SetInitCleanupCurlFlag(options.httpOptions.initAndCleanupCurl);
        Aws::Http::SetInstallSigPipeHandlerFlag(options.httpOptions.installSigPipeHandler);
        Aws::Http::InitHttp();
        Aws::InitializeEnumOverflowContainer();
        cJSON_Hooks hooks;
        hooks.malloc_fn = [](size_t sz) { return Aws::Malloc("cJSON_Tag", sz); };
        hooks.free_fn = Aws::Free;
        cJSON_InitHooks(&hooks);
        Aws::Net::InitNetwork();
        Aws::Monitoring::InitMonitoring(options.monitoringOptions.customizedMonitoringFactory_create_fn);
    }

    void ShutdownAPI(const SDKOptions& options)
    {
        Aws::Monitoring::CleanupMonitoring();
        Aws::Net::CleanupNetwork();
        Aws::CleanupEnumOverflowContainer();
        Aws::Http::CleanupHttp();
        Aws::Utils::Crypto::CleanupCrypto();

        Aws::Config::CleanupConfigAndCredentialsCacheManager();

        if(options.loggingOptions.logLevel != Aws::Utils::Logging::LogLevel::Off)
        {
            Aws::Utils::Logging::ShutdownAWSLogging();
        }

        Aws::Client::CoreErrorsMapper::CleanupCoreErrorsMapper();

#ifdef USE_AWS_MEMORY_MANAGEMENT
        if(options.memoryManagementOptions.memoryManager)
        {
            Aws::Utils::Memory::ShutdownAWSMemorySystem();
        }
#endif // USE_AWS_MEMORY_MANAGEMENT
    }
}
