#include <windows.h>
#include "tms9900win.h"
#include "resource.h"

extern BYTE Keyboard[];
extern HFILE spoolFile;
extern BYTE CPUPins;
//extern BYTE KBDIRQ;
extern BYTE IRQenable;

int decodeKBD(int ch, long l, BOOL m) {
	char myBuf[128];
	static BYTE addedMod;
	int j;

//	for(j=0; j<8; j++)
//		Keyboard[j]=255;
	if(!m) {		// KEYDOWN

  
	switch(ch) {
		case ' ':
      Keyboard[0] = 54;   // (non c'è in tabella ma lo vedo nel codice...
			break;
		case 'A':
      Keyboard[0] = 28;
			break;
		case 'B':
      Keyboard[0] = 44;
			break;
		case 'C':
      Keyboard[0] = 43;
			break;
		case 'D':
      Keyboard[0] = 30;
			break;
		case 'E':
      Keyboard[0] = 12;
			break;
		case 'F':
      Keyboard[0] = 36;
			break;
		case 'G':
      Keyboard[0] = 38;
			break;
		case 'H':
      Keyboard[0] = 26;
			break;
		case 'I':
      Keyboard[0] = 18;
			break;
		case 'J':
      Keyboard[0] = 31;
			break;
		case 'K':
      Keyboard[0] = 34;
			break;
		case 'L':
      Keyboard[0] = 24;
			break;
		case 'M':
      Keyboard[0] = 46;
			break;
		case 'N':
      Keyboard[0] = 6;
			break;
		case 'O':
      Keyboard[0] = 23;
			break;
		case 'P':
      Keyboard[0] = 29;
			break;
		case 'Q':
      Keyboard[0] = 11;
			break;
		case 'R':
      Keyboard[0] = 20;
			break;
		case 'S':
      Keyboard[0] = 35;
			break;
		case 'T':
      Keyboard[0] = 14;
			break;
		case 'U':
      Keyboard[0] = 15;
			break;
		case 'V':
      Keyboard[0] = 4;
			break;
		case 'W':
      Keyboard[0] = 17;
			break;
		case 'X':
      Keyboard[0] = 3;
			break;
		case 'Y':
      Keyboard[0] = 22;
			break;
		case 'Z':
      Keyboard[0] = 41;
			break;
		case '0':
      Keyboard[0] = 13;
			break;
		case ')':
      Keyboard[0] = 13;
      Keyboard[1] |= 0x4;
			break;
		case '1':
      Keyboard[0] = 27;
			break;
		case '!':
      Keyboard[0] = 27;
      Keyboard[1] |= 0x4;
			break;
		case '2':
      Keyboard[0] = 9;
			break;
		case '@':
      Keyboard[0] = 9;
      Keyboard[1] |= 0x4;
			break;
		case '3':
      Keyboard[0] = 25;
			break;
		case '#':
      Keyboard[0] = 25;
      Keyboard[1] |= 0x4;
			break;
		case '4':
      Keyboard[0] = 62;
			break;
		case '$':
      Keyboard[0] = 62;
      Keyboard[1] |= 0x4;
			addedMod=1;
			break;
		case '5':
      Keyboard[0] = 58;
			break;
//		case '%': è VK_LEFT
//      Keyboard[0] = 58;
//      Keyboard[1] |= 0x4;
			addedMod=1;
//			break;
		case '6':
      Keyboard[0] = 10;
			break;
		case '^':
      Keyboard[0] = 10;
      Keyboard[1] |= 0x4;
			addedMod=1;
			break;
		case '7':
      Keyboard[0] = 63;
			break;
/*		case '&':
      Keyboard[0] = 63;
      Keyboard[1] |= 0x4;
			addedMod=1;
			break;*/
		case '8':
      Keyboard[0] = 8;
			break;
		case VK_MULTIPLY:
		case '*':
      Keyboard[0] = 8;
      Keyboard[1] |= 0x4;
			addedMod=1;
			break;
		case '9':
      Keyboard[0] = 16;
			break;
/*		case '(':VK_DOWN
      Keyboard[0] = 16;
      Keyboard[1] |= 0x4;
			addedMod=1;
			break;*/
//		case VK_OEM_PERIOD          /*'.'*/:
		case 0xbe:		// .
      Keyboard[0] = 42;
			break;
		case '[':
      Keyboard[0] = 32;
			break;
		case '{':
      Keyboard[0] = 32;
      Keyboard[1] |= 0x4;
			addedMod=1;
			break;
		case ']':
      Keyboard[0] = 40;
			break;
		case '}':
      Keyboard[0] = 40;
      Keyboard[1] |= 0x4;
			addedMod=1;
			break;
		case 0xdd:
//		case '=':
      Keyboard[0] = 37;
			break;
		case VK_ADD:
		case '+':
      Keyboard[0] = 37;
      Keyboard[1] |= 0x4;
			addedMod=1;
			break;
		case ':':     // 
      Keyboard[0] = 39;
      Keyboard[1] |= 0x4;
			addedMod=1;
			break;
		case ';':
      Keyboard[0] = 39;
			break;
//		case VK_OEM_COMMA          /*','*/:
		case 0xbc:		// ,
      Keyboard[0] = 7;
			break;
//		case '.':
//      Keyboard[0] = 42;
//			break;
/*		case '\'':
      Keyboard[0] = 47;
			break;*/
		case '"':
      Keyboard[0] = 47;
      Keyboard[1] |= 0x4;
			addedMod=1;
			break;
		case '`':   // in effetti esce £
//		case '£':
			break;
		case '~':
      Keyboard[0] = 45;
      Keyboard[1] |= 0x4;
			addedMod=1;
			break;
		case '£':
      Keyboard[0] = 45;
			break;
//		case VK_OEM_MINUS:
		case VK_SUBTRACT:
		case 0xbd:
      Keyboard[0] = 21;
			break;
		case '_':
      Keyboard[0] = 21;
      Keyboard[1] |= 0x4;
			addedMod=1;
			break;
		case 0xdb:
//		case '?':
      Keyboard[0] = 5;     //
      Keyboard[1] |= 0x4;
			addedMod=1;
			break;
		case VK_DIVIDE:
		case '/':
      Keyboard[0] = 5;     //
			break;
		case VK_RETURN:
//		case '\r':
      Keyboard[0] = 48;
			break;
		case '\n':
      Keyboard[0] = 48;
			break;
		case VK_ESCAPE:
//		case '\x1b':
      Keyboard[0] = 51;
			break;
      
		case VK_BACK:     // (idioti :) bah non c'è... lo metto qua
      Keyboard[0] = 49;
      Keyboard[1] |= 0x2;
			addedMod=1;
			break;
		case VK_LEFT:     //
      Keyboard[0] = 49;
			break;
		case VK_UP:
      Keyboard[0] = 50;
			break;
		case VK_RIGHT:
      Keyboard[0] = 52;
			break;
		case VK_DOWN:
      Keyboard[0] = 55;
			break;

		case VK_SHIFT:     // shift...
      Keyboard[1] |= 0x4;
			break;
		case VK_MENU:     // alt
      Keyboard[1] |= 0x1;
			break;
		case VK_CONTROL:     // ctrl
      Keyboard[1] |= 0x2;
			break;
		case VK_CAPITAL:
      Keyboard[0] = 0x33;
			break;
		case VK_TAB:
      Keyboard[0] = 0x13;
			break;
      
		case VK_F1:     // F1
      Keyboard[0] = 0x39;    //ossia sembra 232 dal codice a 4B02
			break;
		case VK_F2:     // F2
      Keyboard[0] = 0x3b;		//ossia sembra 236 a 4B06
			break;
		case VK_F3:
      Keyboard[0] = 0x3c;
			break;
		case VK_F4:
      Keyboard[0] = 0x38;
			break;
		case VK_F5:
      Keyboard[0] = 0x3d;
			break;
      
    default:
      break;
      
		}
  
  Keyboard[2] = 1;
 

//   
//  if(IRQenable & B8(01000000))        //1=attivi
//    KBDIRQ=1;
  
  

  
//  if(IRQenable & 0b01000000)        //1=attivi
//    KBDIRQ=1;

		}
	else {			// KEYUP

	  Keyboard[0] = 0xff;
//	  KBDIRQ=1;  // lo uso solo come semaforo qua!

		}
//	wsprintf(myBuf,"flag: %02x",GetValue(0x28d));
//	SetWindowText(hStatusWnd,myBuf);
	}

