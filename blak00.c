#define BLAK00_VERSION "2021d23-2217"
/*** DEFINES */
     /*#define DEBUG*/
     #define BLAK_OUT_TERMINAL /* if we have a regular, printf-capable stdout interface */
     /*#define BLAK_OUT_SERIAL*/ /* per arduino-nyx, per mandare scritte via Serial.print */
     /*#define BLAK_OUT_VIBRO*/ /* for nyx */
     /*#define BLAK_OUT_DISPLAY_128X64*/ /* for nyx */
     /*#define BLAK_OUT_DISPLAY_LCD*/ /* for nyx */
     
     #define BLAK_IN_TERMINAL /* regular stdin for text input */
     
     #define BLAK_PC_FILESYSTEM /* regular filesystem with file access (to load bytecode ROMs) */
     
     /* poi cmq andranno fatte delle build separate per ogni hardware, come ogni VM che si rispetti :)) */
     
/* DEFINES end. */

/*** INCLUDES */
     #include <stdio.h>
     #include <string.h>
     #include <stdint.h>

     #include "baostring.h"
     #include "baofiles.h"
     #include "blak2.h"

     #ifdef TEST
          #include "baotime.h"
          #endif
/* INCLUDES end. */


/*-------------------------------------*/
/*         USE -O3 to compile          */
/*-------------------------------------*/

/*** TODO:
     - functional - reactive? le variabili si devono autocheckare e updatare (magari solo i child quando vengono chiamati, checkano una loro lista di parent, e si autorievalutano)
       - si puo fare semplicemente che i valori derivati sono sempre e solo indirizzi
         df a 10
         df b add a 2 -- questo viene fatto mettendo : b = addressOf(a) + 2 ogni volta che b viene chiamato
         df c dbl b -- c = 24
         rdf a 15 -- this will automatically influence also b and c
         print c -- c = 34
     
     - unit tests:
          mytest.bla >> mytest.blb
          mytest.expected >> compare

     - (string) >> (convert into bytecode) >> (run)
       "df cix 23\ndf culo 130\ndf na 66\ndf q 45\ndf yo! 2\n" >> bytecode >> eval
     - REPL?
     - implement simple sinewave engine and start experimenting
     - led blinking helloworld
*/


/*** GLOBALS */

/* GLOBALS end. */

/*** MAIN */
int main(){
     
      /* input string */
     /*char* st =     "pragma tmach nyx\ndf culo 23\nsay puppaaaaaaa\n";*/
     
     /*uint32_t t;*/ /* bello ciccio, multiuso */
     bytecode_t bytecodesQty = BLAK_BYTECODES_QTY;
     valType_t typesQty = BLAK_TYPES_QTY;
     
     /*char* inputBuffer;*/
     /*char* currentLine;*/

     printf("=============================\n");
     printf("== blaK Win v.%s ==\n", BLAK_VERSION);
     printf("== blaK00.c v.%s ==\n", BLAK00_VERSION);
     printf("=============================\n\n");
     printf("== VIRTUAL MEMORY (%u bytes)\n", BLAK_MEM_SIZE);
     printf("== VIRTUAL PROGRAM MEMORY (%u bytes)\n", BLAK_PROGMEM_SIZE);
     printf("== REGISTERS (%u x %u bytes) >> %u bytes\n", BLAK_REGISTERS_QTY, sizeof(register_t), sizeof(reg));
     printf("== FLAGS (%u x 1bit) >> %u bytes\n", BLAK_FLAGS_QTY, sizeof(blak_flags));
     printf("== total: %u bytes (%.2fkb)\n",
          BLAK_MEM_SIZE+
          BLAK_PROGMEM_SIZE+
          sizeof(reg)+
          sizeof(blak_flags),
               (float)(BLAK_MEM_SIZE+BLAK_PROGMEM_SIZE+sizeof(reg)+sizeof(blak_flags))/1024);
     printf("== [%u] instructions (bytecode_t size: %u bytes)\n", bytecodesQty, sizeof(bytecode_t));
     printf("== [%u] types (valType_t size: %u bytes)\n", typesQty, sizeof(valType_t));
     
     #ifdef BRANCH
          printf("== branching version\n\n");
     #endif
     #ifndef BRANCH
          printf("== NON-branching version\n\n");
     #endif
     
     #ifdef TEST
          startTimer();
     #endif
     
     printf("==loading test.blb...");
     /*fileRead_nomalloc("test.blb", (char*)progMem);*/
     loadProg("test.blb");
     /*progMemPtr=0;*/
     printf(" >>> ");
     errorPrint(eval());
     printf("\n");

     #ifdef TEST
          stopTimer();
     #endif
     
     printf("==loading showers.blb...");
     /*fileRead_nomalloc("showers.blb", (char*)progMem);*/
     loadProg("showers.blb");
     /*progMemPtr=0;*/
     printf(" >>> ");
     errorPrint(eval());
     printf("\n");
     
     #ifdef TEST
          printf("==EVALed in: %.3fmsec\n", (float)(((float)elapsed().tv_sec*1000)+((float)elapsed().tv_usec/1000)) );
     #endif
     
return 0;}
/* MAIN end. */