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

#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/memory/stl/AWSMap.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/utils/DateTime.h>
#include <aws/core/utils/threading/ReaderWriterLock.h>

namespace Aws
{
    namespace Internal
    {
        class EC2MetadataClient;
    }

    namespace Config
    {
        /**
         * Simple data container for a Profile.
         */
        class Profile
        {
        public:
            inline const Aws::String& GetName() const { return m_name; }
            inline void SetName(const Aws::String& value) { m_name = value; }
            inline const Aws::Auth::AWSCredentials& GetCredentials() const { return m_credentials; }
            inline void SetCredentials(const Aws::Auth::AWSCredentials& value) { m_credentials = value; }
            inline const Aws::String& GetRegion() const { return m_region; }
            inline void SetRegion(const Aws::String& value) { m_region = value; }
            inline const Aws::String& GetRoleArn() const { return m_roleArn; }
            inline void SetRoleArn(const Aws::String& value) { m_roleArn = value; }
            inline const Aws::String& GetExternalId() const { return m_externalId; }
            inline void SetExternalId(const Aws::String& value) { m_externalId = value; }
            inline const Aws::String& GetSourceProfile() const { return m_sourceProfile; }
            inline void SetSourceProfile(const Aws::String& value ) { m_sourceProfile = value; }
            inline const Aws::String& GetCredentialProcess() const { return m_credentialProcess; }
            inline void SetCredentialProcess(const Aws::String& value ) { m_credentialProcess = value; }
            inline void SetAllKeyValPairs(const Aws::Map<Aws::String, Aws::String>& map) { m_allKeyValPairs = map; }
            inline const Aws::String GetValue(const Aws::String& key) const
            {
                auto iter = m_allKeyValPairs.find(key);
                if (iter == m_allKeyValPairs.end()) return {};
                return iter->second;
            }

        private:
            Aws::String m_name;
            Aws::String m_region;
            Aws::Auth::AWSCredentials m_credentials;
            Aws::String m_roleArn;
            Aws::String m_externalId;
            Aws::String m_sourceProfile;
            Aws::String m_credentialProcess;
            Aws::Map<Aws::String, Aws::String> m_allKeyValPairs;
        };

        /**
         * Loads Configuration such as .aws/config, .aws/credentials or ec2 metadata service.
         */
        class AWS_CORE_API AWSProfileConfigLoader
        {
        public:
            virtual ~AWSProfileConfigLoader() = default;

            /**
             * Load the configuration
             */
            bool Load();

            /**
             * Over writes the entire config source with the newly configured profile data.
             */
            bool PersistProfiles(const Aws::Map<Aws::String, Aws::Config::Profile>& profiles);

            /**
             * Gets all profiles from the configuration file.
             */
            inline const Aws::Map<Aws::String, Aws::Config::Profile>& GetProfiles() const { return m_profiles; };

            /**
             * the timestamp from the last time the profile information was loaded from file.
             */
            inline const Aws::Utils::DateTime& LastLoadTime() const { return m_lastLoadTime; }

            using ProfilesContainer = Aws::Map<Aws::String, Aws::Config::Profile>;

        protected:
            /**
             * Subclasses override this method to implement fetching the profiles.
             */
            virtual bool LoadInternal() = 0;

            /**
             * Subclasses override this method to implement persisting the profiles. Default returns false.
             */
            virtual bool PersistInternal(const Aws::Map<Aws::String, Aws::Config::Profile>&) { return false; }

            ProfilesContainer m_profiles;
            Aws::Utils::DateTime m_lastLoadTime;
        };

        /**
         * Reads configuration from a config file (e.g. $HOME/.aws/config or $HOME/.aws/credentials
         */
        class AWS_CORE_API AWSConfigFileProfileConfigLoader : public AWSProfileConfigLoader
        {
        public:
            /**
             * fileName - file to load config from
             * useProfilePrefix - whether or not the profiles are prefixed with "profile", credentials file is not
             * while the config file is. Defaults to off.
             */
            AWSConfigFileProfileConfigLoader(const Aws::String& fileName, bool useProfilePrefix = false);

            virtual ~AWSConfigFileProfileConfigLoader() = default;

            /**
             * File path being used for the config loader.
             */
            const Aws::String& GetFileName() const { return m_fileName; }

            /**
             * Give loader the ability to change the file path to load config from.
             * This can avoid creating new loader object if the file changed.
             */
            void SetFileName(const Aws::String& fileName) { m_fileName = fileName; }

        protected:
            virtual bool LoadInternal() override;
            virtual bool PersistInternal(const Aws::Map<Aws::String, Aws::Config::Profile>&) override;

        private:
            Aws::String m_fileName;
            bool m_useProfilePrefix;
        };

        static const char* const INSTANCE_PROFILE_KEY = "InstanceProfile";

        /**
         * Loads configuration from the EC2 Metadata Service
         */
        class AWS_CORE_API EC2InstanceProfileConfigLoader : public AWSProfileConfigLoader
        {
        public:
            /**
             * If client is nullptr, the default EC2MetadataClient will be created.
             */
            EC2InstanceProfileConfigLoader(const std::shared_ptr<Aws::Internal::EC2MetadataClient>& = nullptr);

            virtual ~EC2InstanceProfileConfigLoader() = default;

        protected:
            virtual bool LoadInternal() override;

        private:
            std::shared_ptr<Aws::Internal::EC2MetadataClient> m_ec2metadataClient;
        };

        /**
         * Stores the contents of config file and credentials file to avoid multiple file readings.
         * At the same time provides the flexibility to reload from file.
         */
        class AWS_CORE_API ConfigAndCredentialsCacheManager
        {
        public:
            ConfigAndCredentialsCacheManager();

            void ReloadConfigFile();

            void ReloadCredentialsFile();

            bool HasConfigProfile(const Aws::String& profileName) const;

            /**
             * Returns cached config profile with the specified profile name.
             * Using copy instead of const reference to avoid reading bad contents due to thread contention.
             */
            Aws::Config::Profile GetConfigProfile(const Aws::String& profileName) const;

            /**
             * Returns cached config profiles
             * Using copy instead of const reference to avoid reading bad contents due to thread contention.
             */
            Aws::Map<Aws::String, Aws::Config::Profile> GetConfigProfiles() const;

            /**
             * Returns cached config value with the specified profile name and key.
             * Using copy instead of const reference to avoid reading bad contents due to thread contention.
             */
            Aws::String GetConfig(const Aws::String& profileName, const Aws::String& key) const;

            bool HasCredentialsProfile(const Aws::String& profileName) const;
            /**
             * Returns cached credentials profile with the specified profile name.
             * Using copy instead of const reference to avoid reading bad contents due to thread contention.
             */
            Aws::Config::Profile GetCredentialsProfile(const Aws::String& profileName) const;

            /**
             * Returns cached credentials with the specified profile name.
             * Using copy instead of const reference to avoid reading bad contents due to thread contention.
             */
            Aws::Auth::AWSCredentials GetCredentials(const Aws::String& profileName) const;

        private:
            mutable Aws::Utils::Threading::ReaderWriterLock m_credentialsLock;
            Aws::Config::AWSConfigFileProfileConfigLoader m_credentialsFileLoader;
            mutable Aws::Utils::Threading::ReaderWriterLock m_configLock;
            Aws::Config::AWSConfigFileProfileConfigLoader m_configFileLoader;
        };

        void AWS_CORE_API InitConfigAndCredentialsCacheManager();

        void AWS_CORE_API CleanupConfigAndCredentialsCacheManager();

        void AWS_CORE_API ReloadCachedConfigFile();

        void AWS_CORE_API ReloadCachedCredentialsFile();

        bool AWS_CORE_API HasCachedConfigProfile(const Aws::String& profileName);

        Aws::Config::Profile AWS_CORE_API GetCachedConfigProfile(const Aws::String& profileName);

        Aws::Map<Aws::String, Aws::Config::Profile> AWS_CORE_API GetCachedConfigProfiles();

        Aws::String AWS_CORE_API GetCachedConfigValue(const Aws::String& profileName, const Aws::String& key);

        Aws::String AWS_CORE_API GetCachedConfigValue(const Aws::String& key);

        bool AWS_CORE_API HasCachedCredentialsProfile(const Aws::String &profileName);

        Aws::Config::Profile AWS_CORE_API GetCachedCredentialsProfile(const Aws::String& profileName);

        Aws::Auth::AWSCredentials AWS_CORE_API GetCachedCredentials(const Aws::String& profileName);
    }
}
