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
BYTE TMS5220[64];		// https://www.unige.ch/medecine/nouspikel/ti99/speech.htm
BYTE TMSVideoRAM[TMSVIDEORAM_SIZE];		// 
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
		t2 = (t-RAM_START) & /*0xff*/ 0xfe;


      if(t == 0x072 || t == 0x073 || t == 0x074 ) {
				int T;
        T=0;
        }


		if(!(t & 1))			// patch big-endian. ev. cambiare
			i=ram_seg[t2+1];
		else
			i=ram_seg[t2];
//		i=ram_seg[t2];
		}
	else switch(t) {
    case 0x8400:		// sound
      i=TMS9919[0];
      break;
    case 0x8800:		// VDP read data
      TMS9918WriteStage=0;
      i=TMS9918Buffer;
      TMS9918Buffer=TMSVideoRAM[(TMS9918RAMPtr++) & (TMSVIDEORAM_SIZE-1)];
      TMS9918WriteStage = 0;
      break;
    case 0x8802:		// VDP read status register
      i=TMS9918RegS;
      TMS9918RegS &= 0x7f;
      TMS9918WriteStage=0;
      break;
    case 0x8c00:		// VDP write data (non dovrebbe esistere
      TMS9918WriteStage = 0;
      break;
    case 0x8c02:		// VDP write register (non dovrebbe esistere
      TMS9918WriteStage = 0;
      break;
    case 0x9400:		// speech
      i=TMS5220[t & 0x3f];
      break;
    case 0x9800:		// GROM read page 0
    case 0x9820:		// GROM read page 1			VERIFICARE!
    case 0x9840:		// GROM read page 2			VERIFICARE!
      if(GROMPtr >= GROM_START && GROMPtr < (GROM_START+GROM_SIZE)) {   // 
//        i=GROMBuffer;
        i=grom_seg[GROMPtr-GROM_START];
        GROMPtr++;
        }
//			else {
//				i=UNIMPLEMENTED_MEMORY;
//				}
      GROMWriteStage=0;
      break;
    case 0x9802:		// GROM read address
      if(!GROMWriteStage) {   // least significant byte goes first
		    i=HIBYTE(GROMPtr);		// big endian
//         GROMWriteStage = 1;
        }
      else {    // https://forums.atariage.com/topic/360111-grom-addressing-for-dummies-please/ https://www.unige.ch/medecine/nouspikel/ti99/titechpages.htm
		    i=LOBYTE(GROMPtr);		// big endian
//          GROMWriteStage = 0;
	      }
      break;
    case 0x9c00:		// GROM write data
      if(GROMPtr >= GROM_START && GROMPtr < (GROM_START+GROM_SIZE)) {   //  (non dovrebbe esistere
//      i=grom_seg[t-GROM_START];
        }
      GROMWriteStage;
      break;
    case 0x9c02:		// GROM set address (non dovrebbe esistere
      GROMWriteStage;
      break;
		}

	return i;
	}

uint16_t _fastcall GetIntValue(uint16_t t) {
	register uint16_t i;

	if(t < ROM_SIZE) {			//
    t &= 0xfffe;
		i=MAKEWORD(rom_seg[t+1],rom_seg[t]);		// big-endian
		}
	else if(t >= RAM_START && t < (RAM_START+RAM_SIZE*4)) {   // 256 bytes, mirrored (?)
		t-=RAM_START;


      if(t == 0x072 || t == 0x073 || t == 0x074 ) {
				int T;
        T=0;
        }


    t &= 0xfe;
		i=MAKEWORD(ram_seg[t+1],ram_seg[t]);		// big-endian
		}

	return i;
	}

uint8_t _fastcall GetValueCRU(uint16_t t) {
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
		t &= 0xfe;
	  Pipe1=MAKEWORD(ram_seg[t+1],ram_seg[t]);
		Pipe2.x=MAKEWORD(ram_seg[t+3],ram_seg[t+2]);
		}

	return Pipe1;
	}

void _fastcall PutValue(uint16_t t,uint8_t t1) {

// printf("rom_seg: %04x, p: %04x\n",rom_seg,p);

	if(t >= RAM_START && t < (RAM_START+RAM_SIZE*4)) {   // 256, mirrored  (?)
		uint16_t t2;
		t2 = (t-RAM_START) & 0xfe;
		if(!(t & 1))			// patch big-endian. ev. cambiare
			ram_seg[t2+1]=t1;
		else
			ram_seg[t2]=t1;
/*		ram_seg[t2]=t1;
		ram_seg[t2+1]=t1;*/

      if(t == 0x072 || t == 0x073 || t == 0x074 ) {
				int T;
        T=0;
        }


		}
	else 
    switch(t) {
      case 0x8400:		// sound
        TMS9919[0]=t1;
        break;
      case 0x8800:		// VDP read data (non dovrebbe esistere
        TMS9918WriteStage = 0;
        break;
      case 0x8802:		// VDP read status register (non dovrebbe esistere
        TMS9918WriteStage = 0;
        break;
      case 0x8c00:	// VDP write data
        TMS9918WriteStage=0;
        TMSVideoRAM[(TMS9918RAMPtr++) & (TMSVIDEORAM_SIZE-1)]=t1;
        TMS9918Buffer = t1;
        TMS9918WriteStage = 0;
        break;
      case 0x8c02:		// VDP write register
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
            TMS9918RAMPtr = TMS9918Sel | ((t1 & 0x3f) << 8);
            if(!(t1 & 0x40)) {
              TMS9918Buffer = TMSVideoRAM[(TMS9918RAMPtr++) & (TMSVIDEORAM_SIZE-1)];
              }
            }
          TMS9918WriteStage = 0;
          } 
        break;
      case 0x9400:		// speech
        TMS5220[t & 0x3f]=t1;
        break;
      case 0x9800:		// GROM read page 0 (non dovrebbe esistere
	    case 0x9820:		// GROM read page 1 (non dovrebbe esistere
        if(GROMPtr >= GROM_START && GROMPtr < (GROM_START+GROM_SIZE)) {   // 
          }
        GROMWriteStage;
        break;
      case 0x9802:		// GROM read address (non dovrebbe esistere
        GROMWriteStage;
        break;
      case 0x9c00:		// GROM write data
        if(GROMPtr >= GROM_START && GROMPtr < (GROM_START+GROM_SIZE)) {   // 
					grom_seg[t-GROM_START]=t1;
          }
        GROMWriteStage=0;
        break;
      case 0x9c02:		// GROM set address
        if(!GROMWriteStage) {   // least significant byte goes first
          GROMPtr = (GROMPtr) | (((uint16_t)t1) << 8);
          GROMWriteStage = 1;
          }
        else {    // https://forums.atariage.com/topic/360111-grom-addressing-for-dummies-please/
          GROMPtr = t1;
//          GROMBuffer = grom_seg[(GROMPtr++) & (GROM_SIZE-1)];
          GROMWriteStage = 0;
	        }
        break;
      break;
		}

	}

void _fastcall PutValueCRU(uint16_t t,uint8_t t1) {
/*	else if(t >= 0x1300 && t < 0x1400) {		// RS232/Timer (CRU?? https://www.unige.ch/medecine/nouspikel/ti99/cru.htm
		}
	else if(t >= 0x0000 && t < 0x0400) {		// Keyboard (CRU?? https://www.unige.ch/medecine/nouspikel/ti99/cru.htm
		}*/
	}

void _fastcall PutIntValue(uint16_t t,uint16_t t1) {
	register uint16_t i;

// printf("rom_seg: %04x, p: %04x\n",rom_seg,p);

	if(t >= RAM_START && t < (RAM_START+RAM_SIZE*4)) {   // 256 bytes, mirrored (?)
		t-=RAM_START;
		t &= 0xfe;
	  ram_seg[t++]=HIBYTE(t1);			// big-endian 
	  ram_seg[t]  =LOBYTE(t1);
		}

  }



void initHW(void) {
  int i;
extern const unsigned char charset_international[2048],tmsFont[(128-32)*8];
  struct SPRITE_ATTR *sa;
  
	TMS9919[1]=B8(00000000);
  TMS9901[0]=0;
  
  memset(TMSVideoRAM,0,TMSVIDEORAM_SIZE);    // mah...
  TMS9918Reg[0]=TMS_R0_EXT_VDP_DISABLE | TMS_R0_MODE_GRAPHICS_I;
  TMS9918Reg[1]=TMS_R1_RAM_16K | TMS_R1_MODE_GRAPHICS_I /* bah   | TMS_R1_DISP_ACTIVE | TMS_R1_INT_ENABLE*/;
  TMS9918Reg[2]=TMS_DEFAULT_VRAM_NAME_ADDRESS >> 10;
  TMS9918Reg[3]=TMS_DEFAULT_VRAM_COLOR_ADDRESS >> 6;
  TMS9918Reg[4]=TMS_DEFAULT_VRAM_PATT_ADDRESS >> 11;
  TMS9918Reg[5]=TMS_DEFAULT_VRAM_SPRITE_ATTR_ADDRESS >> 7;
  TMS9918Reg[6]=TMS_DEFAULT_VRAM_SPRITE_PATT_ADDRESS >> 11;
  TMS9918Reg[7]=(1 /*black*/ << 4) | 7 /*cyan*/;
  TMS9918RegS=0;
  TMS9918Sel=TMS9918WriteStage=0;
  memcpy(TMSVideoRAM+TMS_DEFAULT_VRAM_PATT_ADDRESS,charset_international,2048);// mah...
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

  for(i=0; i<768; i++)
    TMSVideoRAM[TMS_DEFAULT_VRAM_NAME_ADDRESS+i]=i & 0xff;
  
  
//  OC1CONbits.ON = 0;   // spengo buzzer/audio

  }

