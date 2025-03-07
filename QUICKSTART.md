# Quick Starts for common platforms

## Linux CLI-Only QuickStart

### Starting from the home dir, Clone this repository and `cd` into it
```
cd ~
git clone git@github.com:ripplebiz/MeshCore.git
cd MeshCore
```

### Install deps:
```
platformio pkg install
```

### If installing from cli, ensure arduino embed package is installed:
```
platformio pkg install -t platformio/framework-arduino-mbed
```

### Need to patch some libs as per https://learn.rakwireless.com/hc/en-us/articles/26687276346775-How-To-Perform-Installation-of-Board-Support-Package-in-PlatformIO
```
cd ../.platformio
wget https://raw.githubusercontent.com/RAKWireless/WisBlock/master/PlatformIO/RAK_PATCH.zip
unar RAK_PATCH.zip
cd RAK_PATCH
python3 ./rak_patch.py
```

We should see something like:
```
~/.platformio/RAK_PATCH via 🐍 v3.12.8 
❯ python3 ./rak_patch.py 
Patching RAK4631
Patched RAK4631
Patching RAK11200
Patched RAK11200
Patching RAK11310
Patched RAK11310
Patch done, check output if successful
```

### Flash it to a T-Deck
```
platformio run -e ????
```
