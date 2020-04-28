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

#include <aws/core/internal/AWSHttpResourceClient.h>
#include <aws/core/client/DefaultRetryStrategy.h>
#include <aws/core/http/HttpClient.h>
#include <aws/core/http/HttpClientFactory.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/platform/Environment.h>
#include <aws/core/client/AWSError.h>
#include <aws/core/client/CoreErrors.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <mutex>
#include <sstream>

using namespace Aws;
using namespace Aws::Utils;
using namespace Aws::Utils::Logging;
using namespace Aws::Utils::Xml;
using namespace Aws::Http;
using namespace Aws::Client;
using namespace Aws::Internal;

static const char EC2_SECURITY_CREDENTIALS_RESOURCE[] = "/latest/meta-data/iam/security-credentials";
static const char EC2_REGION_RESOURCE[] = "/latest/meta-data/placement/availability-zone";
static const char EC2_IMDS_TOKEN_RESOURCE[] = "/latest/api/token";
static const char EC2_IMDS_TOKEN_TTL_DEFAULT_VALUE[] = "21600";
static const char EC2_IMDS_TOKEN_TTL_HEADER[] = "x-aws-ec2-metadata-token-ttl-seconds";
static const char EC2_IMDS_TOKEN_HEADER[] = "x-aws-ec2-metadata-token";
static const char RESOURCE_CLIENT_CONFIGURATION_ALLOCATION_TAG[] = "AWSHttpResourceClient";
static const char EC2_METADATA_CLIENT_LOG_TAG[] = "EC2MetadataClient";
static const char ECS_CREDENTIALS_CLIENT_LOG_TAG[] = "ECSCredentialsClient";

namespace Aws
{
    namespace Client
    {
        Aws::String ComputeUserAgentString();
    }
}

static ClientConfiguration MakeDefaultHttpResourceClientConfiguration(const char *logtag)
{
    ClientConfiguration res;

    res.maxConnections = 2;
    res.scheme = Scheme::HTTP;

#if defined(WIN32) && defined(BYPASS_DEFAULT_PROXY)
    // For security reasons, we must bypass any proxy settings when fetching sensitive information, for example
    // user credentials. On Windows, IXMLHttpRequest2 does not support bypassing proxy settings, therefore,
    // we force using WinHTTP client. On POSIX systems, CURL is set to bypass proxy settings by default.
    res.httpLibOverride = TransferLibType::WIN_HTTP_CLIENT;
    AWS_LOGSTREAM_INFO(logtag, "Overriding the current HTTP client to WinHTTP to bypass proxy settings.");
#else
    (void) logtag;  // To disable warning about unused variable
#endif
    // Explicitly set the proxy settings to empty/zero to avoid relying on defaults that could potentially change
    // in the future.
    res.proxyHost = "";
    res.proxyUserName = "";
    res.proxyPassword = "";
    res.proxyPort = 0;

    // EC2MetadataService throttles by delaying the response so the service client should set a large read timeout.
    // EC2MetadataService delay is in order of seconds so it only make sense to retry after a couple of seconds.
    res.connectTimeoutMs = 1000;
    res.requestTimeoutMs = 1000;
    res.retryStrategy = Aws::MakeShared<DefaultRetryStrategy>(RESOURCE_CLIENT_CONFIGURATION_ALLOCATION_TAG, 1, 1000);

    return res;
}

AWSHttpResourceClient::AWSHttpResourceClient(const Aws::Client::ClientConfiguration& clientConfiguration, const char* logtag)
: m_logtag(logtag), m_retryStrategy(clientConfiguration.retryStrategy), m_httpClient(nullptr)
{
    AWS_LOGSTREAM_INFO(m_logtag.c_str(),
                       "Creating AWSHttpResourceClient with max connections "
                        << clientConfiguration.maxConnections
                        << " and scheme "
                        << SchemeMapper::ToString(clientConfiguration.scheme));

    m_httpClient = CreateHttpClient(clientConfiguration);
}

AWSHttpResourceClient::AWSHttpResourceClient(const char* logtag)
: AWSHttpResourceClient(MakeDefaultHttpResourceClientConfiguration(logtag), logtag)
{
}

AWSHttpResourceClient::~AWSHttpResourceClient()
{
}

Aws::String AWSHttpResourceClient::GetResource(const char* endpoint, const char* resource, const char* authToken) const
{
    return GetResourceWithAWSWebServiceResult(endpoint, resource, authToken).GetPayload();
}

AmazonWebServiceResult<Aws::String> AWSHttpResourceClient::GetResourceWithAWSWebServiceResult(const char *endpoint, const char *resource, const char *authToken) const
{
    Aws::StringStream ss;
    ss << endpoint << resource;
    std::shared_ptr<HttpRequest> request(CreateHttpRequest(ss.str(), HttpMethod::HTTP_GET,
                                                           Aws::Utils::Stream::DefaultResponseStreamFactoryMethod));

    request->SetUserAgent(ComputeUserAgentString());

    if (authToken)
    {
        request->SetHeaderValue(Aws::Http::AWS_AUTHORIZATION_HEADER, authToken);
    }

    return GetResourceWithAWSWebServiceResult(request);
}

AmazonWebServiceResult<Aws::String> AWSHttpResourceClient::GetResourceWithAWSWebServiceResult(const std::shared_ptr<HttpRequest> &httpRequest) const
{
    AWS_LOGSTREAM_TRACE(m_logtag.c_str(), "Retrieving credentials from " << httpRequest->GetURIString());

    for (long retries = 0;; retries++)
    {
        std::shared_ptr<HttpResponse> response(m_httpClient->MakeRequest(httpRequest));

        if (response->GetResponseCode() == HttpResponseCode::OK)
        {
            Aws::IStreamBufIterator eos;
            return {Aws::String(Aws::IStreamBufIterator(response->GetResponseBody()), eos), response->GetHeaders(), HttpResponseCode::OK};
        }

        const Aws::Client::AWSError<Aws::Client::CoreErrors> error = [this, &response]() {
            if (response->HasClientError() || response->GetResponseBody().tellp() < 1)
            {
                AWS_LOGSTREAM_ERROR(m_logtag.c_str(), "Http request to retrieve credentials failed");
                return AWSError<CoreErrors>(CoreErrors::NETWORK_CONNECTION, true); // Retryable
            }
            else if (m_errorMarshaller)
            {
                return m_errorMarshaller->Marshall(*response);
            }
            else
            {
                const auto responseCode = response->GetResponseCode();

                AWS_LOGSTREAM_ERROR(m_logtag.c_str(), "Http request to retrieve credentials failed with error code "
                                                          << static_cast<int>(responseCode));
                return CoreErrorsMapper::GetErrorForHttpResponseCode(responseCode);
            }
        }();

        if (!m_retryStrategy->ShouldRetry(error, retries))
        {
            AWS_LOGSTREAM_ERROR(m_logtag.c_str(), "Can not retrive resource from " << httpRequest->GetURIString());
            return {{}, response->GetHeaders(), error.GetResponseCode()};
        }
        auto sleepMillis = m_retryStrategy->CalculateDelayBeforeNextRetry(error, retries);
        AWS_LOGSTREAM_WARN(m_logtag.c_str(), "Request failed, now waiting " << sleepMillis << " ms before attempting again.");
        m_httpClient->RetryRequestSleep(std::chrono::milliseconds(sleepMillis));
    }
}

EC2MetadataClient::EC2MetadataClient(const char* endpoint)
    : AWSHttpResourceClient(EC2_METADATA_CLIENT_LOG_TAG), m_endpoint(endpoint), m_tokenRequired(true)
{
}

EC2MetadataClient::EC2MetadataClient(const Aws::Client::ClientConfiguration &clientConfiguration, const char *endpoint)
    : AWSHttpResourceClient(clientConfiguration, EC2_METADATA_CLIENT_LOG_TAG), m_endpoint(endpoint), m_tokenRequired(true)
{
}

EC2MetadataClient::~EC2MetadataClient()
{

}

Aws::String EC2MetadataClient::GetResource(const char* resourcePath) const
{
    return GetResource(m_endpoint.c_str(), resourcePath, nullptr/*authToken*/);
}

Aws::String EC2MetadataClient::GetDefaultCredentials() const
{
    std::unique_lock<std::recursive_mutex> locker(m_tokenMutex);
    if (m_tokenRequired)
    {
        return GetDefaultCredentialsSecurely();
    }

    AWS_LOGSTREAM_TRACE(m_logtag.c_str(), "Getting default credentials for ec2 instance");
    auto result = GetResourceWithAWSWebServiceResult(m_endpoint.c_str(), EC2_SECURITY_CREDENTIALS_RESOURCE, nullptr);
    Aws::String credentialsString = result.GetPayload();
    auto httpResponseCode = result.GetResponseCode();

    // Note, if service is insane, it might return 404 for our initial secure call,
    // then when we fall back to insecure call, it might return 401 ask for secure call,
    // Then, SDK might get into a recursive loop call situation between secure and insecure call.
    if (httpResponseCode == Http::HttpResponseCode::UNAUTHORIZED)
    {
        m_tokenRequired = true;
        return {};
    }
    locker.unlock();

    Aws::String trimmedCredentialsString = StringUtils::Trim(credentialsString.c_str());
    if (trimmedCredentialsString.empty()) return {};

    Aws::Vector<Aws::String> securityCredentials = StringUtils::Split(trimmedCredentialsString, '\n');

    AWS_LOGSTREAM_DEBUG(m_logtag.c_str(), "Calling EC2MetadataService resource, " << EC2_SECURITY_CREDENTIALS_RESOURCE
                                            << " returned credential string " << trimmedCredentialsString);

    if (securityCredentials.size() == 0)
    {
        AWS_LOGSTREAM_WARN(m_logtag.c_str(), "Initial call to ec2Metadataservice to get credentials failed");
        return {};
    }

    Aws::StringStream ss;
    ss << EC2_SECURITY_CREDENTIALS_RESOURCE << "/" << securityCredentials[0];
    AWS_LOGSTREAM_DEBUG(m_logtag.c_str(), "Calling EC2MetadataService resource " << ss.str());
    return GetResource(ss.str().c_str());
}

Aws::String EC2MetadataClient::GetDefaultCredentialsSecurely() const
{
    std::unique_lock<std::recursive_mutex> locker(m_tokenMutex);
    if (!m_tokenRequired)
    {
        return GetDefaultCredentials();
    }

    Aws::StringStream ss;
    ss << m_endpoint << EC2_IMDS_TOKEN_RESOURCE;
    std::shared_ptr<HttpRequest> tokenRequest(CreateHttpRequest(ss.str(), HttpMethod::HTTP_PUT,
                                                           Aws::Utils::Stream::DefaultResponseStreamFactoryMethod));
    tokenRequest->SetHeaderValue(EC2_IMDS_TOKEN_TTL_HEADER, EC2_IMDS_TOKEN_TTL_DEFAULT_VALUE);
    auto userAgentString = ComputeUserAgentString();
    tokenRequest->SetUserAgent(userAgentString);
    AWS_LOGSTREAM_TRACE(m_logtag.c_str(), "Calling EC2MetadataService to get token");
    auto result = GetResourceWithAWSWebServiceResult(tokenRequest);
    Aws::String tokenString = result.GetPayload();
    Aws::String trimmedTokenString = StringUtils::Trim(tokenString.c_str());

    if (result.GetResponseCode() == HttpResponseCode::BAD_REQUEST)
    {
        return {};
    }
    else if (result.GetResponseCode() != HttpResponseCode::OK || trimmedTokenString.empty())
    {
        m_tokenRequired = false;
        AWS_LOGSTREAM_TRACE(m_logtag.c_str(), "Calling EC2MetadataService to get token failed, falling back to less secure way.");
        return GetDefaultCredentials();
    }
    m_token = trimmedTokenString;
    locker.unlock();
    ss.str("");
    ss << m_endpoint << EC2_SECURITY_CREDENTIALS_RESOURCE;
    std::shared_ptr<HttpRequest> profileRequest(CreateHttpRequest(ss.str(), HttpMethod::HTTP_GET,
                                                           Aws::Utils::Stream::DefaultResponseStreamFactoryMethod));
    profileRequest->SetHeaderValue(EC2_IMDS_TOKEN_HEADER, trimmedTokenString);
    profileRequest->SetUserAgent(userAgentString);
    Aws::String profileString = GetResourceWithAWSWebServiceResult(profileRequest).GetPayload();

    Aws::String trimmedProfileString = StringUtils::Trim(profileString.c_str());
    Aws::Vector<Aws::String> securityCredentials = StringUtils::Split(trimmedProfileString, '\n');

    AWS_LOGSTREAM_DEBUG(m_logtag.c_str(), "Calling EC2MetadataService resource, " << EC2_SECURITY_CREDENTIALS_RESOURCE
                                            << " with token returned profile string " << trimmedProfileString);
    if (securityCredentials.size() == 0)
    {
        AWS_LOGSTREAM_WARN(m_logtag.c_str(), "Calling EC2Metadataservice to get profiles failed");
        return {};
    }

    ss.str("");
    ss << m_endpoint << EC2_SECURITY_CREDENTIALS_RESOURCE << "/" << securityCredentials[0];
    std::shared_ptr<HttpRequest> credentialsRequest(CreateHttpRequest(ss.str(), HttpMethod::HTTP_GET,
                                                                  Aws::Utils::Stream::DefaultResponseStreamFactoryMethod));
    credentialsRequest->SetHeaderValue(EC2_IMDS_TOKEN_HEADER, trimmedTokenString);
    credentialsRequest->SetUserAgent(userAgentString);
    AWS_LOGSTREAM_DEBUG(m_logtag.c_str(), "Calling EC2MetadataService resource " << ss.str() << " with token.");
    return GetResourceWithAWSWebServiceResult(credentialsRequest).GetPayload();
}

Aws::String EC2MetadataClient::GetCurrentRegion() const
{
    AWS_LOGSTREAM_TRACE(m_logtag.c_str(), "Getting current region for ec2 instance");

    Aws::StringStream ss;
    ss << m_endpoint << EC2_REGION_RESOURCE;
    std::shared_ptr<HttpRequest> regionRequest(CreateHttpRequest(ss.str(), HttpMethod::HTTP_GET,
                                                                      Aws::Utils::Stream::DefaultResponseStreamFactoryMethod));
    {
        std::lock_guard<std::recursive_mutex> locker(m_tokenMutex);
        if (m_tokenRequired)
        {
            regionRequest->SetHeaderValue(EC2_IMDS_TOKEN_HEADER, m_token);
        }
    }
    regionRequest->SetUserAgent(ComputeUserAgentString());
    Aws::String azString = GetResourceWithAWSWebServiceResult(regionRequest).GetPayload();

    if (azString.empty())
    {
        AWS_LOGSTREAM_INFO(m_logtag.c_str() ,
                "Unable to pull region from instance metadata service ");
        return {};
    }

    Aws::String trimmedAZString = StringUtils::Trim(azString.c_str());
    AWS_LOGSTREAM_DEBUG(m_logtag.c_str(), "Calling EC2MetadataService resource "
            << EC2_REGION_RESOURCE << " , returned credential string " << trimmedAZString);

    Aws::String region;
    region.reserve(trimmedAZString.length());

    bool digitFound = false;
    for (auto character : trimmedAZString)
    {
        if(digitFound && !isdigit(character))
        {
            break;
        }
        if (isdigit(character))
        {
            digitFound = true;
        }

        region.append(1, character);
    }

    AWS_LOGSTREAM_INFO(m_logtag.c_str(), "Detected current region as " << region);
    return region;
}

ECSCredentialsClient::ECSCredentialsClient(const char* resourcePath, const char* endpoint, const char* token)
    : AWSHttpResourceClient(ECS_CREDENTIALS_CLIENT_LOG_TAG),
    m_resourcePath(resourcePath), m_endpoint(endpoint), m_token(token)
{
}

ECSCredentialsClient::ECSCredentialsClient(const Aws::Client::ClientConfiguration& clientConfiguration, const char* resourcePath, const char* endpoint, const char* token)
    : AWSHttpResourceClient(clientConfiguration, ECS_CREDENTIALS_CLIENT_LOG_TAG),
    m_resourcePath(resourcePath), m_endpoint(endpoint), m_token(token)
{
}

static const char STS_RESOURCE_CLIENT_LOG_TAG[] = "STSResourceClient";
STSCredentialsClient::STSCredentialsClient(const Aws::Client::ClientConfiguration& clientConfiguration)
    : AWSHttpResourceClient(clientConfiguration, STS_RESOURCE_CLIENT_LOG_TAG)
{
    SetErrorMarshaller(Aws::MakeUnique<Aws::Client::XmlErrorMarshaller>(STS_RESOURCE_CLIENT_LOG_TAG));

    Aws::StringStream ss;
    if (clientConfiguration.scheme == Aws::Http::Scheme::HTTP)
    {
        ss << "http://";
    }
    else
    {
        ss << "https://";
    }

    static const int CN_NORTH_1_HASH = Aws::Utils::HashingUtils::HashString(Aws::Region::CN_NORTH_1);
    static const int CN_NORTHWEST_1_HASH = Aws::Utils::HashingUtils::HashString(Aws::Region::CN_NORTHWEST_1);
    auto hash = Aws::Utils::HashingUtils::HashString(clientConfiguration.region.c_str());

    ss << "sts." << clientConfiguration.region << ".amazonaws.com";
    if (hash == CN_NORTH_1_HASH || hash == CN_NORTHWEST_1_HASH)
    {
        ss << ".cn";
    }
    m_endpoint =  ss.str();

    AWS_LOGSTREAM_INFO(STS_RESOURCE_CLIENT_LOG_TAG, "Creating STS ResourceClient with endpoint: " << m_endpoint);
}

STSCredentialsClient::STSAssumeRoleWithWebIdentityResult STSCredentialsClient::GetAssumeRoleWithWebIdentityCredentials(const STSAssumeRoleWithWebIdentityRequest& request)
{
    //Calculate query string
    Aws::StringStream ss;
    ss << "/?Action=AssumeRoleWithWebIdentity"
        << "&Version=2011-06-15"
        << "&RoleSessionName=" << Aws::Utils::StringUtils::URLEncode(request.roleSessionName.c_str())
        << "&RoleArn=" << Aws::Utils::StringUtils::URLEncode(request.roleArn.c_str())
        << "&WebIdentityToken=" << Aws::Utils::StringUtils::URLEncode(request.webIdentityToken.c_str());

    Aws::String credentialsStr = GetResource(m_endpoint.c_str(), ss.str().c_str()/*query string*/, nullptr/*no auth token needed*/);

    //Parse credentials
    STSAssumeRoleWithWebIdentityResult result;
    if (credentialsStr.empty())
    {
        AWS_LOGSTREAM_WARN(STS_RESOURCE_CLIENT_LOG_TAG, "Get an empty credential from sts");
        return result;
    }

    const Utils::Xml::XmlDocument xmlDocument = XmlDocument::CreateFromXmlString(credentialsStr);
    XmlNode rootNode = xmlDocument.GetRootElement();
    XmlNode resultNode = rootNode;
    if (!rootNode.IsNull() && (rootNode.GetName() != "AssumeRoleWithWebIdentityResult"))
    {
        resultNode = rootNode.FirstChild("AssumeRoleWithWebIdentityResult");
    }

    if (!resultNode.IsNull())
    {
        XmlNode credentialsNode = resultNode.FirstChild("Credentials");
        if (!credentialsNode.IsNull())
        {
            XmlNode accessKeyIdNode = credentialsNode.FirstChild("AccessKeyId");
            if (!accessKeyIdNode.IsNull())
            {
                result.creds.SetAWSAccessKeyId(accessKeyIdNode.GetText());
            }

            XmlNode secretAccessKeyNode = credentialsNode.FirstChild("SecretAccessKey");
            if (!secretAccessKeyNode.IsNull())
            {
                result.creds.SetAWSSecretKey(secretAccessKeyNode.GetText());
            }

            XmlNode sessionTokenNode = credentialsNode.FirstChild("SessionToken");
            if (!sessionTokenNode.IsNull())
            {
                result.creds.SetSessionToken(sessionTokenNode.GetText());
            }

            XmlNode expirationNode = credentialsNode.FirstChild("Expiration");
            if (!expirationNode.IsNull())
            {
                result.creds.SetExpiration(DateTime(StringUtils::Trim(expirationNode.GetText().c_str()).c_str(), DateFormat::ISO_8601));
            }
        }
    }
    return result;
}
