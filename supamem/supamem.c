/*** TODO

memaddr_t allocOnPart(memsize_t size, uint8_t partition){
}


  32x 4bytes blocks
  2bits per block to keep record of their status
  64bits >> 8bytes

*/

#define SUPAMEM_VERSION "2021d07-2136"

/*** DEFINES */
     #define ERROR 0
     #define SUCCESS 1

     #define MEM_SIZE 1024
     #define MAX_PARTITIONS 8
     #define BLOCK_STATUS_BITS 2
/* DEFINES end. */

/*** INCLUDES */
     #include <stdio.h>
     #include <stdint.h>
     #include <math.h>

     #include "bitty.h"
/* INCLUDES end. */

/*** TYPEDEFS */
     typedef uint16_t memsize_t;/* 65535 bytes (64k) max mem size */
     typedef memsize_t memaddr_t;
     typedef uint8_t partitionid_t;
     typedef uint16_t blockid_t;
/* TYPEDEFS end. */

/*** GLOBALS */
     uint8_t mem[MEM_SIZE];
     memaddr_t memPtr=0;
     memsize_t memFree=MEM_SIZE;
     memaddr_t partitions[MAX_PARTITIONS];/* index of all created partitions, containing addresses of the partitions */
/* GLOBALS end. */

/*** FUNCTION DECLARATIONS */
     uint8_t part(memaddr_t startAddress, memsize_t blockSize, memsize_t blocksQty);
     blockstatus_t twoBitsfromByte(uint8_t byte, uint8_t bits);
     blockstatus_t getStatus(partitionid_t partId, blockid_t blockNum);
     void showmem();
     void showparts();
/* FUNCTION DECLARATIONS end. */

/*** MAIN */
int main(){
     showmem();
     part(64, 4, 37);
     showmem();
     showparts();

     return 0;
}
/* MAIN end. */

/*** FUNCTION DEFINITIONS */

uint8_t part(memaddr_t startAddress, memsize_t blockSize, memsize_t blocksQty){
     memsize_t totSize = blockSize*blocksQty;
     /* how many bytes are needed to keep track of blocks status */
     uint16_t statusBytesQty =
          ((blocksQty*BLOCK_STATUS_BITS)/8)+(((blocksQty*BLOCK_STATUS_BITS)%8)>0);
     memsize_t newMemFree = memFree - 4 - statusBytesQty; /* (2x)blockSize (2x)blocksQty */
     uint8_t i;

     printf("==== %u blocks >> %u status bytes", blocksQty, statusBytesQty);

     if(totSize>newMemFree || startAddress>=MEM_SIZE){
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

blockstatus_t twoBitsfromByte(uint8_t byte, uint8_t bits){
     /*bits: 0 = MSB*/
     if(bits==0){
          return (byte>>6) & 0xff;
     }
     else if(bits==1){
          return (byte>>6) & 0xff;
     }/*kak*/
}

blockstatus_t getStatus(partitionid_t partId, blockid_t blockNum){
     memaddr_t statusAddr = partitions[partId]+4;
     /*kak*/
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

void showparts(){
     uint8_t i = 0;
     uint16_t blockSize;
     uint16_t blockQty;
     uint16_t partAddr;

     for(i=0;i<MAX_PARTITIONS;i++){
          if(partitions[i]>0){
               partAddr = partitions[i];
               blockSize = makeUint16(mem[partAddr], mem[partAddr+1  ]);
               blockQty = makeUint16(mem[partAddr+2], mem[partAddr+3]);
               printf("==part[%u]:%u >> %ux %ubytes blocks >> %ubytes total\n"
                    , i, partAddr, blockQty, blockSize, blockQty*blockSize);
          }
          else{
               printf("==part[%u] is empty\n", i);
          }
     }
}
/* FUNCTION DEFINITIONS end. */
