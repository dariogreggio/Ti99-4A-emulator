//https://onlinedisassembler.com/odaweb/
//https://github.com/kstenerud/Musashi/tree/master

//NB  in byte access, the CPU outputs the byte on the lower as well as the upper eight data lines.


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




int T1;
HFILE spoolFile;



BYTE fExit=0;
SWORD Pipe1;
extern BOOL debug;
SWORD VICRaster;
extern BYTE ram_seg[];

volatile BYTE TIMIRQ,VIDIRQ;


BYTE CPUPins=DoReset;
WORD CPUClock=2000000L/CPU_CLOCK_DIVIDER,HWClock=1000000L/CPU_CLOCK_DIVIDER;
#define MAX_WATCHDOG 100      // x30mS v. sotto
WORD WDCnt=MAX_WATCHDOG;
BYTE ColdReset=1;
union PIPE Pipe2;


// da 68000 Makushi o come cazzo si chiama :D
// res2 è Source e res1 è Dest ossia quindi res3=Result
//#define CARRY_ADD_8() (!!(((res2.b.l & res1.b.l) | (~res3.b.l & (res2.b.l | res1.b.l))) & 0x80))		// ((S & D) | (~R & (S | D)))
// V. DEC/INC! 
#define CARRY_ADD_8() (!!(res3.b.l < res2.b.l))
#define OVF_ADD_8()  (!!(((res2.b.l ^ res3.b.l) & (res1.b.l ^ res3.b.l)) & 0x80))			// ((S^R) & (D^R))
//#define CARRY_ADD_16() (!!(((res2.x & res1.x) | (~res3.x &	(res2.x | res1.x))) & 0x8000))
#define CARRY_ADD_16() (!!(res3.x < res2.x))
#define OVF_ADD_16() (!!(((res2.x ^ res3.x) & (res1.x ^ res3.x)) & 0x8000))
#define CARRY_SUB_8() (!!(((res2.b.l & res3.b.l) | (~res1.b.l & (res2.b.l | res3.b.l))) & 0x80))		// ((S & R) | (~D & (S | R)))
//#define CARRY_SUB_8() (!!((res3.b.l<res2.b.l) || (res2.b.l==0)))
#define OVF_SUB_8()  (!!(((res2.b.l ^ res1.b.l) & (res3.b.l ^ res1.b.l)) & 0x80))			// ((S^D) & (R^D))
#define CARRY_SUB_16() (!!(((res2.x & res3.x) | (~res1.x &	(res2.x | res3.x))) & 0x8000))
//#define CARRY_SUB_16() (!!((res3.x<res2.x) || (res2.x==0)))
#define OVF_SUB_16() (!!(((res2.x ^ res1.x) & (res3.x ^ res1.x)) & 0x8000))

extern BYTE TMS9918Reg[8],TMS9918RegS;
 

#ifdef _DEBUG
	SWORD _pc=0;
#endif
int Emulate(int mode) {
	BOOL bMsgAvail;
	MSG msg;
	HDC hDC;
	
#ifndef _DEBUG
	SWORD _pc=0;
#endif
	SWORD _wp=0;
	BYTE IPL=0;
  union T_REGISTERS *regs=NULL;
  union RESULT res1,res2,res3;
//  union OPERAND op1,op2;
	union REGISTRO_F _st;
	/*register*/ uint16_t i;
  uint8_t workingTS,workingTD,workingRegIndex,workingReg2Index;
  int c=0;

	DWORD cyclesPerSec,cyclesSoFar,cyclesCPU,cyclesHW;
	BYTE screenDivider;

	cyclesPerSec=10000000L;		// AT
	cyclesCPU=0; cyclesHW=0;
	cyclesSoFar=0;

  _pc=GetIntValue(0x0002);
  _wp=GetIntValue(0x0000);
  _st.x=0;
  IPL=B8(0001);   // Ti99
  
  

	do {

		cyclesSoFar++;

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

      if(!screenDivider) {
        // non faccio la divisione qua o diventa complicato riempire bene lo schermo, 240
				hDC=GetDC(ghWnd);
				UpdateScreen(hDC,VICRaster,VICRaster+17);
				ReleaseDC(ghWnd,hDC);
//        screenDivider++;
        }
      VICRaster+=16;     // 
      if(VICRaster>=192) {
				TMS9918RegS |= B8(10000000);
				if(TMS9918Reg[1] & B8(00100000)) {
					VIDIRQ=1;
					}
        screenDivider++;
        screenDivider %= 10;
        VICRaster=0;
        }

/*
			hDC=GetDC(ghWnd);
			UpdateScreen(hDC,0,192);    // fare passate più piccole!
			ReleaseDC(ghWnd,hDC);
						*/
      
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
      CPUPins |= DoIRQ;
      VIDIRQ=0;
      }

    
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
      
			if(IPL <= _st.InterruptMask) {		// TMS9900 will perform BLWP @>0000 through BLWP @>003C depending on the interrupt level. 
//??				IPL = _st.InterruptMask;
				CPUPins &= ~DoIRQ;
        i=_wp;
    		_wp=GetIntValue(0x0006+IPL*2);
		    regs=(union T_REGISTERS *)&ram_seg[_wp & 0xff /* -RAM_START */];     // così oppure cast diretto...
        SET_REG(14,_pc);		// VERIFICARE!
  			_pc=GetIntValue(0x0004+IPL*2);
        SET_REG(13,i);
        SET_REG(15,_st.x);

				}
			}

  
		if(CPUPins & DoIdle) {
      //mettere ritardino per analogia con le istruzioni?
//      __delay_ns(500); non va più nulla... boh...
			continue;		// esegue cmq IRQ 
      }

//printf("Pipe1: %02x, Pipe2w: %04x, Pipe2b1: %02x,%02x\n",Pipe1,Pipe2.word,Pipe2.bytes.byte1,Pipe2.bytes.byte2);
    
    

//      LED2^=1;    // ~700nS 7/6/20, ~600 con 32bit 10/7/21

		if(cyclesSoFar<cyclesCPU)		//
			goto rallenta;
		cyclesCPU += CPUClock;

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
				wsprintf(myBuf,"%04x:[%04x] "
					"%04x %04x %04x %04x %04x %04x %04x %04x-"
					"%04x %04x %04x %04x %04x %04x %04x %04x "
					"(%16s)  %04X "
					"; %04X %02X"
//					"%02X %02X %02X"
					"\n",_pc,_wp,
					GET_REG(0),GET_REG(1),GET_REG(2),GET_REG(3),GET_REG(4),GET_REG(5),GET_REG(6),GET_REG(7),
					GET_REG(8),GET_REG(9),GET_REG(10),GET_REG(11),GET_REG(12),GET_REG(13),GET_REG(14),GET_REG(15),
					myBuf2, GetPipe(_pc) , 
					GROMPtr,grom_seg[GROMPtr-GROM_START]
					/*,
					ram_seg[0x72],ram_seg[0x73],ram_seg[0xd0]*/);
				}
			else
				strcpy(myBuf,"RESET\n");
			_lwrite(spoolFile/*myFile*/,myBuf,strlen(myBuf));

			{WORD src[16];
			src[0]=GetPipe(_pc);
			src[1]=GetPipe(_pc+2);
			src[2]=GetPipe(_pc+4);
			src[3]=GetPipe(_pc+6);
			Disassemble(src,-1,myBuf,0,_pc,7);
			_lwrite(spoolFile,myBuf,strlen(myBuf));
			if(_pc==0x78) {		// fetch GROM token
				extern BYTE GROMBuffer;
				wsprintf(myBuf,"GROMcode %02X\n",GROMBuffer);
				_lwrite(spoolFile,myBuf,strlen(myBuf));
				}
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
    
		{			extern WORD GROMPtr;
      if(_pc == 0xd84  /*_pc == 0xb24 && */ /*_pc == 0xc0c*/ /* && GROMPtr==0x1d7*/) {
				int T;
        T=0;
        }
		}
    regs=(union T_REGISTERS *)&ram_seg[_wp & 0xff /* -RAM_START */];     // così oppure cast diretto...
  
		GetPipe(_pc);
    _pc += 2;
execute:
    workingTS=WORKING_TS; workingTD=WORKING_TD;   // mettere solo dove serve??
    workingRegIndex=WORKING_REG_INDEX; workingReg2Index=WORKING_REG2_INDEX;
		switch(Pipe1 & B16(11110000,00000000)) {
      case B8(0000) << 12:
    		if(Pipe1 & B16(00001000,00000000)) {    // SLA SRA SRC SRL
          if(!(Pipe1 & B16(00000000,11110000)))  // count
            res2.b.l=GET_REG(0) & 0xf;
          else
            res2.b.l=(uint8_t)(Pipe1 & B16(00000000,11110000)) >> 4;
          if(!res2.b.l)
            res2.b.l=16;
          res1.x=GET_WORKING_REG();
        
          switch(Pipe1 & B16(11111111,00000000)) {
            case B8(00001010) << 8:     // SLA Shift left arithmetic
              res3.x=res1.x;
              while(res2.b.l--) {
                _st.Carry= res1.x & 0x8000 ? 1 : 0;
                res1.x <<= 1;
                if((res1.x & 0x8000) != (res3.x & 0x8000))
									_st.Overflow=1;
                res3.x=res1.x;
                }
              
aggRotate:
              SET_WORKING_REG(res3.x);
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
              res3.x=res1.x;
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
          }		// SLA ecc
        else {
          switch(Pipe1 & B16(11111111,11000000)) {
            case B8(0000001000) << 6:     // AI LI 
              switch(Pipe1 & B16(11111111,11100000)) {
                case B8(00000010001) << 5:     // AI Add immediate
                  res2.x=GET_WORKING_REG();
                  res1.x=Pipe2.x;
                  res3.x=res1.x+res2.x;
                  SET_WORKING_REG(res3.x);
									_pc+=2;
                  
aggFlag16A:
                  _st.Carry=CARRY_ADD_16();
                  _st.Overflow = OVF_ADD_16();
									goto aggFlag16Z;
                  break;
                case B8(00000010000) << 5:     // LI Load immediate
                  res3.x=Pipe2.x;
									SET_WORKING_REG(res3.x);
									_pc+=2;
                  goto aggFlag16Z;
                  break;
                }
              break;
              
            case B8(0000001001) << 6:     // ANDI ORI
              switch(Pipe1 & B16(11111111,11100000)) {
                case B8(00000010010) << 5:     // ANDI AND immediate
                  res1.x=GET_WORKING_REG();
                  res2.x=Pipe2.x;
                  res3.x=res1.x & res2.x;
                  SET_WORKING_REG(res3.x);
									_pc+=2;

aggFlag16Z:
                  _st.LogicalGreater=res3.x>0 ? 1 : 0;
                  _st.ArithmeticGreater=((int16_t)res3.x)>((int16_t)0) ? 1 : 0;
                  _st.Zero=res3.x ? 0 : 1;
                  break;
                case B8(00000010011) << 5:     // ORI OR immediate
                  res1.x=GET_WORKING_REG();
                  res2.x=Pipe2.x;
                  res3.x=res1.x | res2.x;
                  SET_WORKING_REG(res3.x);
									_pc+=2;
                  goto aggFlag16Z;
                  break;
                }
              break;
              
            case B8(0000010001) << 6:     // B Branch
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res3.x=GET_WORKING_REG();
                  break;
                case REGISTER_INDIRECT:
// BOH no... tipo B  *R5                  res3.x=GetIntValue(WORKING_REG);
                  res3.x=GET_WORKING_REG();
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    res3.x=GET_WORKING_REG()+(int16_t)Pipe2.x;
                  else
                    res3.x=Pipe2.x;
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res3.x=GetIntValue(GET_WORKING_REG());// BOH...
                  SET_WORKING_REG(GET_WORKING_REG()+2);
                  break;
                }
              _pc=res3.x;
              break;
            case B8(0000011010) << 6:     // BL Branch and Link
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res3.x=GET_WORKING_REG();
                  break;
                case REGISTER_INDIRECT:
                  res3.x=GET_WORKING_REG();
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    res3.x=GET_WORKING_REG()+(int16_t)Pipe2.x;
                  else
                    res3.x=Pipe2.x;
                  _pc+=2;
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res3.x=GetIntValue(GET_WORKING_REG());    //boh..
                  SET_WORKING_REG(GET_WORKING_REG()+2);
                  break;
                }
              SET_REG(11,_pc);
              _pc=res3.x;
              break;
            case B8(0000010000) << 6:     // BLWP Branch and Load Workspace Pointer
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res3.x=GET_WORKING_REG();
                  break;
                case REGISTER_INDIRECT:
                  res3.x=GET_WORKING_REG();
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
//                    res3.x=GetIntValue(WORKING_REG+(int16_t)Pipe2.x);
                    res3.x=GET_WORKING_REG()+(int16_t)Pipe2.x;
                  else
//                    res3.x=GetIntValue(Pipe2.x);
                    res3.x=Pipe2.x;
                  _pc+=2;
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res3.x=GetIntValue(GET_WORKING_REG());//boh
                  SET_WORKING_REG(GET_WORKING_REG()+2);
                  break;
                }
              i=_wp;
          		_wp=GetIntValue(0x0000+res3.x);
						  regs=(union T_REGISTERS *)&ram_seg[_wp & 0xff /* -RAM_START */];     // così oppure cast diretto...
              SET_REG(14,_pc);
            	_pc=GetIntValue(0x0002+res3.x);
              SET_REG(13,i);
              SET_REG(15,_st.x);
              // saltare interrupt dopo di questa, dice...
              break;
            case B8(0000010011) << 6:     // CLR Clear Operand
              res3.x=0;
              
store16_S_noF:
              switch(workingTS) {
                case REGISTER_DIRECT:
                  SET_WORKING_REG(res3.x);
                  break;
                case REGISTER_INDIRECT:
                  PutIntValue(GET_WORKING_REG(),res3.x);
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    PutIntValue(GET_WORKING_REG()+(int16_t)Pipe2.x,res3.x);
                  else
                    PutIntValue(Pipe2.x,res3.x);
                  _pc+=2;
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  PutIntValue(GET_WORKING_REG(),res3.x);
                  SET_WORKING_REG(GET_WORKING_REG()+2);
                  break;
                }
              break;
            case B8(0000011100) << 6:     // SETO Set To Ones
              res3.x=0xffff;
              goto store16_S_noF;
              break;
            case B8(0000010101) << 6:     // INV Invert
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res1.x=GET_WORKING_REG();
                  break;
                case REGISTER_INDIRECT:
                  res1.x=GetIntValue(GET_WORKING_REG());
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    res1.x=GetIntValue(GET_WORKING_REG()+(int16_t)Pipe2.x);
                  else
                    res1.x=GetIntValue(Pipe2.x);
                  _pc+=2;
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res1.x=GetIntValue(GET_WORKING_REG());
                  SET_WORKING_REG(GET_WORKING_REG()+2);
                  break;
                }
              res3.x=~res1.x;

store16_S:
              switch(workingTS) {
                case REGISTER_DIRECT:
                  SET_WORKING_REG(res3.x);
                  break;
                case REGISTER_INDIRECT:
                  PutIntValue(GET_WORKING_REG(),res3.x);
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    PutIntValue(GET_WORKING_REG()+(int16_t)Pipe2.x,res3.x);
                  else
                    PutIntValue(Pipe2.x,res3.x);
                  _pc+=2;
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  PutIntValue(GET_WORKING_REG(),res3.x);
                  SET_WORKING_REG(GET_WORKING_REG()+2);
                  break;
                }
            	goto aggFlag16Z;
              break;
            case B8(0000010100) << 6:     // NEG Negate
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res2.x=GET_WORKING_REG();
                  break;
                case REGISTER_INDIRECT:
                  res2.x=GetIntValue(GET_WORKING_REG());
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    res2.x=GetIntValue(GET_WORKING_REG()+(int16_t)Pipe2.x);
                  else
                    res2.x=GetIntValue(Pipe2.x);
//                  _pc+=2;
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res2.x=GetIntValue(GET_WORKING_REG());
//                  SET_WORKING_REG(GET_WORKING_REG()+2);
                  break;
                }
              res1.x=0;
              res3.x=res1.x-res2.x;
							_st.Carry=CARRY_SUB_16();
			        _st.Overflow = OVF_SUB_16();
              goto store16_S;
              break;
            case B8(0000011101) << 6:     // ABS Absolute Value
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res1.x=GET_WORKING_REG();
                  break;
                case REGISTER_INDIRECT:
                  res1.x=GetIntValue(GET_WORKING_REG());
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    res1.x=GetIntValue(GET_WORKING_REG()+(int16_t)Pipe2.x);
                  else
                    res1.x=GetIntValue(Pipe2.x);
//                  _pc+=2;
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res1.x=GetIntValue(GET_WORKING_REG());
//                  SET_WORKING_REG(GET_WORKING_REG()+2);
                  break;
                }
              res3.x=abs(res1.x);

							//flag van controllati PRIMA!!! 
              
              goto store16_S;
              break;
            case B8(0000011011) << 6:     // SWPB Swap Bytes
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res1.x=GET_WORKING_REG();
                  break;
                case REGISTER_INDIRECT:
                  res1.x=GetIntValue(GET_WORKING_REG());
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    res1.x=GetIntValue(GET_WORKING_REG()+(int16_t)Pipe2.x);
                  else
                    res1.x=GetIntValue(Pipe2.x);
//                  _pc+=2;
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res1.x=GetIntValue(GET_WORKING_REG());
//                  SET_WORKING_REG(GET_WORKING_REG()+1);
                  break;
                }
              res3.x=MAKEWORD(HIBYTE(res1.x),LOBYTE(res1.x));
              goto store16_S_noF;
              break;
            case B8(0000010110) << 6:     // INC Increment
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res1.x=GET_WORKING_REG();
                  break;
                case REGISTER_INDIRECT:
                  res1.x=GetIntValue(GET_WORKING_REG());
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    res1.x=GetIntValue(GET_WORKING_REG()+(int16_t)Pipe2.x);
                  else
                    res1.x=GetIntValue(Pipe2.x);
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res1.x=GetIntValue(GET_WORKING_REG());
//                  SET_WORKING_REG(GET_WORKING_REG()+2);
                  break;
                }
              res3.x=res1.x+1;
//              _st.Carry= res3.x & 0x10 ? 1 : 0;		// v. ti99sim
              _st.Carry= res3.x < 1 ? 1 : 0;		// v. classic99
//					  _st.Overflow= (x3==0x8000) ? 1 : 0;
              _st.Overflow= !!(!(res1.x & 0x8000) && (res3.x & 0x8000));
              goto store16_S;
              break;
            case B8(0000010111) << 6:     // INCT Increment by Two
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res1.x=GET_WORKING_REG();
                  break;
                case REGISTER_INDIRECT:
                  res1.x=GetIntValue(GET_WORKING_REG());
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    res1.x=GetIntValue(GET_WORKING_REG()+(int16_t)Pipe2.x);
                  else
                    res1.x=GetIntValue(Pipe2.x);
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res1.x=GetIntValue(GET_WORKING_REG());
//                  SET_WORKING_REG(GET_WORKING_REG()+2);
                  break;
                }
              res3.x=res1.x+2;
              _st.Carry= res3.x < 2 ? 1 : 0;		// v. classic99
//					  _st.Overflow= ((x3==0x8000)||(x3==0x8001)) ? 1 : 0;
              _st.Overflow= !!(!(res1.x & 0x8000) && (res3.x & 0x8000));
              goto store16_S;
              break;
            case B8(0000011000) << 6:     // DEC Decrement
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res1.x=GET_WORKING_REG();
                  break;
                case REGISTER_INDIRECT:
                  res1.x=GetIntValue(GET_WORKING_REG());
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    res1.x=GetIntValue(GET_WORKING_REG()+(int16_t)Pipe2.x);
                  else
                    res1.x=GetIntValue(Pipe2.x);
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res1.x=GetIntValue(GET_WORKING_REG());
//                  SET_WORKING_REG(GET_WORKING_REG()+2);
                  break;
                }
              res3.x=res1.x-1;
//              _st.Carry= res3.x & 0x10 ? 0 : 1;		// v. ti99sim
              _st.Carry= res3.x != 0xffff ? 1 : 0;		// v. classic99
              _st.Overflow= !!((res1.x & 0x8000) && !(res3.x & 0x8000));
              goto store16_S;
              break;
            case B8(0000011001) << 6:     // DECT Decrement by Two
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res1.x=GET_WORKING_REG();
                  break;
                case REGISTER_INDIRECT:
                  res1.x=GetIntValue(GET_WORKING_REG());
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    res1.x=GetIntValue(GET_WORKING_REG()+(int16_t)Pipe2.x);
                  else
                    res1.x=GetIntValue(Pipe2.x);
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res1.x=GetIntValue(GET_WORKING_REG());
//                  SET_WORKING_REG(GET_WORKING_REG()+2);
                  break;
                }
              res3.x=res1.x-2;
//              _st.Carry= res3.x & 0x10 ? 0 : 1;		// v. ti99sim
              _st.Carry= res3.x < 0xfffe ? 1 : 0;		// v. classic99
              _st.Overflow= !!((res1.x & 0x8000) && !(res3.x & 0x8000));
              goto store16_S;
              break;
            case B8(0000010010) << 6:     // X Execute
              switch(workingTS) {
                case REGISTER_DIRECT:
                  res3.x=GET_WORKING_REG();
                  break;
                case REGISTER_INDIRECT:
                  res3.x=GetIntValue(GET_WORKING_REG());
                  break;
                case REGISTER_SYMBOLIC_INDEXED:
                  if(workingRegIndex)
                    res3.x=GetIntValue(GET_WORKING_REG()+(int16_t)Pipe2.x);
                  else
                    res3.x=GetIntValue(Pipe2.x);
                  _pc+=2;
                  break;
                case REGISTER_INDIRECT_AUTOINCREMENT:
                  res3.x=GetIntValue(GET_WORKING_REG());
                  SET_WORKING_REG(GET_WORKING_REG()+2);
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
                  SET_WORKING_REG(_st.x);
                  break;
                }
              break;
            case B8(0000001100) << 6:     // LWPI LIMI
              switch(Pipe1 & B16(11111111,11100000)) {
                case B8(00000011000) << 5:     // LIMI Load interrupt mask
                  _st.InterruptMask=Pipe2.x & B8(00001111);
                  _pc+=2;
                  break;
                }
              break;
            case B8(0000001010) << 6:     // STWP
              switch(Pipe1 & B16(11111111,11100000)) {
                case B8(00000010101) << 5:     // STWP Store workspace pointer
                  SET_WORKING_REG(_wp);
                  break;
                case B8(00000010100) << 5:     // CI Compare immediate
                  res1.x=GET_WORKING_REG();
                  res2.x=Pipe2.x;
									_pc+=2;
        
compare16:
                  _st.LogicalGreater=res1.x>res2.x ? 1 : 0;
                  _st.ArithmeticGreater=((int16_t)res1.x)>((int16_t)res2.x) ? 1 : 0;
                  _st.Zero=res1.x==res2.x ? 1 : 0;
                  break;
                }
              break;
              
            case B8(0000001110) << 6:     // RTWP
              switch(Pipe1 & B16(11111111,11100000)) {
                case B8(00000011100) << 5:     // RTWP Return workspace pointer
                  _st.x=GET_REG(15);
                  _pc=GET_REG(14);
                  _wp=GET_REG(13);
                  break;
                }
              break;
              
            case B8(0000001101) << 6:     // IDLE
              switch(Pipe1 & B16(11111111,11100000)) {
                case B8(00000011010) << 5:     // IDLE
          			  CPUPins |= DoIdle;
                  break;
                case B8(00000011011) << 5:     // RSET
                  _st.InterruptMask = 0;
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
    		switch(Pipe1 & B16(1111,00000000)) {
          case B8(1011) << 8:     // JH Jump high
            if(_st.LogicalGreater && !_st.Zero)
              goto Jump;
            break;
          case B8(1010) << 8:     // JL Jump low
            if(!_st.LogicalGreater && !_st.Zero)
              goto Jump;
            break;
          case B8(0100) << 8:     // JHE Jump high or equal
            if(_st.LogicalGreater || _st.Zero)
              goto Jump;
            break;
          case B8(0010) << 8:     // JLE Jump low or equal
            if(!_st.LogicalGreater || _st.Zero)
              goto Jump;
            break;
          case B8(0101) << 8:     // JGT Jump greater than
            if(_st.ArithmeticGreater)
              goto Jump;
            break;
          case B8(0001) << 8:     // JLT Jump less than
            if(!_st.ArithmeticGreater && !_st.Zero)
              goto Jump;
            break;
          case B8(0011) << 8:     // JEQ Jump equal
            if(_st.Zero)
              goto Jump;
            break;
          case B8(0110) << 8:     // JNE Jump not equal
            if(!_st.Zero)
              goto Jump;
            break;
          case B8(1000) << 8:     // JOC Jump carry
            if(_st.Carry)
              goto Jump;
            break;
          case B8(0111) << 8:     // JNC Jump no carry
            if(!_st.Carry)
              goto Jump;
            break;
          case B8(1001) << 8:     // JNO Jump no overflow
            if(!_st.Overflow)
              goto Jump;
            break;
          case B8(1100) << 8:     // JOP Jump odd parity
            if(_st.Parity)
              goto Jump;
            break;
          case B8(0000) << 8:     // JMP Jump unconditional  (se 0x1000 vale come NOP !
Jump:
    				_pc += (int8_t)LOBYTE(Pipe1) *2;
            break;

               // SBO SBZ TB (CRU operations)
          case B8(1101) << 8:     // SBO Set bit to one
		        res3.x=GetValueCRU(GET_REG(12)+LOBYTE(Pipe1)/8,1);
		        PutValueCRU(GET_REG(12)+LOBYTE(Pipe1)/8,res3.x,1);
            break;
          case B8(1110) << 8:     // SBZ Set bit to zero
		        res3.x=GetValueCRU(GET_REG(12)+LOBYTE(Pipe1)/8,1);
		        PutValueCRU(GET_REG(12)+LOBYTE(Pipe1)/8,res3.x,1);
            break;
          case B8(1111) << 8:     // TB Test bit 
		        res3.x=GetValueCRU(GET_REG(12)+LOBYTE(Pipe1)/8,1);
						if(res3.x)
							_st.Zero=1;			// occhio invertito
						else
							_st.Zero=0;
            break;
          }
        break;
        
      case B8(1010) << 12:    // A Add
        switch(workingTS) {
          case REGISTER_DIRECT:
            res2.x=GET_WORKING_REG();
            break;
          case REGISTER_INDIRECT:
            res2.x=GetIntValue(GET_WORKING_REG());
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res2.x=GetIntValue(GET_WORKING_REG()+(int16_t)Pipe2.x);
            else
              res2.x=GetIntValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res2.x=GetIntValue(GET_WORKING_REG());
            SET_WORKING_REG(GET_WORKING_REG()+2);
            break;
          }
        switch(workingTD) {
          case REGISTER_DIRECT:
            res1.x=GET_WORKING_REG2();
            break;
          case REGISTER_INDIRECT:
            res1.x=GetIntValue(GET_WORKING_REG2());
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              res1.x=GetIntValue(GET_WORKING_REG2()+(int16_t)Pipe2.x);
            else
              res1.x=GetIntValue(Pipe2.x);
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.x=GetIntValue(GET_WORKING_REG2());
//            SET_WORKING_REG2(GET_WORKING_REG2()+2);
            break;
          }
        res3.x=res1.x+res2.x;
        
        switch(workingTD) {
          case REGISTER_DIRECT:
            SET_WORKING_REG2(res3.x);
            break;
          case REGISTER_INDIRECT:
            PutIntValue(GET_WORKING_REG2(),res3.x);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              PutIntValue(GET_WORKING_REG2()+(int16_t)Pipe2.x,res3.x);
            else
              PutIntValue(Pipe2.x,res3.x);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            PutIntValue(GET_WORKING_REG2(),res3.x);
            SET_WORKING_REG2(GET_WORKING_REG2()+2);
            break;
          }
        goto aggFlag16A;
        break;
      case B8(1011) << 12:    // AB Add bytes
        switch(workingTS) {
          case REGISTER_DIRECT:
            res2.b.l=HIBYTE(GET_WORKING_REG());
            break;
          case REGISTER_INDIRECT:
            res2.b.l=GetValue(GET_WORKING_REG());
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res2.b.l=GetValue(GET_WORKING_REG()+(int16_t)Pipe2.x);
            else
              res2.b.l=GetValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res2.b.l=GetValue(GET_WORKING_REG());
            SET_WORKING_REG(GET_WORKING_REG()+1);
            break;
          }
        switch(workingTD) {
          case REGISTER_DIRECT:
            res1.b.l=HIBYTE(GET_WORKING_REG2());
            break;
          case REGISTER_INDIRECT:
            res1.b.l=GetValue(GET_WORKING_REG2());
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              res1.b.l=GetValue(GET_WORKING_REG2()+(int16_t)Pipe2.x);
            else
              res1.b.l=GetValue(Pipe2.x);
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.b.l=GetValue(GET_WORKING_REG2());
//            SET_WORKING_REG2(GET_WORKING_REG2()+1);
            break;
          }
        res3.b.l=res1.b.l+res2.b.l;
        
//        _st.Overflow = !!(((res1.b.l & 0x40) + (res2.b.l & 0x40)) & 0x80) != !!(((res1.x & 0x80) + (res2.x & 0x80)) & 0x100);
//        _st.Overflow = !!(((res1.b.h & 0x80) == (res2.b.h & 0x80)) && ((res3.b.h & 0x80) != (res2.b.h & 0x80)));
        _st.Overflow = OVF_ADD_8();
        _st.Carry=CARRY_ADD_8();

store8_D:
        switch(workingTD) {
          case REGISTER_DIRECT:
            SET_WORKING_REG2(MAKEWORD(LOBYTE(GET_WORKING_REG2()),res3.b.l));		// se registro, va in MSB
            break;
          case REGISTER_INDIRECT:
            PutValue(GET_WORKING_REG2(),res3.b.l);			// v. di là, big-endian circa
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              PutValue(GET_WORKING_REG2()+(int16_t)Pipe2.x,res3.b.l);
            else
              PutValue(Pipe2.x,res3.b.l);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            PutValue(GET_WORKING_REG2(),res3.b.l);
            SET_WORKING_REG2(GET_WORKING_REG2()+1);
            break;
          }
      
aggFlag8Z:
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
        _st.Parity=par & 1 ? 1 : 0;   // ODD
        }
        break;

      case B8(1000) << 12:    // C Compare
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.x=GET_WORKING_REG();
            break;
          case REGISTER_INDIRECT:
            res1.x=GetIntValue(GET_WORKING_REG());
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.x=GetIntValue(GET_WORKING_REG()+(int16_t)Pipe2.x);
            else
              res1.x=GetIntValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.x=GetIntValue(GET_WORKING_REG());
            SET_WORKING_REG(GET_WORKING_REG()+2);
            break;
          }
        switch(workingTD) {
          case REGISTER_DIRECT:
            res2.x=GET_WORKING_REG2();
            break;
          case REGISTER_INDIRECT:
            res2.x=GetIntValue(GET_WORKING_REG2());
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              res2.x=GetIntValue(GET_WORKING_REG2()+(int16_t)Pipe2.x);
            else
              res2.x=GetIntValue(Pipe2.x);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res2.x=GetIntValue(GET_WORKING_REG2());
            SET_WORKING_REG2(GET_WORKING_REG2()+2);
            break;
          }
        goto compare16;
        break;
      case B8(1001) << 12:    // CB Compare bytes
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.b.l=HIBYTE(GET_WORKING_REG());
            break;
          case REGISTER_INDIRECT:
            res1.b.l=GetValue(GET_WORKING_REG());
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.b.l=GetValue(GET_WORKING_REG()+(int16_t)Pipe2.x);
            else
              res1.b.l=GetValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.b.l=GetValue(GET_WORKING_REG());
            SET_WORKING_REG(GET_WORKING_REG()+1);
            break;
          }
        switch(workingTD) {
          case REGISTER_DIRECT:
            res2.b.l=HIBYTE(GET_WORKING_REG2());
            break;
          case REGISTER_INDIRECT:
            res2.b.l=GetValue(GET_WORKING_REG2());
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              res2.b.l=GetValue(GET_WORKING_REG2()+(int16_t)Pipe2.x);
            else
              res2.b.l=GetValue(Pipe2.x);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res2.b.l=GetValue(GET_WORKING_REG2());
            SET_WORKING_REG2(GET_WORKING_REG2()+1);
            break;
          }

				res3.b.l=res1.b.l;
        _st.LogicalGreater=res1.b.l>res2.b.l ? 1 : 0;
        _st.ArithmeticGreater=((int8_t)res1.b.l)>((int8_t)res2.b.l) ? 1 : 0;
        _st.Zero=res1.b.l == res2.b.l ? 1 : 0;
				goto calcParity;
        break;

      case B8(0110) << 12:    // S Subtract
        switch(workingTS) {
          case REGISTER_DIRECT:
            res2.x=GET_WORKING_REG();
            break;
          case REGISTER_INDIRECT:
            res2.x=GetIntValue(GET_WORKING_REG());
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res2.x=GetIntValue(GET_WORKING_REG()+(int16_t)Pipe2.x);
            else
              res2.x=GetIntValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res2.x=GetIntValue(GET_WORKING_REG());
            SET_WORKING_REG(GET_WORKING_REG()+2);
            break;
          }
        switch(workingTD) {
          case REGISTER_DIRECT:
            res1.x=GET_WORKING_REG2();
            break;
          case REGISTER_INDIRECT:
            res1.x=GetIntValue(GET_WORKING_REG2());
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              res1.x=GetIntValue(GET_WORKING_REG2()+(int16_t)Pipe2.x);
            else
              res1.x=GetIntValue(Pipe2.x);
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.x=GetIntValue(GET_WORKING_REG2());
//            SET_WORKING_REG2(GET_WORKING_REG2()+2);
            break;
          }
        res3.x=res1.x-res2.x;
        
        _st.Carry=CARRY_SUB_16();
//        _st.Overflow = !!(((res1.x & 0x4000) + (res2.x & 0x4000)) & 0x8000) != !!(((res1.d & 0x8000) + (res2.d & 0x8000)) & 0x10000);
//        _st.Overflow = !!(((res1.x & 0x8000) != (res2.x & 0x8000)) && ((res3.x & 0x8000) != (res2.x & 0x8000)));
        _st.Overflow = OVF_SUB_16();
        switch(workingTD) {
          case REGISTER_DIRECT:
            SET_WORKING_REG2(res3.x);
            break;
          case REGISTER_INDIRECT:
            PutIntValue(GET_WORKING_REG2(),res3.x);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              PutIntValue(GET_WORKING_REG2()+(int16_t)Pipe2.x,res3.x);
            else
              PutIntValue(Pipe2.x,res3.x);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            PutIntValue(GET_WORKING_REG2(),res3.x);
            SET_WORKING_REG2(GET_WORKING_REG2()+2);
            break;
          }
        goto aggFlag16Z;
        break;
      case B8(0111) << 12:    // SB Subtract bytes  OPERANDI INVERTITI ;) anche in Add, mentre Compare è dritta!
        switch(workingTS) {
          case REGISTER_DIRECT:
            res2.b.l=HIBYTE(GET_WORKING_REG());
            break;
          case REGISTER_INDIRECT:
            res2.b.l=GetValue(GET_WORKING_REG());
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res2.b.l=GetValue(GET_WORKING_REG()+(int16_t)Pipe2.x);
            else
              res2.b.l=GetValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res2.b.l=GetValue(GET_WORKING_REG());
            SET_WORKING_REG(GET_WORKING_REG()+1);
            break;
          }
        switch(workingTD) {
          case REGISTER_DIRECT:
            res1.b.l=HIBYTE(GET_WORKING_REG2());
            break;
          case REGISTER_INDIRECT:
            res1.b.l=GetValue(GET_WORKING_REG2());
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              res1.b.l=GetValue(GET_WORKING_REG2()+(int16_t)Pipe2.x);
            else
              res1.b.l=GetValue(Pipe2.x);
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.b.l=GetValue(GET_WORKING_REG2());
//            SET_WORKING_REG2(GET_WORKING_REG2()+1);
            break;
          }
        res3.b.l=res1.b.l-res2.b.l;
        _st.Carry=CARRY_SUB_8();
//        _st.Overflow = !!(((res1.b.h & 0x80) != (res2.b.h & 0x80)) && ((res3.b.h & 0x80) != (res2.b.h & 0x80)));
        _st.Overflow = OVF_SUB_8();
        goto store8_D;
        break;
      
      case B8(1110) << 12:    // SOC Set ones corresponding
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.x=GET_WORKING_REG();
            break;
          case REGISTER_INDIRECT:
            res1.x=GetIntValue(GET_WORKING_REG());
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.x=GetIntValue(GET_WORKING_REG()+(int16_t)Pipe2.x);
            else
              res1.x=GetIntValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.x=GetIntValue(GET_WORKING_REG());
            SET_WORKING_REG(GET_WORKING_REG()+2);
            break;
          }
        switch(workingTD) {
          case REGISTER_DIRECT:
            res2.x=GET_WORKING_REG2();
            break;
          case REGISTER_INDIRECT:
            res2.x=GetIntValue(GET_WORKING_REG2());
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              res2.x=GetIntValue(GET_WORKING_REG2()+(int16_t)Pipe2.x);
            else
              res2.x=GetIntValue(Pipe2.x);
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res2.x=GetIntValue(GET_WORKING_REG2());
//            SET_WORKING_REG2(GET_WORKING_REG2()+2);			// qua??
            break;
          }
        res3.x=res2.x | res1.x;
        
store16_D:
        switch(workingTD) {
          case REGISTER_DIRECT:
            SET_WORKING_REG2(res3.x);
            break;
          case REGISTER_INDIRECT:
            PutIntValue(GET_WORKING_REG2(),res3.x);
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              PutIntValue(GET_WORKING_REG2()+(int16_t)Pipe2.x,res3.x);
            else
              PutIntValue(Pipe2.x,res3.x);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            PutIntValue(GET_WORKING_REG2(),res3.x);
            SET_WORKING_REG2(GET_WORKING_REG2()+2);
            break;
          }
        goto aggFlag16Z;
        break;
      case B8(1111) << 12:    // SOCB Set ones corresponding bytes  
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.b.l=HIBYTE(GET_WORKING_REG());
            break;
          case REGISTER_INDIRECT:
            res1.b.l=GetValue(GET_WORKING_REG());
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.b.l=GetValue(GET_WORKING_REG()+(int16_t)Pipe2.x);
            else
              res1.b.l=GetValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.b.l=GetValue(GET_WORKING_REG());
            SET_WORKING_REG(GET_WORKING_REG()+1);
            break;
          }
        switch(workingTD) {
          case REGISTER_DIRECT:
            res2.b.l=HIBYTE(GET_WORKING_REG2());
            break;
          case REGISTER_INDIRECT:
            res2.b.l=GetValue(GET_WORKING_REG2());
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              res2.b.l=GetValue(GET_WORKING_REG2()+(int16_t)Pipe2.x);
            else
              res2.b.l=GetValue(Pipe2.x);
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res2.b.l=GetValue(GET_WORKING_REG2());
//            SET_WORKING_REG2(GET_WORKING_REG2()+1);		// qua??
            break;
          }
        res3.b.l=res2.b.l | res1.b.l;
        goto store8_D;
        break;
      
      case B8(0100) << 12:    // SZC Set zeros corresponding
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.x=GET_WORKING_REG();
            break;
          case REGISTER_INDIRECT:
            res1.x=GetIntValue(GET_WORKING_REG());
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.x=GetIntValue(GET_WORKING_REG()+(int16_t)Pipe2.x);
            else
              res1.x=GetIntValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.x=GetIntValue(GET_WORKING_REG());
            SET_WORKING_REG(GET_WORKING_REG()+2);
            break;
          }
        switch(workingTD) {
          case REGISTER_DIRECT:
            res2.x=GET_WORKING_REG2();
            break;
          case REGISTER_INDIRECT:
            res2.x=GetIntValue(GET_WORKING_REG2());
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              res2.x=GetIntValue(GET_WORKING_REG2()+(int16_t)Pipe2.x);
            else
              res2.x=GetIntValue(Pipe2.x);
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res2.x=GetIntValue(GET_WORKING_REG2());
//            SET_WORKING_REG2(GET_WORKING_REG2()+2);		// qua??
            break;
          }
        res3.x=res2.x & ~res1.x;
        goto store16_D;
        break;
      case B8(0101) << 12:    // SZCB Set zeros corresponding byte
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.b.l=HIBYTE(GET_WORKING_REG());
            break;
          case REGISTER_INDIRECT:
            res1.b.l=GetValue(GET_WORKING_REG());
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.b.l=GetValue(GET_WORKING_REG()+(int16_t)Pipe2.x);
            else
              res1.b.l=GetValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.b.l=GetValue(GET_WORKING_REG());
            SET_WORKING_REG(GET_WORKING_REG()+1);
            break;
          }
        switch(workingTD) {
          case REGISTER_DIRECT:
            res2.b.l=HIBYTE(GET_WORKING_REG2());
            break;
          case REGISTER_INDIRECT:
            res2.b.l=GetValue(GET_WORKING_REG2());
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingReg2Index)
              res2.b.l=GetValue(GET_WORKING_REG2()+(int16_t)Pipe2.x);
            else
              res2.b.l=GetValue(Pipe2.x);
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res2.b.l=GetValue(GET_WORKING_REG2());
//            SET_WORKING_REG2(GET_WORKING_REG2()+1);		// qua??
            break;
          }
        res3.b.l=res2.b.l & ~res1.b.l;
        goto store8_D;
        break;
      
      case B8(1100) << 12:    // MOV Move
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.x=GET_WORKING_REG();
            break;
          case REGISTER_INDIRECT:
            res1.x=GetIntValue(GET_WORKING_REG());
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.x=GetIntValue(GET_WORKING_REG()+(int16_t)Pipe2.x);
            else
              res1.x=GetIntValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.x=GetIntValue(GET_WORKING_REG());
            SET_WORKING_REG(GET_WORKING_REG()+2);
            break;
          }
        res3.x=res1.x;
        goto store16_D;
        break;
      case B8(1101) << 12:    // MOVB Move bytes  
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.b.l=HIBYTE(GET_WORKING_REG());			// se registro, uso MSB
            break;
          case REGISTER_INDIRECT:
            res1.b.l=GetValue(GET_WORKING_REG());		// v. di là, big-endian
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.b.l=GetValue(GET_WORKING_REG()+(int16_t)Pipe2.x);
            else
              res1.b.l=GetValue(Pipe2.x);
            GetPipe(_pc);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.b.l=GetValue(GET_WORKING_REG());
            SET_WORKING_REG(GET_WORKING_REG()+1);
            break;
          }
        res3.b.l=res1.b.l;
        goto store8_D;
        break;
      
      case B8(0010) << 12:    // Compare Ones, Compare Zeros, Exclusive OR
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.x=GET_WORKING_REG();
            break;
          case REGISTER_INDIRECT:
            res1.x=GetIntValue(GET_WORKING_REG());
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.x=GetIntValue(GET_WORKING_REG()+(int16_t)Pipe2.x);
            else
              res1.x=GetIntValue(Pipe2.x);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.x=GetIntValue(GET_WORKING_REG());
            SET_WORKING_REG(GET_WORKING_REG()+2);
            break;
          }
    		switch(Pipe1 & B16(11111100,00000000)) {
          case B8(001000) << 10:     // COC Compare Ones corresponding
            if((res1.x & GET_WORKING_REG2()) == res1.x)
              _st.Zero=1;
						else
              _st.Zero=0;
            break;
          case B8(001001) << 10:     // CZC Compare Zeros corresponding
            if((res1.x & GET_WORKING_REG2()) == 0 /*res1.x*/)
              _st.Zero=1;
						else
              _st.Zero=0;
            break;
          case B8(001010) << 10:     // XOR Exclusive OR
            res2.x=GET_WORKING_REG2();
            res3.x = res1.x ^ res2.x;
            SET_WORKING_REG2(res3.x);
            goto aggFlag16Z;
            break;
          case B8(001011) << 10:     // XOP Extended Operation
#ifdef TMS9940 
        		switch(Pipe1 & B16(00000011,11000000)) {
              case B8(0000) << 6:   // DCA      verificare!!
                res3.b.l=res1.b.l;
                i=_st.Carry;
                _st.Carry=0;
                if((res1.b.l & 0xf) > 9 || _st.DigitCarry) {
                  res3.x+=6;
                  res1.b.l=res3.b.l;
                  _st.Carry= i || res3.b.h;
                  _st.DigitCarry=1;
                  }
                else
                  _st.DigitCarry=0;
                if((res1.b.l>0x99) || i) {
                  res3.b.l+=0x60;  
                  _st.Carry=1;
                  }
                else
                  _st.Carry=0;
                switch(workingTD) {
                  case REGISTER_DIRECT:
                    SET_WORKING_REG2(MAKEWORD(res3.b.l,HIBYTE(GET_WORKING_REG2())));


                    break;
                  case REGISTER_INDIRECT:
                    PutValue(GET_WORKING_REG2(),res3.b.l);
                    break;
                  case REGISTER_SYMBOLIC_INDEXED:
                    if(workingReg2Index)
                      PutValue(GET_WORKING_REG2()+(int16_t)Pipe2.x,res3.b.l);
                    else
                      PutValue(Pipe2.x,res3.b.l);
                    _pc+=2;
                    break;
                  case REGISTER_INDIRECT_AUTOINCREMENT:
                    PutValue(GET_WORKING_REG2(),res3.b.l);
                    SET_WORKING_REG2(GET_WORKING_REG2()+2);
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
                if((res1.b.l & 0xf) > 9 || _st.DigitCarry) {
                  res3.x+=6;
                  res1.b.l=res3.b.l;
                  _st.Carry= i || res3.b.h;
                  _st.DigitCarry=1;
                  }
                else
                  _st.DigitCarry=0;
                if((res1.b.l>0x99) || i) {
                  res3.b.l+=0x60;  
                  _st.Carry=1;
                  }
                else
                  _st.Carry=0;
                goto store_dca;
                break;
              case B8(0010) << 6:   // LIIM
                _st.InterruptMask=(_st.InterruptMask & B8(11111100)) | (Pipe2.x & B8(00000011));
                break;
              default:   // XOP
                i=_wp;
                _wp=GetIntValue(0x0040+GET_REG((Pipe1 & B16(11,11000000)) >> 6)*4);
						    regs=(union T_REGISTERS *)&ram_seg[_wp & 0xff /* -RAM_START */];     // così oppure cast diretto...
                SET_REG(11,res3.x);
                SET_REG(13,i);
                SET_REG(14,_pc);
                SET_REG(15,_st.x);
                _st.XOP=1;
                _pc=GetIntValue(0x0042+GET_REG((Pipe1 & B16(11,11000000)) >> 6)*4);
                break;
              }
            
#else
            i=_wp;
         		_wp=GetIntValue(0x0040+GET_REG((Pipe1 & B16(11,11000000)) >> 6)*4);
				    regs=(union T_REGISTERS *)&ram_seg[_wp & 0xff /* -RAM_START */];     // così oppure cast diretto...
            SET_REG(11,res3.x);
            SET_REG(13,i);
            SET_REG(14,_pc);
            SET_REG(15,_st.x);
            _st.XOP=1;
           	_pc=GetIntValue(0x0042+GET_REG((Pipe1 & B16(11,11000000)) >> 6)*4);
            break;
#endif
          }
        break;
      
      case B8(0011) << 12:    // Multiply, Divide, CRU
        switch(workingTS) {
          case REGISTER_DIRECT:
            res1.x=GET_WORKING_REG();
            break;
          case REGISTER_INDIRECT:
            res1.x=GetIntValue(GET_WORKING_REG());
            break;
          case REGISTER_SYMBOLIC_INDEXED:
            if(workingRegIndex)
              res1.x=GetIntValue(GET_WORKING_REG()+(int16_t)Pipe2.x);
            else
              res1.x=GetIntValue(Pipe2.x);
            _pc+=2;
            break;
          case REGISTER_INDIRECT_AUTOINCREMENT:
            res1.x=GetIntValue(GET_WORKING_REG());
            SET_WORKING_REG(GET_WORKING_REG()+2);
            break;
          }
    		switch(Pipe1 & B16(11111100,00000000)) {
          case B8(001110) << 10:     // MPY Multiply
						res2.x=GET_WORKING_REG2();
            res3.d = res1.x * res2.x;
            SET_WORKING_REG2(HIWORD(res3.d));
            SET_REG(((WORKING_REG2_INDEX+1) /*& 0xf*/),res3.x);   // OKKIO, porcata, & se 15... dice che deve andare in memoria subito dopo! tipo R16
//no!            goto aggFlag;
            break;
          case B8(001111) << 10:     // DIV Divide
            res2.d = MAKELONG(GET_REG((WORKING_REG2_INDEX+1) /*& 0xf*/),GET_WORKING_REG2());    // OKKIO...
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
            if(res2.d < res1.x)
              _st.Overflow = 1;
            else {
	            res3.d = res2.d / (uint32_t)res1.x;		// signed o unsigned??
              SET_WORKING_REG2(LOWORD(res3.d));
              SET_REG((WORKING_REG2_INDEX+1) /*& 0xf*/,res2.d % (uint32_t)res1.x);   // OKKIO, porcata, & se 15... dice che deve andare in memoria subito dopo! tipo R16
              _st.Overflow = 0;
              }
            break;
            
          case B8(001100) << 10:     // LDCR Load communication register
						{uint8_t cnt=WORKING_REG2_INDEX;
            PutValueCRU(GET_REG(12),res1.x,cnt);
						res3.x=res1.x;
						if(cnt>8)
							goto aggFlag16Z;
						else
							goto aggFlag8Z;
						}
            break;
          case B8(001101) << 10:     // STCR Store communication register
						{uint8_t cnt=WORKING_REG2_INDEX;
            res3.x=GetValueCRU(GET_REG(12),cnt);
						if(cnt>8) {
							goto store16_S;
							}
						else {
							res3.b.l=res3.b.h;
							switch(workingTS) {
								case REGISTER_DIRECT:
									SET_WORKING_REG(MAKEWORD(LOBYTE(GET_WORKING_REG()),res3.b.l));		// se registro, va in MSB
									break;
								case REGISTER_INDIRECT:
									PutValue(GET_WORKING_REG(),res3.b.l);			// v. di là, big-endian circa
									break;
								case REGISTER_SYMBOLIC_INDEXED:
									if(workingRegIndex)
										PutValue(GET_WORKING_REG()+(int16_t)Pipe2.x,res3.b.l);
									else
										PutValue(Pipe2.x,res3.b.l);
									_pc+=2;
									break;
								case REGISTER_INDIRECT_AUTOINCREMENT:
									PutValue(GET_WORKING_REG(),res3.b.l);
									SET_WORKING_REG(GET_WORKING_REG()+1);
									break;
								}
							goto aggFlag8Z;
							}
						}
            break;
          }
        break;

			}

rallenta:
			;
		} while(!fExit);
	}


