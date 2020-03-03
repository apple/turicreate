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

#include <aws/core/utils/DateTime.h>

#include <aws/core/platform/Time.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <time.h>
#include <cassert>
#include <iostream>
#include <cstring>

static const char* CLASS_TAG = "DateTime";
static const char* RFC822_DATE_FORMAT_STR_MINUS_Z = "%a, %d %b %Y %H:%M:%S";
static const char* RFC822_DATE_FORMAT_STR_WITH_Z = "%a, %d %b %Y %H:%M:%S %Z";
static const char* ISO_8601_LONG_DATE_FORMAT_STR = "%Y-%m-%dT%H:%M:%SZ";

using namespace Aws::Utils;


std::tm CreateZeroedTm()
{
    std::tm timeStruct;
    timeStruct.tm_hour = 0;
    timeStruct.tm_isdst = -1;
    timeStruct.tm_mday = 0;
    timeStruct.tm_min = 0;
    timeStruct.tm_mon = 0;
    timeStruct.tm_sec = 0;
    timeStruct.tm_wday = 0;
    timeStruct.tm_yday = 0;
    timeStruct.tm_year = 0;

    return timeStruct;
}

//Get the 0-6 week day number from a string representing WeekDay. Case insensitive and will stop on abbreviation
static int GetWeekDayNumberFromStr(const char* timeString, size_t startIndex, size_t stopIndex)
{
    if(stopIndex - startIndex < 3)
    {
        return -1;
    }

    size_t index = startIndex;

    char c = timeString[index];
    char next = 0;

    //it's ugly but this should compile down to EXACTLY 3 comparisons and no memory allocations
    switch(c)
    {
    case 'S':
    case 's':
        next = timeString[++index];
        switch(next)
        {
        case 'A':
        case 'a':
            next = timeString[++index];
            switch (next)
            {
            case 'T':
            case 't':
                return 6;
            default:
                return -1;
            }
        case 'U':
        case 'u':
            next = timeString[++index];
            switch (next)
            {
            case 'N':
            case 'n':
                return 0;
            default:
                return -1;       
            }
        default:
            return -1;
        }
    case 'T':
    case 't':
        next = timeString[++index];
        switch (next)
        {
        case 'H':
        case 'h':
            next = timeString[++index];
            switch(next)
            {
            case 'U':
            case 'u':
                return 4;
            default:
                return -1;
            }
        case 'U':
        case 'u':
            next = timeString[++index];
            switch(next)
            {
            case 'E':
            case 'e':
                return 2;
            default:
                return -1;
            }
        default:
            return -1;
        }
    case 'M':
    case 'm':
        next = timeString[++index];
        switch(next)
        {
        case 'O':
        case 'o':
            next = timeString[++index];
            switch (next)
            {
            case 'N':
            case 'n':
                return 1;
            default:
                return -1;
            }
        default:
            return -1;
        }
    case 'W':
    case 'w':
        next = timeString[++index];
        switch (next)
        {
        case 'E':
        case 'e':
            next = timeString[++index];
            switch (next)
            {
            case 'D':
            case 'd':
                return 3;
            default:
                return -1;
            }
        default:
            return -1;
        }
    case 'F':
    case 'f':
        next = timeString[++index];
        switch (next)
        {
        case 'R':
        case 'r':
            next = timeString[++index];
            switch (next)
            {
            case 'I':
            case 'i':
                return 5;
            default:
                return -1;
            }
        default:
            return -1;
        }
    default:
        return -1;
    }
}

//Get the 0-11 monthy number from a string representing Month. Case insensitive and will stop on abbreviation
static int GetMonthNumberFromStr(const char* timeString, size_t startIndex, size_t stopIndex)
{
    if (stopIndex - startIndex < 3)
    {
        return -1;
    }

    size_t index = startIndex;

    char c = timeString[index];
    char next = 0;

    //it's ugly but this should compile down to EXACTLY 3 comparisons and no memory allocations
    switch (c)
    {
    case 'M':
    case 'm':
        next = timeString[++index];
        switch (next)
        {
        case 'A':
        case 'a':
            next = timeString[++index];
            switch (next)
            {
            case 'Y':
            case 'y':
                return 4;
            case 'R':
            case 'r':
                return 2;
            default:
                return -1;
            }        
        default:
            return -1;
        }
    case 'A':
    case 'a':
        next = timeString[++index];
        switch (next)
        {
        case 'P':
        case 'p':
            next = timeString[++index];
            switch (next)
            {
            case 'R':
            case 'r':
                return 3;
            default:
                return -1;
            }
        case 'U':
        case 'u':
            next = timeString[++index];
            switch (next)
            {
            case 'G':
            case 'g':
                return 7;
            default:
                return -1;
            }
        default:
            return -1;
        }
    case 'J':
    case 'j':
        next = timeString[++index];
        switch (next)
        {
        case 'A':
        case 'a':
            next = timeString[++index];
            switch (next)
            {
            case 'N':
            case 'n':
                return 0;
            default:
                return -1;
            }
        case 'U':
        case 'u':
            next = timeString[++index];
            switch (next)
            {
            case 'N':
            case 'n':
                return 5;
            case 'L':
            case 'l':
                return 6;
            default:
                return -1;
            }
        default:
            return -1;
        }
    case 'F':
    case 'f':
        next = timeString[++index];
        switch (next)
        {
        case 'E':
        case 'e':
            next = timeString[++index];
            switch (next)
            {
            case 'B':
            case 'b':
                return 1;
            default:
                return -1;
            }
        default:
            return -1;
        }
    case 'S':
    case 's':
        next = timeString[++index];
        switch (next)
        {
        case 'E':
        case 'e':
            next = timeString[++index];
            switch (next)
            {
            case 'P':
            case 'p':
                return 8;
            default:
                return -1;
            }
        default:
            return -1;
        }
    case 'O':
    case 'o':
        next = timeString[++index];
        switch (next)
        {
        case 'C':
        case 'c':
            next = timeString[++index];
            switch (next)
            {
            case 'T':
            case 't':
                return 9;
            default:
                return -1;
            }
        default:
            return -1;
        }
    case 'N':
    case 'n':
        next = timeString[++index];
        switch (next)
        {
        case 'O':
        case 'o':
            next = timeString[++index];
            switch (next)
            {
            case 'V':
            case 'v':
                return 10;
            default:
                return -1;
            }
        default:
            return -1;
        }
    case 'D':
    case 'd':
        next = timeString[++index];
        switch (next)
        {
        case 'E':
        case 'e':
            next = timeString[++index];
            switch (next)
            {
            case 'C':
            case 'c':
                return 11;
            default:
                return -1;
            }
        default:
            return -1;
        }
    default:
        return -1;
    }
}

//Detects whether or not the passed in timezone string is a UTC zone.
static bool IsUtcTimeZone(const char* str)
{
    size_t len = strlen(str);
    if (len < 3)
    {
        return false;
    }

    int index = 0;
    char c = str[index];
    switch (c)
    {
    case 'U':
    case 'u':
        c = str[++index];
        switch(c)
        {
        case 'T':
        case 't':
            c = str[++index];
            switch(c)
            {
            case 'C':
            case 'c':
                return true;
            default:
                return false;
            }

        case 'C':
        case 'c':
            c = str[++index];           
            switch (c)
            {
            case 'T':
            case 't':
                return true;
            default:
                return false;
            }
        default:
            return false;
        }
    case 'G':
    case 'g':
        c = str[++index];
        switch (c)
        {
        case 'M':
        case 'm':
            c = str[++index];
            switch (c)
            {
            case 'T':
            case 't':
                return true;
            default:
                return false;
            }
        default:
            return false;
        }
    case '+':
    case '-':
        c = str[++index];
        switch (c)
        {
        case '0':
            c = str[++index];
            switch (c)
            {
            case '0':
                c = str[++index];
                switch (c)
                {
                case '0':
                    return true;
                default:
                    return false;
                }
            default:
                return false;
            }
        default:
            return false;
        }
    case 'Z':
        return true;
    default:
        return false;
    }
    
}

class DateParser
{
public:
    DateParser(const char* toParse) : m_error(false), m_toParse(toParse), m_utcAssumed(true)
    {
        m_parsedTimestamp = CreateZeroedTm();
        memset(m_tz, 0, 5);
    }

    virtual ~DateParser() = default;

    virtual void Parse() = 0;
    bool WasParseSuccessful() const { return !m_error; }
    std::tm& GetParsedTimestamp() { return m_parsedTimestamp; }
    bool ShouldIAssumeThisIsUTC() const { return m_utcAssumed; }
    const char* GetParsedTimezone() const { return m_tz; }

protected:
    bool m_error;
    const char* m_toParse;
    std::tm m_parsedTimestamp;
    bool m_utcAssumed;
    char m_tz[5];
};

static const int MAX_LEN = 100;

//Before you send me hate mail because I'm doing this manually, I encourage you to try using std::get_time on all platforms and getting
//uniform results. Timezone information doesn't parse on Windows and it hardly even works on GCC 4.9.x. This is the only way to make sure
//the standard is parsed correctly. strptime isn't available one Windows. This code gets hit pretty hard during http serialization/deserialization
//as a result I'm going for no dynamic allocations and linear complexity
class RFC822DateParser : public DateParser
{
public:
    RFC822DateParser(const char* toParse) : DateParser(toParse), m_state(0) 
    { 
    }

    /**
     * Really simple state machine for the format %a, %d %b %Y %H:%M:%S %Z
     */
    void Parse() override
    {
        size_t len = strlen(m_toParse);        

        //DOS check
        if (len > MAX_LEN)
        {
            AWS_LOGSTREAM_WARN(CLASS_TAG, "Incoming String to parse too long with len " << len)  
            m_error = true;         
            return;
        }

        size_t index = 0;
        size_t stateStartIndex = 0;
        int finalState = 8;

        while(m_state <= finalState && !m_error && index < len)
        {
            char c = m_toParse[index];

            switch (m_state)
            {
                case 0:
                    if(c == ',')
                    {                        
                        int weekNumber = GetWeekDayNumberFromStr(m_toParse, stateStartIndex, index + 1);

                        if (weekNumber > -1)
                        {
                            m_state = 1;
                            stateStartIndex = index + 1;
                            m_parsedTimestamp.tm_wday = weekNumber;
                        }
                        else
                        {
                            m_error = true;
                        }
                    }
                    else if(!isalpha(c))
                    {
                        m_error = true;
                    }
                    break;
                case 1:                    
                    if (isspace(c))
                    {
                        m_state = 2; 
                        stateStartIndex = index + 1;
                    }
                    else
                    {
                        m_error = true;
                    }
                    break;
                case 2:
                    if (isdigit(c))
                    {
                        m_parsedTimestamp.tm_mday = m_parsedTimestamp.tm_mday * 10 + (c - '0');
                    }
                    else if(isspace(c))
                    {
                        m_state = 3;
                        stateStartIndex = index + 1;
                    }
                    else
                    {
                        m_error = true;
                    }
                    break;
                case 3:
                    if (isspace(c))
                    {
                        int monthNumber = GetMonthNumberFromStr(m_toParse, stateStartIndex, index + 1);

                        if (monthNumber > -1)
                        {
                            m_state = 4;
                            stateStartIndex = index + 1;
                            m_parsedTimestamp.tm_mon = monthNumber;
                        }
                        else
                        {
                            m_error = true;
                        }
                    }
                    else if (!isalpha(c))
                    {
                        m_error = true;
                    }
                    break;
                case 4:
                    if (isspace(c) && index - stateStartIndex == 4)
                    {
                        m_state = 5;
                        stateStartIndex = index + 1;
                        m_parsedTimestamp.tm_year -= 1900;
                    }
                    else if (isspace(c) && index - stateStartIndex == 2)
                    {
                        m_state = 5;
                        stateStartIndex = index + 1;
                        m_parsedTimestamp.tm_year += 2000 - 1900;
                    }
                    else if (isdigit(c))
                    {
                        m_parsedTimestamp.tm_year = m_parsedTimestamp.tm_year * 10 + (c - '0');
                    }
                    else
                    {
                        m_error = true;
                    }
                    break;
                case 5:
                    if(c == ':' && index - stateStartIndex == 2)
                    {
                        m_state = 6;
                        stateStartIndex = index + 1;
                    }
                    else if (isdigit(c))
                    {
                        m_parsedTimestamp.tm_hour = m_parsedTimestamp.tm_hour * 10 + (c - '0');
                    }
                    else
                    {
                        m_error = true;
                    }
                    break;
                case 6:
                    if (c == ':' && index - stateStartIndex == 2)
                    {
                        m_state = 7;
                        stateStartIndex = index + 1;
                    }
                    else if (isdigit(c))
                    {
                        m_parsedTimestamp.tm_min = m_parsedTimestamp.tm_min * 10 + (c - '0');
                    }
                    else
                    {
                        m_error = true;
                    }
                    break;
                case 7:
                    if (isspace(c) && index - stateStartIndex == 2)
                    {
                        m_state = 8;
                        stateStartIndex = index + 1;
                    }
                    else if (isdigit(c))
                    {
                        m_parsedTimestamp.tm_sec = m_parsedTimestamp.tm_sec * 10 + (c - '0');
                    }
                    else
                    {
                        m_error = true;
                    }
                    break;
                case 8:
                    if (isalpha(c) && (index - stateStartIndex) < 5)
                    {
                        m_tz[index - stateStartIndex] = c;
                    }                   
                    
                    break;
            }

            index++;
        }

        if (m_tz[0] != 0)
        {
           m_utcAssumed = IsUtcTimeZone(m_tz);
        }

        m_error = (m_error || m_state != finalState);
    }

    int GetState() const { return m_state; }

private:
    int m_state;
};

//Before you send me hate mail because I'm doing this manually, I encourage you to try using std::get_time on all platforms and getting
//uniform results. Timezone information doesn't parse on Windows and it hardly even works on GCC 4.9.x. This is the only way to make sure
//the standard is parsed correctly. strptime isn't available one Windows. This code gets hit pretty hard during http serialization/deserialization
//as a result I'm going for no dynamic allocations and linear complexity
class ISO_8601DateParser : public DateParser
{
public:
    ISO_8601DateParser(const char* stringToParse) : DateParser(stringToParse), m_state(0)
    {
    }

    //parses "%Y-%m-%dT%H:%M:%SZ or "%Y-%m-%dT%H:%M:%S.000Z"
    void Parse() override
    {
        size_t len = strlen(m_toParse);

        //DOS check
        if (len > MAX_LEN)
        {
            AWS_LOGSTREAM_WARN(CLASS_TAG, "Incoming String to parse too long with len " << len)
            m_error = true;
            return;
        }

        size_t index = 0;
        size_t stateStartIndex = 0;
        const int finalState = 7;

        while (m_state <= finalState && !m_error && index < len)
        {
            char c = m_toParse[index];
            switch (m_state)
            {
                case 0:
                    if (c == '-' && index - stateStartIndex == 4)
                    {
                        m_state = 1;
                        stateStartIndex = index + 1;
                        m_parsedTimestamp.tm_year -= 1900;
                    }
                    else if (isdigit(c))
                    {
                        m_parsedTimestamp.tm_year = m_parsedTimestamp.tm_year * 10 + (c - '0');
                    }
                    else
                    {
                        m_error = true;
                    }
                    break;
                case 1:
                    if (c == '-' && index - stateStartIndex == 2)
                    {
                        m_state = 2;
                        stateStartIndex = index + 1;
                        m_parsedTimestamp.tm_mon -= 1;
                    }
                    else if (isdigit(c))
                    {
                        m_parsedTimestamp.tm_mon = m_parsedTimestamp.tm_mon * 10 + (c - '0');
                    }
                    else
                    {
                        m_error = true;
                    }

                    break;
                case 2:
                    if (c == 'T' && index - stateStartIndex == 2)
                    {
                        m_state = 3;
                        stateStartIndex = index + 1;
                    }
                    else if (isdigit(c))
                    {
                        m_parsedTimestamp.tm_mday = m_parsedTimestamp.tm_mday * 10 + (c - '0');
                    }
                    else
                    {
                        m_error = true;
                    }

                    break;
                case 3:
                    if (c == ':' && index - stateStartIndex == 2)
                    {
                        m_state = 4;
                        stateStartIndex = index + 1;
                    }
                    else if (isdigit(c))
                    {
                        m_parsedTimestamp.tm_hour = m_parsedTimestamp.tm_hour * 10 + (c - '0');
                    }
                    else
                    {
                        m_error = true;
                    }

                    break;
                case 4:
                    if (c == ':' && index - stateStartIndex == 2)
                    {
                        m_state = 5;
                        stateStartIndex = index + 1;
                    }
                    else if (isdigit(c))
                    {
                        m_parsedTimestamp.tm_min = m_parsedTimestamp.tm_min * 10 + (c - '0');
                    }
                    else
                    {
                        m_error = true;
                    }

                    break;
                case 5:
                    if (c == 'Z' && index - stateStartIndex == 2)
                    {
                        m_state = finalState;
                        stateStartIndex = index + 1;
                    }
                    else if (c == '.' && index - stateStartIndex == 2)
                    {
                        m_state = 6;
                        stateStartIndex = index + 1;
                    }
                    else if (isdigit(c))
                    {
                        m_parsedTimestamp.tm_sec = m_parsedTimestamp.tm_sec * 10 + (c - '0');
                    }
                    else
                    {
                        m_error = true;
                    }

                    break;
                case 6:
                    if (c == 'Z')
                    {
                        m_state = finalState;
                        stateStartIndex = index + 1;
                    }
                    else if(!isdigit(c))
                    {
                        m_error = true;
                    }
                    break;
            }
            index++;
        }

        m_error = (m_error || m_state != finalState);
    }


private:
    int m_state;
};

DateTime::DateTime(const std::chrono::system_clock::time_point& timepointToAssign) : m_time(timepointToAssign), m_valid(true)
{   
}

DateTime::DateTime(int64_t millisSinceEpoch) : m_valid(true)
{
    std::chrono::duration<int64_t, std::chrono::milliseconds::period> timestamp(millisSinceEpoch);
    m_time = std::chrono::system_clock::time_point(timestamp);
}

DateTime::DateTime(double epoch_millis) : m_valid(true)
{
    std::chrono::duration<double, std::chrono::seconds::period> timestamp(epoch_millis);
    m_time = std::chrono::system_clock::time_point(std::chrono::duration_cast<std::chrono::milliseconds>(timestamp));
}

DateTime::DateTime(const Aws::String& timestamp, DateFormat format) : m_valid(true)
{
    ConvertTimestampStringToTimePoint(timestamp.c_str(), format);
}

DateTime::DateTime(const char* timestamp, DateFormat format) : m_valid(true)
{
    ConvertTimestampStringToTimePoint(timestamp, format);
}

DateTime::DateTime() : m_valid(true)
{
    //init time_point to default by doing nothing.
}

DateTime& DateTime::operator=(const Aws::String& timestamp)
{
    *this = DateTime(timestamp, DateFormat::AutoDetect);
    return *this;
}

DateTime& DateTime::operator=(double secondsMillis)
{
    *this = DateTime(secondsMillis);
    return *this;
}

DateTime& DateTime::operator=(int64_t millisSinceEpoch)
{
    *this = DateTime(millisSinceEpoch);
    return *this;
}

DateTime& DateTime::operator=(const std::chrono::system_clock::time_point& timepointToAssign)
{
    *this = DateTime(timepointToAssign);
    return *this;
}

bool DateTime::operator == (const DateTime& other) const
{
    return m_time == other.m_time;
}

bool DateTime::operator < (const DateTime& other) const
{
    return m_time < other.m_time;
}

bool DateTime::operator > (const DateTime& other) const
{
    return m_time > other.m_time;
}

bool DateTime::operator != (const DateTime& other) const
{
    return m_time != other.m_time;
}

bool DateTime::operator <= (const DateTime& other) const
{
    return m_time <= other.m_time;
}

bool DateTime::operator >= (const DateTime& other) const
{
    return m_time >= other.m_time;
}

DateTime DateTime::operator +(const std::chrono::milliseconds& a) const
{
    auto timepointCpy = m_time;
    timepointCpy += a;
    return DateTime(timepointCpy);
}

DateTime DateTime::operator -(const std::chrono::milliseconds& a) const
{
    auto timepointCpy = m_time;
    timepointCpy -= a;
    return DateTime(timepointCpy);
}

Aws::String DateTime::ToLocalTimeString(DateFormat format) const
{
    switch (format)
    {
    case DateFormat::ISO_8601:
        return ToLocalTimeString(ISO_8601_LONG_DATE_FORMAT_STR);
    case DateFormat::RFC822:
        return ToLocalTimeString(RFC822_DATE_FORMAT_STR_WITH_Z);   
    default:
        assert(0);
        return "";
    }
}

Aws::String DateTime::ToLocalTimeString(const char* formatStr) const
{
    struct tm localTimeStamp = ConvertTimestampToLocalTimeStruct();

    char formattedString[100];
    std::strftime(formattedString, sizeof(formattedString), formatStr, &localTimeStamp);
    return formattedString;
}

Aws::String DateTime::ToGmtString(DateFormat format) const
{
    switch (format)
    {
    case DateFormat::ISO_8601:
        return ToGmtString(ISO_8601_LONG_DATE_FORMAT_STR);
    case DateFormat::RFC822:
    {
        //Windows erronously drops the local timezone in for %Z
        Aws::String rfc822GmtString = ToGmtString(RFC822_DATE_FORMAT_STR_MINUS_Z);
        rfc822GmtString += " GMT";
        return rfc822GmtString;
    }
    default:
        assert(0);
        return "";
    }
}

Aws::String DateTime::ToGmtString(const char* formatStr) const
{
    struct tm gmtTimeStamp = ConvertTimestampToGmtStruct();

    char formattedString[100];
    std::strftime(formattedString, sizeof(formattedString), formatStr, &gmtTimeStamp);
    return formattedString;
}

double DateTime::SecondsWithMSPrecision() const
{
    std::chrono::duration<double, std::chrono::seconds::period> timestamp(m_time.time_since_epoch());
    return timestamp.count();
}

int64_t DateTime::Millis() const
{
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(m_time.time_since_epoch());
    return timestamp.count();
}

std::chrono::system_clock::time_point DateTime::UnderlyingTimestamp() const
{
    return m_time;
}

int DateTime::GetYear(bool localTime) const
{
    return GetTimeStruct(localTime).tm_year + 1900;
}

Month DateTime::GetMonth(bool localTime) const
{
    return static_cast<Aws::Utils::Month>(GetTimeStruct(localTime).tm_mon);
}

int DateTime::GetDay(bool localTime) const
{
    return GetTimeStruct(localTime).tm_mday;
}

DayOfWeek DateTime::GetDayOfWeek(bool localTime) const
{
    return static_cast<Aws::Utils::DayOfWeek>(GetTimeStruct(localTime).tm_wday);
}

int DateTime::GetHour(bool localTime) const
{
    return GetTimeStruct(localTime).tm_hour;
}

int DateTime::GetMinute(bool localTime) const
{
    return GetTimeStruct(localTime).tm_min;
}

int DateTime::GetSecond(bool localTime) const
{
    return GetTimeStruct(localTime).tm_sec;
}

bool DateTime::IsDST(bool localTime) const
{
    return GetTimeStruct(localTime).tm_isdst == 0 ? false : true;
}

DateTime DateTime::Now()
{
    DateTime dateTime;
    dateTime.m_time = std::chrono::system_clock::now();
    return dateTime;
}

int64_t DateTime::CurrentTimeMillis()
{
    return Now().Millis();
}

Aws::String DateTime::CalculateLocalTimestampAsString(const char* formatStr)
{
   DateTime now = Now();
   return now.ToLocalTimeString(formatStr);
}

Aws::String DateTime::CalculateGmtTimestampAsString(const char* formatStr)
{
    DateTime now = Now();
    return now.ToGmtString(formatStr);
}

Aws::String DateTime::CalculateGmtTimeWithMsPrecision()
{
    auto now = DateTime::Now();
    struct tm gmtTimeStamp = now.ConvertTimestampToGmtStruct();

    char formattedString[100];
    auto len = std::strftime(formattedString, sizeof(formattedString), "%Y-%m-%d %H:%M:%S", &gmtTimeStamp);
    if (len)
    {
        auto ms = now.Millis();
        ms = ms - ms / 1000 * 1000; // calculate the milliseconds as fraction.
        formattedString[len++] = '.';
        int divisor = 100;
        while(divisor)
        {
            auto digit = ms / divisor;
            formattedString[len++] = char('0' + digit);
            ms = ms - divisor * digit;
            divisor /= 10;
        }
        formattedString[len] = '\0';
    }
    return formattedString;
}

int DateTime::CalculateCurrentHour()
{
    return Now().GetHour(true);
}

double DateTime::ComputeCurrentTimestampInAmazonFormat()
{
   return Now().SecondsWithMSPrecision();
}

std::chrono::milliseconds DateTime::Diff(const DateTime& a, const DateTime& b)
{
    auto diff = a.m_time - b.m_time;
    return std::chrono::duration_cast<std::chrono::milliseconds>(diff);
}

std::chrono::milliseconds DateTime::operator-(const DateTime& other) const
{
    auto diff = this->m_time - other.m_time;
    return std::chrono::duration_cast<std::chrono::milliseconds>(diff);
}

void DateTime::ConvertTimestampStringToTimePoint(const char* timestamp, DateFormat format)
{  
    std::tm timeStruct;
    bool isUtc = true;

    switch (format)
    {
    case DateFormat::RFC822:
    {
        RFC822DateParser parser(timestamp);
        parser.Parse();
        m_valid = parser.WasParseSuccessful();
        isUtc = parser.ShouldIAssumeThisIsUTC();
        timeStruct = parser.GetParsedTimestamp();
        break;
    }
    case DateFormat::ISO_8601:
    {
        ISO_8601DateParser parser(timestamp);
        parser.Parse();
        m_valid = parser.WasParseSuccessful();
        isUtc = parser.ShouldIAssumeThisIsUTC();
        timeStruct = parser.GetParsedTimestamp();
        break;      
    }
    case DateFormat::AutoDetect:
    {
        RFC822DateParser rfcParser(timestamp);
        rfcParser.Parse();
        if(rfcParser.WasParseSuccessful())
        {
            m_valid = true;
            isUtc = rfcParser.ShouldIAssumeThisIsUTC();
            timeStruct = rfcParser.GetParsedTimestamp();
            break;
        }
        ISO_8601DateParser isoParser(timestamp);
        isoParser.Parse();
        if (isoParser.WasParseSuccessful())
        {
            m_valid = true;
            isUtc = isoParser.ShouldIAssumeThisIsUTC();
            timeStruct = isoParser.GetParsedTimestamp();
            break;
        }
        m_valid = false;
        break;
    }
    default:       
        assert(0);
    }    
  
    if (m_valid)
    {        
        std::time_t tt;
        if(isUtc)
        {
            tt = Aws::Time::TimeGM(&timeStruct);
        }
        else
        {
            assert(0);
            AWS_LOGSTREAM_WARN(CLASS_TAG, "Non-UTC timestamp detected. This is always a bug. Make the world a better place and fix whatever sent you this timestamp: " << timestamp)
            tt = std::mktime(&timeStruct);
        }
        m_time = std::chrono::system_clock::from_time_t(tt);
    }    
}

tm DateTime::GetTimeStruct(bool localTime) const
{
    return localTime ? ConvertTimestampToLocalTimeStruct() : ConvertTimestampToGmtStruct();
}

tm DateTime::ConvertTimestampToLocalTimeStruct() const
{
    std::time_t time = std::chrono::system_clock::to_time_t(m_time);
    struct tm localTimeStamp;

    Aws::Time::LocalTime(&localTimeStamp, time);

    return localTimeStamp;
}

tm DateTime::ConvertTimestampToGmtStruct() const
{
    std::time_t time = std::chrono::system_clock::to_time_t(m_time);
    struct tm gmtTimeStamp;
    Aws::Time::GMTime(&gmtTimeStamp, time);

    return gmtTimeStamp;
}
