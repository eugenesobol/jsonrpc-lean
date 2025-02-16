// This file is derived from xsonrpc Copyright (C) 2015 Erik Johansson <erik@ejohansson.se>
// This file is part of jsonrpc-lean, a c++11 JSON-RPC client/server library.
//
// Modifications and additions for jsonrpc-lean Copyright (C) 2015 Adriano Maia <tony@stark.im>
//

#ifndef JSONRPC_LEAN_VALUE_H
#define JSONRPC_LEAN_VALUE_H

#include <cstdint>
#include <iosfwd>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <ostream>
#include <boost/optional.hpp>

#include "util.h"
#include "fault.h"
#include "writer.h"

namespace jsonrpc {

    class Value {
    public:
        typedef std::vector<Value> Array;
        typedef tm DateTime;
        typedef std::string String;
        typedef std::map<std::string, Value> Struct;

        enum class Type {
            ARRAY,
            BINARY,
            BOOLEAN,
            DATE_TIME,
            DOUBLE,
            INTEGER_32,
            INTEGER_64,
            NIL,
            STRING,
            STRUCT
        };

        Value() : myType(Type::NIL) {}

        Value(Array value) : myType(Type::ARRAY) {
            as.myArray = new Array(std::move(value));
        }

        Value(bool value) : myType(Type::BOOLEAN) { as.myBoolean = value; }

        Value(const DateTime& value) : myType(Type::DATE_TIME),
            as{.myDateTime = new DateTime(value)} {
            as.myDateTime->tm_isdst = -1;
            optAs.myDateTime        = as.myDateTime;
        }

        Value(double value) : myType(Type::DOUBLE),
            as{.myDouble = value} {
            optAs.myDouble = &as.myDouble;
        }

        Value(int32_t value) : myType(Type::INTEGER_32), as{.myInteger32 = value} {
            as.myInteger64    = value;
            as.myDouble       = value;
            optAs.myInteger32 = &as.myInteger32;
        }

        Value(int64_t value) : myType(Type::INTEGER_64) {
            as.myInteger32 = static_cast<int32_t>(value);
            as.myInteger64 = value;
            as.myDouble = static_cast<double>(value);
        }

        Value(const char* value) : Value(String(value)) {}

        Value(String value, bool binary = false) : myType(binary ? Type::BINARY : Type::STRING) {
            as.myString = new String(std::move(value));
        }

        Value(Struct value) : myType(Type::STRUCT) {
            as.myStruct = new Struct(std::move(value));
        }

        ~Value() {
            Reset();
        }

        template<typename T>
        Value(boost::optional<T> value)
        : Value(value.get_value_or(Value()))
        {
        }

        template<typename T>
        Value(std::vector<T> value) : Value(Array{}) {
            as.myArray->reserve(value.size());
            for (auto& v : value) {
                as.myArray->emplace_back(std::move(v));
            }
        }

        template<typename T>
        Value(const std::map<std::string, T>& value) : Value(Struct{}) {
            for (auto& v : value) {
                as.myStruct->emplace(v.first, v.second);
            }
        }

        template<typename T>
        Value(const std::unordered_map<std::string, T>& value) : Value(Struct{}) {
            for (auto& v : value) {
                as.myStruct->emplace(v.first, v.second);
            }
        }

        void InitOpt()
        {
            switch (myType)
            {
                case Type::BOOLEAN:
                    optAs.myBoolean = &as.myBoolean;
                    break;
                case Type::DOUBLE:
                    optAs.myDouble = &as.myDouble;
                    break;
                case Type::INTEGER_32:
                    optAs.myInteger32 = &as.myInteger32;
                    break;
                case Type::INTEGER_64:
                    optAs.myInteger64 = &as.myInteger64;
                    break;
                case Type::NIL:
                    break;
                case Type::ARRAY:
                    optAs.myArray = as.myArray;
                    break;
                case Type::DATE_TIME:
                    optAs.myDateTime = as.myDateTime;
                    break;
                case Type::BINARY:
                case Type::STRING:
                    optAs.myString = as.myString;
                    break;
                case Type::STRUCT:
                    optAs.myStruct = as.myStruct;
                    break;
            }
        }

        explicit Value(const Value& other) : myType(other.myType), as(other.as) {
            switch (myType) {
            case Type::BOOLEAN:
            case Type::DOUBLE:
            case Type::INTEGER_32:
            case Type::INTEGER_64:
            case Type::NIL:
                break;

            case Type::ARRAY:
                as.myArray = new Array(other.AsArray());
                break;
            case Type::DATE_TIME:
                as.myDateTime = new DateTime(other.AsDateTime());
                break;
            case Type::BINARY:
            case Type::STRING:
                as.myString = new String(other.AsString());
                break;
            case Type::STRUCT:
                as.myStruct = new Struct(other.AsStruct());
                break;
            }
            InitOpt();
        }

        Value& operator=(const Value&) = delete;

        Value(Value&& other) noexcept : myType(other.myType), as(other.as) {
            other.myType = Type::NIL;
        }

        Value& operator=(Value&& other) noexcept {
            if (this != &other) {
                Reset();

                myType = other.myType;
                as = other.as;

                other.myType = Type::NIL;
                InitOpt();
            }
            return *this;
        }

        bool IsArray() const { return myType == Type::ARRAY; }
        bool IsBinary() const { return myType == Type::BINARY; }
        bool IsBoolean() const { return myType == Type::BOOLEAN; }
        bool IsDateTime() const { return myType == Type::DATE_TIME; }
        bool IsDouble() const { return myType == Type::DOUBLE; }
        bool IsInteger32() const { return myType == Type::INTEGER_32; }
        bool IsInteger64() const { return myType == Type::INTEGER_64; }
        bool IsNil() const { return myType == Type::NIL; }
        bool IsString() const { return myType == Type::STRING; }
        bool IsStruct() const { return myType == Type::STRUCT; }

        const Array& AsArray() const {
            if (IsArray()) {
                return *as.myArray;
            }
            throw InvalidParametersFault();
        }

        const String& AsBinary() const { return AsString(); }

        const bool& AsBoolean() const {
            if (IsBoolean()) {
                return as.myBoolean;
            }
            throw InvalidParametersFault();
        }

        const DateTime& AsDateTime() const
        {
            if (IsDateTime())
            {
                return *as.myDateTime;
            }
            throw InvalidParametersFault();
        }

        const double& AsDouble() const {
            if (IsDouble() || IsInteger32() || IsInteger64()) {
                return as.myDouble;
            }
            throw InvalidParametersFault();
        }

        const int32_t& AsInteger32() const {
            if (IsInteger32()) {
                return as.myInteger32;
            } else if (IsInteger64()
                && static_cast<int64_t>(as.myInteger32) == as.myInteger64) {
                return as.myInteger32;
            }
            throw InvalidParametersFault();
        }

        const int64_t& AsInteger64() const {
            if (IsInteger32() || IsInteger64()) {
                return as.myInteger64;
            }
            throw InvalidParametersFault();
        }

        const String& AsString() const {
            if (IsString() || IsBinary()) {
                return *as.myString;
            }
            throw InvalidParametersFault();
        }

        const Struct& AsStruct() const {
            if (IsStruct()) {
                return *as.myStruct;
            }
            throw InvalidParametersFault();
        }

        template<typename T>
        inline const T& AsType() const;

        Type GetType() const { return myType; }

        void Write(Writer& writer) const {
            switch (myType) {
            case Type::ARRAY:
                writer.StartArray();
                for (auto& element : *as.myArray) {
                    element.Write(writer);
                }
                writer.EndArray();
                break;
            case Type::BINARY:
                writer.WriteBinary(as.myString->data(), as.myString->size());
                break;
            case Type::BOOLEAN:
                writer.Write(as.myBoolean);
                break;
            case Type::DATE_TIME:
                writer.Write(*as.myDateTime);
                break;
            case Type::DOUBLE:
                writer.Write(as.myDouble);
                break;
            case Type::INTEGER_32:
                writer.Write(as.myInteger32);
                break;
            case Type::INTEGER_64:
                writer.Write(as.myInteger64);
                break;
            case Type::NIL:
                writer.WriteNull();
                break;
            case Type::STRING:
                writer.Write(*as.myString);
                break;
            case Type::STRUCT:
                writer.StartStruct();
                for (auto& element : *as.myStruct) {
                    writer.StartStructElement(element.first);
                    element.second.Write(writer);
                    writer.EndStructElement();
                }
                writer.EndStruct();
                break;
            }
        }

        inline const Value& operator[](Array::size_type i) const;
        inline const Value& operator[](const Struct::key_type& key) const;

    private:
        void Reset() {
            switch (myType) {
            case Type::ARRAY:
                delete as.myArray;
                break;
            case Type::DATE_TIME:
                delete as.myDateTime;
                break;
            case Type::BINARY:
            case Type::STRING:
                delete as.myString;
                break;
            case Type::STRUCT:
                delete as.myStruct;
                break;

            case Type::BOOLEAN:
            case Type::DOUBLE:
            case Type::INTEGER_32:
            case Type::INTEGER_64:
            case Type::NIL:
                break;
            }

            myType = Type::NIL;
        }

        Type myType;
        union {
            Array* myArray;
            bool myBoolean;
            DateTime* myDateTime;
            String* myString;
            Struct* myStruct;
            struct {
                double myDouble;
                int32_t myInteger32;
                int64_t myInteger64;
            };
        } as;
        struct
        {
            boost::optional<Value::Array*>    myArray;
            boost::optional<bool*>            myBoolean;
            boost::optional<Value::DateTime*> myDateTime;
            boost::optional<Value::String*>   myString;
            boost::optional<Value::Struct*>   myStruct;
            boost::optional<double*>          myDouble;
            boost::optional<int32_t*>         myInteger32;
            boost::optional<int64_t*>         myInteger64;
        } optAs;
    };

    template<>
    inline const boost::optional<Value::Array*>& Value::AsType<boost::optional<Value::Array*>>() const
    {
        return optAs.myArray;
    }

    template<>
    inline const boost::optional<bool*>& Value::AsType<boost::optional<bool*>>() const
    {
        return optAs.myBoolean;
    }

    template<>
    inline const boost::optional<Value::DateTime*>& Value::AsType<boost::optional<Value::DateTime*>>()
    const
    {
        return optAs.myDateTime;
    }

    template<>
    inline const boost::optional<double*>& Value::AsType<boost::optional<double*>>() const
    {
        return optAs.myDouble;
    }

    template<>
    inline const boost::optional<int32_t*>& Value::AsType<boost::optional<int32_t*>>() const
    {
        return optAs.myInteger32;
    }

    template<>
    inline const boost::optional<int64_t*>& Value::AsType<boost::optional<int64_t*>>() const
    {
        return optAs.myInteger64;
    }

    template<>
    inline const boost::optional<Value::String*>& Value::AsType<boost::optional<Value::String*>>() const
    {
        return optAs.myString;
    }

    template<>
    inline const boost::optional<Value::Struct*>& Value::AsType<boost::optional<Value::Struct*>>() const
    {
        return optAs.myStruct;
    }

    template<> inline const Value::Array& Value::AsType<typename Value::Array>() const {
        return AsArray();
    }

    template<> inline const bool& Value::AsType<bool>() const {
        return AsBoolean();
    }

    template<>
    inline const Value::DateTime& Value::AsType<typename Value::DateTime>() const
    {
        return AsDateTime();
    }

    template<> inline const double& Value::AsType<double>() const {
        return AsDouble();
    }

    template<> inline const int32_t& Value::AsType<int32_t>() const {
        return AsInteger32();
    }

    template<> inline const int64_t& Value::AsType<int64_t>() const {
        return AsInteger64();
    }

    template<> inline const Value::String& Value::AsType<typename Value::String>() const {
        return AsString();
    }

    template<> inline const Value::Struct& Value::AsType<typename Value::Struct>() const {
        return AsStruct();
    }

    template<> inline const Value& Value::AsType<Value>() const {
        return *this;
    }

    inline const Value& Value::operator[](Array::size_type i) const {
        return AsArray().at(i);
    };

    inline const Value& Value::operator[](const Struct::key_type& key) const {
        return AsStruct().at(key);
    }

    inline std::ostream& operator<<(std::ostream& os, const Value& value) {
        switch (value.GetType()) {
        case Value::Type::ARRAY: {
            os << '[';
            auto& a = value.AsArray();
            for (auto it = a.begin(); it != a.end(); ++it) {
                if (it != a.begin()) {
                    os << ", ";
                }
                os << *it;
            }
            os << ']';
            break;
        }
        case Value::Type::BINARY:
            os << util::Base64Encode(value.AsBinary());
            break;
        case Value::Type::BOOLEAN:
            os << value.AsBoolean();
            break;
        case Value::Type::DATE_TIME:
            os << util::FormatIso8601DateTime(value.AsDateTime());
            break;
        case Value::Type::DOUBLE:
            os << value.AsDouble();
            break;
        case Value::Type::INTEGER_32:
            os << value.AsInteger32();
            break;
        case Value::Type::INTEGER_64:
            os << value.AsInteger64();
            break;
        case Value::Type::NIL:
            os << "<nil>";
            break;
        case Value::Type::STRING:
            os << '"' << value.AsString() << '"';
            break;
        case Value::Type::STRUCT: {
            os << '{';
            auto& s = value.AsStruct();
            for (auto it = s.begin(); it != s.end(); ++it) {
                if (it != s.begin()) {
                    os << ", ";
                }
                os << it->first << ": " << it->second;
            }
            os << '}';
            break;
        }
        }
        return os;
    }

} // namespace jsonrpc

#endif // JSONRPC_LEAN_VALUE_H
