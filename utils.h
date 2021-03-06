
#ifdef TEST
#include <stdio.h>

inline void log(const char *msg) {
  printf("LOG: %s\n", msg);
}

#define INFO(...) printf(__VA_ARGS__) ; printf("\n");
#define SIMPLE(...) printf(__VA_ARGS__) ;

#else
#define INFO(...) ;
#define SIMPLE(...) ;
#define log(...) ;

#endif

extern uint32_t board_uptime_millis;

inline uint32_t millis() { return board_uptime_millis; }

