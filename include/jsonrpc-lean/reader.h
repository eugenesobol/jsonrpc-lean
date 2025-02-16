// This file is derived from xsonrpc Copyright (C) 2015 Erik Johansson <erik@ejohansson.se>
// This file is part of jsonrpc-lean, a c++11 JSON-RPC client/server library.
//
// Modifications and additions for jsonrpc-lean Copyright (C) 2015 Adriano Maia <tony@stark.im>
//

#ifndef JSONRPC_LEAN_READER_H
#define JSONRPC_LEAN_READER_H

#include <functional>
#include <string>
#include <vector>

namespace jsonrpc {

    class Request;
    class Response;
    class Value;

    class Reader {
    public:
        using NamedParams        = std::vector<std::string>;
        using GetNamedParamsFunc = std::function<const NamedParams&(const std::string& methodName)>;

        virtual ~Reader() {}

        virtual Request GetRequest(Value &id) = 0;
        virtual Response GetResponse() = 0;
        virtual Value GetValue() = 0;
    };

} // namespace jsonrpc

#endif // JSONRPC_LEAN_READER_H
