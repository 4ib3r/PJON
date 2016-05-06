#include <PJON.h>

// <Strategy name> bus(selected device id)
PJON<SoftwareBitBang> bus(45);

void setup() {
  pinModeFast(13, OUTPUT);
  digitalWriteFast(13, LOW); // Initialize LED 13 to be off

  bus.set_pin(12);
  bus.begin();

  bus.set_receiver(receiver_function);
  bus.send(44, "B", 1);
  
  Serial.begin(115200);
};

void receiver_function(uint8_t length, uint8_t *payload) {
 if((char)payload[0] == 'B') {
    bus.send(44, "B", 1);
    digitalWrite(13, HIGH);
    delay(15);
    digitalWrite(13, LOW);
  }
}

void loop() {
  bus.receive(1000);
  bus.update();
};
