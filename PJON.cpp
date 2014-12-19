
 /*-O//\         __     __
   |-gfo\       |__| | |  | |\ |
   |!y°o:\      |  __| |__| | \|
   |y"s§+`\     Giovanni Blu Mitolo 2012 - 2014
  /so+:-..`\    gioscarab@gmail.com
  |+/:ngr-*.`\
   |/:%&-a3f.:/\     PJON is a communication protocol and a network that connects up to 255
    \+//u/+gosv//\    arduino boards with a single digital pin to the same wire and communicates
     \o+&/osw+odss\\   up to 4Kb/s with acknowledge, collision detection, CRC and encpryption.
       \:/+-.§°-:+oss\
        | |       \oy\\   Pull down resistor on the bus is generally used to reduce interference
        > <
       -| |-

ADDRESS: Is possible assign up to 255 different adresses
CRC: XOR Cyclic Redundancy Check to ensure almost errorless data communication
ACKNOLEDGE: Packet delivery is ensured by an acknowledge byte sent by receiver
COLLISION DETECTION: Transmissions collision avoidance analyzing network bus before starting
ENCRYPTION: Private key encryption + initialization vector to ensure almost random data stream

PJON Slow mode
Absolute bandwidth 3.0 kb/s | Practical bandwidth 2.38 kb/s | Accuracy: 99.25%
bit_width 20 - bit_spacer 68 - acceptance 16 - read_delay 9

PJON Medium mode
Absolute bandwidth 3.51 kb/b | Practical bandwidth 2.75 kb/s | Accuracy: 96.6%
bit_width 18 - bit_spacer 36 - acceptance 16 - read_delay 7 */

#include "PJON.h"

/* Initiate PJON passing pin number and the selected device_id */
PJON::PJON(int input_pin, uint8_t device_id) {
  _input_pin = input_pin;
  _device_id = device_id;
}

/* Set collision detection:
 If true Avoids to transmit over other transmitting devices:
 on top of this can be developed a multimaster network of Arduino boards
 that arbitrally decides to communicate (slight reduction of bandwidth) */
void PJON::set_collision_avoidance(boolean state) {
  _collision_avoidance = state;
}

/* Set acknowledge state:
 If true sender waits for a feedback by receiver after transmission:
 After the string is sent, receiver answers with an ACK if
 CRC is ok or NAK if wrong (slight reduction of bandwidth) */
void PJON::set_acknowledge(boolean state) {
  _acknowledge = state;
}

/* Set Encryption state to transmitted data */
void PJON::set_encryption(boolean state) {
  _encryption = state;
}

/* Encrypt string with a custom made private key algorithm */
void PJON::crypt(char *data, boolean initialization_vector, boolean side) {
  uint8_t i, j = 0;
  uint8_t string_length = strlen(data);
  uint8_t encryption_key_length = strlen(encryption_key);

  if(initialization_vector && side)
    for(i = 0; i < string_length; i++)
      data[i] = data[i] ^ data[string_length - 1];

  for (i = 0; i < encryption_strength; i++)
    _s_box[i] = i;

  for (i = 0; i < encryption_strength; i++) {
    j = (j + _s_box[i] + encryption_key[i % encryption_key_length]) % encryption_strength;
    swap(_s_box[i], _s_box[j]);
  }

  i = j = 0;
  for (int k = 0; k < string_length; k++) {
    i = (i + 1) % encryption_strength;
    j = (j + _s_box[i]) % encryption_strength;
    swap(_s_box[i], _s_box[j]);
    hash[k] = data[k] ^ _s_box[ (_s_box[i] + _s_box[j]) % encryption_strength];
  }

  if(initialization_vector && !side) {
    hash[string_length] = this->generate_IV(string_length);
    for(i = 0; i < string_length; i++)
      hash[i] = hash[i] ^ hash[string_length];
  }

  hash[string_length + 1] = '\0';
}

uint8_t PJON::generate_IV(uint8_t string_length) {
  uint8_t IV = (micros() % 254) + 1;
  for(uint8_t i = 0; i < string_length; i++)
    if(hash[i] == IV)
      return this->generate_IV(string_length);
  return IV;
}

/* Send a bit to the pin
 digitalWriteFast is used instead of standard digitalWrite
 function to optimize transmission time */
void PJON::send_bit(uint8_t VALUE, int duration) {
  digitalWriteFast(_input_pin, VALUE);
  delayMicroseconds(duration);
}

/* Send a byte to the pin:

    Init            Byte
 |--------|-----------------------|
 |_____    __       __    __.__   |
 |     |  |  |     |  |  |     |  |
 |1    |0 |1 |0  0 |1 |0 |1  1 |0 |
 |     |__|  |__.__|  |__|     |__|

 Init is a long 1 with a bit_spacer duration
 (in general longer then a bit) then comes the raw byte */
void PJON::send_byte(uint8_t b) {
  pinModeFast(_input_pin, OUTPUT);
  this->send_bit(HIGH, bit_spacer);
  this->send_bit(LOW, bit_width);

  for(uint8_t mask = 0x01; mask; mask <<= 1)
    this->send_bit(b & mask, bit_width);
}

/* Send a string to the pin */
int PJON::send_string(uint8_t ID, char *string) {

  uint8_t package_length = strlen(string) + 4;
  uint8_t CRC = 0;

  if(_collision_avoidance && !this->can_start())
      return BUSY;

  if(_encryption)
    this->crypt(string, true, 0);

  this->send_byte(ID);
  CRC ^= ID;
  this->send_byte(package_length);
  CRC ^= package_length;

  char *string_pointer = (_encryption) ? hash : string;
  while(*string_pointer) {
    this->send_byte(*string_pointer);
    CRC ^= *string_pointer;
    *string_pointer++;
  }

  this->send_byte(CRC);
  digitalWriteFast(_input_pin, LOW);

  if(!_acknowledge) return ACK;
  unsigned long time = micros();
  int response = FAIL;

  while(response == FAIL && micros() - time <= bit_spacer + bit_width)
    response = this->start();

  if(response == NAK) return NAK;
  if(response == ACK) return ACK;
  return FAIL;

};

/* Send a command to the pin:
 Command is formatted in a string and sent as it is */
int PJON::send_command(byte ID, byte command_type, unsigned int value) {
  char bytes_to_send[3] = { command_type, value >> 8, value & 0xFF };
  return this->send_string(ID, bytes_to_send);
}

/* Receive a bit from the pin:
 This function is used only in byte syncronization.
 read_delay has to be tuned to correctly send and
 receive transmissions because this variable it
 shifts in which portion of the bit reading will be
 executed by the next receive_byte function */
uint8_t PJON::receive_bit() {
  delayMicroseconds((bit_width / 2) - read_delay);
  uint8_t bit_value = digitalReadFast(_input_pin);
  delayMicroseconds(bit_width / 2);
  return bit_value;
}

/* Receive a byte from the pin */
uint8_t PJON::receive_byte() {
  uint8_t byte_value = B00000000;
  delayMicroseconds(bit_width / 2);
  for (uint8_t i = 0; i < 8; i++) {
    byte_value += digitalReadFast(_input_pin) << i;
    delayMicroseconds(bit_width);
  }
  return byte_value;
}

/* Check if the channel if free for transmission:
 If an entire byte received contains no 1 it means
 that there is no active transmission */
boolean PJON::can_start() {
  pinModeFast(_input_pin, INPUT);
  this->send_bit(0, 8);
  if(!this->receive_byte())
    return true;

  return false;
}

/* Check if a byte is coming from the pin:
 If there is a 1 and is longer then acceptance
 and after that comes a 0 probably a byte is coming */
int PJON::start() {
  pinModeFast(_input_pin, INPUT);
  digitalWriteFast(_input_pin, LOW);

  unsigned long time = micros();
  while (digitalReadFast(_input_pin) && micros() - time <= bit_spacer);
  time = micros() - time;

  if(time > acceptance && !this->receive_bit())
    return (int)this->receive_byte();

  return FAIL;
}

/* Receive a string from the pin */
int PJON::receive() {
  int package_length = max_package_length;
  uint8_t CRC = 0;

  for (uint8_t i = 0; i < package_length; i++) {
    data[i] = this->start();

    if (data[i] == FAIL)
      return FAIL;

    if(i == 0 && data[i] != _device_id)
      return BUSY;

    if(i == 1)
      package_length = data[i];

    if (i < package_length - 1)
      CRC ^= data[i];
  }

  if (data[package_length - 1] == CRC) {
    if(_acknowledge) {
      this->send_byte(ACK);
      digitalWriteFast(_input_pin, LOW);
    }
    if(_encryption) {
      for(int i = 0; i < package_length - 1; i++)
        if(i > 1) data[i - 2] = data[i];

      this->crypt((char*)data, true, 1);
    }
    return ACK;

  } else {
    if(_acknowledge) {
      this->send_byte(NAK);
      digitalWriteFast(_input_pin, LOW);
    }
    return NAK;
  }
}
