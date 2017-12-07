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

#include <aws/core/utils/json/JsonSerializer.h>

using namespace Aws::Utils;
using namespace Aws::Utils::Json;

JsonValue::JsonValue() : m_wasParseSuccessful(true)
{
}

JsonValue::JsonValue(const Aws::String& value) : m_wasParseSuccessful(true)
{
    Aws::External::Json::Reader reader;

    if (!reader.parse(value, m_value))
    {
        m_wasParseSuccessful = false;
        m_errorMessage = reader.getFormattedErrorMessages();
    }
}

JsonValue::JsonValue(Aws::IStream& istream) : m_wasParseSuccessful(true)
{
    Aws::External::Json::Reader reader;

    if (!reader.parse(istream, m_value))
    {
        m_wasParseSuccessful = false;
        m_errorMessage = reader.getFormattedErrorMessages();
    }
}

JsonValue::JsonValue(const JsonValue& value)
{
    AsObject(value);
}

JsonValue::JsonValue(JsonValue&& value) : m_value(std::move(value.m_value))
{
}

JsonValue::~JsonValue()
{
}

JsonValue& JsonValue::operator=(const JsonValue& other)
{
    if (this == &other)
    {
        return *this;
    }

    return AsObject(other);

}

JsonValue& JsonValue::operator=(JsonValue&& other)
{
    if (this == &other)
    {
        return *this;
    }

    return AsObject(other);
}

JsonValue::JsonValue(const Aws::External::Json::Value& value)
{
    m_value = value;
}

JsonValue& JsonValue::operator=(Aws::External::Json::Value& other)
{
    m_value = other;
    return *this;
}

Aws::String JsonValue::GetString(const char* key) const
{
    return m_value[key].asString();
}

Aws::String JsonValue::GetString(const Aws::String& key) const
{
    return GetString(key.c_str());
}

JsonValue& JsonValue::WithString(const char* key, const Aws::String& value)
{
    m_value[key] = value;
    return *this;
}

JsonValue& JsonValue::WithString(const Aws::String& key, const Aws::String& value)
{
    return WithString(key.c_str(), value);
}

JsonValue& JsonValue::AsString(const Aws::String& value)
{
    m_value = value;
    return *this;
}

Aws::String JsonValue::AsString() const
{
    Aws::String stringValue = m_value.asString();
    return stringValue;
}

bool JsonValue::GetBool(const char* key) const
{
    return m_value[key].asBool();
}

bool JsonValue::GetBool(const Aws::String& key) const
{
    return GetBool(key.c_str());
}

JsonValue& JsonValue::WithBool(const char* key, bool value)
{
    m_value[key] = value;
    return *this;
}

JsonValue& JsonValue::WithBool(const Aws::String& key, bool value)
{
    return WithBool(key.c_str(), value);
}

JsonValue& JsonValue::AsBool(bool value)
{
    m_value = value;
    return *this;
}

bool JsonValue::AsBool() const
{
    return m_value.asBool();
}

int JsonValue::GetInteger(const char* key) const
{
    return m_value[key].asInt();
}

int JsonValue::GetInteger(const Aws::String& key) const
{
    return GetInteger(key.c_str());
}

JsonValue& JsonValue::WithInteger(const char* key, int value)
{
    m_value[key] = value;
    return *this;
}

JsonValue& JsonValue::WithInteger(const Aws::String& key, int value)
{
    return WithInteger(key.c_str(), value);
}

JsonValue& JsonValue::AsInteger(int value)
{
    m_value = value;
    return *this;
}

int JsonValue::AsInteger() const
{
    return m_value.asInt();
}

long long JsonValue::GetInt64(const char* key) const
{
    return m_value[key].asLargestInt();
}

long long JsonValue::GetInt64(const Aws::String& key) const
{
    return GetInt64(key.c_str());
}

JsonValue& JsonValue::WithInt64(const char* key, long long value)
{
    m_value[key] = value;
    return *this;
}

JsonValue& JsonValue::WithInt64(const Aws::String& key, long long value)
{
    return WithInt64(key.c_str(), value);
}

JsonValue& JsonValue::AsInt64(long long value)
{
    m_value = value;
    return *this;
}

long long JsonValue::AsInt64() const
{
    return m_value.asLargestInt();
}

double JsonValue::GetDouble(const char* key) const
{
    return m_value[key].asDouble();
}

double JsonValue::GetDouble(const Aws::String& key) const
{
    return GetDouble(key.c_str());
}

JsonValue& JsonValue::WithDouble(const char* key, double value)
{
    m_value[key] = value;
    return *this;
}

JsonValue& JsonValue::WithDouble(const Aws::String& key, double value)
{
    return WithDouble(key.c_str(), value);
}

JsonValue& JsonValue::AsDouble(double value)
{
    m_value = value;
    return *this;
}

double JsonValue::AsDouble() const
{
    return m_value.asDouble();
}

Array<JsonValue> JsonValue::GetArray(const char* key) const
{
    Array<JsonValue> returnArray(m_value[key].size());

    for (unsigned i = 0; i < returnArray.GetLength(); ++i)
    {
        returnArray[i] = m_value[key][i];
    }

    return returnArray;
}

Array<JsonValue> JsonValue::GetArray(const Aws::String& key) const
{
    return GetArray(key.c_str());
}

JsonValue& JsonValue::WithArray(const char* key, const Array<Aws::String>& array)
{
    Aws::External::Json::Value arrayValue;
    for (unsigned i = 0; i < array.GetLength(); ++i)
    {
        arrayValue.append(array[i]);
    }

    m_value[key] = arrayValue;

    return *this;
}

JsonValue& JsonValue::WithArray(const Aws::String& key, const Array<Aws::String>& array)
{
    return WithArray(key.c_str(), array);
}

JsonValue& JsonValue::WithArray(const Aws::String& key, Array<Aws::String>&& array)
{
    Aws::External::Json::Value arrayValue;
    for (unsigned i = 0; i < array.GetLength(); ++i)
        arrayValue.append(std::move(array[i]));

    m_value[key] = std::move(arrayValue);

    return *this;
}

JsonValue& JsonValue::WithArray(const Aws::String& key, const Array<JsonValue>& array)
{
    Aws::External::Json::Value arrayValue;
    for (unsigned i = 0; i < array.GetLength(); ++i)
        arrayValue.append(array[i].m_value);

    m_value[key] = arrayValue;

    return *this;
}

JsonValue& JsonValue::WithArray(const Aws::String& key, Array<JsonValue>&& array)
{
    Aws::External::Json::Value arrayValue;
    for (unsigned i = 0; i < array.GetLength(); ++i)
        arrayValue.append(std::move(array[i].m_value));

    m_value[key] = std::move(arrayValue);

    return *this;
}

void JsonValue::AppendValue(const JsonValue& value)
{
    m_value.append(value.m_value);
}

JsonValue& JsonValue::AsArray(const Array<JsonValue>& array)
{
    Aws::External::Json::Value arrayValue;
    for (unsigned i = 0; i < array.GetLength(); ++i)
        arrayValue.append(array[i].m_value);

    m_value = arrayValue;
    return *this;
}

JsonValue& JsonValue::AsArray(Array<JsonValue> && array)
{
    Aws::External::Json::Value arrayValue;
    for (unsigned i = 0; i < array.GetLength(); ++i)
        arrayValue.append(std::move(array[i].m_value));

    m_value = std::move(arrayValue);
    return *this;
}

Array<JsonValue> JsonValue::AsArray() const
{
    Array<JsonValue> returnArray(m_value.size());

    for (unsigned i = 0; i < returnArray.GetLength(); ++i)
    {
        returnArray[i] = m_value[i];
    }

    return returnArray;
}

JsonValue JsonValue::GetObject(const char* key) const
{
    return m_value[key];
}

JsonValue JsonValue::GetObject(const Aws::String& key) const
{
    return GetObject(key.c_str());
}

JsonValue& JsonValue::WithObject(const char* key, const JsonValue& value)
{
    m_value[key] = value.m_value;
    return *this;
}

JsonValue& JsonValue::WithObject(const Aws::String& key, const JsonValue& value)
{
    return WithObject(key.c_str(), value);
}

JsonValue& JsonValue::WithObject(const char* key, const JsonValue&& value)
{
    m_value[key] = std::move(value.m_value);
    return *this;
}

JsonValue& JsonValue::WithObject(const Aws::String& key, const JsonValue&& value)
{
    return WithObject(key.c_str(), std::move(value));
}

JsonValue& JsonValue::AsObject(const JsonValue& value)
{
    m_value = value.m_value;
    return *this;
}

JsonValue& JsonValue::AsObject(JsonValue && value)
{
    m_value = std::move(value.m_value);
    return *this;
}

JsonValue JsonValue::AsObject() const
{   
    return m_value;
}

Aws::Map<Aws::String, JsonValue> JsonValue::GetAllObjects() const
{
    Aws::Map<Aws::String, JsonValue> valueMap;

    for (Aws::External::Json::ValueIterator iter = m_value.begin(); iter != m_value.end(); ++iter)
    {
        valueMap[iter.key().asString()] = *iter;
    }

    return valueMap;
}

bool JsonValue::ValueExists(const char* key) const
{
    return m_value.isMember(key);
}

bool JsonValue::ValueExists(const Aws::String& key) const
{
    return ValueExists(key.c_str());
}

bool JsonValue::IsObject() const
{
    return m_value.isObject();
}

bool JsonValue::IsBool() const
{
    return m_value.isBool();
}

bool JsonValue::IsString() const
{
    return m_value.isString();
}

bool JsonValue::IsIntegerType() const
{
    return m_value.isIntegral();
}

bool JsonValue::IsFloatingPointType() const
{
    return m_value.isDouble();
}

bool JsonValue::IsListType() const
{
    return m_value.isArray();
}

Aws::String JsonValue::WriteCompact(bool treatAsObject) const
{
    if (treatAsObject && m_value.isNull())
    {
        return "{}";
    }

    Aws::External::Json::FastWriter fastWriter;
    return fastWriter.write(m_value);
}

void JsonValue::WriteCompact(Aws::OStream& ostream, bool treatAsObject) const
{
    if (treatAsObject && m_value.isNull())
    {
        ostream << "{}";
        return;
    }

    Aws::String compactString = WriteCompact();
    ostream.write(compactString.c_str(), compactString.length());
}

Aws::String JsonValue::WriteReadable(bool treatAsObject) const
{
    if (treatAsObject && m_value.isNull())
    {
        return "{\n}\n";
    }

    Aws::External::Json::StyledWriter styledWriter;
    return styledWriter.write(m_value);
}

void JsonValue::WriteReadable(Aws::OStream& ostream, bool treatAsObject) const
{
    if (treatAsObject && m_value.isNull())
    {
        ostream <<  "{\n}\n";
    }

    Aws::External::Json::StyledStreamWriter styledStreamWriter;
    styledStreamWriter.write(ostream, m_value);
}
