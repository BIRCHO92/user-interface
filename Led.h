#ifndef LED_H_
#define LED_H_
#include <Arduino.h>

class Led {
    byte pin;  
    bool def_high; // True if the LED is on when the pin is HIGH
  public:
    // Led(byte pin);
    void init(byte pin, bool def_high = true );
    void on();
    void off();
    void blink(uint8_t ontime); 
};

#endif