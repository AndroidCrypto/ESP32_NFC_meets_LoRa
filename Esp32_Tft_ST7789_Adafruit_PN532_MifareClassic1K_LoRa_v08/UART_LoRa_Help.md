To setup your LoRa devices run:

+++

AT+HELP
gives an overview about current setting

===
LoRa Parameter: 
+VERSION=V 2.3.1
MODE:0
LEVEL:0 >> 244.140625bps
SLEEP:2
Frequency:868000000hz >> 24
MAC:ff,ff
Bandwidth:0
Spreading Factor:12
Coding rate:2
CRC:0(false)
Preamble:8
IQ:0(false)
Power:22dBm
=

AT+CHANNEL1B
set the frequency to 863.500 MHz

AT+SF7
set the spreading factor to 7

AT+CR1
set the Coding Rate to 4/5

AT+POWE0
set the transmission power to lowest value = 0 dBm

Last control:
AT+HELP
===
LoRa Parameter:
+VERSION=V 2.3.1
MODE:0
LEVEL:0 >> 5468.750000bps
SLEEP:2
Frequency:863500000hz >> 1b
MAC:ff,ff
Bandwidth:0
Spreading Factor:7
Coding rate:1
CRC:0(false)
Preamble:8
IQ:0(false)
Power:0dBm
==

