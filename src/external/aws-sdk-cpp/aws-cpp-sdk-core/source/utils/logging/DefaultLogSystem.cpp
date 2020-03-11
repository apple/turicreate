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


#include <aws/core/utils/logging/DefaultLogSystem.h>

#include <aws/core/utils/DateTime.h>
#include <aws/core/utils/memory/stl/AWSVector.h>

#include <fstream>

using namespace Aws::Utils;
using namespace Aws::Utils::Logging;

static const char* AllocationTag = "DefaultLogSystem";
static const int BUFFERED_MSG_COUNT = 100;

static std::shared_ptr<Aws::OFStream> MakeDefaultLogFile(const Aws::String& filenamePrefix)
{
    Aws::String newFileName = filenamePrefix + DateTime::CalculateGmtTimestampAsString("%Y-%m-%d-%H") + ".log";
    return Aws::MakeShared<Aws::OFStream>(AllocationTag, newFileName.c_str(), Aws::OFStream::out | Aws::OFStream::app);
}

static void LogThread(DefaultLogSystem::LogSynchronizationData* syncData, const std::shared_ptr<Aws::OStream>& logFile, const Aws::String& filenamePrefix, bool rollLog)
{
    // localtime requires access to env. variables to get Timezone, which is not thread-safe
    int32_t lastRolledHour = DateTime::Now().GetHour(false /*localtime*/);
    std::shared_ptr<Aws::OStream> log = logFile;

    for(;;)
    {
        std::unique_lock<std::mutex> locker(syncData->m_logQueueMutex);
        syncData->m_queueSignal.wait(locker, [&](){ return syncData->m_stopLogging == true || syncData->m_queuedLogMessages.size() > 0; } );

        if (syncData->m_stopLogging && syncData->m_queuedLogMessages.size() == 0)
        {
            break;
        }

        Aws::Vector<Aws::String> messages(std::move(syncData->m_queuedLogMessages));
        syncData->m_queuedLogMessages.reserve(BUFFERED_MSG_COUNT);

        locker.unlock();

        if (messages.size() > 0)
        {
            if (rollLog)
            {
                // localtime requires access to env. variables to get Timezone, which is not thread-safe
                int32_t currentHour = DateTime::Now().GetHour(false /*localtime*/); 
                if (currentHour != lastRolledHour)
                {
                    log = MakeDefaultLogFile(filenamePrefix);
                    lastRolledHour = currentHour;
                }
            }

            for (const auto& msg : messages)
            {
                (*log) << msg;
            }

            log->flush();
        }
    }
}

DefaultLogSystem::DefaultLogSystem(LogLevel logLevel, const std::shared_ptr<Aws::OStream>& logFile) :
    Base(logLevel),
    m_syncData(),
    m_loggingThread()
{
    m_loggingThread = std::thread(LogThread, &m_syncData, logFile, "", false);
}

DefaultLogSystem::DefaultLogSystem(LogLevel logLevel, const Aws::String& filenamePrefix) :
    Base(logLevel),
    m_syncData(),
    m_loggingThread()
{
    m_loggingThread = std::thread(LogThread, &m_syncData, MakeDefaultLogFile(filenamePrefix), filenamePrefix, true);
}

DefaultLogSystem::~DefaultLogSystem()
{
    {
        std::lock_guard<std::mutex> locker(m_syncData.m_logQueueMutex);
        m_syncData.m_stopLogging = true;
    }

    m_syncData.m_queueSignal.notify_one();

    m_loggingThread.join();
}

void DefaultLogSystem::ProcessFormattedStatement(Aws::String&& statement)
{
    std::unique_lock<std::mutex> locker(m_syncData.m_logQueueMutex);
    m_syncData.m_queuedLogMessages.emplace_back(std::move(statement));
    if(m_syncData.m_queuedLogMessages.size() >= BUFFERED_MSG_COUNT)
    {
        locker.unlock();
        m_syncData.m_queueSignal.notify_one();
    }
    else
    {
        locker.unlock();
    }
}

void DefaultLogSystem::Flush()
{
    m_syncData.m_queueSignal.notify_one();
}

