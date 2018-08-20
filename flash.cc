

#include <libopencm3/stm32/flash.h>

void xflash_write(uint32_t data) {

  uint32_t addr = 0x800f000;
  uint32_t page = addr;
  if (page & 0x800) page -= page & 0x800;
  
  flash_unlock();
  flash_erase_page(page);
  flash_get_status_flags();
  //uint32_t data = 0x12345678;
  flash_program_word(addr, data);
  flash_program_word(addr+4, data);
  flash_program_word(addr+8, data);
  flash_get_status_flags();
  
}
