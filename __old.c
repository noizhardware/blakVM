     printf(" %d cycles\nstarting...\n", cy);
     
     #ifdef TEST
          startTimer();
     #endif
     
     for(t=0;t<cy;t++){
          detect(st);}
          
          PUSH(detect(st)); /* copies the bytecode into the virtual stack */
          PUSH(99);
     
     #ifdef TEST
          stopTimer();
     #endif
     
     printf("DONE %d cycles\n", t);
     #ifdef TEST
          printf("==finished in: %ld.%ldsec\n", elapsed().tv_sec, elapsed().tv_usec/1000);
     #endif
     printf("input string:\n%s\n", st);
     printf("instruction: %d\n", GET());
     printf("instruction size: %d bits\n", sizeof(instruction_t)*8);
     
     printf("\n--------------------\n\n");
     
     currentLine = terminateStringOnChar(st, '\n', true); /* extract the first line from the code */
     inputBuffer = clearStringUntilChar(st, '\n', true); /* update the buffer, without the just-used line */
     printf("==new shit:\n%s\n", currentLine);
     printf("==the rest:\n%s\n", inputBuffer);
     printf("\n--------------------\n\n");
     printf("STACK (bottom to top):\t");
     for(t=0;t<MAX_STACK;t++){
          printf("%d:%d ",t , STACK[t]);}
          printf("%d:%d\n",t , STACK[t]);
     
     
     printf("REG_SP:\t\t%d\n", REG_SP);
     
     printf("should be 99: %d\n", POP());
     printf("POP: %d\n", POP());
     UP();
     printf("should be the same: %d\n", POP());
     
     ---------------------------
     
          
          /*
               df cix 23
               df culo 130
               df na 66
               df q 45
               df yo! 2
          */
          bytecode_t sampleByteCode[] = {    BLAK_DEFINE, 'c', 'i', 'x', NIL, BLAK_UINT8_T, 23,
                                             BLAK_DEFINE, 'c', 'u', 'l', 'o', BLAK_UINT8_T, 130,
                                             BLAK_DEFINE, 'n', 'a', NIL, BLAK_UINT8_T, 66,
                                             BLAK_DEFINE, 'q', NIL, BLAK_UINT8_T, 45,
                                             BLAK_DEFINE, 'y', 'o', '!', NIL, BLAK_UINT8_T, 2,
                                             /*   dd bla add na q >> viene compilata in bytecode come: */
                                             BLAK_DEFINE, 'b', 'l', 'a', NIL, BLAK_EXPR_T, BLAK_RAVENOP_T, BLAK_ADD, BLAK_REGISTERNAME_T, 'n', 'a', NIL , BLAK_REGISTERNAME_T, 'q', NIL, BLAK_ENDOFEXPR,
                                             /* con questo bytecode la VM deve mettere in mem:
                                                  [BLAK_RAVENOP_T]
                                                  [ADD]
                                                  [BLAK_REGISTERNAME_T]
                                                  [n][a][NIL]
                                                  [BLAK_REGISTERNAME_T]
                                                  [q][NIL]
                                                  >> 9bytes
                                                  così posso usare nomi non ancora dichiarati, che è una feature che me gusta, posso fare:
                                                  dd c add a b --c=(type: BLAK_EXPR_T, value: address of expression, when computed generates a BLAK_UNDEFINED_T type, o semplicemente 0?)
                                                  dd a 12 --c= sempre expr, ma ora puo essere computata, e sarà 12, il secondo addendo ancora mancante
                                                  dd b 7 -- expr >> 19 al momento della computazione
                                                  
                                                  POI metto nel register il nome "bla", type BLAK_EXPR_T, e address che punta a quello che ho appena scritto in mem
                                             */
                                             /*   dd anus mul bla 12  >> viene compilata in bytecode come: */
                                             BLAK_DEFINE, 'a', 'n', 'u', 's', BLAK_EXPR_T, BLAK_RAVENOP_T, BLAK_MUL, BLAK_REGISTERNAME_T, 'b', 'l', 'a', NIL, BLAK_UINT8_T, 12, BLAK_ENDOFEXPR,
                                             BLAK_ENDOFBYTECODE};
                                             
          /*bytecode_t showers[] = {      BLAK_SHOWREGS,
                                        BLAK_SHOWMEM,
                                        BLAK_SAY, BLAK_REGISTERNAME_T, 'n', 'a', NIL,
                                        BLAK_SAY, BLAK_REGISTERNAME_T, 'y', 'o', '!', NIL,
                                        BLAK_SAY, BLAK_REGISTERNAME_T, 'c', 'u', 'l', 'o', NIL,
                                        BLAK_SAY, BLAK_REGISTERNAME_T, 'n', 'a', 'n', 'a', NIL,
                                        BLAK_SAY, BLAK_REGISTERNAME_T, 'a', NIL,
                                        BLAK_SAY, BLAK_REGISTERNAME_T, 'c', 'i', 'x', NIL,
                                        BLAK_SAY, BLAK_REGISTERNAME_T, 'd', 'i', 'o', NIL,
                                        BLAK_ENDOFBYTECODE};*/
          
          bytecode_t sampleByteCode2[] = {   BLAK_UNDEFINE, 'c', 'i', 'x', NIL,
                                             BLAK_DEFINE, 'a', NIL, BLAK_UINT8_T, 55,
                                             BLAK_SET_NAME, 10, 'd', 'i', 'o', NIL,
                                             BLAK_SET_TYPE, 10, BLAK_UINT16_T,
                                             BLAK_SET_VALUE, 10, BLAK_BYTE2, 11, 184, /* value 3000 (MSB LSB) */
                                             BLAK_ENDOFBYTECODE};
                                             /* in assembly:
                                                  udf cix
                                                  def a u8 55
                                             */
          
          /*char sampleString[] =   "df stro 99\n\
                                   df nano 1\n\
                                   df buco 0\n\
                                   df max 255\n";*/
                                   
------------------------------

     /*printf("\n==== sample bytecode:  ");
     t=0;
     while(sampleByteCode[t]!=BLAK_ENDOFBYTECODE){
          printf("%c", sampleByteCode[t]);
          t++;}
     printf("\n");
     printf("\n==== sample bytecode2:  ");
     t=0;
     while(sampleByteCode2[t]!=BLAK_ENDOFBYTECODE){
          printf("%c", sampleByteCode2[t]);
          t++;}
     printf("\n");*/                                   