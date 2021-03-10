/***

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
     - JUMPFLAG -- fatto! blak_flags.jumpFlag
          sjt set jumpflag true
          sjf set jumpflag false
          jmp address unconditional jump
          jmpt
          jmpf
          jmp regname_t regname
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
     - PPU - Physics Processing Unit
          - rigid body
          - soft body
          - fluids
          - knots
          - braids
          - physical modeling(audio)
     - tulpa: bel nome da usare, magari per definire una slave machine

*/

#ifndef _BLAK2_H_
#define _BLAK2_H_

/*** INCLUDES */
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
#define BLAK_VERSION "2021c10-2241"

#define BLAK_REG_NAME_LEN 4
#define BLAK_REGISTERS_QTY 48
#define BLAK_MAX_REGISTERS (REGISTERS_QTY-1)
#define BLAK_MEM_SIZE 639 /* size of virtual memory */
#define BLAK_MAX_MEM (BLAK_MEM_SIZE-1) /* memory array range is [0..size-1] */

/* TODO: ci devono essere dei defined anche all'interno del codice blak stesso, in modo da identificare su che piattaforma sto girando */
#define BLAK_PLATFORM_WIN10
/*#define BLAK_PLATFORM_NYX*/

#define BLAK_SIN_AVAILABLE /* regular sinewave engine, not available in nyx */
#define BLAK_SIN1_AVAILABLE /* 1-bit sinewave engine, available in nyx */
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
     static memPtr_t memPtr = 0; /* the real and only: points to the first free virtual mem byte */
     /* define a structure with bit fields */
     #define BLAK_FLAGS_QTY 8
     static struct {
          bool jumpFlag : 1;
          bool anotherFlag1 : 1;
          bool anotherFlag2 : 1;
          bool anotherFlag3 : 1;
          bool anotherFlag4 : 1;
          bool anotherFlag5 : 1;
          bool anotherFlag6 : 1;
          bool anotherFlag7 : 1;
     } blak_flags = {
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
     static __inline__ void readRegName(bytecode_t byteCode[], uint16_t* bcp, regName_t regName);
     static __inline__ anon_t eval(bytecode_t byteCode[]);
     #ifdef BLAK_OUT_TERMINAL
          static __inline__ void print_valType(valType_t val);
          static __inline__ void print_regName(regName_t regName);
     #endif /* BLAK_OUT_TERMINAL */
/* FUNCTION DECLARATIONS end. */


/*** FUNCTION DEFINITIONS */

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

static __inline__ void readRegName(bytecode_t byteCode[], uint16_t* bcp, regName_t regName_in){ /* fills regName with the name of the register currently pointed at by bcp */
     uint8_t i;
     for(i=0;i<BLAK_REG_NAME_LEN;i++){
          regName_in[i]=byteCode[*bcp];
          if(byteCode[*bcp]==NIL){
               break;
          }
          *bcp = *bcp+1;
     }
}


static __inline__ anon_t eval(bytecode_t byteCode[]){
     uint16_t bcp = 0; /* bytecode pointer */
     uint16_t j = 0;
     uint16_t empty_register = 0;
     /*unsigned char name[REG_NAME_LEN]={'\0', '\0', '\0', '\0'};*/
     uint8_t name[BLAK_REG_NAME_LEN];
     anon_t returnValue;

     returnValue.type=BLAK_ERROR_NEVERRAN_T;

     while(byteCode[bcp]!=BLAK_ENDOFBYTECODE){
          #ifdef DEBUG
               printf("!!!not a BLAK_ENDOFBYTECODE: %d\n", byteCode[bcp]);
               #endif
          if(byteCode[bcp]==BLAK_DEFINE){ /* BLAK_DEFINE name type value */
               #ifdef DEBUG
                    printf("!!!BLAK_DEFINE\n");
                    #endif
               /* let's find the first empty register */
               empty_register=0;
               while(reg[empty_register].type!=BLAK_EMPTY_T){
                    empty_register++;} /* found the first empty register, empty_register now points to it - cacca: e se so tutti pieni? */
               bcp++; /* advance pointer, will now point to the DF variable name - cacca: manage errors, se il bytecode finisce? */
               /* now we start reading the NAME of the define, it can be an unterminated 4-byte sequence, or a NIL-terminated 1, 2 or 3 bytes sequence */
               memset(name, '\0', BLAK_REG_NAME_LEN); /* reset name[] */

               for(j=0;j<BLAK_REG_NAME_LEN;j++){
                    if(byteCode[bcp]!=NIL){
                         #ifdef DEBUG
                              printf("%c", byteCode[bcp]);
                              #endif
                         name[j]=byteCode[bcp];
                         bcp+=(j<(BLAK_REG_NAME_LEN-1));
                    }
               }

               /* name[] is now containing the variable name */
               #ifdef DEBUG
                    printf("!!!define name: %c%c%c%c\n", name[0], name[1], name[2], name[3]);
                    #endif

               #ifdef DEBUG
                    printf("!!!now pointing at: %d\n", byteCode[bcp]);
                    #endif
               /* now I will put name, type and value in the empty register */
               for(j=0;j<BLAK_REG_NAME_LEN;j++){ /* write variable name in register */
                    reg[empty_register].name[j]=name[j];}
               bcp++; /* advance bytecode pointer, now pointing to valType - cacca: manage errors */
               #ifdef DEBUG
                    printf("!!!value type: ");
                    print_valType(byteCode[bcp]);
                    printf("\n");
                    #endif
               reg[empty_register].type = byteCode[bcp]; /* write the value type into register */
               bcp++; /* advance pointer, to point to the first byte of the actual value */
               #ifdef DEBUG
                    printf("!!! raw value: %d\n", byteCode[bcp]);
                    #endif
               /* now write the value into the register, according to its type */
               if(reg[empty_register].type==BLAK_ADDR_T){/* 16bit, mapped to addr */}
               else if(reg[empty_register].type==BLAK_FLOAT_T){/* float, put in memory and store its address */}
               else if(reg[empty_register].type==BLAK_UINT8_T){reg[empty_register].value.uint8=byteCode[bcp];}
               else if(reg[empty_register].type==BLAK_INT8_T){reg[empty_register].value.int8=byteCode[bcp];}
               else if(reg[empty_register].type==BLAK_UINT16_T){
                    uint16_t temp;
                    temp = (uint16_t)byteCode[bcp]<<8; /* MSB */
                    bcp++;
                    temp += (uint16_t)byteCode[bcp]; /* LSB */
                    reg[empty_register].value.uint16=temp;
               }
               else if(reg[empty_register].type==BLAK_INT16_T){reg[empty_register].value.int16=byteCode[bcp];}
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
                    /*bcp++;*/
                    for(;;){ /* write data into virtual mem */
			if(memPtr>BLAK_MAX_MEM){ break;} /* shitty error management */
                         mem[memPtr]=byteCode[bcp]; /* kak cacca: not checking if there's enough space */
                         memPtr++; /* point to next free byte */
                         if(byteCode[bcp]==BLAK_ENDOFEXPR){
                              break;
                         }
                         bcp++;
                    }
               }
               else{
                    returnValue.type=BLAK_ERROR_FAIL_T;
               }

               returnValue.type=BLAK_ERROR_SUCCESS_T;
          }/* if(byteCode[bcp]==BLAK_DEFINE){ */

          else if(byteCode[bcp]==BLAK_UNDEFINE){ /* BLAK_UNDEFINE name */
               regName_t regName = {NIL, NIL, NIL, NIL}; /* it's a pointer, needs initialization */
               uint8_t regNum;

               bcp++;
               readRegName(byteCode, &bcp, regName);
                    /*printf("==================== ");
                    print_regName(regName);
                    printf("\n");*/
               regNum = getRegNum(regName);
               if(regNum){ /* if regNum is 0, it's an error */
                    reg[regNum-1].type = BLAK_EMPTY_T;
                    returnValue.type=BLAK_ERROR_SUCCESS_T;
               }
               else{
                    returnValue.type=BLAK_ERROR_FAIL_T;
               }
          } /* UNDEFINE name */

          else if(byteCode[bcp]==BLAK_SET_NAME){ /* SET_NAME(regNum, name) */
               uint8_t regNum;
               uint8_t i;

               bcp++;
               regNum=byteCode[bcp];
               bcp++;
               for(i=0;i<BLAK_REG_NAME_LEN;i++){
                    reg[regNum].name[i]=byteCode[bcp];
                    if(byteCode[bcp]==NIL){
                         break;
                    }
                    bcp++;
               }
               returnValue.type=BLAK_ERROR_SUCCESS_T;
          } /* SET_NAME(regNum, name) */

          else if(byteCode[bcp]==BLAK_SET_TYPE){ /* SET_TYPE(regNum, type) */
               uint8_t regNum;

               bcp++;
               regNum=byteCode[bcp];
               bcp++;
               reg[regNum].type=byteCode[bcp];
               returnValue.type=BLAK_ERROR_SUCCESS_T;
          } /* SET_TYPE(regNum, type) */

          else if(byteCode[bcp]==BLAK_SET_VALUE){ /* SET_VALUE(regNum, mode, val) */
               uint8_t regNum;
               bytecode_t mode;

               bcp++;
               regNum=byteCode[bcp];
               bcp++;
               mode=byteCode[bcp];
               bcp++;
               if(mode==BLAK_BYTE1){
                    reg[regNum].value.uint8=byteCode[bcp];
               }
               else if(mode==BLAK_BYTE2){
                    uint16_t newVal = byteCode[bcp]<<8; /* get MSB */
                    bcp++;
                    newVal |= byteCode[bcp]; /* get LSB */
                    reg[regNum].value.uint16=newVal;
               }
               else{
                    returnValue.type=BLAK_ERROR_FAIL_T;
               }
               returnValue.type=BLAK_ERROR_SUCCESS_T;
          } /* SET_VALUE(regNum, mode, val) */

          #ifdef BLAK_OUT_TERMINAL
               else if(byteCode[bcp]==BLAK_SAY){ /* print to screen */ /* cacca: forse questa presuppone l'esistenza di eval, ma eval quello vero? */
                    bcp++;
                    if((valType_t)byteCode[bcp]==BLAK_ADDR_T){
                         uint8_t up;
                         uint8_t dn;
                         bcp++;
                         up = byteCode[bcp];
                         bcp++;
                         dn = byteCode[bcp];
                         printf(">> addr[%u] >> ", (up<<8)+dn);
                         bcp++;
                         /* cacca : qui dovrei mettere il ricorsivo "say next-byte" */
                    }
                    else if((valType_t)byteCode[bcp]==BLAK_UINT8_T){
                         bcp++;
                         printf(">> [BLAK_UINT8_T] %u\n", byteCode[bcp]);
                    }
                    else if((valType_t)byteCode[bcp]==BLAK_INT8_T){
                         bcp++;
                         printf(">> [BLAK_INT8_T] %d\n", byteCode[bcp]);
                    }
                    else if((valType_t)byteCode[bcp]==BLAK_REGISTERNAME_T){
                         regName_t regName = {NIL, NIL, NIL, NIL}; /* it's a pointer, needs initialization */
                         uint8_t i;
                         bcp++;
                         for(i=0;i<BLAK_REG_NAME_LEN;i++){
                              regName[i]=byteCode[bcp];
                              if(byteCode[bcp]==NIL){
                                   break;
                              }
                              bcp++;
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

               else if(byteCode[bcp]==BLAK_SHOWREGS){ /* show registers */
                    uint16_t i;
                    printf("\n== REGISTERS (%u x %u bytes) >> %u bytes:\n", BLAK_REGISTERS_QTY, sizeof(register_t), sizeof(reg));
                    for(i=0;i<BLAK_REGISTERS_QTY;i++){
                         printf("== reg[%d] >> \"%c%c%c%c\" [", i, reg[i].name[0], reg[i].name[1], reg[i].name[2], reg[i].name[3]);
                         print_valType(reg[i].type);
                         if(reg[i].type==BLAK_UINT8_T || reg[i].type==BLAK_EXPR_T || reg[i].type==BLAK_EMPTY_T){
                              printf("] %d\n", reg[i].value.uint8);
                         }
                         else if(reg[i].type==BLAK_UINT16_T){
                              printf("] %d\n", reg[i].value.uint16);
                         }
                    }
               } /* show registers */

               else if(byteCode[bcp]==BLAK_SHOWMEM){ /* show virtual memory */
                    uint32_t i; /* 32bits, so it can handle huuuuge memsizes */
                    uint8_t memString[17];
                    uint8_t widecharIndex;

                    memString[16]='\0'; /* terminate string */
                    printf("\n== VIRTUAL MEMORY (%u bytes) ptr:[%u]\n", BLAK_MEM_SIZE, memPtr);
                    /*printf("(00)(01)(02)(03)(04)(05)(06)(07)(08)(09)(10)(11)(12)(13)(14)(15)\n");*/
                    for(i=0;i<BLAK_MEM_SIZE;i++){
                         if(i%16==0){
                              printf(" ");
                         }
                         if(i==memPtr){ printf("\b[");}
                         printf("%02X", mem[i]); /* hex */
                         /*printf("%u", mem[i]);*/ /* decimal */
                         if(i==memPtr){ printf("] ");}
                              else{ printf("  ");}
                         /*memString[i%16]= ((mem[i]>31 && mem[i]<127) || mem[i]>160) ? mem[i] : '.';*/
                         memString[i%16]= mem[i];
                         if(i%16==15){
                              /*printf("\b(%u) (%u-%u) %04X-%04X : ", i-15, i-15, i, i-15, i);*/
                              printf("\b(%u-%u) %04X-%04X : ", i-15, i, i-15, i);
                              for(widecharIndex=0;widecharIndex<16;widecharIndex++){
                                   wprintf(L"%lc", (wchar_t)memString[widecharIndex]);
                              }
                              printf("\n");
                         }
                    }
                    printf("\n");
               } /* show virtual memory */

          #endif /* BLAK_OUT_TERMINAL */

          else{
               returnValue.type=BLAK_ERROR_FAIL_T;
          }

          bcp++; /* advance pointer for the next cycle */
     } /* while(byteCode[bcp]!=ENDOFBYTECODE){ */
     #ifdef DEBUG
          printf("!!! ok I got a BLAK_ENDOFBYTECODE, exiting...\n");
          #endif
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
