//https://www.google.com/url?sa=t&rct=j&q=&esrc=s&source=web&cd=&ved=2ahUKEwjc3b6Lwar2AhUPTcAKHRVnCmUQFnoECAMQAQ&url=http%3A%2F%2Fgoldencrystal.free.fr%2FM68kOpcodes-v2.3.pdf&usg=AOvVaw3a6qPsk5K_kpQd1MnlD07r
//https://www.google.com/url?sa=t&rct=j&q=&esrc=s&source=web&cd=&ved=2ahUKEwjc3b6Lwar2AhUPTcAKHRVnCmUQFnoECAUQAQ&url=https%3A%2F%2Fweb.njit.edu%2F~rosensta%2Fclasses%2Farchitecture%2F252software%2Fcode.pdf&usg=AOvVaw0awr9hRKXycE2-kghhbC3Y
//https://onlinedisassembler.com/odaweb/
//https://github.com/kstenerud/Musashi/tree/master

//NB  in byte access, the CPU outputs the byte on the lower as well as the upper eight data lines.

//#warning i fattori di AND EOR OR ADD SUB sono TUTTI invertiti! (corretto in ADD SUB)...
// in effetti credo che tutti questi siano da rivedere, le  IF DIRECTION... sono ridondanti 

#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <conio.h>
#include "tms9900win.h"

#pragma check_stack(off)
// #pragma check_pointer( off )
#pragma intrinsic( _enable, _disable )



//#define TMS9940 1

int T1;
HFILE spoolFile;



BYTE fExit=0;
SWORD Pipe1;
extern BOOL debug;
//BYTE KBDataI,KBDataO,KBControl,/*KBStatus,*/ KBRAM[32];   // https://wiki.osdev.org/%228042%22_PS/2_Controller
//#define KBStatus KBRAM[0]   // pare...
extern SWORD VICRaster;
extern BYTE ram_seg[];

volatile BYTE TIMIRQ,VIDIRQ;


BYTE CPUPins=DoReset;
#define MAX_WATCHDOG 100      // x30mS v. sotto
WORD WDCnt=MAX_WATCHDOG;
BYTE ColdReset=1;
union PIPE Pipe2;


// da 68000 Makushi o come cazzo si chiama :D
// res2 è Source e res1 è Dest ossia quindi res3=Result
#define CARRY_ADD_8() (!!(((res2.b.l & res1.b.l) | (~res3.b.l & (res2.b.l | res1.b.l))) & 0x80))		// ((S & D) | (~R & (S | D)))
#define OVF_ADD_8()  (!!(((res2.b.l ^ res3.b.l) & (res1.b.l ^ res3.b.l)) & 0x80))			// ((S^R) & (D^R))
#define CARRY_ADD_16() (!!(((res2.x & res1.x) | (~res3.x &	(res2.x | res1.x))) & 0x8000))
#define OVF_ADD_16() (!!(((res2.x ^ res3.x) & (res1.x ^ res3.x)) & 0x8000))
#define CARRY_SUB_8() (!!(((res2.b.l & res3.b.l) | (~res1.b.l & (res2.b.l | res3.b.l))) & 0x80))		// ((S & R) | (~D & (S | R)))
#define OVF_SUB_8()  (!!(((res2.b.l ^ res1.b.l) & (res3.b.l ^ res1.b.l)) & 0x80))			// ((S^D) & (R^D))
#define CARRY_SUB_16() (!!(((res2.x & res3.x) | (~res1.x &	(res2.x | res3.x))) & 0x8000))
#define OVF_SUB_16() (!!(((res2.x ^ res1.x) & (res3.x ^ res1.x)) & 0x8000))

extern BYTE TMS9918Reg[8],TMS9918RegS;
 

int Emulate(int mode) {
	BOOL bMsgAvail;
	MSG msg;
	HDC hDC;


//https://en.wikipedia.org/wiki/TMS9900
#define WORKING_REG_INDEX (Pipe1 & 0xf)
#define WORKING_REG regs->r[workingRegIndex].x      // 
#define WORKING_TS ((Pipe1 >> 4) & B8(11))
#define WORKING_TD ((Pipe1 >> 10) & B8(11))
#define REGISTER_DIRECT 0
#define REGISTER_INDIRECT 1
#define REGISTER_SYMBOLIC_INDEXED 2
#define REGISTER_INDIRECT_AUTOINCREMENT 3
#define WORKING_REG2_INDEX ((Pipe1 >> 6) & 0xf)
#define WORKING_REG2 regs->r[workingReg2Index].x      // 

// USARE, MA OCCHIO!!
#define COMPUTE_SOURCE \
  switch(workingTS) {\
    case REGISTER_DIRECT:\
      res1.x=WORKING_REG;\
      break;\
    case REGISTER_INDIRECT:\
      res1.x=GetIntValue(WORKING_REG);\
      break;\
    case REGISTER_SYMBOLIC_INDEXED:\
      if(workingRegIndex)\
        res1.x=GetIntValue(WORKING_REG+(int16_t)Pipe2.x);\
      else\
        res1.x=GetIntValue(Pipe2.x);\
      GetPipe(_pc);\
      _pc+=2;\
      break;\
    case ADDR_REGISTER_INDIRECT_POSTINCREMENT:\
      res1.x=GetIntValue(WORKING_REG);\
      WORKING_REG+=2;\
      break;\
    }
#define COMPUTE_SOURCE2 \
  switch(workingTD) {\
    case REGISTER_DIRECT:\
      res2.x=WORKING_REG2;\
      break;\
    case REGISTER_INDIRECT:\
      res2.x=GetIntValue(WORKING_REG2);\
      break;\
    case REGISTER_SYMBOLIC_INDEXED:\
      if(workingReg2Index)\
        res2.x=GetIntValue(WORKING_REG2+(int16_t)Pipe2.x);\
      else\
        res2.x=GetIntValue(Pipe2.x);\
      _pc+=2;\
      break;\
    case ADDR_REGISTER_INDIRECT_POSTINCREMENT:\
      res2.x=GetIntValue(WORKING_REG2);\
      break;\
    }
#define COMPUTE_DEST \
  switch(workingTD) {\
    case REGISTER_DIRECT:\
      WORKING_REG2=res3.x;\
      break;\
    case REGISTER_INDIRECT:\
      PutIntValue(WORKING_REG2,res3.x);\
      break;\
    case REGISTER_SYMBOLIC_INDEXED:\
      if(workingReg2Index)\
        PutIntValue(WORKING_REG2+(int16_t)Pipe2.x,res3.x);\
      else\
        PutIntValue(Pipe2.x,res3.x);\
      _pc+=2;\
      break;\
    case ADDR_REGISTER_INDIRECT_POSTINCREMENT:\
      PutIntValue(WORKING_REG2,res3.x);\
      WORKING_REG2+=2;\
      break;\
    }
	
	SWORD _pc=0;
	SWORD _wp=0;
	BYTE IPL=0;
  union Z_REGISTERS *regs=NULL;
  union RESULT res1,res2,res3;
//  union OPERAND op1,op2;
	union REGISTRO_F _st;
	/*register*/ uint16_t i;
  uint8_t workingTS,workingTD,workingRegIndex,workingReg2Index;
  int c=0;


  _pc=GetIntValue(0x0002);
  _wp=GetIntValue(0x0000);
  _st.x=0;
  IPL=B8(0001);   // Ti99
  
  
//  _pc=0x0935;
//  _sp=0x8700;
  

	do {

		c++;
		if(!(c & 0x3ffff)) {
// yield()
			bMsgAvail=PeekMessage(&msg,NULL,0,0,PM_REMOVE /*| PM_NOYIELD*/);

			if(bMsgAvail) {
				if(msg.message == WM_QUIT)
		  		break;
				if(!TranslateAccelerator(msg.hwnd,hAccelTable,&msg)) {
					TranslateMessage(&msg); 	 /* Translates virtual key codes			 */
					DispatchMessage(&msg);		 /* Dispatches message to window			 */
					}
				}

/*
      if(!screenDivider) {
        // non faccio la divisione qua o diventa complicato riempire bene lo schermo, 240
				hDC=GetDC(ghWnd);
				UpdateScreen(hDC,VICRaster,VICRaster+8);
				ReleaseDC(ghWnd,hDC);
//        screenDivider++;
        }
      VICRaster+=8;     // 
      if(VICRaster>=256) {
        if(1 )
          VIDIRQ=1;
        screenDivider++;
        screenDivider %= 10;
        VICRaster=0;
        }
			*/
			hDC=GetDC(ghWnd);
			UpdateScreen(hDC,0,192);    // fare passate più piccole!
			ReleaseDC(ghWnd,hDC);
      TMS9918RegS |= B8(10000000);
      if(TMS9918Reg[1] & B8(00100000)) {
        VIDIRQ=1;
        }
      
//      LED1^=1;    // 42mS~ con SKYNET 7/6/20; 10~mS con Z80NE 10/7/21; 35mS GALAKSIJA 16/10/22; 30mS ZX80 27/10/22
      // QUADRUPLICO/ecc! 27/10/22
      
      
      }

		if(ColdReset) {
			ColdReset=0;
//			initHW();
      CPUPins |= DoReset;
			continue;
      }


    if(TIMIRQ) {
//      DoIRQ=1;
      TIMIRQ=0;
      }
    if(VIDIRQ) {
//      DoIRQ=1;
      VIDIRQ=0;
      }

    
		/*
		if((_pc >= 0xa000) && (_pc <= 0xbfff)) {
			printf("%04x    %02x\n",_pc,GetValue(_pc));
			}
			*/
		if(debug) {
			char myBuf[256],myBuf2[32];
			extern WORD GROMPtr;
			extern BYTE grom_seg[];			
			wsprintf(myBuf2,"%04X",_st.InterruptMask);
			myBuf2[4]='-'; myBuf2[5]='-'; myBuf2[6]='-'; myBuf2[7]='-';
#ifdef TMS9940 
			myBuf2[8]=_st.DigitCarry ? 'D' : ' ';
#else
			myBuf2[8]='-';
#endif
			myBuf2[9]=_st.XOP ? 'X' : ' ';
			myBuf2[10]=_st.Parity ? 'P' : ' ';
			myBuf2[11]=_st.Overflow ? 'O' : ' ';
			myBuf2[12]=_st.Carry ? 'C' : ' ';
			myBuf2[13]=_st.Zero ? 'Z' : ' ';
			myBuf2[14]=_st.ArithmeticGreater ? 'A' : ' ';
			myBuf2[15]=_st.LogicalGreater ? 'L' : ' ';
			myBuf2[16]=0;
			if(regs) {
				wsprintf(myBuf,"%04x: %04x "
					"%04x %04x %04x %04x %04x %04x %04x %04x "
					"%04x %04x %04x %04x %04x %04x %04x %04x "
					"(%16s)  %04X "
					"; %04X %02X\n",_pc,_wp,
					regs->r[0].x,regs->r[1].x,regs->r[2].x,regs->r[3].x,regs->r[4].x,regs->r[5].x,regs->r[6].x,regs->r[7].x,
					regs->r[8].x,regs->r[9].x,regs->r[10].x,regs->r[11].x,regs->r[12].x,regs->r[13].x,regs->r[14].x,regs->r[15].x,
					myBuf2, GetPipe(_pc) , 
					GROMPtr,grom_seg[GROMPtr-GROM_START]);
				}
			else
				strcpy(myBuf,"RESET\n");
			_lwrite(spoolFile/*myFile*/,myBuf,strlen(myBuf));

			{BYTE src[16];
			src[0]=GetPipe(_pc);
			src[1]=GetPipe(_pc+1);
			src[2]=GetPipe(_pc+2);
			src[2]=GetPipe(_pc+3);
//			Disassemble(src,-1,myBuf,0,_pc,7);
//			_lwrite(spoolFile,myBuf,strlen(myBuf));
			}
			}
		/*if(kbhit()) {
			getch();
			printf("%04x    %02x\n",_pc,GetValue(_pc));
			printf("281-284: %02x %02x %02x %02x\n",*(p1+0x281),*(p1+0x282),*(p1+0x283),*(p1+0x284));
			printf("2b-2c: %02x %02x\n",*(p1+0x2b),*(p1+0x2c));
			printf("33-34: %02x %02x\n",*(p1+0x33),*(p1+0x34));
			printf("37-38: %02x %02x\n",*(p1+0x37),*(p1+0x38));
			}*/
		if(CPUPins & DoReset) {
			_pc=GetIntValue(0x0002);
			_wp=GetIntValue(0x0000);
      _st.x=0;
			IPL=B8(0001);   // Ti99
			CPUPins &= ~(DoReset | DoIdle);
      initHW();
      continue;
			}
		if(CPUPins & DoLOAD) {
			CPUPins &= ~(DoLOAD | DoIdle);
//?? serve			IPL=B8(1111);
			_pc=GetIntValue(0xfffe);
			_wp=GetIntValue(0xfffc);

      }
		if(CPUPins & DoIRQ) {   // https://www.unige.ch/medecine/nouspikel/ti99/ints.htm
      
      // LED2^=1;    // 
			CPUPins &= ~(DoIdle);
      
			if(IPL <= _st.InterruptMask) {
//??				IPL = _st.InterruptMask;
				CPUPins &= ~DoIRQ;
        i=_wp;
    		_wp=GetIntValue(0x0000+IPL*2);
        regs->r[11].x=_pc;		// VERIFICARE!
  			_pc=GetIntValue(0x0002+IPL*2);
        regs->r[13].x=i;
        regs->r[15].x=_st.x;
       
				}
			}

  
		if(CPUPins & DoIdle) {
      //mettere ritardino per analogia con le istruzioni?
//      __delay_ns(500); non va più nulla... boh...
			continue;		// esegue cmq IRQ 
      }

//printf("Pipe1: %02x, Pipe2w: %04x, Pipe2b1: %02x,%02x\n",Pipe1,Pipe2.word,Pipe2.bytes.byte1,Pipe2.bytes.byte2);
    
    

//      LED2^=1;    // ~700nS 7/6/20, ~600 con 32bit 10/7/21

    
      if(_pc == 0x086 || _pc == 0x0ae ) {
				int T;
        T=0;
        }
    regs=(union Z_REGISTERS *)&ram_seg[_wp & 0xff /* -RAM_START */];     // così oppure cast diretto...
  
		GetPipe(_pc);
    _pc += 2;
execute:
    workingTS=WORKING_TS; workingTD=WORKING_TD;   // mettere solo dove serve??
    workingRegIndex=WORKING_REG_INDEX; workingReg2Index=WORKING_REG2_INDEX;
		switch(Pipe1 & B16(11110000,00000000)) {
      case B8(0000) << 12:
    		if(Pipe1 & B16(00001000,00000000)) {    // SLA SRA SRC SRL
          if(!(Pipe1 & B16(00000000,11110000))) { // count
            res2.b.l=regs->r[0].x >> 12;
            if(!res2.b.l)
              res2.b.l=16;
            }
          else {
            res2.b.l=(Pipe1 & B16(00000000,11110000)) >> 4;
            }
          res1.x=WORKING_REG;
        
          switch(Pipe1 & B16(11111111,00000000)) {
            case B8(00001010) << 8:     // SLA Shift left arithmetic
              while(res2.b.l--) {
                _st.Carry= res1.x & 0x8000 ? 1 : 0;
                res1.x <<= 1;
                res3.x=res1.x;
                }
              
aggRotate:
              WORKING_REG=res3.x;
              goto aggFlag16Z;
              break;
            case B8(00001000) << 8:     // SRA Shift right arithmetic
              while(res2.b.l--) {
                _st.Carry=res1.x & 0x1;
                res1.x >>= 1;
                if(res1.x & 0x4000)
                  res1.x |= 0x8000;
                res3.x=res1.x;
                }
              goto aggRotate;
              break;
            case B8(00001011) << 8:     // SRC Shift right circular
              while(res2.b.l--) {
                _st.Carry=res3.x & 1;
                res1.x >>= 1;
                if(_st.Carry)
                  res1.x |= 0x8000;
                res3.x=res1.x;
                }
              goto aggRotate;
              break;
            case B8(00001001) << 8:     // SRL Shift right logical
              while(res2.b.l--) {
                _st.Carry=res1.x & 0x1;
                res1.x >>= 1;
                res3.x=res1.x;
                }
              goto aggRotate;
              break;
            }
          }
        else {
          switch(Pipe1 & B16(11111111,11000000)) {
            case B8(0000001000) << 6:     // AI CI LI 
              switch(Pipe1 & B16(11111111,11100000)) {
                case B8(00000010001) << 5:     // AI Add immediate
                  res1.x=WORKING_REG;
                  res2.x=Pipe2.x;
                  res3.x=res1.x+res2.x;
                  
                  WORKING_REG=res3.x;

									_pc+=2;
                  
aggFlag16A:
                  _st.Carry=CARRY_ADD_16();
                  _st.Overflow = OVF_ADD_16();
									goto aggFlag16Z;
                  break;
                case B8(00000010000) << 5:     // LI Load immediate
                  WORKING_REG=Pipe2.x;
                  res3.x=WORKING_REG;
									_pc+=2;
                  goto aggFlag16Z;
                  break;
                }
              break;
              
            case B8(0000001001) << 6:     // ANDI ORI
              switch(Pipe1 & B16(11111111,11100000)) {
                case B8(00000010010) << 5:     // ANDI AND immediate
                  res1.x=WORKING_REG;
                  res2.x=Pipe2.x;
                  res3.x=res1.x & res2.x;
                  WORKING_REG=res3.x;
									_pc+=2;

aggFlag16Z:
                  _st.LogicalGreater=res3.x>0 ? 1 : 0;
                  _st.ArithmeticGreater=((int16_t)res3.x)>((int16_t)0) ? 1 : 0;
                  _st.Zero=res3.x ? 0 : 1;
                  break;
                case B8(00000010011) << 5:     // ORI OR immediate
                  res1.x=WORKING_REG;
                  res2.x=Pipe2.x;
                  res3.x=res1.x | res2.x;
                  WORKING_REG=res3.x;
									_pc+=2;
                  goto aggFlag16Z;
                  break;
                }
              break;
              
            case B8(0000010001) << 6:     // B Branch
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res3.x=WORKING_REG;
                  break;
                case REGISTER_INDIRECT:
// BOH no... tipo B  *R5                  res3.x=GetIntValue(WORKING_REG);
                  res3.x=WORKING_REG;
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
//                    res3.x=GetIntValue(WORKING_REG+(int16_t)Pipe2.x);
                    res3.x=WORKING_REG+(int16_t)Pipe2.x;
                  else
//                    res3.x=GetIntValue(Pipe2.x);
                    res3.x=Pipe2.x;
//                  _pc+=2;
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res3.x=GetIntValue(WORKING_REG);// BOH...
                  WORKING_REG+=2;
                  break;
                }
              _pc=res3.x;
              break;
            case B8(0000011010) << 6:     // BL Branch and Link
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res3.x=WORKING_REG;
                  break;
                case REGISTER_INDIRECT:
//                  res3.x=GetIntValue(WORKING_REG);
// BOH no... b. sopra
                  res3.x=WORKING_REG;
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
//                    res3.x=GetIntValue(WORKING_REG+(int16_t)Pipe2.x);
                    res3.x=WORKING_REG+(int16_t)Pipe2.x;
                  else
//                    res3.x=GetIntValue(Pipe2.x);
                    res3.x=Pipe2.x;
                  _pc+=2;
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res3.x=GetIntValue(WORKING_REG);    //boh..
                  WORKING_REG+=2;
                  break;
                }
              regs->r[11].x=_pc;
              _pc=res3.x;
              break;
            case B8(0000010000) << 6:     // BLWP Branch and Load Workspace Pointer
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res3.x=WORKING_REG;
                  break;
                case REGISTER_INDIRECT:
//                  res3.x=GetIntValue(WORKING_REG);
// BOH no... b. sopra
                  res3.x=WORKING_REG;
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
//                    res3.x=GetIntValue(WORKING_REG+(int16_t)Pipe2.x);
                    res3.x=WORKING_REG+(int16_t)Pipe2.x;
                  else
//                    res3.x=GetIntValue(Pipe2.x);
                    res3.x=Pipe2.x;
                  _pc+=2;
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res3.x=GetIntValue(WORKING_REG);//boh
                  WORKING_REG+=2;
                  break;
                }
              i=_wp;
          		_wp=GetIntValue(0x0000+res3.x);
              regs->r[14].x=_pc;
            	_pc=GetIntValue(0x0002+res3.x);
              regs->r[13].x=i;
              regs->r[15].x=_st.x;
              // saltare interrupt dopo di questa, dice...
              break;
            case B8(0000010011) << 6:     // CLR Clear Operand
              res3.x=0;
              
store16_2_noF:
              switch(workingTS) {
                case REGISTER_DIRECT:
                  WORKING_REG=res3.x;
                  break;
                case REGISTER_INDIRECT:
                  PutIntValue(WORKING_REG,res3.x);
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    PutIntValue(WORKING_REG+(int16_t)Pipe2.x,res3.x);
                  else
                    PutIntValue(Pipe2.x,res3.x);
                  _pc+=2;
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  PutIntValue(WORKING_REG,res3.x);
                  WORKING_REG2+=2;
                  break;
                }
              break;
            case B8(0000011100) << 6:     // SETO Set To Ones
              res3.x=0xffff;
              goto store16_2_noF;
              break;
            case B8(0000010101) << 6:     // INV Invert
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res1.x=WORKING_REG;
                  break;
                case REGISTER_INDIRECT:
                  res1.x=GetIntValue(WORKING_REG);
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    res1.x=GetIntValue(WORKING_REG+(int16_t)Pipe2.x);
                  else
                    res1.x=GetIntValue(Pipe2.x);
                  _pc+=2;
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res1.x=GetIntValue(WORKING_REG);
                  WORKING_REG+=2;
                  break;
                }
              res3.x=~res1.x;

store16_2:
              switch(workingTS) {
                case REGISTER_DIRECT:
                  WORKING_REG=res3.x;
                  break;
                case REGISTER_INDIRECT:
                  PutIntValue(WORKING_REG,res3.x);
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    PutIntValue(WORKING_REG+(int16_t)Pipe2.x,res3.x);
                  else
                    PutIntValue(Pipe2.x,res3.x);
                  _pc+=2;
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  PutIntValue(WORKING_REG,res3.x);
                  WORKING_REG2+=2;
                  break;
                }
            	goto aggFlag16Z;
              break;
            case B8(0000010100) << 6:     // NEG Negate
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res2.x=WORKING_REG;
                  break;
                case REGISTER_INDIRECT:
                  res2.x=GetIntValue(WORKING_REG);
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    res2.x=GetIntValue(WORKING_REG+(int16_t)Pipe2.x);
                  else
                    res2.x=GetIntValue(Pipe2.x);
                  _pc+=2;
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res2.x=GetIntValue(WORKING_REG);
                  WORKING_REG+=2;
                  break;
                }
              res1.x=0;
              res3.x=res1.x-res2.x;
              goto store16_2;
              break;
            case B8(0000011101) << 6:     // ABS Absolute Value
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res1.x=WORKING_REG;
                  break;
                case REGISTER_INDIRECT:
                  res1.x=GetIntValue(WORKING_REG);
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    res1.x=GetIntValue(WORKING_REG+(int16_t)Pipe2.x);
                  else
                    res1.x=GetIntValue(Pipe2.x);
                  _pc+=2;
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res1.x=GetIntValue(WORKING_REG);
                  WORKING_REG+=2;
                  break;
                }
              res3.x=abs(res3.x);
              
              goto store16_2;
              break;
            case B8(0000011011) << 6:     // SWPB Swap Bytes
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res1.x=WORKING_REG;
                  break;
                case REGISTER_INDIRECT:
                  res1.x=GetIntValue(WORKING_REG);
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    res1.x=GetIntValue(WORKING_REG+(int16_t)Pipe2.x);
                  else
                    res1.x=GetIntValue(Pipe2.x);
                  _pc+=2;
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res1.x=GetIntValue(WORKING_REG);
                  WORKING_REG++;
                  break;
                }
              res3.x=MAKEWORD(HIBYTE(res1.x),LOBYTE(res1.x));
              goto store16_2_noF;
              break;
            case B8(0000010110) << 6:     // INC Increment
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res1.x=WORKING_REG;
                  break;
                case REGISTER_INDIRECT:
                  res1.x=GetIntValue(WORKING_REG);
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    res1.x=GetIntValue(WORKING_REG+(int16_t)Pipe2.x);
                  else
                    res1.x=GetIntValue(Pipe2.x);
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res1.x=GetIntValue(WORKING_REG);
                  WORKING_REG+=2;
                  break;
                }
              res3.x=res1.x+1;
              
aggInc:
              _st.Overflow= !!(!(res1.x & 0x8000) && (res3.x & 0x8000));
              _st.Carry=res3.x ? 0 : 1;			// FINIRE se INCT
              goto store16_2;
              break;
            case B8(0000010111) << 6:     // INCT Increment by Two
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res1.x=WORKING_REG;
                  break;
                case REGISTER_INDIRECT:
                  res1.x=GetIntValue(WORKING_REG);
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    res1.x=GetIntValue(WORKING_REG+(int16_t)Pipe2.x);
                  else
                    res1.x=GetIntValue(Pipe2.x);
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res1.x=GetIntValue(WORKING_REG);
                  WORKING_REG+=2;
                  break;
                }
              res3.x=res1.x+2;
              goto aggInc;
              break;
            case B8(0000011000) << 6:     // DEC Decrement
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res1.x=WORKING_REG;
                  break;
                case REGISTER_INDIRECT:
                  res1.x=GetIntValue(WORKING_REG);
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    res1.x=GetIntValue(WORKING_REG+(int16_t)Pipe2.x);
                  else
                    res1.x=GetIntValue(Pipe2.x);
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res1.x=GetIntValue(WORKING_REG);
                  WORKING_REG+=2;
                  break;
                }
              res3.x=res1.x;
              res3.x--;
aggDec:
              _st.Overflow= !!((res1.x & 0x8000) && !(res3.x & 0x8000));
              _st.Carry=res3.x==0xffff ? 1 : 0;			// FINIRE se DECT
              goto store16_2;
              break;
            case B8(0000011001) << 6:     // DECT Decrement by Two
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res1.x=WORKING_REG;
                  break;
                case REGISTER_INDIRECT:
                  res1.x=GetIntValue(WORKING_REG);
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    res1.x=GetIntValue(WORKING_REG+(int16_t)Pipe2.x);
                  else
                    res1.x=GetIntValue(Pipe2.x);
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res1.x=GetIntValue(WORKING_REG);
                  WORKING_REG+=2;
                  break;
                }
              res3.x=res1.x;
              res3.x-=2;
              goto aggDec;
              break;
            case B8(0000010010) << 6:     // X Execute
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res3.x=WORKING_REG;
                  break;
                case REGISTER_INDIRECT:
                  res3.x=GetIntValue(WORKING_REG);
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    res3.x=GetIntValue(WORKING_REG+(int16_t)Pipe2.x);
                  else
                    res3.x=GetIntValue(Pipe2.x);
                  _pc+=2;
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res3.x=GetIntValue(WORKING_REG);
                  WORKING_REG+=2;
                  break;
                }
              Pipe1=res3.x;
              goto execute;
              break;
              
            case B8(0000001011) << 6:     // LWPI LIMI
              switch(Pipe1 & B16(11111111,11100000)) {
                case B8(00000010111) << 5:     // LWPI Load workspace pointer immediate
                  _wp=Pipe2.x;
                  _pc+=2;
                  break;
                  
                case B8(00000010110) << 5:     // STST Store status register
                  WORKING_REG=_st.x;
                  break;
                }
              break;
            case B8(0000001100) << 6:     // LWPI LIMI
              switch(Pipe1 & B16(11111111,11100000)) {
                case B8(00000011000) << 5:     // LIMI Load interrupt mask
                  _st.x=(_st.x & ~ID_INTERRUPTMASK) | (Pipe2.x & B8(0000000000001111));
                  _pc+=2;
                  break;
                }
              break;
            case B8(0000001010) << 6:     // STWP
              switch(Pipe1 & B16(11111111,11100000)) {
                case B8(00000010101) << 5:     // STWP Store workspace pointer
                  WORKING_REG=_wp;
                  break;
                case B8(00000010100) << 5:     // CI Compare immediate
                  res1.x=WORKING_REG;
                  res2.x=Pipe2.x;
                  res3.x=(uint16_t)res1.x-(uint16_t)res2.x;
									_pc+=2;
        
compare16:
                  _st.LogicalGreater=res1.x>res2.x ? 1 : 0;
                  _st.ArithmeticGreater=((int16_t)res1.x)>((int16_t)res2.x) ? 1 : 0;
                  _st.Zero=res3.x ? 0 : 1;
                  break;
                }
              break;
              
            case B8(0000001110) << 6:     // RTWP
              switch(Pipe1 & B16(11111111,11100000)) {
                case B8(00000011100) << 5:     // RTWP Return workspace pointer
                  _st.x=regs->r[15].x;
                  _pc=regs->r[14].x;
                  _wp=regs->r[13].x;
                  break;
                }
              break;
              
            case B8(0000001101) << 6:     // IDLE
              switch(Pipe1 & B16(11111111,11100000)) {
                case B8(00000011010) << 5:     // IDLE
          			  CPUPins |= DoIdle;
                  break;
                case B8(00000011011) << 5:     // RSET
                  _st.x &= B16(11111111,11110000);
                  break;
                }
              break;

            case B8(0000001111) << 6:     // CKOF CKON LREX
              switch(Pipe1 & B16(11111111,11100000)) {
                case B8(00000011110) << 5:     // CKOF
                  break;
                case B8(00000011101) << 5:     // CKON
                  break;
                case B8(00000011111) << 5:     // LREX
                  break;
                }
              break;
            }
          }
        break;
      
      case B8(0001) << 12:
    		switch(Pipe1 & B16(11111111,00000000)) {
          case B8(00010011) << 8:     // JEQ Jump equal
            if(_st.Zero)
              goto Jump;
            break;
          case B8(00010101) << 8:     // JGT Jump greater than
            if(_st.ArithmeticGreater)
              goto Jump;
            break;
          case B8(00011011) << 8:     // JH Jump high
            if(_st.LogicalGreater && !_st.Zero)
              goto Jump;
            break;
          case B8(00010100) << 8:     // JHE Jump high or equal
            if(_st.LogicalGreater || _st.Zero)
              goto Jump;
            break;
          case B8(00011010) << 8:     // JL Jump low
            if(!_st.LogicalGreater && !_st.Zero)
              goto Jump;
            break;
          case B8(00010010) << 8:     // JLE Jump low or equal
            if(!_st.LogicalGreater || _st.Zero)
              goto Jump;
            break;
          case B8(00010001) << 8:     // JLT Jump less than
            if(!_st.ArithmeticGreater && !_st.Zero)
              goto Jump;
            break;
          case B8(00010000) << 8:     // JMP Jump unconditional
Jump:
    				_pc += (int8_t)LOBYTE(Pipe1) *2;
            break;
          case B8(00010111) << 8:     // JNC Jump no carry
            if(!_st.Carry)
              goto Jump;
            break;
          case B8(00010110) << 8:     // JNE Jump not equal
            if(!_st.Zero)
              goto Jump;
            break;
          case B8(00011001) << 8:     // JNO Jump no overflow
            if(!_st.Overflow)
              goto Jump;
            break;
          case B8(00011000) << 8:     // JOC Jump carry
            if(_st.Carry)
              goto Jump;
            break;
          case B8(00011100) << 8:     // JOP Jump odd parity
            if(_st.Parity)
              goto Jump;
            break;
          }
        break;
        
      case B8(1010) << 12:    // A Add
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.x=WORKING_REG;
            break;
          case REGISTER_INDIRECT:
            res1.x=GetIntValue(WORKING_REG);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.x=GetIntValue(WORKING_REG+(int16_t)Pipe2.x);
            else
              res1.x=GetIntValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.x=GetIntValue(WORKING_REG);
            WORKING_REG+=2;
            break;
          }
        switch(workingTD) {
          case REGISTER_DIRECT:
            res2.x=WORKING_REG2;
            break;
          case REGISTER_INDIRECT:
            res2.x=GetIntValue(WORKING_REG2);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              res2.x=GetIntValue(WORKING_REG2+(int16_t)Pipe2.x);
            else
              res2.x=GetIntValue(Pipe2.x);
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res2.x=GetIntValue(WORKING_REG2);
            WORKING_REG2+=2;
            break;
          }
        res3.x=(uint16_t)res1.x+(uint16_t)res2.x;
        
store16:
        switch(workingTD) {
          case REGISTER_DIRECT:
            WORKING_REG2=res3.x;
            break;
          case REGISTER_INDIRECT:
            PutIntValue(WORKING_REG2,res3.x);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              PutIntValue(WORKING_REG2+(int16_t)Pipe2.x,res3.x);
            else
              PutIntValue(Pipe2.x,res3.x);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            PutIntValue(WORKING_REG2,res3.x);
            WORKING_REG2+=2;
            break;
          }
        goto aggFlag16A;
        break;
      case B8(1011) << 12:    // AB Add bytes
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.b.l=LOBYTE(WORKING_REG);
            break;
          case REGISTER_INDIRECT:
            res1.b.l=GetValue(WORKING_REG);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.b.l=GetValue(WORKING_REG+(int16_t)Pipe2.x);
            else
              res1.b.l=GetValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.b.l=GetValue(WORKING_REG);
            WORKING_REG++;
            break;
          }
        switch(workingTD) {
          case REGISTER_DIRECT:
            res2.b.l=LOBYTE(WORKING_REG2);
            break;
          case REGISTER_INDIRECT:
            res2.b.l=GetValue(WORKING_REG2);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              res2.b.l=GetValue(WORKING_REG2+(int16_t)Pipe2.x);
            else
              res2.b.l=GetValue(Pipe2.x);
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res2.b.l=GetValue(WORKING_REG2);
            WORKING_REG2++;
            break;
          }
        res3.b.l=res1.b.l+res2.b.l;
        
//        _st.Overflow = !!(((res1.b.l & 0x40) + (res2.b.l & 0x40)) & 0x80) != !!(((res1.x & 0x80) + (res2.x & 0x80)) & 0x100);
//        _st.Overflow = !!(((res1.b.h & 0x80) == (res2.b.h & 0x80)) && ((res3.b.h & 0x80) != (res2.b.h & 0x80)));
        _st.Overflow = OVF_ADD_8();
          
store8:
        _st.Carry=CARRY_ADD_8();
        
store8_012:
        switch(workingTD) {
          case REGISTER_DIRECT:
            WORKING_REG2=MAKEWORD(LOBYTE(WORKING_REG2),res3.b.l);		// se registro, va in MSB
            break;
          case REGISTER_INDIRECT:
            PutValue(WORKING_REG2,res3.b.l);			// v. di là, big-endian circa
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              PutValue(WORKING_REG2+(int16_t)Pipe2.x,res3.b.l);
            else
              PutValue(Pipe2.x,res3.b.l);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            PutValue(WORKING_REG2,res3.b.l);
            WORKING_REG2++;
            break;
          }
        
        _st.LogicalGreater=res3.b.l>0 ? 1 : 0;
        _st.ArithmeticGreater=((int8_t)res3.b.l)>((int8_t)0) ? 1 : 0;
        _st.Zero=res3.b.l ? 0 : 1;

calcParity:
        {
        BYTE par;
        par= res3.b.l >> 1;			// Microchip AN774
        par ^= res3.b.l;
        res3.b.l= par >> 2;
        par ^= res3.b.l;
        res3.b.l= par >> 4;
        par ^= res3.b.l;
        _st.Parity=par & 1 ? 0 : 1;   // ODD
        }
        break;

      case B8(1000) << 12:    // C Compare
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.x=WORKING_REG;
            break;
          case REGISTER_INDIRECT:
            res1.x=GetIntValue(WORKING_REG);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.x=GetIntValue(WORKING_REG+(int16_t)Pipe2.x);
            else
              res1.x=GetIntValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.x=GetIntValue(WORKING_REG);
            WORKING_REG+=2;
            break;
          }
        switch(workingTD) {
          case REGISTER_DIRECT:
            res2.x=WORKING_REG2;
            break;
          case REGISTER_INDIRECT:
            res2.x=GetIntValue(WORKING_REG2);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              res2.x=GetIntValue(WORKING_REG2+(int16_t)Pipe2.x);
            else
              res2.x=GetIntValue(Pipe2.x);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res2.x=GetIntValue(WORKING_REG2);
            WORKING_REG2+=2;
            break;
          }
        res3.x=(uint16_t)res1.x-(uint16_t)res2.x;
        
        goto compare16;
        break;
      case B8(1001) << 12:    // CB Compare bytes
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.b.l=LOBYTE(WORKING_REG);
            break;
          case REGISTER_INDIRECT:
            res1.b.l=GetValue(WORKING_REG);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.b.l=GetValue(WORKING_REG+(int16_t)Pipe2.x);
            else
              res1.b.l=GetValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.b.l=GetValue(WORKING_REG);
            WORKING_REG++;
            break;
          }
        switch(workingTD) {
          case REGISTER_DIRECT:
            res2.b.l=LOBYTE(WORKING_REG);
            break;
          case REGISTER_INDIRECT:
            res2.b.l=GetValue(WORKING_REG);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res2.b.l=GetValue(WORKING_REG2+(int16_t)Pipe2.x);
            else
              res2.b.l=GetValue(Pipe2.x);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res2.b.l=GetValue(WORKING_REG2);
            WORKING_REG2++;
            break;
          }
        res3.b.l=(uint16_t)res1.b.l-(uint16_t)res2.b.l;

compare8:        
        _st.LogicalGreater=res1.b.l>res2.b.l ? 1 : 0;
        _st.ArithmeticGreater=((int8_t)res1.b.l)>((int8_t)res2.b.l) ? 1 : 0;
        _st.Zero=res3.b.l ? 0 : 1;
        break;

      case B8(0110) << 12:    // S Subtract
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.x=WORKING_REG;
            break;
          case REGISTER_INDIRECT:
            res1.x=GetIntValue(WORKING_REG);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.x=GetIntValue(WORKING_REG+(int16_t)Pipe2.x);
            else
              res1.x=GetIntValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.x=GetIntValue(WORKING_REG);
            WORKING_REG+=2;
            break;
          }
        switch(workingTD) {
          case REGISTER_DIRECT:
            res2.x=WORKING_REG2;
            break;
          case REGISTER_INDIRECT:
            res2.x=GetIntValue(WORKING_REG2);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res2.x=GetIntValue(WORKING_REG2+(int16_t)Pipe2.x);
            else
              res2.x=GetIntValue(Pipe2.x);
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res2.x=GetIntValue(WORKING_REG2);
            WORKING_REG2+=2;
            break;
          }
        res3.x=res1.x-res2.x;
        
aggFlag16S:
        _st.Carry=CARRY_SUB_16();
//        _st.Overflow = !!(((res1.x & 0x4000) + (res2.x & 0x4000)) & 0x8000) != !!(((res1.d & 0x8000) + (res2.d & 0x8000)) & 0x10000);
//        _st.Overflow = !!(((res1.x & 0x8000) != (res2.x & 0x8000)) && ((res3.x & 0x8000) != (res2.x & 0x8000)));
        _st.Overflow = OVF_SUB_16();
        goto store16;
        break;
      case B8(0111) << 12:    // SB Subtract bytes
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.b.l=LOBYTE(WORKING_REG);
            break;
          case REGISTER_INDIRECT:
            res1.b.l=GetValue(WORKING_REG);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.b.l=GetValue(WORKING_REG+(int16_t)Pipe2.x);
            else
              res1.b.l=GetValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.b.l=GetValue(WORKING_REG);
            WORKING_REG++;
            break;
          }
        switch(workingTD) {
          case REGISTER_DIRECT:
            res2.b.l=LOBYTE(WORKING_REG2);
            break;
          case REGISTER_INDIRECT:
            res2.b.l=GetValue(WORKING_REG2);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              res2.b.l=GetValue(WORKING_REG2+(int16_t)Pipe2.x);
            else
              res2.b.l=GetValue(Pipe2.x);
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.b.l=GetValue(WORKING_REG2);
            WORKING_REG2++;
            break;
          }
        res3.b.l=res1.b.l-res2.b.l;
        _st.Carry=CARRY_SUB_8();
//        _st.Overflow = !!(((res1.b.h & 0x80) != (res2.b.h & 0x80)) && ((res3.b.h & 0x80) != (res2.b.h & 0x80)));
        _st.Overflow = OVF_SUB_8();
        goto store8;
        break;
      
      case B8(1110) << 12:    // SOC Set ones corresponding
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.x=WORKING_REG;
            break;
          case REGISTER_INDIRECT:
            res1.x=GetIntValue(WORKING_REG);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.x=GetIntValue(WORKING_REG+(int16_t)Pipe2.x);
            else
              res1.x=GetIntValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.x=GetIntValue(WORKING_REG);
            WORKING_REG+=2;
            break;
          }
        switch(workingTD) {
          case REGISTER_DIRECT:
            res2.x=WORKING_REG2;
            break;
          case REGISTER_INDIRECT:
            res2.x=GetIntValue(WORKING_REG2);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              res2.x=GetIntValue(WORKING_REG2+(int16_t)Pipe2.x);
            else
              res2.x=GetIntValue(Pipe2.x);
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res2.x=GetIntValue(WORKING_REG2);
            WORKING_REG2+=2;			// qua??
            break;
          }
        res3.x=res2.x | res1.x;
        
store16_012:
        switch(workingTD) {
          case REGISTER_DIRECT:
            WORKING_REG2=res3.x;
            break;
          case REGISTER_INDIRECT:
            PutIntValue(WORKING_REG2,res3.x);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              PutIntValue(WORKING_REG2+(int16_t)Pipe2.x,res3.x);
            else
              PutIntValue(Pipe2.x,res3.x);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            PutIntValue(WORKING_REG2,res3.x);
            WORKING_REG2+=2;
            break;
          }
        goto aggFlag16Z;
        break;
      case B8(1111) << 12:    // SOCB Set ones corresponding bytes  
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.b.l=LOBYTE(WORKING_REG);
            break;
          case REGISTER_INDIRECT:
            res1.b.l=GetValue(WORKING_REG);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.b.l=GetValue(WORKING_REG+(int16_t)Pipe2.x);
            else
              res1.b.l=GetValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.b.l=GetValue(WORKING_REG);
            WORKING_REG++;
            break;
          }
        switch(workingTD) {
          case REGISTER_DIRECT:
            res2.b.l=LOBYTE(WORKING_REG2);
            break;
          case REGISTER_INDIRECT:
            res2.b.l=GetValue(WORKING_REG2);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              res2.b.l=GetValue(WORKING_REG2+(int16_t)Pipe2.x);
            else
              res2.b.l=GetValue(Pipe2.x);
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res2.b.l=GetValue(WORKING_REG2);
            WORKING_REG2++;		// qua??
            break;
          }
        res3.b.l=res2.b.l | res1.b.l;
        
        goto store8_012;
        break;
      
      case B8(0100) << 12:    // SZC Set zeros corresponding
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.x=WORKING_REG;
            break;
          case REGISTER_INDIRECT:
            res1.x=GetIntValue(WORKING_REG);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.x=GetIntValue(WORKING_REG+(int16_t)Pipe2.x);
            else
              res1.x=GetIntValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.x=GetIntValue(WORKING_REG);
            WORKING_REG+=2;
            break;
          }
        switch(workingTD) {
          case REGISTER_DIRECT:
            res2.x=WORKING_REG2;
            break;
          case REGISTER_INDIRECT:
            res2.x=GetIntValue(WORKING_REG2);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              res2.x=GetIntValue(WORKING_REG2+(int16_t)Pipe2.x);
            else
              res2.x=GetIntValue(Pipe2.x);
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res2.x=GetIntValue(WORKING_REG2);
            WORKING_REG+=2;		// qua??
            break;
          }
        res3.x=res2.x & ~res1.x;
       
        goto store16_012;
        break;
      case B8(0101) << 12:    // SZCB Set zeros corresponding bytes  
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.b.l=LOBYTE(WORKING_REG);
            break;
          case REGISTER_INDIRECT:
            res1.b.l=GetValue(WORKING_REG);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.b.l=GetValue(WORKING_REG+(int16_t)Pipe2.x);
            else
              res1.b.l=GetValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.b.l=GetValue(WORKING_REG);
            WORKING_REG++;
            break;
          }
        switch(workingTD) {
          case REGISTER_DIRECT:
            res2.b.l=LOBYTE(WORKING_REG2);
            break;
          case REGISTER_INDIRECT:
            res2.b.l=GetValue(WORKING_REG2);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              res2.b.l=GetValue(WORKING_REG2+(int16_t)Pipe2.x);
            else
              res2.b.l=GetValue(Pipe2.x);
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res2.b.l=GetValue(WORKING_REG2);
            WORKING_REG2++;		// qua??
            break;
          }
        res3.b.l=res2.b.l & ~res1.b.l;
        
        goto store8_012;
        break;
      
      case B8(1100) << 12:    // MOV Move
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.x=WORKING_REG;
            break;
          case REGISTER_INDIRECT:
            res1.x=GetIntValue(WORKING_REG);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.x=GetIntValue(WORKING_REG+(int16_t)Pipe2.x);
            else
              res1.x=GetIntValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.x=GetIntValue(WORKING_REG);
            WORKING_REG+=2;
            break;
          }
        res3.x=res1.x;
        goto store16_012;
        break;
      case B8(1101) << 12:    // MOVB Move bytes  
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.b.l=HIBYTE(WORKING_REG);			// se registro, uso MSB
            break;
          case REGISTER_INDIRECT:
            res1.b.l=GetValue(WORKING_REG);		// v. di là, big-endian
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.b.l=GetValue(WORKING_REG+(int16_t)Pipe2.x);
            else
              res1.b.l=GetValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.b.l=GetValue(WORKING_REG);
            WORKING_REG++;
            break;
          }
        res3.b.l=res1.b.l;
        goto store8_012;
        break;
      
      
      case B8(0010) << 12:    // Compare Ones, Compare Zeros, Exclusive OR
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.x=WORKING_REG;
            break;
          case REGISTER_INDIRECT:
            res1.x=GetIntValue(WORKING_REG);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.x=GetIntValue(WORKING_REG+(int16_t)Pipe2.x);
            else
              res1.x=GetIntValue(Pipe2.x);
//            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.x=GetIntValue(WORKING_REG);
            WORKING_REG+=2;
            break;
          }
    		switch(Pipe1 & B16(11111100,00000000)) {
          case B8(001000) << 10:     // COC Compare Ones corresponding
            if((res1.x & WORKING_REG2) == res1.x)
              _st.Zero=1;
            break;
          case B8(001001) << 10:     // CZC Compare Zeros corresponding
            if((~res1.x & ~WORKING_REG2) == ~res1.x)
              _st.Zero=1;
            break;
          case B8(001010) << 10:     // XOR Exclusive OR
            res2.x=WORKING_REG2;
            res3.x = res1.x ^ res2.x;
            WORKING_REG2=res3.x;
            goto aggFlag16Z;
            break;
          case B8(001011) << 10:     // XOP Extended Operation
#ifdef TMS9940 
        		switch(Pipe1 & B16(00000011,11000000)) {
              case B8(0000) << 6:   // DCA      verificare!!
                res3.b.l=res1.b.l;
                i=_st.Carry;
                _st.Carry=0;
                if((_res1.b.l & 0xf) > 9 || _st.DigitCarry) {
                  res3.x+=6;
                  res1.b.l=res3.b.l;
                  _st.Carry= i || res3.b.h;
                  _st.DigitCarry=1;
                  }
                else
                  _f.HalfCarry=0;
                if((res1.b.l>0x99) || i) {
                  res3.b.l+=0x60;  
                  _st.Carry=1;
                  }
                else
                  _st.Carry=0;
                switch(workingTD) {
                  case REGISTER_DIRECT:
                    WORKING_REG2=MAKEWORD(res3.b.l,HIBYTE(WORKING_REG2));
                    break;
                  case REGISTER_INDIRECT:
                    PutValue(WORKING_REG2,res3.b.l);
                    break;
                  case REGISTER_SYMBOLIC_INDEXED:
                    if(workingReg2Index)
                      PutValue(WORKING_REG2+(int16_t)Pipe2.x,res3.b.l);
                    else
                      PutValue(Pipe2.x,res3.b.l);
                    _pc+=2;
                    break;
                  case REGISTER_INDIRECT_AUTOINCREMENT:
                    PutValue(WORKING_REG2,res3.b.l);
                    WORKING_REG2+=2;
                    break;
                  }
store_dca:
								_st.LogicalGreater=res3.b.l>0 ? 1 : 0;
								_st.ArithmeticGreater=((int8_t)res3.b.l)>((int8_t)0) ? 1 : 0;
                _st.Zero=res3.b.l ? 0 : 1;
                goto calcParity;
                break;
              case B8(0001) << 6:   // DCS      verificare!! finire
                res3.b.l=res1.b.l;
                i=_st.Carry;
                _st.Carry=0;
                if((_res1.b.l & 0xf) > 9 || _st.DigitCarry) {
                  res3.x+=6;
                  res1.b.l=res3.b.l;
                  _st.Carry= i || res3.b.h;
                  _st.DigitCarry=1;
                  }
                else
                  _f.HalfCarry=0;
                if((res1.b.l>0x99) || i) {
                  res3.b.l+=0x60;  
                  _st.Carry=1;
                  }
                else
                  _st.Carry=0;
                goto store_dca;
                break;
              case B8(0010) << 6:   // LIIM
                _st.x=(_st.x & B16(11111111,11111100)) | (Pipe2.x & B16(00000000,00000011));
                break;
              default:   // XOP
                i=_wp;
                _wp=GetIntValue(0x0040+regs->r[(Pipe1 & B16(11,11000000)) >> 4].x*4);
                regs->r[11].x=res3.x;
                regs->r[13].x=i;
                regs->r[14].x=_pc;
                regs->r[15].x=_st.x;
                _st.XOP=1;
                _pc=GetIntValue(0x0042+regs->r[(Pipe1 & B16(11,11000000)) >> 4].x*4);
                break;
              }
            
#else
            i=_wp;
         		_wp=GetIntValue(0x0040+regs->r[(Pipe1 & B16(11,11000000)) >> 4].x*4);
            regs->r[11].x=res3.x;
            regs->r[13].x=i;
            regs->r[14].x=_pc;
            regs->r[15].x=_st.x;
            _st.XOP=1;
           	_pc=GetIntValue(0x0042+regs->r[(Pipe1 & B16(11,11000000)) >> 4].x*4);
            break;
#endif
          }
        break;
      
      case B8(0011) << 12:    // Multiply, Divide
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.x=WORKING_REG;
            break;
          case REGISTER_INDIRECT:
            res1.x=GetIntValue(WORKING_REG);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.x=GetIntValue(WORKING_REG+(int16_t)Pipe2.x);
            else
              res1.x=GetIntValue(Pipe2.x);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.x=GetIntValue(WORKING_REG);
            WORKING_REG+=2;
            break;
          }
    		switch(Pipe1 & B8(1111110000000000)) {
          case B8(001110) << 10:     // MPY Multiply
            res3.d = res1.x * res2.x;
            WORKING_REG2=HIWORD(res3.d);
            regs->r[(((Pipe1 >> 8) +1) & 0xf)].x=res3.x;   // OKKIO, porcata, & se 15...
//no!            goto aggFlag;
            break;
          case B8(001111) << 10:     // DIV Divide
            res2.d = MAKELONG(WORKING_REG2,regs->r[(((Pipe1 >> 8) +1) & 0xf)].x);    // OKKIO...
            if(!res1.x) {
              //DIVIDE ZERO??
              }
    /*    da0 = (divident >> 16);     // https://hackaday.io/project/20826-tms9900-compatible-cpu-core-in-vhdl/log/67326-success-fpga-based-ti-994a-working
    da1 = divident & 0xFFFF;
    sa = divisor;
    
    int st4;
    if( (((sa & 0x8000) == 0 && (da0 & 0x8000) == 0x8000))
      || ((sa & 0x8000) == (da0 & 0x8000) && (((da0 - sa) & 0x8000) == 0)) ) {
      st4 = 1;
      } 
    else {
      st4 = 0;
      // actual division loop, here sa is known to be larger than da0.
      for(int i=0; i<16; i++) {
          da0 = (da0 << 1) | ((da1 >> 15) & 1);
          da1 <<= 1;
          if(da0 >= sa) {
              da0 -= sa;
              da1 |= 1;   // successful substraction
          }
      }
      }*/
            if( (((res1.x & 0x8000) == 0 && (res2.x & 0x8000) == 0x8000))
              || ((res1.x & 0x8000) == (res2.x & 0x8000) && (((res2.x - res1.x) & 0x8000) == 0)) )
              _st.Overflow = 1;
            else {
              res3.d = res2.d / (uint32_t)res1.x;
              WORKING_REG2=LOWORD(res3.d);
              regs->r[(((Pipe1 >> 8) +1) & 0xf)].x=res2.d % (uint32_t)res1.x;   // OKKIO, porcata, & se 15...
              _st.Overflow = 0;
              }
            break;
            
          case B8(001100) << 10:     // LDCR Load communication register
//                GetValueCRU();
//                PutValueCRU();
            break;
          case B8(001101) << 10:     // STCR Store communication register
//                GetValueCRU();
//                PutValueCRU();
            break;
            
          case B8(000111) << 10:     // SBO SBZ TB (CRU operations)
        		switch(Pipe1 & B16(11111111,00000000)) {    // https://www.unige.ch/medecine/nouspikel/ti99/cru.htm
              case B8(00111101) << 8:     // SBO Set bit to one
//                GetValueCRU();
//                PutValueCRU();
                break;
              case B8(00111110) << 8:     // SBZ Set bit to zero
//                GetValueCRU();
//                PutValueCRU();
                break;
              case B8(00111111) << 8:     // TB Test bit 
//                GetValueCRU();
                break;
              }
            break;
          }
        break;

        
			
			}
		} while(!fExit);
	}


