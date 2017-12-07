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

#include <aws/core/auth/AWSCredentialsProviderChain.h>

#include <aws/core/utils/memory/AWSMemory.h>

using namespace Aws::Auth;

AWSCredentials AWSCredentialsProviderChain::GetAWSCredentials()
{
    for (auto credentialsProvider : m_providerChain)
    {
        AWSCredentials credentials = credentialsProvider->GetAWSCredentials();
        if (!credentials.GetAWSAccessKeyId().empty() && !credentials.GetAWSSecretKey().empty())
        {
            return credentials;
        }
    }

    return AWSCredentials("", "");
}

static const char* DefaultCredentialsProviderChainTag = "DefaultAWSCredentialsProviderChain";

DefaultAWSCredentialsProviderChain::DefaultAWSCredentialsProviderChain() : AWSCredentialsProviderChain()
{
    AddProvider(Aws::MakeShared<EnvironmentAWSCredentialsProvider>(DefaultCredentialsProviderChainTag));
    AddProvider(Aws::MakeShared<ProfileConfigFileAWSCredentialsProvider>(DefaultCredentialsProviderChainTag));
    AddProvider(Aws::MakeShared<InstanceProfileCredentialsProvider>(DefaultCredentialsProviderChainTag));
}
