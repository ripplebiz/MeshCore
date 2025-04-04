# Packet Structure

| Field         | Size (bytes)            | Description |
|--------------|------------------------|-------------|
| `header`     | 1                        | Contains routing type, payload type, and payload version. |
| `payload_len` | 2                        | Length of the payload in bytes. |
| `path_len`    | 2                        | Length of the path field in bytes. |
| `path`        | `MAX_PATH_SIZE`          | Stores the routing path if applicable. |
| `payload`     | `MAX_PACKET_PAYLOAD`     | The actual data being transmitted. |

## Header Breakdown

| Bits  | Mask            | Field           | Description |
|-------|---------------|----------------|-------------|
| 0-1   | `0x03`        | Route Type     | Specifies the routing type (Flood, Direct, Reserved). |
| 2-5   | `0x0F`        | Payload Type   | Specifies the type of payload (Request, Response, Text, ACK, etc.). |
| 6-7   | `0x03`        | Payload Version | Versioning of the payload format. |

## Route Type Values

| Value | Name                    | Description |
|--------|-------------------------|-------------|
| `0x00` | `ROUTE_TYPE_RESERVED1` | Reserved for future use. |
| `0x01` | `ROUTE_TYPE_FLOOD`     | Flood routing mode (builds up path). |
| `0x02` | `ROUTE_TYPE_DIRECT`    | Direct route (path is supplied). |
| `0x03` | `ROUTE_TYPE_RESERVED2` | Reserved for future use. |

## Payload Type Values

| Value | Name                    | Description |
|--------|-------------------------|-------------|
| `0x00` | `PAYLOAD_TYPE_REQ`      | Request (destination/source hashes + MAC). |
| `0x01` | `PAYLOAD_TYPE_RESPONSE` | Response to REQ or ANON_REQ. |
| `0x02` | `PAYLOAD_TYPE_TXT_MSG`  | Plain text message. |
| `0x03` | `PAYLOAD_TYPE_ACK`      | Acknowledgment. |
| `0x04` | `PAYLOAD_TYPE_ADVERT`   | Node advertisement. |
| `0x05` | `PAYLOAD_TYPE_GRP_TXT`  | Group text message (unverified). |
| `0x06` | `PAYLOAD_TYPE_GRP_DATA` | Group datagram (unverified). |
| `0x07` | `PAYLOAD_TYPE_ANON_REQ` | Anonymous request. |
| `0x08` | `PAYLOAD_TYPE_PATH`     | Returned path. |
| `0x0F` | `PAYLOAD_TYPE_RAW_CUSTOM` | Custom packet (raw bytes, custom encryption). |

## Payload Version Values

| Value | Name            | Description |
|--------|---------------|-------------|
| `0x00` | `PAYLOAD_VER_1` | 1-byte src/dest hashes, 2-byte MAC. |
| `0x01` | `PAYLOAD_VER_2` | Future version (e.g., 2-byte hashes, 4-byte MAC). |
| `0x02` | `PAYLOAD_VER_3` | Future version. |
| `0x03` | `PAYLOAD_VER_4` | Future version. |
