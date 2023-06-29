// This file is derived from xsonrpc Copyright (C) 2015 Erik Johansson <erik@ejohansson.se>
// This file is part of jsonrpc-lean, a c++11 JSON-RPC client/server library.
//
// Modifications and additions for jsonrpc-lean Copyright (C) 2015 Adriano Maia <tony@stark.im>
//

#ifndef JSONRPC_LEAN_JSONREADER_H
#define JSONRPC_LEAN_JSONREADER_H

#include "reader.h"
#include "fault.h"
#include "json.h"
#include "request.h"
#include "response.h"
#include "util.h"
#include "value.h"
#include "dispatcher.h"

#define RAPIDJSON_NO_SIZETYPEDEFINE
namespace rapidjson { typedef ::std::size_t SizeType; }

#include <rapidjson/document.h>
#include <string>

namespace jsonrpc {

    class JsonReader final : public Reader {
    public:
        JsonReader(const std::string& data, GetNamedParamsFunc func)
        : getNamedParamsFunc(func)
        {
            myDocument.Parse(data.c_str());
            if (myDocument.HasParseError()) {
                throw ParseErrorFault(
                    "Parse error: " + std::to_string(myDocument.GetParseError()));
            }
        }

        void parseObjectParams(Request::Parameters&              parameters,
                            const rapidjson::Value&           paramsNode,
                            const MethodWrapper::NamedParams& namedParams)
        {
            //            if (namedParams.empty()) {
            //                throw InternalErrorFault();
            //            }

            auto paramsObj = paramsNode.GetObject();

            // iterate over named parameter names in the order they
            // must be passed to a handler of JSON-RPC method
            for (const auto& namedParam : namedParams)
            {

                auto param = paramsObj.FindMember(namedParam.c_str());

                // named parameter was not found
                if (param == paramsObj.MemberEnd())
                {
                    parameters.emplace_back(Value());
                }
                else
                {
                    parameters.emplace_back(GetValue(param->value));
                }
            }
        }

            // Reader
        Request GetRequest(Value& idOnFault) override {
            if (!myDocument.IsObject()) {
                throw InvalidRequestFault();
            }

            auto id      = myDocument.FindMember(json::ID_NAME);
            auto idFound = id != myDocument.MemberEnd();
            if (idFound)
            {
                try
                {
                    idOnFault = GetId(id->value);
                }
                catch (...)
                {
                }
            }

            ValidateJsonrpcVersion();

            auto method = myDocument.FindMember(json::METHOD_NAME);
            if (method == myDocument.MemberEnd() || !method->value.IsString()) {
                throw InvalidRequestFault();
            }

            Request::Parameters parameters;
            auto params = myDocument.FindMember(json::PARAMS_NAME);
            if (params != myDocument.MemberEnd()) {

                if (params->value.IsArray()) {
                    for (auto param = params->value.Begin(); param != params->value.End(); ++param) {
                        parameters.emplace_back(GetValue(*param));
                    }
                } else if (params->value.IsObject()) {
                    try
                    {

                        if (!this->getNamedParamsFunc)
                        {
                            throw std::runtime_error("NULL callback for named params");
                        }

                        const auto& namedParams = this->getNamedParamsFunc(method->value.GetString());
                        parseObjectParams(parameters, params->value, namedParams);
                    }
                    catch (const std::out_of_range&)
                    {
                        throw MethodNotFoundFault();
                    }
                    catch (const std::exception& exc)
                    {
                        throw InternalErrorFault(exc.what());
                    }
                }
                else
                {
                    throw InvalidRequestFault();
                }
            }

            if (idFound) {
                return Request(method->value.GetString(), std::move(parameters), GetId(id->value));
            } else {
                // Notification
                return Request(method->value.GetString(), std::move(parameters), false);
            }
        }

        Response GetResponse() override {
            if (!myDocument.IsObject()) {
                throw InvalidRequestFault();
            }

            ValidateJsonrpcVersion();

            auto id = myDocument.FindMember(json::ID_NAME);
            if (id == myDocument.MemberEnd()) {
                throw InvalidRequestFault();
            }

            auto result = myDocument.FindMember(json::RESULT_NAME);
            auto error = myDocument.FindMember(json::ERROR_NAME);

            if (result != myDocument.MemberEnd()) {
                if (error != myDocument.MemberEnd()) {
                    throw InvalidRequestFault();
                }
                return Response(GetValue(result->value), GetId(id->value));
            } else if (error != myDocument.MemberEnd()) {
                if (result != myDocument.MemberEnd()) {
                    throw InvalidRequestFault();
                }
                if (!error->value.IsObject()) {
                    throw InvalidRequestFault();
                }
                auto code = error->value.FindMember(json::ERROR_CODE_NAME);
                if (code == error->value.MemberEnd() || !code->value.IsInt()) {
                    throw InvalidRequestFault();
                }
                auto message = error->value.FindMember(json::ERROR_MESSAGE_NAME);
                if (message == error->value.MemberEnd() || !message->value.IsString()) {
                    throw InvalidRequestFault();
                }

                auto data = error->value.FindMember(json::ERROR_DATA_NAME);
                if (data == error->value.MemberEnd()) {
                    return Response(code->value.GetInt(), message->value.GetString(), GetId(id->value));
                } else {
                    return Response(code->value.GetInt(),
                                    message->value.GetString(),
                                    GetId(id->value),
                                    GetValue(data->value));
                }
            } else {
                throw InvalidRequestFault();
            }
        }

        Value GetValue() override {
            return GetValue(myDocument);
        }

    private:
        void ValidateJsonrpcVersion() const {
            auto jsonrpc = myDocument.FindMember(json::JSONRPC_NAME);
            if (jsonrpc == myDocument.MemberEnd()
                || !jsonrpc->value.IsString()
                || strcmp(jsonrpc->value.GetString(), json::JSONRPC_VERSION_2_0) != 0) {
                throw InvalidRequestFault();
            }
        }

        Value GetValue(const rapidjson::Value& value) const {
            switch (value.GetType()) {
            case rapidjson::kNullType:
                return Value();
            case rapidjson::kFalseType:
            case rapidjson::kTrueType:
                return Value(value.GetBool());
            case rapidjson::kObjectType: {
                Value::Struct data;
                for (auto it = value.MemberBegin(); it != value.MemberEnd(); ++it) {
                    std::string name(it->name.GetString(), it->name.GetStringLength());
                    data.emplace(name, GetValue(it->value));
                }
                return Value(std::move(data));
            }
            case rapidjson::kArrayType: {
                Value::Array array;
                array.reserve(value.Size());
                for (auto it = value.Begin(); it != value.End(); ++it) {
                    array.emplace_back(GetValue(*it));
                }
                return Value(std::move(array));
            }
            case rapidjson::kStringType: {
                tm dt;
                if (util::ParseIso8601DateTime(value.GetString(), dt))
                {
                    return Value(dt);
                }

                std::string str(value.GetString(), value.GetStringLength());
                const bool binary = str.find('\0') != std::string::npos;
                return Value(std::move(str), binary);
            }
            case rapidjson::kNumberType:
                if (value.IsDouble()) {
                    return Value(value.GetDouble());
                } else if (value.IsInt()) {
                    return Value(value.GetInt());
                } else if (value.IsUint()) {
                    return Value(static_cast<int64_t>(value.GetUint()));
                } else if (value.IsInt64()) {
                    return Value(value.GetInt64());
                } else {
                    assert(value.IsUint64());
                    return Value(static_cast<double>(value.GetUint64()));
                }
                break;
            }

            throw InternalErrorFault();
        }

        Value GetId(const rapidjson::Value& id) const {
            if (id.IsString()) {
                return id.GetString();
            } else if (id.IsInt()) {
                return id.GetInt();
            } else if (id.IsInt64()) {
                return id.GetInt64();
            } else if (id.IsNull()) {
                return{};
            }

            throw InvalidRequestFault();
        }

        std::string myData;
        rapidjson::Document myDocument;
        GetNamedParamsFunc  getNamedParamsFunc;
    };

} // namespace jsonrpc

#endif // JSONRPC_LEAN_JSONREADER_H
