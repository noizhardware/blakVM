/* Toluene Blak Assembler */
#define TBA_VERSION "2021e01-1306"

/*** TODO
     - NAMCO: named constant: da usare solo nell'assembly, abc convertirà tutte le occorrenze di quella roba con l'altra roba, come #define in C
          però NON è come il preprocessor, qui avviene in ordine(forward reference only)
          la differenza con df è che df lasia il binding tra nome e valore accesisbile all'utente e al programma, mentre namco no, viene perso durante l'assembling
          .namco ciccetto 66 --every occurrence of "ciccetto" will be swapped with 66 before assembling
     - flag isaname -- per specificare che ci sarà un nome, anche se sembra un op o qualcosaltro
     - flag needRegnameT -- fer flaggare direttamente se avro bisogno o no di prependare BLAK_REGNAME_T ad un regname
*/

/*** DEFINES */
     #define BLAK_BLA_MAX_LINE_LEN 128
     #define BLAK_BLA_MAX_OPCODE_LEN 32
     #define BLAK_BLA_MAX_OPCODES_PER_LINE 64
     #define BLAK_BLB_MAX_BYTECODES_PER_LINE 64
     #define BLAK_BLB_MAX_FILENAME_LEN 32
/* DEFINES end. */

/*** INCLUDES */
     #include <stdio.h>
     #include <stdint.h>
     #include <string.h>
     #include <stdbool.h>
     #include "baostring.h"
     #include "blak2.h"
/* INCLUDES end. */

/*** TYPEDEFS */
/* TYPEDEFS end. */

/*** GLOBALS */
     /* TODO: ste flag si potrebbero comprimere in un unico byte, se ho bisogno di ottimizzare il compiler */
     bool isaname = false;
     bool needRegnameT = true;
/* GLOBALS end. */

/*** FUNCTION DECLARATIONS */
static __inline__ void bla_to_blb(char opcodeLine[BLAK_BLA_MAX_OPCODES_PER_LINE][BLAK_BLA_MAX_OPCODE_LEN], uint8_t opcodeLineSize, bytecode_t bytecodeLine[BLAK_BLB_MAX_BYTECODES_PER_LINE]);
/* FUNCTION DECLARATIONS end. */

/*** FUNCTION DEFINITIONS */
static __inline__ void bla_to_blb(char opcodeLine[BLAK_BLA_MAX_OPCODES_PER_LINE][BLAK_BLA_MAX_OPCODE_LEN], uint8_t opcodeLineSize, bytecode_t bytecodeLine[BLAK_BLB_MAX_BYTECODES_PER_LINE]){
     uint8_t opcodeLineIndex=0;
     uint8_t bytecodeLineIndex=1;

     for(opcodeLineIndex=0;opcodeLineIndex<opcodeLineSize;opcodeLineIndex++){ /* scan opcodes in opcodeLineIndex */
          /* can be:
               - an opcode
               - a name
               - a number
               */

          if(startsWith(opcodeLine[opcodeLineIndex], "--")){ break;} /* skip inline comments */

          /*** opcodes */
          else if(strEqual(opcodeLine[opcodeLineIndex], "dd")){
               needRegnameT=false;
               bytecodeLine[bytecodeLineIndex]=BLAK_DEFINE;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "rr")){
               needRegnameT=false;
               bytecodeLine[bytecodeLineIndex]=BLAK_REDEFINE;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "uu")){
               needRegnameT=false;
               bytecodeLine[bytecodeLineIndex]=BLAK_UNDEFINE;}

          else if(strEqual(opcodeLine[opcodeLineIndex], "load")){
               bytecodeLine[bytecodeLineIndex]=BLAK_LOAD;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "uload")){
               bytecodeLine[bytecodeLineIndex]=BLAK_UNLOAD;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "hcf")){
               bytecodeLine[bytecodeLineIndex]=BLAK_HCF;}
               
          else if(strEqual(opcodeLine[opcodeLineIndex], "sna")){
               needRegnameT=false;
               bytecodeLine[bytecodeLineIndex]=BLAK_SET_NAME;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "sty")){
               bytecodeLine[bytecodeLineIndex]=BLAK_SET_TYPE;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "sva")){
               bytecodeLine[bytecodeLineIndex]=BLAK_SET_VALUE;}

          else if(strEqual(opcodeLine[opcodeLineIndex], "b1")){
               bytecodeLine[bytecodeLineIndex]=BLAK_BYTE1;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "b2")){
               bytecodeLine[bytecodeLineIndex]=BLAK_BYTE2;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "b3")){
               bytecodeLine[bytecodeLineIndex]=BLAK_BYTE3;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "b4")){
               bytecodeLine[bytecodeLineIndex]=BLAK_BYTE4;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "b5")){
               bytecodeLine[bytecodeLineIndex]=BLAK_BYTE5;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "b6")){
               bytecodeLine[bytecodeLineIndex]=BLAK_BYTE6;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "b7")){
               bytecodeLine[bytecodeLineIndex]=BLAK_BYTE7;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "b8")){
               bytecodeLine[bytecodeLineIndex]=BLAK_BYTE8;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "b9")){
               bytecodeLine[bytecodeLineIndex]=BLAK_BYTE9;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "b10")){
               bytecodeLine[bytecodeLineIndex]=BLAK_BYTE10;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "b11")){
               bytecodeLine[bytecodeLineIndex]=BLAK_BYTE11;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "b12")){
               bytecodeLine[bytecodeLineIndex]=BLAK_BYTE12;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "b13")){
               bytecodeLine[bytecodeLineIndex]=BLAK_BYTE13;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "b14")){
               bytecodeLine[bytecodeLineIndex]=BLAK_BYTE14;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "b15")){
               bytecodeLine[bytecodeLineIndex]=BLAK_BYTE15;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "b16")){
               bytecodeLine[bytecodeLineIndex]=BLAK_BYTE16;}



          /* BLAK_RAVENOP_T operators */
          else if(strEqual(opcodeLine[opcodeLineIndex], "+")){
               bytecodeLine[bytecodeLineIndex]=BLAK_RAVENOP_T; bytecodeLineIndex++;
               bytecodeLine[bytecodeLineIndex]=BLAK_ADD;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "-")){
               bytecodeLine[bytecodeLineIndex]=BLAK_RAVENOP_T; bytecodeLineIndex++;
               bytecodeLine[bytecodeLineIndex]=BLAK_SUB;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "*")){
               bytecodeLine[bytecodeLineIndex]=BLAK_RAVENOP_T; bytecodeLineIndex++;
               bytecodeLine[bytecodeLineIndex]=BLAK_MUL;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "/")){
               bytecodeLine[bytecodeLineIndex]=BLAK_RAVENOP_T; bytecodeLineIndex++;
               bytecodeLine[bytecodeLineIndex]=BLAK_DIV;}

          else if(strEqual(opcodeLine[opcodeLineIndex], "==")){
               bytecodeLine[bytecodeLineIndex]=BLAK_RAVENOP_T; bytecodeLineIndex++;
               bytecodeLine[bytecodeLineIndex]=BLAK_EQUAL;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "!=")){
               bytecodeLine[bytecodeLineIndex]=BLAK_RAVENOP_T; bytecodeLineIndex++;
               bytecodeLine[bytecodeLineIndex]=BLAK_NOTEQUAL;}

          else if(strEqual(opcodeLine[opcodeLineIndex], "frv")){
               bytecodeLine[bytecodeLineIndex]=BLAK_RAVENOP_T; bytecodeLineIndex++;
               bytecodeLine[bytecodeLineIndex]=BLAK_FOREVER;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "if")){
               bytecodeLine[bytecodeLineIndex]=BLAK_RAVENOP_T; bytecodeLineIndex++;
               bytecodeLine[bytecodeLineIndex]=BLAK_IF;}

          else if(strEqual(opcodeLine[opcodeLineIndex], "eval")){
               bytecodeLine[bytecodeLineIndex]=BLAK_RAVENOP_T; bytecodeLineIndex++;
               bytecodeLine[bytecodeLineIndex]=BLAK_EVAL;}

          else if(strEqual(opcodeLine[opcodeLineIndex], "say")){
               printf("|say|");
               bytecodeLine[bytecodeLineIndex]=BLAK_SAY;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "showregs")){
               bytecodeLine[bytecodeLineIndex]=BLAK_SHOWREGS;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "showmem")){
               bytecodeLine[bytecodeLineIndex]=BLAK_SHOWMEM;}

          else if(strEqual(opcodeLine[opcodeLineIndex], "lamb")){
               needRegnameT=false;
               bytecodeLine[bytecodeLineIndex]=BLAK_LAMBDA;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "lamba")){
               bytecodeLine[bytecodeLineIndex]=BLAK_LAMBDA_A;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "lambb")){
               bytecodeLine[bytecodeLineIndex]=BLAK_LAMBDA_B;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "larg")){
               needRegnameT=false;
               bytecodeLine[bytecodeLineIndex]=BLAK_LAMBDAARG_T;}

          else if(strEqual(opcodeLine[opcodeLineIndex], "mu")){
               bytecodeLine[bytecodeLineIndex]=BLAK_MU;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "mua")){
               bytecodeLine[bytecodeLineIndex]=BLAK_MU_A;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "mub")){
               bytecodeLine[bytecodeLineIndex]=BLAK_MU_B;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "mue")){
               bytecodeLine[bytecodeLineIndex]=BLAK_MU_END;}

          else if(strEqual(opcodeLine[opcodeLineIndex], "go")){
               bytecodeLine[bytecodeLineIndex]=BLAK_GO;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "halt")){
               bytecodeLine[bytecodeLineIndex]=BLAK_HALT;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "ext")){
               bytecodeLine[bytecodeLineIndex]=BLAK_EXTERN;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "hbea")){
               bytecodeLine[bytecodeLineIndex]=BLAK_HEARTBEAT;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "set")){
               bytecodeLine[bytecodeLineIndex]=BLAK_SET;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "live")){
               bytecodeLine[bytecodeLineIndex]=BLAK_LIVE;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "cod")){
               bytecodeLine[bytecodeLineIndex]=BLAK_CODING;}

          else if(strEqual(opcodeLine[opcodeLineIndex], "prag")){
               bytecodeLine[bytecodeLineIndex]=BLAK_PRAGMA;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "nop")){
               bytecodeLine[bytecodeLineIndex]=BLAK_NOP;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "eol")){
               bytecodeLine[bytecodeLineIndex]=BLAK_ENDOFLIST;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "eox")){
               bytecodeLine[bytecodeLineIndex]=BLAK_ENDOFEXPR;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "dser")){
               bytecodeLine[bytecodeLineIndex]=BLAK_DUMPSERIAL;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "tser")){
               bytecodeLine[bytecodeLineIndex]=BLAK_TAKESERIAL;}

          /* TODO: else if ... other opcodes */
          /* opcodes end. */

          /*** types */
          else if(strEqual(opcodeLine[opcodeLineIndex], "u8")){
               /*printf(" |isu8| ");*/
               bytecodeLine[bytecodeLineIndex]=BLAK_UINT8_T;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "s8")){
               bytecodeLine[bytecodeLineIndex]=BLAK_INT8_T;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "u16")){
               bytecodeLine[bytecodeLineIndex]=BLAK_UINT16_T;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "s16")){
               bytecodeLine[bytecodeLineIndex]=BLAK_INT16_T;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "list")){
               bytecodeLine[bytecodeLineIndex]=BLAK_LIST_T;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "bool")){
               bytecodeLine[bytecodeLineIndex]=BLAK_BOOL_T;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "func")){
               needRegnameT=false;
               bytecodeLine[bytecodeLineIndex]=BLAK_FUNC_T;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "addr")){
               bytecodeLine[bytecodeLineIndex]=BLAK_ADDR_T;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "udfd")){
               bytecodeLine[bytecodeLineIndex]=BLAK_UNDEFINED_T;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "empty")){
               bytecodeLine[bytecodeLineIndex]=BLAK_EMPTY_T;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "func")){
               bytecodeLine[bytecodeLineIndex]=BLAK_FUNC_T;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "fmem")){
               bytecodeLine[bytecodeLineIndex]=BLAK_FREEMEM_T;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "fmems")){
               bytecodeLine[bytecodeLineIndex]=BLAK_FREEMEMSINGLE_T;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "expr")){
               bytecodeLine[bytecodeLineIndex]=BLAK_EXPR_T;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "regnam")){
               bytecodeLine[bytecodeLineIndex]=BLAK_REGISTERNAME_T;}
          else if(strEqual(opcodeLine[opcodeLineIndex], "regnum")){
               bytecodeLine[bytecodeLineIndex]=BLAK_REGISTERNUMBER_T;}

          /* TODO: else if ... other types */
          /* types end. */

          else{ /* not an opcode nor a type */
               /*printf("==bla_to_blb: not op nor type\n");*/
               if(string_isInt(opcodeLine[opcodeLineIndex])){ /* is a number */
                     /* cacca: sto ignorando numeri colla virgola, senò dovrei usare string_isNum */
                     /* cacca: per ora solo unsigned... */
                    uint16_t numba; /* cacca: cosi non puo contenere una roba a 32bit */
                    /*printf(" |isInt: ");*/
                    numba = str_toUint16(opcodeLine[opcodeLineIndex]);
                    /*printf("%u| ", numba);*/
                    if(numba<256){ /* 8bit */
                         bytecodeLine[bytecodeLineIndex]=(uint8_t)numba;
                    }
                    else{
                         bytecodeLine[bytecodeLineIndex]=(uint8_t)(numba >> 8); /* write MSB to bytecode */
                         bytecodeLineIndex++;
                         bytecodeLine[bytecodeLineIndex]=(uint8_t)(numba & 0x00FF); /* write LSB to bytecode */
                    }
               } /* is a number */
               else{ /* not an opcode, not a number, soooo... it must be a name! */
                    uint8_t namIndex;
                    printf("==bla_to_blb: must be a name:\"%s\" >> ", opcodeLine[opcodeLineIndex]);
                    /*printf("-1(%d)|%u|-2()%d|%u|", bytecodeLineIndex-1, bytecodeLine[bytecodeLineIndex-1], bytecodeLineIndex-2, bytecodeLine[bytecodeLineIndex-2]);*/



                    /* list of opcodes that are obvoiously followed by a regname, they don't need BLAK_REGISTERNAME_T to be prepended to the actual regname, OR they are something else, so, still, they don't need BLAK_REGISTERNAME_T to be prepended to the actual regname */
                    /*if(  (((bytecodeLineIndex-1)>=1) && (
                              bytecodeLine[bytecodeLineIndex-1]!=BLAK_DEFINE &&
                              bytecodeLine[bytecodeLineIndex-1]!=BLAK_UNDEFINE &&
                              bytecodeLine[bytecodeLineIndex-1]!=BLAK_REDEFINE &&
                              bytecodeLine[bytecodeLineIndex-1]!=BLAK_LAMBDA &&
                              bytecodeLine[bytecodeLineIndex-1]!=(bytecode_t)BLAK_LAMBDAARG_T
                         )) ||
                         (((bytecodeLineIndex-2)>=1) && (
                              bytecodeLine[bytecodeLineIndex-2]!=BLAK_SET_NAME
                              ))
                         ){*/
                    if(needRegnameT){ /* needs BLAK_REGNAME_T */
                         printf("|needs regnam_t|"); /* kak cacca: line12 non triggera questo, crede non ci sia bisogno di regnam */
                         bytecodeLine[bytecodeLineIndex]=BLAK_REGISTERNAME_T;
                         bytecodeLineIndex++;
                    }
                    else{ /* didn't need BLAK_REGNAME_T, now set it back to neeeeeeed */
                         needRegnameT=true;
                    }
                    for(namIndex=0;namIndex<BLAK_REG_NAME_LEN;namIndex++){ /* write registername to bytecode */
                         bytecodeLine[bytecodeLineIndex]=opcodeLine[opcodeLineIndex][namIndex];
                         /*printf(" |%u|", bytecodeLine[bytecodeLineIndex]);*/
                         if(opcodeLine[opcodeLineIndex][namIndex]==NIL || namIndex==3){
                              break;
                         }
                         bytecodeLineIndex++;
                    }
               } /* soooo.. it must be a name */
          } /* not an opcode */

          /*printf("==bla_to_blb: %s >> [%u]%u\n", opcodeLine[opcodeLineIndex], bytecodeLineIndex, bytecodeLine[bytecodeLineIndex]);*/

          bytecodeLineIndex++;
     }
     bytecodeLine[0]=bytecodeLineIndex; /* write size into 0 element */
}
/* FUNCTION DEFINITIONS end. */

/*** MAIN */
int main(int argc, char* argv[]){
     FILE* fptrIn = NULL;
     FILE* fptrOut = NULL;
     uint16_t lineNum = 0;
     uint8_t lineIndex = 0;
     /*char* fileIn = "test.bla";*/ /* blak assembly */
     /*char* fileOut = "test.blb";*/ /* blak bytecode */
     char line[BLAK_BLA_MAX_LINE_LEN];
     char opcodeLine[BLAK_BLA_MAX_OPCODES_PER_LINE][BLAK_BLA_MAX_OPCODE_LEN];
     uint8_t opcodeIndex;
     uint8_t opcodeCharIndex;
     bytecode_t bytecodeLine[BLAK_BLB_MAX_BYTECODES_PER_LINE];
     uint8_t ind; /* multipurpose index */

     char fileIn[BLAK_BLB_MAX_FILENAME_LEN];
     char fileOut[BLAK_BLB_MAX_FILENAME_LEN];
     
     (void)blak_flags;

     printf("==============================\n");
     printf("==  Toluene Blak Assembler  ==\n");
     printf("==   version %s   ==\n", TBA_VERSION);
     printf("==  blaK Win v.%s ==\n", BLAK_VERSION);
     printf("==============================\n");
     printf("== MAX_LINE_LEN: %u\n", BLAK_BLA_MAX_LINE_LEN);
     printf("== MAX_OPCODE_LEN: %u\n", BLAK_BLA_MAX_OPCODE_LEN);
     printf("== MAX_OPCODES_PER_LINE: %u\n", BLAK_BLA_MAX_OPCODES_PER_LINE);
     printf("== MAX_BYTECODES_PER_LINE: %u\n", BLAK_BLB_MAX_BYTECODES_PER_LINE);
     printf("== MAX_FILENAME_LEN: %u\n", BLAK_BLB_MAX_FILENAME_LEN);

     printf("\n");
     printf("== BLAK_DEFINE : %u\n", BLAK_DEFINE);
     printf("== BLAK_UNDEFINE : %u\n", BLAK_UNDEFINE);
     printf("== BLAK_REDEFINE : %u\n", BLAK_REDEFINE);
     printf("== BLAK_LAMBDA : %u\n", BLAK_LAMBDA);
     printf("== BLAK_LAMBDAARG_T : %u\n", BLAK_LAMBDAARG_T);
     printf("== BLAK_SET_NAME : %u\n", BLAK_SET_NAME);
     printf("== BLAK_ENDOFBYTECODE : %u\n", BLAK_ENDOFBYTECODE);

     printf("\n");

     if(argc>1){
          sprintf(fileIn, "%s.bla", argv[1]);
          sprintf(fileOut, "%s.blb", argv[1]);
     }
     printf("==assembling %s >> %s ...\n", fileIn, fileOut);

     line[0]='\0'; /* init */
     fptrIn = fopen(fileIn, "r");
     fptrOut = fopen(fileOut, "wb"); /* wipe the file */
     fclose(fptrOut);
     fptrOut = fopen(fileOut, "ab"); /* ab appends, wb overwrites. I append here, just in case an error occours, I don't want to overwrite an existing file */

     while(fgets(line, BLAK_BLA_MAX_LINE_LEN, fptrIn)){ /* line by line */
          if(line[strlen(line)-1]=='\n'){ /* get rid of newline, if present */
               line[strlen(line)-1]='\0';
          }
          printf(">>line[%u]: %s >> ", lineNum, line);

          lineIndex=0;
          opcodeIndex=0;
          opcodeCharIndex=0;
          while(lineIndex<strlen(line)){ /* fill opcodeLine array with the line from the file */
               while(line[lineIndex]!='\0' && line[lineIndex]!=' '){
                    opcodeLine[opcodeIndex][opcodeCharIndex]=line[lineIndex];
                    lineIndex++;
                    if(opcodeCharIndex<BLAK_BLA_MAX_OPCODE_LEN){
                      opcodeCharIndex++;
                      opcodeLine[opcodeIndex][opcodeCharIndex]='\0';
                    }
                    else{
                      printf("::== opcodeCharIndex overflow: %u\n", opcodeCharIndex);
                    }
               }
               printf("[%s]", opcodeLine[opcodeIndex]);
               lineIndex++;
               opcodeIndex+=(opcodeIndex<BLAK_BLA_MAX_OPCODES_PER_LINE);
               opcodeCharIndex=0;
          }
          printf(" (%ux) >> ", opcodeIndex);

          bla_to_blb(opcodeLine, opcodeIndex, bytecodeLine); /* compile, line by line */
          for(ind=1;ind<bytecodeLine[0];ind++){
               printf("%u ", bytecodeLine[ind]);
          }
          printf("\n");
          /* write this line of bytecodes to file */
          for(ind=1;ind<bytecodeLine[0];ind++){
               fputc(bytecodeLine[ind], fptrOut);
          }

          lineNum++;
     } /* line by line */


     /* write BLAK_ENDOFBYTECODE  */
     fputc(BLAK_ENDOFBYTECODE, fptrOut);

     fclose(fptrOut);
     fclose(fptrIn);
     
     printf("==DONE!\n");

     /*(void)argc;*//* cacca: why void? I'm using them */
     /*(void)argv;*/
     return 0;
}
/* MAIN end. */
