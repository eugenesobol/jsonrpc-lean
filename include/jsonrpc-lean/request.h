// This file is derived from xsonrpc Copyright (C) 2015 Erik Johansson <erik@ejohansson.se>
// This file is part of jsonrpc-lean, a c++11 JSON-RPC client/server library.
//
// Modifications and additions for jsonrpc-lean Copyright (C) 2015 Adriano Maia <tony@stark.im>
//

#ifndef JSONRPC_LEAN_REQUEST_H
#define JSONRPC_LEAN_REQUEST_H

#include "value.h"

#include <deque>
#include <string>

namespace jsonrpc {

    class Writer;

    class Request
    {
    public:
        typedef std::deque<Value> Parameters;
        using NamedParams = std::deque<std::string>;

        Request(std::string methodName, Parameters parameters, Value id)
            : myMethodName(std::move(methodName)),
            myParameters(std::move(parameters)),
            myId(std::move(id)) {
            // Empty
        }

        const std::string& GetMethodName() const { return myMethodName; }
        const Parameters& GetParameters() const { return myParameters; }
        const Value& GetId() const { return myId; }

        void Write(Writer& writer) const {
            Write(myMethodName, myParameters, myId, writer);
        }

        static void Write(const std::string& methodName, const Parameters& params, const Value& id, Writer& writer) {
            writer.StartDocument();
            writer.StartRequest(methodName, id);
            writer.StartArray();
            for (auto& param : params) {
                writer.StartParameter();
                param.Write(writer);
                writer.EndParameter();
            }
            writer.EndArray();
            writer.EndRequest();
            writer.EndDocument();
        }

        // quick and crappy way to implement parameters in JSON object
        static void Write(const std::string& methodName,
                      const Parameters&  params,
                      const Value&       id,
                      Writer&            writer,
                      const NamedParams& namedParams)
        {

            if (namedParams.size() != params.size())
            {
                throw std::runtime_error("Arrays of parameter values and names must have equal size");
            }

            writer.StartDocument();
            writer.StartRequest(methodName, id);
            writer.StartStruct();

            std::size_t index = 0;
            for (const auto& paramName : namedParams)
            {
                writer.StartStructElement(paramName);
                params[index++].Write(writer);
                writer.EndStructElement();
            }

            writer.EndStruct();
            writer.EndRequest();
            writer.EndDocument();
        }

    private:
        std::string myMethodName;
        Parameters myParameters;
        Value myId;
    };

} // namespace jsonrpc

#endif //JSONRPC_LEAN_REQUEST_H
