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

static const char *getRegister(BYTE reg) {
	const char *r;

	switch(reg & 15) {
		case 0:
			r="R0";
			break;
		case 1:
			r="R1";
			break;
		case 2:
			r="R2";
			break;
		case 3:
			r="R3";
			break;
		case 4:
			r="R4";
			break;
		case 5:
			r="R5";
			break;
		case 6:
			r="R6";
			break;
		case 7:
			r="R7";
			break;
		case 8:
			r="R8";
			break;
		case 9:
			r="R9";
			break;
		case 10:
			r="R10";
			break;
		case 11:
			r="R11";
			break;
		case 12:
			r="R12";
			break;
		case 13:
			r="R13";
			break;
		case 14:
			r="R14";
			break;
		case 15:
			r="R15";
			break;
		}

	return r;
	}

static const char *getAddressing(BYTE m,BYTE reg,WORD n) {
	const char *r;
	static    char myBuf[32];		// migliorare
	char myBuf2[32];	

  switch(m) {
    case REGISTER_DIRECT:
		  r=getRegister(reg);
			strcpy(myBuf,r);
      break;
    case REGISTER_INDIRECT:
		  r=getRegister(reg);
			strcpy(myBuf,"*");
			strcat(myBuf,r);
      break;
    case REGISTER_SYMBOLIC_INDEXED:
      if(reg)
				wsprintf(myBuf,"@>%04X(R%u)",n,reg);
      else
				wsprintf(myBuf,"@>%04X",n);
			if(Opzioni & 4) {
				*myBuf2=0;		// safety!
				wsprintf(myBuf2,"%u",n);
				if(*commento)
					strcat(commento," ");
				strcat(commento,myBuf2);
		    }
      break;
    case REGISTER_INDIRECT_AUTOINCREMENT:
		  r=getRegister(reg);
			strcpy(myBuf,"*");
			strcat(myBuf,r);
			strcat(myBuf,"+");
      break;
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

	wsprintf(myBuf,">%04Xh",n);
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

static char *outputUnknown(char *d,WORD n) {

	outputText(d,"DW ");
	outputWord(d,n);
	return d;
	}

int Disassemble(const WORD *src,HFILE f,char *dest,DWORD len,WORD pcaddr,BYTE opzioni) {
	// opzioni: b0 addbytes, b1 addPC, b2 numeri dec in commenti
	const WORD *oldsrc;
	const char *r,*r2;
	WORD Pipe1;
	union PIPE Pipe2,Pipe3;
  register uint16_t i;
	BYTE fExit=0;
  uint8_t workingTS,workingTD,workingRegIndex,workingReg2Index;
	char S1[64],S2[64];


	Opzioni=opzioni;
	CPUType=0;

	*dest=0;


	do {

		oldsrc=src;

		*commento=0;

// https://www.masswerk.at/6502/6502_instruction_set.html
		Pipe1=*src++;
		Pipe3.x=Pipe2.x=*src;

	  workingTS=WORKING_TS; workingTD=WORKING_TD;   // mettere solo dove serve??
		workingRegIndex=WORKING_REG_INDEX; workingReg2Index=WORKING_REG2_INDEX;


		if((Pipe1 & B16(11110000,00000000)) == B8(0000)) {
			if((Pipe1 & B16(11111111,00000000)) == B16(00000010,00000000)) {
				switch(Pipe1 & B16(11111111,11110000)) {
					case B16(00000010,00000000):		// LI
						outputText(dest,"LI ");
						r=getRegister((BYTE)workingRegIndex);
						strcpy(S1,r);
						outputText(dest,S1);
						outputComma(dest);
						outputWord(dest,Pipe2.x);
						src++;
						break;

					case B16(00000010,00100000):		// AI
						outputText(dest,"AI ");
						r=getRegister((BYTE)workingRegIndex);
						strcpy(S1,r);
						outputText(dest,S1);
						outputComma(dest);
						outputWord(dest,Pipe2.x);
						src++;
						break;

					case B16(00000010,01000000):		// ANDI
						outputText(dest,"ANDI ");
						r=getRegister((BYTE)workingRegIndex);
						strcpy(S1,r);
						outputText(dest,S1);
						outputComma(dest);
						outputWord(dest,Pipe2.x);
						src++;
						break;

					case B16(00000010,01100000):		// ORI
						outputText(dest,"ORI ");
						r=getRegister((BYTE)workingRegIndex);
						strcpy(S1,r);
						outputText(dest,S1);
						outputComma(dest);
						outputWord(dest,Pipe2.x);
						src++;
						break;

					case B16(00000010,10000000):		// CI
						outputText(dest,"CI ");
						r=getRegister((BYTE)workingRegIndex);
						strcpy(S1,r);
						outputText(dest,S1);
						outputComma(dest);
						outputWord(dest,Pipe2.x);
						src++;
						break;
					case B16(00000010,10100000):		// STWP
						outputText(dest,"STWP ");
						r=getRegister((BYTE)workingRegIndex);
						strcpy(S1,r);
						outputText(dest,S1);
						break;
					case B16(00000010,11000000):		// STST
						outputText(dest,"STST ");
						r=getRegister((BYTE)workingRegIndex);
						strcpy(S1,r);
						outputText(dest,S1);
						break;
					case B16(00000010,11100000):		// LWPI
						outputText(dest,"LWPI ");
						outputWord(dest,Pipe2.x);
						src++;
						break;
					default:
						goto unknown;
						break;
					}
				}
			else if((Pipe1 & B16(11111111,00000000)) == B16(00000011,00000000)) {
				switch(Pipe1 & B16(11111111,11110000)) {
					case B16(00000011,00000000):		// LIMI
						outputText(dest,"LIMI ");
						outputWord(dest,Pipe2.x & 0xf);
						src++;
						break;

					case B16(00000011,01000000):		// IDLE
						outputText(dest,"IDLE ");
						break;
					case B16(00000011,01100000):		// RSET
						outputText(dest,"RSET ");
						break;
					case B16(00000011,10000000):		// RTWP
						outputText(dest,"RTWP ");
						break;
					case B16(00000011,10100000):		// CKON
						outputText(dest,"CKON ");
						break;
					case B16(00000011,11000000):		// CKOF
						outputText(dest,"CKOF ");
						break;
					case B16(00000011,11100000):		// LREX
						outputText(dest,"LREX ");
						break;
					default:
						goto unknown;
						break;
					}
				}

			else if((Pipe1 & B16(11111100,00000000)) == B16(00000100,00000000)) {
				switch(Pipe1 & B16(11111111,11000000)) {
					case B16(00000100,00000000):		// BLWP
						outputText(dest,"BLWP ");
						r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
						outputText(dest,r);
						if(workingTS==REGISTER_SYMBOLIC_INDEXED)
							src++;
						break;
					case B16(00000100,01000000):		// B
						outputText(dest,"B ");
						r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
						outputText(dest,r);
						if(workingTS==REGISTER_SYMBOLIC_INDEXED)
							src++;
						break;
					case B16(00000100,10000000):		// X
						outputText(dest,"X ");
						r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
						outputText(dest,r);
						if(workingTS==REGISTER_SYMBOLIC_INDEXED)
							src++;
						break;
					case B16(00000100,11000000):		// CLR
						outputText(dest,"CLR ");
						r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
						outputText(dest,r);
						if(workingTS==REGISTER_SYMBOLIC_INDEXED)
							src++;
						break;
					case B16(00000101,00000000):		// NEG
						outputText(dest,"NEG ");
						r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
						outputText(dest,r);
						if(workingTS==REGISTER_SYMBOLIC_INDEXED)
							src++;
						break;
					case B16(00000101,01000000):		// INV
						outputText(dest,"INV ");
						r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
						outputText(dest,r);
						if(workingTS==REGISTER_SYMBOLIC_INDEXED)
							src++;
						break;
					case B16(00000101,10000000):		// INC
						outputText(dest,"INC ");
						r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
						outputText(dest,r);
						if(workingTS==REGISTER_SYMBOLIC_INDEXED)
							src++;
						break;
					case B16(00000101,11000000):		// INCT
						outputText(dest,"INCT ");
						r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
						outputText(dest,r);
						if(workingTS==REGISTER_SYMBOLIC_INDEXED)
							src++;
						break;
					case B16(00000110,00000000):		// DEC
						outputText(dest,"DEC ");
						r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
						outputText(dest,r);
						if(workingTS==REGISTER_SYMBOLIC_INDEXED)
							src++;
						break;
					case B16(00000110,01000000):		// DECT
						outputText(dest,"DECT ");
						r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
						outputText(dest,r);
						if(workingTS==REGISTER_SYMBOLIC_INDEXED)
							src++;
						break;
					case B16(00000110,10000000):		// BL
						outputText(dest,"BL ");
						r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
						outputText(dest,r);
						if(workingTS==REGISTER_SYMBOLIC_INDEXED)
							src++;
						break;
					case B16(00000110,11000000):		// SWPB
						outputText(dest,"SWPB ");
						r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
						outputText(dest,r);
						if(workingTS==REGISTER_SYMBOLIC_INDEXED)
							src++;
						break;
					case B16(00000111,00000000):		// SETO
						outputText(dest,"SETO ");
						r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
						outputText(dest,r);
						if(workingTS==REGISTER_SYMBOLIC_INDEXED)
							src++;
						break;
					case B16(00000111,01000000):		// ABS
						outputText(dest,"ABS ");
						r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
						outputText(dest,r);
						if(workingTS==REGISTER_SYMBOLIC_INDEXED)
							src++;
						break;
#ifdef TMS9940 
					case B16(00101100,00000000):		// DCA
						outputText(dest,"DCA ");
						r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
						outputText(dest,r);
						if(workingTS==REGISTER_SYMBOLIC_INDEXED)
							src++;
						break;
					case B16(00101100,01000000):		// DCS
						outputText(dest,"DCS ");
						r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
						outputText(dest,r);
						if(workingTS==REGISTER_SYMBOLIC_INDEXED)
							src++;
						break;
					case B16(00101100,10000000):		// LIM
						outputText(dest,"LIM ");
						r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
						outputText(dest,r);
						if(workingTS==REGISTER_SYMBOLIC_INDEXED)
							src++;
						break;
#endif
				default:
					goto unknown;
					break;
					}
				}
			else if((Pipe1 & B16(11111000,00000000)) == B16(00001000,00000000)) {
				switch(Pipe1 & B16(11111111,00000000)) {
					case B16(00001000,00000000):		// SRA
						outputText(dest,"SRA ");
						r=getRegister((BYTE)workingRegIndex);
						strcpy(S1,r);
						outputText(dest,S1);
						outputComma(dest);
						if(!(Pipe1 & B16(00000000,11110000))) {
							r=getRegister(0);
							strcpy(S1,r);
							outputText(dest,S1);
							}
						else {
							outputByte(dest,LOBYTE(Pipe1) >> 4);
							}
						break;
					case B16(00001001,00000000):		// SRL
						outputText(dest,"SRL ");
						r=getRegister((BYTE)workingRegIndex);
						strcpy(S1,r);
						outputText(dest,S1);
						outputComma(dest);
						if(!(Pipe1 & B16(00000000,11110000))) {
							r=getRegister(0);
							strcpy(S1,r);
							outputText(dest,S1);
							}
						else {
							outputByte(dest,LOBYTE(Pipe1) >> 4);
							}
						break;
					case B16(00001010,00000000):		// SLA
						outputText(dest,"SLA ");
						r=getRegister((BYTE)workingRegIndex);
						strcpy(S1,r);
						outputText(dest,S1);
						outputComma(dest);
						if(!(Pipe1 & B16(00000000,11110000))) {
							r=getRegister(0);
							strcpy(S1,r);
							outputText(dest,S1);
							}
						else {
							outputByte(dest,LOBYTE(Pipe1) >> 4);
							}
						break;
					case B16(00001011,00000000):		// SRC
						outputText(dest,"SRC ");
						r=getRegister((BYTE)workingRegIndex);
						strcpy(S1,r);
						outputText(dest,S1);
						outputComma(dest);
						if(!(Pipe1 & B16(00000000,11110000))) {
							r=getRegister(0);
							strcpy(S1,r);
							outputText(dest,S1);
							}
						else {
							outputByte(dest,LOBYTE(Pipe1) >> 4);
							}
						break;
					default:
						goto unknown;
						break;
					}
				}
			}
		else if((Pipe1 & B16(11110000,00000000)) == B16(00010000,00000000)) {
			switch(Pipe1 & B16(11111111,00000000)) {
				case B16(00010000,00000000):		// JMP
					outputText(dest,"JMP ");
					outputWord(dest,pcaddr+(int8_t)(LOBYTE(Pipe1)*2)+2);
					break;
				case B16(00010001,00000000):		// JLT
					outputText(dest,"JLT ");
					outputWord(dest,pcaddr+(int8_t)(LOBYTE(Pipe1)*2)+2);
					src++;
					break;
				case B16(00010010,00000000):		// JLE
					outputText(dest,"JLE ");
					outputWord(dest,pcaddr+(int8_t)(LOBYTE(Pipe1)*2)+2);
					src++;
					break;
				case B16(00010011,00000000):		// JEQ
					outputText(dest,"JEQ ");
					outputWord(dest,pcaddr+(int8_t)(LOBYTE(Pipe1)*2)+2);
					src++;
					break;
				case B16(00010100,00000000):		// JHE
					outputText(dest,"JHE ");
					outputWord(dest,pcaddr+(int8_t)(LOBYTE(Pipe1)*2)+2);
					src++;
					break;
				case B16(00010101,00000000):		// JGT
					outputText(dest,"JGT ");
					outputWord(dest,pcaddr+(int8_t)(LOBYTE(Pipe1)*2)+2);
					src++;
					break;
				case B16(00010110,00000000):		// JNE
					outputText(dest,"JNE ");
					outputWord(dest,pcaddr+(int8_t)(LOBYTE(Pipe1)*2)+2);
					src++;
					break;
				case B16(00010111,00000000):		// JNC
					outputText(dest,"JNC ");
					outputWord(dest,pcaddr+(int8_t)(LOBYTE(Pipe1)*2)+2);
					src++;
					break;
				case B16(00011000,00000000):		// JOC
					outputText(dest,"JOC ");
					outputWord(dest,pcaddr+(int8_t)(LOBYTE(Pipe1)*2)+2);
					src++;
					break;
				case B16(00011001,00000000):		// JNO
					outputText(dest,"JNO ");
					outputWord(dest,pcaddr+(int8_t)(LOBYTE(Pipe1)*2)+2);
					src++;
					break;
				case B16(00011010,00000000):		// JL
					outputText(dest,"JL ");
					outputWord(dest,pcaddr+(int8_t)(LOBYTE(Pipe1)*2)+2);
					src++;
					break;
				case B16(00011011,00000000):		// JH
					outputText(dest,"JH ");
					outputWord(dest,pcaddr+(int8_t)(LOBYTE(Pipe1)*2)+2);
					src++;
					break;
				case B16(00011100,00000000):		// JOP
					outputText(dest,"JOP ");
					outputWord(dest,pcaddr+(int8_t)(LOBYTE(Pipe1)*2)+2);
					src++;
					break;
				case B16(00011101,00000000):		// SBO
					outputText(dest,"SBO ");
					outputByte(dest,LOBYTE(Pipe1));
					break;
				case B16(00011110,00000000):		// SBZ
					outputText(dest,"SBZ ");
					outputByte(dest,LOBYTE(Pipe1));
					break;
				case B16(00011111,00000000):		// TB
					outputText(dest,"TB ");
					outputByte(dest,LOBYTE(Pipe1));
					break;

				default:
					goto unknown;
					break;
				}
			}
		else if((Pipe1 & B16(11110000,00000000)) == B16(00100000,00000000)) {
			switch(Pipe1 & B16(11111100,00000000)) {
				case B16(00100000,00000000):		// COC
					outputText(dest,"COC ");
					r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
					outputText(dest,r);
					if(workingTS==REGISTER_SYMBOLIC_INDEXED)
						src++;
					outputComma(dest);
					r=getRegister((BYTE)workingTD);
					strcpy(S1,r);
					outputText(dest,S1);
					break;
				case B16(00100100,00000000):		// CZC
					outputText(dest,"CZC ");
					r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
					outputText(dest,r);
					if(workingTS==REGISTER_SYMBOLIC_INDEXED)
						src++;
					outputComma(dest);
					r=getRegister((BYTE)workingTD);
					strcpy(S1,r);
					outputText(dest,S1);
					break;
				case B16(00101000,00000000):		// XOR
					outputText(dest,"XOR ");
					r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
					outputText(dest,r);
					if(workingTS==REGISTER_SYMBOLIC_INDEXED)
						src++;
					outputComma(dest);
					r=getRegister((BYTE)workingTD);
					strcpy(S1,r);
					outputText(dest,S1);
					break;
				case B16(00101100,00000000):		// XOP
					outputText(dest,"XOP ");
					r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
					outputText(dest,r);
					if(workingTS==REGISTER_SYMBOLIC_INDEXED)
						src++;
					outputComma(dest);
					r=getRegister((BYTE)workingTD);
					strcpy(S1,r);
					outputText(dest,S1);
					break;
				default:
					goto unknown;
					break;
				}
			}
		else if((Pipe1 & B16(11110000,00000000)) == B16(00110000,00000000)) {
			switch(Pipe1 & B16(11111100,00000000)) {
				case B16(00110000,00000000):		// LDCR
					outputText(dest,"LDCR ");
					r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
					outputText(dest,r);
					if(workingTS==REGISTER_SYMBOLIC_INDEXED)
						src++;
					outputComma(dest);
					outputByte(dest,(Pipe1 >> 6) & 0xf);
					break;
				case B16(00110100,00000000):		// STCR
					outputText(dest,"STCR ");
					r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
					outputText(dest,r);
					if(workingTS==REGISTER_SYMBOLIC_INDEXED)
						src++;
					outputComma(dest);
					outputByte(dest,(Pipe1 >> 6) & 0xf);
					break;
				case B16(00111000,00000000):		// MPY
					outputText(dest,"MPY ");
					r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
					outputText(dest,r);
					if(workingTS==REGISTER_SYMBOLIC_INDEXED)
						src++;
					outputComma(dest);
					r=getAddressing(0,REGISTER_DIRECT,workingReg2Index);
					outputText(dest,r);
					break;
				case B16(00111100,00000000):		// DIV
					outputText(dest,"DIV ");
					r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
					outputText(dest,r);
					if(workingTS==REGISTER_SYMBOLIC_INDEXED)
						src++;
					outputComma(dest);
					r=getAddressing(0,REGISTER_DIRECT,workingReg2Index);
					outputText(dest,r);
					break;
				default:
					goto unknown;
					break;
				}
			}
		else {
			switch(Pipe1 & B16(11110000,00000000)) {
				case B16(01000000,00000000):		// SZC
					outputText(dest,"SZC ");
					r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
					outputText(dest,r);
					if(workingTS==REGISTER_SYMBOLIC_INDEXED) {
						src++;
						Pipe3.x=*src;
						}
					outputComma(dest);
					r=getAddressing(workingTD,workingReg2Index,Pipe3.x);
					outputText(dest,r);
					if(workingTD==REGISTER_SYMBOLIC_INDEXED)
						src++;
					break;
				case B16(01010000,00000000):		// SZCB
					outputText(dest,"SZCB ");
					r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
					outputText(dest,r);
					if(workingTS==REGISTER_SYMBOLIC_INDEXED) {
						src++;
						Pipe3.x=*src;
						}
					outputComma(dest);
					r=getAddressing(workingTD,workingReg2Index,Pipe3.x);
					outputText(dest,r);
					if(workingTD==REGISTER_SYMBOLIC_INDEXED)
						src++;
					break;
				case B16(01100000,00000000):		// S
					outputText(dest,"S ");
					r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
					outputText(dest,r);
					if(workingTS==REGISTER_SYMBOLIC_INDEXED) {
						src++;
						Pipe3.x=*src;
						}
					outputComma(dest);
					r=getAddressing(workingTD,workingReg2Index,Pipe3.x);
					outputText(dest,r);
					if(workingTD==REGISTER_SYMBOLIC_INDEXED)
						src++;
					break;
				case B16(01110000,00000000):		// SB
					outputText(dest,"SB ");
					r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
					outputText(dest,r);
					if(workingTS==REGISTER_SYMBOLIC_INDEXED) {
						src++;
						Pipe3.x=*src;
						}
					outputComma(dest);
					r=getAddressing(workingTD,workingReg2Index,Pipe3.x);
					outputText(dest,r);
					if(workingTD==REGISTER_SYMBOLIC_INDEXED)
						src++;
					break;
				case B16(10000000,00000000):		// C
					outputText(dest,"C ");
					r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
					outputText(dest,r);
					if(workingTS==REGISTER_SYMBOLIC_INDEXED) {
						src++;
						Pipe3.x=*src;
						}
					outputComma(dest);
					r=getAddressing(workingTD,workingReg2Index,Pipe3.x);
					outputText(dest,r);
					if(workingTD==REGISTER_SYMBOLIC_INDEXED)
						src++;
					break;
				case B16(10010000,00000000):		// CB
					outputText(dest,"CB ");
					r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
					outputText(dest,r);
					if(workingTS==REGISTER_SYMBOLIC_INDEXED) {
						src++;
						Pipe3.x=*src;
						}
					outputComma(dest);
					r=getAddressing(workingTD,workingReg2Index,Pipe3.x);
					outputText(dest,r);
					if(workingTD==REGISTER_SYMBOLIC_INDEXED)
						src++;
					break;
				case B16(10100000,00000000):		// A
					outputText(dest,"A ");
					r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
					outputText(dest,r);
					if(workingTS==REGISTER_SYMBOLIC_INDEXED) {
						src++;
						Pipe3.x=*src;
						}
					outputComma(dest);
					r=getAddressing(workingTD,workingReg2Index,Pipe3.x);
					outputText(dest,r);
					if(workingTD==REGISTER_SYMBOLIC_INDEXED)
						src++;
					break;
				case B16(10110000,00000000):		// AB
					outputText(dest,"AB ");
					r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
					outputText(dest,r);
					if(workingTS==REGISTER_SYMBOLIC_INDEXED) {
						src++;
						Pipe3.x=*src;
						}
					outputComma(dest);
					r=getAddressing(workingTD,workingReg2Index,Pipe3.x);
					outputText(dest,r);
					if(workingTD==REGISTER_SYMBOLIC_INDEXED)
						src++;
					break;
				case B16(11000000,00000000):		// MOV
					outputText(dest,"MOV ");
					r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
					outputText(dest,r);
					if(workingTS==REGISTER_SYMBOLIC_INDEXED) {
						src++;
						Pipe3.x=*src;
						}
					outputComma(dest);
					r=getAddressing(workingTD,workingReg2Index,Pipe3.x);
					outputText(dest,r);
					if(workingTD==REGISTER_SYMBOLIC_INDEXED)
						src++;
					break;
				case B16(11010000,00000000):		// MOVB
					outputText(dest,"MOVB ");
					r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
					outputText(dest,r);
					if(workingTS==REGISTER_SYMBOLIC_INDEXED) {
						src++;
						Pipe3.x=*src;
						}
					outputComma(dest);
					r=getAddressing(workingTD,workingReg2Index,Pipe3.x);
					outputText(dest,r);
					if(workingTD==REGISTER_SYMBOLIC_INDEXED)
						src++;
					break;
				case B16(11100000,00000000):		// SOC
					outputText(dest,"SOC ");
					r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
					outputText(dest,r);
					if(workingTS==REGISTER_SYMBOLIC_INDEXED) {
						src++;
						Pipe3.x=*src;
						}
					outputComma(dest);
					r=getAddressing(workingTD,workingReg2Index,Pipe3.x);
					outputText(dest,r);
					if(workingTD==REGISTER_SYMBOLIC_INDEXED)
						src++;
					break;
				case B16(11110000,00000000):		// SOCB
					outputText(dest,"SOCB ");
					r=getAddressing(workingTS,workingRegIndex,Pipe2.x);
					outputText(dest,r);
					if(workingTS==REGISTER_SYMBOLIC_INDEXED) {
						src++;
						Pipe3.x=*src;
						}
					outputComma(dest);
					r=getAddressing(workingTD,workingReg2Index,Pipe3.x);
					outputText(dest,r);
					if(workingTD==REGISTER_SYMBOLIC_INDEXED)
						src++;
					break;
				default:
unknown:        
					outputUnknown(dest,Pipe1);
					break;

				}
        
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
				wsprintf(myBuf2,"%04X ",*oldsrc);
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


	}


