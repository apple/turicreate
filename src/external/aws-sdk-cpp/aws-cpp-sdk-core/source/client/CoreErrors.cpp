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

#include "aws/core/client/AWSError.h"
#include "aws/core/client/CoreErrors.h"
#include "aws/core/utils/HashingUtils.h"  

using namespace Aws::Client;
using namespace Aws::Utils;

//we can't use a static map here due to memory allocation ordering. 
//instead we compute the hash of these strings to avoid so many string compares.
static const int INCOMPLETE_SIGNATURE_EXCEPTION_HASH = HashingUtils::HashString("IncompleteSignatureException");
static const int INCOMPLETE_SIGNATURE_HASH = HashingUtils::HashString("IncompleteSignature");
static const int INTERNAL_FAILURE_HASH = HashingUtils::HashString("InternalFailure");
static const int INTERNAL_SERVER_ERROR_HASH = HashingUtils::HashString("InternalServerError");
static const int INVALID_ACTION_HASH = HashingUtils::HashString("InvalidAction");
static const int INVALID_CLIENT_TOKEN_ID_HASH = HashingUtils::HashString("InvalidClientTokenId");
static const int INVALID_CLIENT_TOKEN_ID_EXCEPTION_HASH = HashingUtils::HashString("InvalidClientTokenIdException");
static const int INVALID_PARAMETER_COMBINATION_HASH = HashingUtils::HashString("InvalidParameterCombination");
static const int INVALID_PARAMETER_VALUE_HASH = HashingUtils::HashString("InvalidParameterValue");
static const int INVALID_QUERY_PARAMETER_HASH = HashingUtils::HashString("InvalidQueryParameter");
static const int INVALID_QUERY_PARAMETER_EXCEPTION_HASH = HashingUtils::HashString("InvalidQueryParameterException");
static const int MALFORMED_QUERY_STRING_HASH = HashingUtils::HashString("MalformedQueryString");
static const int MALFORMED_QUERY_STRING_EXCEPTION_HASH = HashingUtils::HashString("MalformedQueryStringException");
static const int MISSING_AUTHENTICATION_TOKEN_HASH = HashingUtils::HashString("MissingAuthenticationToken");
static const int MISSING_AUTHENTICATION_TOKEN_EXCEPTION_HASH = HashingUtils::HashString("MissingAuthenticationTokenException");
static const int MISSING_PARAMETER_HASH = HashingUtils::HashString("MissingParameterException");
static const int OPT_IN_REQUIRED_HASH = HashingUtils::HashString("OptInRequired");
static const int REQUEST_EXPIRED_HASH = HashingUtils::HashString("RequestExpired");
static const int REQUEST_EXPIRED_EXCEPTION_HASH = HashingUtils::HashString("RequestExpiredException");
static const int SERVICE_UNAVAILABLE_HASH = HashingUtils::HashString("ServiceUnavailable");
static const int SERVICE_UNAVAILABLE_EXCEPTION_HASH = HashingUtils::HashString("ServiceUnavailableException");
static const int THROTTLING_HASH = HashingUtils::HashString("Throttling");
static const int THROTTLING_EXCEPTION_HASH = HashingUtils::HashString("ThrottlingException");
static const int VALIDATION_ERROR_HASH = HashingUtils::HashString("ValidationError");
static const int VALIDATION_ERROR_EXCEPTION_HASH = HashingUtils::HashString("ValidationErrorException");
static const int VALIDATION_EXCEPTION_HASH = HashingUtils::HashString("ValidationException");
static const int ACCESS_DENIED_HASH = HashingUtils::HashString("AccessDenied");
static const int ACCESS_DENIED_EXCEPTION_HASH = HashingUtils::HashString("AccessDeniedException");
static const int RESOURCE_NOT_FOUND_HASH = HashingUtils::HashString("ResourceNotFound");
static const int RESOURCE_NOT_FOUND_EXCEPTION_HASH = HashingUtils::HashString("ResourceNotFoundException");
static const int UNRECOGNIZED_CLIENT_HASH = HashingUtils::HashString("UnrecognizedClient");
static const int UNRECOGNIZED_CLIENT_EXCEPTION_HASH = HashingUtils::HashString("UnrecognizedClientException");

AWSError<CoreErrors> CoreErrorsMapper::GetErrorForName(const char* errorName)
{
  int errorHash = HashingUtils::HashString(errorName);

  if (errorHash == INCOMPLETE_SIGNATURE_HASH || errorHash == INCOMPLETE_SIGNATURE_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::INCOMPLETE_SIGNATURE, false);
  }
  else if (errorHash == INTERNAL_FAILURE_HASH || errorHash == INTERNAL_SERVER_ERROR_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::INTERNAL_FAILURE, true);
  }
  else if (errorHash == INVALID_ACTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::INVALID_ACTION, false);
  }
  else if (errorHash == INVALID_CLIENT_TOKEN_ID_HASH || errorHash == INVALID_CLIENT_TOKEN_ID_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::INVALID_CLIENT_TOKEN_ID, false);
  }
  else if (errorHash == INVALID_PARAMETER_COMBINATION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::INVALID_PARAMETER_COMBINATION, false);
  }
  else if (errorHash == INVALID_PARAMETER_VALUE_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::INVALID_PARAMETER_VALUE, false);
  }
  else if (errorHash == INVALID_QUERY_PARAMETER_HASH || errorHash == INVALID_QUERY_PARAMETER_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::INVALID_QUERY_PARAMETER, false);
  }
  else if (errorHash == MALFORMED_QUERY_STRING_HASH || errorHash == MALFORMED_QUERY_STRING_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::MALFORMED_QUERY_STRING, false);
  }
  else if (errorHash == MISSING_AUTHENTICATION_TOKEN_HASH || errorHash == MISSING_AUTHENTICATION_TOKEN_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::MISSING_AUTHENTICATION_TOKEN, false);
  }
  else if (errorHash == MISSING_PARAMETER_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::MISSING_PARAMETER, false);
  }
  else if (errorHash == OPT_IN_REQUIRED_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::OPT_IN_REQUIRED, false);
  }
  else if (errorHash == REQUEST_EXPIRED_HASH || errorHash == REQUEST_EXPIRED_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::REQUEST_EXPIRED, true);
  }
  else if (errorHash == SERVICE_UNAVAILABLE_HASH || errorHash == SERVICE_UNAVAILABLE_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::SERVICE_UNAVAILABLE, true);
  }
  else if (errorHash == THROTTLING_HASH || errorHash == THROTTLING_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::THROTTLING, true);
  }
  else if (errorHash == VALIDATION_ERROR_HASH || errorHash == VALIDATION_ERROR_EXCEPTION_HASH || errorHash == VALIDATION_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::VALIDATION, false);
  }
  else if (errorHash == ACCESS_DENIED_HASH || errorHash == ACCESS_DENIED_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::ACCESS_DENIED, false);
  }
  else if (errorHash == RESOURCE_NOT_FOUND_HASH || errorHash == RESOURCE_NOT_FOUND_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::RESOURCE_NOT_FOUND, false);
  }
  else if (errorHash == UNRECOGNIZED_CLIENT_HASH || errorHash == UNRECOGNIZED_CLIENT_EXCEPTION_HASH)
  {
    return AWSError<CoreErrors>(CoreErrors::UNRECOGNIZED_CLIENT, false);
  }

  return AWSError<CoreErrors>(CoreErrors::UNKNOWN, false);
}
