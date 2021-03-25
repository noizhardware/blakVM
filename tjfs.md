trenitaja fs
filesystem

[0] type (1byte)

## type A [0x00]
- **max 4gb**
- [1, 2, 3, 4] uint32_t containing the higher memory address available (size-1)
- [name of this filesystem - string]['\0']
- [filename string]['\0'][size of file][file contents]

- file gets deleted:
  - [freemem tag][size of freemem, including the freemem tag]
  - [size of freemem, including the freemem tag] is 4bytes >> min freemem = 5bytes

- special freemem tags:
  - [freemem tag - 4bytes] indicates this and the next 3bytes are free
  - [freemem tag - 3bytes] indicates this and the next 2bytes are free
  - [freemem tag - 2bytes] indicates this and the next byte are free
  - [freemem tag - 1byte] indicates a single free byte. foreveralone.

## type B [0x01] - smol
- use for eeprom, **max 64k**
- [1, 2] uint16_t containing the higher memory address available (size-1)
- so the header occupies only 3bytes : [type][maxmem MSB][maxmem LSB]
- then directly the files:
  - [filename string]['\0'][size of file][file contents]

- file gets deleted:
  - [freemem tag][size of freemem, including the freemem tag]
  - [size of freemem, including the freemem tag] is 2bytes >> min freemem = 3bytes

- special freemem tags:
  - [freemem tag - 2bytes] indicates this and the next byte are free
  - [freemem tag - 1byte] indicates a single free byte. foreveralone.