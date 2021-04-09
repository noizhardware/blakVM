/*** TODO

  32x 4bytes blocks
  2bits per block to keep record of their status
  64bits >> 8bytes
  
  at the moment I'm using uint16_t absolute addresses, so I can map a total physical memory of MAX 64k
  
  I can request to allocate a pool of memory blocks of X size each
  uint8_t part(memaddr_t startAddress, memsize_t blockSize, memsize_t blocksQty)
     the function will return SUCCESS if successful and will add the address of the
     newly created partition in the array "partitions" cacca: dovrebbe almeno buttarmi fuori
     o un indirizzo o il ID per l'array partitions in modo che posso sapere DOVE l'ha creato
  structure of allocated partition:
  [(2bytes)blocksize][(2bytes)how many blocks][status bytes][...actual mem space]

*/

#define SUPAMEM_VERSION "2021d09-2131"

/*** DEFINES */
     #define ERROR 0
     #define SUCCESS 1

     #define MEM_SIZE 1024
     #define MAX_PARTITIONS 8
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
     typedef uint8_t partitionid_t;
     typedef uint16_t blockid_t;
     typedef uint8_t blockstatus_t;/* actually using only 2 bits >> [0, 1, 2, 3] */
/* TYPEDEFS end. */

/*** GLOBALS */
     uint8_t mem[MEM_SIZE];
     memaddr_t memPtr=0;
     memsize_t memFree=MEM_SIZE;
     memaddr_t partitions[MAX_PARTITIONS];/* index of all created partitions, containing addresses of the partitions */
/* GLOBALS end. */

/*** FUNCTION DECLARATIONS */
     uint8_t makePool(const memaddr_t startAddress, const memsize_t blockSize, const memsize_t blocksQty);
     
     blockstatus_t twoBitsfromByte(const uint8_t byte, const uint8_t bitsPos);
     uint8_t setTwoBitsOnByte(const uint8_t byte, const uint8_t bitsPos, const uint8_t bits);
     
     blockstatus_t getStatus(const partitionid_t partId, const blockid_t blockNum);
     uint8_t setStatus(const partitionid_t partId, const blockid_t blockNum, const uint8_t bits);
     
     memsize_t getBlockSize(const partitionid_t partitionId);
     uint16_t getBlocksQty(const partitionid_t partitionId);
     memsize_t getFreeMem(const partitionid_t partitionId);
          
     void showmem();
     void showparts();
     
/* FUNCTION DECLARATIONS end. */

/*** MAIN */
int main(){
     showmem();
     makePool(64, 4, 37);
     showmem();
     showparts();

     printf("part[%u], block[%u] : status -> %u\n", 0, 5, getStatus(0, 5));
     
     if(setStatus(0, 5, 2)){
          printf("part[%u], block[%u] : status -> %u\n", 0, 5, getStatus(0, 5));
     }

     printf("free mem: %u\n", getFreeMem(0));

     return 0;
}
/* MAIN end. */

/*** FUNCTION DEFINITIONS */

memaddr_t allocOnPool(memsize_t size, partitionid_t partitionId){
     memsize_t blockSize = getBlockSize(partitionId);
     blockid_t blocksQty = getBlocksQty(partitionId);
     if(size<= 99 /*getFreeMem here*/){
          /* kak */
     }
     else{
          return -1; /* cacca: return waaat? */
     }
}

memsize_t getFreeMem(const partitionid_t partitionId){
     memsize_t blockSize = getBlockSize(partitionId);
     blockid_t blocksQty = getBlocksQty(partitionId);
     blockid_t i;
     memsize_t freeMem = 0;
     
     printf("==getFreeMem: blockSize : %u\n", blockSize);
     printf("==getFreeMem: blocksQty : %u\n", blocksQty);
     
     printf("==getFreeMem: statuses: ", blocksQty);
     for(i=0;i<blocksQty;i++){
          printf("%u ", getStatus(partitionId, i));
          freeMem += (getStatus(partitionId, i)==BLOCK_EMPTY)*blockSize;
     }
     printf("\n");
     return freeMem;
}

uint8_t makePool(const memaddr_t startAddress, const memsize_t blockSize, const memsize_t blocksQty){
     memsize_t totSize = blockSize*blocksQty;
     /* how many bytes are needed to keep track of blocks status */
     uint16_t statusBytesQty =
          ((blocksQty*BLOCK_STATUS_BITS)/8)+(((blocksQty*BLOCK_STATUS_BITS)%8)>0);
     memsize_t newMemFree = memFree - 4 - statusBytesQty; /* (2x)blockSize (2x)blocksQty */
     uint8_t i;

     if(totSize>newMemFree || startAddress>=MEM_SIZE || blockSize<2){ /* minimum block size is 2bytes */
          return ERROR;
     }
     for(i=0;i<MAX_PARTITIONS;i++){
               if(partitions[i]==0){/* if partition slot is empty */
                    partitions[i]=startAddress;/* fill it with the current partition address */
                    break;
               }
          if(i==(MAX_PARTITIONS-1)){
               return ERROR;
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
     return SUCCESS;
}/*uint8_t part(memaddr_t startAddress, memsize_t partitionSize, memsize_t partitionsQty)*/

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


blockstatus_t getStatus(const partitionid_t partId, const blockid_t blockNum){
     memaddr_t statusAddr = partitions[partId]+4;/* where the status bits-bytes begin */
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
               (fract(blockNum/4)==.5)*1 +
               (fract(blockNum/4)==.75)*2 +
               (fract(blockNum/4)==0.)*3
     );
}

uint8_t setStatus(const partitionid_t partId, const blockid_t blockNum, const uint8_t bits){
     memaddr_t statusAddr = partitions[partId]+4;/* where the status bits-bytes begin */
     
     mem[statusAddr+(uint32_t)ceil(blockNum*BLOCK_STATUS_BITS/8)] =
          setTwoBitsOnByte(
               mem[statusAddr+(uint32_t)ceil(blockNum*BLOCK_STATUS_BITS/8)],
               0 +
                    (fract(blockNum/4)==.5)*1 +
                    (fract(blockNum/4)==.75)*2 +
                    (fract(blockNum/4)==0.)*3,
               bits
          );
     return SUCCESS;
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
     printf("==tot free mem: %ubytes\n\n", memFree);
}/* showmem */

memsize_t getBlockSize(const partitionid_t partitionId){
     return makeUint16(mem[partitions[partitionId]], mem[partitions[partitionId]+1]);
}
uint16_t getBlocksQty(const partitionid_t partitionId){
     return makeUint16(mem[partitions[partitionId]+2], mem[partitions[partitionId]+3]);
}

void showparts(){
     uint8_t i = 0;

     for(i=0;i<MAX_PARTITIONS;i++){
          if(partitions[i]){
               printf("==part[%u]:%u >> %ux %ubytes blocks >> %ubytes total\n"
                    , i, partitions[i], getBlocksQty(i), getBlockSize(i), getBlocksQty(i)*getBlockSize(i));
          }
          else{
               printf("==part[%u] is empty\n", i);
          }
     }
}
/* FUNCTION DEFINITIONS end. */
