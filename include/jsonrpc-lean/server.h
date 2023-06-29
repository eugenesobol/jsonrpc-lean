// This file is derived from xsonrpc Copyright (C) 2015 Erik Johansson <erik@ejohansson.se>
// This file is part of jsonrpc-lean, a c++11 JSON-RPC client/server library.
//
// Modifications and additions for jsonrpc-lean Copyright (C) 2015 Adriano Maia <tony@stark.im>
//

#ifndef JSONRPC_LEAN_SERVER_H
#define JSONRPC_LEAN_SERVER_H

#include "request.h"
#include "value.h"
#include "fault.h"
#include "formathandler.h"
#include "jsonformathandler.h"
#include "reader.h"
#include "response.h"
#include "writer.h"
#include "formatteddata.h"
#include "jsonformatteddata.h"
#include "dispatcher.h"


#include <string>

namespace jsonrpc {

    class Server {
    public:
        Server() : getNamedParamsFunc(std::bind(&Server::getNamedParams, this, std::placeholders::_1))
        {
        }
        ~Server() {}

        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;
        Server(Server&&) = delete;
        Server& operator=(Server&&) = delete;

        void RegisterFormatHandler(FormatHandler& formatHandler) {
            myFormatHandlers.push_back(&formatHandler);
        }

        Dispatcher& GetDispatcher() { return myDispatcher; }

        const Reader::NamedParams& getNamedParams(const std::string& methodName)
        {
            return GetDispatcher().GetMethod(methodName).GetNamedParams();
        }

        Request parse(const std::string& aRequestData,
                    Value&             idOnFault,
                    const std::string& aContentType = "application/json")
        {
            // first find the correct handler
            auto fmtHandler = getFormatHandler(aContentType);

            if (fmtHandler == nullptr)
            {
                // no FormatHandler able to handle this request type was found
                throw InvalidRequestFault();
            }

            auto reader = fmtHandler->CreateReader(aRequestData, getNamedParamsFunc);
            return reader->GetRequest(idOnFault);
        }

        // aContentType is here to allow future implementation of other rpc formats with minimal code changes
        // Will return NULL if no FormatHandler is found, otherwise will return a FormatedData
        // If aRequestData is a Notification (the client doesn't expect a response), the returned FormattedData will have an empty ->GetData() buffer and ->GetSize() will be 0
        std::shared_ptr<jsonrpc::FormattedData> HandleRequest(const std::string& aRequestData, const std::string& aContentType = "application/json") {

            auto fmtHandler = getFormatHandler(aContentType);

            if (nullptr == fmtHandler)
            {
                throw std::runtime_error("FormatHandler is NULL");
            }

            auto writer = fmtHandler->CreateWriter();

            Value id;
            try
            {

                Request request    = parse(aRequestData, id, aContentType);
                auto    parameters = request.GetParameters();
                auto&   a          = parameters.front();
                if (a.IsNil()) {
                    parameters.pop_front();
                }

                int64_t intId = 0;
                if (request.GetId().IsInteger32() or request.GetId().IsInteger64()) {
                    intId = request.GetId().AsInteger64();
                } else if (request.GetId().IsString()) {
                    auto strId = request.GetId().AsString();

                    try
                    {
                        intId = std::stoi(strId);
                    }
                    catch (...)
                    {
                    }
                }
                parameters.push_front(Value(intId));

                auto response = myDispatcher.Invoke(request.GetMethodName(), parameters, request.GetId());
                if (!response.GetId().IsBoolean() || response.GetId().AsBoolean() != false) {
                    // if Id is false, this is a notification and we don't have to write a response
                    response.Write(*writer);
                }
            } catch (const Fault& ex) {
                Response(ex.GetCode(), ex.GetString(), std::move(id)).Write(*writer);
            }

            return writer->GetData();
        }
    private:
        FormatHandler* getFormatHandler(const std::string& aContentType = "application/json")
        {

            // first find the correct handler
            for (auto handler : myFormatHandlers)
            {

                if (!handler)
                    continue;

                if (handler->CanHandleRequest(aContentType))
                {
                    return handler;
                }
            }

            return nullptr;
        }

        Dispatcher myDispatcher;
        std::vector<FormatHandler*> myFormatHandlers;
        Reader::GetNamedParamsFunc  getNamedParamsFunc;
    };

} // namespace jsonrpc

#endif // JSONRPC_LEAN_SERVER_H
