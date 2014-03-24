/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <jansson/jansson.h>
#include <et-ext/json/json.h>

using namespace et;
using namespace et::json;

et::ValueBase::Pointer deserializeJson(const char*, size_t, ValueClass&);

ArrayValue deserializeArray(json_t* json);
Dictionary deserializeDictionary(json_t* json);
StringValue deserializeString(json_t* json);
IntegerValue deserializeInteger(json_t* json);
FloatValue deserializeFloat(json_t* json);

json_t* serializeArray(ArrayValue);
json_t* serializeString(StringValue);
json_t* serializeInteger(IntegerValue);
json_t* serializeFloat(FloatValue value);
json_t* serializeDictionary(Dictionary);
json_t* serializeValue(ValueBase::Pointer);

std::string et::json::serialize(const Dictionary& msg, bool readableFormat)
{
	json_t* dictionary = serializeDictionary(msg);
	size_t flags = readableFormat ? JSON_INDENT(2) : JSON_COMPACT;
	char* dump = json_dumps(dictionary, flags | JSON_PRESERVE_ORDER | JSON_ENSURE_ASCII);
	
	std::string serialized(dump);
	
	free(dump);
	json_decref(dictionary);

	return serialized;
}

std::string et::json::serialize(const et::ArrayValue& arr, bool readableFormat)
{
	json_t* dictionary = serializeArray(arr);
	size_t flags = readableFormat ? JSON_INDENT(2) : JSON_COMPACT;
	char* dump = json_dumps(dictionary, flags | JSON_PRESERVE_ORDER | JSON_ENSURE_ASCII);
	
	std::string serialized(dump);
	
	free(dump);
	json_decref(dictionary);
	
	return serialized;
}

et::ValueBase::Pointer et::json::deserialize(const char* input, ValueClass& c)
	{ return deserializeJson(input, strlen(input), c); }

et::ValueBase::Pointer  et::json::deserialize(const std::string& s, ValueClass& c)
	{ return deserializeJson(s.c_str(), s.length(), c); }

et::ValueBase::Pointer deserializeJson(const char* buffer, size_t len, ValueClass& c)
{
	c = ValueClass_Invalid;
	
	if ((buffer == nullptr) || (len == 0))
		return et::ValueBase::Pointer();
	
	json_error_t error = { };
	json_t* root = json_loadb(buffer, len, 0, &error);
	
	if (error.line != -1)
	{
		log::error("JSON parsing error (%d,%d): %s %s", error.line, error.column, error.source, error.text);
		log::error("%s", buffer);
		json_decref(root);
		return ValueBase::Pointer();
	}
	
	et::ValueBase::Pointer result;
	
	if (json_is_object(root))
	{
		c = ValueClass_Dictionary;
		result = deserializeDictionary(root);
	}
	else if (json_is_array(root))
	{
		c = ValueClass_Array;
		result = deserializeArray(root);
	}
	else
	{
		ET_ASSERT(false && "Unsupported (yet) root object type.");
	}
	
	json_decref(root);
	return result;
}

StringValue deserializeString(json_t* json)
{
	ET_ASSERT(json_is_string(json));
	return StringValue(std::string(json_string_value(json)));
}

IntegerValue deserializeInteger(json_t* json)
{
	ET_ASSERT(json_is_integer(json));
	return IntegerValue(static_cast<int>(json_integer_value(json) & 0xffffffff));
}

FloatValue deserializeFloat(json_t* json)
{
	ET_ASSERT(json_is_real(json));
	return FloatValue(static_cast<float>(json_real_value(json)));
}

ArrayValue deserializeArray(json_t* json)
{
	ET_ASSERT(json_is_array(json));
	
	ArrayValue result;
	for (size_t i = 0; i < json_array_size(json); ++i)
	{
		json_t* value = json_array_get(json, i);
		if (json_is_string(value))
			result->content.push_back(deserializeString(value));
		else if (json_is_integer(value))
			result->content.push_back(deserializeInteger(value));
		else if (json_is_real(value))
			result->content.push_back(deserializeFloat(value));
		else if (json_is_array(value))
			result->content.push_back(deserializeArray(value));
		else if (json_is_object(value))
			result->content.push_back(deserializeDictionary(value));
		else if (json_is_null(value))
			result->content.push_back(Dictionary());
		else if (value)
		{
			log::error("Unsupported JSON type: %d", value->type);
			ET_ASSERT(false);
		}
	}
	return result;
}

Dictionary deserializeDictionary(json_t* root)
{
	ET_ASSERT(json_is_object(root));
	
	Dictionary result;
	
	void* child = json_object_iter(root);
	while (child)
	{
		const std::string& key = json_object_iter_key(child);
		json_t* value = json_object_iter_value(child);
		
		if (json_is_string(value))
			result.setStringForKey(key, deserializeString(value));
		else if (json_is_integer(value))
			result.setIntegerForKey(key, deserializeInteger(value));
		else if (json_is_real(value))
			result.setFloatForKey(key, deserializeFloat(value));
		else if (json_is_array(value))
			result.setArrayForKey(key, deserializeArray(value));
		else if (json_is_object(value))
			result.setDictionaryForKey(key, deserializeDictionary(value));
		else if (json_is_null(value))
			result.setDictionaryForKey(key, Dictionary());
		else if (json_is_true(value))
			result.setIntegerForKey(key, IntegerValue(1));
		else if (json_is_false(value))
			result.setIntegerForKey(key, IntegerValue(0));
		else if (value != nullptr)
		{
			log::error("Unsupported JSON type: %d", value->type);
			ET_ASSERT(false);
		}
		child = json_object_iter_next(root, child);
	}
	
	return result;
}

json_t* serializeArray(ArrayValue value)
{
	json_t* array = json_array();
	
	for (auto i : value->content)
	{
		json_t* arrayValue = serializeValue(i);
		json_array_append(array, arrayValue);
		json_decref(arrayValue);
	}

	return array;
}

json_t* serializeString(StringValue value)
{
	return json_string(value->content.c_str());
}

json_t* serializeInteger(IntegerValue value)
{
	return json_integer(value->content);
}

json_t* serializeFloat(FloatValue value)
{
	return json_real(value->content);
}

json_t* serializeDictionary(Dictionary msg)
{
	json_t* root = json_object();
		
	for (auto& v : msg->content)
	{
		json_t* value = serializeValue(v.second);
		json_object_set(root, v.first.c_str(), value);
		json_decref(value);
	}
		
	return root;
}

json_t* serializeValue(ValueBase::Pointer v)
{
	json_t* value = nullptr;
	
	if (v->valueClass() == ValueClass_String)
		value = serializeString(v);
	else if (v->valueClass() == ValueClass_Integer)
		value = serializeInteger(v);
	else if (v->valueClass() == ValueClass_Float)
		value = serializeFloat(v);
	else if (v->valueClass() == ValueClass_Array)
		value = serializeArray(v);
	else if (v->valueClass() == ValueClass_Dictionary)
		value = serializeDictionary(v);
	else
	{
		log::error("Unknown dictionary class %d", v->valueClass());
		ET_ASSERT(false);
	}
	
	return value;
}