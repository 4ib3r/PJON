
![PJON](http://www.gioblu.com/PJON/PJON-github-header-tiny.png)
## PJON v8.2
PJON™ (Padded Jittering Operative Network) is an Arduino compatible, multi-master, multi-media communications bus system. It proposes a Standard, it is designed as a framework and implements a totally software emulated network protocol stack that can be easily cross-compiled on many architectures like ATtiny, ATmega, ESP8266, Teensy, Raspberry Pi and Windows x86 machines. It is a valid tool to quickly and comprehensibly build a network of devices. Visit [wiki](https://github.com/gioblu/PJON/wiki) and [documentation](https://github.com/gioblu/PJON/wiki/Documentation) to know more about the PJON Standard.

[![Get PJON bus id](https://img.shields.io/badge/GET-PJON%20bus%20id-lightgrey.svg)](http://www.pjon.org/get-bus-id.php)
[![Video introduction](https://img.shields.io/badge/PJON-video%20introduction-blue.svg)](https://www.youtube.com/watch?v=vjc4ZF5own8)
[![Join the chat at https://gitter.im/gioblu/PJON](https://badges.gitter.im/gioblu/PJON.svg)](https://gitter.im/gioblu/PJON?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge) [![Donate](https://img.shields.io/badge/DONATE-Paypal-green.svg)](https://www.paypal.me/PJON)

PJON is used in thousands of devices and its community has spread worldwide because of the following 5 key factors:
- **Increased Security**: Ethernet and WiFi are exposing many smart things to ransomware, illegal cyber-warfare activities and putting privacy at risk. PJON has been engineered to enhance security not necessarily implementing the standard network protocol stack together with its vulnerabilities where it is not absolutely necessary, offering a set of alternatives covering many use cases.
- **New technology**: PJON, while supporting TCP, UDP, Serial and RS485, it implements the PJDL data link able to communicate data through a single common conductive element shared by up to 255 devices, or through a single LED used as a wireless or optic fiber transceiver and also supporting many ASK/FSK radio modules available on the market, easing integration and enabling a lot of applications.
- **High flexibility**: The PJON network protocol stack is modular enabling the use of the same network protocol implementation on different data links or media simply changing its configuration.
- **Fast portability**: PJON is totally "software emulated" or executed by software without the use of dedicated hardware; this engineering choice and its simplicity makes it easy to be ported on any computer or microcontroller.
- **Low cost**: Without any additional hardware needed to operate, minimal network wiring requirements and direct pin-to-pin or LED-to-LED low current communication, PJON is extremely energy efficient, cheap to be implemented and maintained, thanks also to the strong support of its growing community of end users, testers and developers.

#### Features
- Cross-compilation support with the [interfaces](interfaces) system calls abstraction   
- Multi-media support with the [strategies](strategies) data link layer abstraction
- Master-slave or multi-master [dynamic addressing](specification/PJON-dynamic-addressing-specification-v0.1.md)
- Hot-swap support, replacing or adding devices with no need of system reset or shut down
- Configurable synchronous and/or asynchronous [acknowledgement](specification/PJON-protocol-acknowledge-specification-v0.1.md)
- Configurable 2 level addressing (device and bus id) for scalable applications
- Configurable 1 or 2 bytes packet length (max 255 or 65535 bytes)
- Collision avoidance to enable multi-master capability
- Configurable CRC8 or CRC32 table-less cyclic redundancy check
- Packet manager to handle, track and if necessary retransmit a packet sending in background
- Optional ordered packet sending
- Error handling

#### PJON (Padded Jittering Operative Network) Protocol specification
- PJON [v1.1](specification/PJON-protocol-specification-v1.1.md)
- PJON Acknowledge [v0.1](specification/PJON-protocol-acknowledge-specification-v0.1.md)
- PJON Dynamic addressing [v0.1](specification/PJON-dynamic-addressing-specification-v0.1.md)

#### PJDL (Padded Jittering Data Link) specification
- PJDL [v1.1](strategies/SoftwareBitBang/specification/PJDL-specification-v1.1.md)
- PJDLR [v1.1](strategies/OverSampling/specification/PJDLR-specification-v1.1.md)

#### Compliant tools
- [ModuleInterface](https://github.com/fredilarsen/ModuleInterface) by Fred Larsen
- [PJON-piper](https://github.com/Girgitt/PJON-piper) by Zbigniew Zasieczny
- [PJON-python](https://github.com/Girgitt/PJON-python) by Zbigniew Zasieczny
- [saleae-pjon-protocol-analyzer](https://github.com/aperepel/saleae-pjon-protocol-analyzer) by Andrew Grande

PJON™ is a self-funded, no-profit open-source project created in 2010 and maintained by Giovanni Blu Mitolo with the support of the internet community. If you want to see the PJON project growing with a faster pace, consider a donation at the following link: https://www.paypal.me/PJON

PJON™ and its brand are unregistered trademarks, property of Giovanni Blu Mitolo gioscarab@gmail.com
