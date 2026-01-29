#ifndef VEHICLE_H
#define VEHICLE_H

#include "NAPacket.h"
#include <Arduino.h>

class Vehicle {
public:
  virtual void setup() = 0;
  virtual void loop() = 0;
  virtual void setInputs(NAPacket *packet) = 0;
  virtual String getName() const = 0;
};

#endif
