/************************************************************************************************
 *============================ IoTecha Proprietary and Confidential ============================*
 ************************************************************************************************
 *                                                                                              *
 *                                 All information contained in this document is confidential   *
 *  _____   _______        _       and proprietary to IoTecha Corp.  No license, expressed or   *
 * |_   _| |__   __|      | |      implied, under any patent, copyright or trade secret right   *
 *   | |  ___ | | ___  ___| |__   __ _  is granted or implied by the conveyance of this         *
 *   | | / _ \| |/ _ \/ __| '_ \ / _` | document.  No part of this document may be reproduced,  *
 *  _| || (_) | |  __/ (__| | | | (_| | transmitted, stored in a retrieval system, translated   *
 * |_____\___/|_|\___|\___|_| |_|\__,_| into any language, or computer language in any form by  *
 *                                      any means without prior written permission of IoTecha   *
 *                                                                                              *
 ************************************************************************************************/
#pragma once

#include "value.h"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace jsonrpc
{

class UserFault : public std::runtime_error
{
  public:
    UserFault(const std::string& what, Value faultData, std::int32_t faultCode = 0)
      : std::runtime_error(what)
      , faultCode(faultCode)
      , faultData(std::move(faultData))
    {
    }

    Value getFaultData() const
    {
        return Value(faultData);
    }

    std::int32_t getFaultCode() const
    {
        return faultCode;
    }

  private:
    std::int32_t faultCode;
    Value        faultData;
};

} // namespace jsonrpc
