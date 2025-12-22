//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#define STRICT

#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE

#ifndef WIN32
#include <shell.h>
#endif

#include "afxres.h"
#include "resource.h"


#define CPU_CLOCK_DIVIDER 10000L		// 
#define HW_CLOCK_DIVIDER 20000L		// 

#define RAM_START 0x8000		// in certi posti pare mirrored tra 8000 e 8400, in altri solo 8300..8400...
#define RAM_SIZE 256
//#define RAM_START 0xc000
//#define RAM_SIZE 16384 ma DEVE partire dal Top!!
//#define RAM_START 0xf000  // per VELOCIZZAER debug!
//#define RAM_SIZE 4096 // ma DEVE partire dal Top!!
//#define RAM_SIZE2 65536
//#define RAM_START2 0x0000   // in slot 2, v.sotto
#define ROM_START 0x0000
#define ROM_SIZE 8192
#define GROM_START 0x0000
#define GROM_SIZE (3* 0x2000)    // in effetti 3 da 0x1800.. v. sotto


#define MIN_RASTER 0        // 
#define MAX_RASTER 255
#define HORIZ_SIZE 256      // o 
#define VERT_SIZE 192
#define HORIZ_OFFSCREEN 0
#define VERT_OFFSCREEN 0      // 
#define VIDEORAM_SIZE ((HORIZ_SIZE+HORIZ_OFFSCREEN*2)*(VERT_SIZE+VERT_OFFSCREEN*2))/2


typedef unsigned short int SWORD;

#pragma pack( push, before_structs )
#pragma pack(1)
union PIPE {
	SWORD x;
	BYTE bb[4];
	struct {
		BYTE l;
		BYTE h;
//		BYTE u;		 bah no, sposto la pipe quando ci sono le istruzioni lunghe 4...
		} b;
	} Pipe2;

union Z_REG {
  SWORD x;
  struct { 
    BYTE l;
    BYTE h;
    } b;
  };
union Z_REGISTERS {
  BYTE b[32];
  union Z_REG r[16];
  };
#define ID_LG 0x8000
#define ID_AG 0x4000
#define ID_ZERO 0x2000
#define ID_CARRY 0x1000
#define ID_OVF 0x0800
#define ID_PARITY 0x0400
#define ID_XOP 0x0200
#ifdef TMS9940 
#define ID_DIGITCARRY 0x100  // boh!
  // v. anche 990/10?? b7=PR b8=MF
#endif
#define ID_INTERRUPTMASK 0x000F
union REGISTRO_F {
  SWORD x;
  struct {
    unsigned int InterruptMask: 4;
    unsigned int unused2: 4;
#ifdef TMS9940 
//      unsigned int MapFile: 1;   // 
//      unsigned int Privileged: 1;   // v. anche 990/10?? b7=PR b8=MF
    unsigned int DigitCarry: 1;   // boh
#else
    unsigned int unused: 1;
#endif
    unsigned int XOP: 1;
    unsigned int Parity: 1;   // 0=pari 1=dispari (ODD PARITY)
    unsigned int Overflow: 1;
    unsigned int Carry: 1;
    unsigned int Zero: 1;
    unsigned int ArithmeticGreater: 1;
    unsigned int LogicalGreater: 1;
    };
  };
union OPERAND {
  BYTE *reg8;
  SWORD *reg16;
  SWORD mem;
  };
union RESULT {
  struct {
    BYTE l;
    BYTE h;
    } b;
  SWORD x;
  DWORD d;
  };

#pragma pack( pop, before_structs )


#define TMS99xx_BASE 0x8800
#define TMSVIDEORAM_SIZE 16384
#define TMS_R0_MODE_GRAPHICS_I    0x00
#define TMS_R0_MODE_GRAPHICS_II   0x02
#define TMS_R0_MODE_MULTICOLOR    0x00
#define TMS_R0_MODE_TEXT          0x00
#define TMS_R0_EXT_VDP_ENABLE     0x01
#define TMS_R0_EXT_VDP_DISABLE    0x00

#define TMS_R1_RAM_16K            0x80
#define TMS_R1_RAM_4K             0x00
#define TMS_R1_DISP_BLANK         0x00
#define TMS_R1_DISP_ACTIVE        0x40
#define TMS_R1_INT_ENABLE         0x20
#define TMS_R1_INT_DISABLE        0x00
#define TMS_R1_MODE_GRAPHICS_I    0x00
#define TMS_R1_MODE_GRAPHICS_II   0x00
#define TMS_R1_MODE_MULTICOLOR    0x08
#define TMS_R1_MODE_TEXT          0x10
#define TMS_R1_SPRITE_8           0x00
#define TMS_R1_SPRITE_16          0x02
#define TMS_R1_SPRITE_MAG1        0x00
#define TMS_R1_SPRITE_MAG2        0x01
#define LAST_SPRITE_YPOS	        0xC0		// dice 0xBE  https://www.unige.ch/medecine/nouspikel/ti99/tms9918a.htm#Sprites

#define TMS_DEFAULT_VRAM_NAME_ADDRESS          0x3800		// qua boh, forse 0
#define TMS_DEFAULT_VRAM_COLOR_ADDRESS         0x0400
#define TMS_DEFAULT_VRAM_PATT_ADDRESS          0x0800
#define TMS_DEFAULT_VRAM_SPRITE_ATTR_ADDRESS   0x0500
#define TMS_DEFAULT_VRAM_SPRITE_PATT_ADDRESS   0x1000
extern BYTE TMSVideoRAM[];
struct SPRITE_ATTR {
  BYTE ypos,xpos;    // v. sotto, a volte usato come signed
  BYTE name;
  union {
    struct {
      unsigned int color:4;
      unsigned int unused:3;
      unsigned int eclock:1;
      };
    BYTE tag;
    };
  };




enum {
  DoReset=1,
  DoIRQ=2,
//  DoWait=8,     // 
  DoIdle=4,
	DoLOAD=8
	};

extern HWND ghWnd,hStatusWnd;
extern HINSTANCE g_hinst;
extern HANDLE hAccelTable;

ATOM MyRegisterClass(CONST WNDCLASS*);
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK Dump0(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK Dump1(HWND, UINT, WPARAM, LPARAM);
BOOL PlayResource(LPSTR lpName,BOOL bStop);
int decodeKBD(int, long, BOOL);
int UpdateScreen(HDC,SWORD,SWORD);

WORD _fastcall GetPipe(SWORD);
BYTE _fastcall GetValue(SWORD);
SWORD _fastcall GetValueCRU(SWORD,SWORD,BYTE);
SWORD _fastcall GetIntValue(SWORD);
void _fastcall PutValue(SWORD,BYTE);
void _fastcall PutIntValue(SWORD,SWORD);
void _fastcall PutValueCRU(SWORD,SWORD,BYTE,BYTE);
int Emulate(int);
BOOL CenterWindow(HWND, HWND);
int WritePrivateProfileInt(char *,char *,int,char *);
void initHW();





/* turn a numeric literal into a hex constant
(avoids problems with leading zeroes)
8-bit constants max value 0b11111111, always fits in unsigned long
Tom Torfs
https://post.bytes.com/forum/topic/c/176274-binary-constant-macros
*/
#define HEX__(n) 0x##n##LU

/* 8-bit conversion function */
#define B8__(x) ((x&0x0000000FLU)?1:0) \
| ((x&0x000000F0LU)?2:0) \
| ((x&0x00000F00LU)?4:0) \
| ((x&0x0000F000LU)?8:0) \
| ((x&0x000F0000LU)?16:0) \
| ((x&0x00F00000LU)?32:0) \
| ((x&0x0F000000LU)?64:0) \
| ((x&0xF0000000LU)?128:0)

/* *** user macros *** /

/* for upto 8-bit binary constants */
#define B8(d) ((unsigned char)B8__(HEX__(d)))

/* for upto 16-bit binary constants, MSB first */
#define B16(dmsb,dlsb) (((unsigned short)B8(dmsb)<<8) \
| B8(dlsb))

/* for upto 32-bit binary constants, MSB first */
#define B32(dmsb,db2,db3,dlsb) (((unsigned long)B8(dmsb)<< 24) \
| ((unsigned long)B8(db2)<<16) \
| ((unsigned long)B8(db3)<<8) \
| B8(dlsb))

//https://en.wikipedia.org/wiki/TMS9900
#define WORKING_REG_INDEX (Pipe1 & 0xf)
#define GET_REG(q) (MAKEWORD(regs->r[q].b.h,regs->r[q].b.l))      // 
#define GET_WORKING_REG() GET_REG(workingRegIndex)      // 
#define SET_REG(q,n) {regs->r[q].b.l=HIBYTE(n);regs->r[q].b.h=LOBYTE(n);}      // 
#define SET_WORKING_REG(n) SET_REG(workingRegIndex,n);
#define WORKING_TS ((Pipe1 >> 4) & B8(11))
#define WORKING_TD ((Pipe1 >> 10) & B8(11))
#define REGISTER_DIRECT 0
#define REGISTER_INDIRECT 1
#define REGISTER_SYMBOLIC_INDEXED 2
#define REGISTER_INDIRECT_AUTOINCREMENT 3
#define WORKING_REG2_INDEX ((Pipe1 >> 6) & 0xf)
#define GET_WORKING_REG2() GET_REG(workingReg2Index)      // 
#define SET_WORKING_REG2(n) SET_REG(workingReg2Index,n);

// USARE, MA OCCHIO!!  ci sono le piccole differenze con getPipe e pc+=2
#define COMPUTE_SOURCE \
  switch(workingTS) {\
    case REGISTER_DIRECT:\
      res1.x=GET_WORKING_REG();\
      break;\
    case REGISTER_INDIRECT:\
      res1.x=GetIntValue(GET_WORKING_REG());\
      break;\
    case REGISTER_SYMBOLIC_INDEXED:\
      if(workingRegIndex)\
        res1.x=GetIntValue(GET_WORKING_REG()+(int16_t)Pipe2.x);\
      else\
        res1.x=GetIntValue(Pipe2.x);\
      /*GetPipe(_pc);*/\
      _pc+=2;\
      break;\
    case REGISTER_INDIRECT_AUTOINCREMENT:\
      res1.x=GetIntValue(GET_WORKING_REG());\
      SET_WORKING_REG(GET_WORKING_REG()+2);\
      break;\
    }
#define COMPUTE_SOURCE2 \
  switch(workingTD) {\
    case REGISTER_DIRECT:\
      res2.x=GET_WORKING_REG2();\
      break;\
    case REGISTER_INDIRECT:\
      res2.x=GetIntValue(GET_WORKING_REG2());\
      break;\
    case REGISTER_SYMBOLIC_INDEXED:\
      if(workingReg2Index)\
        res2.x=GetIntValue(GET_WORKING_REG2()+(int16_t)Pipe2.x);\
      else\
        res2.x=GetIntValue(Pipe2.x);\
      _pc+=2;\
      break;\
    case REGISTER_INDIRECT_AUTOINCREMENT:\
      res2.x=GetIntValue(GET_WORKING_REG2());\
      break;\
    }
#define COMPUTE_DEST \
  switch(workingTD) {\
    case REGISTER_DIRECT:\
      SET_WORKING_REG2(res3.x);\
      break;\
    case REGISTER_INDIRECT:\
      PutIntValue(GET_WORKING_REG2(),res3.x);\
      break;\
    case REGISTER_SYMBOLIC_INDEXED:\
      if(workingReg2Index)\
        PutIntValue(GET_WORKING_REG2()+(int16_t)Pipe2.x,res3.x);\
      else\
        PutIntValue(Pipe2.x,res3.x);\
      _pc+=2;\
      break;\
    case REGISTER_INDIRECT_AUTOINCREMENT:\
      PutIntValue(GET_WORKING_REG2(),res3.x);\
      SET_WORKING_REG2(GET_WORKING_REG2()+2);\
      break;\
    }
#define COMPUTE_SOURCE8 \
  switch(workingTS) {\
    case REGISTER_DIRECT:\
      res1.b.l=HIBYTE(GET_WORKING_REG());\
      break;\
    case REGISTER_INDIRECT:\
      res1.b.l=GetValue(GET_WORKING_REG());\
      break;\
    case REGISTER_SYMBOLIC_INDEXED:\
      if(workingRegIndex)\
        res1.b.l=GetValue(GET_WORKING_REG()+(int16_t)Pipe2.x);\
      else\
        res1.b.l=GetValue(Pipe2.x);\
      /*GetPipe(_pc);*/\
      _pc+=2;\
      break;\
    case REGISTER_INDIRECT_AUTOINCREMENT:\
      res1.b.l=GetValue(GET_WORKING_REG());\
      SET_WORKING_REG(GET_WORKING_REG()+1);\
      break;\
    }
#define COMPUTE_SOURCE82 \
  switch(workingTD) {\
    case REGISTER_DIRECT:\
      res2.b.l=HIBYTE(GET_WORKING_REG2());\
      break;\
    case REGISTER_INDIRECT:\
      res2.b.l=GetValue(GET_WORKING_REG2());\
      break;\
    case REGISTER_SYMBOLIC_INDEXED:\
      if(workingReg2Index)\
        res2.b.l=GetValue(GET_WORKING_REG2()+(int16_t)Pipe2.x);\
      else\
        res2.b.l=GetValue(Pipe2.x);\
      _pc+=2;\
      break;\
    case REGISTER_INDIRECT_AUTOINCREMENT:\
      res2.b.l=GetValue(GET_WORKING_REG2());\
      break;\
    }
#define COMPUTE_DEST8 \
  switch(workingTD) {\
    case REGISTER_DIRECT:\
      SET_WORKING_REG2(MAKEWORD(LOBYTE(GET_WORKING_REG2()),res3.b.l));	/* se registro, va in MSB*/ \
      break;\
    case REGISTER_INDIRECT:\
      PutValue(GET_WORKING_REG2(),res3.b.l);\
      break;\
    case REGISTER_SYMBOLIC_INDEXED:\
      if(workingReg2Index)\
        PutValue(GET_WORKING_REG2()+(int16_t)Pipe2.x,res3.b.l);\
      else\
        PutValue(Pipe2.x,res3.b.l);\
      _pc+=2;\
      break;\
    case REGISTER_INDIRECT_AUTOINCREMENT:\
      PutValue(GET_WORKING_REG2(),res3.b.l);\
      SET_WORKING_REG2(GET_WORKING_REG2()+1);\
      break;\
    }

