#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>

#include <string.h>




static char to_send[100]= "";

void usart_setup() {
//  AFIO_MAPR |= AFIO_MAPR_USART1_REMAP;

  to_send[0] = 0;

  //rcc_periph_clock_enable(RCC_GPIOB);
  //rcc_periph_clock_enable(RCC_AFIO);
  rcc_periph_clock_enable(RCC_USART1);

  AFIO_MAPR |= AFIO_MAPR_USART1_REMAP;


  nvic_enable_irq(NVIC_USART1_IRQ);
  gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_RE_TX);
  gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RE_RX); 

  //gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);
  //gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RX); 

  usart_set_baudrate(USART1, 9600);
  usart_set_databits(USART1, 8);
  usart_set_stopbits(USART1, USART_STOPBITS_1);
  usart_set_parity(USART1, USART_PARITY_NONE);
  usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
  usart_set_mode(USART1, USART_MODE_TX_RX); 

  USART_CR1(USART1) |= USART_CR1_RXNEIE;
  usart_enable(USART1);
}

void usart1_send(const char *data) {
  strcpy(to_send, data);
  USART_CR1(USART1) |= USART_CR1_TXEIE;
}

extern "C" void usart1_isr(void)
{
  /* Check if we were called because of RXNE. */
  if (((USART_CR1(USART1) & USART_CR1_RXNEIE) != 0) && ((USART_SR(USART1) & USART_SR_RXNE) != 0)) {

    /*data = */usart_recv(USART1);
    usart1_send("don't disturb\n\r");
  }

  /* Check if we were called because of TXE. */
  if (((USART_CR1(USART1) & USART_CR1_TXEIE) != 0) && ((USART_SR(USART1) & USART_SR_TXE) != 0)) {

    if (strlen(to_send) > 0) {
      usart_send(USART1, to_send[0]);
      memmove(to_send, to_send+1, strlen(to_send)+1);
    }

    /* Disable the TXE interrupt as we don't need it anymore. */
    if (strlen(to_send) == 0) USART_CR1(USART1) &= ~USART_CR1_TXEIE;
  }
}


