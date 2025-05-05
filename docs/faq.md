# MeshCore-FAQ
A list of frequently-asked questions and answers for MeshCore

The current version of this MeshCore FAQ is at https://github.com/ripplebiz/MeshCore/blob/main/docs/faq.md.  
This MeshCore FAQ is also mirrored at https://github.com/LitBomb/MeshCore-FAQ and might have newer updates if pull requests on Scott's MeshCore repo are not approved yet.

author: https://github.com/LitBomb
---

## Q: What is MeshCore?

**A:** MeshCore is free and open source
* MeshCore is the routing and firmware etc, available on GitHub under MIT license
* There are clients made by the community, such as the web clients, these are free to use, and some are open source too
* The cross platform mobile app developed by [Liam Cottle](https://liamcottle.net) for Android/iOS/PC etc is free to download and use
* The T-Deck firmware is developed by Scott at Ripple Radios, the creator of MeshCore, is also free to flash on your devices and use


Some more advanced, but optional features are available on T-Deck if you register your device for a key to unlock.  On the MeshCore smartphone clients for Android and iOS/iPadOS, you can unlock the wait timer for repeater and room server remote management over RF feature. 

These features are completely optional and aren't needed for the core messaging experience. They're like super bonus features and to help the developers continue to work on these amazing features, they may charge a small fee for an unlock code to utilise the advanced features.

Anyone is able to build anything they like on top of MeshCore without paying anything.

## Q: What do you need to start using MeshCore?
**A:** Everything you need for MeshCore is available at:
 Main web site: [https://meshcore.co.uk/](https://meshcore.co.uk/)
 Firmware Flasher: https://flasher.meshcore.co.uk/
 Phone Client Applications: https://meshcore.co.uk/apps.html
 MeshCore Fimrware Github: https://github.com/ripplebiz/MeshCore

 NOTE: Andy Kirby has a very useful [intro video](https://www.youtube.com/watch?v=t1qne8uJBAc) for beginners.
 

 You need LoRa hardware devices to run MeshCore firmware as clients or server (repeater and room server).

### Hardware
To use MeshCore without using a phone as the client interface, you can run MeshCore on a T-Deck or T-Deck Plus. It is a complete off-grid secure communication solution.  

MeshCore is also available on a variety of 868MHz and 915MHz LoRa devices. For example, RAK4631 devices (19003, 19007, 19026), Heltec V3, Xiao S3 WIO, Xiao C3, Heltec T114, Station G2, Seeed Studio T1000-E. More devices will be supported later.

### Firmware
MeshCore has four firmware types that are not available on other LoRa systems. MeshCore has the following:

#### Companion Radio Firmware
Companion radios are for connecting to the Android app or web app as a messenger client. There are two different companion radio firmware versions:

1. **BLE Companion**  
   BLE Companion firmware runs on a supported LoRa device and connects to a smart device running the Android MeshCore client over BLE (iOS MeshCore client will be available soon)  
   <https://meshcore.co.uk/apps.html>

2. **USB Serial Companion**  
   USB Serial Companion firmware runs on a supported LoRa device and connects to a smart device or a computer over USB Serial running the MeshCore web client  
   <https://meshcore.liamcottle.net/#/>  
   <https://client.meshcore.co.uk/tabs/devices>

#### Repeater
Repeaters are used to extend the range of a MeshCore network. Repeater firmware runs on the same devices that run client firmware. A repeater's job is to forward MeshCore packets to the destination device. It does **not** forward or retransmit every packet it receives, unlike other LoRa mesh systems.  

A repeater can be remotely administered using a T-Deck running the MeshCore firwmware with remote admistration features unlocked, or from a BLE Companion client connected to a smartphone running the MeshCore app.

#### Room Server
A room server is a simple BBS server for sharing posts. T-Deck devices running MeshCore firmware or a BLE Companion client connected to a smartphone running the MeshCore app can connect to a room server. 

room servers store message history on them, and push the stored messages to users.  Room servers allow roaming users to come back later and retrieve message history.  Contrast to channels, messages are either received  when it's sent, or not received and missed if the a room user is out of range.  You can think  of room servers like email servers where you can come back later and get your emails from your mail server 

A room server can be remotely administered using a T-Deck running the MeshCore firwmware with remote admistration features unlocked, or from a BLE Companion client connected to a smartphone running the MeshCore app.  

When a client logs into a room server, the client will receive the previously 16 unseen messages.

A room server can also take on the repeater role.  To enable repeater role on a room server, use this command:

`set repeat {on|off}`

---

## Initial Setup

### Q: How many devices do I need to start using meshcore?
**A:** If you have one supported device, flash the BLE Companion firmware and use your device as a client.  You can connect to the device using the Android client via bluetooth (iOS client will be available later).  You can start communiating with other MeshCore users near you.

If you have two supported devices, and there are not many MeshCore users near you, flash both of them to BLE Companion firmware so you can use your devices to communiate with your near-by friends and family.

If you have two supported devices, and there are other MeshcCore users near by, you can flash one of your devices with BLE Companion firmware, and flash another supported device to repeater firmware.  Place the repeater high above ground o extend your MeshCore network's reach.

After you flashed the latest firmware onto your repeater device, keep the device connected to your computer via USB serial, use the console feature on the web flasher and set the frequency for your region or country, so your client can remote administer the rpeater or room server over RF:

`set freq {frequency}`

The repeater and room server CLI reference is here: https://github.com/ripplebiz/MeshCore/wiki/Repeater-&-Room-Server-CLI-Reference

If you have more supported devices, you can use your additional deivces with the room server firmware.  

### Q: Does MeshCore cost any money?

**A:** All radio firmware versions (e.g. for Heltec V3, RAK, T-1000E, etc) are free and open source developed by Scott at Ripple Radios.  

The native Android and iOS client uses the freemium model and is developed by Liam Cottle, developer of meshtastic map at [meshtastic.liamcottle.net](https://meshtastic.liamcottle.net) on [github ](https://github.com/liamcottle/meshtastic-map)and [reticulum-meshchat on github](https://github.com/liamcottle/reticulum-meshchat). 

The T-Deck firmware is free to download and most features are available without cost.  To support the firmware developer, you can pay for a registration key to unlock your T-Deck for deeper map zoom and remote server administration over RF using the T-Deck.  You do not need to pay for the registration to use your T-Deck for direct messaging and connecting to repeaters and room servers. 


### Q: What frequencies are supported by MeshCore?
**A:** It supports the 868MHz range in the UK/EU and the 915MHz range in New Zealand, Australia, and the USA. Countries and regions in these two frequency ranges are also supported. The firmware and client allow users to set their preferred frequency.  
- Australia and New Zealand are on **915.8MHz**
- UK and EU are on **869.525MHz**
- Canada and USA are on **910.525MHz**
- For other regions and countries, please check your local LoRa frequency

the rest of the radio settings are the same for all frequencies:  
- Spread Factor (SF): 10  
- Coding Rate (CR): 5  
- Bandwidth (BW): 250.00  

### Q: What is an "advert" in MeshCore?
**A:** 
Advert means to advertise yourself on the network. In Reticulum terms it would be to announce. In Meshtastic terms it would be the node sending it's node info.

MeshCore allows you to manually broadcast your name, position and public encryption key, which is also signed to prevent spoofing.  When you click the advert button, it broadcasts that data over LoRa.  MeshCore calls that an Advert. There's two ways to advert, "zero hop" and "flood".

* Zero hop means your advert is broadcasted out to anyone that can hear it, and that's it.
* Flooded means it's broadcasted out, and then repeated by all the repeaters that hear it.

MeshCore clients only advertise themselves when the user initiates it. A repeater (and room server?) advertises its presence once every 240 minutes. This interval can be configured using the following command:

`set advert.interval {minutes}`

### Q: Is there a hop limit?

**A:** Internally the firmware has maximum limit of 64 hops.  In real world settings it will be difficult to get close to the limit due to the environments and timing as packets travel further and further.  We want to hear how far your MeshCore conversations go. 


---


## Server Administration

### Q: How do you configure a repeater or a room server?
**A:** One of these servers can be administered with one of the options below:
- Connect the server device using a USB cable to a computer running Chrome on https://flasher.meshcore.co.uk/, then use the `console` feature to connect to the device
	- this is necessary to set the server device's frequency if it doesn't match the frequency for your local region or country
- MeshCore smart device clients have the ability to remotely administer servers.
- A T-Deck running unlocked/registered MeshCore firmware. Remote server administration is enabled through registering your T-Deck with Ripple Radios. It is one of the ways to support MeshCore development. You can register your T-Deck at:  
  <https://buymeacoffee.com/ripplebiz/e/249834>

### Q: Do I need to set the location for a repeater?
**A:** With location set for a repeater, it can show up on a MeshCore map in the future. Set location with the following commands:

`set lat <GPS Lat> set long <GPS Lon>`

You can get the latitude and longitude from Google Maps by right-clicking the location you are at on the map.

### Q: What is the password to administer a repeater or a room server?
**A:** The default admin password to a repeater and room server is `password`. Use the following command to change the admin password:

`password {new-password}`


### Q: What is the password to join a room server?
**A:** The default guest password to a room server is `hello`. Use the following command to change the guest password:

`set guest.password {guest-password}`


---

## T-Deck Related

### Q: What are the steps to get a T-Deck into DFU (Device Firmware Update) mode?
**A:**  
1. Device off  
2. Connect USB cable to device  
3. Hold down trackball (keep holding)  
4. Turn on device  
5. Hear USB connection sound  
6. Release trackball  
7. T-Deck in DFU mode now  
8. At this point you can begin flashing using <https://flasher.meshcore.co.uk/>

### Q: Why is my T-Deck Plus not getting any satellite lock?
**A:** For T-Deck Plus, the GPS baud rate should be set to **38400**. Also, a number of T-Deck Plus devices were found to have the GPS module installed upside down, with the GPS antenna facing down instead of up. If your T-Deck Plus still doesn't get any satellite lock after setting the baud rate to 38400, you might need to open up the device to check the GPS orientation.

### Q: Why is my OG (non-Plus) T-Deck not getting any satellite lock?
**A:** The OG (non-Plus) T-Deck doesn't come with a GPS. If you added a GPS to your OG T-Deck, please refer to the manual of your GPS to see what baud rate it requires. Alternatively, you can try to set the baud rate from 9600, 19200, etc., and up to 115200 to see which one works.

### Q: What size of SD card does the T-Deck support?
**A:** Users have had no issues using 16GB or 32GB SD cards. Format the SD card to **FAT32**.

### Q: How do I get maps on T-Deck?
**A:** You need map tiles. You can get pre-downloaded map tiles here (a good way to support development):  
- <https://buymeacoffee.com/ripplebiz/e/342543> (Europe)  
- <https://buymeacoffee.com/ripplebiz/e/342542> (US)

Another way to download map tiles is to use this Python script to get the tiles in the areas you want:  
<https://github.com/fistulareffigy/MTD-Script>  

There is also a modified script that adds additional error handling and parallel downloads:  
<https://discord.com/channels/826570251612323860/1330643963501351004/1338775811548905572>  

UK map tiles are available separately from Andy Kirby on his discord server:  
<https://discord.com/channels/826570251612323860/1330643963501351004/1331346597367386224>

### Q: Where do the map tiles go?
Once you have the tiles downloaded, copy the `\tiles` folder to the root of your T-Deck's SD card.

### Q: How to unlock deeper map zoom and server management features on T-Deck?
**A:** You can download, install, and use the T-Deck firmware for free, but it has some features (map zoom, server administration) that are enabled if you purchase an unlock code for \$10 per T-Deck device.  
Unlock page: <https://buymeacoffee.com/ripplebiz/e/249834>


### Q: The T-Deck sound is too loud?
### Q: Can you customize the sound?

**A:** You can customise the sounds on the T-Deck, just by placing `.mp3` files onto the `root` dir of the SD card. `startup.mp3`, `alert.mp3` and `new-advert.mp3`

### Q: What is the 'Import from Clipboard' feature on the t-deck and is there a way to manually add nodes without having to receive adverts?

**A:** 'Import from Clipboard' is for importing a contact via a file named 'clipboard.txt' on the SD card. The opposite, is in the Identity screen, the 'Card to Clipboard' menu, which writes to 'clipboard.txt' so you can share yourself (call these 'biz cards', that start with "meshcore://...")

---

## General

### Q: What are BW, SF, and CR?

**A:** 

**BW is bandwidth** - width of frequency spectrum that is used for transmission

**SF is spreading factor** - how much should the communication spread in time

**CR is coding rate** - https://www.thethingsnetwork.org/docs/lorawan/fec-and-code-rate/
Making the bandwidth 2x wider (from BW125 to BW250) allows you to send 2x more bytes in the same time. Making the spreading factor 1 step lower (from SF10 to SF9) allows you to send 2x more bytes in the same time. 

Lowering the spreading factor makes it more difficult for the gateway to receive a transmission, as it will be more sensitive to noise. You could compare this to two people taking in a noisy place (a bar for example). If you’re far from each other, you have to talk slow (SF10), but if you’re close, you can talk faster (SF7)

So it's balancing act between speed of the transmission and resistance to noise.
things network is mainly focused on LoRaWAN, but the LoRa low-level stuff still checks out for any LoRa project

### Q: What happens when a node learns a route via a mobile repeater, and that repeater is gone?

**A:** If you used to reach a node through a repeater and the repeater is no longer reachable, the client will send the message using the existing (but now broken) known path, the message will fail after 3 retries, and the app will reset the path and send the message as flood on the last retry by default.  This can be turned off in settings.  If the destination is reachable directly or through another repeater, the new path will be used going forward.  Or you can set the path manually if you know a specific repeater to use to reach that destination.

In the case if users are moving around frequently, and the paths are breaking, they just see the phone client retries and revert to flood to attempt to reestablish a path. 


### Q: Is MeshCore open source?
**A:** Most of the firmware is freely available. Everything is open source except the T-Deck firmware and Liam's native mobile apps.  
- Firmware repo: <https://github.com/ripplebiz/MeshCore>  

### Q: How can I support MeshCore?
**A:** Provide your honest feedback on GitHub and on AndyKirby's Discord server <http://discord.com/invite/H62Re4DCeD>. Spread the word of MeshCore to your friends and communities; help them get started with MeshCore. Support Scott's MeshCore development at <https://buymeacoffee.com/ripplebiz>.

Support Liam Cottle's smartphone client development by unlocking the server administration wait gate with in-app purchase

Support Rastislav Vysoky (recrof)'s flasher web site and the map web site development through [PayPal](https://www.paypal.com/donate/?business=DREHF5HM265ES&no_recurring=0&item_name=If+you+enjoy+my+work%2C+you+can+support+me+here%3A&currency_code=EUR) or [Revolut](https://revolut.me/recrof)

### Q: How do I build MeshCore firmware from source?
**A:** See instructions here:  
<https://discord.com/channels/826570251612323860/1330643963501351004/1342120825251299388>  

Andy also has a video on how to build using VS Code:  
*How to build and flash Meshcore repeater firmware | Heltec V3*  
<https://www.youtube.com/watch?v=WJvg6dt13hk> *(Link referenced in the Discord post)*

### Q: Are there other MeshCore related open source projects?

**A:** [Liam Cottle](https://liamcottle.net)'s MeshCore web client and MeshCore Javascript libary are open source under MIT license.

Web client: https://github.com/liamcottle/meshcore-web
Javascript: https://github.com/liamcottle/meshcore.js

### Q: Does MeshCore support ATAK
**A:** ATAK is not currently on MeshCore's roadmap.

### Q: How do I add a node to the [MeshCore Map]([url](https://meshcore.co.uk/map.html))
**A:** From the smartphone app, connect to a BLE Companion radio
- To add the BLE Companion radio your smartphone is connected to to the map, tap the `advert` icon, then tap `Advert (To Clipboard)`.  
- To add a Repeater or Room Server to the map, tap the 3 dots next to the Repeater or Room Server you want to add to the map, then tap `Share (To Clipboard)`.  
- Go to the [MeshCore Map web site]([url](https://meshcore.co.uk/map.html)), tap the plus sign on the lower right corner and paste in the meshcore://... blob, then tap `Add Node`
    
---

## Troubleshooting

### Q: My client says another client or a repeater or a room server was last seen many, many days ago.
### Q: A repeater or a client or a room server I expect to see on my discover list (on T-Deck) or contact list (on a smart device client) are not listed.
**A:**  
- If your client is a T-Deck, it may not have its time set (no GPS installed, no GPS lock, or wrong GPS baud rate).  
- If you are using the Android or iOS client, the other client, repeater, or room server may have the wrong time.  

You can get the epoch time on <https://www.epochconverter.com/> and use it to set your T-Deck clock. For a repeater and room server, the admin can use a T-Deck to remotely set their clock (clock sync), or use the `time` command in the USB serial console with the server device connected.

### Q: How to connect to a repeater via BLE (bluetooth)?
**A:** You can't connect to a device running repeater firmware  via bluetooth.  Devices running the BLE companion firmware you can connect to it via bluetooth using the android app

### Q: I can't connect via bluetooth, what is the bluetooth pairing code?

**A:** the default bluetooth pairing code is `123456`

### Q: My Heltec V3 keeps disconnecting from my smartphone.  It can't hold a solid Bluetooth connection.

**A:** Heltec V3 has a very small coil antenna on its PCB for WiFi and Bluetooth connectivty.  It has a very short range, only a few feet.  It is possible to remove the coil antenna  and replace it with a 31mm wire.  The BT range is much improved with the modification.

---
## Other Questions:
### Q: How to  Update repeater and room server firmware over the air?

**A:** Only nRF-based RAK4631 and Heltec T114 OTA firmware update are verified using nRF smartphone app.  Lilygo T-Echo doesn't work currently.
You can update repeater and room server firmware with a bluetooth connection between your smartphone and your LoRa radio using the nRF app.

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
 appears in the top right corner of the app![Pasted image 20250309171919](https://github.com/user-attachments/assets/08007ec8-4924-49c1-989f-ca2611e78793)

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
    