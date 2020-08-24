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
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/utils/event/EventStreamHandler.h>
#include <aws/core/client/AWSError.h>
#include <aws/s3/S3_EXPORTS.h>
#include <aws/s3/S3Errors.h>

#include <aws/s3/model/RecordsEvent.h>
#include <aws/s3/model/StatsEvent.h>
#include <aws/s3/model/ProgressEvent.h>

namespace Aws
{
namespace S3
{
namespace Model
{
    enum class SelectObjectContentEventType
    {
        RECORDS,
        STATS,
        PROGRESS,
        CONT,
        END,
        UNKNOWN
    };

    class AWS_S3_API SelectObjectContentHandler : public Aws::Utils::Event::EventStreamHandler
    {
        typedef std::function<void(const RecordsEvent&)> RecordsEventCallback;
        typedef std::function<void(const StatsEvent&)> StatsEventCallback;
        typedef std::function<void(const ProgressEvent&)> ProgressEventCallback;
        typedef std::function<void()> ContinuationEventCallback;
        typedef std::function<void()> EndEventCallback;
        typedef std::function<void(const Aws::Client::AWSError<S3Errors>& error)> ErrorCallback;

    public:
        SelectObjectContentHandler();
        SelectObjectContentHandler& operator=(const SelectObjectContentHandler& handler) = default;

        virtual void OnEvent() override;

        inline void SetRecordsEventCallback(const RecordsEventCallback& callback) { m_onRecordsEvent = callback; }
        inline void SetStatsEventCallback(const StatsEventCallback& callback) { m_onStatsEvent = callback; }
        inline void SetProgressEventCallback(const ProgressEventCallback& callback) { m_onProgressEvent = callback; }
        inline void SetContinuationEventCallback(const ContinuationEventCallback& callback) { m_onContinuationEvent = callback; }
        inline void SetEndEventCallback(const EndEventCallback& callback) { m_onEndEvent = callback; }
        inline void SetOnErrorCallback(const ErrorCallback& callback) { m_onError = callback; }

    private:
        void HandleEventInMessage();
        void HandleErrorInMessage();
        void MarshallError(const Aws::String& errorCode, const Aws::String& errorMessage);

        RecordsEventCallback m_onRecordsEvent;
        StatsEventCallback m_onStatsEvent;
        ProgressEventCallback m_onProgressEvent;
        ContinuationEventCallback m_onContinuationEvent;
        EndEventCallback m_onEndEvent;
        ErrorCallback m_onError;
    };

namespace SelectObjectContentEventMapper
{
    AWS_S3_API SelectObjectContentEventType GetSelectObjectContentEventTypeForName(const Aws::String& name);

    AWS_S3_API Aws::String GetNameForSelectObjectContentEventType(SelectObjectContentEventType value);
} // namespace SelectObjectContentEventMapper
} // namespace Model
} // namespace S3
} // namespace Aws
