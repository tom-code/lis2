#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/flash.h>

#include "usart.h"
#include "utils.h"

#include "component.h"
#include "signal.h"

#include <vector>
#include <string.h>
#include <stdlib.h>


uint32_t cas_plneni = 3000;
//auto p_status = new povel_t();
auto p_status = new blikac_t();


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
auto s_dfo = new timed_signal_t();
auto s_dfz = new signal_t();

auto pm_fot = new povel_t();
auto pm_fza = new povel_t();

std::vector<component_t*> components {p_status, s_start, p_rdy, s_lih, s_lid, s_lip, s_lir, p_lid, p_lih, s_zfz, s_zfo, p_fzoff, p_fzon, s_dfo, s_dfz, pm_fot, pm_fza};

void xflash_write(uint32_t data);
void dump_all() {
#if TEST
  for (auto c : components) c->dump();
    SIMPLE("\n");
#endif
}
void tick_all() {
  for (auto c : components) c->tick();
}

void setup() {
  //AFIO_MAPR |= AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_OFF; // free jtag pins
  //AFIO_MAPR |= AFIO_MAPR_USART1_REMAP;
  AFIO_MAPR |= AFIO_MAPR_SWJ_CFG_FULL_SWJ_NO_JNTRST; // free jtag pins


  p_status->setup("p_status", GPIOC, GPIO13);


  s_start->setup("s_start", GPIOB, GPIO8);

  p_rdy->setup("p_rdy", GPIOA, GPIO7);
  s_lih->setup("s_lih", GPIOB, GPIO15);
  s_lid->setup("s_lid", GPIOB, GPIO13);
  s_lip->setup("s_lip", GPIOB, GPIO12);
  s_lir->setup("s_lir", GPIOB, GPIO14);
  p_lid->setup("p_lid", GPIOA, GPIO1);
  p_lih->setup("p_lih", GPIOA, GPIO0);

  s_zfz->setup  ("s_zfz",   GPIOB, GPIO5);
  s_zfo->setup  ("s_zfo",   GPIOB, GPIO4);
  p_fzoff->setup("p_fzoff", GPIOA, GPIO3);
  p_fzon->setup ("p_fzon",  GPIOA, GPIO2);

  s_dfo->setup ("s_dfo",  GPIOA, GPIO9);
  s_dfz->setup ("s_dfz",  GPIOA, GPIO10);

  pm_fot->setup ("pm_fot",  GPIOB, GPIO1);
  pm_fza->setup ("pm_fza",  GPIOB, GPIO0);


  cas_plneni = *((uint32_t*)0x800f000);
  if (cas_plneni > 100000) cas_plneni = 2000;

  usart_setup();

  char buf[32];
  strcpy(buf, "\r\n hello  ");
  utoa(cas_plneni, buf+9, 10);
  strcat(buf, "\r\n");
  usart1_send(buf);
}


enum class stav_t  {ERR, START, TEST_LISU, TEST_FORMY, KONTROLA, PLNENI_FORMY, LISOVANI, VYJMUTI_VYLISKU};
static const char *get_stav(stav_t s) {
  switch (s) {
    case stav_t::ERR: return "ERR";
    case stav_t::START: return "START";
    case stav_t::TEST_LISU: return "TEST_LISU";
    case stav_t::TEST_FORMY: return "TEST_FORMY";
    case stav_t::PLNENI_FORMY: return "PLNENI_FORMY";
    case stav_t::KONTROLA: return "KONTROLA";
    case stav_t::LISOVANI: return "LISOVANI";
    case stav_t::VYJMUTI_VYLISKU: return "VYJMUTI_VYLISKU";
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

  }
  if ((s_lir->get() == STAV_L) && ((s_lid->get() == STAV_L) || (s_lih->get() == STAV_L))) {
    stav = stav_t::ERR;
    log("lis v ref i krajni poloze -> error");
    return;
  }
  if (s_lir->get() == STAV_L) {
    log("lis v ref poloze -> test formy");
    handle_test_formy_start();
    p_lid->set(STAV_L);
    p_lih->set(STAV_L);
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
stav_test_formy_t stav_test_formy = stav_test_formy_t::ERR;
/*void usart_command(char cmd) {
  char s = cmd-0x30;
  if (s > 9) return;
  stav = (stav_t)s;
  if (stav == stav_t::TEST_FORMY) stav_test_formy = stav_test_formy_t::ODJISTUJI;
  if (stav == stav_t::PLNENI_FORMY) stav_plneni_formy = stav_plneni_formy_t::START;

}*/

enum class stav_plneni_formy_t  {START, LIS_JEDE_HORE, LIS_CEKA, LIS_JEDE_DOLU};
stav_plneni_formy_t stav_plneni_formy;
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
//poslat b1
      pm_fot->set(STAV_H);
      return;
    }
  }
  if (stav_test_formy == stav_test_formy_t::OTEVIRAM) {
    if (s_dfo->get() != STAV_L) return;
//b1 off
    pm_fot->set(STAV_L);
    pm_fza->set(STAV_H);
//pockat 2 vteriny
    stav_test_formy = stav_test_formy_t::ZAVIRAM;
//poslat b0
  }

  if (stav_test_formy == stav_test_formy_t::ZAVIRAM) {
    if (s_dfz->get() != STAV_L) return;
//b0 off
    pm_fza->set(STAV_L);
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
  stav_plneni_formy = stav_plneni_formy_t::START;
}


enum class stav_lisovani_t  {DOLU, NAHORU};
stav_lisovani_t stav_lisovani;

uint32_t list_ceka_start;
void handle_plneni_formy() {
  if (stav_plneni_formy == stav_plneni_formy_t::START) {
    p_lid->set(STAV_L);
    p_lih->set(STAV_H);
    stav_plneni_formy = stav_plneni_formy_t::LIS_JEDE_HORE;
    return;
  }
  if (stav_plneni_formy == stav_plneni_formy_t::LIS_JEDE_HORE) {
    if (s_lih->get() != STAV_L) return;
    p_lid->set(STAV_H);
    p_lih->set(STAV_L);
    stav_plneni_formy = stav_plneni_formy_t::LIS_CEKA;
    list_ceka_start = millis();
    return;
  }
  if (stav_plneni_formy == stav_plneni_formy_t::LIS_CEKA) {
    if ((list_ceka_start+cas_plneni) > millis()) return;
    stav_plneni_formy = stav_plneni_formy_t::LIS_JEDE_DOLU;
  }
  if (stav_plneni_formy == stav_plneni_formy_t::LIS_JEDE_DOLU) {
    if (s_lir->get() != STAV_L) return;
    stav = stav_t::LISOVANI;
    stav_lisovani = stav_lisovani_t::DOLU;
  }
}

enum class stav_vyjmuti_t  {START, ODJISTUJI, OTEVIRAM, ZAVIRAM, ZAJISTUJI};
stav_vyjmuti_t stav_vyjmuti;
void handle_lisovani() {
  if (stav_lisovani == stav_lisovani_t::DOLU) {
    if (s_lid->get() != STAV_L) return;
    p_lih->set(STAV_H);
    p_lid->set(STAV_L);
    stav_lisovani = stav_lisovani_t::NAHORU;
  }
  if (stav_lisovani == stav_lisovani_t::NAHORU) {
    if (s_lir->get() != STAV_L) return;
    p_lih->set(STAV_L);
    p_lid->set(STAV_L);
    stav = stav_t::VYJMUTI_VYLISKU;
    stav_vyjmuti = stav_vyjmuti_t::START;
  }
}

void handle_vyjmuti() {
  if (stav_vyjmuti == stav_vyjmuti_t::START) {
    p_fzoff->set(STAV_H);
    stav_vyjmuti = stav_vyjmuti_t::ODJISTUJI;
    return;
  }
  if (stav_vyjmuti == stav_vyjmuti_t::ODJISTUJI) {
    if (s_zfo->get() != STAV_L) return;
    p_fzoff->set(STAV_L);
    stav_vyjmuti = stav_vyjmuti_t::OTEVIRAM;
    pm_fot->set(STAV_H);
    return;
  }
  if (stav_vyjmuti == stav_vyjmuti_t::OTEVIRAM) {
    if (s_dfo->get() != STAV_L) return;
    pm_fot->set(STAV_L);
    pm_fza->set(STAV_H);
    stav_vyjmuti = stav_vyjmuti_t::ZAVIRAM;
    return;
  }
  if (stav_vyjmuti == stav_vyjmuti_t::ZAVIRAM) {
    if (s_dfz->get() != STAV_L) return;
    pm_fza->set(STAV_L);
    stav_vyjmuti = stav_vyjmuti_t::ZAJISTUJI;
    return;
  }
  if (stav_vyjmuti == stav_vyjmuti_t::ZAJISTUJI) {
    if (s_zfz->get() != STAV_L) return;
    p_fzon->set(STAV_L);
    stav = stav_t::KONTROLA;
    return;
  }
}
void usart_command(char cmd) {

  if (cmd == 'z') if (cas_plneni > 100) cas_plneni-=100;
  if (cmd == 'x') if (cas_plneni < 10000) cas_plneni+=100;
  if (cmd == 'c') xflash_write(cas_plneni);

  char s = cmd-0x30;
  if (s > 9) return;
  stav = (stav_t)s;
  if (stav == stav_t::TEST_FORMY) stav_test_formy = stav_test_formy_t::ODJISTUJI;
  if (stav == stav_t::PLNENI_FORMY) stav_plneni_formy = stav_plneni_formy_t::START;
  if (stav == stav_t::LISOVANI) stav_lisovani = stav_lisovani_t::DOLU;
  if (stav == stav_t::VYJMUTI_VYLISKU) stav_vyjmuti = stav_vyjmuti_t::START;
}

void send_debug() {
  char buf[1000];
  strcpy(buf, get_stav(stav));
  if (stav == stav_t::START) {
    strcat(buf, " cas_plneni:");
    utoa(cas_plneni, buf+strlen(buf), 10);
  }
  if (stav == stav_t::VYJMUTI_VYLISKU) {
    switch (stav_vyjmuti) {
      case stav_vyjmuti_t::START: strcat(buf, " start"); break;
      case stav_vyjmuti_t::ODJISTUJI: strcat(buf, " odjistuji"); break;
      case stav_vyjmuti_t::OTEVIRAM: strcat(buf, " oteviram"); break;
      case stav_vyjmuti_t::ZAVIRAM: strcat(buf, " zaviram"); break;
      case stav_vyjmuti_t::ZAJISTUJI: strcat(buf, " zajistuji"); break;
      default : strcat(buf, " ???"); break;
    }
  }
  if (stav == stav_t::TEST_FORMY) {
    switch (stav_test_formy) {
      case stav_test_formy_t::ODJISTUJI: strcat(buf, " odjistuji"); break;
      case stav_test_formy_t::OTEVIRAM: strcat(buf, " oteviram"); break;
      case stav_test_formy_t::ZAVIRAM: strcat(buf, " zaviram"); break;
      case stav_test_formy_t::ZAJISTUJI: strcat(buf, " zajistuji"); break;
      case stav_test_formy_t::ERR: strcat(buf, " error"); break;
      default: strcat(buf, " ???"); break;
    }
  }
  if (stav == stav_t::PLNENI_FORMY) {
    switch (stav_plneni_formy) {
      case stav_plneni_formy_t::START: strcat(buf, " start"); break;
      case stav_plneni_formy_t::LIS_JEDE_HORE: strcat(buf, " jede nahoru"); break;
      case stav_plneni_formy_t::LIS_CEKA: strcat(buf, " ceka"); break;
      case stav_plneni_formy_t::LIS_JEDE_DOLU: strcat(buf, " jede_dolu"); break;
      default: strcat(buf, " ???"); break;
    }
  }
  //if (s_dfo->get() == STAV_L) strcat(buf, " aaa");
  //if (s_dfo->get() == STAV_H) strcat(buf, " bbb");
  strcat(buf, " time:");
  utoa(millis(), buf+strlen(buf), 10);
  strcat(buf, "\r\n");
  usart1_send(buf);
}

int tt = 0;
int last_millis = 0;
void tick() {
//#ifndef TEST
//  if ((millis() % 1000) < 100) p_status->set(STAV_H); else p_status->set(STAV_L);
//#endif
  p_status->set(5);
  tt = tt+1;
  int now = millis();
  if (now != last_millis) {
    //if ((now % 500) == 0) usart1_send(get_stav(stav));
    //if ((now % 500) == 200) usart1_send("\r\n");
    if ((now % 500) == 0) send_debug();
    last_millis = now;
  }
  INFO("\ntick stav=%s cas=%d", get_stav(stav), time); time++;
  switch (stav) {
    case stav_t::ERR: break;
    case stav_t::START: handle_start(); break;
    case stav_t::TEST_LISU: handle_test_lisu(); break;
    case stav_t::TEST_FORMY: handle_test_formy(); break;
    case stav_t::KONTROLA: handle_kontrola(); break;
    case stav_t::PLNENI_FORMY: handle_plneni_formy(); break;
    case stav_t::LISOVANI: handle_lisovani(); break;
    case stav_t::VYJMUTI_VYLISKU: handle_vyjmuti(); break;
  }
  dump_all();
  tick_all();
}


extern "C" void my_setup() {
  setup();
  reset();
//  xflash_write(1000);
}
extern "C" void my_loop() {
  tick();
}

