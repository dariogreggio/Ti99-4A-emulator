#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <conio.h>
#include "tms9900win.h"



HFILE spoolFile;

char commento[256];
BYTE Opzioni,CPUType;

#if 0

static const char *getRegister(BYTE reg) {
	const char *r;

	switch(reg & 7) {
		case 0:
			r="A";
			break;
		case 1:
			r="X";
			break;
		case 2:
			r="Y";
			break;
		case 3:
			r="S";
			break;
		case 4:
			r="P";
			break;
		}

	return r;
	}

static const char *getAddressing(BYTE m,WORD n) {
	const char *r;
	static    char myBuf[32];		// migliorare
	char myBuf2[32];	

  switch(m) {
    case 0:			// ZP
			wsprintf(myBuf,"$%02X",(BYTE)n);
	    break;
    case 1:			// ZP,X
			wsprintf(myBuf,"$%02X,X",(BYTE)n);
      break;
    case 2:			// ZP,Y
			wsprintf(myBuf,"$%02X,Y",(BYTE)n);
      break;
    case 4:			// ABS
			wsprintf(myBuf,"$%04X",(WORD)n);
      break;
    case 5:			// ABS,X
			wsprintf(myBuf,"$%04X,X",(WORD)n);
      break;
    case 6:			// ABS,Y
			wsprintf(myBuf,"$%04X,Y",(WORD)n);
      break;
    case 8:			// INDIRECT
			wsprintf(myBuf,"($%04X)",(WORD)n);
      break;
    case 9:			// X,IND
			wsprintf(myBuf,"($%02X,X)",(BYTE)n);
      break;
    case 10:		// IND,Y
			wsprintf(myBuf,"($%02X),Y",(BYTE)n);
      break;
    }

	if(Opzioni & 4) {
		*myBuf2=0;		// safety!
	  switch(m) {
		  case 0:			// ZP
			case 1:			// ZP,X
			case 2:			// ZP,Y
			case 9:			// X,IND
			case 10:		// IND,Y
				wsprintf(myBuf2,"%u",(BYTE)n);
	      break;
			case 4:			// ABS
			case 5:			// ABS,X
			case 6:			// ABS,Y
			case 8:			// INDIRECT
				wsprintf(myBuf2,"%u",n);
	      break;
	    }
		if(*commento)
			strcat(commento," ");
		strcat(commento,myBuf2);
		}

	r=myBuf;

	return r;
	}

static char *outputText(char *d,const char *s) {

	strcat(d,s);
	return d;
	}
	
static char *outputChar(char *d,char s) {
	WORD i=strlen(d);

	d[i++]=s;
	d[i]=0;
	return d;
	}
#define outputComma(d) outputChar(d,',')

static char *outputByte(char *d,BYTE n) {
	char myBuf[64];

	wsprintf(myBuf,"%02Xh",n);
	strcat(d,myBuf);
	if(Opzioni & 4) {
		wsprintf(myBuf,"%d",n);
		if(*commento)
			strcat(commento," ");
		strcat(commento,myBuf);
		}

	return d;
	}

static char *outputWord(char *d,WORD n) {
	char myBuf[64];

	wsprintf(myBuf,"%04Xh",n);
	strcat(d,myBuf);
	if(Opzioni & 4) {
		wsprintf(myBuf,"%d",n);
		if(*commento)
			strcat(commento," ");
		strcat(commento,myBuf);
		}
	return d;
	}

static char *outputAddress(char *d,WORD n) {
	char myBuf[64];

	wsprintf(myBuf,"%04Xh",n);
	strcat(d,myBuf);
	return d;
	}

static char *outputUnknown(char *d,BYTE n) {

	outputText(d,"DB ");
	outputByte(d,n);
	return d;
	}
#endif

int Disassemble(const BYTE *src,HFILE f,char *dest,DWORD len,WORD pcaddr,BYTE opzioni) {
	// opzioni: b0 addbytes, b1 addPC, b2 numeri dec in commenti
	const BYTE *oldsrc;
	const char *r,*r2;
	BYTE Pipe1;
	union PIPE Pipe2;
  register uint16_t i;
	BYTE fExit=0;


	Opzioni=opzioni;
	CPUType=0;

	*dest=0;


#if 0
	do {

		oldsrc=src;

		*commento=0;

// https://www.masswerk.at/6502/6502_instruction_set.html
		Pipe1=*src++;
		Pipe2.b[0]=*src; Pipe2.b[1]=*(src+1);		// 


		switch(Pipe1) {
			case 0:		// BRK
				outputText(dest,"BRK");
				break;

			case 1:
				outputText(dest,"ORA ");
				r=getAddressing(9,Pipe2.bytes.byte1);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src++;
				break;

			case 2:     // halt and catch fire :D opp. SINC
#ifdef CPU_KIMKLONE
        _IP.w++;
#else
				outputText(dest,"HCF");
#endif
				break;
        
#ifdef CPU_KIMKLONE
			case 0x3:    // NOP implied 1 cycle
				break;
#endif

#ifdef CPU_65C02
			case 0x04:    // TSB
				outputText(dest,"TSB ");
				res3.b.l= _a & ram_seg[Pipe2.bytes.byte1];		// 
				ram_seg[Pipe2.bytes.byte1] |= _a;
				src++;
#endif

			case 5:
				outputText(dest,"ORA ");
				r=getAddressing(0,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

			case 6:
				outputText(dest,"ASL ");
				r=getAddressing(0,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

#ifdef CPU_65C02		// rockwell
			case 7:
				outputText(dest,"RES ");
				ram_seg[Pipe2.bytes.byte1] &= ~0x1;
				src++;
				break;
#endif

			case 8:
				outputText(dest,"PHP");
				break;

			case 9:
				outputText(dest,"ORA #");
				outputByte(dest,Pipe2.bytes.byte1);
				src++;
				break;

			case 0xa:
				outputText(dest,"ASL A");
				break;

#ifdef CPU_KIMKLONE
			case 0x0b:    // LDK2
				outputText(dest,"LDK2 ");
        _K[2]=GetValue(Pipe2.word);
        theK=&_K[0];
				src+=2;
				break;
#endif

#ifdef CPU_65C02
			case 0x0c:    // TSB
				outputText(dest,"TSB ");
				res3.b.l= _a & GetValue(Pipe2.word);		// 
				PutValue(Pipe2.word,GetValue(Pipe2.word) | _a);
				src+=2;
				goto aggTrb;
#endif

			case 0xd:
				outputText(dest,"ORA ");
				r=getAddressing(4,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

			case 0xe:
				outputText(dest,"ASL ");
				r=getAddressing(4,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

#ifdef CPU_65C02		// rockwell
			case 0x0f:
				outputText(dest,"B ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src=2;
				break;
#endif

			case 0x10:
				outputText(dest,"BPL ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src++;
				break;

			case 0x11:
				outputText(dest,"ORA ");
				r=getAddressing(10,Pipe2.bytes.byte1);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src++;
				break;

#ifdef CPU_65C02
			case 0x12:
				outputText(dest,"ORA ");
				_a |= GetValue(MAKEWORD(ram_seg[Pipe2.bytes.byte1],ram_seg[Pipe2.bytes.byte1+1]));
				src++;
				break;

			case 0x14:    // TRB
				outputText(dest,"TRB ");
				res3.b.l= _a & ram_seg[Pipe2.bytes.byte1];		// 
				ram_seg[Pipe2.bytes.byte1] &= ~_a;
				src++;
aggTrb:
				_p.Zero=!res3.b.l;
				break;
#endif

#ifdef CPU_KIMKLONE    //https://laughtonelectronics.com/Arcana/KimKlone/KK%20Instruction%20List.html
			case 0x13:    // JMP_K3
				outputText(dest,"JMP_K3");
        res3.b.l=_K[0];
        _K[0]=_K[3];
        _K[3]=res3.b.l;
				src=Pipe2.word;
				break;
#endif

			case 0x15:
				outputText(dest,"ORA ");
				r=getAddressing(1,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

			case 0x16:
				outputText(dest,"ASL ");
				r=getAddressing(1,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

#ifdef CPU_65C02		// rockwell
			case 0x17:
				outputText(dest,"RES ");
				ram_seg[Pipe2.bytes.byte1] &= ~0x2;
				src++;
				break;
#endif

			case 0x18:
				outputText(dest,"CLC");
				break;

			case 0x19:
				outputText(dest,"ORA ");
				r=getAddressing(6,Pipe2.bytes.byte1);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

#ifdef CPU_65C02
			case 0x1a:
				outputText(dest,"INCA");
				_a++;
				break;

			case 0x1c:    // TRB
				outputText(dest,"TRB ");
				res3.b.l= _a & GetValue(Pipe2.word);		// 
				PutValue(Pipe2.word,GetValue(Pipe2.word) & ~_a);
				src++;
				goto aggTrb;
				break;
#endif

#ifdef CPU_KIMKLONE
			case 0x1b:    // PLYIPL
				_IP.b.l=_y=stack_seg[++_s];
				goto aggFlagY;
				break;
#endif

			case 0x1d:
				outputText(dest,"ORA ");
				r=getAddressing(5,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

			case 0x1e:
				outputText(dest,"ASL ");
				r=getAddressing(5,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

#ifdef CPU_65C02		// rockwell
			case 0x1f:
				outputText(dest,"B ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src=2;
				break;
#endif

			case 0x20:
				outputText(dest,"JSR ");
				outputAddress(dest,Pipe2.word);
				src+=2;
				break;

			case 0x21:
				outputText(dest,"AND ");
				r=getAddressing(9,Pipe2.bytes.byte1);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
    		src++;
				break;

#ifdef CPU_KIMKLONE    //https://laughtonelectronics.com/Arcana/KimKlone/KK%20Instruction%20List.html
      case 0x22:    // DINC
				outputText(dest,"DINC ");
        _IP.w+=2;
        break;
        
			case 0x23:    // JSR_K3
				outputText(dest,"JSR_K3");
				stack_seg[_s--]=HIBYTE(++src);      // v. RTS...
				stack_seg[_s--]=LOBYTE(src);
        res3.b.l=_K[0];
        _K[0]=_K[3];
        _K[3]=res3.b.l;
				src=Pipe2.word;
				break;
#endif

			case 0x24:    // BIT zp
				outputText(dest,"BIT ");
				r=getAddressing(0,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

			case 0x25:
				outputText(dest,"AND ");
				r=getAddressing(0,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

			case 0x26:
				outputText(dest,"ROL ");
				r=getAddressing(0,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

#ifdef CPU_65C02		// rockwell
			case 0x27:
				outputText(dest,"RES ");
				ram_seg[Pipe2.bytes.byte1] &= ~0x4;
				src++;
				break;
#endif

			case 0x28:
				outputText(dest,"PLP");
				break;

			case 0x29:
				outputText(dest,"AND #");
				outputByte(dest,Pipe2.bytes.byte1);
				src++;
				break;

			case 0x2a:
				outputText(dest,"ROL");
				break;

#ifdef CPU_KIMKLONE
			case 0x2b:    // PLYIPH
				_IP.b.h=_y=stack_seg[++_s];
				goto aggFlagY;
				break;
#endif

			case 0x2c:    // BIT abs
				outputText(dest,"BIT ");
				r=getAddressing(4,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

			case 0x2d:
				outputText(dest,"AND ");
				r=getAddressing(4,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

			case 0x2e:
				outputText(dest,"ROL ");
				r=getAddressing(4,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

#ifdef CPU_65C02		// rockwell
			case 0x2f:
				outputText(dest,"B ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src=2;
				break;
#endif

			case 0x30:
				outputText(dest,"BMI ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src++;
				break;

			case 0x31:
				outputText(dest,"AND ");
				r=getAddressing(10,Pipe2.bytes.byte1);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src++;
				break;

#ifdef CPU_65C02
			case 0x32:
				outputText(dest,"AND ");
				_a &= GetValue(MAKEWORD(ram_seg[Pipe2.bytes.byte1],ram_seg[Pipe2.bytes.byte1+1]));
				src++;
				break;

			case 0x34:
				res3.b.l = ram_seg[Pipe2.bytes.byte1+_x];
				src++;
				break;
#endif

#ifdef CPU_KIMKLONE
			case 0x33:    // SCAN_K3
// simula una JSR e genera pixel-output...
        src+=2;
				break;
#endif

			case 0x35:
				outputText(dest,"AND ");
				r=getAddressing(1,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

			case 0x36:
				outputText(dest,"ROL ");
				r=getAddressing(1,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

#ifdef CPU_65C02		// rockwell
			case 0x37:
				outputText(dest,"RES ");
				ram_seg[Pipe2.bytes.byte1] &= ~0x8;
				src++;
				break;
#endif

			case 0x38:
				outputText(dest,"SEC");
				break;

			case 0x39:
				outputText(dest,"AND ");
				r=getAddressing(6,Pipe2.bytes.byte1);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

#ifdef CPU_65C02
			case 0x3a:
				outputText(dest,"DECA ");
				_a--;
				break;

			case 0x3c:
				outputText(dest,"");
				res3.b.l = GetValue(Pipe2.word+_x);
				src+=2;
				goto aggBit;
				break;
#endif
        
#ifdef CPU_KIMKLONE
			case 0x3b:    // NEXT
				src=_IP.w;
        _IP.w+=2;
				break;
#endif

			case 0x3d:
				outputText(dest,"AND ");
				r=getAddressing(5,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

			case 0x3e:
				outputText(dest,"ROL ");
				r=getAddressing(5,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

#ifdef CPU_65C02		// rockwell
			case 0x3f:
				outputText(dest,"B ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src=2;
				break;
#endif

			case 0x40:
				outputText(dest,"RTI");
				break;

			case 0x41:
				outputText(dest,"EOR ");
				r=getAddressing(9,Pipe2.bytes.byte1);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src++;
				break;

#ifdef CPU_KIMKLONE
			case 0x42:    // LDK1
				_K[1]=Pipe2.bytes.byte1;
				src++;
				break;

			case 0x43:    // K3_
        theK=&_K[3];
				break;

			case 0x44:    // LDK2
        _K[2]=ram_seg[Pipe2.bytes.byte1];
				src++;
				break;
#endif

			case 0x45:
				outputText(dest,"EOR ");
				r=getAddressing(0,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

			case 0x46:
				outputText(dest,"LSR ");
				r=getAddressing(0,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

#ifdef CPU_65C02		// rockwell
			case 0x47:
				outputText(dest,"RES ");
				ram_seg[Pipe2.bytes.byte1] &= ~0x10;
				src++;
				break;
#endif

			case 0x48:
				outputText(dest,"PHA");
				break;

			case 0x49:
				outputText(dest,"EOR #");
				outputByte(dest,Pipe2.bytes.byte1);
				src++;
				break;

			case 0x4a:
				outputText(dest,"LSR A");
				break;

#ifdef CPU_KIMKLONE
			case 0x4b:    // PUSHK0
				stack_seg[_s--]=_K[0];
				break;
#endif

			case 0x4c:
				outputText(dest,"JMP ");
				outputAddress(dest,Pipe2.word);
				src+=2;
				break;

			case 0x4d:
				outputText(dest,"EOR ");
				r=getAddressing(4,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

			case 0x4e:
				outputText(dest,"LSR ");
				r=getAddressing(4,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

#ifdef CPU_65C02		// rockwell
			case 0x4f:
				outputText(dest,"B ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src=2;
				break;
#endif

			case 0x50:
				outputText(dest,"BVC ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src++;
				break;

			case 0x51:
				outputText(dest,"EOR ");
				r=getAddressing(10,Pipe2.bytes.byte1);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src++;
				break;

#ifdef CPU_65C02
			case 0x52:
				outputText(dest,"EOR ");
				_a ^= GetValue(MAKEWORD(ram_seg[Pipe2.bytes.byte1],ram_seg[Pipe2.bytes.byte1+1]));
				src++;
				break;
#endif

#ifdef CPU_KIMKLONE
			case 0x53:    // PLYK3
				_K[3]=_y=stack_seg[++_s];
				goto aggFlagY;
				break;
			case 0x54:    // LDK1
        _K[1]=ram_seg[Pipe2.bytes.byte1+_x];
				src++;
				break;
#endif

			case 0x55:
				outputText(dest,"EOR ");
				r=getAddressing(1,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

			case 0x56:
				outputText(dest,"LSR ");
				r=getAddressing(1,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

#ifdef CPU_65C02		// rockwell
			case 0x57:
				outputText(dest,"RES ");
				ram_seg[Pipe2.bytes.byte1] &= ~0x20;
				src++;
				break;
#endif

			case 0x58:
				outputText(dest,"SEI");
				break;

			case 0x59:
				outputText(dest,"EOR ");
				r=getAddressing(6,Pipe2.bytes.byte1);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

#ifdef CPU_65C02
			case 0x5a:
				outputText(dest,"PHY");
				break;
#endif

#ifdef CPU_KIMKLONE
			case 0x5b:    // PUSHK3
				stack_seg[_s--]=_K[3];
				break;
#endif

#ifdef CPU_KIMKLONE
			case 0x5c:    // K0<>K3
        res3.b.l=_K[0];
        _K[0]=_K[3];
        _K[3]=res3.b.l;
				break;
#endif

			case 0x5d:
				outputText(dest,"EOR ");
				r=getAddressing(5,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

			case 0x5e:
				outputText(dest,"LSR ");
				r=getAddressing(5,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

#ifdef CPU_65C02		// rockwell
			case 0x5f:
				outputText(dest,"B ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src=2;
				break;
#endif

			case 0x60:
				outputText(dest,"RTS");
				break;

			case 0x61:
				outputText(dest,"ADC ");
				r=getAddressing(9,Pipe2.bytes.byte1);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src++;
				break;

#ifdef CPU_KIMKLONE
			case 0x63:    // PLYK1
				_K[1]=_y=stack_seg[++_s];
				goto aggFlagY;
				break;
#endif

#ifdef CPU_65C02
			case 0x64:
				outputText(dest,"STZ ");
				PutValue(Pipe2.bytes.byte1,0);
				src++;
				break;
#endif

			case 0x65:
				outputText(dest,"ADC ");
				r=getAddressing(0,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

			case 0x66:
				outputText(dest,"ROR ");
				r=getAddressing(0,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

#ifdef CPU_65C02		// rockwell
			case 0x67:
				outputText(dest,"RES ");
				ram_seg[Pipe2.bytes.byte1] &= ~0x40;
				src++;
				break;
#endif

			case 0x68:
				outputText(dest,"PLA");
				break;

			case 0x69:
				outputText(dest,"ADC #");
				outputByte(dest,Pipe2.bytes.byte1);
				src++;
				break;

			case 0x6a:
				outputText(dest,"ROR A");
				break;

#ifdef CPU_KIMKLONE
			case 0x6b:    // PUSHK1
				stack_seg[_s--]=_K[1];
				break;
#endif

			case 0x6c:
				outputText(dest,"JMP (");
				outputAddress(dest,Pipe2.word);
				outputText(dest,")");
				break;

			case 0x6d:
				outputText(dest,"ADC ");
				r=getAddressing(4,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

			case 0x6e:
				outputText(dest,"ROR ");
				r=getAddressing(4,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

#ifdef CPU_65C02		// rockwell
			case 0x6f:
				outputText(dest,"B ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src=2;
				break;
#endif

			case 0x70:
				outputText(dest,"BVS ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src++;
				break;

			case 0x71:
				outputText(dest,"ADC ");
				r=getAddressing(10,Pipe2.bytes.byte1);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src++;
				break;

#ifdef CPU_65C02
			case 0x72:
				outputText(dest,"ADC ");
				res2.b.l=_a + GetValue(MAKEWORD(ram_seg[Pipe2.bytes.byte1],ram_seg[Pipe2.bytes.byte1+1]));
				src++;
				break;

			case 0x74:
				outputText(dest,"STZ ");
				ram_seg[Pipe2.bytes.byte1+_x]=0;
				src++;
				break;
#endif

#ifdef CPU_KIMKLONE
			case 0x73:    // PLYK2
				_K[2]=_y=stack_seg[++_s];
				goto aggFlagY;
				break;
#endif

			case 0x75:
				outputText(dest,"ADC ");
				r=getAddressing(1,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

			case 0x76:
				outputText(dest,"ROR ");
				r=getAddressing(1,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

#ifdef CPU_65C02		// rockwell
			case 0x77:
				outputText(dest,"RES ");
				ram_seg[Pipe2.bytes.byte1] &= ~0x80;
				src++;
				break;
#endif

			case 0x78:
				outputText(dest,"SEI");
				break;

			case 0x79:
				outputText(dest,"ADC ");
				r=getAddressing(6,Pipe2.bytes.byte1);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

#ifdef CPU_65C02
			case 0x7a:    // PLY
				outputText(dest,"PLY");
				break;

			case 0x7c:
				PC=GetIntValue(Pipe2.word+_x);
				break;
#endif

#ifdef CPU_KIMKLONE
			case 0x7b:    // PUSHK2
				stack_seg[_s--]=_K[2];
				break;
#endif

			case 0x7d:
				outputText(dest,"ADC ");
				r=getAddressing(5,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

			case 0x7e:
				outputText(dest,"ROR ");
				r=getAddressing(5,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

#ifdef CPU_65C02
			// rockwell
			case 0x7f:
				outputText(dest,"B ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src=2;
				break;

			case 0x80:
				outputText(dest,"BRA ");
				src+=(signed char)Pipe2.bytes.byte1+2;
				break;
#endif

			case 0x81:
				outputText(dest,"STA ");
				r=getAddressing(9,Pipe2.bytes.byte1);
				src++;
				break;

#ifdef CPU_KIMKLONE
			case 0x83:    // K1_
        theK=&_K[1];
				break;
#endif

			case 0x84:
				outputText(dest,"STY ");
				r=getAddressing(0,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

			case 0x85:
				outputText(dest,"STA ");
				r=getAddressing(0,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

			case 0x86:
				outputText(dest,"STX ");
				r=getAddressing(0,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

#ifdef CPU_65C02		// rockwell
			case 0x87:
				outputText(dest,"SET ");
				ram_seg[Pipe2.bytes.byte1] |= 0x1;
				src++;
				break;
#endif

			case 0x88:
				outputText(dest,"DEY");
				break;

#ifdef CPU_65C02
			case 0x89:
				res3.b.l = Pipe2.bytes.byte1;
				src++;
				goto aggBit;		// OCCHIO AI flag, dice qua http://6502.org/tutorials/65c02opcodes.html
				break;
#endif

			case 0x8a:
				outputText(dest,"TXA ");
				break;

#ifdef CPU_KIMKLONE
			case 0x8b:    // TSB_K1 
				res3.b.l= _K[1] & GetValue(Pipe2.word);
				PutValue(Pipe2.word,GetValue(Pipe2.word) | _K[1]);
				src+=2;
aggTrb:
				_p.Zero=!res3.b.l;
				break;
#endif

			case 0x8c:
				outputText(dest,"STY ");
				r=getAddressing(4,Pipe2.word);
				outputText(dest,r);
				src+=2;
				break;

			case 0x8d:
				outputText(dest,"STA ");
				r=getAddressing(4,Pipe2.word);
				outputText(dest,r);
				src+=2;
				break;

			case 0x8e:
				outputText(dest,"STX ");
				r=getAddressing(4,Pipe2.word);
				outputText(dest,r);
				src+=2;
				break;

#ifdef CPU_65C02		// rockwell
			case 0x8f:
				outputText(dest,"B ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src=2;
				break;
#endif

			case 0x90:
				outputText(dest,"BCC ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src++;
				break;

			case 0x91:
				outputText(dest,"STA ");
				r=getAddressing(10,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

#ifdef CPU_65C02
			case 0x92:
				outputText(dest,"STA ");
				PutValue(MAKEWORD(ram_seg[Pipe2.bytes.byte1],ram_seg[Pipe2.bytes.byte1+1]),_a);
				src++;
				break;
#endif

#ifdef CPU_KIMKLONE
			case 0x93:    // STA_K1
        theK=&_K[1];
				PutValue(Pipe2.word,_a);
        theK=&_K[0];
        src+=2;
				break;
#endif

			case 0x94:
				outputText(dest,"STY ");
				r=getAddressing(1,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

			case 0x95:
				outputText(dest,"STA ");
				r=getAddressing(1,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

			case 0x96:
				outputText(dest,"STX ");
				r=getAddressing(1,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

#ifdef CPU_65C02		// rockwell
			case 0x97:
				outputText(dest,"SET ");
				ram_seg[Pipe2.bytes.byte1] |= 0x2;
				src++;
				break;
#endif

			case 0x98:
				outputText(dest,"TYA");
				break;

			case 0x99:
				outputText(dest,"STA ");
				r=getAddressing(6,Pipe2.bytes.byte1);
				outputText(dest,r);
				src+=2;
				break;

			case 0x9a:
				outputText(dest,"TXS");
				break;

#ifdef CPU_KIMKLONE
			case 0x9b:    // TRB_K1
				res3.b.l= _K[1] & GetValue(Pipe2.word);		
				PutValue(Pipe2.word,GetValue(Pipe2.word) & ~_K[1]);
				src+=2;
        goto aggTrb;
				break;
#endif

#ifdef CPU_65C02
			case 0x9c:
				outputText(dest,"STZ ");
				PutValue(Pipe2.word,0);
				src+=2;
				break;
#endif

			case 0x9d:
				outputText(dest,"STA ");
				r=getAddressing(5,Pipe2.word);
				outputText(dest,r);
				src+=2;
				break;

#ifdef CPU_65C02
			case 0x9e:
				outputText(dest,"STZ ");
				r=getAddressing(5,Pipe2.word);
				outputText(dest,r);
				src+=2;
				break;

			// rockwell
			case 0x9f:
				outputText(dest,"B ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src=2;
				break;
#endif

			case 0xa0:
				outputText(dest,"LDY #");
				outputByte(dest,Pipe2.bytes.byte1);
				src++;
				break;

			case 0xa1:
				outputText(dest,"LDA ");
				r=getAddressing(9,Pipe2.bytes.byte1);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src++;
				break;

			case 0xa2:
				outputText(dest,"LDX #");
				outputByte(dest,Pipe2.bytes.byte1);
				src++;
				break;

#ifdef CPU_KIMKLONE
			case 0xa3:    // STA_K1
        theK=&_K[1];
				PutValue(MAKEWORD(ram_seg[Pipe2.bytes.byte1],ram_seg[Pipe2.bytes.byte1+1])+_y,_a);
        theK=&_K[0];
				break;
#endif

			case 0xa4:
				outputText(dest,"LDY ");
				r=getAddressing(0,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

			case 0xa5:
				outputText(dest,"LDA ");
				r=getAddressing(0,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

			case 0xa6:
				outputText(dest,"LDX ");
				r=getAddressing(0,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

#ifdef CPU_65C02		// rockwell
			case 0xa7:
				outputText(dest,"SET ");
				ram_seg[Pipe2.bytes.byte1] |= 0x4;
				src++;
				break;
#endif

			case 0xa8:
				outputText(dest,"TAY");
				break;

			case 0xa9:
				outputText(dest,"LDA #");
				outputByte(dest,Pipe2.bytes.byte1);
				src++;
				break;

			case 0xaa:
				outputText(dest,"TAX");
				break;

#ifdef CPU_KIMKLONE
			case 0xab:    // TSB_K1
				res3.b.l= _K[1] & GetValue(_IP.w);
				PutValue(_IP.w,GetValue(_IP.w) | _K[1]);
        goto aggTrb;
				break;
#endif

			case 0xac:
				outputText(dest,"LDY ");
				r=getAddressing(4,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

			case 0xad:
				outputText(dest,"LDA ");
				r=getAddressing(4,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

			case 0xae:
				outputText(dest,"LDX ");
				r=getAddressing(4,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

#ifdef CPU_65C02		// rockwell
			case 0xaf:
				outputText(dest,"B ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src=2;
				break;
#endif

			case 0xb0:
				outputText(dest,"BCS ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src++;
				break;

			case 0xb1:
				outputText(dest,"LDA ");
				r=getAddressing(10,Pipe2.bytes.byte1);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src++;
				break;

#ifdef CPU_65C02
			case 0xb2:
				outputText(dest,"LDA ");
				_a = GetValue(MAKEWORD(ram_seg[Pipe2.bytes.byte1],ram_seg[Pipe2.bytes.byte1+1]));
				src++;
				break;
#endif

#ifdef CPU_KIMKLONE
			case 0xb3:    // STA_K1
        theK=&_K[1];
				PutValue(MAKEWORD(ram_seg[Pipe2.bytes.byte1+_x],ram_seg[Pipe2.bytes.byte1+_x+1]),_a);
        theK=&_K[0];
				src++;
				break;
#endif

			case 0xb4:
				outputText(dest,"LDY ");
				r=getAddressing(1,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

			case 0xb5:
				outputText(dest,"LDA ");
				r=getAddressing(1,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

			case 0xb6:
				outputText(dest,"LDX ");
				r=getAddressing(1,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

#ifdef CPU_65C02		// rockwell
			case 0xb7:
				outputText(dest,"SET ");
				ram_seg[Pipe2.bytes.byte1] |= 0x8;
				src++;
				break;
#endif

			case 0xb8:
				outputText(dest,"CLV");
				break;

			case 0xb9:
				outputText(dest,"LDA ");
				r=getAddressing(6,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

			case 0xba:
				outputText(dest,"TSX");
				break;

#ifdef CPU_KIMKLONE
			case 0xbb:    // TRB_K1
				res3.b.l= _K[1] & GetValue(_IP.w);		// 
				PutValue(Pipe2.word,GetValue(_IP.w) & ~_K[1]);
        goto aggTrb;
				break;
#endif

			case 0xbc:
				outputText(dest,"LDY ");
				r=getAddressing(5,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

			case 0xbd:
				outputText(dest,"LDA ");
				r=getAddressing(5,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

			case 0xbe:
				outputText(dest,"LDX ");
				r=getAddressing(6,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

#ifdef CPU_65C02		// rockwell
			case 0xbf:
				outputText(dest,"B ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src=2;
				break;
#endif

			case 0xc0:
				outputText(dest,"CPY ");
				outputByte(dest,Pipe2.bytes.byte1);
				src++;
				break;

			case 0xc1:
				outputText(dest,"CMP ");
				r=getAddressing(9,Pipe2.bytes.byte1);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src++;
				break;

#ifdef CPU_KIMKLONE
			case 0xc2:    // LDK2
				_K[2]=Pipe2.bytes.byte1;
				src++;
				break;

			case 0xc3:    // K2_
        theK=&_K[2];
				break;
#endif

			case 0xc4:
				outputText(dest,"CPY ");
				r=getAddressing(0,Pipe2.word);
				outputText(dest,r);
				src++;
				break;

			case 0xc5:
				outputText(dest,"CMP ");
				r=getAddressing(0,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

			case 0xc6:
				outputText(dest,"DEC ");
				r=getAddressing(0,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

#ifdef CPU_65C02		// rockwell
			case 0xc7:
				outputText(dest,"SET ");
				ram_seg[Pipe2.bytes.byte1] |= 0x10;
				src++;
				break;
#endif

			case 0xc8:
				outputText(dest,"INY");
				break;

			case 0xc9:
				outputText(dest,"CMP #");
				outputByte(dest,Pipe2.bytes.byte1);
				src++;
				break;

			case 0xca:
				outputText(dest,"DEX");
				break;

#ifdef CPU_65C02		// rockwell
			case 0xcb:			//WAI
				// while(!(CPUPins & (DoReset | DoNMI | DoIRQ))) ClrWdt();
				outputText(dest,"WAI");
				break;
#endif
#ifdef CPU_KIMKLONE
			case 0xcb:    // LDAW
        _W.w=MAKEWORD(ram_seg[Pipe2.bytes.byte1+_x],ram_seg[Pipe2.bytes.byte1+_x+1]);
				_a = GetValue(_W.w);
        theK=&_K[0];
				src++;
				break;
#endif

			case 0xcc:
				outputText(dest,"CMP ");
				r=getAddressing(4,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

			case 0xcd:
				outputText(dest,"CMP ");
				r=getAddressing(4,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

			case 0xce:
				outputText(dest,"DEC ");
				r=getAddressing(4,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

#ifdef CPU_65C02		// rockwell
			case 0xcf:
				outputText(dest,"B ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src=2;
				break;
#endif

			case 0xd0:
				outputText(dest,"BNE ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src++;
				break;

			case 0xd1:
				outputText(dest,"CMP ");
				r=getAddressing(10,Pipe2.bytes.byte1);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src++;
				break;

#ifdef CPU_65C02
			case 0xd2:
				outputText(dest,"CMP ");
				res3.w=(SWORD)_a-GetValue(MAKEWORD(ram_seg[Pipe2.bytes.byte1],ram_seg[Pipe2.bytes.byte1+1]));
				src++;
				break;
#endif

#ifdef CPU_KIMKLONE
			case 0xd3:    // LDA_K2
        theK=&_K[2];
				_a=GetValue(Pipe2.word);
        theK=&_K[0];
        src+=2;
				break;
			case 0xd4:    // LDK2
        _K[2]=ram_seg[Pipe2.bytes.byte1+_x];
				src++;
				break;
#endif

			case 0xd5:
				outputText(dest,"CMP ");
				r=getAddressing(1,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

			case 0xd6:
				outputText(dest,"DEC ");
				r=getAddressing(1,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

#ifdef CPU_65C02		// rockwell
			case 0xd7:
				outputText(dest,"SET ");
				ram_seg[Pipe2.bytes.byte1] |= 0x20;
				src++;
				break;
#endif

			case 0xd8:
				outputText(dest,"CLD");
				break;

			case 0xd9:
				outputText(dest,"CMP ");
				r=getAddressing(6,Pipe2.bytes.byte1);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

#ifdef CPU_65C02
			case 0xda:
				outputText(dest,"PHX");
				break;

			case 0xdb: //STP
				outputText(dest,"STP");
				// while(!(CPUPins & DoReset)) ClrWdt();
				break;
#endif

#ifdef CPU_KIMKLONE
			case 0xdb:    //STAW
        _W.w=MAKEWORD(ram_seg[Pipe2.bytes.byte1+_x],ram_seg[Pipe2.bytes.byte1+_x+1]);
				PutValue(_W.w,_a);
				src++;
				break;
			case 0xdc:    // LDK1
        _K[1]=GetValue(Pipe2.word);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;
#endif

			case 0xdd:
				outputText(dest,"CMP ");
				r=getAddressing(5,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

			case 0xde:
				outputText(dest,"DEC ");
				r=getAddressing(5,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

#ifdef CPU_65C02		// rockwell
			case 0xdf:
				outputText(dest,"B ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src=2;
				break;
#endif

			case 0xe0:
				outputText(dest,"CPX #");
				outputByte(dest,Pipe2.bytes.byte1);
				src++;
				break;

			case 0xe1:
				outputText(dest,"SBC ");
				r=getAddressing(9,Pipe2.bytes.byte1);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src++;
				break;

#ifdef CPU_KIMKLONE
			case 0xe2:    // LDK3
				_K[3]=Pipe2.bytes.byte1;
				src++;
				break;
			case 0xe3:    // LDA_K2
        theK=&_K[2];
				_a = GetValue(MAKEWORD(ram_seg[Pipe2.bytes.byte1],ram_seg[Pipe2.bytes.byte1+1])+_y);
        theK=&_K[0];
				src++;
				break;
#endif

			case 0xe4:
				outputText(dest,"CPX ");
				r=getAddressing(0,Pipe2.bytes.byte1);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src++;
				break;

			case 0xe5:
				outputText(dest,"SBC ");
				r=getAddressing(0,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

			case 0xe6:
				outputText(dest,"INC ");
				r=getAddressing(0,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

#ifdef CPU_65C02		// rockwell
			case 0xe7:
				outputText(dest,"SET ");
				ram_seg[Pipe2.bytes.byte1] |= 0x40;
				src++;
				break;
#endif

			case 0xe8:
				outputText(dest,"INX");
				break;

			case 0xe9:
				outputText(dest,"SBC #");
				outputByte(dest,Pipe2.bytes.byte1);
				src++;
				break;

			case 0xea:    // NOP
				outputText(dest,"NOP");
				break;

#ifdef CPU_KIMKLONE
			case 0xeb:    // RTS_K3
        res3.b.l=_K[0];
        _K[0]=_K[3];
        _K[3]=res3.b.l;
				src=stack_seg[++_s];
				src |= ((short int)stack_seg[++_s]) << 8;
				src++;
				break;
#endif

			case 0xec:
				outputText(dest,"CPX ");
				r=getAddressing(4,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

			case 0xed:
				outputText(dest,"SBC ");
				r=getAddressing(4,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

			case 0xee:
				outputText(dest,"INC ");
				r=getAddressing(4,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

#ifdef CPU_65C02		// rockwell
			case 0xef:
				outputText(dest,"B ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src=2;
				break;
#endif

			case 0xf0:    // BEQ
				outputText(dest,"BEQ ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src++;
				break;

			case 0xf1:
				outputText(dest,"SBC ");
				r=getAddressing(10,Pipe2.bytes.byte1);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src++;
				break;

#ifdef CPU_65C02
			case 0xf2:
				outputText(dest,"SBC ");
				res2.b.l=GetValue(MAKEWORD(ram_seg[Pipe2.bytes.byte1],ram_seg[Pipe2.bytes.byte1+1]));
				src++;
				break;
#endif

#ifdef CPU_KIMKLONE
			case 0xf3:    // LDA_K2
        theK=&_K[2];
				_a = GetValue(MAKEWORD(ram_seg[Pipe2.bytes.byte1+_x],ram_seg[Pipe2.bytes.byte1+_x+1]));
        theK=&_K[0];
				src++;
				break;
			case 0xf4:    // LDK3
        _K[3]=ram_seg[Pipe2.bytes.byte1+_x];
				src++;
				break;
#endif

			case 0xf5:
				outputText(dest,"SBC ");
				r=getAddressing(1,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

			case 0xf6:
				outputText(dest,"INC ");
				r=getAddressing(1,Pipe2.bytes.byte1);
				outputText(dest,r);
				src++;
				break;

#ifdef CPU_65C02		// rockwell
			case 0xf7:
				outputText(dest,"SET ");
				ram_seg[Pipe2.bytes.byte1] |= 0x80;
				// e i REGISTRI CPU??
				src++;
				break;
#endif

			case 0xf8:
				outputText(dest,"SED");
				break;

			case 0xf9:
				outputText(dest,"SBC ");
				r=getAddressing(6,Pipe2.bytes.byte1);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

#ifdef CPU_65C02
			case 0xfa:
				outputText(dest,"PLX");
				break;
#endif

#ifdef CPU_KIMKLONE
			case 0xfb:    // RTI_K3
        res3.b.l=_K[0];
        _K[0]=_K[3];
        _K[3]=res3.b.l;
				_p.b = stack_seg[++_s];
				src=stack_seg[++_s];
				src |= ((short int)stack_seg[++_s]) << 8;
				_p.Break=0; _p.unused=1;    // meglio, viste le specifiche :)
				break;
			case 0xfc:    // LDK3
        _K[3]=GetValue(Pipe2.word);
        theK=&_K[0];
				src+=2;
				break;
#endif

			case 0xfd:
				outputText(dest,"SBC ");
				r=getAddressing(5,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

			case 0xfe:
				outputText(dest,"INC ");
				r=getAddressing(5,Pipe2.word);
				outputText(dest,r);
#ifdef CPU_KIMKLONE
        theK=&_K[0];
#endif
				src+=2;
				break;

#ifdef CPU_65C02		// rockwell
			case 0xff:
				outputText(dest,"B ");
				outputAddress(dest,pcaddr+(signed char)Pipe2.bytes.byte1+2);
				src=2;
				break;
#endif
        
			default:
        
				outputUnknown(dest,Pipe1);
        
				break;
			}

		pcaddr+=(src-oldsrc);
		if(len)
			len-=(src-oldsrc);
		if(opzioni & 1) {
			char myBuf[256],myBuf2[32];

			strcpy(myBuf,dest);
			*dest=0;
			if(opzioni & 2) {
				wsprintf(dest,"%04X: ",(WORD)(pcaddr-(src-oldsrc)));
				}
			while(oldsrc<src) {
				wsprintf(myBuf2,"%02X ",*oldsrc);
				strcat(dest,myBuf2);
				oldsrc++;
				}
			strcat(dest,"\t");
			strcat(dest,myBuf);

			}
		if(*commento) {
			strcat(dest,"\t; ");
			strcat(dest,commento);
			}
		strcat(dest,"\r\n");

		if(f != -1/*INVALID_HANDLE*/) {
			_lwrite(spoolFile,dest,strlen(dest));
			*dest=0;
			}

		if(!len)
			fExit=1;

		} while(!fExit);
#endif


	}


