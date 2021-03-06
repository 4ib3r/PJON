### ThroughAsyncSerial

**Medium:** Hardware/Software Serial port |
**Pins used:** 1 or 2

With `ThroughSerialAsync` strategy, PJON can run through a software or hardware Serial port working out of the box with many Arduino compatible serial transceivers, like RS485 or radio modules like HC-12 (HCMODU0054). It complies with [TSDL v2.0](/src/strategies/ThroughSerial/specification/TSDL-specification-v2.0.md).  

This strategy is based upon the `ThroughSerial` but reception is asynchronous and completely non-blocking, making good use of hardware buffers and sparing time that `ThroughSerial` looses polling. It is not required to call `bus.receive()` with any delay, just call it frequently to see if there are any packets that have been received.

#### Why PJON over Serial?
Serial communication is fast and reliable but it is often useless without all the features PJON contains. `ThroughAsyncSerial` has been developed to enable PJON communication through a serial data link. Adding PJON on top of Serial it is possible to leverage of the PJON protocol layer features like acknowledge, addressing, multiplexing, packet handling, 8 or 32-bit CRC and traffic control.  

Being impossible to detect or avoid collisions over a serial port, `ThroughSerialAsync` has been developed primarily to be used in master-slave mode. `ThroughSerialAsync` in multi-master mode, being unable to detect or avoid collisions, operates using the pure ALOHA medium access method. Of all contention based random multiple access methods, pure ALOHA, which maximum data throughput is only 18.4% of the available bandwidth, is the least efficient and should not be applied in networks where many devices often need to arbitrarily transmit data.

`ThroughSerialAsync` performs well if used with ESP8266 and ESP32 where blocking procedures can strongly degrade functionality. The reception phase is entirely non-blocking. Sending and acknowledgement however are still blocking.

There is a default reception interval of 100 microseconds used to allow data to accumulate in the hardware UART buffer. This value is configurable using `bus.strategy.set_read_interval(100)` passing an arbitrary interval in microseconds. The read interval may require adjustment depending on UART RX buffer size and baud rate.  

#### How to use ThroughAsyncSerial
Pass the `ThroughSerial` type as PJON template parameter to instantiate a PJON object ready to communicate through this Strategy.
```cpp  
#include PJON_INCLUDE_TAS
PJON<ThroughAsyncSerial> bus;
```
Call the `begin` method on the `Serial` or `SoftwareSerial`  object you want to use for PJON communication and pass it to the `set_serial` method:
```cpp  
/* Set 100 microseconds (default) as the minimum interval between every
   Depending on the latency, baud rate and computation time the
   optimal TSA_READ_INTERVAL value may variate.
   Always set: TSA_READ_INTERVAL > (byte transmission time + latency) */
#define TSA_READ_INTERVAL 100

/* Set 1000000 microseconds or 1s (default) as the maximum timeframe for
   byte reception. Always set:
   TSA_BYTE_TIME_OUT > (byte transmission time + latency) */
#define TSA_BYTE_TIME_OUT      1000000

/* Set 10 milliseconds as the maximum timeframe between
   transmission and synchronous acknowledgement response.
   Its  optimal configuration is strictly related to the
   maximum time needed by receiver to receive the packet, compute and
   transmit back an ACK (decimal 6) */
#define TSA_RESPONSE_TIME_OUT 10000

/* Set the back-off exponential degree (default 4) */
#define TSA_BACK_OFF_DEGREE 4

/* Set the maximum sending attempts (default 20) */
#define TSA_MAX_ATTEMPTS   20

#include <PJON.h>

PJON<ThroughAsyncSerial> bus;

void setup() {
  Serial.begin(9600);
  bus.strategy.set_serial(&Serial);
  bus.strategy.set_read_interval(100);
}
```
For a simple use with RS485 serial modules a transmission enable pin setter has been added:
```cpp  
bus.strategy.set_enable_RS485_pin(11);
```
If separate enable setters are needed use:
```cpp  
// Set RS485 reception enable pin
bus.strategy.set_RS485_rxe_pin(11);
// Set RS485 transmission enable pin
bus.strategy.set_RS485_txe_pin(12);
```
See [RS485-Blink](../../examples/ARDUINO/Local/ThroughSerial/RS485-Blink) and [RS485-AsyncAck](../../examples/ARDUINO/Local/ThroughSerial/RS485-AsyncAck) examples.

HC-12 wireless module supports both synchronous and asynchronous acknowledgement, see [HC-12-Blink](../../examples/ARDUINO/Local/ThroughSerial/HC-12-Blink), [HC-12-SendAndReceive](../../examples/ARDUINO/Local/ThroughSerial/HC-12-SendAndReceive) and [HC-12-AsyncAck](../../examples/ARDUINO/Local/ThroughSerial/HC-12-AsyncAck) examples.

All the other necessary information is present in the general [Documentation](/documentation).

#### Known issues
- Transmission is still blocking, will be made non-blocking in the next versions.
- acknowledgement procedure is still blocking, will be made non-blocking in the next versions.
