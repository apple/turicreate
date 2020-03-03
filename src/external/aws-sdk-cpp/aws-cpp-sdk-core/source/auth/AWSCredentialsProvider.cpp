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


#include <aws/core/auth/AWSCredentialsProvider.h>

#include <aws/core/config/AWSProfileConfigLoader.h>
#include <aws/core/platform/Environment.h>
#include <aws/core/platform/FileSystem.h>
#include <aws/core/platform/OSVersionInfo.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/FileSystemUtils.h>
#include <aws/core/client/AWSError.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <cstdlib>
#include <fstream>
#include <string.h>
#include <climits>


using namespace Aws::Utils;
using namespace Aws::Utils::Logging;
using namespace Aws::Auth;
using namespace Aws::Internal;
using namespace Aws::FileSystem;
using namespace Aws::Utils::Xml;
using namespace Aws::Client;
using Aws::Utils::Threading::ReaderLockGuard;
using Aws::Utils::Threading::WriterLockGuard;

static const char ACCESS_KEY_ENV_VAR[] = "AWS_ACCESS_KEY_ID";
static const char SECRET_KEY_ENV_VAR[] = "AWS_SECRET_ACCESS_KEY";
static const char SESSION_TOKEN_ENV_VAR[] = "AWS_SESSION_TOKEN";
static const char DEFAULT_PROFILE[] = "default";
static const char AWS_PROFILE_ENV_VAR[] = "AWS_PROFILE";
static const char AWS_PROFILE_DEFAULT_ENV_VAR[] = "AWS_DEFAULT_PROFILE";

static const char AWS_CREDENTIALS_FILE[] = "AWS_SHARED_CREDENTIALS_FILE";
extern const char AWS_CONFIG_FILE[] = "AWS_CONFIG_FILE";

extern const char PROFILE_DIRECTORY[] = ".aws";
static const char DEFAULT_CREDENTIALS_FILE[] = "credentials";
extern const char DEFAULT_CONFIG_FILE[] = "config";


static const int EXPIRATION_GRACE_PERIOD = 5 * 1000;

void AWSCredentialsProvider::Reload()
{
    m_lastLoadedMs = DateTime::Now().Millis();
}

bool AWSCredentialsProvider::IsTimeToRefresh(long reloadFrequency)
{
    if (DateTime::Now().Millis() - m_lastLoadedMs > reloadFrequency)
    {
        return true;
    }
    return false;
}


static const char* ENVIRONMENT_LOG_TAG = "EnvironmentAWSCredentialsProvider";


AWSCredentials EnvironmentAWSCredentialsProvider::GetAWSCredentials()
{
    auto accessKey = Aws::Environment::GetEnv(ACCESS_KEY_ENV_VAR);
    AWSCredentials credentials;

    if (!accessKey.empty())
    {
        credentials.SetAWSAccessKeyId(accessKey);

        AWS_LOGSTREAM_DEBUG(ENVIRONMENT_LOG_TAG, "Found credential in environment with access key id " << accessKey);
        auto secretKey = Aws::Environment::GetEnv(SECRET_KEY_ENV_VAR);

        if (!secretKey.empty())
        {
            credentials.SetAWSSecretKey(secretKey);
            AWS_LOGSTREAM_INFO(ENVIRONMENT_LOG_TAG, "Found secret key");
        }

        auto sessionToken = Aws::Environment::GetEnv(SESSION_TOKEN_ENV_VAR);

        if(!sessionToken.empty())
        {
            credentials.SetSessionToken(sessionToken);
            AWS_LOGSTREAM_INFO(ENVIRONMENT_LOG_TAG, "Found sessionToken");
        }
    }

    return credentials;
}

Aws::String Aws::Auth::GetConfigProfileFilename()
{
    auto configFileNameFromVar = Aws::Environment::GetEnv(AWS_CONFIG_FILE);
    if (!configFileNameFromVar.empty())
    {
        return configFileNameFromVar;
    }
    else
    {
        return Aws::FileSystem::GetHomeDirectory() + PROFILE_DIRECTORY + PATH_DELIM + DEFAULT_CONFIG_FILE;
    }
}

Aws::String Aws::Auth::GetConfigProfileName()
{
    auto profileFromVar = Aws::Environment::GetEnv(AWS_PROFILE_DEFAULT_ENV_VAR);
    if (profileFromVar.empty())
    {
        profileFromVar = Aws::Environment::GetEnv(AWS_PROFILE_ENV_VAR);
    }

    if (profileFromVar.empty())
    {
        return Aws::String(DEFAULT_PROFILE);
    }
    else
    {
        return profileFromVar;
    }
}

static const char* PROFILE_LOG_TAG = "ProfileConfigFileAWSCredentialsProvider";

Aws::String ProfileConfigFileAWSCredentialsProvider::GetCredentialsProfileFilename()
{
    auto credentialsFileNameFromVar = Aws::Environment::GetEnv(AWS_CREDENTIALS_FILE);

    if (credentialsFileNameFromVar.empty())
    {
        return Aws::FileSystem::GetHomeDirectory() + PROFILE_DIRECTORY + PATH_DELIM + DEFAULT_CREDENTIALS_FILE;
    }
    else
    {
        return credentialsFileNameFromVar;
    }
}

Aws::String ProfileConfigFileAWSCredentialsProvider::GetProfileDirectory()
{
    Aws::String credentialsFileName = GetCredentialsProfileFilename();
    auto lastSeparator = credentialsFileName.find_last_of(PATH_DELIM);
    if (lastSeparator != std::string::npos)
    {
        return credentialsFileName.substr(0, lastSeparator);
    }
    else
    {
        return {};
    }
}

ProfileConfigFileAWSCredentialsProvider::ProfileConfigFileAWSCredentialsProvider(long refreshRateMs) :
    m_profileToUse(Aws::Auth::GetConfigProfileName()),
    m_credentialsFileLoader(GetCredentialsProfileFilename()),
    m_loadFrequencyMs(refreshRateMs)
{
    AWS_LOGSTREAM_INFO(PROFILE_LOG_TAG, "Setting provider to read credentials from " <<  GetCredentialsProfileFilename() << " for credentials file"
                                      << " and " <<  GetConfigProfileFilename() << " for the config file "
                                      << ", for use with profile " << m_profileToUse);
}

ProfileConfigFileAWSCredentialsProvider::ProfileConfigFileAWSCredentialsProvider(const char* profile, long refreshRateMs) :
    m_profileToUse(profile),
    m_credentialsFileLoader(GetCredentialsProfileFilename()),
    m_loadFrequencyMs(refreshRateMs)
{
    AWS_LOGSTREAM_INFO(PROFILE_LOG_TAG, "Setting provider to read credentials from " <<  GetCredentialsProfileFilename() << " for credentials file"
                                      << " and " <<  GetConfigProfileFilename() << " for the config file "
                                      << ", for use with profile " << m_profileToUse);
}

AWSCredentials ProfileConfigFileAWSCredentialsProvider::GetAWSCredentials()
{
    RefreshIfExpired();
    ReaderLockGuard guard(m_reloadLock);
    auto credsFileProfileIter = m_credentialsFileLoader.GetProfiles().find(m_profileToUse);

    if(credsFileProfileIter != m_credentialsFileLoader.GetProfiles().end())
    {
        return credsFileProfileIter->second.GetCredentials();
    }

    return AWSCredentials();
}


void ProfileConfigFileAWSCredentialsProvider::Reload()
{
    m_credentialsFileLoader.Load();
    AWSCredentialsProvider::Reload();
}

void ProfileConfigFileAWSCredentialsProvider::RefreshIfExpired()
{
    ReaderLockGuard guard(m_reloadLock);
    if (!IsTimeToRefresh(m_loadFrequencyMs))
    {
       return;
    }

    guard.UpgradeToWriterLock();
    if (!IsTimeToRefresh(m_loadFrequencyMs)) // double-checked lock to avoid refreshing twice
    {
        return;
    }

    Reload();
}

static const char* INSTANCE_LOG_TAG = "InstanceProfileCredentialsProvider";

InstanceProfileCredentialsProvider::InstanceProfileCredentialsProvider(long refreshRateMs) :
    m_ec2MetadataConfigLoader(Aws::MakeShared<Aws::Config::EC2InstanceProfileConfigLoader>(INSTANCE_LOG_TAG)),
    m_loadFrequencyMs(refreshRateMs)
{
    AWS_LOGSTREAM_INFO(INSTANCE_LOG_TAG, "Creating Instance with default EC2MetadataClient and refresh rate " << refreshRateMs);
}


InstanceProfileCredentialsProvider::InstanceProfileCredentialsProvider(const std::shared_ptr<Aws::Config::EC2InstanceProfileConfigLoader>& loader, long refreshRateMs) :
    m_ec2MetadataConfigLoader(loader),
    m_loadFrequencyMs(refreshRateMs)
{
    AWS_LOGSTREAM_INFO(INSTANCE_LOG_TAG, "Creating Instance with injected EC2MetadataClient and refresh rate " << refreshRateMs);
}


AWSCredentials InstanceProfileCredentialsProvider::GetAWSCredentials()
{
    RefreshIfExpired();
    ReaderLockGuard guard(m_reloadLock);
    auto profileIter = m_ec2MetadataConfigLoader->GetProfiles().find(Aws::Config::INSTANCE_PROFILE_KEY);

    if(profileIter != m_ec2MetadataConfigLoader->GetProfiles().end())
    {
        return profileIter->second.GetCredentials();
    }

    return AWSCredentials();
}

void InstanceProfileCredentialsProvider::Reload()
{
    AWS_LOGSTREAM_INFO(INSTANCE_LOG_TAG, "Credentials have expired attempting to repull from EC2 Metadata Service.");
    m_ec2MetadataConfigLoader->Load();
    AWSCredentialsProvider::Reload();
}

void InstanceProfileCredentialsProvider::RefreshIfExpired()
{
    AWS_LOGSTREAM_DEBUG(INSTANCE_LOG_TAG, "Checking if latest credential pull has expired.");
    ReaderLockGuard guard(m_reloadLock);
    if (!IsTimeToRefresh(m_loadFrequencyMs))
    {
        return;
    }

    guard.UpgradeToWriterLock();
    if (!IsTimeToRefresh(m_loadFrequencyMs)) // double-checked lock to avoid refreshing twice
    {
        return;
    }
    Reload();
}

static const char TASK_ROLE_LOG_TAG[] = "TaskRoleCredentialsProvider";

TaskRoleCredentialsProvider::TaskRoleCredentialsProvider(const char* URI, long refreshRateMs) :
    m_ecsCredentialsClient(Aws::MakeShared<Aws::Internal::ECSCredentialsClient>(TASK_ROLE_LOG_TAG, URI)),
    m_loadFrequencyMs(refreshRateMs)
{
    AWS_LOGSTREAM_INFO(TASK_ROLE_LOG_TAG, "Creating TaskRole with default ECSCredentialsClient and refresh rate " << refreshRateMs);
}

TaskRoleCredentialsProvider::TaskRoleCredentialsProvider(const char* endpoint, const char* token, long refreshRateMs) :
    m_ecsCredentialsClient(Aws::MakeShared<Aws::Internal::ECSCredentialsClient>(TASK_ROLE_LOG_TAG, ""/*resourcePath*/, endpoint, token)),
    m_loadFrequencyMs(refreshRateMs)
{
    AWS_LOGSTREAM_INFO(TASK_ROLE_LOG_TAG, "Creating TaskRole with default ECSCredentialsClient and refresh rate " << refreshRateMs);
}

TaskRoleCredentialsProvider::TaskRoleCredentialsProvider(
        const std::shared_ptr<Aws::Internal::ECSCredentialsClient>& client, long refreshRateMs) :
    m_ecsCredentialsClient(client),
    m_loadFrequencyMs(refreshRateMs)
{
    AWS_LOGSTREAM_INFO(TASK_ROLE_LOG_TAG, "Creating TaskRole with default ECSCredentialsClient and refresh rate " << refreshRateMs);
}

AWSCredentials TaskRoleCredentialsProvider::GetAWSCredentials()
{
    RefreshIfExpired();
    ReaderLockGuard guard(m_reloadLock);
    return m_credentials;
}

bool TaskRoleCredentialsProvider::ExpiresSoon() const
{
    return ((m_credentials.GetExpiration() - Aws::Utils::DateTime::Now()).count() < EXPIRATION_GRACE_PERIOD);
}

void TaskRoleCredentialsProvider::Reload()
{
    AWS_LOGSTREAM_INFO(TASK_ROLE_LOG_TAG, "Credentials have expired or will expire, attempting to repull from ECS IAM Service.");

    auto credentialsStr = m_ecsCredentialsClient->GetECSCredentials();
    if (credentialsStr.empty()) return;

    Json::JsonValue credentialsDoc(credentialsStr);
    if (!credentialsDoc.WasParseSuccessful())
    {
        AWS_LOGSTREAM_ERROR(TASK_ROLE_LOG_TAG, "Failed to parse output from ECSCredentialService.");
        return;
    }

    Aws::String accessKey, secretKey, token;
    Utils::Json::JsonView credentialsView(credentialsDoc);
    accessKey = credentialsView.GetString("AccessKeyId");
    secretKey = credentialsView.GetString("SecretAccessKey");
    token = credentialsView.GetString("Token");
    AWS_LOGSTREAM_DEBUG(TASK_ROLE_LOG_TAG, "Successfully pulled credentials from metadata service with access key " << accessKey);

    m_credentials.SetAWSAccessKeyId(accessKey);
    m_credentials.SetAWSSecretKey(secretKey);
    m_credentials.SetSessionToken(token);
    m_credentials.SetExpiration(Aws::Utils::DateTime(credentialsView.GetString("Expiration"), DateFormat::ISO_8601));
    AWSCredentialsProvider::Reload();
}

void TaskRoleCredentialsProvider::RefreshIfExpired()
{
    AWS_LOGSTREAM_DEBUG(TASK_ROLE_LOG_TAG, "Checking if latest credential pull has expired.");
    ReaderLockGuard guard(m_reloadLock);
    if (!m_credentials.IsEmpty() && !IsTimeToRefresh(m_loadFrequencyMs) && !ExpiresSoon())
    {
        return;
    }

    guard.UpgradeToWriterLock();

    if (!m_credentials.IsEmpty() && !IsTimeToRefresh(m_loadFrequencyMs) && !ExpiresSoon())
    {
        return;
    }

    Reload();
}

static const char PROCESS_LOG_TAG[] = "ProcessCredentialsProvider";
ProcessCredentialsProvider::ProcessCredentialsProvider() :
    m_profileToUse(Aws::Auth::GetConfigProfileName())
{
    AWS_LOGSTREAM_INFO(PROCESS_LOG_TAG, "Setting process credentials provider to read config from " <<  m_profileToUse);
}

ProcessCredentialsProvider::ProcessCredentialsProvider(const Aws::String& profile) :
    m_profileToUse(profile)
{
    AWS_LOGSTREAM_INFO(PROCESS_LOG_TAG, "Setting process credentials provider to read config from " <<  m_profileToUse);
}

AWSCredentials ProcessCredentialsProvider::GetAWSCredentials()
{
    RefreshIfExpired();
    ReaderLockGuard guard(m_reloadLock);
    return m_credentials;
}


void ProcessCredentialsProvider::Reload()
{
    auto profile = Aws::Config::GetCachedConfigProfile(m_profileToUse);
    const Aws::String &command = profile.GetCredentialProcess();
    if (command.empty())
    {
        AWS_LOGSTREAM_ERROR(PROCESS_LOG_TAG, "Failed to find credential process's profile: " << m_profileToUse);
        return;
    }
    m_credentials = GetCredentialsFromProcess(command);
}

void ProcessCredentialsProvider::RefreshIfExpired()
{
    ReaderLockGuard guard(m_reloadLock);
    if (!m_credentials.IsExpiredOrEmpty())
    {
       return;
    }

    guard.UpgradeToWriterLock();
    if (!m_credentials.IsExpiredOrEmpty()) // double-checked lock to avoid refreshing twice
    {
        return;
    }

    Reload();
}

AWSCredentials Aws::Auth::GetCredentialsFromProcess(const Aws::String& process)
{
    Aws::String command = process;
    command.append(" 2>&1"); // redirect stderr to stdout
    Aws::String result = Aws::Utils::StringUtils::Trim(Aws::OSVersionInfo::GetSysCommandOutput(command.c_str()).c_str());
    Json::JsonValue credentialsDoc(result);
    if (!credentialsDoc.WasParseSuccessful())
    {
        AWS_LOGSTREAM_ERROR(PROFILE_LOG_TAG, "Failed to load credential from running: " << command << " Error: " << result);
        return {};
    }

    Aws::Utils::Json::JsonView credentialsView(credentialsDoc);
    if (!credentialsView.KeyExists("Version") || credentialsView.GetInteger("Version") != 1)
    {
        AWS_LOGSTREAM_ERROR(PROFILE_LOG_TAG, "Encountered an unsupported process credentials payload version:" << credentialsView.GetInteger("Version"));
        return {};
    }

    AWSCredentials credentials;
    Aws::String accessKey, secretKey, token, expire;
    if (credentialsView.KeyExists("AccessKeyId"))
    {
        credentials.SetAWSAccessKeyId(credentialsView.GetString("AccessKeyId"));
    }

    if (credentialsView.KeyExists("SecretAccessKey"))
    {
        credentials.SetAWSSecretKey(credentialsView.GetString("SecretAccessKey"));
    }

    if (credentialsView.KeyExists("SessionToken"))
    {
        credentials.SetSessionToken(credentialsView.GetString("SessionToken"));
    }

    if (credentialsView.KeyExists("Expiration"))
    {
        const auto expiration = Aws::Utils::DateTime(credentialsView.GetString("Expiration"), DateFormat::ISO_8601);
        if (expiration.WasParseSuccessful())
        {
            credentials.SetExpiration(expiration);
        }
        else
        {
            AWS_LOGSTREAM_ERROR(PROFILE_LOG_TAG, "Failed to parse credential's expiration value as an ISO 8601 Date. Credentials will be marked expired.");
            credentials.SetExpiration(Aws::Utils::DateTime::Now());
        }
    }
    else
    {
        credentials.SetExpiration((std::chrono::time_point<std::chrono::system_clock>::max)());
    }

    AWS_LOGSTREAM_DEBUG(PROFILE_LOG_TAG, "Successfully pulled credentials from process credential with AccessKey: " << accessKey << ", Expiration:" << credentialsView.GetString("Expiration"));
    return credentials;
}

