/*
  * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/FileSystemUtils.h>

#include <cstdlib>
#include <fstream>
#include <string.h>


using namespace Aws::Utils;
using namespace Aws::Utils::Logging;
using namespace Aws::Auth;
using namespace Aws::Internal;


static const char* ACCESS_KEY_ENV_VARIABLE = "AWS_ACCESS_KEY_ID";
static const char* SECRET_KEY_ENV_VAR = "AWS_SECRET_ACCESS_KEY";
static const char* SESSION_TOKEN_ENV_VARIABLE = "AWS_SESSION_TOKEN";
static const char* DEFAULT_PROFILE = "default";
static const char* AWS_PROFILE_ENVIRONMENT_VARIABLE = "AWS_DEFAULT_PROFILE";

static const char* AWS_CREDENTIAL_PROFILES_FILE = "AWS_SHARED_CREDENTIALS_FILE";

static const char* PROFILE_DEFAULT_FILENAME = "credentials";
static const char* CONFIG_FILENAME = "config";

#ifndef _WIN32
static const char* PROFILE_DIRECTORY = "/.aws";
static const char* DIRECTORY_JOIN = "/";

#else
    static const char* PROFILE_DIRECTORY = "\\.aws";
    static const char* DIRECTORY_JOIN = "\\";
#endif // _WIN32


bool AWSCredentialsProvider::IsTimeToRefresh(long reloadFrequency)
{
    if (DateTime::Now().Millis() - m_lastLoadedMs > reloadFrequency)
    {
        m_lastLoadedMs = DateTime::Now().Millis();
        return true;
    }

    return false;
}


static const char* environmentLogTag = "EnvironmentAWSCredentialsProvider";


AWSCredentials EnvironmentAWSCredentialsProvider::GetAWSCredentials()
{
    auto accessKey = Aws::Environment::GetEnv(ACCESS_KEY_ENV_VARIABLE);
    AWSCredentials credentials("", "", "");

    if (!accessKey.empty())
    {
        credentials.SetAWSAccessKeyId(accessKey);

        AWS_LOGSTREAM_INFO(environmentLogTag, "Found credential in environment with access key id " << accessKey);
        auto secretKey = Aws::Environment::GetEnv(SECRET_KEY_ENV_VAR);

        if (!secretKey.empty())
        {
            credentials.SetAWSSecretKey(secretKey);
            AWS_LOG_INFO(environmentLogTag, "Found secret key");
        }

        auto sessionToken = Aws::Environment::GetEnv(SESSION_TOKEN_ENV_VARIABLE);

        if(!sessionToken.empty())
        {
            credentials.SetSessionToken(sessionToken);
            AWS_LOG_INFO(environmentLogTag, "Found sessionToken");
        }
    }

    return credentials;
}

static Aws::String GetBaseDirectory()
{
    return Aws::FileSystem::GetHomeDirectory();
}

Aws::String ProfileConfigFileAWSCredentialsProvider::GetConfigProfileFilename()
{
    return GetBaseDirectory() + PROFILE_DIRECTORY + DIRECTORY_JOIN + CONFIG_FILENAME;
}

Aws::String ProfileConfigFileAWSCredentialsProvider::GetCredentialsProfileFilename()
{
    auto profileFileNameFromVar = Aws::Environment::GetEnv(AWS_CREDENTIAL_PROFILES_FILE);

    if (!profileFileNameFromVar.empty())
    {
        return profileFileNameFromVar;
    }
    else
    {
        return GetBaseDirectory() + PROFILE_DIRECTORY + DIRECTORY_JOIN + PROFILE_DEFAULT_FILENAME;
    }
}

Aws::String ProfileConfigFileAWSCredentialsProvider::GetProfileDirectory()
{
    Aws::String profileFileName = GetCredentialsProfileFilename();
    auto lastSeparator = profileFileName.find_last_of(DIRECTORY_JOIN);
    if (lastSeparator != std::string::npos)
    {
        return profileFileName.substr(0, lastSeparator);
    }
    else
    {
        return "";
    }
}

static const char* profileLogTag = "ProfileConfigFileAWSCredentialsProvider";


ProfileConfigFileAWSCredentialsProvider::ProfileConfigFileAWSCredentialsProvider(long refreshRateMs) :
        m_configFileLoader(Aws::MakeShared<Aws::Config::AWSConfigFileProfileConfigLoader>(profileLogTag, GetConfigProfileFilename(), true)),
        m_credentialsFileLoader(Aws::MakeShared<Aws::Config::AWSConfigFileProfileConfigLoader>(profileLogTag, GetCredentialsProfileFilename())),
        m_loadFrequencyMs(refreshRateMs)
{
    auto profileFromVar = Aws::Environment::GetEnv(AWS_PROFILE_ENVIRONMENT_VARIABLE);
    if (!profileFromVar.empty())
    {
        m_profileToUse = profileFromVar;
    }
    else
    {
        m_profileToUse = DEFAULT_PROFILE;
    }

    AWS_LOGSTREAM_INFO(profileLogTag, "Setting provider to read credentials from " <<  GetCredentialsProfileFilename() << " for credentials file"
                                      << " and " <<  GetConfigProfileFilename() << " for the config file "
                                      << ", for use with profile " << m_profileToUse);
}

ProfileConfigFileAWSCredentialsProvider::ProfileConfigFileAWSCredentialsProvider(const char* profile, long refreshRateMs) :
        m_profileToUse(profile),
        m_configFileLoader(Aws::MakeShared<Aws::Config::AWSConfigFileProfileConfigLoader>(profileLogTag, GetConfigProfileFilename(), true)),
        m_credentialsFileLoader(Aws::MakeShared<Aws::Config::AWSConfigFileProfileConfigLoader>(profileLogTag, GetCredentialsProfileFilename())),
        m_loadFrequencyMs(refreshRateMs)
{
    AWS_LOGSTREAM_INFO(profileLogTag, "Setting provider to read credentials from " <<  GetCredentialsProfileFilename() << " for credentials file"
                                      << " and " <<  GetConfigProfileFilename() << " for the config file "
                                      << ", for use with profile " << m_profileToUse);
}

AWSCredentials ProfileConfigFileAWSCredentialsProvider::GetAWSCredentials()
{
    RefreshIfExpired();
    auto credsFileProfileIter = m_credentialsFileLoader->GetProfiles().find(m_profileToUse);

    if(credsFileProfileIter != m_credentialsFileLoader->GetProfiles().end())
    {
        return credsFileProfileIter->second.GetCredentials();
    }

    auto configFileProfileIter = m_configFileLoader->GetProfiles().find(m_profileToUse);
    if(configFileProfileIter != m_configFileLoader->GetProfiles().end())
    {
        return configFileProfileIter->second.GetCredentials();
    }

    return AWSCredentials();
}


void ProfileConfigFileAWSCredentialsProvider::RefreshIfExpired()
{
    std::lock_guard<std::mutex> locker(m_reloadMutex);

    if (IsTimeToRefresh(m_loadFrequencyMs))
    {
        //fall-back to config file.
        if(!m_credentialsFileLoader->Load())
        {
            m_configFileLoader->Load();
        }
    }
}

static const char* instanceLogTag = "InstanceProfileCredentialsProvider";

InstanceProfileCredentialsProvider::InstanceProfileCredentialsProvider(long refreshRateMs) :
        m_ec2MetadataConfigLoader(Aws::MakeShared<Aws::Config::EC2InstanceProfileConfigLoader>(instanceLogTag)),
        m_loadFrequencyMs(refreshRateMs)
{
    AWS_LOGSTREAM_INFO(instanceLogTag, "Creating Instance with default EC2MetadataClient and refresh rate " << refreshRateMs);
}


InstanceProfileCredentialsProvider::InstanceProfileCredentialsProvider(const std::shared_ptr<Aws::Config::EC2InstanceProfileConfigLoader>& loader,
                                                                       long refreshRateMs) :
        m_ec2MetadataConfigLoader(loader),
        m_loadFrequencyMs(refreshRateMs)
{
    AWS_LOGSTREAM_INFO(instanceLogTag, "Creating Instance with injected EC2MetadataClient and refresh rate " << refreshRateMs);
}


AWSCredentials InstanceProfileCredentialsProvider::GetAWSCredentials()
{
    RefreshIfExpired();
    auto profileIter = m_ec2MetadataConfigLoader->GetProfiles().find(Aws::Config::INSTANCE_PROFILE_KEY);

    if(profileIter != m_ec2MetadataConfigLoader->GetProfiles().end())
    {
        return profileIter->second.GetCredentials();
    }

    return AWSCredentials();
}


void InstanceProfileCredentialsProvider::RefreshIfExpired()
{
    AWS_LOG_DEBUG(instanceLogTag, "Checking if latest credential pull has expired.");

    std::lock_guard<std::mutex> locker(m_reloadMutex);
    if (IsTimeToRefresh(m_loadFrequencyMs))
    {
        AWS_LOG_INFO(instanceLogTag, "Credentials have expired attempting to repull from EC2 Metadata Service.");
        m_ec2MetadataConfigLoader->Load();
    }
}


