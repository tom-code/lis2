#include "utils.h"
#include "component.h"
#include "signal.h"
#include <vector>
#include <string.h>

int board_uptime_millis = 0;

void setup();
void reset();
void tick();

//extern signal_t *s_start;
//extern povel_t *p_rdy;
//extern signal_t *s_lih;
//extern signal_t *s_lid;
//extern signal_t *s_lip;
//extern signal_t *s_lir;
//extern povel_t *p_lid;
//extern povel_t *p_lih;
//
//extern signal_t *s_zfo;
//extern signal_t *s_dfo;
//extern signal_t *s_dfz;
//extern signal_t *s_zfz;


extern std::vector<component_t*> components;

void kontrola(const char *name, int stav, int line) {
  printf("test kontroluje %s=%d\n", name, stav);
  for (auto c : components) {
    if (strcmp(c->get_name(), name) == 0) {
      povel_t *s = dynamic_cast<povel_t*>(c);
      if (s->stav != stav) {
        printf("CHYBA kontrola parametru %s radek %d\n", name, line);
      }
      return;
    }
  }
  printf("neznamy parameter %s\n", name);
}
#define K(povel, _stav) kontrola(povel, _stav, __LINE__);

void SET(const char *name, int stav) {
  for (auto c : components) {
    if (strcmp(c->get_name(), name) == 0) {
      printf("test nastavuje %s na %d\n", name, stav);
      signal_t *s = dynamic_cast<signal_t*>(c);
      s->stav = stav;
      return;
    }
  }
  printf("neznamy parameter %s\n", name);
}

int main() {
  setup();
  reset();

  tick();
  SET("s_start", STAV_L);

  //nastartovano vstupy = H
  tick();
  tick();

  K("p_lid", STAV_H); K("p_lih", STAV_L)

  SET("s_lip", STAV_L);
  tick();

  K("p_lid", STAV_L); K("p_lih", STAV_H)
  tick();

  SET("s_lir", STAV_L);
  tick();
  K("p_lid", STAV_H); K("p_lih", STAV_H)
  tick();
  tick();

  SET("s_zfo", STAV_L);
  tick();
  tick();

  SET("s_dfo", STAV_L);
  tick();
  tick();


  SET("s_dfz", STAV_L);
  tick();
  tick();

 
  SET("s_zfz", STAV_L);
  tick();
  tick();
  tick();
  tick();
}
