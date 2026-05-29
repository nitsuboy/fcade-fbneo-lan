#include "discovery.h"

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <iphlpapi.h>
    typedef HANDLE ThreadHandle;
    #define THREAD_FUNC DWORD WINAPI
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <ifaddrs.h>
    #include <net/if.h>
    #include <pthread.h>
    typedef pthread_t ThreadHandle;
    #define THREAD_FUNC void*
#endif

#include <stdio.h>
#include <string.h>

#define MAX_LOCAL_IPS 16

// ── platform: enumerate broadcast addresses ──────────────────────────

#ifdef _WIN32

static int winsock_initialized;

static int winsock_start(void)
{
    if (winsock_initialized) return 1;
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) return 0;
    winsock_initialized = 1;
    return 1;
}

static void winsock_end(void)
{
    if (!winsock_initialized) return;
    WSACleanup();
    winsock_initialized = 0;
}

static int enum_broadcasts(struct sockaddr_in *out, int max,
                           struct in_addr *local_ips, int *local_count)
{
    ULONG len = 15000;
    IP_ADAPTER_INFO *info = malloc(len);
    if (!info) return 0;

    DWORD r = GetAdaptersInfo(info, &len);
    if (r == ERROR_BUFFER_OVERFLOW) {
        IP_ADAPTER_INFO *tmp = realloc(info, len);
        if (!tmp) { free(info); return 0; }
        info = tmp;
        r = GetAdaptersInfo(info, &len);
    }

    int count = 0;
    *local_count = 0;

    if (r == NO_ERROR) {
        for (IP_ADAPTER_INFO *a = info; a && count < max; a = a->Next) {
            if (a->Type == 24) continue; // MIB_IF_TYPE_LOOPBACK

            struct in_addr ip, mask;
            ip.S_un.S_addr   = inet_addr(a->IpAddressList.IpAddress.String);
            mask.S_un.S_addr = inet_addr(a->IpAddressList.IpMask.String);

            if (ip.S_un.S_addr == 0 || mask.S_un.S_addr == 0) continue;

            if (*local_count < MAX_LOCAL_IPS) {
                local_ips[*local_count] = ip;
                (*local_count)++;
            }

            out[count].sin_family      = AF_INET;
            out[count].sin_port        = htons(BROADCAST_PORT);
            out[count].sin_addr.S_un.S_addr = ip.S_un.S_addr
                                            | ~mask.S_un.S_addr;
            count++;
        }
    }

    free(info);
    return count;
}

#else // Linux

static int enum_broadcasts(struct sockaddr_in *out, int max,
                           struct in_addr *local_ips, int *local_count)
{
    struct ifaddrs *ifaddr;
    if (getifaddrs(&ifaddr) < 0) return 0;

    int count = 0;
    *local_count = 0;

    for (struct ifaddrs *ifa = ifaddr; ifa && count < max; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;
        if (ifa->ifa_addr->sa_family != AF_INET) continue;
        if (!(ifa->ifa_flags & IFF_BROADCAST)) continue;
        if (ifa->ifa_flags & IFF_LOOPBACK) continue;

        struct sockaddr_in *bcast = (struct sockaddr_in *)ifa->ifa_broadaddr;
        if (!bcast || bcast->sin_addr.s_addr == 0) continue;

        if (*local_count < MAX_LOCAL_IPS) {
            struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
            local_ips[*local_count]  = addr->sin_addr;
            (*local_count)++;
        }

        out[count].sin_family = AF_INET;
        out[count].sin_port   = htons(BROADCAST_PORT);
        out[count].sin_addr   = bcast->sin_addr;
        count++;
    }

    freeifaddrs(ifaddr);
    return count;
}

#endif

// ── helper: is this a local IP? ──────────────────────────────────────

static int is_local_ip(struct in_addr *ip,
                       struct in_addr *local_ips, int count)
{
    for (int i = 0; i < count; i++) {
#ifdef _WIN32
        if (local_ips[i].S_un.S_addr == ip->S_un.S_addr) return 1;
#else
        if (local_ips[i].s_addr == ip->s_addr) return 1;
#endif
    }
    return 0;
}

// ── discovery thread ─────────────────────────────────────────────────

static THREAD_FUNC discovery_thread(void *arg)
{
    DiscoveryState *state = (DiscoveryState *)arg;

    // ── sockets ──────────────────────────────────────────────────────

#ifdef _WIN32
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET) return 1;
#else
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return NULL;
#endif

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
               (const char *)&opt, sizeof(opt));

    // bind to receive
    struct sockaddr_in recv_addr;
    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port   = htons(BROADCAST_PORT);
#ifdef _WIN32
    recv_addr.sin_addr.S_un.S_addr = INADDR_ANY;
    if (bind(sock, (struct sockaddr *)&recv_addr, sizeof(recv_addr))
        == SOCKET_ERROR) {
        closesocket(sock);
        return 1;
    }
#else
    recv_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sock, (struct sockaddr *)&recv_addr, sizeof(recv_addr)) < 0) {
        close(sock);
        return NULL;
    }
#endif

    // receive timeout
    struct timeval tv = {0, 200000}; // 200 ms
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
               (const char *)&tv, sizeof(tv));

    // ── enumerate broadcast addresses ────────────────────────────────

    struct sockaddr_in bcasts[16];
    struct in_addr     local_ips[MAX_LOCAL_IPS];
    int local_count;
    int bcast_count = enum_broadcasts(bcasts, 16, local_ips, &local_count);

    if (bcast_count == 0) {
        // fallback: limited broadcast on port 7500
        bcasts[0].sin_family = AF_INET;
        bcasts[0].sin_port   = htons(BROADCAST_PORT);
#ifdef _WIN32
        bcasts[0].sin_addr.S_un.S_addr = INADDR_BROADCAST;
#else
        bcasts[0].sin_addr.s_addr = INADDR_BROADCAST;
#endif
        bcast_count = 1;
        local_count = 0;
    }

    // hostname
    char hostname[PEER_NAME_LEN] = "unknown";
    gethostname(hostname, sizeof(hostname));

    // ── main loop ────────────────────────────────────────────────────

    state->running = 1;

    while (state->running) {
        // send beacon to every broadcast address
        for (int i = 0; i < bcast_count; i++)
            sendto(sock, hostname, strlen(hostname), 0,
                   (struct sockaddr *)&bcasts[i], sizeof(bcasts[i]));

        // collect replies (loop until timeout)
        for (;;) {
            struct sockaddr_in from;
            socklen_t from_len = sizeof(from);
            char buf[PEER_NAME_LEN];

#ifdef _WIN32
            int n = recvfrom(sock, buf, sizeof(buf) - 1, 0,
                             (struct sockaddr *)&from, &from_len);
            if (n == SOCKET_ERROR) break;
#else
            int n = recvfrom(sock, buf, sizeof(buf) - 1, 0,
                             (struct sockaddr *)&from, &from_len);
            if (n < 0) break;
#endif
            buf[n] = 0;

            // skip our own broadcast
            if (is_local_ip(&from.sin_addr, local_ips, local_count))
                continue;

            // dedup by IP
            int found = 0;
            for (int i = 0; i < state->count; i++) {
#ifdef _WIN32
                unsigned long a = inet_addr(state->peers[i].ip);
#else
                struct in_addr a;
                inet_pton(AF_INET, state->peers[i].ip, &a);
#endif
#ifdef _WIN32
                if (a == from.sin_addr.S_un.S_addr) { found = 1; break; }
#else
                if (a.s_addr == from.sin_addr.s_addr) { found = 1; break; }
#endif
            }
            if (found) continue;
            if (state->count >= MAX_PEERS) break;

            int idx = state->count;
#ifdef _WIN32
            snprintf(state->peers[idx].ip, sizeof(state->peers[idx].ip), "%s",
                     inet_ntoa(from.sin_addr));
#else
            inet_ntop(AF_INET, &from.sin_addr,
                      state->peers[idx].ip, sizeof(state->peers[idx].ip));
#endif
            snprintf(state->peers[idx].hostname, sizeof(state->peers[idx].hostname),
                     "%s", buf);
            state->count++;
            state->updated = 1;
        }

        // sleep between beacon cycles
#ifdef _WIN32
        Sleep(2000);
#else
        sleep(2);
#endif
    }

    // ── cleanup ──────────────────────────────────────────────────────

#ifdef _WIN32
    closesocket(sock);
    winsock_end();
#else
    close(sock);
#endif
    return 0;
}

// ── public API ───────────────────────────────────────────────────────

void discovery_start(DiscoveryState *state)
{
    state->count    = 0;
    state->running  = 0;
    state->updated  = 0;

#ifdef _WIN32
    if (!winsock_start()) return;
    ThreadHandle h = CreateThread(NULL, 0, discovery_thread, state, 0, NULL);
    if (h) CloseHandle(h);
    else   winsock_end();
#else
    ThreadHandle tid;
    if (pthread_create(&tid, NULL, discovery_thread, state) == 0)
        pthread_detach(tid);
#endif
}

void discovery_stop(DiscoveryState *state)
{
    state->running = 0;
#ifdef _WIN32
    Sleep(100);
#endif
}
