#pragma once
#include "Common/String.h"
#include "Common/Types.h"

/* A resolved or literal host/port pair identifying one end of a connection. */
struct IpEndpoint {
    String Address = "0.0.0.0";
    uint16_t Port = 0;

    IpEndpoint() = default;
    IpEndpoint(const String& InAddress, uint16_t InPort)
        : Address(InAddress), Port(InPort) {}
};
