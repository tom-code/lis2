
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

struct timed_signal_t : public component_t {

  int stav;
  int curr_stav;
  int curr_time;
  const char *name;
  int port, pin;
  int up_time = 1;
  int down_time = 2000;

  void setup(const char *_name, int _port, int _pin) {
    port = _port;
    pin = _pin;
    if (port != -1) {
      gpio_set_mode(port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, pin);
      gpio_set(port, pin); //this controls up/down?
    }
    stav = STAV_H;
    curr_stav = STAV_H;
    curr_time = millis();
  }

  void tick() {
    if (port == -1) return;
    int tmp = (gpio_get(port, pin) && true);
    if (tmp != curr_stav) {
      curr_stav = tmp;
      curr_time = millis();
      return;
    }
    if (stav != curr_stav) {
      if (curr_stav == STAV_H && ((curr_time+up_time) < millis())) stav = curr_stav;
      if (curr_stav == STAV_L && ((curr_time+down_time) < millis())) stav = curr_stav;
    }
  }

  int get() {
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

struct blikac_t : public component_t {
  int stav;
  int max = 10;
  const char *name;
  int pin, port;

  void setup(const char *_name, int _port, int _pin) {
    port = _port;
    pin = _pin;
    if (port != -1)
      gpio_set_mode(port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, pin);
    stav = STAV_L;
  }
  
  void set(int _stav) {
    stav = _stav;
    max = stav+2;
  }

  void tick() {
    int slot = (millis()/200) % (max*2);

    if ((slot % 2) == 0) {
      gpio_set(port, pin);
      return;
    } else {
    //gpio_set(port, pin);
      if (slot<(stav*2)) gpio_clear(port, pin);
    }
  }

  void dump() {
    SIMPLE(" %s:%d\n", name, stav);
  }
  const char *get_name() {
    return name;
  }

};
