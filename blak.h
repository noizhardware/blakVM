/*#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN*/

#include <stdint.h>
#include <stdbool.h>

#include "baostring.h"



/****************************************

https://wiki.xxiivv.com/site/gyo.html
https://git.sr.ht/~rabbits/gyo/tree/master/cpu.c

sequential stack (non stack-like) ???????

bottom of stack = STACK[255]

* (178x) 32-bit float registers [REG[0]..REG[127]] >> [-32,768..32,767]
* (16x) 1-bit flags [REG_FLAGS]>>[FLAG_1..FLAG_16]
* 8-bit instructions (set of 256)

- Arduino Micro : SRAM 2.5 KB (ATmega32U4)
     + 256b for instruction stack >> 256 instructions program length (MAX_STACK)
     + 712b for GP registers 
     + 2b for flags
     + 7b other registers
     + total of 977, round up to next power of two, 1024b reserved for the VM, that's 1kb
     + 47b remaining
     
- instructions refer to register numbers to retreive variable data

register_t :
  char name[4]
  valType_t valType
  regValue_t value
REG[register number here]

56x 9b REGISTERS >> 504b


****************************************/

/*** DEFINES */

     #define STACK_SIZE 108 /* instruction stack size, max value [0..255] */
     #define MAX_STACK STACK_SIZE-1 /* stack array range is [0..size-1] */
     #define STACK_BOTTOM MAX_STACK
     #define STACK_TOP 0
     
     #define REG_SIZE 56 /* number of registers */
     #define MAX_REG REG_SIZE-1 /* register array range is [0..size-1] */
     
     #define MEM_SIZE 512 /* size of virtual memory */
     #define MAX_MEM MEM_SIZE-1 /* memory array range is [0..size-1] */
     
     #define REG_NAME_LEN 4 /* chars for the name of a register-stored variable */
/****************************************/

/*** TYPES */

     typedef uint8_t mem_t; /* size of memory elements */
     typedef uint16_t mempointer_t; /* (size of this pointer depends on size of memory, 16bits are necessary for up to 65,536 bytes) */

     typedef enum __attribute__ ((__packed__)){ /* __packed__ keeps the size of the enum as small as possible (1byte in this case) */
          NIL,
          DF, 
          RDF,
          UDF,
          LD,
          ULD,
          HCF,
          SUM,
          SUB,
          MUL,
          DIV,
          EQ,
          FRV,
          IF,
          SAY,
          DUMPSERIAL,
          TAKESERIAL,
          LAMBDA,
          MU,
          GO,
          HH,
          EXTERN,
          HB,
          SET,
          LV,
          DD,
          PRAGMA,
          NOP,
          TERMINATOR /* used to terminate a sequence of bytecodes, like the '\0' at the end of a string */
     } instruction_t;

     typedef union regValue_t{
          uint16_t   addr;
          float      fval;
          uint8_t    ubyte1;
          int8_t     byte1;
          uint16_t   ubyte2;
          int16_t    byte2;
          uint32_t   ubyte4;
          int32_t    byte4;
     } regValue_t;

     typedef enum __attribute__ ((__packed__)){ /* __packed__ keeps the size of the enum as small as possible (1byte in this case) */
          EMPTY, /* register is empty (when you want to reset a register, no need to wipe it, just set its valType to EMPTY) */
          ADDR, /* 16bit, mapped to addr */
          FUNC, /* 16bit, mapped to addr, contains the address of auser-defined function */
          FVAL, /* float, mapped to fval */
          UBYTE1, /* unsigned byte, mapped to ubyte1 */
          BYTE1, /* signed byte, mapped to byte1 */
          UINT16B, /* 2byte unsigned int, mapped to ubyte2 */
          INT16B, /* 2byte signed int, mapped to byte2 */
          UBYTE2,/* 2 separate unsigned bytes, mapped to ubyte2 */
          BYTE2,/* 2 separate signed bytes, mapped to byte2 */

          UBYTE3,/* 3 separate unsigned bytes, mapped to ubyte4 */
          BYTE3,/* 3 separate signed bytes, mapped to byte4 */
          UBYTE4,/* 4 separate unsigned bytes, mapped to ubyte4 */
          BYTE4,/* 4 separate signed bytes, mapped to byte4 */

          UINT32B, /* 4byte unsigned int, mapped to ubyte4 */
          INT32B, /* 4byte signed int, mapped to byte4 */
          FLAG1,
          FLAG2, /*and so on, all mapped to single bits of a ubyte4, up to 32 */
          LONGNAME, /* can contain the address on the continuation of a name, and follow some structure after that address to contain the rest of the data */
          NOTHING, /* Nothing monad (probably monadic types shouldn't be here, they should be on top of regular types, so I can have "JUST INT32B") */
          /* la cosa giusta sarebbe che ogni variabile ha DUE types: uno usato per i tipi normali e uno monadico */
          JUST /* Just monad */
     } valType_t;

     typedef struct register_t{ /* total 9bytes >> 56x >> 504bytes */
       unsigned char name[REG_NAME_LEN]; /* 4byte : array for variable name */
       valType_t valType; /* 1byte : type of the variable (see valType_t), determines what type of data is stored in .value */
       regValue_t value; /* 4bytes : the actual value */
     } register_t;

/* TYPES end. */



/*** FUNCTION DECLARATIONS */
     static __inline__ void PUSH(instruction_t in);
     static __inline__ instruction_t GET();
     static __inline__ instruction_t POP();
     static __inline__ void UP();
     static __inline__ void DN();
     static __inline__ void UPP();
     static __inline__ void DNN();

     static __inline__ instruction_t detect(char* st);

     static __inline__ void eval(instruction_t byteCode[]);

     static __inline__ bool mpFwd(); /* move memory pointer */
     static __inline__ bool mpRew();

     #ifdef BLAK_SCREEN
          static __inline__ void print_valType(valType_t val);
     #endif

/* FUNCTION DECLARATIONS end. */


/*** GLOBALS */
     static instruction_t STACK[STACK_SIZE];
     
     static mem_t MEMORY[MEM_SIZE];
     static mempointer_t MP = 0; /* memory pointer */
          
     static register_t REG[REG_SIZE];
     
     static uint16_t REG_FLAGS __attribute__((unused)) = 0; /* 16x 1-bit flags */
     
     static instruction_t REG_IR __attribute__((unused)) = NIL; /* instruction register -- contains current instruction (these are the user-visible instructions)*/
     /*static register_t REG_ACC __attribute__((unused)) = 0;*/ /* ACCumulator */
     static mempointer_t REG_RET __attribute__((unused)) = 0; /* RETurn address */
     static mempointer_t REG_SP = STACK_BOTTOM; /* Instruction Stack Pointer */
          
/* GLOBALS end. */

/*** FUNCTION DEFINITIONS */

static __inline__ bool mpFwd(){ /* move memory pointer */
     bool ret = (MP<MAX_MEM);
     MP += ret;
     return ret;} /* returns true if successful */

static __inline__ bool mpRew(){ /* move memory pointer */
     bool ret = (MP>0);
     MP -= ret;
     return ret;} /* returns true if successful */
     
static __inline__ void eval(instruction_t byteCode[]){
     uint16_t bcp = 0; /* bytecode pointer */
     uint16_t j = 0;
     uint16_t empty_register = 0;
     unsigned char name[REG_NAME_LEN]={'\0', '\0', '\0', '\0'};
     while(byteCode[bcp]!=TERMINATOR){
          #ifdef DEBUG
               printf("!!!not a terminator: %d\n", byteCode[bcp]);
               #endif
          if(byteCode[bcp]==DF){
               #ifdef DEBUG
                    printf("!!!DF\n");
                    #endif
               /* let's find the first empty register */
               empty_register=0;
               while(REG[empty_register].valType!=EMPTY){
                    empty_register++;} /* found the first empty register, empty_register now points to it */
               bcp++; /* advance pointer, will now point to the DF variable name */
               #ifdef DEBUG
                    printf("!!!define name: ");
                    #endif
               /* now we start reading the NAME of the define, it can be an unterminated 4-byte sequence, or a NIL-terminated 1, 2 or 3 bytes sequence */
               memset(name, '\0', REG_NAME_LEN); /* reset name[] */
               
               
               
               /* old version, with unrolled "for" loop */
               /*j=0;
               if(byteCode[bcp]!=NIL){
                    #ifdef DEBUG
                         printf("%c", byteCode[bcp]);
                         #endif
                    name[j]=byteCode[bcp];
                    bcp++; j++;}
               if(byteCode[bcp]!=NIL){
                    #ifdef DEBUG
                         printf("%c", byteCode[bcp]);
                         #endif
                    name[j]=byteCode[bcp];
                    bcp++; j++;}
               if(byteCode[bcp]!=NIL){
                    #ifdef DEBUG
                         printf("%c", byteCode[bcp]);
                         #endif
                    name[j]=byteCode[bcp];
                    bcp++; j++;}
               if(byteCode[bcp]!=NIL){
                    #ifdef DEBUG
                         printf("%c", byteCode[bcp]);
                         #endif
                    name[j]=byteCode[bcp];}*/
               
               
               for(j=0;j<REG_NAME_LEN;j++){
                    if(byteCode[bcp]!=NIL){
                         #ifdef DEBUG
                              printf("%c", byteCode[bcp]);
                              #endif
                         name[j]=byteCode[bcp];
                         bcp+=(j<(REG_NAME_LEN-1));
                    }
               }
                    
               /* name[] is now containing the variable name */
                    
               #ifdef DEBUG
                    printf("\n");
                    printf("!!!now pointing at: %d\n", byteCode[bcp]);
                    #endif
               /* now I will put name, type and value in the empty register */
               for(j=0;j<REG_NAME_LEN;j++){ /* write variable name in register */
                    REG[empty_register].name[j]=name[j];}
               bcp++; /* advance bytecode pointer, now pointing to valType */
               #ifdef DEBUG
                    printf("!!!value type: %d\n", byteCode[bcp]);
                    #endif
               REG[empty_register].valType=byteCode[bcp]; /* write the value type into register */
               bcp++; /* advance pointer, to point to the first byte of the actual value */
               #ifdef DEBUG
                    printf("!!! value: %d\n", byteCode[bcp]);
                    #endif
                    /* now write the value into the register, according to its type */
                    if(REG[empty_register].valType==ADDR){/* 16bit, mapped to addr */}
                    else if(REG[empty_register].valType==FVAL){/* float, mapped to fval */}
                    else if(REG[empty_register].valType==UBYTE1){REG[empty_register].value.ubyte1=byteCode[bcp];}
                    else if(REG[empty_register].valType==BYTE1){REG[empty_register].value.byte1=byteCode[bcp];}
                    else if(REG[empty_register].valType==UINT16B){/* 2byte unsigned int, mapped to ubyte2 */}
                    else if(REG[empty_register].valType==INT16B){/* 2byte signed int, mapped to byte2 */}
                    else if(REG[empty_register].valType==UBYTE2){/* 2 separate unsigned bytes, mapped to ubyte2 */}
                    else if(REG[empty_register].valType==BYTE2){/* 2 separate signed bytes, mapped to byte2 */}
                    else if(REG[empty_register].valType==UBYTE3){/* 3 separate unsigned bytes, mapped to ubyte4 */}
                    else if(REG[empty_register].valType==BYTE3){/* 3 separate signed bytes, mapped to byte4 */}
                    else if(REG[empty_register].valType==UBYTE4){/* 4 separate unsigned bytes, mapped to ubyte4 */}
                    else if(REG[empty_register].valType==BYTE4){/* 4 separate signed bytes, mapped to byte4 */}
                    else if(REG[empty_register].valType==UINT32B){/* 4byte unsigned int, mapped to ubyte4 */}
                    else if(REG[empty_register].valType==INT32B){/* 4byte signed int, mapped to byte4 */}
                    else if(REG[empty_register].valType==FLAG1){}
                    else if(REG[empty_register].valType==FLAG2){/*and so on, all mapped to single bits of a ubyte4, up to 32 */}
          }/* DF end. */
          
          bcp++; /* advance pointer for the next cycle */
     }
     #ifdef DEBUG
          printf("!!! ok I got a terminator, exiting...\n");
          #endif
}


static __inline__ void PUSH(instruction_t in){ /* raise SP of one if possible, and write new value, overwrites the top plate if the stack is full */
     REG_SP -= (REG_SP!=STACK_TOP);
     STACK[REG_SP] = in;}

static __inline__ instruction_t GET(){ /* return the top element from the stack, doesn't move pointer */
     return STACK[REG_SP];}
          
static __inline__ instruction_t POP(){ /* return the top element from the stack, lowers the pointer if possible, doesn't delete the element */
     instruction_t out = STACK[REG_SP];
     REG_SP += (REG_SP!=STACK_BOTTOM);
     return out;}
     
static __inline__ void UP(){ /* raise stack pointer if possible */
     REG_SP -= (REG_SP!=STACK_TOP);}
     
static __inline__ void DN(){ /* lower stack pointer if possible */
     REG_SP += (REG_SP!=STACK_BOTTOM);}
     
static __inline__ void UPP(){ /* raise stack pointer by 2 if possible */
     REG_SP -= 2*(REG_SP!=STACK_TOP);}

static __inline__ void DNN(){ /* lower stack pointer by 2 if possible */
     REG_SP += 2*(REG_SP!=STACK_BOTTOM);}
     
static __inline__ instruction_t detect(char* st){ /* string in >> an instruction is detected and returned */
          #ifndef BRANCH
          return
               DF * startsWith(st, "df ")+
               RDF * startsWith(st, "rdf ")+
               UDF * startsWith(st, "udf ")+
               LD * startsWith(st, "ld ")+
               ULD * startsWith(st, "uld ")+
               HCF * strEqual(st, "hcf")+
               SUM * startsWith(st, "sum ")+
               SUB * startsWith(st, "sub ")+
               MUL * startsWith(st, "mul ")+
               DIV * startsWith(st, "div ")+
               EQ * startsWith(st, "eq ")+
               FRV * startsWith(st, "frv ")+
               IF * startsWith(st, "if ")+
               SAY * startsWith(st, "say ")+
               DUMPSERIAL * strEqual(st, "dumpserial")+
               TAKESERIAL * strEqual(st, "takeserial")+
               LAMBDA * startsWith(st, "lambda ")+
               MU * startsWith(st, "mu ")+
               GO * startsWith(st, "go ")+
               HH * startsWith(st, "hh ")+
               EXTERN * startsWith(st, "extern ")+
               HB * startsWith(st, "hb ")+
               SET * startsWith(st, "set ")+
               LV * startsWith(st, "lv ")+
               DD * startsWith(st, "dd ")+
               PRAGMA * startsWith(st, "pragma ")+
               NOP * strEqual(st, "nop");
          #endif
          #ifdef BRANCH /* todo: mettere gli operatori in ordine di frequenza d'uso */
               if(startsWith(st, "df ")){return DF;}
               else if(startsWith(st, "rdf ")){return RDF;}
               else if(startsWith(st, "udf ")){return UDF;}
               else if(startsWith(st, "ld ")){return LD;}
               else if(startsWith(st, "uld ")){return ULD;}
               else if(startsWith(st, "hcf ")){return HCF;}
               else if(startsWith(st, "sum ")){return SUM;}
               else if(startsWith(st, "sub ")){return SUB;}
               else if(startsWith(st, "mul ")){return MUL;}
               else if(startsWith(st, "div ")){return DIV;}
               else if(startsWith(st, "eq ")){return EQ;}
               else if(startsWith(st, "frv ")){return FRV;}
               else if(startsWith(st, "if ")){return IF;}
               else if(startsWith(st, "say ")){return SAY;}
               else if(strEqual(st, "dumpserial")){return DUMPSERIAL;}
               else if(strEqual(st, "takeserial")){return TAKESERIAL;}
               else if(startsWith(st, "lambda ")){return LAMBDA;}
               else if(startsWith(st, "mu ")){return MU;}
               else if(startsWith(st, "go ")){return GO;}
               else if(startsWith(st, "hh ")){return HH;}
               else if(startsWith(st, "extern ")){return EXTERN;}
               else if(startsWith(st, "hb ")){return HB;}
               else if(startsWith(st, "set ")){return SET;}
               else if(startsWith(st, "lv ")){return LV;}
               else if(startsWith(st, "dd ")){return DD;}
               else if(startsWith(st, "pragma ")){return PRAGMA;}
               else{return NIL;}
          #endif
     }
     
     
     #ifdef BLAK_SCREEN
          static __inline__ void print_valType(valType_t val){
               if(val==EMPTY){printf("EMPTY");}
               else if(val==ADDR){printf("ADDR");}
               else if(val==FVAL){printf("FVAL");}
               else if(val==UBYTE1){printf("UBYTE1");}
               else if(val==BYTE1){printf("BYTE1");}
               else if(val==UINT16B){printf("UINT16B");}
               else if(val==INT16B){printf("INT16B");}
               else if(val==UBYTE2){printf("UBYTE2");}
               else if(val==BYTE2){printf("BYTE2");}
               else if(val==UBYTE3){printf("UBYTE3");}
               else if(val==BYTE3){printf("BYTE3");}
               else if(val==UBYTE4){printf("UBYTE4");}
               else if(val==BYTE4){printf("BYTE4");}
               else if(val==UINT32B){printf("UINT32B");}
               else if(val==INT32B){printf("INT32B");}
               else if(val==FLAG1){printf("FLAG1");}
               else if(val==FLAG2){printf("FLAG2");}
               /*else if(val==){printf("");}*/
          }
     #endif
/* FUNCTION DEFINITIONS end. */
