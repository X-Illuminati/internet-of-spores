#ifndef _PULSE2_H_
#define _PULSE2_H_

#include <stdint.h>


// max number of pins that can be watched
#define PULSE2_MAX_PINS    6
// number of pulses that will be stored for each pin
#define PULSE2_WATCH_DEPTH 6
// invalid pin number
#define PULSE2_NO_PIN      0xff


// NOTE: none of this is threadsafe...
class Pulse2
{
public:
  //constructor
  Pulse2(void);
  //destructor - automatically deregisters any pins
  ~Pulse2(void);

  //add or overwrite a watch for pin to pulse in direction
  //returns false if PULSE2_MAX_PINS have already been registered
  bool register_pin(uint8_t pin, uint8_t direction);
  void unregister_pin(uint8_t pin);

  //block (with timeout) until one of the pins is triggered
  //returns pin number on success (with pulse length written to result)
  //returns PULSE2_NO_PIN on timeout
  uint8_t watch(unsigned long *result, unsigned long timeout=1000000L);

  //reset the state machines and throw out any existing results
  //does not unregister any pins
  void reset(void);

private:
  volatile int wait_status;
  unsigned long pin_start_micros[PULSE2_MAX_PINS];
  volatile int pin_result_count[PULSE2_MAX_PINS];
  volatile int pin_result_slot[PULSE2_MAX_PINS];
  volatile unsigned long pin_result[PULSE2_MAX_PINS][PULSE2_WATCH_DEPTH];
  volatile unsigned long overflow_count;
  volatile unsigned long missed_count;

  uint8_t pin[PULSE2_MAX_PINS];
  uint8_t direction[PULSE2_MAX_PINS];

  //helper called by interrupt closure -
  //arduino interrupts must be void (*)(void)
  void handle_interrupt(int slot, uint8_t pin, int mode);

  //helper called by watch to parse the result structures above
  uint8_t check_result(unsigned long *result);
};

#endif /* _PULSE2_H_ */
