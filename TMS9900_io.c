#include <windows.h>
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>
#include <conio.h>
#include "tms9900win.h"

#pragma check_stack(off)
// #pragma check_pointer( off )
#pragma intrinsic( _enable, _disable )

// https://forums.atariage.com/topic/360111-grom-addressing-for-dummies-please/ 
// https://www.unige.ch/medecine/nouspikel/ti99/titechpages.htm

#define UNIMPLEMENTED_MEMORY_VALUE 0xFF


BYTE rom_seg[ROM_SIZE],rom_seg2[1024];



BYTE ram_seg[RAM_SIZE];
#ifdef RAM_SIZE2 
BYTE ram_seg2[RAM_SIZE2];
#endif
BYTE rom_seg[ROM_SIZE];			
#ifdef ROM_SIZE2 
BYTE rom_seg2[ROM_SIZE2];
#endif
BYTE grom_seg[GROM_SIZE];		// 3x	
WORD GROMPtr;
BYTE GROMWriteStage,GROMBuffer;
volatile BYTE TIMIRQ,VIDIRQ;
volatile WORD TIMEr;
BYTE TMS9918Reg[8],TMS9918RegS,TMS9918Sel,TMS9918WriteStage,TMS9918Buffer;
WORD TMS9918RAMPtr;
BYTE TMS9919[1];    // https://www.unige.ch/medecine/nouspikel/ti99/tms9919.htm
BYTE TMS9901[32];   // https://www.unige.ch/medecine/nouspikel/ti99/tms9901.htm
BYTE TMS5220[1];		// https://www.unige.ch/medecine/nouspikel/ti99/speech.htm
BYTE TMSVideoRAM[TMSVIDEORAM_SIZE];		// 
BYTE Keyboard[8];
extern BYTE CPUPins;
extern SWORD Pipe1;
extern union PIPE Pipe2;





uint8_t _fastcall GetValue(uint16_t t) {
	register uint8_t i;

	if(t < ROM_SIZE) {			//
		i=rom_seg[t];
		}
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
				switch(t & 0xfe) {
					case 0x00:		// VDP read data
						TMS9918WriteStage=0;
						i=TMS9918Buffer;
						TMS9918Buffer=TMSVideoRAM[(TMS9918RAMPtr++) & (TMSVIDEORAM_SIZE-1)];
						break;
					case 0x02:		// VDP read status register
						i=TMS9918RegS;
						TMS9918RegS &= 0x7f;
						TMS9918WriteStage=0;
						break;
					}
				break;
			case 0x8c:
				switch(t & 0xfe) {
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
						if(GROMPtr >= GROM_START && GROMPtr < (GROM_START+GROM_SIZE)) {   // 
							i=GROMBuffer;
							GROMBuffer = grom_seg[(GROMPtr++ - GROM_START) & (GROM_SIZE-1)];
							}
						else {
							i=UNIMPLEMENTED_MEMORY_VALUE;
							}
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
					case 0x04:		// GROM read address #2
						i=UNIMPLEMENTED_MEMORY_VALUE;
	/*					if(GROMPtr >= GROM_START && GROMPtr < (GROM_START+GROM_SIZE)) {   // 
							i=GROMBuffer;
							GROMBuffer = grom_seg[(GROMPtr++ - GROM_START) & (GROM_SIZE-1)];
							}
						else {
							i=UNIMPLEMENTED_MEMORY_VALUE;
							}*/
						GROMPtr++;
						GROMWriteStage=0;
						break;
					case 0x08:		// GROM read address #3
						i=UNIMPLEMENTED_MEMORY_VALUE;
						GROMPtr++;
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
					case 0x00:
						if(GROMPtr >= GROM_START && GROMPtr < (GROM_START+GROM_SIZE)) {   //  (non dovrebbe esistere
			//      i=grom_seg[t-GROM_START];
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
	uint16_t t2=t;

	if(t < ROM_SIZE) {			//
    t &= 0xfffe;
		i=MAKEWORD(rom_seg[t+1],rom_seg[t]);		// big-endian
		}
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

uint16_t _fastcall GetValueCRU(uint16_t r12,uint16_t t,uint8_t cnt) {

	t=t+r12;
	while(cnt--)
		;

/*					{char myBuf[128];
extern HFILE spoolFile;
					wsprintf(myBuf,"CRU get: r12=%04x, %04X: %02x\n",r12,t,cnt);
				_lwrite(spoolFile,myBuf,strlen(myBuf));
				}*/
	// kbd:
//CRU put: r12=0024, 0500: 00 03			// col
//CRU get: r12=0006, 0005: ff					// rows

	if(Keyboard[0] != 0xff) {
		return MAKEWORD(0xff,Keyboard[0]);
		}
	else
		return 0xffff;

/*	else if(t >= 0x1300 && t < 0x1400) {		// RS232/Timer (CRU?? https://www.unige.ch/medecine/nouspikel/ti99/cru.htm
		}
	else if(t >= 0x0000 && t < 0x0400) {		// Keyboard (CRU?? https://www.unige.ch/medecine/nouspikel/ti99/cru.htm
		}*/
	}

uint16_t _fastcall GetPipe(uint16_t t) {

	if(t < ROM_SIZE) {			//
		t &= 0xfffe;
	  Pipe1=MAKEWORD(rom_seg[t+1],rom_seg[t]);
		Pipe2.x=MAKEWORD(rom_seg[t+3],rom_seg[t+2]);
		}
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
			uint8_t sel=(t & 0x3e),chan=t1 >> 4,parm=t1 & 0xf;
			switch(sel) {
				case 0x0:		// sound				https://unige.ch/medecine/nouspikel/ti99/tms9919.htm
					TMS9919[0]=t1;
					if(chan & 1) {		// volume (9f bf df ff alla partenza
						}
					else {
						PlayResource(MAKEINTRESOURCE(IDR_WAVE_TONE1),FALSE);
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

/*				{char myBuf[128];
extern HFILE spoolFile;
					wsprintf(myBuf,"videoRAM write: %04X: %02x; GROM ptr=%04X\n",TMS9918RAMPtr,t1,GROMPtr);
				_lwrite(spoolFile,myBuf,strlen(myBuf));
				}*/
						TMSVideoRAM[(TMS9918RAMPtr++) & (TMSVIDEORAM_SIZE-1)]=t1;

						TMS9918Buffer = t1;
						break;
					case 0x02:		// VDP write register
						if(!TMS9918WriteStage) {   /* first stage byte - either an address LSB or a register value */
							TMS9918Sel = t1;
							TMS9918WriteStage = 1;
							}
						else {    /* second byte - either a register number or an address MSB */
							if(t1 & 0x80) { /* register */
				//          if((t1 & 0x7f) < 8)
								TMS9918Reg[t1 & 0x07] = TMS9918Sel;
								}
							else {  /* address */
								TMS9918RAMPtr = MAKEWORD(TMS9918Sel,t1 & 0x3f);
								if(!(t1 & 0x40)) {
									TMS9918Buffer = TMSVideoRAM[(TMS9918RAMPtr /*++*/) & (TMSVIDEORAM_SIZE-1)];
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
						if(GROMPtr >= GROM_START && GROMPtr < (GROM_START+GROM_SIZE)) {   // 
							GROMPtr++;
							// wrap...
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
						if(GROMPtr >= GROM_START && GROMPtr < (GROM_START+GROM_SIZE)) {   // 
							grom_seg[t-GROM_START]=t1;
							}
						GROMWriteStage=0;
						break;
					case 0x02:		// GROM set address
						if(!GROMWriteStage) {   // least significant byte goes first
							GROMPtr = MAKEWORD(LOBYTE(GROMPtr),t1);
							GROMWriteStage = 1;
							}
						else {    // https://forums.atariage.com/topic/360111-grom-addressing-for-dummies-please/
							GROMPtr = MAKEWORD(t1,HIBYTE(GROMPtr));
							GROMBuffer = grom_seg[(GROMPtr++ - GROM_START) & (GROM_SIZE-1)];
							GROMWriteStage = 0;
							}
						break;
					case 0x06:		// GROM set address #2
						if(!GROMWriteStage) {   // least significant byte goes first
							GROMPtr = MAKEWORD(LOBYTE(GROMPtr),t1);
							GROMWriteStage = 1;
							}
						else {    // https://forums.atariage.com/topic/360111-grom-addressing-for-dummies-please/
							GROMPtr = MAKEWORD(t1,HIBYTE(GROMPtr));
							GROMBuffer = grom_seg[(GROMPtr++ - GROM_START) & (GROM_SIZE-1)];
							GROMWriteStage = 0;
							}
						break;
					default:			// tutti gli altri ?!
						if(!GROMWriteStage) {   // least significant byte goes first
							GROMPtr = MAKEWORD(LOBYTE(GROMPtr),t1);
							GROMWriteStage = 1;
							}
						else {    // https://forums.atariage.com/topic/360111-grom-addressing-for-dummies-please/
							GROMPtr = MAKEWORD(t1,HIBYTE(GROMPtr));
							GROMBuffer = grom_seg[(GROMPtr++ - GROM_START) & (GROM_SIZE-1)];
							GROMWriteStage = 0;
							}
						GROMWriteStage = 0;
						break;
					}
				}
        break;
      break;
		}

	}

void _fastcall PutValueCRU(uint16_t r12,uint16_t t,uint8_t t1,uint8_t cnt) {

					/*{char myBuf[128];
extern HFILE spoolFile;
					wsprintf(myBuf,"CRU put: r12=%04x, %04X: %02x %02x\n",r12,t,t1,cnt);
				_lwrite(spoolFile,myBuf,strlen(myBuf));
				}*/
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


	t=t+r12;
	while(cnt--)
		;
	if(r12==0x30) {			// audio gate ... no?
		t=1;
		}
/*	else if(t >= 0x1300 && t < 0x1400) {		// RS232/Timer (CRU?? https://www.unige.ch/medecine/nouspikel/ti99/cru.htm
		}
	else if(t >= 0x0000 && t < 0x0400) {		// Keyboard (CRU?? https://www.unige.ch/medecine/nouspikel/ti99/cru.htm
		}*/
	}

void _fastcall PutIntValue(uint16_t t,uint16_t t1) {
	register uint16_t i;
	uint16_t t2=t;

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


void initHW(void) {
  int i;
extern const unsigned char charset_international[2048],tmsFont[(128-32)*8];
  struct SPRITE_ATTR *sa;

	GROMWriteStage=0;
	GROMPtr=0;

	Keyboard[0] = 0xff;

  
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

