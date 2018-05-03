
#ifndef TEST
void usart_setup();
void usart1_send(const char *data);
#else
  #include <stdio.h>
  void usart_setup() {}
  void usart1_send(const char *data) {printf("USART: %s", data);}
#endif
