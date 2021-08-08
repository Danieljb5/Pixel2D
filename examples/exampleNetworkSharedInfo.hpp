#ifndef EXAMPLE_NETWORK_IDS_HPP
#define EXAMPLE_NETWORK_IDS_HPP

#include <cstdint>

enum MsgTypes : uint32_t
{
    ServerAccept, ServerDeny, ServerPing, MessageAll, ServerMessage, ConnectionsCheck
};

#endif