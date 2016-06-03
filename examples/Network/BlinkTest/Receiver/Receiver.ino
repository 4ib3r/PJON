#include <PJON.h>

// Bus id definition
uint8_t bus_id[] = {0, 0, 0, 1};

// PJON object
PJON<SoftwareBitBang> bus(bus_id, 44);

void setup() {
  pinModeFast(13, OUTPUT);
  digitalWriteFast(13, LOW); // Initialize LED 13 to be off

  bus.set_pin(12);
  bus.begin();
  bus.set_receiver(receiver_function);

  Serial.begin(9600);
};

void receiver_function(uint8_t id, uint8_t *payload, uint8_t length) {
  Serial.print("Message received for bus id: ");
  Serial.print(payload[0]);
  Serial.print(payload[1]);
  Serial.print(payload[2]);
  Serial.print(payload[3]);
  Serial.print(" - device id: ");
  Serial.println(id);

  if(payload[4] == 'B') {
    Serial.println(" BLINK!");
    digitalWrite(13, HIGH);
    delay(30);
    digitalWrite(13, LOW);
  }
}

void loop() {
  bus.receive(1000);
};
