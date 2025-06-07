# Meshcore payloads
Inside of each [meshcore packet](./packet_structure.md) is a payload, identified by the payload type in the packet header. The types of payloads are:

* Request (destination/source hashes + MAC).
* Response to REQ or ANON_REQ.
* Plain text message.
* Acknowledgment.
* Node advertisement.
* Group text message (unverified).
* Group datagram (unverified).
* Anonymous request.
* Returned path.
* Custom packet (raw bytes, custom encryption).

This document defines the structure of each of these payload types

# Node advertisement
This kind of payload notifies receivers that a node exists, and gives information about the node

| Field         | Size (bytes)    | Description                                              |
|---------------|-----------------|----------------------------------------------------------|
| public key    | 32              | Ed25519 public key                                       |
| timestamp     | 4               | unix timestamp of advertisement                          |
| signature     | 64              | Ed25519 signature of public key, timestamp, and app data |
| appdata       | rest of payload | optional, see below                                      |

Appdata

| Field         | Size (bytes)    | Description                                           |
|---------------|-----------------|-------------------------------------------------------|
| flags         | 1               | specifies which of the fields are present, see below  |
| latitude      | 4               | decimal latitude multiplied by 1000000, integer       |
| longitude     | 4               | decimal longitude multiplied by 1000000, integer      |
| feature 1     | 2               | reserved for future use                               |
| feature 2     | 2               | reserved for future use                               |
| name          | rest of appdata | name of the node                                      |

Appdata Flags

| Value  | Name      | Description                           |
|--------|-----------|---------------------------------------|
| `0x10` | location  | appdata contains lat/long information |
| `0x20` | feature 1 | Reserved for future use.              |
| `0x40` | feature 2 | Reserved for future use.              |
| `0x80` | name      | appdata contains a node name          |

# Acknowledgement
| Field    | Size (bytes) | Description                                                |
|----------|--------------|------------------------------------------------------------|
| checksum | 4            | CRC checksum of message timestamp, text, and sender pubkey |


# Returned path, request, response, and plain text message
| Field            | Size (bytes)    | Description                                          |
|------------------|-----------------|------------------------------------------------------|
| destination hash | 1               | first byte of destination node public key            |
| source hash      | 1               | first byte of source node public key                 |
| cipher MAC       | 2               | MAC for encrypted data in next field                 |
| ciphertext       | rest of payload | encrypted message, see subsections below for details |

## Returned path

| Field       | Size (bytes) | Description                                                                                  |
|-------------|--------------|----------------------------------------------------------------------------------------------|
| path length | 1            | length of next field                                                                         |
| path        | see above    | a list of node hashes (one byte each) describing the route from us to the packet author      |
| extra type  | 1            | extra, bundled payload type, eg., acknowledgement or response. See packet structure spec     |
| extra       | rest of data | extra, bundled payload content, follows same format as main content defined by this document |

## Request

| Field        | Size (bytes)    | Description                |
|--------------|-----------------|----------------------------|
| timestamp    | 4               | send time (unix timestamp) |
| request type | 1               | see below                  |
| request data | rest of payload | depends on request type    |

Request type

| Value  | Name               | Description                           |
|--------|--------------------|---------------------------------------|
| `0x01` | get status         | get status of repeater or room server |
| `0x02` | keepalive          | TODO |
| `0x03` | get telemetry data | TODO |

### Get status

Gets information about the node, possibly including the following:

* Battery level (millivolts)
* Current transmit queue length
* Current free queue length
* Last RSSI value
* Number of received packets
* Number of sent packets
* Total airtime (seconds)
* Total uptime (seconds)
* Number of packets sent as flood
* Number of packets sent directly
* Number of packets received as flood
* Number of packets received directly
* Error flags
* Last SNR value
* Number of direct route duplicates
* Number of flood route duplicates
* Number posted (?)
* Number of post pushes (?)

### Keepalive

No-op request.

### Get telemetry data

Request data about sensors on the node, including battery level.

## Response

| Field   | Size (bytes)    | Description |
|---------|-----------------|-------------|
| tag     | 4               | TODO        |
| content | rest of payload | TODO        |

## Plain text message

| Field        | Size (bytes)    | Description                                                  |
|--------------|-----------------|--------------------------------------------------------------|
| timestamp    | 4               | send time (unix timestamp)                                   |
| flags + TODO | 1               | first six bits are flags (see below), last two bits are TODO |
| message      | rest of payload | the message content, see next table                          |

Flags

| Value  | Description               | Message content                                            |
|--------|---------------------------|------------------------------------------------------------|
| `0x00` | plain text message        | the plain text of the message                              |
| `0x01` | CLI command               | the command text of the message                            |
| `0x02` | signed plain text message | two bytes of sender prefix, followed by plain text message |

# Anonymous request

| Field            | Size (bytes)    | Description                               |
|------------------|-----------------|-------------------------------------------|
| destination hash | 1               | first byte of destination node public key |
| public key       | 32              | sender's Ed25519 public key               |
| cipher MAC       | 2               | MAC for encrypted data in next field      |
| ciphertext       | rest of payload | encrypted message, see below for details  |

Plaintext message

| Field          | Size (bytes)    | Description                                                                   |
|----------------|-----------------|-------------------------------------------------------------------------------|
| timestamp      | 4               | send time (unix timestamp)                                                    |
| sync timestamp | 4               | for room server, otherwise absent: sender's "sync messages SINCE x" timestamp |
| password       | rest of message | password for repeater/room                                                    |

# Group text message / datagram

| Field        | Size (bytes)    | Description                              |
|--------------|-----------------|------------------------------------------|
| channel hash | 1               | TODO                                     | 
| cipher MAC   | 2               | MAC for encrypted data in next field     |
| ciphertext   | rest of payload | encrypted message, see below for details |

Plaintext for text message

| Field     | Size (bytes)    | Description                      |
|-----------|-----------------|----------------------------------|
| timestamp | 4               | send time (unix timestamp)       |
| content   | rest of message | plain group text message content |

TODO: describe what datagram looks like

# Custom packet

Custom packets have no defined format.