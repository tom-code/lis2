
```
openocd -f interface/stlink-v2.cfg -f target/stm32f1x.cfg
telnet localhost 4444

reset halt
flash write_image erase /data/blue/openmc3/opencm3-cppcomp/test1.bin 0x0800000
reset run
```
