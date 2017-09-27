- PJON (Padded Jittering Operative Network) Protocol specification:
[v1.1](/specification/PJON-protocol-specification-v1.1.md)
- Acknowledge specification: [v0.1](/specification/PJON-protocol-acknowledge-specification-v0.1.md)
- Dynamic addressing specification: [v0.1](/specification/PJON-dynamic-addressing-specification-v0.1.md)
- PJDL (Padded Jittering Data Link) specification:
[PJDL v2.0](/strategies/SoftwareBitBang/specification/PJDL-specification-v2.0.md) - **[PJDLR v2.0](/strategies/OverSampling/specification/PJDLR-specification-v2.0.md)**

```cpp
/*
Milan, Italy - Originally published: 10/04/2010 - latest revision: 24/09/2017
PJDLR (Padded jittering data link) v2.0
Invented by Giovanni Blu Mitolo, preamble proposed by Fred Larsen
released into the public domain

Related implementation: /strategies/OverSampling/
Compliant implementation versions: PJON 9.0 and following
Changelog: Added frame initializer
*/
```
### PJDLR (Padded Jittering Data Link Radio)
PJDLR is a simplex or half-duplex data link layer, that can be easily software emulated, enabling one or many to many communication in both master-slave and multi-master configuration optimized to obtain long range and high reliability using radio transceivers. It has been engineered to have limited minimum requirements, and to be efficiently executed on limited microcontrollers with poor clock accuracy. No additional hardware is required to apply PJDLR, and, being implemented in c++, in less than 350 lines of code, it is easily portable to many different architectures.

#### Basic concepts
* Define a synchronization pad initializer to identify a byte
* Use synchronization pad's falling edge to achieve byte level synchronization
* Use 3 consequent synchronization pads identify a frame
* Use a frame preamble to support gain regulation before reception
* Detect interference or absence of communication at byte level
* Support collision avoidance and detection
* Support error detection
* Support 1 byte synchronous response to frame

#### Byte transmission
Every byte is prepended with a synchronization pad and transmission occurs LSB-first. The first bit is a shorter than standard logic 1 followed by a standard logic 0. The reception method is based on finding a logic 1 as long as the first padding bit, synchronizing to its falling edge and checking if it is followed by a logic 0. If this pattern is detected, reception starts, if not, interference, synchronization loss or simply absence of communication is detected at byte level.    
```cpp  
 _____ ___________________________
| Pad | Byte                      |
|_    |___       ___     _____    |
| |   |   |     |   |   |     |   |
|1| 0 | 1 | 0 0 | 1 | 0 | 1 1 | 0 |
|_|___|___|_____|___|___|_____|___|
```
Padding bits add a certain overhead but are reducing the need of precise timing because synchronization is renewed every byte.

#### Frame transmission
Before a frame transmission, the communication medium is analysed, if logic 1 is present ongoing communication is detected and collision avoided, if logic 0 is detected for a duration longer than a byte transmission plus its synchronization pad and a small random timeframe, a packet preamble, composed of a long 1 and a long 0, is transmitted to let a potential receiver to adjust its gain to the transmitted signal magnitude. The duration of the preamble bits have to be selected considering hardware sensitivity and gain refresh time. frame transmission starts after preamble, with 3 synchronization pads, followed by data bytes. The presence of synchronization pads with their logic 1 between each byte ensures that also a frame composed of a series of bytes with decimal value 0 can be transmitted safely without risk of third-party collision.

```cpp     
           INITIALIZER  DATA
 _________ ___________ __________ _______________ ______________
|Preamble |Pad|Pad|Pad| Byte     |Pad| Byte      |Pad| Byte     |
|_____    |_  |_  |_  |     __   |_  |      _   _|_  |      _   |
|     |   | | | | | | |    |  |  | | |     | | | | | |     | |  |
|  1  | 0 |1|0|1|0|1|0|0000|11|00|1|0|00000|1|0|1|1|0|00000|1|00|
|_____|___|_|_|_|_|_|_|____|__|__|_|_|_____|_|_|_|_|_|_____|_|__|
```
In a scenario where a frame is received, low performance microcontrollers with inaccurate clock can correctly synchronize with transmitter during the frame initializer, and consequently each byte is received. The frame initializer is detected if 3 synchronizations occurred and if its duration is coherent with its expected duration. With a correct bit and synchronization pad ratio and timing configuration, the frame initializer is 100% reliable, false positives cannot occur if not because of externally induced interference.     

#### Synchronous response
A frame transmission can be optionally followed by a synchronous response by its recipient.
```cpp  
Transmission                                                      Response
 ________ ______  ______  ______  ______                   ________ _____
|PREAMBLE| INIT || BYTE || BYTE || BYTE | CRC COMPUTATION |PREAMBLE| ACK |
|____    |------||------||------||------|-----------------|____    |     |
|    |   |      ||      ||      ||      | LATENCY         |    |   |  6  |
|____|___|______||______||______||______|                 |____|___|_____|
```

The maximum time dedicated to potential acknowledgement reception has to be same for all connected devices, and it is defined by the use case constraints like maximum packet length and latency or physical distance between devices.
