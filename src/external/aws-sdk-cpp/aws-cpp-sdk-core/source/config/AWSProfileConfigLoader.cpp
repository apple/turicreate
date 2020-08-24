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

#include <aws/core/config/AWSProfileConfigLoader.h>
#include <aws/core/internal/AWSHttpResourceClient.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/utils/memory/stl/AWSList.h>
#include <aws/core/utils/memory/stl/AWSStreamFwd.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <fstream>

namespace Aws
{
    namespace Config
    {
        using namespace Aws::Utils;
        using namespace Aws::Auth;

        static const char* const CONFIG_LOADER_TAG = "Aws::Config::AWSProfileConfigLoader";
        #ifdef _MSC_VER
            // VS2015 compiler's bug, warning s_CoreErrorsMapper: symbol will be dynamically initialized (implementation limitation)
            AWS_SUPPRESS_WARNING(4592,
                static Aws::UniquePtr<ConfigAndCredentialsCacheManager> s_configManager(nullptr);
            )
        #else
            static Aws::UniquePtr<ConfigAndCredentialsCacheManager> s_configManager(nullptr);
        #endif

        static const char CONFIG_CREDENTIALS_CACHE_MANAGER_TAG[] = "ConfigAndCredentialsCacheManager";

        bool AWSProfileConfigLoader::Load()
        {
            if(LoadInternal())
            {
                AWS_LOGSTREAM_INFO(CONFIG_LOADER_TAG, "Successfully reloaded configuration.");
                m_lastLoadTime = DateTime::Now();
                AWS_LOGSTREAM_TRACE(CONFIG_LOADER_TAG, "reloaded config at "
                        << m_lastLoadTime.ToGmtString(DateFormat::ISO_8601));
                return true;
            }

            AWS_LOGSTREAM_INFO(CONFIG_LOADER_TAG, "Failed to reload configuration.");
            return false;
        }

        bool AWSProfileConfigLoader::PersistProfiles(const Aws::Map<Aws::String, Profile>& profiles)
        {
            if(PersistInternal(profiles))
            {
                AWS_LOGSTREAM_INFO(CONFIG_LOADER_TAG, "Successfully persisted configuration.");
                m_profiles = profiles;
                m_lastLoadTime = DateTime::Now();
                AWS_LOGSTREAM_TRACE(CONFIG_LOADER_TAG, "persisted config at "
                        << m_lastLoadTime.ToGmtString(DateFormat::ISO_8601));
                return true;
            }

            AWS_LOGSTREAM_WARN(CONFIG_LOADER_TAG, "Failed to persist configuration.");
            return false;
        }

        static const char REGION_KEY[]                       = "region";
        static const char ACCESS_KEY_ID_KEY[]                = "aws_access_key_id";
        static const char SECRET_KEY_KEY[]                   = "aws_secret_access_key";
        static const char SESSION_TOKEN_KEY[]                = "aws_session_token";
        static const char ROLE_ARN_KEY[]                     = "role_arn";
        static const char EXTERNAL_ID_KEY[]                  = "external_id";
        static const char CREDENTIAL_PROCESS_COMMAND[]       = "credential_process";
        static const char SOURCE_PROFILE_KEY[]               = "source_profile";
        static const char PROFILE_PREFIX[]                   = "profile ";
        static const char EQ                                 = '=';
        static const char LEFT_BRACKET                       = '[';
        static const char RIGHT_BRACKET                      = ']';
        static const char PARSER_TAG[]                       = "Aws::Config::ConfigFileProfileFSM";

        class ConfigFileProfileFSM
        {
        public:
            ConfigFileProfileFSM() : m_parserState(START) {}

            const Aws::Map<String, Profile>& GetProfiles() const { return m_foundProfiles; }

            void ParseStream(Aws::IStream& stream)
            {
                static const size_t ASSUME_EMPTY_LEN = 3;

                Aws::String line;
                while(std::getline(stream, line) && m_parserState != FAILURE)
                {
                    if (line.empty() || line.length() < ASSUME_EMPTY_LEN)
                    {
                        continue;
                    }

                    auto openPos = line.find(LEFT_BRACKET);
                    auto closePos = line.find(RIGHT_BRACKET);

                    switch(m_parserState)
                    {

                    case START:
                        if(openPos != std::string::npos && closePos != std::string::npos)
                        {
                            FlushProfileAndReset(line, openPos, closePos);
                            m_parserState = PROFILE_FOUND;
                        }
                        break;

                    //fallthrough here is intentional to reduce duplicate logic
                    case PROFILE_KEY_VALUE_FOUND:
                        if(openPos != std::string::npos && closePos != std::string::npos)
                        {
                            m_parserState = PROFILE_FOUND;
                            FlushProfileAndReset(line, openPos, closePos);
                            break;
                        }
                        // fall through
                    case PROFILE_FOUND:
                    {
                        auto equalsPos = line.find(EQ);
                        if (equalsPos != std::string::npos)
                        {
                            auto key = line.substr(0, equalsPos);
                            auto value = line.substr(equalsPos + 1);
                            m_profileKeyValuePairs[StringUtils::Trim(key.c_str())] =
                                    StringUtils::Trim(value.c_str());
                            m_parserState = PROFILE_KEY_VALUE_FOUND;
                        }

                        break;
                    }
                    default:
                        m_parserState = FAILURE;
                        break;
                    }
                }

                FlushProfileAndReset(line, std::string::npos, std::string::npos);
            }

        private:

            void FlushProfileAndReset(Aws::String& line, size_t openPos, size_t closePos)
            {
                if(!m_currentWorkingProfile.empty() && !m_profileKeyValuePairs.empty())
                {
                    Profile profile;
                    profile.SetName(m_currentWorkingProfile);

                    auto regionIter = m_profileKeyValuePairs.find(REGION_KEY);
                    if (regionIter != m_profileKeyValuePairs.end())
                    {
                        AWS_LOGSTREAM_DEBUG(PARSER_TAG, "found region " << regionIter->second);
                        profile.SetRegion(regionIter->second);
                    }

                    auto accessKeyIdIter = m_profileKeyValuePairs.find(ACCESS_KEY_ID_KEY);
                    Aws::String accessKey, secretKey, sessionToken;
                    if (accessKeyIdIter != m_profileKeyValuePairs.end())
                    {
                        accessKey = accessKeyIdIter->second;
                        AWS_LOGSTREAM_DEBUG(PARSER_TAG, "found access key " << accessKey);

                        auto secretAccessKeyIter = m_profileKeyValuePairs.find(SECRET_KEY_KEY);
                        auto sessionTokenIter = m_profileKeyValuePairs.find(SESSION_TOKEN_KEY);
                        if (secretAccessKeyIter != m_profileKeyValuePairs.end())
                        {
                            secretKey = secretAccessKeyIter->second;
                        }
                        else
                        {
                            AWS_LOGSTREAM_ERROR(PARSER_TAG, "No secret access key found even though an access key was specified. This will cause all signed AWS calls to fail.");
                        }

                        if (sessionTokenIter != m_profileKeyValuePairs.end())
                        {
                            sessionToken = sessionTokenIter->second;
                        }

                        profile.SetCredentials(Aws::Auth::AWSCredentials(accessKey, secretKey, sessionToken));
                    }

                    auto assumeRoleArnIter = m_profileKeyValuePairs.find(ROLE_ARN_KEY);
                    if (assumeRoleArnIter != m_profileKeyValuePairs.end())
                    {
                        AWS_LOGSTREAM_DEBUG(PARSER_TAG, "found role arn " << assumeRoleArnIter->second);
                        profile.SetRoleArn(assumeRoleArnIter->second);
                    }

                    auto externalIdIter = m_profileKeyValuePairs.find(EXTERNAL_ID_KEY);
                    if (externalIdIter != m_profileKeyValuePairs.end())
                    {
                        AWS_LOGSTREAM_DEBUG(PARSER_TAG, "found external id " << externalIdIter->second);
                        profile.SetExternalId(externalIdIter->second);
                    }

                    auto sourceProfileIter = m_profileKeyValuePairs.find(SOURCE_PROFILE_KEY);
                    if (sourceProfileIter != m_profileKeyValuePairs.end())
                    {
                        AWS_LOGSTREAM_DEBUG(PARSER_TAG, "found source profile " << sourceProfileIter->second);
                        profile.SetSourceProfile(sourceProfileIter->second);
                    }

                    auto credentialProcessIter = m_profileKeyValuePairs.find(CREDENTIAL_PROCESS_COMMAND);
                    if (credentialProcessIter != m_profileKeyValuePairs.end())
                    {
                        AWS_LOGSTREAM_DEBUG(PARSER_TAG, "found credential process " << credentialProcessIter->second);
                        profile.SetCredentialProcess(credentialProcessIter->second);
                    }
                    profile.SetAllKeyValPairs(m_profileKeyValuePairs);

                    m_foundProfiles[profile.GetName()] = std::move(profile);
                    m_currentWorkingProfile.clear();
                    m_profileKeyValuePairs.clear();
                }

                if(!line.empty() && openPos != std::string::npos && closePos != std::string::npos)
                {
                    m_currentWorkingProfile = StringUtils::Trim(line.substr(openPos + 1, closePos - openPos - 1).c_str());
                    StringUtils::Replace(m_currentWorkingProfile, PROFILE_PREFIX, "");
                    AWS_LOGSTREAM_DEBUG(PARSER_TAG, "found profile " << m_currentWorkingProfile);
                }
            }

            enum State
            {
                START = 0,
                PROFILE_FOUND,
                PROFILE_KEY_VALUE_FOUND,
                FAILURE
            };

            Aws::String m_currentWorkingProfile;
            Aws::Map<String, String> m_profileKeyValuePairs;
            State m_parserState;
            Aws::Map<String, Profile> m_foundProfiles;
        };

        static const char* const CONFIG_FILE_LOADER = "Aws::Config::AWSConfigFileProfileConfigLoader";

        AWSConfigFileProfileConfigLoader::AWSConfigFileProfileConfigLoader(const Aws::String& fileName, bool useProfilePrefix) :
                m_fileName(fileName), m_useProfilePrefix(useProfilePrefix)
        {
            AWS_LOGSTREAM_INFO(CONFIG_FILE_LOADER, "Initializing config loader against fileName "
                    << fileName << " and using profilePrefix = " << useProfilePrefix);
        }

        bool AWSConfigFileProfileConfigLoader::LoadInternal()
        {
            m_profiles.clear();

            Aws::IFStream inputFile(m_fileName.c_str());
            if(inputFile)
            {
                ConfigFileProfileFSM parser;
                parser.ParseStream(inputFile);
                m_profiles = parser.GetProfiles();
                return m_profiles.size() > 0;
            }

            AWS_LOGSTREAM_INFO(CONFIG_FILE_LOADER, "Unable to open config file " << m_fileName << " for reading.");

            return false;
        }

        bool AWSConfigFileProfileConfigLoader::PersistInternal(const Aws::Map<Aws::String, Profile>& profiles)
        {
            Aws::OFStream outputFile(m_fileName.c_str(), std::ios_base::out | std::ios_base::trunc);
            if(outputFile)
            {
                for(auto& profile : profiles)
                {
                    Aws::String prefix = m_useProfilePrefix ? PROFILE_PREFIX : "";

                    AWS_LOGSTREAM_DEBUG(CONFIG_FILE_LOADER, "Writing profile " << profile.first << " to disk.");

                    outputFile << LEFT_BRACKET << prefix << profile.second.GetName() << RIGHT_BRACKET << std::endl;
                    const Aws::Auth::AWSCredentials& credentials = profile.second.GetCredentials();
                    outputFile << ACCESS_KEY_ID_KEY << EQ << credentials.GetAWSAccessKeyId() << std::endl;
                    outputFile << SECRET_KEY_KEY << EQ << credentials.GetAWSSecretKey() << std::endl;

                    if(!credentials.GetSessionToken().empty())
                    {
                        outputFile << SESSION_TOKEN_KEY << EQ << credentials.GetSessionToken() << std::endl;
                    }

                    if(!profile.second.GetRegion().empty())
                    {
                        outputFile << REGION_KEY << EQ << profile.second.GetRegion() << std::endl;
                    }

                    if(!profile.second.GetRoleArn().empty())
                    {
                        outputFile << ROLE_ARN_KEY << EQ << profile.second.GetRoleArn() << std::endl;
                    }

                    if(!profile.second.GetSourceProfile().empty())
                    {
                        outputFile << SOURCE_PROFILE_KEY << EQ << profile.second.GetSourceProfile() << std::endl;
                    }

                    outputFile << std::endl;
                }

                AWS_LOGSTREAM_INFO(CONFIG_FILE_LOADER, "Profiles written to config file " << m_fileName);

                return true;
            }

            AWS_LOGSTREAM_WARN(CONFIG_FILE_LOADER, "Unable to open config file " << m_fileName << " for writing.");

            return false;
        }

        static const char* const EC2_INSTANCE_PROFILE_LOG_TAG = "Aws::Config::EC2InstanceProfileConfigLoader";

        EC2InstanceProfileConfigLoader::EC2InstanceProfileConfigLoader(const std::shared_ptr<Aws::Internal::EC2MetadataClient>& client)
            : m_ec2metadataClient(client == nullptr ? Aws::MakeShared<Aws::Internal::EC2MetadataClient>(EC2_INSTANCE_PROFILE_LOG_TAG) : client)
        {
        }

        bool EC2InstanceProfileConfigLoader::LoadInternal()
        {
            auto credentialsStr = m_ec2metadataClient->GetDefaultCredentialsSecurely();
            if(credentialsStr.empty()) return false;

            Json::JsonValue credentialsDoc(credentialsStr);
            if (!credentialsDoc.WasParseSuccessful())
            {
                AWS_LOGSTREAM_ERROR(EC2_INSTANCE_PROFILE_LOG_TAG,
                        "Failed to parse output from EC2MetadataService.");
                return false;
            }
            const char* accessKeyId = "AccessKeyId";
            const char* secretAccessKey = "SecretAccessKey";
            Aws::String accessKey, secretKey, token;

            auto credentialsView = credentialsDoc.View();
            accessKey = credentialsView.GetString(accessKeyId);
            AWS_LOGSTREAM_INFO(EC2_INSTANCE_PROFILE_LOG_TAG,
                    "Successfully pulled credentials from metadata service with access key " << accessKey);

            secretKey = credentialsView.GetString(secretAccessKey);
            token = credentialsView.GetString("Token");

            auto region = m_ec2metadataClient->GetCurrentRegion();

            Profile profile;
            profile.SetCredentials(AWSCredentials(accessKey, secretKey, token));
            profile.SetRegion(region);
            profile.SetName(INSTANCE_PROFILE_KEY);

            m_profiles[INSTANCE_PROFILE_KEY] = profile;

            return true;
        }

        ConfigAndCredentialsCacheManager::ConfigAndCredentialsCacheManager() :
            m_credentialsFileLoader(Aws::Auth::ProfileConfigFileAWSCredentialsProvider::GetCredentialsProfileFilename()),
            m_configFileLoader(Aws::Auth::GetConfigProfileFilename(), true/*use profile prefix*/)
        {
            ReloadCredentialsFile();
            ReloadConfigFile();
        }

        void ConfigAndCredentialsCacheManager::ReloadConfigFile()
        {
            Aws::Utils::Threading::WriterLockGuard guard(m_configLock);
            m_configFileLoader.SetFileName(Aws::Auth::GetConfigProfileFilename());
            m_configFileLoader.Load();
        }

        void ConfigAndCredentialsCacheManager::ReloadCredentialsFile()
        {
            Aws::Utils::Threading::WriterLockGuard guard(m_credentialsLock);
            m_credentialsFileLoader.SetFileName(Aws::Auth::ProfileConfigFileAWSCredentialsProvider::GetCredentialsProfileFilename());
            m_credentialsFileLoader.Load();
        }

        bool ConfigAndCredentialsCacheManager::HasConfigProfile(const Aws::String& profileName) const
        {
            Aws::Utils::Threading::ReaderLockGuard guard(m_configLock);
            return (m_configFileLoader.GetProfiles().count(profileName) == 1);
        }

        Aws::Config::Profile ConfigAndCredentialsCacheManager::GetConfigProfile(const Aws::String& profileName) const
        {
            Aws::Utils::Threading::ReaderLockGuard guard(m_configLock);
            const auto& profiles = m_configFileLoader.GetProfiles();
            const auto &iter = profiles.find(profileName);
            if (iter == profiles.end())
            {
                return {};
            }
            return iter->second;
        }

        Aws::Map<Aws::String, Aws::Config::Profile> ConfigAndCredentialsCacheManager::GetConfigProfiles() const
        {
            Aws::Utils::Threading::ReaderLockGuard guard(m_configLock);
            return m_configFileLoader.GetProfiles();
        }

        Aws::String ConfigAndCredentialsCacheManager::GetConfig(const Aws::String& profileName, const Aws::String& key) const
        {
            Aws::Utils::Threading::ReaderLockGuard guard(m_configLock);
            const auto& profiles = m_configFileLoader.GetProfiles();
            const auto &iter = profiles.find(profileName);
            if (iter == profiles.end())
            {
                return {};
            }
            return iter->second.GetValue(key);
        }

        bool ConfigAndCredentialsCacheManager::HasCredentialsProfile(const Aws::String& profileName) const
        {
            Aws::Utils::Threading::ReaderLockGuard guard(m_credentialsLock);
            return (m_credentialsFileLoader.GetProfiles().count(profileName) == 1);
        }

        Aws::Config::Profile ConfigAndCredentialsCacheManager::GetCredentialsProfile(const Aws::String& profileName) const
        {
            Aws::Utils::Threading::ReaderLockGuard guard(m_credentialsLock);
            const auto &profiles = m_credentialsFileLoader.GetProfiles();
            const auto &iter = profiles.find(profileName);
            if (iter == profiles.end())
            {
                return {};
            }
            return iter->second;
        }

        Aws::Auth::AWSCredentials ConfigAndCredentialsCacheManager::GetCredentials(const Aws::String& profileName) const
        {
            Aws::Utils::Threading::ReaderLockGuard guard(m_credentialsLock);
            const auto& profiles = m_credentialsFileLoader.GetProfiles();
            const auto &iter = profiles.find(profileName);
            if (iter == profiles.end())
            {
                return {};
            }
            return iter->second.GetCredentials();
        }

        void InitConfigAndCredentialsCacheManager()
        {
            if (s_configManager)
            {
                return;
            }
            s_configManager = Aws::MakeUnique<ConfigAndCredentialsCacheManager>(CONFIG_CREDENTIALS_CACHE_MANAGER_TAG);
        }

        void CleanupConfigAndCredentialsCacheManager()
        {
            if (!s_configManager)
            {
                return;
            }
            s_configManager = nullptr;
        }

        void ReloadCachedConfigFile()
        {
            assert(s_configManager);
            s_configManager->ReloadConfigFile();
        }

        void ReloadCachedCredentialsFile()
        {
            assert(s_configManager);
            s_configManager->ReloadCredentialsFile();
        }

        bool HasCachedConfigProfile(const Aws::String& profileName)
        {
            assert(s_configManager);
            return s_configManager->HasConfigProfile(profileName);
        }

        Aws::Config::Profile GetCachedConfigProfile(const Aws::String& profileName)
        {
            assert(s_configManager);
            return s_configManager->GetConfigProfile(profileName);
        }

        Aws::Map<Aws::String, Aws::Config::Profile> GetCachedConfigProfiles()
        {
            assert(s_configManager);
            return s_configManager->GetConfigProfiles();
        }

        Aws::String GetCachedConfigValue(const Aws::String &profileName, const Aws::String &key)
        {
            assert(s_configManager);
            return s_configManager->GetConfig(profileName, key);
        }

        Aws::String GetCachedConfigValue(const Aws::String &key)
        {
            assert(s_configManager);
            return s_configManager->GetConfig(Aws::Auth::GetConfigProfileName(), key);
        }

        bool HasCachedCredentialsProfile(const Aws::String& profileName)
        {
            assert(s_configManager);
            return s_configManager->HasCredentialsProfile(profileName);
        }

        Aws::Config::Profile GetCachedCredentialsProfile(const Aws::String &profileName)
        {
            assert(s_configManager);
            return s_configManager->GetCredentialsProfile(profileName);
        }

        Aws::Auth::AWSCredentials GetCachedCredentials(const Aws::String &profileName)
        {
            assert(s_configManager);
            return s_configManager->GetCredentials(profileName);
        }
    } // Config namespace
} // Aws namespace
