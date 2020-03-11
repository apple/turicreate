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

#include <aws/core/utils/json/JsonSerializer.h>

#include <iterator>
#include <algorithm>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

using namespace Aws::Utils;
using namespace Aws::Utils::Json;

JsonValue::JsonValue() : m_wasParseSuccessful(true)
{
    m_value = nullptr;
}

JsonValue::JsonValue(cJSON* value) :
    m_value(cJSON_Duplicate(value, true /* recurse */)),
    m_wasParseSuccessful(true)
{
}

JsonValue::JsonValue(const Aws::String& value) : m_wasParseSuccessful(true)
{
    const char* return_parse_end;
    m_value = cJSON_ParseWithOpts(value.c_str(), &return_parse_end, 1/*require_null_terminated*/);

    if (!m_value || cJSON_IsInvalid(m_value))
    {
        m_wasParseSuccessful = false;
        m_errorMessage = "Failed to parse JSON at: ";
        m_errorMessage += return_parse_end;
    }
}

JsonValue::JsonValue(Aws::IStream& istream) : m_wasParseSuccessful(true)
{
    Aws::StringStream memoryStream;
    std::copy(std::istreambuf_iterator<char>(istream), std::istreambuf_iterator<char>(), std::ostreambuf_iterator<char>(memoryStream));
    const char* return_parse_end;
    const auto input = memoryStream.str();
    m_value = cJSON_ParseWithOpts(input.c_str(), &return_parse_end, 1/*require_null_terminated*/);

    if (!m_value || cJSON_IsInvalid(m_value))
    {
        m_wasParseSuccessful = false;
        m_errorMessage = "Failed to parse JSON. Invalid input at: ";
        m_errorMessage += return_parse_end;
    }
}

JsonValue::JsonValue(const JsonValue& value) :
    m_value(cJSON_Duplicate(value.m_value, true/*recurse*/)),
    m_wasParseSuccessful(value.m_wasParseSuccessful),
    m_errorMessage(value.m_errorMessage)
{
}

JsonValue::JsonValue(JsonValue&& value) :
    m_value(value.m_value),
    m_wasParseSuccessful(value.m_wasParseSuccessful),
    m_errorMessage(std::move(value.m_errorMessage))
{
    value.m_value = nullptr;
}

void JsonValue::Destroy()
{
    cJSON_Delete(m_value);
}

JsonValue::~JsonValue()
{
    Destroy();
}

JsonValue& JsonValue::operator=(const JsonValue& other)
{
    if (this == &other)
    {
        return *this;
    }

    Destroy();
    m_value = cJSON_Duplicate(other.m_value, true /*recurse*/);
    m_wasParseSuccessful = other.m_wasParseSuccessful;
    m_errorMessage = other.m_errorMessage;
    return *this;
}

JsonValue& JsonValue::operator=(JsonValue&& other)
{
    if (this == &other)
    {
        return *this;
    }

    using std::swap;
    swap(m_value, other.m_value);
    swap(m_errorMessage, other.m_errorMessage);
    m_wasParseSuccessful = other.m_wasParseSuccessful;
    return *this;
}

static void AddOrReplace(cJSON* root, const char* key, cJSON* value)
{
    const auto existing = cJSON_GetObjectItemCaseSensitive(root, key);
    if (existing)
    {
        cJSON_ReplaceItemInObjectCaseSensitive(root, key, value);
    }
    else
    {
        cJSON_AddItemToObject(root, key, value);
    }
}

JsonValue& JsonValue::WithString(const char* key, const Aws::String& value)
{
    if (!m_value)
    {
        m_value = cJSON_CreateObject();
    }

    const auto val = cJSON_CreateString(value.c_str());
    AddOrReplace(m_value, key, val);
    return *this;
}

JsonValue& JsonValue::WithString(const Aws::String& key, const Aws::String& value)
{
    return WithString(key.c_str(), value);
}

JsonValue& JsonValue::AsString(const Aws::String& value)
{
    Destroy();
    m_value = cJSON_CreateString(value.c_str());
    return *this;
}

JsonValue& JsonValue::WithBool(const char* key, bool value)
{
    if (!m_value)
    {
        m_value = cJSON_CreateObject();
    }

    const auto val = cJSON_CreateBool(value);
    AddOrReplace(m_value, key, val);
    return *this;
}

JsonValue& JsonValue::WithBool(const Aws::String& key, bool value)
{
    return WithBool(key.c_str(), value);
}

JsonValue& JsonValue::AsBool(bool value)
{
    Destroy();
    m_value = cJSON_CreateBool(value);
    return *this;
}

JsonValue& JsonValue::WithInteger(const char* key, int value)
{
    return WithDouble(key, static_cast<double>(value));
}

JsonValue& JsonValue::WithInteger(const Aws::String& key, int value)
{
    return WithDouble(key.c_str(), static_cast<double>(value));
}

JsonValue& JsonValue::AsInteger(int value)
{
    Destroy();
    m_value = cJSON_CreateNumber(static_cast<double>(value));
    return *this;
}

JsonValue& JsonValue::WithInt64(const char* key, long long value)
{
    return WithDouble(key, static_cast<double>(value));
}

JsonValue& JsonValue::WithInt64(const Aws::String& key, long long value)
{
    return WithDouble(key.c_str(), static_cast<double>(value));
}

JsonValue& JsonValue::AsInt64(long long value)
{
    return AsDouble(static_cast<double>(value));
}

JsonValue& JsonValue::WithDouble(const char* key, double value)
{
    if (!m_value)
    {
        m_value = cJSON_CreateObject();
    }

    const auto val = cJSON_CreateNumber(value);
    AddOrReplace(m_value, key, val);
    return *this;
}

JsonValue& JsonValue::WithDouble(const Aws::String& key, double value)
{
    return WithDouble(key.c_str(), value);
}

JsonValue& JsonValue::AsDouble(double value)
{
    Destroy();
    m_value = cJSON_CreateNumber(value);
    return *this;
}

JsonValue& JsonValue::WithArray(const char* key, const Array<Aws::String>& array)
{
    if (!m_value)
    {
        m_value = cJSON_CreateObject();
    }

    auto arrayValue = cJSON_CreateArray();
    for (unsigned i = 0; i < array.GetLength(); ++i)
    {
        cJSON_AddItemToArray(arrayValue, cJSON_CreateString(array[i].c_str()));
    }

    AddOrReplace(m_value, key, arrayValue);
    return *this;
}

JsonValue& JsonValue::WithArray(const Aws::String& key, const Array<Aws::String>& array)
{
    return WithArray(key.c_str(), array);
}

JsonValue& JsonValue::WithArray(const Aws::String& key, const Array<JsonValue>& array)
{
    if (!m_value)
    {
        m_value = cJSON_CreateObject();
    }

    auto arrayValue = cJSON_CreateArray();
    for (unsigned i = 0; i < array.GetLength(); ++i)
    {
        cJSON_AddItemToArray(arrayValue, cJSON_Duplicate(array[i].m_value, true /*recurse*/));
    }

    AddOrReplace(m_value, key.c_str(), arrayValue);
    return *this;
}

JsonValue& JsonValue::WithArray(const Aws::String& key, Array<JsonValue>&& array)
{
    if (!m_value)
    {
        m_value = cJSON_CreateObject();
    }

    auto arrayValue = cJSON_CreateArray();
    for (unsigned i = 0; i < array.GetLength(); ++i)
    {
        cJSON_AddItemToArray(arrayValue, array[i].m_value);
        array[i].m_value = nullptr;
    }

    AddOrReplace(m_value, key.c_str(), arrayValue);
    return *this;
}

JsonValue& JsonValue::AsArray(const Array<JsonValue>& array)
{
    auto arrayValue = cJSON_CreateArray();
    for (unsigned i = 0; i < array.GetLength(); ++i)
    {
        cJSON_AddItemToArray(arrayValue, cJSON_Duplicate(array[i].m_value, true /*recurse*/));
    }

    Destroy();
    m_value = arrayValue;
    return *this;
}

JsonValue& JsonValue::AsArray(Array<JsonValue>&& array)
{
    auto arrayValue = cJSON_CreateArray();
    for (unsigned i = 0; i < array.GetLength(); ++i)
    {
        cJSON_AddItemToArray(arrayValue, array[i].m_value);
        array[i].m_value = nullptr;
    }

    Destroy();
    m_value = arrayValue;
    return *this;
}

JsonValue& JsonValue::WithObject(const char* key, const JsonValue& value)
{
    if (!m_value)
    {
        m_value = cJSON_CreateObject();
    }

    const auto copy = value.m_value == nullptr ? cJSON_CreateObject() : cJSON_Duplicate(value.m_value, true /*recurse*/);
    AddOrReplace(m_value, key, copy);
    return *this;
}

JsonValue& JsonValue::WithObject(const Aws::String& key, const JsonValue& value)
{
    return WithObject(key.c_str(), value);
}

JsonValue& JsonValue::WithObject(const char* key, JsonValue&& value)
{
    if (!m_value)
    {
        m_value = cJSON_CreateObject();
    }

    AddOrReplace(m_value, key, value.m_value == nullptr ? cJSON_CreateObject() : value.m_value);
    value.m_value = nullptr;
    return *this;
}

JsonValue& JsonValue::WithObject(const Aws::String& key, JsonValue&& value)
{
    return WithObject(key.c_str(), std::move(value));
}

JsonValue& JsonValue::AsObject(const JsonValue& value)
{
    *this = value;
    return *this;
}

JsonValue& JsonValue::AsObject(JsonValue && value)
{
    *this = std::move(value);
    return *this;
}

bool JsonValue::operator==(const JsonValue& other) const
{
    return cJSON_Compare(m_value, other.m_value, true /*case-sensitive*/) != 0;
}

bool JsonValue::operator!=(const JsonValue& other) const
{
    return !(*this == other);
}

JsonView JsonValue::View() const
{
    return *this;
}

JsonView::JsonView() : m_value(nullptr)
{
}

JsonView::JsonView(const JsonValue& val) : m_value(val.m_value)
{
}

JsonView::JsonView(cJSON* val) : m_value(val)
{
}

JsonView& JsonView::operator=(const JsonValue& v)
{
    m_value = v.m_value;
    return *this;
}

JsonView& JsonView::operator=(cJSON* val)
{
    m_value = val;
    return *this;
}

Aws::String JsonView::GetString(const Aws::String& key) const
{
    assert(m_value);
    auto item = cJSON_GetObjectItemCaseSensitive(m_value, key.c_str());
    auto str = cJSON_GetStringValue(item);
    return str ? str : "";
}

Aws::String JsonView::AsString() const
{
    const char* str = cJSON_GetStringValue(m_value);
    if (str == nullptr)
    {
        return {};
    }
    return str;
}

bool JsonView::GetBool(const Aws::String& key) const
{
    assert(m_value);
    auto item = cJSON_GetObjectItemCaseSensitive(m_value, key.c_str());
    assert(item);
    return item->valueint != 0;
}

bool JsonView::AsBool() const
{
    assert(cJSON_IsBool(m_value));
    return cJSON_IsTrue(m_value) != 0;
}

int JsonView::GetInteger(const Aws::String& key) const
{
    assert(m_value);
    auto item = cJSON_GetObjectItemCaseSensitive(m_value, key.c_str());
    assert(item);
    return item->valueint;
}

int JsonView::AsInteger() const
{
    assert(cJSON_IsNumber(m_value)); // can be double or value larger than int_max, but at least not UB
    return m_value->valueint;
}

int64_t JsonView::GetInt64(const Aws::String& key) const
{
    return static_cast<long long>(GetDouble(key));
}

int64_t JsonView::AsInt64() const
{
    assert(cJSON_IsNumber(m_value));
    return static_cast<long long>(m_value->valuedouble);
}

double JsonView::GetDouble(const Aws::String& key) const
{
    assert(m_value);
    auto item = cJSON_GetObjectItemCaseSensitive(m_value, key.c_str());
    assert(item);
    return item->valuedouble;
}

double JsonView::AsDouble() const
{
    assert(cJSON_IsNumber(m_value));
    return m_value->valuedouble;
}

JsonView JsonView::GetObject(const Aws::String& key) const
{
    assert(m_value);
    auto item = cJSON_GetObjectItemCaseSensitive(m_value, key.c_str());
    return item;
}

JsonView JsonView::AsObject() const
{
    assert(cJSON_IsObject(m_value));
    return m_value;
}

Array<JsonView> JsonView::GetArray(const Aws::String& key) const
{
    assert(m_value);
    auto array = cJSON_GetObjectItemCaseSensitive(m_value, key.c_str());
    assert(cJSON_IsArray(array));
    Array<JsonView> returnArray(cJSON_GetArraySize(array));

    auto element = array->child;
    for (unsigned i = 0; element && i < returnArray.GetLength(); ++i, element = element->next)
    {
        returnArray[i] = element;
    }

    return returnArray;
}

Array<JsonView> JsonView::AsArray() const
{
    assert(cJSON_IsArray(m_value));
    Array<JsonView> returnArray(cJSON_GetArraySize(m_value));

    auto element = m_value->child;

    for (unsigned i = 0; element && i < returnArray.GetLength(); ++i, element = element->next)
    {
        returnArray[i] = element;
    }

    return returnArray;
}

Aws::Map<Aws::String, JsonView> JsonView::GetAllObjects() const
{
    Aws::Map<Aws::String, JsonView> valueMap;
    if (!m_value)
    {
        return valueMap;
    }

    for (auto iter = m_value->child; iter; iter = iter->next)
    {
        valueMap.emplace(std::make_pair(Aws::String(iter->string), JsonView(iter)));
    }

    return valueMap;
}

bool JsonView::ValueExists(const Aws::String& key) const
{
    if (!cJSON_IsObject(m_value))
    {
        return false;
    }

    auto item = cJSON_GetObjectItemCaseSensitive(m_value, key.c_str());
    return !(item == nullptr || cJSON_IsNull(item));
}

bool JsonView::KeyExists(const Aws::String& key) const
{
    if (!cJSON_IsObject(m_value))
    {
        return false;
    }

    return cJSON_GetObjectItemCaseSensitive(m_value, key.c_str()) != nullptr;;
}

bool JsonView::IsObject() const
{
    return cJSON_IsObject(m_value) != 0;
}

bool JsonView::IsBool() const
{
    return cJSON_IsBool(m_value) != 0;
}

bool JsonView::IsString() const
{
    return cJSON_IsString(m_value) != 0;
}

bool JsonView::IsIntegerType() const
{
    if (!cJSON_IsNumber(m_value))
    {
        return false;
    }

    return m_value->valuedouble == static_cast<long long>(m_value->valuedouble);
}

bool JsonView::IsFloatingPointType() const
{
    if (!cJSON_IsNumber(m_value))
    {
        return false;
    }

    return m_value->valuedouble != static_cast<long long>(m_value->valuedouble);
}

bool JsonView::IsListType() const
{
    return cJSON_IsArray(m_value) != 0;
}

bool JsonView::IsNull() const
{
    return cJSON_IsNull(m_value) != 0;
}

Aws::String JsonView::WriteCompact(bool treatAsObject) const
{
    if (!m_value)
    {
        if (treatAsObject)
        {
            return "{}";
        }
        return {};
    }

    auto temp = cJSON_PrintUnformatted(m_value);
    Aws::String out(temp);
    cJSON_free(temp);
    return out;
}

Aws::String JsonView::WriteReadable(bool treatAsObject) const
{
    if (!m_value)
    {
        if (treatAsObject)
        {
            return "{\n}\n";
        }
        return {};
    }

    auto temp = cJSON_Print(m_value);
    Aws::String out(temp);
    cJSON_free(temp);
    return out;
}

JsonValue JsonView::Materialize() const
{
    return m_value;
}
