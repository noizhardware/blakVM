/*** DOCU
“A clear conscience is the sure sign of a bad memory.”
― Mark Twain

  at the moment I'm using uint16_t absolute addresses, so I can map a total physical memory of MAX 64k
  
  pools = [pool, pool, pool, ....]
  pool = [block, block, block, ...]
  
  I can request to allocate a pool of memory blocks of X size each
  uint8_t makePool(const memaddr_t startAddress, const memsize_t blockSize, const memsize_t blocksQty);
     the function will return SUCCESS if successful and will add the address of the
     newly created pool in the array "pools" cacca: dovrebbe almeno buttarmi fuori
     o un indirizzo o il ID per l'array pools in modo che posso sapere DOVE l'ha creato
  structure of allocated pool:
  [(2bytes)blocksize][(2bytes)how many blocks][status bytes][...actual mem space(called "blocks")]

*/

#define SUPAMEM_VERSION "2021d10-1543"

/*** DEFINES */
     #define ERROR 0
     #define SUCCESS 1

     #define MEM_SIZE 1024
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
     typedef uint16_t memsize_t;/* 65535 bytes (64k) max mem size */
     typedef memsize_t memaddr_t;
     typedef uint8_t poolid_t;
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
     
     showmem();
     myPool = makePool(64, 4, 37);
     showmem();
     showparts();

     printf("free mem in pool[%u]: %u\n", myPool, getFreeMem(myPool));

     allocOnPool(10, myPool);
     printf("free mem in pool[%u]: %u\n", myPool, getFreeMem(myPool));
     showparts();
     showmem();
     
     allocOnPool(23, myPool);
     printf("free mem in pool[%u]: %u\n", myPool, getFreeMem(myPool));
     showparts();
     showmem();
     
     
     return 0;
}
/* MAIN end. */

/*** FUNCTION DEFINITIONS */

void freeOnPool(const memsize_t size, const poolid_t poolId){
     /* problema: l'allocazione non si "ricorda" di quanti blocks è fatta */
}

memaddr_t allocOnPool(const memsize_t size, const poolid_t poolId){
     printf("==trying to allocate %ubytes on pool[%u] ...\n", size, poolId);
     
     if(size<= getFreeMem(poolId)){
          memaddr_t returnAddr;
          memsize_t blockSize = getBlockSize(poolId);
          blockid_t blocksQty = getBlocksQty(poolId);
          blockid_t blocksNeeded = (blockid_t)ceil((float)size/(float)blockSize);
          blockid_t blocksFound = 0;
          blockid_t i;
          blockid_t j;
          
          printf("==enough mem!\n");
          printf("==[%u]blocks will be used\n", blocksNeeded);
          for(i=0;i<blocksQty;i++){
               blocksFound += getStatus(poolId, i)==BLOCK_EMPTY; /* if empty, add */
               blocksFound *= getStatus(poolId, i)==BLOCK_EMPTY; /* if not empty, zero-out "blocksFound" */
               if(blocksFound==blocksNeeded){
                    printf("::blocksFound==blocksNeeded!!! (i=%u)\n", i);
                    returnAddr = pools[poolId]+4+((i+1-blocksNeeded)*blockSize);
                    for(j=i+1-blocksNeeded;j<=i;j++){
                         setStatus(poolId, j, BLOCK_FULL);
                    }
                    break;
               }
          }
          if(blocksFound==blocksNeeded){
               printf("==[%u]consecutive blocks found at addr: %u\n", blocksNeeded, returnAddr);
               return returnAddr;
          }
          else{
               printf("::==[%u]consecutive blocks NOT found\n", blocksNeeded);
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
          if(getStatus(poolId, i)==BLOCK_NEXT){ printf(">");}
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

     if(totSize>newMemFree || startAddress>=MEM_SIZE || blockSize<2){ /* minimum block size is 2bytes */
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
     
     printf("==allocating %ux(%ub)blocks(tot %ub) >> %u status bytes(%.2f%) (%u of %u %u-bits groups used (%u left in the last byte))\n", blocksQty, blockSize, blocksQty*blockSize, statusBytesQty, (float)statusBytesQty/(blocksQty*blockSize+statusBytesQty+4)*100, blocksQty, statusBytesQty*8/BLOCK_STATUS_BITS, BLOCK_STATUS_BITS, statusBytesQty*8/BLOCK_STATUS_BITS-blocksQty);
     
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
                    if(getStatus(i, j)==BLOCK_NEXT){ printf(">");}
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
