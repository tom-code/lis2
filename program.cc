#include <libopencm3/stm32/gpio.h>

#include "usart.h"
#include "utils.h"

#include "component.h"
#include "signal.h"

#include <vector>

auto p_status = new povel_t();


auto s_start = new signal_t();
auto p_rdy = new povel_t();
auto s_lih = new signal_t();
auto s_lid = new signal_t();
auto s_lip = new signal_t();
auto s_lir = new signal_t();
auto p_lid = new povel_t();
auto p_lih = new povel_t();

auto s_zfz = new signal_t();
auto s_zfo = new signal_t();
auto p_fzoff = new povel_t();
auto p_fzon = new povel_t();
auto s_dfo = new signal_t();
auto s_dfz = new signal_t();

std::vector<component_t*> components {s_start, p_rdy, s_lih, s_lid, s_lip, s_lir, p_lid, p_lih, s_zfz, s_zfo, p_fzoff, p_fzon, s_dfo, s_dfz};

void dump_all() {
#if TEST
  for (auto c : components) c->dump();
    SIMPLE("\n");
#endif
}

void setup() {
  //AFIO_MAPR |= AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_OFF; // free jtag pins
  //AFIO_MAPR |= AFIO_MAPR_USART1_REMAP;


  p_status->setup("p_status", GPIOC, GPIO13);


  s_start->setup("s_start", GPIOB, GPIO8);

  p_rdy->setup("p_rdy", GPIOA, GPIO7);
  s_lih->setup("s_lih", GPIOB, GPIO15);
  s_lid->setup("s_lid", GPIOB, GPIO13);
  s_lip->setup("s_lip", GPIOB, GPIO12);
  s_lir->setup("s_lir", GPIOB, GPIO14);
  p_lid->setup("p_lid", GPIOA, GPIO1);
  p_lih->setup("p_lih", GPIOA, GPIO0);

  s_zfz->setup  ("s_zfz",   -1, 0);
  s_zfo->setup  ("s_zfo",   -1, 0);
  p_fzoff->setup("p_fzoff", -1, 0);
  p_fzon->setup ("p_fzon",  -1, 0);

  s_dfo->setup ("s_dfo",  -1, 0);
  s_dfz->setup ("s_dfz",  -1, 0);

  usart_setup();
  usart1_send("hello\n\r");
}


enum class stav_t  {ERR, START, TEST_LISU, TEST_FORMY, KONTROLA, PLNENI_FORMY};
static const char *get_stav(stav_t s) {
  switch (s) {
    case stav_t::ERR: return "ERR";
    case stav_t::START: return "START";
    case stav_t::TEST_LISU: return "TEST_LISU";
    case stav_t::TEST_FORMY: return "TEST_FORMY";
    case stav_t::PLNENI_FORMY: return "PLNENI_FORMY";
    case stav_t::KONTROLA: return "KONTROLA";
  }
  return "??";
}

stav_t stav = stav_t::ERR;
int time = 0;
void reset() {
  INFO("reset");
  time = 0;
  p_rdy->set(STAV_L);
  stav = stav_t::START;
}


void handle_start() {
  if (s_start->get() == STAV_L) {
    log("zmacknuto start -> jdeme na test lisu");
    p_rdy->set(STAV_H);
    stav = stav_t::TEST_LISU;
  }
}

void handle_test_formy_start();
void handle_test_lisu() {
  if ((s_lir->get() == STAV_L) && ((s_lid->get() == STAV_L) || (s_lih->get() == STAV_L))) {
    stav = stav_t::ERR;
    log("lis v ref i krajni poloze -> error");
    return;
  }
  if (s_lir->get() == STAV_L) {
    log("lis v ref poloze -> test formy");
    handle_test_formy_start();
    p_lid->set(STAV_H);
    p_lih->set(STAV_H);
    return;
  }
  if (s_lip->get() == STAV_H) {
    INFO("lis je pod -> lis pojede nahoru")
    p_lid->set(STAV_H);
    p_lih->set(STAV_L);
  } else {
    INFO("lis je nad -> lis pojede dolu")
    p_lid->set(STAV_L);
    p_lih->set(STAV_H);
  }
}


enum class stav_test_formy_t  {ERR, ODJISTUJI, OTEVIRAM, ZAVIRAM, ZAJISTUJI};
const char *test_formy_string(stav_test_formy_t s) {
  switch (s) {
    case stav_test_formy_t::ERR: return "ERR";
    case stav_test_formy_t::ODJISTUJI: return "ODJISTUJI";
    case stav_test_formy_t::OTEVIRAM: return "OTEVIRAM";
    case stav_test_formy_t::ZAVIRAM: return "ZAVIRAM";
    case stav_test_formy_t::ZAJISTUJI: return "ZAJISTUJI";
  }
  return "??";
}
stav_test_formy_t stav_test_formy = stav_test_formy_t::ERR;
void handle_test_formy_start() {
  log("test formy start");
  stav = stav_t::TEST_FORMY;
  stav_test_formy = stav_test_formy_t::ODJISTUJI;
}
void handle_test_formy() {
  INFO("test formy stav: %s", test_formy_string(stav_test_formy));
  if (stav_test_formy == stav_test_formy_t::ODJISTUJI) {
    if (s_zfo->get() != STAV_L) {
      p_fzoff->set(STAV_H);
      return;
    } else {
      stav_test_formy = stav_test_formy_t::OTEVIRAM;
      p_fzoff->set(STAV_L);
      return;
    }
  }
  if (stav_test_formy == stav_test_formy_t::OTEVIRAM) {
    if (s_dfo->get() != STAV_L) return;
    stav_test_formy = stav_test_formy_t::ZAVIRAM;
  }
  if (stav_test_formy == stav_test_formy_t::ZAVIRAM) {
    if (s_dfz->get() != STAV_L) return;
    stav_test_formy = stav_test_formy_t::ZAJISTUJI;
    p_fzon->set(STAV_H);
  }
  if (stav_test_formy == stav_test_formy_t::ZAJISTUJI) {
    if (s_zfz->get() != STAV_L) return;
    p_fzon->set(STAV_L);
    stav_test_formy = stav_test_formy_t::ERR;
    stav = stav_t::KONTROLA;
  }
}

void handle_kontrola() {
  if (s_zfz->get() != STAV_L) return;
  if (s_lir->get() != STAV_L) return;
  stav = stav_t::PLNENI_FORMY;
}

int tt = 0;
void tick() {
#ifndef TEST
  if ((millis() % 1000) < 100) p_status->set(STAV_H); else p_status->set(STAV_L);
#endif
  tt = tt+1;
  if ((tt % 100000) == 0) usart1_send(get_stav(stav));
  if ((tt % 100000) == 5000) usart1_send("\r\n");
  INFO("\ntick stav=%s cas=%d", get_stav(stav), time); time++;
  switch (stav) {
    case stav_t::ERR: break;
    case stav_t::START: handle_start(); break;
    case stav_t::TEST_LISU: handle_test_lisu(); break;
    case stav_t::TEST_FORMY: handle_test_formy(); break;
    case stav_t::KONTROLA: handle_kontrola(); break;
    case stav_t::PLNENI_FORMY: break;
  }
  dump_all();
}

extern "C" void my_setup() {
  setup();
  reset();
}
extern "C" void my_loop() {
  tick();
}
