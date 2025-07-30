#pragma once

#define UDP_BRIDGE_IPV4 0x0
#define UDP_BRIDGE_IPV6 0x1

#define UDP_BRIDGE_MODE_BROADCAST 0x0
#define UDP_BRIDGE_MODE_MULTICAST 0x1
#define UDP_BRIDGE_MODE_DIRECT 0x2

typedef struct UDPBridgeFlags {
    uint8_t network_listen_enable: 1;
    uint8_t rx_enable: 1;
    uint8_t tx_enable: 1;
    uint8_t ip_version: 1;               // ip_mode(ipv4, ipv6)
    uint8_t mode : 2;                    // bridge_mode(broadcast, multicast, direct)
    uint8_t reserved: 2;
}; 

typedef struct UDPBridgePrefs {
    UDPBridgeFlags flags; 
    uint16_t port;

    union address {
        uint8_t ipv4[4];
        uint8_t ipv6[16];    
    };
};

//bool udp_bridge_flags;  // bridge_rx_enable, bridge_tx_enable, bridge_listen_enable, ip_mode(ipv4, ipv6),