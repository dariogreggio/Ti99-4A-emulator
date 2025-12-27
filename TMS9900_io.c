#include <windows.h>
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include "tms9900win.h"

#pragma check_stack(off)
// #pragma check_pointer( off )
#pragma intrinsic( _enable, _disable )

// https://forums.atariage.com/topic/360111-grom-addressing-for-dummies-please/ 
// https://www.unige.ch/medecine/nouspikel/ti99/titechpages.htm

#define UNIMPLEMENTED_MEMORY_VALUE 0xFF


BYTE rom_seg[ROM_SIZE];



BYTE ram_seg[RAM_SIZE];
#ifdef RAM_SIZE2 
BYTE ram_seg2[RAM_SIZE2];
#endif
BYTE rom_seg[ROM_SIZE];			
#ifdef ROM_SIZE2 
BYTE rom_seg2[ROM_SIZE2];
#endif
BYTE grom_seg[GROM_SIZE];		// 3x	
#ifdef GROM_SIZE2
BYTE grom_seg2[GROM_SIZE2];
BYTE rom_seg2[ROM_SIZE2];
#endif
WORD GROMPtr;
BYTE GROMWriteStage,GROMBuffer;
volatile BYTE TIMIRQ,VIDIRQ;
volatile WORD TIMEr;
BYTE TMS9918Reg[8],TMS9918RegS,TMS9918Sel,TMS9918WriteStage,TMS9918Buffer;
WORD TMS9918RAMPtr;
BYTE TMS9919[1],TMSvolume[4];
WORD TMSfreq[4];    // https://www.unige.ch/medecine/nouspikel/ti99/tms9919.htm
BYTE TMS9901[32];   // https://www.unige.ch/medecine/nouspikel/ti99/tms9901.htm
BYTE TMS5220[1];		// https://www.unige.ch/medecine/nouspikel/ti99/speech.htm
BYTE TMSVideoRAM[TMSVIDEORAM_SIZE];		// 
BYTE Keyboard[8],KeyboardCol=0;
extern BYTE CPUPins;
extern SWORD Pipe1;
extern union PIPE Pipe2;





uint8_t _fastcall GetValue(uint16_t t) {
	register uint8_t i;

#ifdef ROM_SIZE2
	if(t >= ROM_START2 && t < (ROM_START2+ROM_SIZE2)) 
		i=rom_seg2[t-ROM_START2];
	else if(t < ROM_SIZE) {			//
		i=rom_seg[t];
		}
#else
	if(t < ROM_SIZE) {			//
		i=rom_seg[t];
		}
#endif
	else if(t >= RAM_START && t < (RAM_START+RAM_SIZE*4)) {   // 256, mirrored (?)
		uint16_t t2;
		t2 = (t-RAM_START) & 0xff /* 0xfe*/;


/*      if(t == 0x072 || t == 0x073 || t == 0x074 ) {
				int T;
        T=0;
        }*/


		i=ram_seg[t2];
		}
	else 
		switch(t >> 8) {
			case 0x84:		// sound
				i=TMS9919[0];
				break;
			case 0x88:
				switch(t & 0x3e) {
					case 0x00:		// VDP read data
						TMS9918WriteStage=0;
						i=TMS9918Buffer;
						TMS9918Buffer=TMSVideoRAM[(TMS9918RAMPtr++) & (TMSVIDEORAM_SIZE-1)];
/*				{char myBuf[128];
extern HFILE spoolFile;
					wsprintf(myBuf,"videoRAM read: %04X: %02x; GROM ptr=%04X\n",TMS9918RAMPtr,i,GROMPtr);
				_lwrite(spoolFile,myBuf,strlen(myBuf));
				}*/
						break;
					case 0x02:		// VDP read status register
						i=TMS9918RegS;
						TMS9918RegS &= 0x7f;
						TMS9918WriteStage=0;
						break;
					}
				break;
			case 0x8c:
				switch(t & 0x3e) {
					case 0x00:		// VDP write data (non dovrebbe esistere
						TMS9918WriteStage = 0;
						break;
					case 0x02:				// VDP write register (non dovrebbe esistere
						TMS9918WriteStage = 0;
						break;
					}
				break;
			case 0x94:		// speech
				i=TMS5220[0];
				break;
			case 0x98:
				{
				uint8_t sel=(t & 0x3e);
				switch(sel) {
					case 0x00:		// GROM read page 0
						{   // 
							WORD n;
							i=GROMBuffer;
#ifdef GROM_SIZE2
							if(GROMPtr<0x6000)
								GROMBuffer = grom_seg[GROMPtr];
							else if(GROMPtr<0x6000+GROM_SIZE2)
								GROMBuffer = grom_seg2[GROMPtr-0x6000];
							else
								GROMBuffer = UNIMPLEMENTED_MEMORY_VALUE;
#else
							if(GROMPtr<0x6000)
								GROMBuffer = grom_seg[GROMPtr];
							else
								GROMBuffer = UNIMPLEMENTED_MEMORY_VALUE;
#endif
							n=GROMPtr & 0xe000;
							GROMPtr &= ~0xe000;
							GROMPtr=(GROMPtr+1-GROM_START) & (GROM_SIZE-1);
							GROMPtr |= n;
							}
/*						else {
							i=UNIMPLEMENTED_MEMORY_VALUE;
							}*/
						GROMWriteStage=0;
						break;
					case 0x02:		// GROM read address
					// da Classic99: address read is destructive;  Is the address incremented anyway? ie: if you keep reading, what do you get?
						i=HIBYTE(GROMPtr);		//uint8_t z=(GRMADD&0xff00)>>8;
						GROMPtr=MAKEWORD(LOBYTE(GROMPtr),LOBYTE(GROMPtr));		// 		GRMADD=(((GRMADD&0xff)<<8)|(GRMADD&0xff));
#if 0
						if(!GROMWriteStage) {   // least significant byte goes first
							i=HIBYTE(GROMPtr);		// big endian
							GROMWriteStage = 1;
							}
						else {    // https://forums.atariage.com/topic/360111-grom-addressing-for-dummies-please/ https://www.unige.ch/medecine/nouspikel/ti99/titechpages.htm
							i=LOBYTE(GROMPtr);		// big endian
							GROMWriteStage = 0;
							}
#endif
						break;
					case 0x04:		// GROM read page #1
						{   // 
							WORD n;
							i=GROMBuffer;
#ifdef GROM_SIZE2
							if(GROMPtr<0x6000)
								GROMBuffer = grom_seg[GROMPtr];
							else if(GROMPtr<0x6000+GROM_SIZE2)
								GROMBuffer = grom_seg2[GROMPtr-0x6000];
							else
								GROMBuffer = UNIMPLEMENTED_MEMORY_VALUE;
#else
							if(GROMPtr<0x6000)
								GROMBuffer = grom_seg[GROMPtr];
							else
								GROMBuffer = UNIMPLEMENTED_MEMORY_VALUE;
#endif
							n=GROMPtr & 0xe000;
							GROMPtr &= ~0xe000;
							GROMPtr=(GROMPtr+1-GROM_START) & (GROM_SIZE-1);
							GROMPtr |= n;
							}
						GROMWriteStage=0;
						break;
					case 0x06:		// GROM read address #1
					// da Classic99: address read is destructive;  Is the address incremented anyway? ie: if you keep reading, what do you get?
						i=HIBYTE(GROMPtr);		//uint8_t z=(GRMADD&0xff00)>>8;
						GROMPtr=MAKEWORD(LOBYTE(GROMPtr),LOBYTE(GROMPtr));		// 		GRMADD=(((GRMADD&0xff)<<8)|(GRMADD&0xff));
						break;
					case 0x08:		// GROM read address #3
						{   // 
							WORD n;
							i=GROMBuffer;
#ifdef GROM_SIZE2
							if(GROMPtr<0x6000)
								GROMBuffer = grom_seg[GROMPtr];
							else if(GROMPtr<0x6000+GROM_SIZE2)
								GROMBuffer = grom_seg2[GROMPtr-0x6000];
							else
								GROMBuffer = UNIMPLEMENTED_MEMORY_VALUE;
#else
							if(GROMPtr<0x6000)
								GROMBuffer = grom_seg[GROMPtr];
							else
								GROMBuffer = UNIMPLEMENTED_MEMORY_VALUE;
#endif
							n=GROMPtr & 0xe000;
							GROMPtr &= ~0xe000;
							GROMPtr=(GROMPtr+1-GROM_START) & (GROM_SIZE-1);
							GROMPtr |= n;
							}
						GROMWriteStage=0;
						break;
					case 0x20:		// GROM read page 1			VERIFICARE! questa arriva ma pare errore, 9b9b (è solo per GramKarte, dice
	//				case 0x40:		// GROM read page 2			VERIFICARE!
	//    case 0x60:
	//    case 0x80:
	//    case 0xa0:
					default:		// beh li controlla tutti da 04 a 3c in step da 4, come dice il doc
						i=UNIMPLEMENTED_MEMORY_VALUE;
						break;
					}
				}
				break;
			case 0x9c:		// GROM write data (non dovrebbe esistere
				switch(t & 0xfe) {
					case 0x00:						//  (non dovrebbe esistere
						{   // 
						WORD n;
#ifdef GROM_SIZE2
							if(GROMPtr<0x6000)
								GROMBuffer = grom_seg[GROMPtr];
							else if(GROMPtr<0x6000+GROM_SIZE2)
								GROMBuffer = grom_seg2[GROMPtr-0x6000];
							else
								GROMBuffer = UNIMPLEMENTED_MEMORY_VALUE;
#else
							if(GROMPtr<0x6000)
								GROMBuffer = grom_seg[GROMPtr];
							else
								GROMBuffer = UNIMPLEMENTED_MEMORY_VALUE;
#endif
						n=GROMPtr & 0xe000;
						GROMPtr &= ~0xe000;
						GROMPtr=(GROMPtr+1-GROM_START) & (GROM_SIZE-1);
						GROMPtr |= n;
						}
						GROMWriteStage=0;
						break;
					case 0x02:		// GROM set address (non dovrebbe esistere
						GROMWriteStage=0;
						break;
					default:		// 
						GROMWriteStage=0;
						break;
					}
				break;
			}

	return i;
	}

uint16_t _fastcall GetIntValue(uint16_t t) {
	register uint16_t i;

#ifdef ROM_SIZE2
	if(t >= ROM_START2 && t < (ROM_START2+ROM_SIZE2)) {
    t &= 0xfffe;
		t -= ROM_START2;
		i=MAKEWORD(rom_seg2[t+1],rom_seg2[t]);		// big-endian
		}
	else 	if(t < ROM_SIZE) {			//
    t &= 0xfffe;
		i=MAKEWORD(rom_seg[t+1],rom_seg[t]);		// big-endian
		}
#else
	if(t < ROM_SIZE) {			//
    t &= 0xfffe;
		i=MAKEWORD(rom_seg[t+1],rom_seg[t]);		// big-endian
		}
#endif
	else if(t >= RAM_START && t < (RAM_START+RAM_SIZE*4)) {   // 256 bytes, mirrored (?)
		t-=RAM_START;


/*      if(t == 0x072 || t == 0x073 || t == 0x074 ) {
				int T;
        T=0;
        }*/


    t &= 0xfe /*0xfe*/;
		i=MAKEWORD(ram_seg[t+1],ram_seg[t]);		// big-endian
		}

	return i;
	}

uint16_t _fastcall GetValueCRU(uint16_t r12,uint8_t cnt) {
	uint8_t i,t;

#ifdef _DEBUG
/*					{char myBuf[128];
extern HFILE spoolFile;
					wsprintf(myBuf,"CRU  get: r12=%04x, %02x   sel=%u\n",r12,cnt,KeyboardCol);
				_lwrite(spoolFile,myBuf,strlen(myBuf));
				}*/
#endif


	// kbd:
//CRU put: r12=0024, 0500: 00 03			// col
//CRU get: r12=0006, 0005: ff					// rows

	if(!cnt)
		cnt=16;
	if(r12>=6 && r12<=0x14) {
//			if(( m_CapsLock == false ) && ( address == 7 ))		{
		 //((GetKeyState(VK_CAPITAL) & 0x0001)!=0)
//			return 1;
//		}
		t=0xff;
		for(i=1; i<=cnt; i++) {			// cnt = 1..8 qua (direi
			t >>= 1;
			t |= 0x80;

			if(!(Keyboard[r12/2-3 /*6..14hex*/  +i-1] & (1 << (7-KeyboardCol))))
				t &= ~0x80;
			}
		for(i; i<=8; i++) {
			t >>= 1;
			t |= 0x80;
			}
		return MAKEWORD(0xff,t);
		}
	else if(r12==0x24) {
		return MAKEWORD(0xff,KeyboardCol);
		}
	else if(r12==0x00) {		// ?? vertical sync dice, in IRQ, e cnt=2
		return 0x00;
		}

	return 0xff;


/*	else if(t >= 0x1300 && t < 0x1400) {		// RS232/Timer (CRU?? https://www.unige.ch/medecine/nouspikel/ti99/cru.htm
		}
	else if(t >= 0x0000 && t < 0x0400) {		// Keyboard (CRU?? https://www.unige.ch/medecine/nouspikel/ti99/cru.htm
		}*/
	}

uint16_t _fastcall GetPipe(uint16_t t) {

#ifdef ROM_SIZE2
	if(t >= ROM_START2 && t < (ROM_START2+ROM_SIZE2)) {
    t &= 0xfffe;
		t -= ROM_START2;
	  Pipe1=MAKEWORD(rom_seg2[t+1],rom_seg2[t]);
		Pipe2.x=MAKEWORD(rom_seg2[t+3],rom_seg2[t+2]);
		}
	else if(t < ROM_SIZE) {			//
		t &= 0xfffe;
	  Pipe1=MAKEWORD(rom_seg[t+1],rom_seg[t]);
		Pipe2.x=MAKEWORD(rom_seg[t+3],rom_seg[t+2]);
		}
#else
	if(t < ROM_SIZE) {			//
		t &= 0xfffe;
	  Pipe1=MAKEWORD(rom_seg[t+1],rom_seg[t]);
		Pipe2.x=MAKEWORD(rom_seg[t+3],rom_seg[t+2]);
		}
#endif
	else if(t >= RAM_START && t < (RAM_START+RAM_SIZE*4)) {   // 256, mirrored (?) NON SONO SICURO QUA ABBIA SENSO!
		t-=RAM_START;
		t &= 0xfe /*0xfe*/;
	  Pipe1=MAKEWORD(ram_seg[t+1],ram_seg[t]);
		Pipe2.x=MAKEWORD(ram_seg[t+3],ram_seg[t+2]);
		}

	return Pipe1;
	}

void _fastcall PutValue(uint16_t t,uint8_t t1) {

// printf("rom_seg: %04x, p: %04x\n",rom_seg,p);

	if(t >= RAM_START && t < (RAM_START+RAM_SIZE*4)) {   // 256, mirrored  (?)
		uint16_t t2;
		t2 = (t-RAM_START) & 0xff /*0xfe*/;
		ram_seg[t2]=t1;


		/*
      if(t == 0x072 || t == 0x073 || t == 0x074 ) {
				int T;
        T=0;
        }*/


		}
	else 
    switch(t >> 8) {
      case 0x84:
			{
			uint8_t sel=(t & 0x3e),chan=(t1 >> 5) & 3;
			switch(sel) {
				case 0x0:		// sound				https://unige.ch/medecine/nouspikel/ti99/tms9919.htm
					TMS9919[0]=t1;
					if(t1 & 0x80) {		// comando
						if(t1 & 0x10) {		// volume (9f bf df ff alla partenza
							TMSvolume[chan]=t1 & 0xf;
							}
						else {			// parte bassa freq
							TMSfreq[chan]=t1 & 0xf;
							}
						}
					else {				// 2° parte freq
						if(t1==0x20)		// patch brutale! sarebbe il secondo parametro: bf df ff  80 05 92
							PlayResource(MAKEINTRESOURCE(IDR_WAVE_TONE1),FALSE);
							
						else if(t1==0x05)
							PlayResource(MAKEINTRESOURCE(IDR_WAVE_TONE2),FALSE);
							
						TMSfreq[chan] |= (t1 & 0x3f) << 4;
/*						if(TMSfreq[chan])			// safety
							playTone(0.3, sine_generator, 223700L/4/TMSfreq[chan], 22050, 
								100-(TMSvolume[chan]*6), TRUE);		// diceva /2 ... SISTEMARE PLAY è ciucca di *2
								*/
						}
/*
Generator 	Frequency 	Volume
Tone 1 			>8z >xy 		>9v
Tone 2 			>Az >yx 		>Bv
Tone 2 			>Cz >yx 		>Dv
Noise 			>En 				>Fv

Frequency = 111860.8 Hz / xyz
Volume v:  +1 = -2 dB (>F = off)
*/
#ifdef _DEBUG
				{char myBuf[128];
extern HFILE spoolFile;
					wsprintf(myBuf,"sound write: %02x\n",t1);
				_lwrite(spoolFile,myBuf,strlen(myBuf));
				}
#endif
	        break;
				}
				}
        break;
      case 0x88:		// VDP read data (non dovrebbe esistere
				{
				uint8_t sel=(t & 0x3e);
				switch(sel) {
		      case 0x00:		// VDP read data (non dovrebbe esistere
				    TMS9918WriteStage = 0;
		        break;
					case 0x02:		// VDP read status register (non dovrebbe esistere
						TMS9918WriteStage = 0;
						break;
					}
					break;
				}
				break;
      case 0x8c:	// VDP write data
				{
				uint8_t sel=(t & 0x3e);
				switch(sel) {
					case 0x00:	// VDP write data
						TMS9918WriteStage=0;

#ifdef _DEBUG
/*				{char myBuf[128];
extern HFILE spoolFile;
					wsprintf(myBuf,"videoRAM write: %04X: %02x; GROM ptr=%04X\n",TMS9918RAMPtr,t1,GROMPtr);
				_lwrite(spoolFile,myBuf,strlen(myBuf));
				}*/
#endif
						TMSVideoRAM[(TMS9918RAMPtr++) & (TMSVIDEORAM_SIZE-1)]=t1;
						break;
					case 0x02:		// VDP write register
#ifdef _DEBUG
/*				{char myBuf[128];
extern HFILE spoolFile;
					wsprintf(myBuf,"video write: %02X,%u: %02x; GROM ptr=%04X\n",TMS9918Sel,TMS9918WriteStage,t1,GROMPtr);
				_lwrite(spoolFile,myBuf,strlen(myBuf));
				}*/
#endif
						if(!TMS9918WriteStage) {   /* first stage byte - either an address LSB or a register value */
							TMS9918Sel = t1;
							TMS9918WriteStage = 1;
							}
						else {    /* second byte - either a register number or an address MSB */
							if(t1 & 0x80) { /* register */
								TMS9918Reg[t1 & 0x07] = TMS9918Sel;
/*if((t1 & 7) ==1)	*/		/*	{char myBuf[128];
extern HFILE spoolFile;
extern SWORD _pc;
					wsprintf(myBuf,"videoREG[%u]   write: %04X: %02x; GROM ptr=%04X\n",t1,_pc,TMS9918Sel,GROMPtr);
				_lwrite(spoolFile,myBuf,strlen(myBuf));
				}*/
								}
							else {  /* address */
								TMS9918RAMPtr = MAKEWORD(TMS9918Sel,t1 & 0x3f);
								if(!(t1 & 0x40)) {
									TMS9918Buffer = TMSVideoRAM[(TMS9918RAMPtr++) & (TMSVIDEORAM_SIZE-1)];
									}
								}
							TMS9918WriteStage = 0;
							} 
		        break;
					} 
				} 
        break;
      case 0x94:		// speech
        TMS5220[0]=t1;
        break;
			case 0x98:
				{
				uint8_t sel=(t & 0x3e);
				switch(sel) {
					case 0x00:		// GROM read page 0 (non dovrebbe esistere
					case 0x20:		// GROM read page 1 (non dovrebbe esistere
						{   // 
						WORD n;
//						i=GROMBuffer;
#ifdef GROM_SIZE2
							if(GROMPtr<0x6000)
								GROMBuffer = grom_seg[GROMPtr];
							else if(GROMPtr<0x6000+GROM_SIZE2)
								GROMBuffer = grom_seg2[GROMPtr-0x6000];
							else
								GROMBuffer = UNIMPLEMENTED_MEMORY_VALUE;
#else
							if(GROMPtr<0x6000)
								GROMBuffer = grom_seg[GROMPtr];
							else
								GROMBuffer = UNIMPLEMENTED_MEMORY_VALUE;
#endif
						n=GROMPtr & 0xe000;
						GROMPtr &= ~0xe000;
						GROMPtr=(GROMPtr+1-GROM_START) & (GROM_SIZE-1);
						GROMPtr |= n;
						}
						GROMWriteStage=0;
			      break;
		      case 0x02:		// GROM read address (non dovrebbe esistere
					  GROMWriteStage=0;
						break;
					}
				}
        break;
      case 0x9c:		// GROM write data (ev. GRAM, dice
				{
				uint8_t sel=(t & 0x3e);
				switch(sel) {
					case 0x00:		// GROM write data (ev. GRAM, dice
						{   // 
						WORD n;
						grom_seg[GROMPtr-GROM_START]=t1;
						n=GROMPtr & 0xe000;
						GROMPtr &= ~0xe000;
						GROMPtr=(GROMPtr+1-GROM_START) & (GROM_SIZE-1);
						GROMPtr |= n;
						}
						GROMWriteStage=0;
						break;
					case 0x02:		// GROM set address
						if(!GROMWriteStage) {   // least significant byte goes first
							GROMPtr = MAKEWORD(LOBYTE(GROMPtr),t1);
							GROMWriteStage = 1;
							}
						else {    // https://forums.atariage.com/topic/360111-grom-addressing-for-dummies-please/
							WORD n;
							GROMPtr = MAKEWORD(t1,HIBYTE(GROMPtr));
#ifdef GROM_SIZE2
							if(GROMPtr<0x6000)
								GROMBuffer = grom_seg[GROMPtr];
							else if(GROMPtr<0x6000+GROM_SIZE2)
								GROMBuffer = grom_seg2[GROMPtr-0x6000];
							else
								GROMBuffer = UNIMPLEMENTED_MEMORY_VALUE;
#else
							if(GROMPtr<0x6000)
								GROMBuffer = grom_seg[GROMPtr];
							else
								GROMBuffer = UNIMPLEMENTED_MEMORY_VALUE;
#endif
							n=GROMPtr & 0xe000;
							GROMPtr &= ~0xe000;
							GROMPtr=(GROMPtr+1-GROM_START) & (GROM_SIZE-1);
							GROMPtr |= n;
							GROMWriteStage = 0;
							}
						break;
					case 0x06:		// GROM set address #2
						if(!GROMWriteStage) {   // least significant byte goes first
							GROMPtr = MAKEWORD(LOBYTE(GROMPtr),t1);
							GROMWriteStage = 1;
							}
						else {    // https://forums.atariage.com/topic/360111-grom-addressing-for-dummies-please/
							WORD n;
							GROMPtr = MAKEWORD(t1,HIBYTE(GROMPtr));
#ifdef GROM_SIZE2
							if(GROMPtr<0x6000)
								GROMBuffer = grom_seg[GROMPtr];
							else if(GROMPtr<0x6000+GROM_SIZE2)
								GROMBuffer = grom_seg2[GROMPtr-0x6000];
							else
								GROMBuffer = UNIMPLEMENTED_MEMORY_VALUE;
#else
							if(GROMPtr<0x6000)
								GROMBuffer = grom_seg[GROMPtr];
							else
								GROMBuffer = UNIMPLEMENTED_MEMORY_VALUE;
#endif
							n=GROMPtr & 0xe000;
							GROMPtr &= ~0xe000;
							GROMPtr=(GROMPtr+1-GROM_START) & (GROM_SIZE-1);
							GROMPtr |= n;
							GROMWriteStage = 0;
							}
						break;
					default:			// tutti gli altri ?!
						if(!GROMWriteStage) {   // least significant byte goes first
							GROMPtr = MAKEWORD(LOBYTE(GROMPtr),t1);
							GROMWriteStage = 1;
							}
						else {    // https://forums.atariage.com/topic/360111-grom-addressing-for-dummies-please/
							WORD n;
							GROMPtr = MAKEWORD(t1,HIBYTE(GROMPtr));
#ifdef GROM_SIZE2
							if(GROMPtr<0x6000)
								GROMBuffer = grom_seg[GROMPtr];
							else if(GROMPtr<0x6000+GROM_SIZE2)
								GROMBuffer = grom_seg2[GROMPtr-0x6000];
							else
								GROMBuffer = UNIMPLEMENTED_MEMORY_VALUE;
#else
							if(GROMPtr<0x6000)
								GROMBuffer = grom_seg[GROMPtr];
							else
								GROMBuffer = UNIMPLEMENTED_MEMORY_VALUE;
#endif
							n=GROMPtr & 0xe000;
							GROMPtr &= ~0xe000;
							GROMPtr=(GROMPtr+1-GROM_START) & (GROM_SIZE-1);
							GROMPtr |= n;
							GROMWriteStage = 0;
							}
						break;
					}
				}
        break;
      break;
		}

	}

void _fastcall PutValueCRU(uint16_t r12,uint16_t t,uint8_t cnt) {

#ifdef _DEBUG
/*					{char myBuf[128];
extern HFILE spoolFile;
					wsprintf(myBuf,"CRU put: r12=%04x, %04x %02x\n",r12,t,cnt);
				_lwrite(spoolFile,myBuf,strlen(myBuf));
				}*/
#endif
//	al boot:
//CRU put: r12=0006, 0000: 00 08			// kbd
//CRU put: r12=0020, 0000: 00 01			// ?? n.c.
//CRU put: r12=0030, 0000: 00 01			// audio
//CRU put: r12=0004, FF00: 00 01			// VDU irq
//CRU put: r12=0002, FF00: 00 01			// periph irq
//CRU put: r12=002c, FF00: 00 02			// CS1 motor

	// kbd:
//CRU put: r12=0024, 0500: 00 03			// col
//CRU get: r12=0006, 0005: ff					// rows


	if(r12==0x30) {			// audio gate ... no?
		t=1;
		}
	if(r12==0x24) {			// tastiera
		KeyboardCol=HIBYTE(t);		// cnt=3 per 3 bit, ma ok ignoro
		}
/*	else if(t >= 0x1300 && t < 0x1400) {		// RS232/Timer (CRU?? https://www.unige.ch/medecine/nouspikel/ti99/cru.htm
		}
	else if(t >= 0x0000 && t < 0x0400) {		// Keyboard (CRU?? https://www.unige.ch/medecine/nouspikel/ti99/cru.htm
		}*/
	if(r12==0x00) {			// tastiera?? cos'è? è a inizio SCAN keyboard con 21: potrebbe essere caps-lock (da Classic99
//				m_CapsLock = ( data != 0 ) ? true : false;
		 //((GetKeyState(VK_CAPITAL) & 0x0001)!=0)

		}
	}

void _fastcall PutIntValue(uint16_t t,uint16_t t1) {
	register uint16_t i;

// printf("rom_seg: %04x, p: %04x\n",rom_seg,p);

	if(t >= RAM_START && t < (RAM_START+RAM_SIZE*4)) {   // 256 bytes, mirrored (?)
		t-=RAM_START;
		t &= 0xfe /*0xfe*/;
	  ram_seg[t++]=HIBYTE(t1);			// big-endian 
	  ram_seg[t]  =LOBYTE(t1);
		}

  }



BOOL PlayResource(LPSTR lpName,BOOL bStop) { 
  BOOL bRtn; 
  LPSTR lpRes; 
  HANDLE hResInfo, hRes; 

	if(!lpName) {
    sndPlaySound(NULL, 0);
		return TRUE;
		}

  // Find the WAVE resource.  
  hResInfo = FindResource(g_hinst, lpName, "WAVE"); 
  if(hResInfo == NULL) 
    return FALSE; 

  // Load the WAVE resource. 
  hRes = LoadResource(g_hinst, hResInfo); 
  if(hRes == NULL) 
    return FALSE; 

  // Lock the WAVE resource and play it. 
  lpRes = LockResource(hRes); 
  if(lpRes != NULL) { 
    bRtn = sndPlaySound(lpRes, SND_MEMORY | SND_ASYNC | SND_NODEFAULT | (bStop ? 0 : SND_NOSTOP));
    UnlockResource(hRes); 
		} 
  else 
    bRtn = 0; 
 
  // Free the WAVE resource and return success or failure. 
 
  FreeResource(hRes); 
  return bRtn; 
	}


MMRESULT playTone(float nSeconds, generator_type signal, uint16_t context, uint32_t samplesPerSecond, 
									uint8_t volume,uint8_t bWait) {
	// https://stackoverflow.com/questions/5814869/playing-an-arbitrary-sound-on-windows
  UINT timePeriod = 1;
	size_t i;
	unsigned short j;
  /*const*/ size_t nBuffer;
  uint8_t *buffer;

  MMRESULT mmresult = MMSYSERR_NOERROR;
  WAVEFORMATEX waveFormat = {0};
  waveFormat.cbSize = 0;
  waveFormat.wFormatTag = WAVE_FORMAT_PCM /*WAVE_FORMAT_IEEE_FLOAT*/;
  waveFormat.wBitsPerSample = CHAR_BIT*2;  // ?? CHAR_BIT * sizeof(buffer[0]);
  waveFormat.nChannels = 2;
  waveFormat.nSamplesPerSec = samplesPerSecond;
  waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / CHAR_BIT;
  waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
  nBuffer = (size_t)(nSeconds * waveFormat.nChannels * waveFormat.nSamplesPerSec);


  buffer = (uint16_t *)calloc(nBuffer, sizeof(*buffer));
  __try {
    HWAVEOUT hWavOut = NULL;
    for(i=0; i < nBuffer; i += waveFormat.nChannels)
      for(j=0; j < waveFormat.nChannels; j++)
        buffer[i+j] = (*signal)((i+j) * nSeconds / nBuffer, j, context, volume);
    mmresult = waveOutOpen(&hWavOut, WAVE_MAPPER, &waveFormat, 0, 0, CALLBACK_NULL);
    if(mmresult == MMSYSERR_NOERROR) {
      __try {
        timeBeginPeriod(timePeriod);
        __try {
          WAVEHDR hdr = {0};
          hdr.dwBufferLength = (ULONG)(nBuffer * sizeof(buffer[0]));
          hdr.lpData = (LPSTR)&buffer[0];
          mmresult = waveOutPrepareHeader(hWavOut,&hdr,sizeof(hdr));
          if(mmresult == MMSYSERR_NOERROR) {
            __try {
              ULONG start = GetTickCount();
              mmresult = waveOutWrite(hWavOut, &hdr, sizeof(hdr));
							if(bWait)			// ovviamente non va così... bisogna fare asincrona...
								Sleep((ULONG)(1000 * nSeconds - (GetTickCount() - start)));
              }
            __finally { 
							waveOutUnprepareHeader(hWavOut, &hdr, sizeof(hdr)); }
              }
            }
          __finally { 
						timeEndPeriod(timePeriod); 
						}
          }
        __finally { 
					waveOutClose(hWavOut); 
					}
			}
		}
	__finally { 
		free(buffer); 
		}
  return mmresult;
	}

// Triangle wave generator
uint16_t triangle_generator(float timeInSeconds, uint8_t channel, uint16_t context) {
  const uint16_t frequency = context;
  const float angle = (float)(frequency * 2 * PI * timeInSeconds);

  switch(channel) {
    case 0: 
			return asin(sin(angle + 0 * PI / 2))*0x7f;
			break;
//return (float)asin(sin(angle + 0 * M_PI / 2)) * 2 / M_PI;.
    default:
			return asin(sin(angle + 1 * PI / 2))*0x7f;
			break;
    }
	}

// Sine tone generator
uint16_t sine_generator(float timeInSeconds, uint8_t channel, uint16_t context) {
  const uint16_t frequency = context;
  const float angle = (float)(frequency * 2 * PI * timeInSeconds);

  switch(channel) {
    case 0: 
			return sin(angle + 0 * PI / 2)*0x7f;
			break;
    default:
			return sin(angle + 1 * PI / 2)*0x7f;
			break;
		}
	}


void initHW(void) {
  int i;
extern const unsigned char charset_international[2048],tmsFont[(128-32)*8];
  struct SPRITE_ATTR *sa;

	GROMWriteStage=0;
	GROMPtr=0;

	memset(Keyboard,0xff,sizeof(Keyboard));
	KeyboardCol=0;

  
	TMS9919[0]=B8(00000000);
  TMS9901[0]=0;
  TMS5220[0]=0;
  
  memset(TMSVideoRAM,0,TMSVIDEORAM_SIZE);    // mah...
// Ti99 dice:	The real 9918A will set all VRs to 0, which basically makes the screen black, blank, and off, 4K VRAM selected, and no interrupts. 
  TMS9918Reg[0]=TMS_R0_EXT_VDP_DISABLE | TMS_R0_MODE_GRAPHICS_I;
  TMS9918Reg[1]=TMS_R1_RAM_16K | TMS_R1_MODE_GRAPHICS_I /* bah   | TMS_R1_DISP_ACTIVE | TMS_R1_INT_ENABLE*/;
  TMS9918Reg[2]=TMS_DEFAULT_VRAM_NAME_ADDRESS >> 10;
  TMS9918Reg[3]=TMS_DEFAULT_VRAM_COLOR_ADDRESS >> 6;
  TMS9918Reg[4]=TMS_DEFAULT_VRAM_PATT_ADDRESS >> 11;
  TMS9918Reg[5]=TMS_DEFAULT_VRAM_SPRITE_ATTR_ADDRESS >> 7;
  TMS9918Reg[6]=TMS_DEFAULT_VRAM_SPRITE_PATT_ADDRESS >> 11;
  TMS9918Reg[7]=(1 /*black*/ << 4) | 15 /*bianco*/;		//(1 /*black*/ << 4) | 7 /*cyan*/;
  TMS9918RegS=0;
  TMS9918Sel=TMS9918WriteStage=0;
//  memcpy(TMSVideoRAM+TMS_DEFAULT_VRAM_PATT_ADDRESS,charset_international,2048);// mah... non serve
//  memcpy(TMSVideoRAM+TMS_DEFAULT_VRAM_PATT_ADDRESS,tmsFont,(128-32)*8);
  sa=(struct SPRITE_ATTR *)&TMSVideoRAM[TMS_DEFAULT_VRAM_SPRITE_ATTR_ADDRESS];
  for(i=0; i<32; i++) {
//    TMSVideoRAM[TMS_DEFAULT_VRAM_SPRITE_ATTR_ADDRESS+i*4]=LAST_SPRITE_YPOS;
    sa->ypos=LAST_SPRITE_YPOS;
    sa->xpos=sa->tag=sa->name=0;
//    TMSVideoRAM[TMS_DEFAULT_VRAM_SPRITE_ATTR_ADDRESS+i*4+1]=0;
//    TMSVideoRAM[TMS_DEFAULT_VRAM_SPRITE_ATTR_ADDRESS+i*4+2]=0;
//    TMSVideoRAM[TMS_DEFAULT_VRAM_SPRITE_ATTR_ADDRESS+i*4+3]=0;
    sa++;
    }

//  for(i=0; i<768; i++)		non serve idem
//    TMSVideoRAM[TMS_DEFAULT_VRAM_NAME_ADDRESS+i]=i & 0xff;
  
  
//  OC1CONbits.ON = 0;   // spengo buzzer/audio

  }

