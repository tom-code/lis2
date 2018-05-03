
#define STAV_L 0
#define STAV_H 1

struct signal_t : public component_t {

  int stav;
  const char *name;
  int port, pin;

  void setup(const char *_name, int _port, int _pin) {
#ifdef TEST
    name = _name;
#else
  port = _port;
  pin = _pin;
  if (port != -1) {
    gpio_set_mode(port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, pin);
    gpio_set(port, pin); //this controls up/down?
  }
#endif
    stav = STAV_H;
  }

  int get() {
#ifndef TEST
    if (port != -1)
      stav = (gpio_get(port, pin) && true);
#endif
    return stav;
  }
 
  void dump() {
    SIMPLE(" %s:%d\n", name, stav);
  }

  const char *get_name() {
    return name;
  }
};


struct povel_t : public component_t {
  int stav;
  const char *name;
  int pin, port;

  void setup(const char *_name, int _port, int _pin) {
#ifdef TEST
    name = _name;
#else
    port = _port;
    pin = _pin;
    if (port != -1)
      gpio_set_mode(port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, pin);
#endif
    stav = STAV_L;
  }
  
  void set(int _stav) {
    stav = _stav;
#ifndef TEST
    if (port != -1) {
      if (stav) gpio_set(port, pin); else gpio_clear(port, pin);
    }
#else
    INFO("zmena stavu povelu %s na %d", name, stav);
#endif
  }

  void dump() {
    SIMPLE(" %s:%d\n", name, stav);
  }
  const char *get_name() {
    return name;
  }

};
