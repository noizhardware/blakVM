#define BLAK_VERSION "2021d23-2346"

/***

- random access file (in embedded it'll be EEPROM)
http://tutorialtous.com/c/randomaccesstofile.php
fseek() ftell() rewind()

- bytecode is stored in eeprom (or in a file, for a regular computer)
     and then it's loaded into progMem
     data memory and program memory are separated
     HARVARD ARCHITECTURE https://en.wikipedia.org/wiki/Harvard_architecture

     
* 8-bit instructions (set of 256)
* named registers: register_t :
  char name[4]
  valType_t valType
  regValue_t value

- register_t reg[BLAK_REGISTERS_QTY]
- mem_t mem[BLAK_MEM_SIZE]
- bytecode_t progMem[BLAK_PROGMEM_SIZE]
- memPtr_t memPtr = 0
- memPtr_t progMemPtr = 0

- Arduino Micro (ATmega32U4) :
     Flash Memory : 32 kb (4 kb used by bootloader) >> 28 kb
     SRAM 2.5 kb
     EEPROM 1 kb

- instructions refer to register numbers and-or register names to retreive variable data

---

(64x) registers : [1b type][4b name][2b address/data] >> 448b
eg: 32 val a 16bit e 32 a 32bit:
i primi 32 non richiedono mem aggiuntiva
gli altri: 32x 4b >> 128
TOT 576
-- se i secondi 32 sono liste da 10b >> 320b
TOT 768 THIS ONE!!!



*/

/*** TODO
     - cpu(byte in from file[bytecodePtr]) >> do things, and also advance/move bytecodePtr
          6502 ha un data bus bidirezionale, per ricevere opcodes da eseguire, e per mandare fuori roba
          e un output a 16bit con il quale richiede opcodes(come io muovo byteCodePtr e poi leggo l'istruzione da esso puntata)
     - la funzione eval(addr) deve servire per:
          copio una BLAK_EXPR_T in un address in mem
          chiamo eval per quell'indirizzo, e la funzione viene evallata
          esiste gia l'opcode, è BLAK_EVAL
     - unnamed 16bit registers:
          ra rah ral - rx rxh rxl - ry ryh ryl
          load to registers: lla llah, ecc...
     
     - system constants: memsize, registersQty, outpins, I/O memaddr, etc...
     - dual memspace: think again...the program must be loaded directly from secondary memory
     * evalFile -- evaluate a bytecode file bytecode byte by byte, so I don't need a ton of memory
     * evalString -- evaluate a buffer string of bytecodes, this will be used in the REPL, both assembly repl and higher-lang repl

     - JUMPFLAG -- fatto! blak_flags.jumpFlag
          jft --set jumpflag true
          jff --set jumpflag false
          jmp address --unconditional jump
          jmpt address --jump if jumpflag true
          jmpf address --jump if jumpflag false
          jmp regname_t regname BHOOOOOO
     - WMEM address -- per scrivere direttamente in memoria
     - MMPTR address -- move mem pointer to address
     - minc mdec -- increase-decrease mem pointer
     - function declarations
     - routine che libera memory con BLAK_FREEMEM_T
          es:
          [BLAK_RAVENOP_T][ADD][BLAK_REGISTERNAME_T][n][a][NIL][BLAK_REGISTERNAME_T][q][NIL] diventa >>
          [BLAK_FREEMEM_T][9][BLAK_REGISTERNAME_T][n][a][NIL][BLAK_REGISTERNAME_T][q][NIL]
          il 9 è comprensivo anche del blocco blak_freemem. se avanza spazio verrà freememmato, se è da solo, freemem_single
     - memory fragmentation status analysis - use R:\s\c\colprint.c to color trapped free memory bytes
     - integrate SUPAMEM
          - create an unfragmentable memory system:
            you need to choose all functions to natively allocate memory in blocks of the same size. nais. poissibol? dunnoh LoL.
     - integrate ODIN
     - PPU - Physics Processing Unit
          - rigid body
          - soft body
          - fluids
          - knots
          - braids
          - physical modeling(audio)
     - tulpa: bel nome da usare, magari per definire una slave machine
     - arc: avicular raven compiler
     - try:
       + a dbl function
       + fibonacci https://youtu.be/yOyaJXpAYZQ
*/

#ifndef _BLAK2_H_
#define _BLAK2_H_

/*** INCLUDES */
     #include <stdint.h>
     #include <stdbool.h>
     #include "baofiles.h"
     
     #ifdef BLAK_OUT_TERMINAL
          #include <stdio.h>
          #include <wchar.h>
          #include <locale.h>
     #endif
/* INCLUDES end. */

#ifdef __cplusplus
extern "C" {
#endif

/*** DEFINES */

#define BLAK_REG_NAME_LEN 4
#define BLAK_REGISTERS_QTY 48 /* 8bytes X 48 -> 384bytes tot */
#define BLAK_MAX_REGISTERS (REGISTERS_QTY-1)
#define BLAK_MEM_SIZE 575 /* size of virtual memory */
#define BLAK_MAX_MEM (BLAK_MEM_SIZE-1) /* memory array range is [0..size-1] */
#define BLAK_PROGMEM_SIZE 575 /* size of virtual program memory */

/* TODO: ci devono essere dei defined anche all'interno del codice blak stesso, in modo da identificare su che piattaforma sto girando */
#define BLAK_PLATFORM_WIN10
/*#define BLAK_PLATFORM_NYX*/

#define BLAK_SIN_AVAILABLE /* regular sinewave engine, not available on nyx */
#define BLAK_SIN1_AVAILABLE /* 1-bit sinewave engine, available on nyx */
/* DEFINES end. */

/*** TYPEDEFS */
typedef uint16_t regData_t; /* max 64k memory addressing OR uint16_t value */
typedef enum __attribute__ ((__packed__)) valType_t_{ /* __packed__ keeps the size of the enum as small as possible (1byte in this case) */
     BLAK_EMPTY_T, /* register is empty (when you want to reset a register, no need to wipe it, just set its valType to EMPTY) */
     BLAK_UNDEFINED_T, /* esiste ma non ha ancora un valore definito */
     BLAK_NONEXISTENT_T, /* non esistente, per i return value di not found error */
     BLAK_ADDR_T, /* 16bit, mapped to addr */
     BLAK_FUNC_T, /* 16bit, mapped to addr, contains the address of auser-defined function - made with lambda */
     BLAK_RAVENOP_T, /* raven operator */
     BLAK_UINT8_T,
     BLAK_INT8_T,
     BLAK_UINT16_T,
     BLAK_INT16_T,

     BLAK_LIST_T, /* generic list, mapped to addr */

     BLAK_BOOL_T,

     BLAK_ERROR_FAIL_T,
     BLAK_ERROR_SUCCESS_T,
     BLAK_ERROR_NEVERRAN_T, /* error must be initialized to this value, if this is returned, the errorstate was untouched */
     /* todo: altri tipi di errori */

     BLAK_FREEMEM_T, /* followed by size of freeable memory */
     BLAK_FREEMEMSINGLE_T, /* if it's a single free byte */

     BLAK_UINT8D_T,/* 2 separated unsigned bytes, mapped to dual */
     BLAK_INT8D_T,/* 2 separated signed bytes, mapped to dual */
     /* todo: dual e triple di qualsiasi roba, tutti mappati su address */

     BLAK_UINT24_T, /* uint24_t, put in memory and store its address */
     BLAK_INT24_T, /* int24_t, put in memory and store its address */
     BLAK_UINT32_T, /* uint32_t, put in memory and store its address */
     BLAK_INT32_T, /* int32_t, put in memory and store its address */
     /* todo: vie di mezzo? UINT40, ecc? */
     BLAK_UINT64_T, /* uint64_t, put in memory and store its address */
     BLAK_INT64_T, /* int64_t, put in memory and store its address */
     BLAK_FLOAT_T, /* float, put in memory and store its address */

     BLAK_FLAG1_T, /*and so on, all mapped to single bits of flags , up to 16 */
     BLAK_FLAG2_T,
     BLAK_FLAG3_T,
     BLAK_FLAG4_T,
     BLAK_FLAG5_T,
     BLAK_FLAG6_T,
     BLAK_FLAG7_T,
     BLAK_FLAG8_T,
     BLAK_FLAG9_T,
     BLAK_FLAG10_T,
     BLAK_FLAG11_T,
     BLAK_FLAG12_T,
     BLAK_FLAG13_T,
     BLAK_FLAG14_T,
     BLAK_FLAG15_T,
     BLAK_FLAG16_T,

     BLAK_LONGNAME_T, /* can contain the address of the continuation of a name, and follow some structure after that address to contain the rest of the data - map to addr*/

     BLAK_NOTHING_T, /* Nothing monad (probably monadic types shouldn't be here, they should be on top of regular types, so I can have "JUST INT32B") */
     /* la cosa giusta sarebbe che ogni variabile ha DUE types: uno usato per i tipi normali e uno monadico */
     /* oppure li metto tutti qui gia con le combo, tanto ho spazio: UINT16NOTHING, UINT8NOTHING, eccetera... */
     BLAK_JUST_T, /* Just monad */

     BLAK_EXPR_T, /* expression, mapped to addr -- denotes something that needs evaluation, can also point to a lambda */

     BLAK_REGISTERNAME_T, /* name of a register, mapped to nothing, name will follow in the bytecode */
     BLAK_REGISTERNUMBER_T, /* number of a register, mapped to regNum */

     BLAK_LAMBDAARG_T, /* name of a lambda argument, mapped to nothing, name will follow in the bytecode */

     BLAK_TYPES_QTY /* just to count how many instructions I have at the moment */
} valType_t;


typedef uint8_t mem_t; /* size of virtual memory elements */
typedef uint16_t memPtr_t; /* max 64k virtual memory */

typedef union regVal_t_{ /* 16bit */
     memPtr_t  addr; /* address */
     uint8_t   regNum; /* cacca: max 256 registers, need to change type if I want more registers */

     uint8_t   uint8;
     int8_t    int8;
     uint16_t  uint16;
     int16_t   int16;
     uint8_t   boool;
     uint16_t  dual;

     uint16_t  flags;
     /*uint32_t   ubyte4;*/
     /*int32_t    byte4;*/
} regVal_t;

typedef uint8_t regName_t[BLAK_REG_NAME_LEN]; /* 4byte : array for variable name */

typedef struct register_t_{ /* aligned to 8 bytes - min align 2bytes on win10 */
  regName_t name; /* 4byte : array for variable name */
  regVal_t value; /* 2bytes : value/address */
  valType_t type; /* 1byte : type of the variable (see valType_t), determines what type of data is stored in .value */
  uint8_t type2; /* cacca: unused, can be used for type2 if I step-up my types system with subtypes */
} register_t;

typedef struct anon_t_{ /* anonymous value, with type - aligned to 4bytes */
  valType_t type; /* 1byte : type of the variable (see valType_t), determines what type of data is stored in .value */
  valType_t type2; /* cacca: unused, can be used for type2 if I step-up my types system with subtypes */
  regVal_t value; /* 2bytes : value/address */
} anon_t;

typedef enum __attribute__ ((__packed__)) bytecode_t_{ /* __packed__ keeps the size of the enum as small as possible (1byte in this case) */
     NIL,

     BLAK_DEFINE, /* cacca: right now I CAN define two regs with the same name LoL */
     BLAK_REDEFINE,
     BLAK_UNDEFINE, /* clear */
     BLAK_LOAD,
     BLAK_UNLOAD,
     BLAK_HCF,

     BLAK_SET_NAME,
     BLAK_SET_TYPE,
     BLAK_SET_VALUE,

     /* useful to know what type of raw value to expect next */
     BLAK_BYTE1, /* single byte */
     BLAK_BYTE2,
     BLAK_BYTE3,
     BLAK_BYTE4,
     BLAK_BYTE5,
     BLAK_BYTE6,
     BLAK_BYTE7,
     BLAK_BYTE8,
     BLAK_BYTE9,
     BLAK_BYTE10,
     BLAK_BYTE11,
     BLAK_BYTE12,
     BLAK_BYTE13,
     BLAK_BYTE14,
     BLAK_BYTE15,
     BLAK_BYTE16, /* veri looooooong */

     BLAK_ADD,
     BLAK_SUB,
     BLAK_MUL,
     BLAK_DIV,
     BLAK_EQUAL,
     BLAK_NOTEQUAL,
     BLAK_FOREVER,
     BLAK_IF,
     BLAK_EVAL, /* eval address to exec something! */

     BLAK_SAY, /* it's like print */
     BLAK_SHOWREGS,
     BLAK_SHOWMEM,

     BLAK_LAMBDA,
          BLAK_LAMBDA_A, /* separate function args from function body */
          BLAK_LAMBDA_B, /* terminator of function body */
     BLAK_MU,
          BLAK_MU_A,
          BLAK_MU_B,
          BLAK_MU_END,
     BLAK_GO,
     BLAK_HALT,
     BLAK_EXTERN, /* for .c or .h external routines -- forse non ha senso... */
     BLAK_HEARTBEAT,
     BLAK_SET,
     BLAK_LIVE,
     BLAK_CODING,

     BLAK_PRAGMA,

     BLAK_NOP, /* no operation, just an empty op */

     BLAK_ENDOFBYTECODE, /* used to terminate a sequence of bytecodes, like the '\0' at the end of a string */
     BLAK_ENDOFLIST,
     BLAK_ENDOFEXPR,

     BLAK_DUMPSERIAL,
     BLAK_TAKESERIAL,

     BLAK_BYTECODES_QTY /* just to count how many instructions I have at the moment */
} bytecode_t;

/* TYPES end. */

/*** GLOBALS */
     static register_t reg[BLAK_REGISTERS_QTY] /*= {{'\0', '\0', '\0', '\0'}, EMPTY, 0}*/;
     static mem_t mem[BLAK_MEM_SIZE];
     static bytecode_t progMem[BLAK_PROGMEM_SIZE];
     static memPtr_t memPtr = 0; /* the real and only: points to the first free virtual mem byte */
     static memPtr_t progMemPtr = 0; /* instruction pointer */
     
     /* define a structure with bit fields */
     #define BLAK_FLAGS_QTY 16
     static struct {
          bool jumpFlag : 1;
          bool anotherFlag1 : 1;
          bool anotherFlag2 : 1;
          bool anotherFlag3 : 1;
          bool anotherFlag4 : 1;
          bool anotherFlag5 : 1;
          bool anotherFlag6 : 1;
          bool anotherFlag7 : 1;
          bool anotherFlag8 : 1;
          bool anotherFlag9 : 1;
          bool anotherFlag10 : 1;
          bool anotherFlag11 : 1;
          bool anotherFlag12 : 1;
          bool anotherFlag13 : 1;
          bool anotherFlag14 : 1;
          bool anotherFlag15 : 1;
     } blak_flags = {
          false,
          false,
          false,
          false,
          false,
          false,
          false,
          false,
          false,
          false,
          false,
          false,
          false,
          false,
          false,
          false
     };
/* GLOBALS end. */

/*** FUNCTION DECLARATIONS */
     static __inline__ anon_t getReg(regName_t regName);
     static __inline__ uint8_t getRegNum(regName_t regName);
     static __inline__ void readRegName(regName_t regName);
     static __inline__ anon_t eval();
     #ifdef BLAK_OUT_TERMINAL
          static __inline__ void print_valType(valType_t val);
          static __inline__ void print_regName(regName_t regName);
          static __inline__ void errorPrint(anon_t err);
     #endif /* BLAK_OUT_TERMINAL */
     #ifdef BLAK_PC_FILESYSTEM
          static __inline__ void loadProg(char* fileName);
     #endif /* BLAK_PC_FILESYSTEM */
          
     
/* FUNCTION DECLARATIONS end. */


/*** FUNCTION DEFINITIONS */

#ifdef BLAK_PC_FILESYSTEM
     static __inline__ void loadProg(char* fileName){
          fileRead_nomalloc(fileName, (char*)(progMem+progMemPtr));
          /* cacca: no error management */
     }
#endif /* BLAK_PC_FILESYSTEM */

#ifdef BLAK_OUT_TERMINAL
     static __inline__ void errorPrint(anon_t err){
          if(err.type==BLAK_ERROR_FAIL_T){
               printf("FAIL");
          }
          else if(err.type==BLAK_ERROR_SUCCESS_T){
               printf("SUCCESS");
          }
          else if(err.type==BLAK_ERROR_NEVERRAN_T){
               printf("NEVERRAN");
          }
          else{
               printf("::non an error message::");
          }
     }

#endif

static __inline__ anon_t getReg(regName_t regName_in){
     uint8_t i;
     uint8_t j;
     anon_t retVal;
     for(i=0;i<BLAK_REGISTERS_QTY;i++){
          for(j=0;j<BLAK_REG_NAME_LEN;j++){
               if(reg[i].name[j]==regName_in[j]){
                    if(reg[i].name[j]==NIL || j==3){
                         retVal.type=reg[i].type;
                         retVal.value=reg[i].value; /* cacca: deve essere diverso a seconda del type ??? */
                         return retVal;
                    }
               }
               else{
                    break;
               }
          }
     }
     retVal.type = BLAK_NONEXISTENT_T;
     retVal.value.uint8 = 0;
     return retVal;
}

/*
returns 0 if not found, regNum+1 if found, so you neeg to use regNum-1 in your code
usage:
     regNum = getRegNum(regName);
     if(regNum){
          codecodecode using (regNum-1)
     }
*/
static __inline__ uint8_t getRegNum(regName_t regName_in){
     uint8_t i = 0;
     uint8_t j = 0;
     for(i=0;i<BLAK_REGISTERS_QTY;i++){
          for(j=0;j<BLAK_REG_NAME_LEN;j++){
               if(reg[i].name[j]==regName_in[j]){
                    if(reg[i].name[j]==NIL || j==3){
                         return i+1; /* 0 is reserved for error */
                    }
               }
               else{
                    break;
               }
          }
     }
     return 0;
}

static __inline__ void readRegName(regName_t regName_in){ /* fills regName_in with the name of the register currently pointed at by the current instruction */
     uint8_t i;
     for(i=0;i<BLAK_REG_NAME_LEN;i++){
          regName_in[i]=progMem[progMemPtr];
          if(progMem[progMemPtr]==NIL){
               break;
          }
          progMemPtr++;
     }
}


static __inline__ anon_t eval(){
     /*uint16_t bcp = 0;*/ /* bytecode pointer *//*kak*/
     uint16_t j = 0;
     uint16_t empty_register = 0;
     /*unsigned char name[REG_NAME_LEN]={'\0', '\0', '\0', '\0'};*/
     uint8_t name[BLAK_REG_NAME_LEN];
     anon_t returnValue;

     returnValue.type=BLAK_ERROR_NEVERRAN_T;

     while(progMem[progMemPtr]!=BLAK_ENDOFBYTECODE){
          #ifdef DEBUG
               printf("!!!not a BLAK_ENDOFBYTECODE: %d\n", progMem[progMemPtr]);
               #endif
          if(progMem[progMemPtr]==BLAK_DEFINE){ /* BLAK_DEFINE name type value */
               #ifdef DEBUG
                    printf("!!!BLAK_DEFINE\n");
                    #endif
               /* let's find the first empty register */
               empty_register=0;
               while(reg[empty_register].type!=BLAK_EMPTY_T){
                    empty_register++;} /* found the first empty register, empty_register now points to it - cacca: e se so tutti pieni? */
               progMemPtr++; /* advance pointer, will now point to the DF variable name - cacca: manage errors, se il bytecode finisce? */
               /* now we start reading the NAME of the define, it can be an unterminated 4-byte sequence, or a NIL-terminated 1, 2 or 3 bytes sequence */
               memset(name, '\0', BLAK_REG_NAME_LEN); /* reset name[] */

               for(j=0;j<BLAK_REG_NAME_LEN;j++){
                    if(progMem[progMemPtr]!=NIL){
                         #ifdef DEBUG
                              printf(">>>>[%u]%c\n", progMemPtr, progMem[progMemPtr]);
                              #endif
                         name[j]=progMem[progMemPtr];
                         progMemPtr+=(j<(BLAK_REG_NAME_LEN-1));
                    }
               }

               /* name[] is now containing the variable name */
               #ifdef DEBUG
                    printf("!!!define name: %c%c%c%c\n", name[0], name[1], name[2], name[3]);
                    #endif

               #ifdef DEBUG
                    printf("!!!now pointing at: %d\n", progMem[progMemPtr]);
                    #endif
               /* now I will put name, type and value in the empty register */
               for(j=0;j<BLAK_REG_NAME_LEN;j++){ /* write variable name in register */
                    reg[empty_register].name[j]=name[j];}
               progMemPtr++; /* advance bytecode pointer, now pointing to valType - cacca: manage errors */
               #ifdef DEBUG
                    printf("!!!value type: ");
                    print_valType(progMem[progMemPtr]);
                    printf("\n");
                    #endif
               reg[empty_register].type = progMem[progMemPtr]; /* write the value type into register */
               progMemPtr++; /* advance pointer, to point to the first byte of the actual value */
               #ifdef DEBUG
                    printf("!!! raw value: %d\n", progMem[progMemPtr]);
                    #endif
               /* now write the value into the register, according to its type */
               if(reg[empty_register].type==BLAK_ADDR_T){/* 16bit, mapped to addr */}
               else if(reg[empty_register].type==BLAK_FLOAT_T){/* float, put in memory and store its address */}
               else if(reg[empty_register].type==BLAK_UINT8_T){reg[empty_register].value.uint8=progMem[progMemPtr];}
               else if(reg[empty_register].type==BLAK_INT8_T){reg[empty_register].value.int8=progMem[progMemPtr];}
               else if(reg[empty_register].type==BLAK_UINT16_T){
                    uint16_t temp;
                    temp = (uint16_t)progMem[progMemPtr]<<8; /* MSB */
                    progMemPtr++;
                    temp += (uint16_t)progMem[progMemPtr]; /* LSB */
                    reg[empty_register].value.uint16=temp;
               }
               else if(reg[empty_register].type==BLAK_INT16_T){reg[empty_register].value.int16=progMem[progMemPtr];}
               else if(reg[empty_register].type==BLAK_UINT24_T){/* put in memory and store its address */}
               else if(reg[empty_register].type==BLAK_INT24_T){/* put in memory and store its address */}
               else if(reg[empty_register].type==BLAK_UINT32_T){/* put in memory and store its address */}
               else if(reg[empty_register].type==BLAK_INT32_T){/* put in memory and store its address */}
               else if(reg[empty_register].type==BLAK_UINT64_T){/* put in memory and store its address */}
               else if(reg[empty_register].type==BLAK_INT64_T){/* put in memory and store its address */}

               else if(reg[empty_register].type==BLAK_UINT8D_T){/* 2 separate unsigned bytes, mapped to dual */}
               else if(reg[empty_register].type==BLAK_INT8D_T){/* 2 separate signed bytes, mapped to dual */}

               /* all these should exist, and be mapped to addr */
               /*else if(reg[empty_register].type==BLAK_UBYTE3_T){}
               else if(reg[empty_register].type==BLAK_BYTE3_T){}
               else if(reg[empty_register].type==BLAK_UBYTE4_T){}
               else if(reg[empty_register].type==BLAK_BYTE4_T){}
               else if(reg[empty_register].type==BLAK_UINT32B_T){}
               else if(reg[empty_register].type==BLAK_INT32B_T){}*/

               else if(reg[empty_register].type==BLAK_FLAG1_T){/* all mapped to single bits of "flags" */}
               else if(reg[empty_register].type==BLAK_FLAG2_T){}
               else if(reg[empty_register].type==BLAK_FLAG3_T){}
               else if(reg[empty_register].type==BLAK_FLAG4_T){}
               else if(reg[empty_register].type==BLAK_FLAG5_T){}
               else if(reg[empty_register].type==BLAK_FLAG6_T){}
               else if(reg[empty_register].type==BLAK_FLAG7_T){}
               else if(reg[empty_register].type==BLAK_FLAG8_T){}
               else if(reg[empty_register].type==BLAK_FLAG9_T){}
               else if(reg[empty_register].type==BLAK_FLAG10_T){}
               else if(reg[empty_register].type==BLAK_FLAG11_T){}
               else if(reg[empty_register].type==BLAK_FLAG12_T){}
               else if(reg[empty_register].type==BLAK_FLAG13_T){}
               else if(reg[empty_register].type==BLAK_FLAG14_T){}
               else if(reg[empty_register].type==BLAK_FLAG15_T){}
               else if(reg[empty_register].type==BLAK_FLAG16_T){}

               else if(reg[empty_register].type==BLAK_EXPR_T){
                    reg[empty_register].value.addr = memPtr; /* the named register will point to the data I'm going to write to virtual mem */
                    /*progMemPtr++;*/
                    for(;;){ /* write data into virtual mem */
			if(memPtr>BLAK_MAX_MEM){ break;} /* shitty error management */
                         mem[memPtr]=progMem[progMemPtr]; /* kak cacca: not checking if there's enough space */
                         memPtr++; /* point to next free byte */
                         if(progMem[progMemPtr]==BLAK_ENDOFEXPR){
                              break;
                         }
                         progMemPtr++;
                    }
               }
               else{
                    #ifdef BLAK_OUT_TERMINAL
                         printf("==:: ERROR: memPtr>BLAK_MAX_MEM\n");
                         #endif
                    returnValue.type=BLAK_ERROR_FAIL_T;
               }

               returnValue.type=BLAK_ERROR_SUCCESS_T;
          }/* if(progMem[progMemPtr]==BLAK_DEFINE){ */

          else if(progMem[progMemPtr]==BLAK_UNDEFINE){ /* BLAK_UNDEFINE name */
               regName_t regName = {NIL, NIL, NIL, NIL}; /* it's a pointer, needs initialization */
               uint8_t regNum;

               progMemPtr++;
               readRegName(regName);
                    /*printf("==================== ");
                    print_regName(regName);
                    printf("\n");*/
               regNum = getRegNum(regName);
               if(regNum){ /* if regNum is 0, it's an error */
                    reg[regNum-1].type = BLAK_EMPTY_T;
                    returnValue.type=BLAK_ERROR_SUCCESS_T;
               }
               else{
                    #ifdef BLAK_OUT_TERMINAL
                         printf("==:: ERROR: regNum is 0\n");
                         #endif
                    returnValue.type=BLAK_ERROR_FAIL_T;
               }
          } /* UNDEFINE name */

          else if(progMem[progMemPtr]==BLAK_SET_NAME){ /* SET_NAME(regNum, name) */
               uint8_t regNum;
               uint8_t i;

               progMemPtr++;
               regNum=progMem[progMemPtr];
               progMemPtr++;
               for(i=0;i<BLAK_REG_NAME_LEN;i++){
                    reg[regNum].name[i]=progMem[progMemPtr];
                    if(progMem[progMemPtr]==NIL){
                         break;
                    }
                    progMemPtr++;
               }
               returnValue.type=BLAK_ERROR_SUCCESS_T;
          } /* SET_NAME(regNum, name) */

          else if(progMem[progMemPtr]==BLAK_SET_TYPE){ /* SET_TYPE(regNum, type) */
               uint8_t regNum;

               progMemPtr++;
               regNum=progMem[progMemPtr];
               progMemPtr++;
               reg[regNum].type=progMem[progMemPtr];
               returnValue.type=BLAK_ERROR_SUCCESS_T;
          } /* SET_TYPE(regNum, type) */

          else if(progMem[progMemPtr]==BLAK_SET_VALUE){ /* SET_VALUE(regNum, mode, val) */
               uint8_t regNum;
               bytecode_t mode;

               progMemPtr++;
               regNum=progMem[progMemPtr];
               progMemPtr++;
               mode=progMem[progMemPtr];
               progMemPtr++;
               if(mode==BLAK_BYTE1){
                    reg[regNum].value.uint8=progMem[progMemPtr];
               }
               else if(mode==BLAK_BYTE2){
                    uint16_t newVal = progMem[progMemPtr]<<8; /* get MSB */
                    progMemPtr++;
                    newVal |= progMem[progMemPtr]; /* get LSB */
                    reg[regNum].value.uint16=newVal;
               }
               else{
                    #ifdef BLAK_OUT_TERMINAL
                         printf("==:: ERROR: unrecognized type: mode==%d\n", mode);
                         #endif
                    returnValue.type=BLAK_ERROR_FAIL_T;
               }
               returnValue.type=BLAK_ERROR_SUCCESS_T;
          } /* SET_VALUE(regNum, mode, val) */

          #ifdef BLAK_OUT_TERMINAL
               else if(progMem[progMemPtr]==BLAK_SAY){ /* print to screen */ /* cacca: forse questa presuppone l'esistenza di eval, ma eval quello vero? */
                    progMemPtr++;
                    if((valType_t)progMem[progMemPtr]==BLAK_ADDR_T){
                         uint8_t up;
                         uint8_t dn;
                         progMemPtr++;
                         up = progMem[progMemPtr];
                         progMemPtr++;
                         dn = progMem[progMemPtr];
                         printf(">> addr[%u] >> ", (up<<8)+dn);
                         progMemPtr++;
                         /* cacca : qui dovrei mettere il ricorsivo "say next-byte" */
                    }
                    else if((valType_t)progMem[progMemPtr]==BLAK_UINT8_T){
                         progMemPtr++;
                         printf(">> [BLAK_UINT8_T] %u\n", progMem[progMemPtr]);
                    }
                    else if((valType_t)progMem[progMemPtr]==BLAK_INT8_T){
                         progMemPtr++;
                         printf(">> [BLAK_INT8_T] %d\n", progMem[progMemPtr]);
                    }
                    else if((valType_t)progMem[progMemPtr]==BLAK_REGISTERNAME_T){
                         regName_t regName = {NIL, NIL, NIL, NIL}; /* it's a pointer, needs initialization */
                         uint8_t i;
                         progMemPtr++;
                         for(i=0;i<BLAK_REG_NAME_LEN;i++){
                              regName[i]=progMem[progMemPtr];
                              if(progMem[progMemPtr]==NIL){
                                   break;
                              }
                              progMemPtr++;
                         }
                         printf(">> [BLAK_REGISTERNAME_T] \"");
                         print_regName(regName);
                         printf("\" >> ");
                         if(getRegNum(regName)!=0){
                              printf("reg[%u] >> ", getRegNum(regName)-1); /* 0 is reserved for errors */
                         }
                         if(getReg(regName).type==BLAK_NONEXISTENT_T){
                              printf("[BLAK_NONEXISTENT_T]\n");
                         }
                         else{
                              printf("[");
                              print_valType(getReg(regName).type);
                              /* cacca : qui dovrei mettere il ricorsivo "say qualcosa che stampo a seconda di che type è" */
                              printf("] %d\n", getReg(regName).value.uint8);
                         }
                    }

                    returnValue.type=BLAK_ERROR_SUCCESS_T;
               } /* SAY - print to screen */

               else if(progMem[progMemPtr]==BLAK_SHOWREGS){ /* show registers */
                    uint16_t i;
                    printf("\n== REGISTERS (%u x %u bytes) >> %u bytes:\n", BLAK_REGISTERS_QTY, sizeof(register_t), sizeof(reg));
                    for(i=0;i<BLAK_REGISTERS_QTY;i++){
                         printf("== reg[%02d] >> \"%c%c%c%c\" [", i, reg[i].name[0], reg[i].name[1], reg[i].name[2], reg[i].name[3]);
                         print_valType(reg[i].type);
                         if(reg[i].type==BLAK_UINT8_T || reg[i].type==BLAK_EXPR_T || reg[i].type==BLAK_EMPTY_T){
                              printf("] %d\n", reg[i].value.uint8);
                         }
                         else if(reg[i].type==BLAK_UINT16_T){
                              printf("] %d\n", reg[i].value.uint16);
                         }
                    }
               } /* show registers */

               else if(progMem[progMemPtr]==BLAK_SHOWMEM){ /* show virtual memory */
                    uint32_t i; /* 32bits, so it can handle huuuuge memsizes */
                    uint8_t memString[17];
                    uint8_t widecharIndex;

                    memString[16]='\0'; /* terminate string */
                    printf("\n== VIRTUAL MEMORY (%u bytes) ptr:[%u] (%.2f%% free)\n", BLAK_MEM_SIZE, memPtr, (float)(BLAK_MEM_SIZE-memPtr)/BLAK_MEM_SIZE*100);
                    /*printf("(00)(01)(02)(03)(04)(05)(06)(07)(08)(09)(10)(11)(12)(13)(14)(15)\n");*/
                    for(i=0;i<BLAK_MEM_SIZE;i++){
                         if(i%16==0){
                              printf(" ");
                         }
                         if(i==memPtr){ printf("\b[");}
                         /*printf("%02X", mem[i]);*/ /* hex */
                         printf("%3u", mem[i]); /* decimal */
                         if(i==memPtr){ printf("] ");}
                              else{ printf("  ");}
                         /*memString[i%16]= ((mem[i]>31 && mem[i]<127) || mem[i]>160) ? mem[i] : '.';*/
                         memString[i%16]= mem[i];
                         if(i%16==15){
                              /*printf("\b(%u) (%u-%u) %04X-%04X : ", i-15, i-15, i, i-15, i);*/
                              printf("\b(%04u-%04u) %04X-%04X : ", i-15, i, i-15, i);
                              for(widecharIndex=0;widecharIndex<16;widecharIndex++){
                                   if(memString[widecharIndex]==10){/* skip newline */
                                        printf("*");
                                   }
                                   else{
                                        wprintf(L"%lc", (wchar_t)memString[widecharIndex]);
                                   }
                              }
                              printf("\n");
                         }
                    }
                    
                    if(i%16!=16){
                         uint16_t k;
                         for(k=0;k<(16-(i%16));k++){
                              printf("...  ");
                         }
                         /*printf("\b(%u) (%u-%u) %04X-%04X : ", i-15, i-15, i, i-15, i);*/
                         printf("\b(%04u-%04u) %04X-%04X : ", i-(i%16), i+15-(i%16), i-(i%16), i+15-(i%16));
                         for(widecharIndex=0;widecharIndex<(i%16);widecharIndex++){
                              if(memString[widecharIndex]==10){/* skip newline */
                                   printf("*");
                              }
                              else{
                                   wprintf(L"%lc", (wchar_t)memString[widecharIndex]);
                              }
                         }
                         /*printf("\n");*/
                    }
                    printf("\n");
                    
                    memString[16]='\0'; /* terminate string */
                    printf("\n== VIRTUAL PROGRAM MEMORY (%u bytes) ptr:[%u] (%.2f%% free)\n", BLAK_PROGMEM_SIZE, progMemPtr, (float)(BLAK_PROGMEM_SIZE-progMemPtr)/BLAK_PROGMEM_SIZE*100);
                    /*printf("(00)(01)(02)(03)(04)(05)(06)(07)(08)(09)(10)(11)(12)(13)(14)(15)\n");*/
                    for(i=0;i<BLAK_PROGMEM_SIZE;i++){
                         if(i%16==0){
                              printf(" ");
                         }
                         if(i==progMemPtr){ printf("\b[");}
                         /*printf("%02X", progMem[i]);*/ /* hex */
                         printf("%3u", progMem[i]); /* decimal */
                         if(i==progMemPtr){ printf("] ");}
                              else{ printf("  ");}
                         memString[i%16]= progMem[i];
                         if(i%16==15){
                              /*printf("\b(%u) (%u-%u) %04X-%04X : ", i-15, i-15, i, i-15, i);*/
                              printf("\b(%04u-%04u) %04X-%04X : ", i-15, i, i-15, i);
                              for(widecharIndex=0;widecharIndex<16;widecharIndex++){
                                   if(memString[widecharIndex]==10){/* skip newline */
                                        printf("*");
                                   }
                                   else{
                                        wprintf(L"%lc", (wchar_t)memString[widecharIndex]);
                                   }
                              }
                              printf("\n");
                         }
                    }
                    
                    if(i%16!=16){
                         uint16_t k;
                         for(k=0;k<(16-(i%16));k++){
                              printf("...  ");
                         }
                         /*printf("\b(%u) (%u-%u) %04X-%04X : ", i-15, i-15, i, i-15, i);*/
                         printf("\b(%04u-%04u) %04X-%04X : ", i-(i%16), i+15-(i%16), i-(i%16), i+15-(i%16));
                         for(widecharIndex=0;widecharIndex<(i%16);widecharIndex++){
                              if(memString[widecharIndex]==10){/* skip newline */
                                   printf("*");
                              }
                              else{
                                   wprintf(L"%lc", (wchar_t)memString[widecharIndex]);
                              }
                         }
                         /*printf("\n");*/
                    }
                    printf("\n");
               } /* show virtual memories */

          #endif /* BLAK_OUT_TERMINAL */

          else{
               #ifdef BLAK_OUT_TERMINAL
                    printf("==:: ERROR: unrecognized bytecode: [%u]%u\n", progMemPtr, progMem[progMemPtr]);
                    /*TODO: sprintf the error message to a buffer, to be displayed ONLY if the function actually returns error at the end */
                    #endif
               returnValue.type=BLAK_ERROR_FAIL_T;
          }

          progMemPtr++; /* advance pointer for the next cycle */
     } /* while(progMem[progMemPtr]!=ENDOFBYTECODE){ */
     #ifdef DEBUG
          printf("!!! ok I got a BLAK_ENDOFBYTECODE, exiting...\n");
          #endif
     /*progMemPtr++;*/ /* so now pointer will point at the BLAK_ENDOFBYTECODE */
     return returnValue;
} /* eval */

#ifdef BLAK_OUT_TERMINAL
     static __inline__ void print_valType(valType_t val){
          if(val==BLAK_EMPTY_T){printf("BLAK_EMPTY_T");}
          else if(val==BLAK_ADDR_T){printf("BLAK_ADDR_T");}
          else if(val==BLAK_FLOAT_T){printf("BLAK_FLOAT_T");}
          else if(val==BLAK_UINT8_T){printf("BLAK_UINT8_T");}
          else if(val==BLAK_INT8_T){printf("BLAK_INT8_T");}
          else if(val==BLAK_UINT16_T){printf("BLAK_UINT16_T");}
          else if(val==BLAK_INT16_T){printf("BLAK_INT16_T");}
          else if(val==BLAK_UINT8D_T){printf("BLAK_UINT8D_T");}
          else if(val==BLAK_INT8D_T){printf("BLAK_INT8D_T");}
          else if(val==BLAK_FLAG1_T){printf("BLAK_FLAG1_T");}
          else if(val==BLAK_FLAG2_T){printf("BLAK_FLAG2_T");}
          else if(val==BLAK_FLAG3_T){printf("BLAK_FLAG_T");}
          else if(val==BLAK_FLAG4_T){printf("BLAK_FLAG_T");}
          else if(val==BLAK_FLAG5_T){printf("BLAK_FLAG_T");}
          else if(val==BLAK_FLAG6_T){printf("BLAK_FLAG_T");}
          else if(val==BLAK_FLAG7_T){printf("BLAK_FLAG_T");}
          else if(val==BLAK_FLAG8_T){printf("BLAK_FLAG_T");}
          else if(val==BLAK_FLAG9_T){printf("BLAK_FLAG_T");}
          else if(val==BLAK_FLAG10_T){printf("BLAK_FLAG10_T");}
          else if(val==BLAK_FLAG11_T){printf("BLAK_FLAG11_T");}
          else if(val==BLAK_FLAG12_T){printf("BLAK_FLAG12_T");}
          else if(val==BLAK_FLAG13_T){printf("BLAK_FLAG13_T");}
          else if(val==BLAK_FLAG14_T){printf("BLAK_FLAG14_T");}
          else if(val==BLAK_FLAG15_T){printf("BLAK_FLAG15_T");}
          else if(val==BLAK_FLAG16_T){printf("BLAK_FLAG16_T");}
          else if(val==BLAK_EXPR_T){printf("BLAK_EXPR_T");}
          else if(val==BLAK_REGISTERNAME_T){printf("BLAK_REGISTERNAME_T");}
          else if(val==BLAK_REGISTERNUMBER_T){printf("BLAK_REGISTERNUMBER_T");}
     }
     static __inline__ void print_regName(regName_t regName){
          uint8_t i;
          for(i=0;i<BLAK_REG_NAME_LEN;i++){
               if(regName[i]==NIL){
                    break;
               }
               printf("%c", regName[i]);
          }
     }

#endif /* BLAK_OUT_TERMINAL */

/* FUNCTION DEFINITIONS end. */

#ifdef __cplusplus
}
#endif
#endif /* _BLAK2_H_ */
