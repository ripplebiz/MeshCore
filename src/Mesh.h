#pragma once

#include <Dispatcher.h>

namespace mesh {

/**
 * An abstraction of the device's Realtime Clock.
*/
class RTCClock {
public:
  /**
   * \returns  the current time. in UNIX epoch seconds.
  */
  virtual uint32_t getCurrentTime() = 0;

  /**
   * \param time  current time in UNIX epoch seconds.
  */
  virtual void setCurrentTime(uint32_t time) = 0;
};

class GroupChannel {
public:
  uint8_t hash[PATH_HASH_SIZE];
  uint8_t secret[PUB_KEY_SIZE];
};

/**
 * An abstraction of the data tables needed to be maintained
*/
class MeshTables {
public:
  virtual bool hasSeen(const Packet* packet) = 0;
};

/**
 * \brief  The next layer in the basic Dispatcher task, Mesh recognises the particular Payload TYPES,
 *     and provides virtual methods for sub-classes on handling incoming, and also preparing outbound Packets.
*/
class Mesh : public Dispatcher {
  RTCClock* _rtc;
  RNG* _rng;
  MeshTables* _tables;

protected:
  DispatcherAction onRecvPacket(Packet* pkt) override;

  /**
   * \brief  Decide what to do with received packet, ie. discard, forward, or hold
   */
  DispatcherAction routeRecvPacket(Packet* packet);

  /**
   * \brief  Check whether this packet should be forwarded (re-transmitted) or not.
   *     Is sub-classes responsibility to make sure given packet is only transmitted ONCE (by this node)
   */
  virtual bool allowPacketForward(const Packet* packet);

  /**
   * \returns  number of milliseconds delay to apply to retransmitting the given packet.
   */
  virtual uint32_t getRetransmitDelay(const Packet* packet);

  /**
   * \brief  Perform search of local DB of peers/contacts.
   * \returns  Number of peers with matching hash
   */
  virtual int searchPeersByHash(const uint8_t* hash);

  /**
   * \brief  lookup the ECDH shared-secret between this node and peer by idx (calculate if necessary)
   * \param  dest_secret  destination array to copy the secret (must be PUB_KEY_SIZE bytes)
   * \param  peer_idx  index of peer, [0..n) where n is what searchPeersByHash() returned
   */
  virtual void getPeerSharedSecret(uint8_t* dest_secret, int peer_idx) { }

  /**
   * \brief  A (now decrypted) data packet has been received (by a known peer).
   *         NOTE: these can be received multiple times (per sender/msg-id), via different routes
   * \param  type  one of: PAYLOAD_TYPE_TXT_MSG, PAYLOAD_TYPE_REQ, PAYLOAD_TYPE_RESPONSE
   * \param  sender_idx  index of peer, [0..n) where n is what searchPeersByHash() returned
  */
  virtual void onPeerDataRecv(Packet* packet, uint8_t type, int sender_idx, uint8_t* data, size_t len) { }

  /**
   * \brief  A path TO peer (sender_idx) has been received. (also with optional 'extra' data encoded)
   *         NOTE: these can be received multiple times (per sender), via differen routes
   * \param  sender_idx  index of peer, [0..n) where n is what searchPeersByHash() returned
  */
  virtual void onPeerPathRecv(Packet* packet, int sender_idx, uint8_t* path, uint8_t path_len, uint8_t extra_type, uint8_t* extra, uint8_t extra_len) { }

  virtual int searchChannelsByHash(const uint8_t* hash, GroupChannel channels[], int max_matches);

  /**
   * \brief  A new incoming Advertisement has been received.
   *         NOTE: these can be received multiple times (per id/timestamp), via different routes
  */
  virtual void onAdvertRecv(Packet* packet, const Identity& id, uint32_t timestamp, const uint8_t* app_data, size_t app_data_len) { }

  /**
   * \brief  A (now decrypted) data packet has been received.
   *         NOTE: these can be received multiple times (per sender/contents), via different routes
   * \param  type  one of: PAYLOAD_TYPE_ANON_REQ
   * \param  sender  public key provided by sender
  */
  virtual void onAnonDataRecv(Packet* packet, uint8_t type, const Identity& sender, uint8_t* data, size_t len) { }

  /**
   * \brief  A path TO 'sender' has been received. (also with optional 'extra' data encoded)
   *         NOTE: these can be received multiple times (per sender), via differen routes
  */
  virtual void onPathRecv(Packet* packet, Identity& sender, uint8_t* path, uint8_t path_len, uint8_t extra_type, uint8_t* extra, uint8_t extra_len) { }

  /**
   * \brief  An encrypted group data packet has been received.
   *         NOTE: the same payload can be received multiple times, via different routes
   * \param  type  one of: PAYLOAD_TYPE_GRP_TXT, PAYLOAD_TYPE_GRP_DATA
   * \param  channel  the matching GroupChannel
  */
  virtual void onGroupDataRecv(Packet* packet, uint8_t type, const GroupChannel& channel, uint8_t* data, size_t len) { }

  /**
   * \brief  A simple ACK packet has been received.
   *         NOTE: same ACK can be received multiple times, via different routes
  */
  virtual void onAckRecv(Packet* packet, uint32_t ack_crc) { }

  Mesh(Radio& radio, MillisecondClock& ms, RNG& rng, RTCClock& rtc, PacketManager& mgr, MeshTables& tables)
    : Dispatcher(radio, ms, mgr), _rng(&rng), _rtc(&rtc), _tables(&tables)
  {
  }

public:
  void begin();
  void loop();

  LocalIdentity self_id;

  RNG* getRNG() const { return _rng; }
  RTCClock* getRTCClock() const { return _rtc; }

  Packet* createAdvert(const LocalIdentity& id, const uint8_t* app_data=NULL, size_t app_data_len=0);
  Packet* createDatagram(uint8_t type, const Identity& dest, const uint8_t* secret, const uint8_t* data, size_t len);
  Packet* createAnonDatagram(uint8_t type, const LocalIdentity& sender, const Identity& dest, const uint8_t* secret, const uint8_t* data, size_t data_len);
  Packet* createGroupDatagram(uint8_t type, const GroupChannel& channel, const uint8_t* data, size_t data_len);
  Packet* createAck(uint32_t ack_crc);
  Packet* createPathReturn(const Identity& dest, const uint8_t* secret, const uint8_t* path, uint8_t path_len, uint8_t extra_type, const uint8_t*extra, size_t extra_len);

  /**
   * \brief  send a locally-generated Packet with flood routing
  */
  void sendFlood(Packet* packet, uint32_t delay_millis=0);

  /**
   * \brief  send a locally-generated Packet with Direct routing
  */
  void sendDirect(Packet* packet, const uint8_t* path, uint8_t path_len, uint32_t delay_millis=0);

  /**
   * \brief  send a locally-generated Packet to just neigbor nodes (zero hops)
  */
  void sendZeroHop(Packet* packet, uint32_t delay_millis=0);
};

}
