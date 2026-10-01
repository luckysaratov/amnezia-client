#include "reconnectIcmp.h"

#if defined(_WIN32)

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>

#include <vector>

#ifdef _MSC_VER
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#endif

namespace reconnect_icmp
{
    int pingIpv4(const std::string &host, int timeoutMs)
    {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
            return -1;
        }

        int result = -1;

        addrinfo hints = {};
        hints.ai_family = AF_INET;
        addrinfo *res = nullptr;
        if (getaddrinfo(host.c_str(), nullptr, &hints, &res) == 0 && res != nullptr) {
            const IPAddr address = reinterpret_cast<sockaddr_in *>(res->ai_addr)->sin_addr.S_un.S_addr;
            freeaddrinfo(res);

            HANDLE icmp = IcmpCreateFile();
            if (icmp != INVALID_HANDLE_VALUE) {
                char payload[32] = "amnezia-reconnect-probe";
                std::vector<char> reply(sizeof(ICMP_ECHO_REPLY) + sizeof(payload) + 8);

                const DWORD replies = IcmpSendEcho(icmp, address, payload, sizeof(payload), nullptr,
                                                   reply.data(), static_cast<DWORD>(reply.size()),
                                                   static_cast<DWORD>(timeoutMs));
                if (replies > 0) {
                    const ICMP_ECHO_REPLY *echo = reinterpret_cast<const ICMP_ECHO_REPLY *>(reply.data());
                    result = (echo->Status == IP_SUCCESS) ? 1 : 0;
                } else {
                    result = 0;
                }
                IcmpCloseHandle(icmp);
            }
        } else if (res != nullptr) {
            freeaddrinfo(res);
        }

        WSACleanup();
        return result;
    }
}

#else

namespace reconnect_icmp
{
    int pingIpv4(const std::string &, int)
    {
        return -1;
    }
}

#endif
