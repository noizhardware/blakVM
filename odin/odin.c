/*** TODO
     odin is an experiment: push and pull single bits form memory, for max efficiency
     the full pointer to memory is memPtr + memPtrBits
     memPtr is the regular pointer, points to a byte
     memPtrBits is an offset pointer, telling us what BIT in the byte to point at (big endian)
     example:
     memPtr==37
     memPtrBits==3
     we're pointing at the third bit of the byte located at address 37 in mem
     |   36   |   37   |        |
     |00001010|00110101|10001100|
                  ^
     
     - setBits WORKS now - but needs some more features -- see TODOs in the function
     - getBits TODO
*/

#define FILENAME_VERSION "2021d25-2025"

/*** DEFINES */
     /**** blak setup */
          #define BLAK_OUT_TERMINAL /* if we have a regular, printf-capable stdout interface */
          #define BLAK_IN_TERMINAL /* regular stdin for text input */
          #define BLAK_PC_FILESYSTEM /* regular filesystem with file access (to load bytecode ROMs) */
     /**** blak setup END. */
/* DEFINES end. */

/*** INCLUDES */

#include "../blak2.h"
#include "bitty.h"
/* INCLUDES end. */

/*** TYPEDEFS */
/* TYPEDEFS end. */

/*** GLOBALS */
     uint8_t memPtrBits = 0;
/* GLOBALS end. */

/*** FUNCTION DECLARATIONS */
static __inline__ void setBits(const uint8_t bitsQty, const uint8_t bits){
     /* "bits" contains the bits to set, "bitsQty" tells how many of these bits are to be set in memory (0=MSB)(big-endian) */
     uint8_t i;
     
     printBits(mem[memPtr]);
     printf(" : mem before push [%u]\n", memPtr);
     
     /* TODO: I should take into account the memPtrBits offset, and manage if it leads to point to the next byte */
     for(i=0;i<bitsQty;i++){
          setBit(&mem[memPtr], i/*+ offset*/, getBit(bits, i));
     }
     
     printBits(mem[memPtr]);
     printf(" : mem after push [%u]\n", memPtr);
     
     /* TODO: I should move memPtr, and/or memPtrBits */
}
/* FUNCTION DECLARATIONS end. */

/*** MAIN */
int main(int argc, char* argv[]){

     progMem[0]=BLAK_SHOWREGS;
     progMem[1]=BLAK_SHOWMEM;
     progMem[2]=BLAK_ENDOFBYTECODE;
     eval();

     setBits(3, B10111010);

     (void)blak_flags;
     (void)argc;
     (void)argv;
     return 0;
}
/* MAIN end. */

/*** FUNCTION DEFINITIONS */
/* FUNCTION DEFINITIONS end. */