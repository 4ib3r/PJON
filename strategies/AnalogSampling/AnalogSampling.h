
/* AnalogSampling data link layer
   used as a Strategy by the PJON framework
   Compliant with the Padded jittering data link layer specification v0.1

   It is designed to sample digital data using analog readings.
   It can be effectively used to communicate data wirelessly, through
   any sort of radiation transceiver. The most basic example is to use a couple
   of visible light LEDs connected to the A0 pin used as wireless transceivers
   infact, leveraging the duality of LEDs:
   - Ability to emit photons if electrons are travelling through the junction
   - Ability to emit electrons if photons are hitting the junction (photo-electric effect)
   it is possibile to use them as wireless (bidirectional) transceivers!

   The obtained range is related to:
   - ADC prescaler (reading duration)
   - Analog reference (voltage reading resolution)
   - LED sensitivity to the signal
   - Available current for transmitter

   For a long range use case a couple of photodiodes and laser emitters is
   suggested. It may be necessary to teak timing constants in Timing.h.
   ____________________________________________________________________________

   Copyright 2012-2017 Giovanni Blu Mitolo gioscarab@gmail.com

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. */

#include "Timing.h"
#include "../../utils/PJON_IO.h" // Dedicated version of digitalWriteFast

/* Default reading state threshold: */

#ifndef AS_THRESHOLD
  #define AS_THRESHOLD    1
#endif

/* _AS_STANDARD transmission mode performance:
   Transfer speed: 1024Bb or 128B/s
   Absolute  communication speed: 128B/s (data length 20 of characters)
   Data throughput: 100B/s (data length 20 of characters) */
#define _AS_STANDARD  1

/* Set here the selected transmission mode - default STANDARD */
#ifndef AS_MODE
  #define AS_MODE _AS_STANDARD
#endif

class AnalogSampling {
  public:

    /* Returns the suggested delay related to the attempts passed as parameter: */

    uint32_t back_off(uint8_t attempts) {
      uint32_t result = attempts;
      for(uint8_t d = 0; d < AS_BACK_OFF_DEGREE; d++)
        result *= (uint32_t)(attempts);
      return result;
    };


    /* Begin method, to be called before transmission or reception:
       (returns always true) */

    boolean begin(uint8_t additional_randomness = 0) {
      delay(random(0, AS_INITIAL_DELAY) + additional_randomness);
      PJON_IO_PULL_DOWN(_input_pin);
      if(_output_pin != _input_pin)
        PJON_IO_PULL_DOWN(_output_pin);
      uint32_t time = micros();
      compute_analog_read_duration();
      return true;
    };


    /* Check if the channel is free for transmission:
    If receiving 10 bits no 1s are detected
    there is no active transmission */

    boolean can_start() {
      if(read_byte() != B00000000) return false;
      delayMicroseconds(AS_BIT_SPACER / 2);
      if(analogRead(_input_pin) > threshold) return false;
      delayMicroseconds(AS_BIT_SPACER / 2);
      if(analogRead(_input_pin) > threshold) return false;
      delayMicroseconds(random(0, AS_COLLISION_DELAY));
      if(analogRead(_input_pin) > threshold) return false;
      return true;
    };


    /* compute analogRead duration: */

    void compute_analog_read_duration() {
      uint32_t time = micros();
      analogRead(_input_pin);
      _analog_read_time = (uint32_t)(micros() - time);
      for(uint8_t i = 0; i < 10; i++) {
        time = micros();
        analogRead(_input_pin);
        _analog_read_time = (_analog_read_time * 0.75) + ((uint32_t)(micros() - time) * 0.25);
        // TODO - check for granularity
      }
    };


    /* Returns the maximum number of attempts for each transmission: */

    static uint8_t get_max_attempts() {
      return AS_MAX_ATTEMPTS;
    };


    /* Handle a collision: */

    void handle_collision() {
      delayMicroseconds(random(0, AS_COLLISION_DELAY));
    };


    /* Read a byte from the pin */

    uint8_t read_byte() {
      int bit_value;
      int high_bit = 0;
      int low_bit = 0;
      uint8_t byte_value = 0;
      for(int i = 0; i < 8; i++) {
        long time = micros();
        delayMicroseconds((AS_BIT_WIDTH / 2) - AS_READ_DELAY);
        bit_value = analogRead(_input_pin);
        byte_value += (bit_value > threshold) << i;
        high_bit = (((bit_value > threshold) ? bit_value : high_bit) + high_bit) / 2;
        low_bit  = (((bit_value < threshold) ? bit_value : low_bit) + low_bit) / 2;
        delayMicroseconds(AS_BIT_WIDTH - (uint32_t)(micros() - time));
      }
      threshold = (high_bit + low_bit) / 2;
      _last_update = micros();
      return byte_value;
    };


    /* Check if a byte is coming from the pin:
     This function is looking for padding bits before a byte.
     If value is 1 for more than ACCEPTANCE and after
     that comes a 0 probably a byte is coming:
      ________
     |  Init  |
     |--------|
     |_____   |
     |  |  |  |
     |1 |  |0 |
     |__|__|__|
        |
      ACCEPTANCE */


    uint16_t receive_byte() {
      PJON_IO_PULL_DOWN(_input_pin);
      if(_output_pin != PJON_NOT_ASSIGNED && _output_pin != _input_pin)
        PJON_IO_PULL_DOWN(_output_pin);
      uint32_t time = micros();

      if(
        ((uint32_t)(micros() - _last_update) > AS_THRESHOLD_DECREASE_INTERVAL) &&
        threshold
      ) {
        threshold *= 0.25;
        _last_update = micros();
      }

      while(
        (analogRead(_input_pin) > threshold) &&
        ((uint32_t)(micros() - time) <= AS_BIT_SPACER)
      ); // Do nothing

      time  = micros() - time;
      if(time >= AS_BIT_SPACER * 0.75 && time <= AS_BIT_SPACER * 1.25) {
        delayMicroseconds(AS_BIT_WIDTH);
        return read_byte();
      }
      return FAIL;
    };


    /* Receive byte response */

    uint16_t receive_response() {
      PJON_IO_PULL_DOWN(_input_pin);
      if(_output_pin != PJON_NOT_ASSIGNED && _output_pin != _input_pin)
        PJON_IO_WRITE(_output_pin, LOW);
      uint16_t response = FAIL;
      uint32_t time = micros();
      while(
        (response != ACK) &&
        (response != NAK) &&
        (uint32_t)(micros() - AS_TIMEOUT) <= time
      ) response = receive_byte();
      return response;
    };


    /* Every byte is prepended with 2 synchronization padding bits. The first
       is a longer than standard logic 1 followed by a standard logic 0.
       __________ ___________________________
      | SyncPad  | Byte                      |
      |______    |___       ___     _____    |
      | |    |   |   |     |   |   |     |   |
      | | 1  | 0 | 1 | 0 0 | 1 | 0 | 1 1 | 0 |
      |_|____|___|___|_____|___|___|_____|___|
        |
       ACCEPTANCE

    The reception tecnique is based on finding a logic 1 as long as the
    first padding bit within a certain threshold, synchronizing to its
    falling edge and checking if it is followed by a logic 0. If this
    pattern is recognised, reception starts, if not, interference,
    synchronization loss or simply absence of communication is
    detected at byte level. */

    void send_byte(uint8_t b) {
      PJON_IO_WRITE(_output_pin, HIGH);
      delayMicroseconds(AS_BIT_SPACER);
      PJON_IO_WRITE(_output_pin, LOW);
      delayMicroseconds(AS_BIT_WIDTH);
      for(uint8_t mask = 0x01; mask; mask <<= 1) {
        PJON_IO_WRITE(_output_pin, b & mask);
        delayMicroseconds(AS_BIT_WIDTH);
      }
    };


    /* Send byte response to package transmitter */

    void send_response(uint8_t response) {
      delayMicroseconds(AS_BIT_WIDTH);
      PJON_IO_PULL_DOWN(_input_pin);
      PJON_IO_MODE(_output_pin, OUTPUT);
      send_byte(response);
      PJON_IO_PULL_DOWN(_output_pin);
    };


    /* Send a string: */

    void send_string(uint8_t *string, uint16_t length) {
      PJON_IO_MODE(_output_pin, OUTPUT);
      for(uint16_t b = 0; b < length; b++)
        send_byte(string[b]);
      PJON_IO_PULL_DOWN(_output_pin);
    };


    /* Set the communicaton pin: */

    void set_pin(uint8_t pin) {
      _input_pin = pin;
      _output_pin = pin;
    };


    /* Set a pair of communication pins: */

    void set_pins(
      uint8_t input_pin = PJON_NOT_ASSIGNED,
      uint8_t output_pin = PJON_NOT_ASSIGNED
    ) {
      _input_pin = input_pin;
      _output_pin = output_pin;
    };


    /* Set the threshold analog value between a LOW and a HIGH read: */

    void set_threshold(uint16_t value) {
      threshold = value;
    };


    /* Set the ADC prescaler (4 default): */

    void set_prescaler(uint16_t prescaler) {
      _prescaler = prescaler;
      compute_analog_read_duration();
    };

    uint16_t threshold = AS_THRESHOLD;
  private:
    uint16_t _analog_read_time;
    uint8_t  _input_pin;
    uint8_t  _output_pin;
    uint8_t  _prescaler = B00000111;
    uint32_t _last_update;
};
