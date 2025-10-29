// Arduino Serial Monitor Bridge from ChatGPT
// Forwards serial data from its RX pin (connected to ATmega TX)
// to your computer's USB serial port.

void setup() {
  Serial.begin(9600); // Must match the ATmega's Serial.begin() baud rate
}

void loop() {
  if (Serial.available()) {
    // Optional: forward data from PC → ATmega if you want to send commands back
    // Serial.write(Serial.read());
  }

  // Read data from ATmega and send it to PC
  if (Serial.available() == 0) {
    // No new data from PC; just listen for ATmega output
  }

  while (Serial.available() > 0) {
    Serial.write(Serial.read());
  }
}
