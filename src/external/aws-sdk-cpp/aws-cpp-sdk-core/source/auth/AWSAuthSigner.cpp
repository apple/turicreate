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

#include <aws/core/auth/AWSAuthSigner.h>

#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/http/HttpRequest.h>
#include <aws/core/http/HttpResponse.h>
#include <aws/core/utils/DateTime.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/memory/AWSMemory.h>
#include <aws/core/utils/crypto/Sha256.h>
#include <aws/core/utils/crypto/Sha256HMAC.h>

#include <cstdio>
#include <iomanip>
#include <math.h>
#include <string.h>

using namespace Aws;
using namespace Aws::Client;
using namespace Aws::Auth;
using namespace Aws::Http;
using namespace Aws::Utils;
using namespace Aws::Utils::Logging;

static const char* EQ = "=";
static const char* SIGNATURE = "Signature";
static const char* AWS_HMAC_SHA256 = "AWS4-HMAC-SHA256";
static const char* AWS4_REQUEST = "aws4_request";
static const char* SIGNED_HEADERS = "SignedHeaders";
static const char* CREDENTIAL = "Credential";
static const char* NEWLINE = "\n";
static const char* X_AMZ_SIGNED_HEADERS = "X-Amz-SignedHeaders";
static const char* X_AMZ_ALGORITHM = "X-Amz-Algorithm";
static const char* X_AMZ_CREDENTIAL = "X-Amz-Credential";
static const char* UNSIGNED_PAYLOAD = "UNSIGNED-PAYLOAD";
static const char* X_AMZ_SIGNATURE = "X-Amz-Signature";
static const char* SIGNING_KEY = "AWS4";
static const char* LONG_DATE_FORMAT_STR = "%Y%m%dT%H%M%SZ";
static const char* SIMPLE_DATE_FORMAT_STR = "%Y%m%d";

static const char* v4LogTag = "AWSAuthV4Signer";

Aws::String CanonicalizeRequestSigningString(HttpRequest& request, bool urlEscapePath)
{
    request.CanonicalizeRequest();
    Aws::StringStream signingStringStream;
    signingStringStream << HttpMethodMapper::GetNameForHttpMethod(request.GetMethod());

    //double encode paths unless explicitly stated otherwise (for s3 compatibility)
    URI uriCpy = request.GetUri();
    uriCpy.SetPath(uriCpy.GetURLEncodedPath());

    signingStringStream << NEWLINE << (urlEscapePath ? uriCpy.GetURLEncodedPath() : uriCpy.GetPath()) << NEWLINE;

    if (request.GetQueryString().size() > 1 && request.GetQueryString().find("=") != std::string::npos)
    {
        signingStringStream << request.GetQueryString().substr(1) << NEWLINE;
    }
    else if (request.GetQueryString().size() > 1)
    {
        signingStringStream << request.GetQueryString().substr(1) << "=" << NEWLINE;
    }
    else
    {
        signingStringStream << NEWLINE;
    }

    return signingStringStream.str();
}

AWSAuthV4Signer::AWSAuthV4Signer(const std::shared_ptr<Auth::AWSCredentialsProvider>& credentialsProvider,
    const char* serviceName, const Aws::String& region, bool signPayloads, bool urlEscapePath) :
    m_credentialsProvider(credentialsProvider),
    m_serviceName(serviceName),
    m_region(region),
    m_hash(Aws::MakeUnique<Aws::Utils::Crypto::Sha256>(v4LogTag)),
    m_HMAC(Aws::MakeUnique<Aws::Utils::Crypto::Sha256HMAC>(v4LogTag)),
    m_signPayloads(signPayloads),
    m_urlEscapePath(urlEscapePath)
{
}

AWSAuthV4Signer::~AWSAuthV4Signer()
{
    // empty destructor in .cpp file to keep from needing the implementation of (AWSCredentialsProvider, Sha256, Sha256HMAC) in the header file 
}

bool AWSAuthV4Signer::SignRequest(Aws::Http::HttpRequest& request) const
{
    AWSCredentials credentials = m_credentialsProvider->GetAWSCredentials();

    //don't sign anonymous requests
    if (credentials.GetAWSAccessKeyId().empty() || credentials.GetAWSSecretKey().empty())
    {
        return true;
    }

    if (!credentials.GetSessionToken().empty())
    {
        request.SetAwsSessionToken(credentials.GetSessionToken());
    }

    Aws::String payloadHash(UNSIGNED_PAYLOAD);
    if(m_signPayloads || request.GetUri().GetScheme() != Http::Scheme::HTTPS)
    {
        payloadHash.assign(ComputePayloadHash(request));
        if (payloadHash.empty())
        {
            return false;
        }
    }
    else
    {
        AWS_LOGSTREAM_DEBUG(v4LogTag, "Note: Http payloads are not being signed. signPayloads=" << m_signPayloads
                << " http scheme=" << Http::SchemeMapper::ToString(request.GetUri().GetScheme()));
    }

    request.SetHeaderValue("x-amz-content-sha256", payloadHash);

    //calculate date header to use in internal signature (this also goes into date header).
    Aws::String dateHeaderValue = DateTime::CalculateGmtTimestampAsString(LONG_DATE_FORMAT_STR);
    request.SetHeaderValue(AWS_DATE_HEADER, dateHeaderValue);

    Aws::StringStream headersStream;
    Aws::StringStream signedHeadersStream;

    for (const auto& header : request.GetHeaders())
    {
        headersStream << header.first << ":" << header.second << NEWLINE;
        signedHeadersStream << header.first << ";";
    }

    Aws::String canonicalHeadersString = headersStream.str();
    AWS_LOGSTREAM_DEBUG(v4LogTag, "Canonical Header String: " << canonicalHeadersString);

    //calculate signed headers parameter
    Aws::String signedHeadersValue = signedHeadersStream.str();
    //remove that last semi-colon
    signedHeadersValue.erase(signedHeadersValue.length() - 1);
    AWS_LOGSTREAM_DEBUG(v4LogTag, "Signed Headers value:" << signedHeadersValue);

    //generate generalized canonicalized request string.
    Aws::String canonicalRequestString = CanonicalizeRequestSigningString(request, m_urlEscapePath);

    //append v4 stuff to the canonical request string.
    canonicalRequestString.append(canonicalHeadersString);
    canonicalRequestString.append(NEWLINE);
    canonicalRequestString.append(signedHeadersValue);
    canonicalRequestString.append(NEWLINE);
    canonicalRequestString.append(payloadHash);

    AWS_LOGSTREAM_DEBUG(v4LogTag, "Canonical Request String: " << canonicalRequestString);

    //now compute sha256 on that request string
    auto hashResult = m_hash->Calculate(canonicalRequestString);
    if (!hashResult.IsSuccess())
    {
        AWS_LOGSTREAM_ERROR(v4LogTag, "Failed to hash (sha256) request string \"" << canonicalRequestString << "\"");
        return false;
    }

    auto sha256Digest = hashResult.GetResult();
    Aws::String cannonicalRequestHash = HashingUtils::HexEncode(sha256Digest);
    Aws::String simpleDate = DateTime::CalculateGmtTimestampAsString(SIMPLE_DATE_FORMAT_STR);

    Aws::String stringToSign = GenerateStringToSign(dateHeaderValue, simpleDate, cannonicalRequestHash);
    auto finalSignature = GenerateSignature(credentials, stringToSign, simpleDate);

    Aws::StringStream ss;
    ss << AWS_HMAC_SHA256 << " " << CREDENTIAL << EQ << credentials.GetAWSAccessKeyId() << "/" << simpleDate
        << "/" << m_region << "/" << m_serviceName << "/" << AWS4_REQUEST << ", " << SIGNED_HEADERS << EQ
        << signedHeadersValue << ", " << SIGNATURE << EQ << finalSignature;

    auto awsAuthString = ss.str();
    AWS_LOGSTREAM_DEBUG(v4LogTag, "Signing request with: " << awsAuthString);
    request.SetAwsAuthorization(awsAuthString);

    return true;
}

bool AWSAuthV4Signer::PresignRequest(Aws::Http::HttpRequest& request, long long expirationTimeInSeconds) const
{
    AWSCredentials credentials = m_credentialsProvider->GetAWSCredentials();

    //don't sign anonymous requests
    if (credentials.GetAWSAccessKeyId().empty() || credentials.GetAWSSecretKey().empty())
    {
        return true;
    }

    Aws::StringStream intConversionStream;
    intConversionStream << expirationTimeInSeconds;
    request.AddQueryStringParameter(Http::X_AMZ_EXPIRES_HEADER, intConversionStream.str());   

    if (!credentials.GetSessionToken().empty())
    {
        request.AddQueryStringParameter(Http::AWS_SECURITY_TOKEN, credentials.GetSessionToken());       
    }

    //calculate date header to use in internal signature (this also goes into date header).
    Aws::String dateQueryValue = DateTime::CalculateGmtTimestampAsString(LONG_DATE_FORMAT_STR);
    request.AddQueryStringParameter(Http::AWS_DATE_HEADER, dateQueryValue);

    Aws::StringStream ss;
    ss << Http::HOST_HEADER << ":" << request.GetHeaderValue(Http::HOST_HEADER) << NEWLINE;
    Aws::String canonicalHeadersString(ss.str());
    ss.str("");

    AWS_LOGSTREAM_DEBUG(v4LogTag, "Canonical Header String: " << canonicalHeadersString);

    //calculate signed headers parameter
    Aws::String signedHeadersValue(Http::HOST_HEADER);
    request.AddQueryStringParameter(X_AMZ_SIGNED_HEADERS, signedHeadersValue);
    
    AWS_LOGSTREAM_DEBUG(v4LogTag, "Signed Headers value: " << signedHeadersValue);

    Aws::String simpleDate = DateTime::CalculateGmtTimestampAsString(SIMPLE_DATE_FORMAT_STR);
    ss << credentials.GetAWSAccessKeyId() << "/" << simpleDate
        << "/" << m_region << "/" << m_serviceName << "/" << AWS4_REQUEST;

    request.AddQueryStringParameter(X_AMZ_ALGORITHM, AWS_HMAC_SHA256);
    request.AddQueryStringParameter(X_AMZ_CREDENTIAL, ss.str());
    ss.str("");

    //generate generalized canonicalized request string.
    Aws::String canonicalRequestString = CanonicalizeRequestSigningString(request, m_urlEscapePath);

    //append v4 stuff to the canonical request string.
    canonicalRequestString.append(canonicalHeadersString);
    canonicalRequestString.append(NEWLINE);
    canonicalRequestString.append(signedHeadersValue);
    canonicalRequestString.append(NEWLINE);
    canonicalRequestString.append(UNSIGNED_PAYLOAD);
    AWS_LOGSTREAM_DEBUG(v4LogTag, "Canonical Request String: " << canonicalRequestString);

    //now compute sha256 on that request string
    auto hashResult = m_hash->Calculate(canonicalRequestString);
    if (!hashResult.IsSuccess())
    {
        AWS_LOGSTREAM_ERROR(v4LogTag, "Failed to hash (sha256) request string \"" << canonicalRequestString << "\"");
        return false;
    }

    auto sha256Digest = hashResult.GetResult();
    auto cannonicalRequestHash = HashingUtils::HexEncode(sha256Digest);   

    auto stringToSign = GenerateStringToSign(dateQueryValue, simpleDate, cannonicalRequestHash);

    auto finalSigningHash = GenerateSignature(credentials, stringToSign, simpleDate);
    if (finalSigningHash.empty())
    {
        return false;
    }

    //add that the signature to the query string    
    request.AddQueryStringParameter(X_AMZ_SIGNATURE, finalSigningHash);

    return true;
}

Aws::String AWSAuthV4Signer::GenerateSignature(const AWSCredentials& credentials, const Aws::String& stringToSign, const Aws::String& simpleDate) const
{
    AWS_LOGSTREAM_DEBUG(v4LogTag, "Final String to sign: " << stringToSign);

    Aws::StringStream ss;

    //now we do the complicated part of deriving a signing key.
    Aws::String signingKey(SIGNING_KEY);
    signingKey.append(credentials.GetAWSSecretKey());

    //we use digest only for the derivation process.
    auto hashResult = m_HMAC->Calculate(ByteBuffer((unsigned char*)simpleDate.c_str(), simpleDate.length()),
        ByteBuffer((unsigned char*)signingKey.c_str(), signingKey.length()));
    if (!hashResult.IsSuccess())
    {
        AWS_LOGSTREAM_ERROR(v4LogTag, "Failed to hmac (sha256) date string \"" << simpleDate << "\"");
        return "";
    }

    auto kDate = hashResult.GetResult();
    hashResult = m_HMAC->Calculate(ByteBuffer((unsigned char*)m_region.c_str(), m_region.length()), kDate);
    if (!hashResult.IsSuccess())
    {
        AWS_LOGSTREAM_ERROR(v4LogTag, "Failed to hmac (sha256) region string \"" << m_region << "\"");
        return "";
    }

    auto kRegion = hashResult.GetResult();
    hashResult = m_HMAC->Calculate(ByteBuffer((unsigned char*)m_serviceName.c_str(), m_serviceName.length()), kRegion);
    if (!hashResult.IsSuccess())
    {
        AWS_LOGSTREAM_ERROR(v4LogTag, "Failed to hmac (sha256) service string \"" << m_serviceName << "\"");
        return "";
    }

    auto kService = hashResult.GetResult();
    hashResult = m_HMAC->Calculate(ByteBuffer((unsigned char*)AWS4_REQUEST, strlen(AWS4_REQUEST)), kService);
    if (!hashResult.IsSuccess())
    {
        AWS_LOGSTREAM_ERROR(v4LogTag, "Unable to hmac (sha256) request string \"" << AWS4_REQUEST << "\"");
        return "";
    }

    auto kSigning = hashResult.GetResult();
    hashResult = m_HMAC->Calculate(ByteBuffer((unsigned char*)stringToSign.c_str(), stringToSign.length()), kSigning);
    if (!hashResult.IsSuccess())
    {
        AWS_LOGSTREAM_ERROR(v4LogTag, "Unable to hmac (sha256) final string \"" << stringToSign << "\"");
        return "";
    }

    //now we finally sign our request string with our hex encoded derived hash.
    auto finalSigningDigest = hashResult.GetResult();

    auto finalSigningHash = HashingUtils::HexEncode(finalSigningDigest);
    AWS_LOGSTREAM_DEBUG(v4LogTag, "Final computed signing hash: " << finalSigningHash);

    return finalSigningHash;
}

Aws::String AWSAuthV4Signer::ComputePayloadHash(Aws::Http::HttpRequest& request) const
{
    //compute hash on payload if it exists.
    auto hashResult = request.GetContentBody() ? m_hash->Calculate(*request.GetContentBody())
        : m_hash->Calculate("");

    if(request.GetContentBody())
    {
        request.GetContentBody()->clear();
        request.GetContentBody()->seekg(0);
    }

    if (!hashResult.IsSuccess())
    {
        AWS_LOG_ERROR(v4LogTag, "Unable to hash (sha256) request body");
        return "";
    }

    auto sha256Digest = hashResult.GetResult();

    Aws::String payloadHash(HashingUtils::HexEncode(sha256Digest));
    AWS_LOGSTREAM_DEBUG(v4LogTag, "Calculated sha256 " << payloadHash << " for payload.");
    return payloadHash;
}

Aws::String AWSAuthV4Signer::GenerateStringToSign(const Aws::String& dateValue, const Aws::String& simpleDate, const Aws::String& canonicalRequestHash) const
{
    //generate the actual string we will use in signing the final request.
    Aws::StringStream ss;

    ss << AWS_HMAC_SHA256 << NEWLINE << dateValue << NEWLINE << simpleDate << "/" << m_region << "/"
        << m_serviceName << "/" << AWS4_REQUEST << NEWLINE << canonicalRequestHash;

    return ss.str();
}
