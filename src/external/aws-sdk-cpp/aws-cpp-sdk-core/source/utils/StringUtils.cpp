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


#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>
#include <algorithm>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <functional>

using namespace Aws::Utils;

void StringUtils::Replace(Aws::String& s, const char* search, const char* replace)
{
    if(!search || !replace)
    {
        return;
    }

    size_t replaceLength = strlen(replace);
    size_t searchLength = strlen(search);

    for (std::size_t pos = 0;; pos += replaceLength)
    {
        pos = s.find(search, pos);
        if (pos == Aws::String::npos)
            break;

        s.erase(pos, searchLength);
        s.insert(pos, replace);
    }
}


Aws::String StringUtils::ToLower(const char* source)
{
    Aws::String copy;
    size_t sourceLength = strlen(source);
    copy.resize(sourceLength);
    std::transform(source, source + sourceLength, copy.begin(), ::tolower);

    return copy;
}


Aws::String StringUtils::ToUpper(const char* source)
{
    Aws::String copy;
    size_t sourceLength = strlen(source);
    copy.resize(sourceLength);
    std::transform(source, source + sourceLength, copy.begin(), ::toupper);

    return copy;
}


bool StringUtils::CaselessCompare(const char* value1, const char* value2)
{
    Aws::String value1Lower = ToLower(value1);
    Aws::String value2Lower = ToLower(value2);

    return value1Lower == value2Lower;
}


Aws::Vector<Aws::String> StringUtils::Split(const Aws::String& toSplit, char splitOn)
{
    Aws::StringStream input(toSplit);
    Aws::Vector<Aws::String> returnValues;
    Aws::String item;

    while(std::getline(input, item, splitOn))
    {
        if(item.size() > 0)
        {
            returnValues.push_back(item);
        }
    }

    return returnValues;
}


Aws::Vector<Aws::String> StringUtils::SplitOnLine(const Aws::String& toSplit)
{
    Aws::StringStream input(toSplit);
    Aws::Vector<Aws::String> returnValues;
    Aws::String item;

    while (std::getline(input, item))
    {
        if (item.size() > 0)
        {
            returnValues.push_back(item);
        }
    }

    return returnValues;
}


Aws::String StringUtils::URLEncode(const char* unsafe)
{
    Aws::StringStream escaped;
    escaped.fill('0');
    escaped << std::hex << std::uppercase;

    size_t unsafeLength = strlen(unsafe);
    for (auto i = unsafe, n = unsafe + unsafeLength; i != n; ++i)
    {
        int c = *i;
		//MSVC 2015 has an assertion that c is positive in isalnum(). This breaks unicode support.
		//bypass that with the first check.
        if (c >= 0 && (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~'))
        {
            escaped << (char)c;
        }
        else
        {
            //this unsigned char cast allows us to handle unicode characters.
            escaped << '%' << std::setw(2) << int((unsigned char)c) << std::setw(0);
        }
    }

    return escaped.str();
}

Aws::String StringUtils::UTF8Escape(const char* unicodeString, const char* delimiter)
{
    Aws::StringStream escaped;
    escaped.fill('0');
    escaped << std::hex << std::uppercase;

    size_t unsafeLength = strlen(unicodeString);
    for (auto i = unicodeString, n = unicodeString + unsafeLength; i != n; ++i)
    {
        int c = *i;
        //MSVC 2015 has an assertion that c is positive in isalnum(). This breaks unicode support.
        //bypass that with the first check.
        if (c >= ' ' && c < 127 )
        {
            escaped << (char)c;
        }
        else
        {
            //this unsigned char cast allows us to handle unicode characters.
            escaped << delimiter << std::setw(2) << int((unsigned char)c) << std::setw(0);
        }
    }

    return escaped.str();
}

Aws::String StringUtils::URLEncode(double unsafe)
{
    char buffer[32];
#if defined(_MSC_VER) && _MSC_VER < 1900
    _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%g", unsafe);
#else
    snprintf(buffer, sizeof(buffer), "%g", unsafe);
#endif

    return StringUtils::URLEncode(buffer);
}


Aws::String StringUtils::URLDecode(const char* safe)
{
    Aws::StringStream unescaped;
    unescaped.fill('0');
    unescaped << std::hex;

    size_t safeLength = strlen(safe);
    for (auto i = safe, n = safe + safeLength; i != n; ++i)
    {
        char c = *i;
        if(c == '%')
        {
            char hex[3];
            hex[0] = *(i + 1);
            hex[1] = *(i + 2);
            hex[2] = 0;
            i += 2;
            auto hexAsInteger = strtol(hex, nullptr, 16);
            unescaped << (char)hexAsInteger;
        }
        else
        {
            unescaped << *i;
        }
    }

    return unescaped.str();
}

Aws::String StringUtils::LTrim(const char* source)
{
    Aws::String copy(source);
    copy.erase(copy.begin(), std::find_if(copy.begin(), copy.end(), std::not1(std::ptr_fun<int, int>(::isspace))));
    return copy;
}

// trim from end
Aws::String StringUtils::RTrim(const char* source)
{
    Aws::String copy(source);
    copy.erase(std::find_if(copy.rbegin(), copy.rend(), std::not1(std::ptr_fun<int, int>(::isspace))).base(), copy.end());
    return copy;
}

// trim from both ends
Aws::String StringUtils::Trim(const char* source)
{
    return LTrim(RTrim(source).c_str());
}

long long StringUtils::ConvertToInt64(const char* source)
{
    if(!source)
    {
        return 0;
    }

#ifdef __ANDROID__
    return atoll(source);
#else
    return std::atoll(source);
#endif // __ANDROID__
}


long StringUtils::ConvertToInt32(const char* source)
{
    if (!source)
    {
        return 0;
    }

    return std::atol(source);
}


bool StringUtils::ConvertToBool(const char* source)
{
    if(!source)
    {
        return false;
    }

    Aws::String strValue = ToLower(source);
    if(strValue == "true" || strValue == "1")
    {
        return true;
    }

    return false;
}


double StringUtils::ConvertToDouble(const char* source)
{
    if(!source)
    {
        return 0.0;
    }

    return std::strtod(source, NULL);
}

#ifdef _WIN32

Aws::WString StringUtils::ToWString(const char* source)
{
    Aws::WString outString;

    outString.resize(std::strlen(source));
    std::copy(source, source + std::strlen(source), outString.begin());
    return outString;
}

Aws::String StringUtils::FromWString(const wchar_t* source)
{
    Aws::WString inWString(source);

    Aws::String outString(inWString.begin(), inWString.end());
    return outString;
}

#endif


