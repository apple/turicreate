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
#include <aws/s3/S3_EXPORTS.h>
#include <aws/core/Region.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/s3/S3ARN.h>

namespace Aws
{

namespace S3
{
namespace S3Endpoint
{
  /**
   * Compute endpoint based on region.
   * @param regionName The AWS region used in the endpoint
   * @param useDualStack Using dual-stack endpoint if true
   * @param USEast1UseRegionalEndpoint Using global endpoint for us-east-1 if the value is LEGACY, or using regional endpoint if it's REGIONAL
   */
  AWS_S3_API Aws::String ForRegion(const Aws::String& regionName, bool useDualStack = false, bool USEast1UseRegionalEndpoint = false);

  /**
   * Compute endpoint based on Access Point ARN.
   * @param arn The S3 Access Point ARN
   * @param regionNameOverride Override region name in ARN if it's not empty
   * @param useDualStack Using dual-stack endpoint if true
   */
  AWS_S3_API Aws::String ForAccessPointArn(const S3ARN& arn, const Aws::String& regionNameOverride = "", bool useDualStack = false);
} // namespace S3Endpoint
} // namespace S3
} // namespace Aws
