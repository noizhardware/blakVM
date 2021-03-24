trenitaja fs
filesystem

max 4gb
[0, 1, 2, 3] first 4 bytes is an uint32_t containing the higher memory address available (size-1)
[filename string]['\0'][address of next file][current file contents]