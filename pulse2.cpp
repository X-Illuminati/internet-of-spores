#include <Arduino.h>
#include <string.h>
#include <functional>
#include <FunctionalInterrupt.h>
#include "pulse2.h"

Pulse2::Pulse2(void)
{
  //initialize the pins list
  memset(this->pin, PULSE2_NO_PIN, sizeof(this->pin));
  this->overflow_count=0;
  this->missed_count=0;
}

Pulse2::~Pulse2(void)
{
  for (int i=0; i<PULSE2_MAX_PINS; i++)
    if (this->pin[i] != PULSE2_NO_PIN)
      this->unregister_pin(this->pin[i]);
}

bool Pulse2::register_pin(uint8_t pin, uint8_t direction)
{
  bool retval = true;
  int slot = 0;
  int pinmode;

  //Serial.printf("%s start\n", __func__);
  for(slot=0; slot<PULSE2_MAX_PINS; slot++) {
    if (this->pin[slot] == PULSE2_NO_PIN)
      break;
    if (this->pin[slot] == pin)
      break;
  }
  //Serial.printf("%s slot=%d\n", __func__, slot);

  if (this->pin[slot] == pin)
    detachInterrupt(digitalPinToInterrupt(pin));
  else
    this->pin[slot] = pin;

  if (slot == PULSE2_MAX_PINS)
    return false;

  this->direction[slot] = direction;
  this->pin_start_micros[slot] = 0;
  this->pin_result_count[slot] = 0;
  this->pin_result_slot[slot] = 0;
  memset((void*)(this->pin_result[slot]), 0, sizeof(this->pin_result[slot]));

  if (HIGH == direction)
    pinmode = RISING;
  else
    pinmode = FALLING;

  //Serial.printf("%s pinmode=%d\n", __func__, pinmode);

  std::function<void (void)> handler = std::bind(&Pulse2::handle_interrupt, this, slot, pin, pinmode);
  attachInterrupt(digitalPinToInterrupt(pin), handler, pinmode);

  return retval;
}

void Pulse2::unregister_pin(uint8_t pin)
{
  int slot = 0;

  detachInterrupt(digitalPinToInterrupt(pin));

  for(int slot=0; slot<PULSE2_MAX_PINS; slot++)
    if (this->pin[slot] == pin)
      break;

  if (slot == PULSE2_MAX_PINS)
    return;

  this->pin[slot] = PULSE2_NO_PIN;
}

uint8_t Pulse2::watch(unsigned long *result, unsigned long timeout)
{
  static unsigned long overflow_count = 0;
  static unsigned long missed_count = 0;
  unsigned long start_time = micros();
  uint8_t retval;

  if (this->overflow_count > overflow_count) {
    overflow_count = this->overflow_count;
    //Serial.printf("%lu: Overflow: %lu\n", millis(), overflow_count);
  }
  if (this->missed_count > missed_count) {
    missed_count = this->missed_count;
    //Serial.printf("%lu: Missed: %lu\n", millis(), missed_count);
  }
  retval = this->check_result(result);
  if (PULSE2_NO_PIN != retval)
    return retval;

  while (micros() - start_time < timeout) {
    if (this->wait_status != 0) {
      retval = this->check_result(result);
      if (PULSE2_NO_PIN != retval)
        return retval;
    }
    optimistic_yield(5000);
  }

  //Serial.printf("timeout - count[0]=%d\n", this->pin_result_count[0]);
  return PULSE2_NO_PIN;
}

void Pulse2::reset(void)
{
  for(int slot=0; slot<PULSE2_MAX_PINS; slot++)
    if (PULSE2_NO_PIN != this->pin[slot])
      detachInterrupt(digitalPinToInterrupt(this->pin[slot]));

  memset(this->pin_start_micros, 0, sizeof(this->pin_start_micros));
  memset((void*)(this->pin_result_count), 0, sizeof(this->pin_result_count));
  memset((void*)(this->pin_result_slot), 0, sizeof(this->pin_result_slot));
  memset((void*)(this->pin_result), 0, sizeof(this->pin_result));
  this->overflow_count = 0;
  this->missed_count = 0;

  for(int slot=0; slot<PULSE2_MAX_PINS; slot++) {
    if (PULSE2_NO_PIN != this->pin[slot]) {
      int pinmode;
      if (HIGH == this->direction[slot])
        pinmode = RISING;
      else
        pinmode = FALLING;
    
      std::function<void (void)> handler = std::bind(&Pulse2::handle_interrupt, this, slot, this->pin[slot], pinmode);
      attachInterrupt(digitalPinToInterrupt(this->pin[slot]), handler, pinmode);
    }
  }
}

void ICACHE_RAM_ATTR Pulse2::handle_interrupt(int slot, uint8_t pin, int mode)
{
  int newmode;
  int currentval;
  unsigned long result = micros() - this->pin_start_micros[slot];

  if (RISING == mode)
    newmode = FALLING;
  else
    newmode = RISING;

  if (0 == this->pin_start_micros[slot]) {
    this->pin_start_micros[slot] = result;
  } else {
    this->pin_result[slot][this->pin_result_slot[slot]] = result;
    this->pin_result_count[slot]++;
    if (this->pin_result_count[slot] >= PULSE2_WATCH_DEPTH) {
      this->pin_result_count[slot] = PULSE2_WATCH_DEPTH;
      this->overflow_count++;
    }
    this->pin_result_slot[slot]++;
    if (this->pin_result_slot[slot] >= PULSE2_WATCH_DEPTH)
      this->pin_result_slot[slot] -= PULSE2_WATCH_DEPTH;
    this->pin_start_micros[slot] = 0;
    this->wait_status = 1;
  }

  std::function<void (void)> handler = std::bind(&Pulse2::handle_interrupt, this, slot, pin, newmode);
  attachInterrupt(digitalPinToInterrupt(pin), handler, newmode);

  currentval = digitalRead(pin);
  if ((RISING == newmode) && (HIGH == currentval))
    this->handle_interrupt(slot, pin, newmode);
  else if ((FALLING == newmode) && (LOW == currentval))
    this->handle_interrupt(slot, pin, newmode);
}

uint8_t Pulse2::check_result(unsigned long *result)
{
  ETS_GPIO_INTR_DISABLE();
  this->wait_status=0; // clear the status while we have interrupts disabled

  for (int slot=0; slot<PULSE2_MAX_PINS; slot++) {
    int result_slot = this->pin_result_count[slot];
    if (result_slot) {
      uint8_t pin = this->pin[slot];
      result_slot = this->pin_result_slot[slot] - result_slot; // subtract count
      if (result_slot < 0)
        result_slot += PULSE2_WATCH_DEPTH;
      *result = this->pin_result[slot][result_slot];
      this->pin_result_count[slot]--;
      ETS_GPIO_INTR_ENABLE();
      return pin;
    }
  }

  ETS_GPIO_INTR_ENABLE();
  return PULSE2_NO_PIN;
}
