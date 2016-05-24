#include <PJON.h>

// Bus id definition
uint8_t bus_id[] = {0, 0, 0, 1};

// PJON object
PJON<SoftwareBitBang> bus(bus_id, 45);

int packet;

void setup() {
  bus.set_pin(12);
  bus.begin();
  bus.set_error(error_handler);

  Serial.begin(115200);

  unsigned long time = micros();
  bus.acquire_id();
  Serial.print(micros() - time);

  Serial.print(" microseconds needed - ID found: ");
  Serial.print(bus.device_id());

  char actual_id = bus.device_id();
  packet = bus.send(44, &actual_id, 1, 1000000);
}

void error_handler(uint8_t code, uint8_t data) {
  if(code == ID_ACQUISITION_FAIL) {
    Serial.print("Can't acquire a free id ");
    Serial.println(data);
  }
}

void loop() {
 bus.update();
 bus.receive();
}
