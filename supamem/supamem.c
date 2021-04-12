/*** DOCU
“A clear conscience is the sure sign of a bad memory.”
― Mark Twain

  at the moment I'm using uint16_t absolute addresses, so I can map a total physical memory of MAX 64k
     see MEM_ADDRESS_BYTES variable #define
  
  pools(array) = [pool address, pool address, pool address, ....]
  (pool address) -> [blockSize, blocksQty, statuses, block, block, block, ...]
  (allocation address) -> [(MEM_ADDRESS_BYTES)allocation size, actual usable mem space!]
  
  I can request to allocate a pool of memory blocks of X size each
  poolid_t makePool(const memaddr_t startAddress, const memsize_t blockSize, const memsize_t blocksQty);
     the function, if successful, will add the address of the
     newly created pool in the array "pools" and will return the array index "poolId"

     supamem can be used with zero fragmentation risk, if you only request allocations of blockSize multiples:
     memsize_t myBlockSize = 4;
     memaddr_t myVar;
     poolid_t myPoolId = makePool(anyAddress, myBlockSize, 50); -- 50x 4b blocks >> 200b tot
     myVar = allocOnPool(anIntValueHere * myBlockSize, myPoolId);
     
     - for livecoding:
     you should declare a pool at the beginning of the program, encompassing all the available memory 
     block size should be carefully chosen
     
     - interesting: https://stackoverflow.com/questions/46298257/does-using-heap-memory-malloc-new-create-a-non-deterministic-program
     - suuuupa interesting: https://github.com/pavel-kirienko/o1heap
*/

/*** TODO:
     - blockid_t getFreeBlocks(poolid_t poolId) tells how many free blocks in pool
     - deterministic? do I know how long does each operation take to complete? is it constant?
          need to calculate big O's of supaMalloc and supaFree (also supaPool, but less important)
     - now I'm asking(and writing to the pool header): (blockSize + blocksQty), another possibility would be (poolSize, blocksQty)
          PRO : immediately know the total size of the pool
          CON: poolsize might not be divisible by blocksQty
     - from https://www.embedded.com/deterministic-dynamic-memory-allocation-fragmentation-in-c-c/
          The idea is to define a series of partition pools with block sizes in a geometric progression; e.g. 32, 64, 128, 256 bytes. A malloc() function may be written to deterministically select the correct pool to provide enough space for a given allocation request.
          This approach takes advantage of the deterministic behavior of the partition allocation API call, the robust error handling (e.g. task suspend) and the immunity from fragmentation offered by block memory.
     - supaMalloc - supaFree >> for embedded-friendly, all-memory allocation (you need to define the pool accordingly, before doing supaAlloc-free)
     - make so I can pass a MAX and min block size at pool creation, so eg. I can decide i want (4, 16) >> only 4b, 8b and 16b blocks will be created
     - to display errors, create and use a throwError function, so the output channel for stderr can change depending on the implementation
*/

#define SUPAMEM_VERSION "2021d12-2102"

#define SUPAMEM_DEBUG

/*** DEFINES */
     /*#define FAIL 0
     #define SUCCESS 1*/

     #define MEM_SIZE 1024
     
     #if MEM_SIZE < 256
          #define MEM_ADDRESS_BYTES 1
     #elif MEM_SIZE >= 256 && MEM_SIZE < 65536
          #define MEM_ADDRESS_BYTES 2 /* unit: bytes used to address physical memory : 2bytes(16bits) >> can address MAX 2^16 = 65536 = 64kbytes of physical memory */
     #elif MEM_SIZE >= 65536 && MEM_SIZE < 4294967296
          #define MEM_ADDRESS_BYTES 4
     #endif
     
     #define MAX_POOLS 8
     #define BLOCK_STATUS_BITS 2
     
     /* block statuses: */
     #define BLOCK_EMPTY 0
     #define BLOCK_FULL 1
     #define BLOCK_NEXT 2 /* contains address of where the allocation continues */
     /* 4 ??? */
     
/* DEFINES end. */

/*** INCLUDES */
#define _POSIX_C_SOURCE 199309L
     #include <stdio.h>
     #include <stdint.h>
     #include <math.h>
     #include <time.h>
     #include <locale.h>
     
     #include "bitty.h"
     #include "baomath.h"
/* INCLUDES end. */

/*** TYPEDEFS */
     #if MEM_ADDRESS_BYTES == 1
          typedef uint16_t memsize_t; /* 256 bytes max mem size */
     #elif MEM_ADDRESS_BYTES == 2
          typedef uint16_t memsize_t; /* 65535 bytes (64kb) max mem size */
     /* TODO: #elif MEM_ADDRESS_BYTES == 3 */
     #elif MEM_ADDRESS_BYTES == 4
          typedef uint32_t memsize_t; /* 4,294,967,296 bytes (4Gb) max mem size */
     /* TODO: #elif MEM_ADDRESS_BYTES == more than 4... */
     #endif
     
     typedef memsize_t memaddr_t;
     typedef uint8_t poolid_t; /* TODO: same as MEM_ADDRESS_BYTES, but with MAX_POOLS */
     typedef uint16_t blockid_t;
     typedef uint8_t blockstatus_t;/* actually using only 2 bits >> [0, 1, 2, 3] */
     
     /*typedef uint8_t outcome_t;*/ /* to manage fail/success */
/* TYPEDEFS end. */

/*** GLOBALS */
     uint8_t mem[MEM_SIZE];
     memaddr_t memPtr=0;
     memsize_t memFree=MEM_SIZE;
     memaddr_t pools[MAX_POOLS];/* index of all created pools, containing addresses of the pools */
     
     struct timespec supamem_start, supamem_end;
     double supamem_time_spent;
/* GLOBALS end. */

/*** FUNCTION DECLARATIONS */
     poolid_t makePool(const memaddr_t startAddress, const memsize_t blockSize, const memsize_t blocksQty);
     
     memaddr_t allocOnPool(const memsize_t size, const poolid_t poolId);
     /*void freeOnPool(const memaddr_t allocAddr, const poolid_t poolId);*/
     memsize_t getAllocSize(const memaddr_t memAddr);
     
     static __inline__ memsize_t supaPool(const memaddr_t addr, const memsize_t size, memaddr_t* supaPoolAddr);
     static __inline__ memsize_t supaMalloc(const memsize_t size, const memaddr_t supaPoolAddr, memaddr_t* blockAddr);
     
     blockstatus_t twoBitsfromByte(const uint8_t byte, const uint8_t bitsPos);
     uint8_t setTwoBitsOnByte(const uint8_t byte, const uint8_t bitsPos, const uint8_t bits);
     
     blockstatus_t getStatus(const poolid_t poolId, const blockid_t blockNum);
     void setStatus(const poolid_t poolId, const blockid_t blockNum, const blockstatus_t status);
     
     memsize_t getBlockSize(const poolid_t poolId);
     uint16_t getBlocksQty(const poolid_t poolId);
     memsize_t getFreeMem(const poolid_t poolId);
     
     static __inline__ void setBitOnAddr(const memaddr_t addr, const uint32_t bitId, const uint8_t bit);
     static __inline__ uint8_t getBitFromAddr(const memaddr_t addr, const uint32_t bitId);
     static __inline__ uint16_t getUint16FromAddr(const memaddr_t addr);
     
     static __inline__ uint16_t getUint16FromPtr(const memaddr_t addr);
     
     static __inline__ void setUint16OnAddr(const memaddr_t addr, const uint16_t val);
     
          
     void showmem();
     void showparts();
     
/* FUNCTION DECLARATIONS end. */

/*** MAIN */
int main(){
     /*poolid_t myPool;*/
     memaddr_t mySupaPool = 66;
     memaddr_t myMem;
     uint16_t* my16BitPtr;
     memaddr_t myMem8; /* note: it's good practice to embed variable size in their name */
     memaddr_t myMem16;
     memaddr_t myMem32;
     
     
     memaddr_t myMemDual16a;
     memaddr_t myMemDual16b;
     
     printf("== SUPAMEM v. %s==\n", SUPAMEM_VERSION);
     setlocale(LC_NUMERIC, "");

     /*showmem();*/

     /*clock_gettime(CLOCK_REALTIME, &supamem_start);
myPool = makePool(64, 4, 37);
     clock_gettime(CLOCK_REALTIME, &supamem_end);
     supamem_time_spent = ((supamem_end.tv_sec - supamem_start.tv_sec) * 1000000000) +
                    (supamem_end.tv_nsec - supamem_start.tv_nsec);
     printf(">>>>>>>>>>>>makePool took %'.0f nsec\n", supamem_time_spent);

     showmem();
     showparts();

     printf("free mem in pool[%u]: %ub in [%ux]of[%ux] [%ub]blocks\n", myPool, getFreeMem(myPool), getFreeMem(myPool)/getBlockSize(myPool), getBlocksQty(myPool), getBlockSize(myPool));

 
     clock_gettime(CLOCK_REALTIME, &supamem_start);
allocOnPool(9, myPool);
     clock_gettime(CLOCK_REALTIME, &supamem_end);
     supamem_time_spent = ((supamem_end.tv_sec - supamem_start.tv_sec) * 1000000000) +
                    (supamem_end.tv_nsec - supamem_start.tv_nsec);
     printf(">>>>>>>>>>>>allocOnPool took %'.0f nsec\n", supamem_time_spent);

     printf("free mem in pool[%u]: %ub in [%ux]of[%ux] [%ub]blocks\n", myPool, getFreeMem(myPool), getFreeMem(myPool)/getBlockSize(myPool), getBlocksQty(myPool), getBlockSize(myPool));
     showparts();
     showmem();
     
     
     clock_gettime(CLOCK_REALTIME, &supamem_start);
allocOnPool(23, myPool);
     clock_gettime(CLOCK_REALTIME, &supamem_end);
     supamem_time_spent = ((supamem_end.tv_sec - supamem_start.tv_sec) * 1000000000) +
                    (supamem_end.tv_nsec - supamem_start.tv_nsec);
     printf(">>>>>>>>>>>>allocOnPool took %'.0f nsec\n", supamem_time_spent);
     
     printf("free mem in pool[%u]: %ub in [%ux]of[%ux] [%ub]blocks\n", myPool, getFreeMem(myPool), getFreeMem(myPool)/getBlockSize(myPool), getBlocksQty(myPool), getBlockSize(myPool));
     showparts();
     showmem();*/
     
     clock_gettime(CLOCK_REALTIME, &supamem_start);
if(supaPool(0, 256, &mySupaPool)){ printf(">> pool creation successful at [%u]addr\n", mySupaPool);};
     clock_gettime(CLOCK_REALTIME, &supamem_end);
     supamem_time_spent = ((supamem_end.tv_sec - supamem_start.tv_sec) * 1000000000) +
                    (supamem_end.tv_nsec - supamem_start.tv_nsec);
     printf(">>>>>>>>>>>>supaPool took %'.0f nsec\n", supamem_time_spent);
     
     clock_gettime(CLOCK_REALTIME, &supamem_start);
if(supaMalloc(4, mySupaPool, &myMem)){ printf(">> allocation successful at [%u]addr\n", myMem);};
     clock_gettime(CLOCK_REALTIME, &supamem_end);
     supamem_time_spent = ((supamem_end.tv_sec - supamem_start.tv_sec) * 1000000000) +
                    (supamem_end.tv_nsec - supamem_start.tv_nsec);
     printf(">>>>>>>>>>>>supaMalloc took %'.0f nsec\n", supamem_time_spent);
     
     
     /*mem[myMem] = 99;*/
     *((uint16_t*)(&mem[myMem]))=50000; /* using absolute physical address */
     printf(">> should be 50000: %u\n", getUint16FromPtr(myMem));
     
     my16BitPtr = (uint16_t*)(&mem[myMem]); /* also using absolute physical address */
     *my16BitPtr = 12000;
     printf(">> should be 12000: %u\n", *my16BitPtr);
     printf(">> my16BitPtr size is %lub\n", sizeof(my16BitPtr));/* pointer is bigger than stored value, avoid this, at least for embedded use */
     
     setUint16OnAddr(myMem, 64000); /* using custom assign function <<<<< BEST SOLUTION */
     printf(">> should be 64000: %u\n", getUint16FromAddr(myMem));
     
     setUint16OnAddr(myMem+2, 44000); /* using custom assign function */
     printf(">> should be 44000: %u\n", getUint16FromAddr(myMem+2));
     

     clock_gettime(CLOCK_REALTIME, &supamem_start);
if(supaMalloc(4, mySupaPool, &myMemDual16a)){ printf(">> allocation successful at [%u]addr\n", myMemDual16a);};
     clock_gettime(CLOCK_REALTIME, &supamem_end);
     supamem_time_spent = ((supamem_end.tv_sec - supamem_start.tv_sec) * 1000000000) +
                    (supamem_end.tv_nsec - supamem_start.tv_nsec);
     printf(">>>>>>>>>>>>supaMalloc took %'.0f nsec\n", supamem_time_spent);
     
     myMemDual16b=myMemDual16a+2;
     setUint16OnAddr(myMemDual16a, 18000);
     setUint16OnAddr(myMemDual16b, 17000);
     printf(">> should be 18000: %u\n", getUint16FromAddr(myMemDual16a));
     printf(">> should be 17000: %u\n", getUint16FromAddr(myMemDual16b));


     clock_gettime(CLOCK_REALTIME, &supamem_start);
if(supaMalloc(8, mySupaPool, &myMem8)){ printf(">> allocation successful at [%u]addr\n", myMemDual16a);};
     clock_gettime(CLOCK_REALTIME, &supamem_end);
     supamem_time_spent = ((supamem_end.tv_sec - supamem_start.tv_sec) * 1000000000) +
                    (supamem_end.tv_nsec - supamem_start.tv_nsec);
     printf(">>>>>>>>>>>>supaMalloc took %'.0f nsec\n", supamem_time_spent);

     clock_gettime(CLOCK_REALTIME, &supamem_start);
if(supaMalloc(16, mySupaPool, &myMem16)){ printf(">> allocation successful at [%u]addr\n", myMemDual16a);};
     clock_gettime(CLOCK_REALTIME, &supamem_end);
     supamem_time_spent = ((supamem_end.tv_sec - supamem_start.tv_sec) * 1000000000) +
                    (supamem_end.tv_nsec - supamem_start.tv_nsec);
     printf(">>>>>>>>>>>>supaMalloc took %'.0f nsec\n", supamem_time_spent);

     clock_gettime(CLOCK_REALTIME, &supamem_start);
if(supaMalloc(32, mySupaPool, &myMem32)){ printf(">> allocation successful at [%u]addr\n", myMemDual16a);};
     clock_gettime(CLOCK_REALTIME, &supamem_end);
     supamem_time_spent = ((supamem_end.tv_sec - supamem_start.tv_sec) * 1000000000) +
                    (supamem_end.tv_nsec - supamem_start.tv_nsec);
     printf(">>>>>>>>>>>>supaMalloc took %'.0f nsec\n", supamem_time_spent);     
     
     
     showmem();

     
     return 0;
}
/* MAIN end. */

/*** FUNCTION DEFINITIONS */

static __inline__ void setUint16OnAddr(const memaddr_t addr, const uint16_t val){
     mem[addr]=getHiByte(val);
     mem[addr+1]=getLoByte(val);
}

static __inline__ memsize_t supaPool(const memaddr_t addr, const memsize_t size, memaddr_t* supaPoolAddr){ /* make a pool of blocks */
     /* 4 8 16 32 bytes blocks*/
     /* with a 2-bytes size descriptor I can index a max of 1Mb of physical memory:
          (65536x)[4b] + (32768x)[8b] + (16384x)[16b] + (8192x)[32] >> 1048576b(1024kb)(1Mb)
     */

     if(size<71){ /* minimum pool size: (memStartAddr)(block4 + block8 + block16 + block32)+(pool size)+(status bits) >> (2)+(4 + 8 + 16 + 32)+(8)+(1) bytes */
          /* cacca: if I try to create a 69b pool, it will be missing the 32b block... but maybe that's ok, 1 of each size is NOT a good pool! */
          printf("::==pool size too small to be SUPA\n");
          return 0;
     }
     else if((addr+size)>MEM_SIZE){
          printf("::==not enuff space in mem\n");
          return 0;
     }
     else{/* actual function body */
          memsize_t block4Qty = floor(floor((float)(size-11)/4.)/4.);/* a quarter of how many 4b blocks can fit in "size"(already excluding 8b for blocksQty's, 1b for memStartAddr, and also 1b for statuses, as I will always use at least 1b for statuses!) */
          memsize_t block8Qty = floor((float)block4Qty/2.);/* a quarter of how many 8b blocks */
          memsize_t block16Qty = floor((float)block8Qty/2.);/* a quarter of how many 16b blocks */
          memsize_t block32Qty = floor((float)block16Qty/2.);/* a quarter of how many 32b blocks */
          memsize_t statusBytes = ceil((float)(block4Qty+block8Qty+block16Qty+block32Qty)/8.);/* how many status bytes are needed to keep track of all blocks */
          uint8_t cycle = 0;
          uint16_t i;
          
          /* here I start removing blocks IF the total allocation size exceeds the requested size */
          /* cacca: I can find a better way to choose what blocksize to eliminate to make space for metadata+statuses */
          while((block4Qty*4 + block8Qty*8 + block16Qty*16 + block32Qty*32 + 10 + statusBytes)>size){
               #ifdef SUPAMEM_DEBUG
                    printf("::[%u]tot is: %ub >> ", cycle, block4Qty*4 + block8Qty*8 + block16Qty*16 + block32Qty*32 + 10 + statusBytes);
                    #endif
               if(cycle==0){
                    #ifdef SUPAMEM_DEBUG
                         printf("-4\n");
                         #endif
                    block4Qty--;
                    statusBytes = ceil((float)(block4Qty+block8Qty+block16Qty+block32Qty)/8.);
                    cycle++;
               }
               else if(cycle==1){
                    #ifdef SUPAMEM_DEBUG
                         printf("-8\n");
                         #endif
                    block8Qty--;
                    statusBytes = ceil((float)(block4Qty+block8Qty+block16Qty+block32Qty)/8.);
                    cycle++;
               }
               else if(cycle==2){
                    #ifdef SUPAMEM_DEBUG
                         printf("-16\n");
                         #endif
                    block16Qty--;
                    statusBytes = ceil((float)(block4Qty+block8Qty+block16Qty+block32Qty)/8.);
                    cycle++;
               }
               else if(cycle==3){
                    #ifdef SUPAMEM_DEBUG
                         printf("-32\n");
                         #endif
                    block32Qty--;
                    statusBytes = ceil((float)(block4Qty+block8Qty+block16Qty+block32Qty)/8.);
                    cycle=0;
               }
          }
          #ifdef SUPAMEM_DEBUG
               printf("==supaPool:: [%ub]request at [%u]addr >> [%ux]block4Qty [%ux]block8Qty [%ux]block16Qty [%ux]block32Qty\n  totUsable[%ub] memStartAddr[2b] blockQtys[8b] (%ux)statuses[%ub](%uB left) >> tot[%ub] >> [%ub]less than requested >> [%ub]lost in the void",
                    size, addr,
                    block4Qty, block8Qty, block16Qty, block32Qty,
                    block4Qty*4 + block8Qty*8 + block16Qty*16 + block32Qty*32,
                    block4Qty + block8Qty + block16Qty + block32Qty,
                    statusBytes,
                    statusBytes*8-(block4Qty + block8Qty + block16Qty + block32Qty),
                    block4Qty*4 + block8Qty*8 + block16Qty*16 + block32Qty*32 + 10 + statusBytes,
                    size-(block4Qty*4 + block8Qty*8 + block16Qty*16 + block32Qty*32),
                    size-(block4Qty*4 + block8Qty*8 + block16Qty*16 + block32Qty*32 + 10 + statusBytes)
               );
               if(size-(block4Qty*4 + block8Qty*8 + block16Qty*16 + block32Qty*32 + 10 + statusBytes)==0){
                    printf(" >> ::PERFECT!::");/* nothing was lost in the void! */
               }
               printf("\n");
               printf("==memStartAddr:[%u]\n", addr+10+statusBytes);
               #endif
               /* TODO: I can compare bytes lost in the void and status BITS left, and try to reclaim part/all of that space */
          
          /* TODO: often using only one of the two bytes for each size-data;
             it'd be possible to reclaim those bytes using a status-byte, 4x 2-bits blocks, to tell how many bytes is big each size-data
             example: (this is the best alloc I can make before needing more than 256bytes to store the qty of block4s)
             ==supaPool:: [4110b]request at [0]addr >> [255x]block4Qty [127x]block8Qty [63x]block16Qty [31x]block32Qty
               totUsable[4036b] metadata[8b] (476x)statuses[60b](4B left) >> tot[4104b] >> [74b]less than requested >> [6b]lost in the void
             if I make a bigger one, like 4200b;
             ==supaPool:: [4200b]request at [0]addr >> [261x]block4Qty [130x]block8Qty [64x]block16Qty [31x]block32Qty
               totUsable[4100b] metadata[8b] (486x)statuses[61b](2B left) >> tot[4169b] >> [100b]less than requested >> [31b]lost in the void
             - normal size-data:
               uint16 uint16 uint16 uint16 >> 8b
             - flagged size-data:
               status_byte uint16 uint8 uint8 uint8 >> 6b LoL how shitty! maybe it's not worth implementing this optimization...
             - on embedded, using a 1024b pool:
               ==supaPool:: [1024b]request at [0]addr >> [63x]block4Qty [31x]block8Qty [15x]block16Qty [8x]block32Qty
                 totUsable[996b] metadata[8b] (117x)statuses[15b](3B left) >> tot[1019b] >> [28b]less than requested >> [5b]lost in the void
               * normal: 8b
               * flagged: status uint8 uint8 uint8 >> 5b ... still shitty LoL
          */
          
          /* first 2b is a pointer to actual memory start */
          mem[addr]=getHiByte(addr+10+statusBytes);
          mem[addr+1]=getLoByte(addr+10+statusBytes);
          /* writing size metadata */
          mem[addr+2]=getHiByte(block4Qty);
          mem[addr+3]=getLoByte(block4Qty);
          mem[addr+4]=getHiByte(block8Qty);
          mem[addr+5]=getLoByte(block8Qty);
          mem[addr+6]=getHiByte(block16Qty);
          mem[addr+7]=getLoByte(block16Qty);
          mem[addr+8]=getHiByte(block32Qty);
          mem[addr+9]=getLoByte(block32Qty);
          /* writing status bits; writing all 0's, all blocks are empty right now */
          for(i=0;i<statusBytes;i++){
               mem[addr+10+i]=B00000000;
          }
          
          *supaPoolAddr = addr;
          return block4Qty*4 + block8Qty*8 + block16Qty*16 + block32Qty*32;/* return total usable allocated memory, in bytes */
     }/* actual function body */
}/* supaPool */

static __inline__ uint16_t getUint16FromPtr(const memaddr_t addr){ /* this is using LO-HI byte order (little-endian), to read standard uint16_t's */
     return makeUint16(mem[addr+1], mem[addr]);
}
static __inline__ uint16_t getUint16FromAddr(const memaddr_t addr){ /* this is using HI-LO byte order (big-endian), that's my standard :) */
     return makeUint16(mem[addr], mem[addr+1]);
}

static __inline__ void setBitOnAddr(const memaddr_t addr, const uint32_t bitId, const uint8_t bit){/* sets the (bitId)th bit from addr in mem[] */
     memaddr_t byteAddr = addr + ceil((float)(bitId+1)/8.) - 1;
     uint8_t bitIdOnByte = bitId % 8;
     mem[byteAddr] = (bit) ? (mem[byteAddr] | (B10000000 >> bitIdOnByte)) : (mem[byteAddr] & ~(B10000000 >> bitIdOnByte));
     /* unittests:
     memWipe();
     mem[512]=B10010011;
     mem[513]=B11011101;
     setBitOnAddr(512, 0, 0);
          assert(mem[512], B00010011);
     setBitOnAddr(512, 0, 1);
          assert(mem[512], B10010011);
     setBitOnAddr(512, 7, 0);
          assert(mem[512], B10010010);
     setBitOnAddr(512, 8, 0);
          assert(mem[512], B10010011);
          assert(mem[513], B01011101);
     */
}

static __inline__ uint8_t getBitFromAddr(const memaddr_t addr, const uint32_t bitId){/* gets the (bitId)th bit from addr in mem[] */
     memaddr_t byteAddr = addr + ceil((float)(bitId+1)/8.) - 1;
     uint8_t bitIdOnByte = bitId % 8;
     return (mem[byteAddr] & (B10000000 >> bitIdOnByte))!=0;
     /* unittest:
     memWipe();
     mem[50] = B10110110;
     mem[51] = B11010010;
     mem[52] = B01011110;
     assert(getBitFromAddr(50, 0), 1);
     assert(getBitFromAddr(50, 1), 0);
     assert(getBitFromAddr(50, 2), 1);
     assert(getBitFromAddr(50, 3), 1);
     assert(getBitFromAddr(50, 4), 0);
     assert(getBitFromAddr(50, 5), 1);
     assert(getBitFromAddr(50, 6), 1);
     assert(getBitFromAddr(50, 7), 0);
     assert(getBitFromAddr(50, 8), 1);
     assert(getBitFromAddr(50, 9), 1);
     assert(getBitFromAddr(50, 10), 0);
     assert(getBitFromAddr(50, 11), 1);
     assert(getBitFromAddr(50, 12), 0);
     assert(getBitFromAddr(50, 13), 0);
     assert(getBitFromAddr(50, 14), 1);
     assert(getBitFromAddr(50, 15), 0);
     assert(getBitFromAddr(50, 16), 0);
     assert(getBitFromAddr(50, 17), 1);
     assert(getBitFromAddr(50, 18), 0);
     assert(getBitFromAddr(50, 19), 1);
     assert(getBitFromAddr(50, 20), 1);
     assert(getBitFromAddr(50, 21), 1);
     assert(getBitFromAddr(50, 22), 1);
     assert(getBitFromAddr(50, 23), 0);
     */
}

/* TODO: should this just return a bool? I'm not really using the memsize_t return value... */
static __inline__ memsize_t supaMalloc(const memsize_t size, const memaddr_t supaPoolAddr, memaddr_t* blockAddr){
     /*kak*/
     /* todo: alloc block8, block16 and block32 */
     /* make function: alloc4xUint16(&var16Ba, &var16Bb, &var16Bc, &var16Bd) >> this will alloc 4x uint16_t's on a block8 */
     
     if(size<=0){
          printf("::== supaMalloc:: requested size is too small: [%u]\n", size);
          return 0;
     }
     else if(size<=4){ /* alloc a block4 */
          memsize_t block4Qty = getUint16FromAddr(supaPoolAddr+2);
          memsize_t i;
          for(i=0;i<block4Qty;i++){
               if(!getBitFromAddr(supaPoolAddr+10, i)){/* status bit==0 >> block is free! */
                    setBitOnAddr(supaPoolAddr+10, i, 1);/* status bit==1 >> block is now set as full */
                    *blockAddr = supaPoolAddr + getUint16FromAddr(supaPoolAddr) + (i*4);
                    return 4;
               }
          }
          printf("::== supaMalloc:: no blocks of size[4b] available\n");
          /* TODO: should I then try to alloc it on the next bigger block? let's waste some more memoryyyy! weeeeeee! */
          return 0;
     }
     else if((size>4)&(size<=8)){ /* alloc a block8 */
          memsize_t block4Qty = getUint16FromAddr(supaPoolAddr+2);
          memsize_t block8Qty = getUint16FromAddr(supaPoolAddr+4);
          memsize_t i;
          printf("---------------b8\n");
          
          for(i=block4Qty;i<(block4Qty+block8Qty);i++){
               printf(">>supaMalloc:: block8 >> i[%u]%u\n", i, getBitFromAddr(supaPoolAddr+10, i));
               if(!getBitFromAddr(supaPoolAddr+10, i)){/* status bit==0 >> block is free! */ kak - overwrites an already used address
                    setBitOnAddr(supaPoolAddr+10, i, 1);/* status bit==1 >> block is now set as full */
                    *blockAddr = supaPoolAddr + getUint16FromAddr(supaPoolAddr) + (block4Qty*4) + (i*8);
                    return 8;
               }
          }
          printf("::== supaMalloc:: no blocks of size[8b] available\n");
          /* TODO: should I then try to alloc it on the next bigger block? let's waste some more memoryyyy! weeeeeee! */
          return 0;
     }
     else if((size>8)&(size<=16)){ /* alloc a block16 */
          memsize_t block4Qty = getUint16FromAddr(supaPoolAddr+2);
          memsize_t block8Qty = getUint16FromAddr(supaPoolAddr+4);
          memsize_t block16Qty = getUint16FromAddr(supaPoolAddr+6);
          memsize_t i;
          for(i=block4Qty+block8Qty;i<(block4Qty+block8Qty+block16Qty);i++){
               if(!getBitFromAddr(supaPoolAddr+10, i)){/* status bit==0 >> block is free! */
                    setBitOnAddr(supaPoolAddr+10, i, 1);/* status bit==1 >> block is now set as full */
                    *blockAddr = supaPoolAddr + getUint16FromAddr(supaPoolAddr) + (block4Qty*4) + (block8Qty*8) + (i*16);
                    return 16;
               }
          }
          printf("::== supaMalloc:: no blocks of size[16b] available\n");
          /* TODO: should I then try to alloc it on the next bigger block? let's waste some more memoryyyy! weeeeeee! */
          return 0;
     }
     else if((size>16)&(size<=32)){ /* alloc a block32 */
          memsize_t block4Qty = getUint16FromAddr(supaPoolAddr+2);
          memsize_t block8Qty = getUint16FromAddr(supaPoolAddr+4);
          memsize_t block16Qty = getUint16FromAddr(supaPoolAddr+6);
          memsize_t block32Qty = getUint16FromAddr(supaPoolAddr+8);
          memsize_t i;
          for(i=block4Qty+block8Qty+block16Qty;i<(block4Qty+block8Qty+block16Qty+block32Qty);i++){
               if(!getBitFromAddr(supaPoolAddr+10, i)){/* status bit==0 >> block is free! */
                    setBitOnAddr(supaPoolAddr+10, i, 1);/* status bit==1 >> block is now set as full */
                    *blockAddr = supaPoolAddr + getUint16FromAddr(supaPoolAddr) + (block4Qty*4) + (block8Qty*8) + (block8Qty*16) + (i*32);
                    return 32;
               }
          }
          printf("::== supaMalloc:: no blocks of size[32b] available\n");
          /* TODO: should I then try to alloc it on the next bigger block? let's waste some more memoryyyy! weeeeeee! */
          return 0;
     }
     else if(size>32){
          printf("::== supaMalloc:: requested size is too BIG: [%u]\n", size);
          return 0;
     }
     else{
          /* just because the compiler complains about "control reaches end of non-void function", even tho my elseifs should catch all possible cases... */
          printf("::== supaMalloc:: unknown; size request was [%u]\n", size);
          return 0;
     }
} /* supaMalloc */


/*void freeOnPool(const memaddr_t allocAddr, const poolid_t poolId){*/
     /* should it just receive the allocAddr and autodetect on which poolId it's been allocated? */
     /* cacca: è un casino, devo retrievare un botto di info per fare il dealloc */
/*}*/

/* cacca: old, only for allocOnPool */
memsize_t getAllocSize(const memaddr_t memAddr){
     return getUint16FromAddr(memAddr);
}


memaddr_t allocOnPool(const memsize_t size, const poolid_t poolId){
     #ifdef SUPAMEM_DEBUG
          printf("==trying to allocate %ubytes on pool[%u] ...\n", size, poolId);
          #endif
     
     if(size+MEM_ADDRESS_BYTES<= getFreeMem(poolId)){ /* MEM_ADDRESS_BYTES are also used to express a memory size */
          memaddr_t returnAddr;
          memsize_t blockSize = getBlockSize(poolId);
          blockid_t blocksQty = getBlocksQty(poolId);
          blockid_t blocksNeeded = (blockid_t)ceil((float)(size+MEM_ADDRESS_BYTES)/(float)blockSize); /* MEM_ADDRESS_BYTES to store the allocation size in the allocation itself */
          blockid_t blocksFound = 0;
          blockid_t i;
          blockid_t j;
          uint16_t statusBytesQty = /* how many bytes are being used to store blocks status on the pool */
               ((blocksQty*BLOCK_STATUS_BITS)/8)+(((blocksQty*BLOCK_STATUS_BITS)%8)>0);
          
          #ifdef SUPAMEM_DEBUG
               printf("==thoretically enough mem...\n");
               printf("==[%u]blocks will be used\n", blocksNeeded);
               #endif
          for(i=0;i<blocksQty;i++){
               blocksFound += getStatus(poolId, i)==BLOCK_EMPTY; /* if empty, add */
               blocksFound *= getStatus(poolId, i)==BLOCK_EMPTY; /* if not empty, zero-out "blocksFound" */
               if(blocksFound==blocksNeeded){
                    #ifdef SUPAMEM_DEBUG
                         printf("::blocksFound==blocksNeeded!!! (block index=%u)\n", i);
                         #endif
                    returnAddr = pools[poolId]+2*MEM_ADDRESS_BYTES+statusBytesQty+((i+1-blocksNeeded)*blockSize);
                    
                    #ifdef SUPAMEM_DEBUG
                         printf("==now writing status bits on pool header...\n");
                         #endif
                    for(j=i+1-blocksNeeded;j<=i;j++){
                         setStatus(poolId, j, BLOCK_FULL);
                    }
                    break;
               }
          }
          if(blocksFound==blocksNeeded){
               #ifdef SUPAMEM_DEBUG
                    printf("==[%u]consecutive blocks found at addr: %u\n", blocksNeeded, returnAddr);
                    printf("==writing size on the first %u bytes of the allocation...\n\n", MEM_ADDRESS_BYTES);
                    #endif
               mem[returnAddr]=getHiByte(size); /* cacca: not MEM_ADDRESS_BYTES-agnostic, I'm assuming a memory address is uint16_t */
               mem[returnAddr+1]=getLoByte(size); /* cacca: not MEM_ADDRESS_BYTES-agnostic, I'm assuming a memory address is uint16_t */
               return returnAddr;
          }
          else{
               printf("::==[%u]consecutive blocks NOT found\n\n", blocksNeeded);
               /* TODO: try to do the "BLOCK_NEXT" trick if I don't find enough consecutive blocks */
               return -1; /* cacca: return waaat? */
          }
     }
     else{
          printf("::==not enough memory in this pool\n");
          return -1; /* cacca: return waaat? */
     }
}

/* cacca: old...? */
memsize_t getFreeMem(const poolid_t poolId){
     memsize_t blockSize = getBlockSize(poolId);
     blockid_t blocksQty = getBlocksQty(poolId);
     blockid_t i;
     memsize_t freeMem = 0;
     
     /*printf("==getFreeMem: blockSize : %u\n", blockSize);
     printf("==getFreeMem: blocksQty : %u\n", blocksQty);
     
     printf("==getFreeMem: statuses: ", blocksQty);*/
     for(i=0;i<blocksQty;i++){
          /*if(getStatus(poolId, i)==BLOCK_EMPTY){ printf(".");}
          if(getStatus(poolId, i)==BLOCK_FULL){ printf("x");}
          if(getStatus(poolId, i)==BLOCK_NEXT){ printf("N");}
          if((i+1)%4==0){ printf (" ");}*/
          freeMem += (getStatus(poolId, i)==BLOCK_EMPTY)*blockSize;
     }
     /*printf("\n");*/
     return freeMem;
}

poolid_t makePool(const memaddr_t startAddress, const memsize_t blockSize, const memsize_t blocksQty){
     memsize_t totSize = blockSize*blocksQty;
     poolid_t returnPoolId;
     uint16_t statusBytesQty = /* how many bytes are needed to keep track of blocks status */
          ((blocksQty*BLOCK_STATUS_BITS)/8)+(((blocksQty*BLOCK_STATUS_BITS)%8)>0);
     memsize_t newMemFree = memFree - 4 - statusBytesQty; /* (2x)blockSize (2x)blocksQty */
     uint8_t i;

     if(totSize>newMemFree || startAddress>=MEM_SIZE || blockSize<MEM_ADDRESS_BYTES){ /* minimum block size must be able to contain an address to physical memory */
          return -1; /* cacca: return what?? */
     }
     for(i=0;i<MAX_POOLS;i++){
               if(pools[i]==0){/* if pool slot is empty */
                    pools[i]=startAddress;/* fill it with the current pool address */
                    returnPoolId = i;
                    break;
               }
          if(i==(MAX_POOLS-1)){
               return -1; /* cacca: return what?? */
          }
     }
     
     #ifdef SUPAMEM_DEBUG
          printf("==making a POOL of %ux(%ub)blocks(tot %ub) >> %u status bytes(%.2f%) (%u of %u %u-bits groups used (%u left in the last byte))\n",
               blocksQty,
               blockSize,
               blocksQty*blockSize,
               statusBytesQty,
               (float)statusBytesQty/(blocksQty*blockSize+statusBytesQty+2*MEM_ADDRESS_BYTES)*100,
               blocksQty,
               statusBytesQty*8/BLOCK_STATUS_BITS,
               BLOCK_STATUS_BITS,
               statusBytesQty*8/BLOCK_STATUS_BITS-blocksQty
          );
          printf("==using [%ub] to store [%ub] >> pool metadata is (%.2f%) of the total used space\n",
               blocksQty*blockSize+2*MEM_ADDRESS_BYTES+statusBytesQty,
               blocksQty*blockSize,
               ((float)(2*MEM_ADDRESS_BYTES+statusBytesQty)/(blocksQty*blockSize))*100
          );
          printf("==ACHTUNG: there will also be %ubytes of metadata for each allocation on any pool\n", MEM_ADDRESS_BYTES);
     #endif
     
     mem[startAddress]=getHiByte(blockSize);
     mem[startAddress+1]=getLoByte(blockSize);
     mem[startAddress+2]=getHiByte(blocksQty);
     mem[startAddress+3]=getLoByte(blocksQty);
     for(i=0;i<statusBytesQty;i++){
          mem[startAddress+4+i]=0;/* set status = FREE for all blocks */
     }

     memFree=newMemFree-totSize;
     return returnPoolId;
}/* makePool */

/* TODO: move twoBitsfromByte and setTwoBitsOnByte to bitty.h?? */
blockstatus_t twoBitsfromByte(const uint8_t byte, const uint8_t bitsPos){
     /*bitsPos: 0 = MSB*/
     if(bitsPos==0){
          return (byte>>6);
     }
     else if(bitsPos==1){
          return (byte>>4) & B00000011;
     }
     else if(bitsPos==2){
          return (byte>>2) & B00000011;
     }
     else if(bitsPos==3){
          return byte & B00000011;
     }
     else{
          printf("::== NOPE. bits can only be [0, 1, 2, 3]\n");
          return -1; /* cacca: return what?? */
     }
}
uint8_t setTwoBitsOnByte(const uint8_t byte, const uint8_t bitsPos, const uint8_t bits){
     /*bitsPos: 0 = MSB*/
     if(bitsPos==0){
          return (byte & B00111111) | (bits<<6);
     }
     else if(bitsPos==1){
          return (byte & B11001111) | (bits<<4);
     }
     else if(bitsPos==2){
          return (byte & B11110011) | (bits<<2);
     }
     else if(bitsPos==3){
          return (byte & B11111100) | bits;
     }
     else{
          printf("::== NOPE. bits can only be [0, 1, 2, 3]\n");
          return -1; /* cacca: return what?? */
     }
}

/* cacca: old */
blockstatus_t getStatus(const poolid_t poolId, const blockid_t blockNum){
     memaddr_t statusAddr = pools[poolId]+4;/* where the status bits-bytes begin */
     /*
     ceil(blockNum*BLOCK_STATUS_BITS/8) == which byte contains my 2bits
     statusAddr + ceil(blockNum*BLOCK_STATUS_BITS/8) == actual address of byte with my 2bits
     mem[statusAddr + ceil(blockNum*BLOCK_STATUS_BITS/8)] == I'm actually reading the byte from memory

     fract(blockNum/4) will be:
     .25 >> I select bit 0
     .5 >> I select bit 1
     .75 >> I select bit 2
     0 >> I select bit 3
     */
     return twoBitsfromByte(
          mem[statusAddr + (uint32_t)ceil(blockNum*BLOCK_STATUS_BITS/8)],
               0 +
               (fract((float)blockNum/4)==.25)*1 +
               (fract((float)blockNum/4)==.5)*2 +
               (fract((float)blockNum/4)==.75)*3
     );
}

/* cacca: old */
void setStatus(const poolid_t poolId, const blockid_t blockNum, const blockstatus_t status){
     memaddr_t statusAddr = pools[poolId]+4;/* where the status bytes begin */
     
     /*printf("::setStatus: blockNum[%u] >> byte[%u]\n", blockNum, (uint32_t)ceil(blockNum*BLOCK_STATUS_BITS/8));
     printf("::setStatus: fract(blockNum/4): %.2f\n", fract((float)blockNum/4));
     printBits(mem[statusAddr+(uint32_t)ceil(blockNum*BLOCK_STATUS_BITS/8)]);*/
     
     mem[statusAddr+(uint32_t)ceil(blockNum*BLOCK_STATUS_BITS/8)] =
          setTwoBitsOnByte(
               mem[statusAddr+(uint32_t)ceil(blockNum*BLOCK_STATUS_BITS/8)],
                    0 +
                    (fract((float)blockNum/4)==.25)*1 +
                    (fract((float)blockNum/4)==.5)*2 +
                    (fract((float)blockNum/4)==.75)*3,
               status
          );
          
     /*printBits(mem[statusAddr+(uint32_t)ceil(blockNum*BLOCK_STATUS_BITS/8)]);*/

     /* cacca: not managing errors */
}


void showmem(){
     uint16_t i;
     for(i=0;i < MEM_SIZE;i++){
          printf("%u ", mem[i]);
          if((i+1)%32==0){
               printf("\n");
          }
     }
     printf("==tot unallocated mem: %ubytes\n\n", memFree);
}/* showmem */

/* cacca: old */
memsize_t getBlockSize(const poolid_t poolId){
     return getUint16FromAddr(pools[poolId]);
}
/* cacca: old */
uint16_t getBlocksQty(const poolid_t poolId){
     return getUint16FromAddr(pools[poolId]+2);
}

void showparts(){
     uint8_t i;
     blockid_t j;

     for(i=0;i<MAX_POOLS;i++){
          blockid_t blocksQty = getBlocksQty(i);
          memsize_t blockSize = getBlockSize(i);
          if(pools[i]){
               printf("==pool[%u]at[%u] >> %ux %ubytes blocks >> %ubytes total : "
                    , i, pools[i], blocksQty, blockSize, blocksQty*blockSize);
               for(j=0;j<blocksQty;j++){
                    if(getStatus(i, j)==BLOCK_EMPTY){ printf(".");}
                    if(getStatus(i, j)==BLOCK_FULL){ printf("x");}
                    if(getStatus(i, j)==BLOCK_NEXT){ printf("N");}
                    if((j+1)%4==0){ printf (" ");}
               }
               printf("\n");
          }
          else{
               printf("==part[%u] is empty\n", i);
          }
     }
}
/* FUNCTION DEFINITIONS end. */
