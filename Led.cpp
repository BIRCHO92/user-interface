// Simple library to handle instances of LEDs

#include <Led.h>

void Led::init(byte pin, bool def_high) {
  this->pin = pin;
  this->def_high = def_high;
  pinMode(pin, OUTPUT);
  off();
}

void Led::on() {
  // Switch the LED on.
  if (def_high) {
    digitalWrite(pin, LOW);
  } else {
      digitalWrite(pin, HIGH);
  }
}

void Led::off() {
  // Switch the LED off.
  if (def_high) {
    digitalWrite(pin, HIGH); }
  else {
    digitalWrite(pin, LOW);
  }
}

void Led::blink(uint8_t ontime ) {
  // Blink in a 255ms cycle, where it is on for 'ontime' and off for 255 - ontime.
  if ( (millis() & 0xff) < ontime) {
    on();
  } else {
    off();
  }
}

