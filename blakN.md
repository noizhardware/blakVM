New
- easy port on low-resource MCUs
- fast native ttl or fpga implementation (no asic)

- 128x64 OLED screen >> 1024bytes 1bit display
- tiles VRAM: 8x8 tiles:
     per coprire 1k screen sarebbero 128tiles (16x8)
     facciamo che ne ho 64 >> 512bytes -> 6bits to address them + 2bits extra for flipping and colorinverting...
     128bytes to address -> tot **640bytes**
- se uso lo screen solo come char screen, 8x8font, 1byte per ogni char, sono 128bytes
     - each byte: 7bits ascii + 1bit colorinverting

- registers: 48x 8bytes -> tot **384bytes**
     addressed by name(4bytes max) or by number(6bits)
- memory: **512bytes**

- audio buffer size? (1bit audio)
     
tot 1.5kbytes(1536 bytes)
     
     
- keyboard input interrupt