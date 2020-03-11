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
#include <aws/s3/model/GlacierJobParameters.h>
#include <aws/s3/model/RestoreRequestType.h>
#include <aws/s3/model/Tier.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/s3/model/SelectParameters.h>
#include <aws/s3/model/OutputLocation.h>
#include <utility>

namespace Aws
{
namespace Utils
{
namespace Xml
{
  class XmlNode;
} // namespace Xml
} // namespace Utils
namespace S3
{
namespace Model
{

  /**
   * <p>Container for restore job parameters.</p><p><h3>See Also:</h3>   <a
   * href="http://docs.aws.amazon.com/goto/WebAPI/s3-2006-03-01/RestoreRequest">AWS
   * API Reference</a></p>
   */
  class AWS_S3_API RestoreRequest
  {
  public:
    RestoreRequest();
    RestoreRequest(const Aws::Utils::Xml::XmlNode& xmlNode);
    RestoreRequest& operator=(const Aws::Utils::Xml::XmlNode& xmlNode);

    void AddToNode(Aws::Utils::Xml::XmlNode& parentNode) const;


    /**
     * <p>Lifetime of the active copy in days. Do not use with restores that specify
     * <code>OutputLocation</code>.</p>
     */
    inline int GetDays() const{ return m_days; }

    /**
     * <p>Lifetime of the active copy in days. Do not use with restores that specify
     * <code>OutputLocation</code>.</p>
     */
    inline bool DaysHasBeenSet() const { return m_daysHasBeenSet; }

    /**
     * <p>Lifetime of the active copy in days. Do not use with restores that specify
     * <code>OutputLocation</code>.</p>
     */
    inline void SetDays(int value) { m_daysHasBeenSet = true; m_days = value; }

    /**
     * <p>Lifetime of the active copy in days. Do not use with restores that specify
     * <code>OutputLocation</code>.</p>
     */
    inline RestoreRequest& WithDays(int value) { SetDays(value); return *this;}


    /**
     * <p>Glacier related parameters pertaining to this job. Do not use with restores
     * that specify <code>OutputLocation</code>.</p>
     */
    inline const GlacierJobParameters& GetGlacierJobParameters() const{ return m_glacierJobParameters; }

    /**
     * <p>Glacier related parameters pertaining to this job. Do not use with restores
     * that specify <code>OutputLocation</code>.</p>
     */
    inline bool GlacierJobParametersHasBeenSet() const { return m_glacierJobParametersHasBeenSet; }

    /**
     * <p>Glacier related parameters pertaining to this job. Do not use with restores
     * that specify <code>OutputLocation</code>.</p>
     */
    inline void SetGlacierJobParameters(const GlacierJobParameters& value) { m_glacierJobParametersHasBeenSet = true; m_glacierJobParameters = value; }

    /**
     * <p>Glacier related parameters pertaining to this job. Do not use with restores
     * that specify <code>OutputLocation</code>.</p>
     */
    inline void SetGlacierJobParameters(GlacierJobParameters&& value) { m_glacierJobParametersHasBeenSet = true; m_glacierJobParameters = std::move(value); }

    /**
     * <p>Glacier related parameters pertaining to this job. Do not use with restores
     * that specify <code>OutputLocation</code>.</p>
     */
    inline RestoreRequest& WithGlacierJobParameters(const GlacierJobParameters& value) { SetGlacierJobParameters(value); return *this;}

    /**
     * <p>Glacier related parameters pertaining to this job. Do not use with restores
     * that specify <code>OutputLocation</code>.</p>
     */
    inline RestoreRequest& WithGlacierJobParameters(GlacierJobParameters&& value) { SetGlacierJobParameters(std::move(value)); return *this;}


    /**
     * <p>Type of restore request.</p>
     */
    inline const RestoreRequestType& GetType() const{ return m_type; }

    /**
     * <p>Type of restore request.</p>
     */
    inline bool TypeHasBeenSet() const { return m_typeHasBeenSet; }

    /**
     * <p>Type of restore request.</p>
     */
    inline void SetType(const RestoreRequestType& value) { m_typeHasBeenSet = true; m_type = value; }

    /**
     * <p>Type of restore request.</p>
     */
    inline void SetType(RestoreRequestType&& value) { m_typeHasBeenSet = true; m_type = std::move(value); }

    /**
     * <p>Type of restore request.</p>
     */
    inline RestoreRequest& WithType(const RestoreRequestType& value) { SetType(value); return *this;}

    /**
     * <p>Type of restore request.</p>
     */
    inline RestoreRequest& WithType(RestoreRequestType&& value) { SetType(std::move(value)); return *this;}


    /**
     * <p>Glacier retrieval tier at which the restore will be processed.</p>
     */
    inline const Tier& GetTier() const{ return m_tier; }

    /**
     * <p>Glacier retrieval tier at which the restore will be processed.</p>
     */
    inline bool TierHasBeenSet() const { return m_tierHasBeenSet; }

    /**
     * <p>Glacier retrieval tier at which the restore will be processed.</p>
     */
    inline void SetTier(const Tier& value) { m_tierHasBeenSet = true; m_tier = value; }

    /**
     * <p>Glacier retrieval tier at which the restore will be processed.</p>
     */
    inline void SetTier(Tier&& value) { m_tierHasBeenSet = true; m_tier = std::move(value); }

    /**
     * <p>Glacier retrieval tier at which the restore will be processed.</p>
     */
    inline RestoreRequest& WithTier(const Tier& value) { SetTier(value); return *this;}

    /**
     * <p>Glacier retrieval tier at which the restore will be processed.</p>
     */
    inline RestoreRequest& WithTier(Tier&& value) { SetTier(std::move(value)); return *this;}


    /**
     * <p>The optional description for the job.</p>
     */
    inline const Aws::String& GetDescription() const{ return m_description; }

    /**
     * <p>The optional description for the job.</p>
     */
    inline bool DescriptionHasBeenSet() const { return m_descriptionHasBeenSet; }

    /**
     * <p>The optional description for the job.</p>
     */
    inline void SetDescription(const Aws::String& value) { m_descriptionHasBeenSet = true; m_description = value; }

    /**
     * <p>The optional description for the job.</p>
     */
    inline void SetDescription(Aws::String&& value) { m_descriptionHasBeenSet = true; m_description = std::move(value); }

    /**
     * <p>The optional description for the job.</p>
     */
    inline void SetDescription(const char* value) { m_descriptionHasBeenSet = true; m_description.assign(value); }

    /**
     * <p>The optional description for the job.</p>
     */
    inline RestoreRequest& WithDescription(const Aws::String& value) { SetDescription(value); return *this;}

    /**
     * <p>The optional description for the job.</p>
     */
    inline RestoreRequest& WithDescription(Aws::String&& value) { SetDescription(std::move(value)); return *this;}

    /**
     * <p>The optional description for the job.</p>
     */
    inline RestoreRequest& WithDescription(const char* value) { SetDescription(value); return *this;}


    /**
     * <p>Describes the parameters for Select job types.</p>
     */
    inline const SelectParameters& GetSelectParameters() const{ return m_selectParameters; }

    /**
     * <p>Describes the parameters for Select job types.</p>
     */
    inline bool SelectParametersHasBeenSet() const { return m_selectParametersHasBeenSet; }

    /**
     * <p>Describes the parameters for Select job types.</p>
     */
    inline void SetSelectParameters(const SelectParameters& value) { m_selectParametersHasBeenSet = true; m_selectParameters = value; }

    /**
     * <p>Describes the parameters for Select job types.</p>
     */
    inline void SetSelectParameters(SelectParameters&& value) { m_selectParametersHasBeenSet = true; m_selectParameters = std::move(value); }

    /**
     * <p>Describes the parameters for Select job types.</p>
     */
    inline RestoreRequest& WithSelectParameters(const SelectParameters& value) { SetSelectParameters(value); return *this;}

    /**
     * <p>Describes the parameters for Select job types.</p>
     */
    inline RestoreRequest& WithSelectParameters(SelectParameters&& value) { SetSelectParameters(std::move(value)); return *this;}


    /**
     * <p>Describes the location where the restore job's output is stored.</p>
     */
    inline const OutputLocation& GetOutputLocation() const{ return m_outputLocation; }

    /**
     * <p>Describes the location where the restore job's output is stored.</p>
     */
    inline bool OutputLocationHasBeenSet() const { return m_outputLocationHasBeenSet; }

    /**
     * <p>Describes the location where the restore job's output is stored.</p>
     */
    inline void SetOutputLocation(const OutputLocation& value) { m_outputLocationHasBeenSet = true; m_outputLocation = value; }

    /**
     * <p>Describes the location where the restore job's output is stored.</p>
     */
    inline void SetOutputLocation(OutputLocation&& value) { m_outputLocationHasBeenSet = true; m_outputLocation = std::move(value); }

    /**
     * <p>Describes the location where the restore job's output is stored.</p>
     */
    inline RestoreRequest& WithOutputLocation(const OutputLocation& value) { SetOutputLocation(value); return *this;}

    /**
     * <p>Describes the location where the restore job's output is stored.</p>
     */
    inline RestoreRequest& WithOutputLocation(OutputLocation&& value) { SetOutputLocation(std::move(value)); return *this;}

  private:

    int m_days;
    bool m_daysHasBeenSet;

    GlacierJobParameters m_glacierJobParameters;
    bool m_glacierJobParametersHasBeenSet;

    RestoreRequestType m_type;
    bool m_typeHasBeenSet;

    Tier m_tier;
    bool m_tierHasBeenSet;

    Aws::String m_description;
    bool m_descriptionHasBeenSet;

    SelectParameters m_selectParameters;
    bool m_selectParametersHasBeenSet;

    OutputLocation m_outputLocation;
    bool m_outputLocationHasBeenSet;
  };

} // namespace Model
} // namespace S3
} // namespace Aws
