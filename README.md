# ESP Home POE Daikin Alterma

## Jan 2026 \- Dennis
# 

[**What it does;	1**](#what-it-does;)

[**Features;	2**](#features;)

[**Disclaimer;	2**](#disclaimer;)

[**Requirements;	2**](#requirements;)

[**How to make it yourself;	2**](#hardware-installation;)

[**How to update	6**](#how-to-update)

[**Background;	6**](#background;)

[**Still to do;	7**](#still-to-do;)

# 

# 

# What it does; {#what-it-does;}

* read many parameters of your Daikin Alterma heatpump from Home Assistant  
* set the operation mode of your heat pump \[off\]/\[heat\]/\[cool\] from Home Assistant  
* set the smartgrid feature from Home Assistant

![][image7]

# Features; {#features;}

* Build on ESP Home, easier to build, easier to update  
* Build for a M5 stack POE (power over ethernet) device, because ‘a cable is stable’  
* Build for the m5 stack 4 channel relay, because standardized and state restore

# Disclaimer; {#disclaimer;}

This project is provided as-is and is intended for educational and experimental purposes only.  
Use of this software and any related information is entirely at your own risk. The authors and contributors make no guarantees regarding correctness, safety, reliability, or suitability for any particular purpose. Improper use may result in malfunction, damage to equipment, data loss, or other unintended consequences. No support, maintenance, or updates are provided.By using this project, you agree that the authors and contributors are not liable for any damages, losses, or issues arising from its use. This project was built for the Daikin Altherma 3 R F.

# Requirements; {#requirements;}

* M5 stack ESP32 Ethernet Unit with PoE / SKU: U138  
* M5 stack 4-Relay Unit / SKU: U097  
* M5 stack ESP32 Downloader Kit / SKU: A105 (yes you really need it)  
* Home Assistant  
* A POE switch with cable  
* A compatible heat pump  
* Internet connection and a laptop/computer  
  


# Hardware installation; {#hardware-installation;}

Follow these steps to get everything working;

1. Buy the hardware, see the requirements  
2. Connecting the wires.  
   Pictures and connection scheme are for the models; Daikin Altherma 3 R F ERGA04EV3 ERGA06EV3H ERGA08EV3H ERGA04EV3A ERGA06EV3A ERGA08EV3A EHVH04S18E6V EHVH04S23E6V EHVH08S18E6V EHVH08S23E6V EHVH08S18E9W EHVH08S23E9W EHVX04S18E3V EHVX04S18E6V EHVX04S23E3V EHVX04S23E6V EHVX08S18E6V EHVX08S23E6V EHVX08S18E9W EHVX08S23E9W  
   Double check the manual for your heat pump if this information also applies for your situation. These steps also require opening the heat pump, see the manual of the heat pump on how to do this in a safe way. Ignoring the safety information could lead to damage or risking an electrical shock. 

     
3. For the serial connection;

| Heat Pump X10A poort ![][image2] Notice the little encircled one, indicating PIN 1\. Do not connect PIN 1, we use POE for power. | M5 stack POE ESP32![][image3]  |
| ----- | ----- |
| PIN 2 \=  TX | Onboard RX |
| PIN 3 \= RX | Onboard TX |
| PIN 5 \= GND | Onboard GND |

     
   

# For thermostat and smart-grid;

We use the relay’s NO \= Normal Open port. If the relay is without power, the heat pump won’t go into an energy hungry mode. That's the red centre connection on the m5 4 channel relay.  

| Heat Pump X5M poort![][image4] | M5 stack relay![][image5] |
| :---- | :---- |
| X5M poort 5 smart grid 1 | 1 NO |
| X5M poort 6 smart grid 1 | 1 NO |
| X5M poort 9 smart grid 2 | 2 NO |
| X5M poort 10 smart grid 2 | 2 NO |
| X5M poort 7 thermostat | 3 NO |
| X5M poort 8 thermostat | 3 NO |

Mode selection;  
This only works if your heat pump can also cool.

| X2M block ![][image6] | M5 stack relay![][image5] |
| :---- | :---- |
| X2M poort 7 | 4 NO |
| X2M poort 9 | 4 NO |

# Software installation

1. Set up your ESP32 POE unit to have a static IP. preferably on your DHCP server.   
2. Get ESP Home on the POE device and into Home Assistant using the ESP32 downloader kit. I’m not gonna explain that here, plenty of resources on the net on how to do that.   
3. Paste the contents of the YML file, except your API key  
4. Press install in ESP Home from Home Assistant

# How to update {#how-to-update}

1. In Home Assistant, ESP Home, Clean build files  
2. Install

# Background; {#background;}

A few years ago when [https://github.com/raomin/ESPAltherma](https://github.com/raomin/ESPAltherma) just came out, I modified it to work with the 4 channel relay from the M5 stack. After running stable for a few years, I’ve forked ESPAltherma again because it had been updated, and I had to redo my alterations for the 4 channel relay. After recompiling, I got a lot of Wifi problems. Since my house is cold without this software, I was in need of a more reliable solution. And I’m a big fan of POE.

So based on the original code of Raomin, I’ve created this ESP Home version,this project would not be possible without the O.G.; [https://github.com/raomin/ESPAltherma](https://github.com/raomin/ESPAltherma)

# Still to do; {#still-to-do;}

Support for other devices/other languages/support for the Daikin S protocol.

[image1]: images/image1.png

[image2]: images/image2.png

[image3]: images/image3.png

[image4]: images/image4.png

[image5]: images/image5.png

[image6]: images/image6.png
