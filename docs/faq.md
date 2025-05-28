**MeshCore-FAQ**<!-- omit from toc -->
A list of frequently-asked questions and answers for MeshCore

The current version of this MeshCore FAQ is at https://github.com/ripplebiz/MeshCore/blob/main/docs/faq.md.  
This MeshCore FAQ is also mirrored at https://github.com/LitBomb/MeshCore-FAQ and might have newer updates if pull requests on Scott's MeshCore repo are not approved yet.

author: https://github.com/LitBomb<!-- omit from toc -->
---

- [1. Introduction](#1-introduction)
  - [1.1. Q: What is MeshCore?](#11-q-what-is-meshcore)
  - [1.2. Q: What do you need to start using MeshCore?](#12-q-what-do-you-need-to-start-using-meshcore)
    - [1.2.1. Hardware](#121-hardware)
    - [1.2.2. Firmware](#122-firmware)
    - [1.2.3. Companion Radio Firmware](#123-companion-radio-firmware)
    - [1.2.4. Repeater](#124-repeater)
    - [1.2.5. Room Server](#125-room-server)
- [2. Initial Setup](#2-initial-setup)
  - [2.1. Q: How many devices do I need to start using MeshCore?](#21-q-how-many-devices-do-i-need-to-start-using-meshcore)
  - [2.2. Q: Does MeshCore cost any money?](#22-q-does-meshcore-cost-any-money)
  - [2.3. Q: What frequencies are supported by MeshCore?](#23-q-what-frequencies-are-supported-by-meshcore)
  - [2.4. Q: What is an "advert" in MeshCore?](#24-q-what-is-an-advert-in-meshcore)
  - [2.5. Q: Is there a hop limit?](#25-q-is-there-a-hop-limit)
- [3. Server Administration](#3-server-administration)
  - [3.1. Q: How do you configure a repeater or a room server?](#31-q-how-do-you-configure-a-repeater-or-a-room-server)
  - [3.2. Q: Do I need to set the location for a repeater?](#32-q-do-i-need-to-set-the-location-for-a-repeater)
  - [3.3. Q: What is the password to administer a repeater or a room server?](#33-q-what-is-the-password-to-administer-a-repeater-or-a-room-server)
  - [3.4. Q: What is the password to join a room server?](#34-q-what-is-the-password-to-join-a-room-server)
- [4. T-Deck Related](#4-t-deck-related)
  - [4.1. Q: What are the steps to get a T-Deck into DFU (Device Firmware Update) mode?](#41-q-what-are-the-steps-to-get-a-t-deck-into-dfu-device-firmware-update-mode)
  - [4.2. Q: Why is my T-Deck Plus not getting any satellite lock?](#42-q-why-is-my-t-deck-plus-not-getting-any-satellite-lock)
  - [4.3. Q: Why is my OG (non-Plus) T-Deck not getting any satellite lock?](#43-q-why-is-my-og-non-plus-t-deck-not-getting-any-satellite-lock)
  - [4.4. Q: What size of SD card does the T-Deck support?](#44-q-what-size-of-sd-card-does-the-t-deck-support)
  - [4.5. Q: How do I get maps on T-Deck?](#45-q-how-do-i-get-maps-on-t-deck)
  - [4.6. Q: Where do the map tiles go?](#46-q-where-do-the-map-tiles-go)
  - [4.7. Q: How to unlock deeper map zoom and server management features on T-Deck?](#47-q-how-to-unlock-deeper-map-zoom-and-server-management-features-on-t-deck)
  - [4.8. Q: How to decipher the diagnostics screen on T-Deck?](#48-q-how-to-decipher-the-diagnostics-screen-on-t-deck)
  - [4.9. Q: The T-Deck sound is too loud?](#49-q-the-t-deck-sound-is-too-loud)
  - [4.10. Q: Can you customize the sound?](#410-q-can-you-customize-the-sound)
  - [4.11. Q: What is the 'Import from Clipboard' feature on the t-deck and is there a way to manually add nodes without having to receive adverts?](#411-q-what-is-the-import-from-clipboard-feature-on-the-t-deck-and-is-there-a-way-to-manually-add-nodes-without-having-to-receive-adverts)
- [5. General](#5-general)
  - [5.1. Q: What are BW, SF, and CR?](#51-q-what-are-bw-sf-and-cr)
  - [5.2. Q: Do MeshCore clients repeat?](#52-q-do-meshcore-clients-repeat)
  - [5.3. Q: What happens when a node learns a route via a mobile repeater, and that repeater is gone?](#53-q-what-happens-when-a-node-learns-a-route-via-a-mobile-repeater-and-that-repeater-is-gone)
  - [5.4. Q: How does a node discovery a path to its destination and then use it to send messages in the future, instead of flooding every message it sends like Meshtastic?](#54-q-how-does-a-node-discovery-a-path-to-its-destination-and-then-use-it-to-send-messages-in-the-future-instead-of-flooding-every-message-it-sends-like-meshtastic)
  - [5.5. Q: Do public channels always flood? Do private channels always flood?](#55-q-do-public-channels-always-flood-do-private-channels-always-flood)
  - [5.6. Q: what is the public key for the default public channel?](#56-q-what-is-the-public-key-for-the-default-public-channel)
  - [5.7. Q: Is MeshCore open source?](#57-q-is-meshcore-open-source)
  - [5.8. Q: How can I support MeshCore?](#58-q-how-can-i-support-meshcore)
  - [5.9. Q: How do I build MeshCore firmware from source?](#59-q-how-do-i-build-meshcore-firmware-from-source)
  - [5.10. Q: Are there other MeshCore related open source projects?](#510-q-are-there-other-meshcore-related-open-source-projects)
  - [5.11. Q: Does MeshCore support ATAK](#511-q-does-meshcore-support-atak)
  - [5.12. Q: How do I add a node to the MeshCore Map](#512-q-how-do-i-add-a-node-to-the-meshcore-map)
  - [5.13. Q: Can I use a Raspberry Pi to update a MeshCore radio?](#513-q-can-i-use-a-raspberry-pi-to-update-a-meshcore-radio)
  - [5.14. Q: Are there are projects built around MeshCore?](#514-q-are-there-are-projects-built-around-meshcore)
    - [5.14.1. meshcoremqtt](#5141-meshcoremqtt)
    - [5.14.2. MeshCore for Home Assistant](#5142-meshcore-for-home-assistant)
    - [5.14.3. Python MeshCore](#5143-python-meshcore)
    - [5.14.4. meshcore-cli](#5144-meshcore-cli)
    - [5.14.5. meshcore.js](#5145-meshcorejs)
- [6. Troubleshooting](#6-troubleshooting)
  - [6.1. Q: My client says another client or a repeater or a room server was last seen many, many days ago.](#61-q-my-client-says-another-client-or-a-repeater-or-a-room-server-was-last-seen-many-many-days-ago)
  - [6.2. Q: A repeater or a client or a room server I expect to see on my discover list (on T-Deck) or contact list (on a smart device client) are not listed.](#62-q-a-repeater-or-a-client-or-a-room-server-i-expect-to-see-on-my-discover-list-on-t-deck-or-contact-list-on-a-smart-device-client-are-not-listed)
  - [6.3. Q: How to connect to a repeater via BLE (Bluetooth)?](#63-q-how-to-connect-to-a-repeater-via-ble-bluetooth)
  - [6.4. Q: I can't connect via Bluetooth, what is the Bluetooth pairing code?](#64-q-i-cant-connect-via-bluetooth-what-is-the-bluetooth-pairing-code)
  - [6.5. Q: My Heltec V3 keeps disconnecting from my smartphone.  It can't hold a solid Bluetooth connection.](#65-q-my-heltec-v3-keeps-disconnecting-from-my-smartphone--it-cant-hold-a-solid-bluetooth-connection)
- [7. Other Questions:](#7-other-questions)
  - [7.1. Q: How to  Update repeater and room server firmware over the air?](#71-q-how-to--update-repeater-and-room-server-firmware-over-the-air)

## 1. Introduction

### 1.1. Q: What is MeshCore?

**A:** MeshCore is free and open source
* MeshCore is the routing and firmware etc, available on GitHub under MIT license
* There are clients made by the community, such as the web clients, these are free to use, and some are open source too
* The cross platform mobile app developed by [Liam Cottle](https://liamcottle.net) for Android/iOS/PC etc is free to download and use
* The T-Deck firmware is developed by Scott at Ripple Radios, the creator of MeshCore, is also free to flash on your devices and use


Some more advanced, but optional features are available on T-Deck if you register your device for a key to unlock.  On the MeshCore smartphone clients for Android and iOS/iPadOS, you can unlock the wait timer for repeater and room server remote management over RF feature. 

These features are completely optional and aren't needed for the core messaging experience. They're like super bonus features and to help the developers continue to work on these amazing features, they may charge a small fee for an unlock code to utilise the advanced features.

Anyone is able to build anything they like on top of MeshCore without paying anything.

### 1.2. Q: What do you need to start using MeshCore?
**A:** Everything you need for MeshCore is available at:
 Main web site: [https://meshcore.co.uk/](https://meshcore.co.uk/)
 Firmware Flasher: https://flasher.meshcore.co.uk/
 Phone Client Applications: https://meshcore.co.uk/apps.html
 MeshCore Fimrware GitHub: https://github.com/ripplebiz/MeshCore

 NOTE: Andy Kirby has a very useful [intro video](https://www.youtube.com/watch?v=t1qne8uJBAc) for beginners.
 

 You need LoRa hardware devices to run MeshCore firmware as clients or server (repeater and room server).

#### 1.2.1. Hardware
To use MeshCore without using a phone as the client interface, you can run MeshCore on a T-Deck or T-Deck Plus. It is a complete off-grid secure communication solution.  

MeshCore is also available on a variety of 868MHz and 915MHz LoRa devices. For example, RAK4631 devices (19003, 19007, 19026), Heltec V3, Xiao S3 WIO, Xiao C3, Heltec T114, Station G2, Seeed Studio T1000-E. More devices will be supported later.

#### 1.2.2. Firmware
MeshCore has four firmware types that are not available on other LoRa systems. MeshCore has the following:

#### 1.2.3. Companion Radio Firmware
Companion radios are for connecting to the Android app or web app as a messenger client. There are two different companion radio firmware versions:

1. **BLE Companion**  
   BLE Companion firmware runs on a supported LoRa device and connects to a smart device running the Android MeshCore client over BLE (iOS MeshCore client will be available soon)  
   <https://meshcore.co.uk/apps.html>

2. **USB Serial Companion**  
   USB Serial Companion firmware runs on a supported LoRa device and connects to a smart device or a computer over USB Serial running the MeshCore web client  
   <https://meshcore.liamcottle.net/#/>  
   <https://client.meshcore.co.uk/tabs/devices>

#### 1.2.4. Repeater
Repeaters are used to extend the range of a MeshCore network. Repeater firmware runs on the same devices that run client firmware. A repeater's job is to forward MeshCore packets to the destination device. It does **not** forward or retransmit every packet it receives, unlike other LoRa mesh systems.  

A repeater can be remotely administered using a T-Deck running the MeshCore firmware with remote administration features unlocked, or from a BLE Companion client connected to a smartphone running the MeshCore app.

#### 1.2.5. Room Server
A room server is a simple BBS server for sharing posts. T-Deck devices running MeshCore firmware or a BLE Companion client connected to a smartphone running the MeshCore app can connect to a room server. 

Room servers store message history on them and push the stored messages to users.  Room servers allow roaming users to come back later and retrieve message history.  With channels, messages are either received when it's sent, or not received and missed if the channel user is out of range.  Room servers are different and more like email servers where you can come back later and get your emails from your mail server.

A room server can be remotely administered using a T-Deck running the MeshCore firmware with remote administration features unlocked, or from a BLE Companion client connected to a smartphone running the MeshCore app.  

When a client logs into a room server, the client will receive the previously 32 unseen messages.

A room server can also take on the repeater role.  To enable repeater role on a room server, use this command:

`set repeat {on|off}`

---

## 2. Initial Setup

### 2.1. Q: How many devices do I need to start using MeshCore?
**A:** If you have one supported device, flash the BLE Companion firmware and use your device as a client.  You can connect to the device using the Android client via Bluetooth (iOS client will be available later).  You can start communicating with other MeshCore users near you.

If you have two supported devices, and there are not many MeshCore users near you, flash both to BLE Companion firmware so you can use your devices to communicate with your near-by friends and family.

If you have two supported devices, and there are other MeshcCore users nearby, you can flash one of your devices with BLE Companion firmware and flash another supported device to repeater firmware.  Place the repeater high above ground to extend your MeshCore network's reach.

After you flashed the latest firmware onto your repeater device, keep the device connected to your computer via USB serial, use the console feature on the web flasher and set the frequency for your region or country, so your client can remote administer the repeater or room server over RF:

`set freq {frequency}`

The repeater and room server CLI reference is here: https://github.com/ripplebiz/MeshCore/wiki/Repeater-&-Room-Server-CLI-Reference

If you have more supported devices, you can use your additional devices with the room server firmware.  

### 2.2. Q: Does MeshCore cost any money?

**A:** All radio firmware versions (e.g. for Heltec V3, RAK, T-1000E, etc) are free and open source developed by Scott at Ripple Radios.  

The native Android and iOS client uses the freemium model and is developed by Liam Cottle, developer of meshtastic map at [meshtastic.liamcottle.net](https://meshtastic.liamcottle.net) on [GitHub](https://github.com/liamcottle/meshtastic-map) and [reticulum-meshchat on github](https://github.com/liamcottle/reticulum-meshchat). 

The T-Deck firmware is free to download and most features are available without cost.  To support the firmware developer, you can pay for a registration key to unlock your T-Deck for deeper map zoom and remote server administration over RF using the T-Deck.  You do not need to pay for the registration to use your T-Deck for direct messaging and connecting to repeaters and room servers. 


### 2.3. Q: What frequencies are supported by MeshCore?
**A:** It supports the 868MHz range in the UK/EU and the 915MHz range in New Zealand, Australia, and the USA. Countries and regions in these two frequency ranges are also supported. The firmware and client allow users to set their preferred frequency.  
- Australia and New Zealand are on **915.8MHz**
- UK and EU are on **869.525MHz**
- Canada and USA are on **910.525MHz**
- For other regions and countries, please check your local LoRa frequency

In UK and EU, 867.5MHz is not allowed to use 250kHz bandwidth and it only allows 2.5% duty cycle for clients.  869.525Mhz allows an airtime of 10%, 250KHz bandwidth, and a higher EIRP, therefore MeshCore nodes can send more often and with more power. That is why this frequency is chosen for UK and EU.  This is also why Meshtastic also uses this frequency.  

[Source]([https://](https://discord.com/channels/826570251612323860/1330643963501351004/1356540643853209641))

the rest of the radio settings are the same for all frequencies:  
- Spread Factor (SF): 11  
- Coding Rate (CR): 5  
- Bandwidth (BW): 250.00  

(Originally MeshCore started with SF 10.  recently (as of late April 2025) the community has advocated SF 11 also a viable option for longer range but a little slower transmission. Currently there are MeshCore meshes with SF 10 and SF 11.  Liam Cottle's smartphone app's presets now recommend SF 10 for Australia and SF 11 for all other regions and countries.  EU and UK has SF 10 and SF 11 presets.  Work with your local meshers on deciding with SF number is best for your use cases.  In the future, there may be bridge nodes that can bridge SF 10 and SF 11 (or even different frequencies) traffic.)

### 2.4. Q: What is an "advert" in MeshCore?
**A:** 
Advert means to advertise yourself on the network. In Reticulum terms it would be to announce. In Meshtastic terms it would be the node sending its node info.

MeshCore allows you to manually broadcast your name, position and public encryption key, which is also signed to prevent spoofing.  When you click the advert button, it broadcasts that data over LoRa.  MeshCore calls that an Advert. There's two ways to advert, "zero hop" and "flood".

* Zero hop means your advert is broadcasted out to anyone that can hear it, and that's it.
* Flooded means it's broadcasted out and then repeated by all the repeaters that hear it.

MeshCore clients only advertise themselves when the user initiates it. A repeater (and room server?) advertises its presence once every 240 minutes. This interval can be configured using the following command:

`set advert.interval {minutes}`

### 2.5. Q: Is there a hop limit?

**A:** Internally the firmware has maximum limit of 64 hops.  In real world settings it will be difficult to get close to the limit due to the environments and timing as packets travel further and further.  We want to hear how far your MeshCore conversations go. 


---


## 3. Server Administration

### 3.1. Q: How do you configure a repeater or a room server?

**A:** - When MeshCore is flashed onto a LoRa device is for the first time, it is necessary to set the server device's frequency to make it utilize the frequency that is legal in your country or region.  

Repeater or room server can be administered with one of the options below:
  
- After a repeater or room server firmware is flashed on to a LoRa device, go to <https://config.meshcore.dev> and use the web user interface to connect to the LoRa device via USB serial.  From there you can set the name of the server, its frequency and other related settings, location, passwords etc.

![image](https://github.com/user-attachments/assets/bec28ff3-a7d6-4a1e-8602-cb6b290dd150)

 
- Connect the server device using a USB cable to a computer running Chrome on https://flasher.meshcore.co.uk/, then use the `console` feature to connect to the device

- Use a MeshCore smartphone clients  to remotely administer servers via LoRa.

- A T-Deck running unlocked/registered MeshCore firmware. Remote server administration is enabled through registering your T-Deck with Ripple Radios. It is one of the ways to support MeshCore development. You can register your T-Deck at:

<https://buymeacoffee.com/ripplebiz/e/249834>

  

### 3.2. Q: Do I need to set the location for a repeater?
**A:** With location set for a repeater, it can show up on a MeshCore map in the future. Set location with the following commands:

`set lat <GPS Lat> set long <GPS Lon>`

You can get the latitude and longitude from Google Maps by right-clicking the location you are at on the map.

### 3.3. Q: What is the password to administer a repeater or a room server?
**A:** The default admin password to a repeater and room server is `password`. Use the following command to change the admin password:

`password {new-password}`


### 3.4. Q: What is the password to join a room server?
**A:** The default guest password to a room server is `hello`. Use the following command to change the guest password:

`set guest.password {guest-password}`


---

## 4. T-Deck Related

### 4.1. Q: What are the steps to get a T-Deck into DFU (Device Firmware Update) mode?
**A:**  
1. Device off  
2. Connect USB cable to device  
3. Hold down trackball (keep holding)  
4. Turn on device  
5. Hear USB connection sound  
6. Release trackball  
7. T-Deck in DFU mode now  
8. At this point you can begin flashing using <https://flasher.meshcore.co.uk/>

### 4.2. Q: Why is my T-Deck Plus not getting any satellite lock?
**A:** For T-Deck Plus, the GPS baud rate should be set to **38400**. Also, some T-Deck Plus devices were found to have the GPS module installed upside down, with the GPS antenna facing down instead of up. If your T-Deck Plus still doesn't get any satellite lock after setting the baud rate to 38400, you might need to open the device to check the GPS orientation.

GPS on T-Deck is always enabled.  You can skip the "GPS clock sync" and the T-Deck will continue to try to get a GPS lock.  You can go to the `GPS Info` screen; you should see the `Sentences:` counter increasing if the baud rate is correct.

[Source]([https://](https://discord.com/channels/826570251612323860/1330643963501351004/1356609240302616689))

### 4.3. Q: Why is my OG (non-Plus) T-Deck not getting any satellite lock?
**A:** The OG (non-Plus) T-Deck doesn't come with a GPS. If you added a GPS to your OG T-Deck, please refer to the manual of your GPS to see what baud rate it requires. Alternatively, you can try to set the baud rate from 9600, 19200, etc., and up to 115200 to see which one works.

### 4.4. Q: What size of SD card does the T-Deck support?
**A:** Users have had no issues using 16GB or 32GB SD cards. Format the SD card to **FAT32**.

### 4.5. Q: How do I get maps on T-Deck?
**A:** You need map tiles. You can get pre-downloaded map tiles here (a good way to support development):  
- <https://buymeacoffee.com/ripplebiz/e/342543> (Europe)  
- <https://buymeacoffee.com/ripplebiz/e/342542> (US)

Another way to download map tiles is to use this Python script to get the tiles in the areas you want:  
<https://github.com/fistulareffigy/MTD-Script>  

There is also a modified script that adds additional error handling and parallel downloads:  
<https://discord.com/channels/826570251612323860/1330643963501351004/1338775811548905572>  

UK map tiles are available separately from Andy Kirby on his discord server:  
<https://discord.com/channels/826570251612323860/1330643963501351004/1331346597367386224>

### 4.6. Q: Where do the map tiles go?
Once you have the tiles downloaded, copy the `\tiles` folder to the root of your T-Deck's SD card.

### 4.7. Q: How to unlock deeper map zoom and server management features on T-Deck?
**A:** You can download, install, and use the T-Deck firmware for free, but it has some features (map zoom, server administration) that are enabled if you purchase an unlock code for \$10 per T-Deck device.  
Unlock page: <https://buymeacoffee.com/ripplebiz/e/249834>

### 4.8. Q: How to decipher the diagnostics screen on T-Deck?

**A: ** Space is tight on T-Deck's screen, so the information is a bit cryptic.  The format is :
`{hops} l:{packet-length}({payload-len}) t:{packet-type} snr:{n} rssi:{n}`

See here for packet-type: [https://github.com/ripplebiz/MeshCore/blob/main/src/Packet.h#L19](https://github.com/ripplebiz/MeshCore/blob/main/src/Packet.h#L19 "https://github.com/ripplebiz/MeshCore/blob/main/src/Packet.h#L19")
 

    #define PAYLOAD_TYPE_REQ 0x00 // request (prefixed with dest/src hashes, MAC) (enc data: timestamp, blob) 
    #define PAYLOAD_TYPE_RESPONSE 0x01 // response to REQ or ANON_REQ (prefixed with dest/src hashes, MAC) (enc data: timestamp, blob) 
    #define PAYLOAD_TYPE_TXT_MSG 0x02 // a plain text message (prefixed with dest/src hashes, MAC) (enc data: timestamp, text) 
    #define PAYLOAD_TYPE_ACK 0x03 // a simple ack #define PAYLOAD_TYPE_ADVERT 0x04 // a node advertising its Identity 
    #define PAYLOAD_TYPE_GRP_TXT 0x05 // an (unverified) group text message (prefixed with channel hash, MAC) (enc data: timestamp, "name: msg") 
    #define PAYLOAD_TYPE_GRP_DATA 0x06 // an (unverified) group datagram (prefixed with channel hash, MAC) (enc data: timestamp, blob) 
    #define PAYLOAD_TYPE_ANON_REQ 0x07 // generic request (prefixed with dest_hash, ephemeral pub_key, MAC) (enc data: ...) 
    #define PAYLOAD_TYPE_PATH 0x08 // returned path (prefixed with dest/src hashes, MAC) (enc data: path, extra)

[Source](https://discord.com/channels/1343693475589263471/1343693475589263474/1350611321040932966)

### 4.9. Q: The T-Deck sound is too loud?
### 4.10. Q: Can you customize the sound?

**A:** You can customise the sounds on the T-Deck, just by placing `.mp3` files onto the `root` dir of the SD card. `startup.mp3`, `alert.mp3` and `new-advert.mp3`

### 4.11. Q: What is the 'Import from Clipboard' feature on the t-deck and is there a way to manually add nodes without having to receive adverts?

**A:** 'Import from Clipboard' is for importing a contact via a file named 'clipboard.txt' on the SD card. The opposite, is in the Identity screen, the 'Card to Clipboard' menu, which writes to 'clipboard.txt' so you can share yourself (call these 'biz cards', that start with "meshcore://...")

---

## 5. General

### 5.1. Q: What are BW, SF, and CR?

**A:** 

**BW is bandwidth** - width of frequency spectrum that is used for transmission

**SF is spreading factor** - how much should the communication spread in time

**CR is coding rate** - https://www.thethingsnetwork.org/docs/lorawan/fec-and-code-rate/
Making the bandwidth 2x wider (from BW125 to BW250) allows you to send 2x more bytes in the same time. Making the spreading factor 1 step lower (from SF10 to SF9) allows you to send 2x more bytes in the same time. 

Lowering the spreading factor makes it more difficult for the gateway to receive a transmission, as it will be more sensitive to noise. You could compare this to two people taking in a noisy place (a bar for example). If you’re far from each other, you have to talk slow (SF10), but if you’re close, you can talk faster (SF7)

So, it's balancing act between speed of the transmission and resistance to noise.
things network is mainly focused on LoRaWAN, but the LoRa low-level stuff still checks out for any LoRa project

### 5.2. Q: Do MeshCore clients repeat?
**A:** No, MeshCore clients do not repeat.  This is the core of MeshCore's messaging-first design.  This is to avoid devices flooding the air ware and create endless collisions, so messages sent aren't received.  
In MeshCore, only repeaters and room server with `set repeat on` repeat.  

### 5.3. Q: What happens when a node learns a route via a mobile repeater, and that repeater is gone?

**A:** If you used to reach a node through a repeater and the repeater is no longer reachable, the client will send the message using the existing (but now broken) known path, the message will fail after 3 retries, and the app will reset the path and send the message as flood on the last retry by default.  This can be turned off in settings.  If the destination is reachable directly or through another repeater, the new path will be used going forward.  Or you can set the path manually if you know a specific repeater to use to reach that destination.

In the case if users are moving around frequently, and the paths are breaking, they just see the phone client retries and revert to flood to attempt to reestablish a path. 

### 5.4. Q: How does a node discovery a path to its destination and then use it to send messages in the future, instead of flooding every message it sends like Meshtastic?

Routes are stored in sender's contact list.  When you send a message the first time, the message first gets to your destination by flood routing. When your destination node gets the message, it will send back a delivery report to the sender with all repeaters that the original message went through. This delivery report is flood-routed back to you the sender and is a basis for future direct path. When you send the next message, the path will get embedded into the packet and be evaluated by repeaters. If the hop and address of the repeater matches, it will retransmit the message, otherwise it will not retransmit, hence minimizing utilization.

[Source](https://discord.com/channels/826570251612323860/1330643963501351004/1351279141630119996)

### 5.5. Q: Do public channels always flood? Do private channels always flood?

**A:** Yes, group channels are A to B, so there is no defined path.  They have to flood.  Repeaters can however deny flood traffic up to some hop limit, with the `set flood.max` CLI command. Administrators of repeaters get to set the rules of their repeaters.

[Source](https://discord.com/channels/1343693475589263471/1343693475589263474/1350023009527664672)


### 5.6. Q: what is the public key for the default public channel?
**A:** The smartphone app key is in hex:
` 8b3387e9c5cdea6ac9e5edbaa115cd72`

T-Deck uses the same key but in base64 
`izOH6cXN6mrJ5e26oRXNcg==`
The third character is the capital letter 'O', not zero `0`
[Source](https://discord.com/channels/826570251612323860/1330643963501351004/1354194409213792388)

### 5.7. Q: Is MeshCore open source?
**A:** Most of the firmware is freely available. Everything is open source except the T-Deck firmware and Liam's native mobile apps.  
- Firmware repo: <https://github.com/ripplebiz/MeshCore>  

### 5.8. Q: How can I support MeshCore?
**A:** Provide your honest feedback on GitHub and on AndyKirby's Discord server <http://discord.com/invite/H62Re4DCeD>. Spread the word of MeshCore to your friends and communities; help them get started with MeshCore. Support Scott's MeshCore development at <https://buymeacoffee.com/ripplebiz>.

Support Liam Cottle's smartphone client development by unlocking the server administration wait gate with in-app purchase

Support Rastislav Vysoky (recrof)'s flasher web site and the map web site development through [PayPal](https://www.paypal.com/donate/?business=DREHF5HM265ES&no_recurring=0&item_name=If+you+enjoy+my+work%2C+you+can+support+me+here%3A&currency_code=EUR) or [Revolut](https://revolut.me/recrof)

### 5.9. Q: How do I build MeshCore firmware from source?
**A:** See instructions here:  
https://discord.com/channels/826570251612323860/1330643963501351004/1341826372120608769

Build instructions for MeshCore:

For Windows, first install WSL and Python+pip via: https://plainenglish.io/blog/setting-up-python-on-windows-subsystem-for-linux-wsl-26510f1b2d80

(Linux, Windows+WSL) In the terminal/shell:
```
sudo apt update
sudo apt install libpython3-dev
sudo apt install python3-venv
```
Mac: python3 should be already installed.

Then it should be the same for all platforms:
```
python3 -m venv meshcore
cd meshcore && source bin/activate
pip install -U platformio
git clone https://github.com/ripplebiz/MeshCore.git 
cd MeshCore
```
open platformio.ini and in `[arduino_base]` edit the `LORA_FREQ=867.5`
save, then run:
```
pio run -e RAK_4631_Repeater
```
then you'll find `firmware.zip` in `.pio/build/RAK_4631_Repeater`

Andy also has a video on how to build using VS Code:  
*How to build and flash Meshcore repeater firmware | Heltec V3*  
<https://www.youtube.com/watch?v=WJvg6dt13hk> *(Link referenced in the Discord post)*

### 5.10. Q: Are there other MeshCore related open source projects?

**A:** [Liam Cottle](https://liamcottle.net)'s MeshCore web client and MeshCore Javascript libary are open source under MIT license.

Web client: https://github.com/liamcottle/meshcore-web
Javascript: https://github.com/liamcottle/meshcore.js

### 5.11. Q: Does MeshCore support ATAK
**A:** ATAK is not currently on MeshCore's roadmap.

Meshcore would not be best suited to ATAK  because MeshCore:
clients do not repeat and therefore you would need a network of repeaters in place
will not have a stable path where all clients are constantly moving between repeaters

MeshCore clients would need to reset path constantly and flood traffic across the network which could lead to lots of collisions with something as chatty as ATAK. 

This could change in the future if MeshCore develops a client firmware that repeats.
[Source](https://discord.com/channels/826570251612323860/1330643963501351004/1354780032140054659)

### 5.12. Q: How do I add a node to the [MeshCore Map]([url](https://meshcore.co.uk/map.html))
**A:** From the smartphone app, connect to a BLE Companion radio
- To add the BLE Companion radio your smartphone is connected to to the map, tap the `advert` icon, then tap `Advert (To Clipboard)`.  
- To add a Repeater or Room Server to the map, tap the 3 dots next to the Repeater or Room Server you want to add to the map, then tap `Share (To Clipboard)`.  
- Go to the [MeshCore Map web site]([url](https://meshcore.co.uk/map.html)), tap the plus sign on the lower right corner and paste in the meshcore://... blob, then tap `Add Node`

### 5.13. Q: Can I use a Raspberry Pi to update a MeshCore radio?
** A:** Yes.
You will need to install picocom on the pi.
`sudo apt install picocom`

Then run the following commands to setup the repeater.
```
picocom -b 115200 /dev/ttyUSB0 --imap lfcrlf
set name your_repeater_name
time epoch_time
password your_unique_password
set advert.interval 240
advert
```
Note: If using a RAK the path will most likely be /dev/ttyACM0

Epoch time comes from https://www.epochconverter.com/

You can also flash the repeater using esptool.  You will need to install esptool with the following command...

`pip install esptool --break-system-packages`

Then to flash the firmware to Heltec, obtain the .bin file from https://flasher.meshcore.co.uk/ (download all firmware link)

For Heltec:
`esptool.py -p /dev/ttyUSB0 --chip esp32-s3 write_flash 0x00000 firmware.bin`

If flashing a visual studio code build bin file, flash with the following offset:
`esptool.py -p /dev/ttyUSB0 --chip esp32-s3 write_flash 0x10000 firmware.bin`

For Pi
Download the zip from the online flasher website and use the following command:

Note: Requires adafruit-nrfutil command which can be installed as follows.
`pip install adafruit-nrfutil --break-system-packages`

```
adafruit-nrfutil --verbose dfu serial --package t1000_e_bootloader-0.9.1-5-g488711a_s140_7.3.0.zip -p /dev/ttyACM0 -b 115200 --singlebank --touch 1200
```

[Source](https://discord.com/channels/826570251612323860/1330643963501351004/1342120825251299388)
  
### 5.14. Q: Are there are projects built around MeshCore?

**A:** Yes.  See the following:

#### 5.14.1. meshcoremqtt
A Python script to send meshore debug and packet capture data to MQTT for analysis
https://github.com/Andrew-a-g/meshcoretomqtt

#### 5.14.2. MeshCore for Home Assistant
A custom Home Assistant integration for MeshCore mesh radio nodes. It allows you to monitor and control MeshCore nodes via USB, BLE, or TCP connections.
https://github.com/awolden/meshcore-ha

#### 5.14.3. Python MeshCore
Bindings to access your MeshCore companion radio nodes in python.
https://github.com/fdlamotte/meshcore_py

#### 5.14.4. meshcore-cli  
CLI interface to MeshCore companion radio over BLE, TCP, or serial.  Uses Pyton MeshCore above.
 https://github.com/fdlamotte/meshcore-cli

#### 5.14.5. meshcore.js
A JavaScript library for interacting with a MeshCore device running the companion radio firmware
https://github.com/liamcottle/meshcore.js

---

## 6. Troubleshooting

### 6.1. Q: My client says another client or a repeater or a room server was last seen many, many days ago.
### 6.2. Q: A repeater or a client or a room server I expect to see on my discover list (on T-Deck) or contact list (on a smart device client) are not listed.
**A:**  
- If your client is a T-Deck, it may not have its time set (no GPS installed, no GPS lock, or wrong GPS baud rate).  
- If you are using the Android or iOS client, the other client, repeater, or room server may have the wrong time.  

You can get the epoch time on <https://www.epochconverter.com/> and use it to set your T-Deck clock. For a repeater and room server, the admin can use a T-Deck to remotely set their clock (clock sync), or use the `time` command in the USB serial console with the server device connected.

### 6.3. Q: How to connect to a repeater via BLE (Bluetooth)?
**A:** You can't connect to a device running repeater firmware  via Bluetooth.  Devices running the BLE companion firmware you can connect to it via Bluetooth using the android app

### 6.4. Q: I can't connect via Bluetooth, what is the Bluetooth pairing code?

**A:** the default Bluetooth pairing code is `123456`

### 6.5. Q: My Heltec V3 keeps disconnecting from my smartphone.  It can't hold a solid Bluetooth connection.

**A:** Heltec V3 has a very small coil antenna on its PCB for Wi-Fi and Bluetooth connectivity.  It has a very short range, only a few feet.  It is possible to remove the coil antenna  and replace it with a 31mm wire.  The BT range is much improved with the modification.

---
## 7. Other Questions:
### 7.1. Q: How to  Update repeater and room server firmware over the air?

**A:** Only nRF-based RAK4631 and Heltec T114 OTA firmware update are verified using nRF smartphone app.  Lilygo T-Echo doesn't work currently.
You can update repeater and room server firmware with a Bluetooth connection between your smartphone and your LoRa radio using the nRF app.

1. Download the ZIP file for the specific node from the web flasher to your smartphone
2. On the phone client, log on to the repeater as administrator (default password is `password`) to issue the `start ota`command to the repeater or room server to get the device into OTA DFU mode
   
![image](https://github.com/user-attachments/assets/889bb81b-7214-4a1c-955a-396b5a05d8ad)

1. `start ota` can be initiated from USB serial console on the web flasher page or a T-Deck
4. On the smartphone, download and run the nRF app and scan for Bluetooth devices
5. Connect to the repeater/room server node you want to update
	1. nRF app is available on both Android and iOS

**Android continues after the iOS section:**

**iOS continues here:**
5. Once connected successfully, a `DFU` icon ![Pasted image 20250309173039](https://github.com/user-attachments/assets/af7a9f78-8739-4946-b734-02bade9c8e71)
 appears in the top right corner of the app
 ![Pasted image 20250309171919](https://github.com/user-attachments/assets/08007ec8-4924-49c1-989f-ca2611e78793)

6. Scroll down to change the `PRN(s)` number:

![Pasted image 20250309190158](https://github.com/user-attachments/assets/11f69cdd-12f3-4696-a6fc-14a78c85fe32)

- For the T114, change the number of packets `(PRN(s)` to 8
- For RAK, it can be 10, but it also works on 8.

7. Click the `DFU` icon ![Pasted image 20250309173039](https://github.com/user-attachments/assets/af7a9f78-8739-4946-b734-02bade9c8e71), select the type of file to upload (choose ZIP), then select the ZIP file that was downloaded earlier  from the web flasher
8. The upload process will start now. If everything goes well, the node resets and is flashed successfully.
![Pasted image 20250309190342](https://github.com/user-attachments/assets/a60e25d0-33b8-46cf-af90-20a7d8ac2adb)



**Android steps continues below:**
1. on the top left corner of the nRF Connect app on Android, tap the 3-bar hamburger menu, then `Settings`, then `nRF5 DFU Options`

![Android nRF Hamberger](https://github.com/user-attachments/assets/ea6dfeef-9367-4830-bd70-1441d517c706)

![Android nRF Settings](https://github.com/user-attachments/assets/c63726bf-cecd-4987-be68-afb6358c7190)

![Android nRF DFU Options](https://github.com/user-attachments/assets/b20e872f-5122-41d9-90df-0215cff5fbc9)

2. Change `Number of packets` to `10` for RAK, `8` for Heltec T114

![Android nRF Number of Packets](https://github.com/user-attachments/assets/c092adaf-4cb3-460b-b7ef-8d7f450d602b)

3. Go back to the main screen
4. Your LoRa device should already ben in DFU mode from previous steps
5. tap `SCANNER` and then `SCAN` to find the device you want to update, tap `CONNECT`

![Android nRF Scanner Scan Connect](https://github.com/user-attachments/assets/37218717-f167-48b6-a6ca-93d132ef77ca)

6. On the top left corner of the nRF Connect app, tap the `DFU` icon next to the three dots

![Android nRF DFU](https://github.com/user-attachments/assets/1ec3b818-bf0c-461f-8fdf-37c41a63cafa)

7. Choose `Distribution packet (ZIP)` and then `OK`

![Android nRF Distribution Packet (ZIP)](https://github.com/user-attachments/assets/e65f5616-9793-44f5-95c0-a3eb15aa7152)

8. Choose the firmware file in ZIP formate that you downloaded earlier from the MeshCore web flasher, update will start as soon as you tap the file

![Android nRF FW Updating](https://github.com/user-attachments/assets/0814d123-85ce-4c87-90a7-e1a25dc71900)

9. When the update process is done, the device will disconnect from nRF app and the LoRa device is updated


---
    
