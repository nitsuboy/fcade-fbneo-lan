#ifndef DISCOVERY_H
#define DISCOVERY_H

#define MAX_PEERS      64
#define PEER_NAME_LEN  64
#define BROADCAST_PORT 7500

typedef struct {
    char ip[16];
    char hostname[PEER_NAME_LEN];
} Peer;

typedef struct {
    Peer  peers[MAX_PEERS];
    int   count;
    int   running;
    int   updated;
} DiscoveryState;

void discovery_start(DiscoveryState *state);
void discovery_stop(DiscoveryState *state);

#endif
