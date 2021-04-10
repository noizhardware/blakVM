/*** DOCU
“A clear conscience is the sure sign of a bad memory.”
― Mark Twain

  at the moment I'm using uint16_t absolute addresses, so I can map a total physical memory of MAX 64k
  
  pools = [pool address, pool address, pool address, ....]
  (pool address) -> [blockSize, blocksQty, statuses, block, block, block, ...]
  (allocation address) -> [(MEM_ADDRESS_BYTES)allocation size, actual usable mem space!]
  
  I can request to allocate a pool of memory blocks of X size each
  poolid_t makePool(const memaddr_t startAddress, const memsize_t blockSize, const memsize_t blocksQty);
     the function, if successful, will add the address of the
     newly created pool in the array "pools" and returns the poolId
  structure of an allocated pool:
  [(MEM_ADDRESS_BYTES)blockSize][(MEM_ADDRESS_BYTES)blocksQty][status bytes][...actual mem space(called "blocks")]

*/

/*** TODO:
     - blockid_t getFreeBlocks(poolid_t poolId) tells how many free blocks in pool
     - deterministic? do I know how long does each operation take to complete? is it constant?
*/

#define SUPAMEM_VERSION "2021d10-1759"

/*** DEFINES */

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
     #include <stdio.h>
     #include <stdint.h>
     #include <math.h>

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
/* TYPEDEFS end. */

/*** GLOBALS */
     uint8_t mem[MEM_SIZE];
     memaddr_t memPtr=0;
     memsize_t memFree=MEM_SIZE;
     memaddr_t pools[MAX_POOLS];/* index of all created pools, containing addresses of the pools */
/* GLOBALS end. */

/*** FUNCTION DECLARATIONS */
     poolid_t makePool(const memaddr_t startAddress, const memsize_t blockSize, const memsize_t blocksQty);
     memaddr_t allocOnPool(const memsize_t size, const poolid_t poolId);
     
     blockstatus_t twoBitsfromByte(const uint8_t byte, const uint8_t bitsPos);
     uint8_t setTwoBitsOnByte(const uint8_t byte, const uint8_t bitsPos, const uint8_t bits);
     
     blockstatus_t getStatus(const poolid_t poolId, const blockid_t blockNum);
     void setStatus(const poolid_t poolId, const blockid_t blockNum, const blockstatus_t status);
     
     memsize_t getBlockSize(const poolid_t poolId);
     uint16_t getBlocksQty(const poolid_t poolId);
     memsize_t getFreeMem(const poolid_t poolId);
          
     void showmem();
     void showparts();
     
/* FUNCTION DECLARATIONS end. */

/*** MAIN */
int main(){
     poolid_t myPool;
     
     printf("== SUPAMEM v. %s==\n", SUPAMEM_VERSION);
     
     showmem();
     myPool = makePool(64, 4, 37);
     showmem();
     showparts();

     printf("free mem in pool[%u]: %ub in [%ux]of[%ux] [%ub]blocks\n", myPool, getFreeMem(myPool), getFreeMem(myPool)/getBlockSize(myPool), getBlocksQty(myPool), getBlockSize(myPool));

     allocOnPool(9, myPool);
     printf("free mem in pool[%u]: %ub in [%ux]of[%ux] [%ub]blocks\n", myPool, getFreeMem(myPool), getFreeMem(myPool)/getBlockSize(myPool), getBlocksQty(myPool), getBlockSize(myPool));
     showparts();
     showmem();
     
     allocOnPool(23, myPool);
     printf("free mem in pool[%u]: %ub in [%ux]of[%ux] [%ub]blocks\n", myPool, getFreeMem(myPool), getFreeMem(myPool)/getBlockSize(myPool), getBlocksQty(myPool), getBlockSize(myPool));
     showparts();
     showmem();
     
     
     return 0;
}
/* MAIN end. */

/*** FUNCTION DEFINITIONS */

/* TODO kak void freeOnPool(memaddr_t allocAddr, const poolid_t poolId){*/
     /* should it just receive the allocAddr and autodetect on which poolId it's been allocated? */
/*}*/

memaddr_t allocOnPool(const memsize_t size, const poolid_t poolId){
     printf("==trying to allocate %ubytes on pool[%u] ...\n", size, poolId);
     
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
          
          printf("==thoretically enough mem...\n");
          printf("==[%u]blocks will be used\n", blocksNeeded);
          for(i=0;i<blocksQty;i++){
               blocksFound += getStatus(poolId, i)==BLOCK_EMPTY; /* if empty, add */
               blocksFound *= getStatus(poolId, i)==BLOCK_EMPTY; /* if not empty, zero-out "blocksFound" */
               if(blocksFound==blocksNeeded){
                    printf("::blocksFound==blocksNeeded!!! (block index=%u)\n", i);
                    returnAddr = pools[poolId]+2*MEM_ADDRESS_BYTES+statusBytesQty+((i+1-blocksNeeded)*blockSize);
                    
                    printf("==now writing status bits on pool header...\n");
                    for(j=i+1-blocksNeeded;j<=i;j++){
                         setStatus(poolId, j, BLOCK_FULL);
                    }
                    break;
               }
          }
          if(blocksFound==blocksNeeded){
               printf("==[%u]consecutive blocks found at addr: %u\n", blocksNeeded, returnAddr);
               printf("==writing size on the first %u bytes of the allocation...\n\n", MEM_ADDRESS_BYTES);
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
     
     printf("==making a POOL of %ux(%ub)blocks(tot %ub) >> %u status bytes(%.2f%) (%u of %u %u-bits groups used (%u left in the last byte))\n", blocksQty, blockSize, blocksQty*blockSize, statusBytesQty, (float)statusBytesQty/(blocksQty*blockSize+statusBytesQty+2*MEM_ADDRESS_BYTES)*100, blocksQty, statusBytesQty*8/BLOCK_STATUS_BITS, BLOCK_STATUS_BITS, statusBytesQty*8/BLOCK_STATUS_BITS-blocksQty);
     printf("==using [%ub] to store [%ub] >> pool metadata is (%.2f%) of the total used space\n", blocksQty*blockSize+2*MEM_ADDRESS_BYTES+statusBytesQty, blocksQty*blockSize, ((float)(2*MEM_ADDRESS_BYTES+statusBytesQty)/(blocksQty*blockSize))*100);
     printf("==ACHTUNG: there will also be %ubytes of metadata for each allocation on any pool\n", MEM_ADDRESS_BYTES);
     
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

memsize_t getBlockSize(const poolid_t poolId){
     return makeUint16(mem[pools[poolId]], mem[pools[poolId]+1]);
}
uint16_t getBlocksQty(const poolid_t poolId){
     return makeUint16(mem[pools[poolId]+2], mem[pools[poolId]+3]);
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
