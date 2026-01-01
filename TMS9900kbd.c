#include <windows.h>
#include "tms9900win.h"
#include "resource.h"


#pragma warning T (e pure E,V,B,N,M) vale come FCNT 4 OSSIA RESET!! :) (che e gestito in IRQ peraltro, 0x0A66~


extern BYTE Keyboard[];
extern HFILE spoolFile;
extern BYTE CPUPins;
//extern BYTE KBDIRQ;
extern BYTE IRQenable;

/*Bit 	R12 address I/O/I+ 			Usage
	3 		>0006 			I				  =    .   ,   M   N   /  fire1  fire2 
	4 		>0008 			I				space  L   K   J   H   ;  left1  left2
	5 		>000A 			I				enter  O   I   U   Y   P  right1 right2
	6 		>000C 			I				(none) 9   8   7   6   0  down1  down2
	7 		>000E 			I				fctn   2   3   4   5   1  up1    up2 
	8 		>0010 			I				shift  S   D   F   G   A 
	9 		>0012 			I				ctrl   W   E   R   T   Q 
	10 		>0014 			I				(none) X   C   V   B   Z 

	18 		>0024 			O 			Select keyboard column (or joystick) 
*/

int decodeKBD(int ch, long l, BOOL m) {
	char myBuf[128];
	static BYTE addedMod;
	int j;

//	for(j=0; j<8; j++)
//		Keyboard[j]=255;
	if(!m) {		// KEYDOWN
  
		switch(ch) {
			case ' ':		//VK_SPACE
				Keyboard[1]&=~B8(10000000);
				break;
			case 'A':
				Keyboard[5]&=~B8(00000100);
				break;
			case 'B':
				Keyboard[7]&=~B8(00001000);
				break;
			case 'C':
				Keyboard[7]&=~B8(00100000);
				break;
			case 'D':
				Keyboard[5]&=~B8(00100000);
				break;
			case 'E':
				Keyboard[6]&=~B8(00100000);
				break;
			case 'F':
				Keyboard[5]&=~B8(00010000);
				break;
			case 'G':
				Keyboard[5]&=~B8(00001000);
				break;
			case 'H':
				Keyboard[1]&=~B8(00001000);
				break;
			case 'I':
				Keyboard[2]&=~B8(00100000);
				break;
			case 'J':
				Keyboard[1]&=~B8(00010000);
				break;
			case 'K':
				Keyboard[1]&=~B8(00100000);
				break;
			case 'L':
				Keyboard[1]&=~B8(01000000);
				break;
			case 'M':
				Keyboard[0]&=~B8(00010000);
				break;
			case 'N':
				Keyboard[0]&=~B8(00001000);
				break;
			case 'O':
				Keyboard[2]&=~B8(01000000);
				break;
			case 'P':
				Keyboard[2]&=~B8(00000100);
				break;
			case 'Q':
				Keyboard[6]&=~B8(00000100);
				break;
			case 'R':
				Keyboard[6]&=~B8(00010000);
				break;
			case 'S':
				Keyboard[5]&=~B8(01000000);
				break;
			case 'T':
				Keyboard[6]&=~B8(00001000);
				break;
			case 'U':
				Keyboard[2]&=~B8(00010000);
				break;
			case 'V':
				Keyboard[7]&=~B8(00010000);
				break;
			case 'W':
				Keyboard[6]&=~B8(01000000);
				break;
			case 'X':
				Keyboard[7]&=~B8(01000000);
				break;
			case 'Y':
				Keyboard[2]&=~B8(00001000);
				break;
			case 'Z':
				Keyboard[7]&=~B8(00000100);
				break;
			case VK_NUMPAD0:
			case '0':
				Keyboard[3]&=~B8(00000100);
				break;
			case VK_NUMPAD1:
			case '1':
				Keyboard[4]&=~B8(00000100);
				break;
			case VK_NUMPAD2:
			case '2':
				Keyboard[4]&=~B8(01000000);
				break;
			case VK_NUMPAD3:
			case '3':
				Keyboard[4]&=~B8(00100000);
				break;
			case VK_NUMPAD4:
			case '4':
				Keyboard[4]&=~B8(00010000);
				break;
			case VK_NUMPAD5:
			case '5':
				Keyboard[4]&=~B8(00001000);
				break;
			case VK_NUMPAD6:
			case '6':
				Keyboard[3]&=~B8(00001000);
				break;
			case VK_NUMPAD7:
			case '7':
				Keyboard[3]&=~B8(00010000);
				break;
			case VK_NUMPAD8:
			case '8':
				Keyboard[3]&=~B8(00100000);
				break;
			case VK_NUMPAD9:
			case '9':
				Keyboard[3]&=~B8(01000000);
				break;
			case VK_HOME:
				break;
			case VK_DOWN:
				Keyboard[4]&=~B8(10000000);
				Keyboard[7]&=~B8(01000000);
				break;
			case VK_RIGHT:
				Keyboard[4]&=~B8(10000000);
				Keyboard[5]&=~B8(00100000);
				break;
			case VK_UP:
				Keyboard[4]&=~B8(10000000);
				Keyboard[6]&=~B8(00100000);
				break;
			case VK_LEFT:
				Keyboard[4]&=~B8(10000000);
				Keyboard[5]&=~B8(01000000);
				break;
			case VK_LSHIFT:
			case VK_SHIFT:
			case VK_RSHIFT:
				Keyboard[5]&=~B8(10000000);
				break;
			case VK_CONTROL:
				Keyboard[6]&=~B8(10000000);
				break;
			case VK_MENU:     // alt ossia FCTN
				Keyboard[4]&=~B8(10000000);
				break;
			case VK_CAPITAL:
				break;
			case VK_MULTIPLY:
				Keyboard[5]&=~B8(10000000);
				Keyboard[3]&=~B8(00100000);
				break;
			case VK_SUBTRACT:
				Keyboard[5]&=~B8(10000000);
				Keyboard[0]&=~B8(00000100);
				break;
			case 0xdb:		// '?
				break;
			case 0xdd:		// ì^
				Keyboard[0]&=~B8(10000000);
				break;
			case 0xde:		// à#  faccio "
				Keyboard[4]&=~B8(10000000);
				Keyboard[2]&=~B8(00000100);
				break;
			case 0xba:		// è
				Keyboard[4]&=~B8(10000000);
				Keyboard[6]&=~B8(00010000);			// sarebbe R o F a seconda
				break;
			case 0xbb:
				Keyboard[4]&=~B8(10000000);
				Keyboard[6]&=~B8(00001000);			// sarebbe T o G a seconda
				break;
			case VK_ADD:
				Keyboard[5]&=~B8(10000000);
				Keyboard[0]&=~B8(10000000);
				break;
			case 0xc0:		// ò@
				break;
			case 0xbf:		// ù		~
				Keyboard[4]&=~B8(10000000);
				Keyboard[2]&=~B8(01000000);
				break;
			case 0xbc:		// ,
				Keyboard[0]&=~B8(00100000);
				break;
			case VK_DECIMAL:
	//		case VK_OEM_PERIOD          /*'.'*/:
			case 0xbe:		// .
				Keyboard[0]&=~B8(01000000);
				break;
			case 0xbd:		// -_
			case VK_DIVIDE:
				Keyboard[0]&=~B8(00000100);
				break;
			case VK_RETURN:
				Keyboard[2]&=~B8(10000000);
				break;
			case VK_BACK:			// FCNT 9
				Keyboard[4]&=~B8(10000000);
//				Keyboard[3]&=~B8(01000000);		// non va... forse extended basic?
				Keyboard[5]&=~B8(01000000);		// faccio left quindi
				break;
			case VK_DELETE:		// FCNT 1
				Keyboard[4]&=~B8(10000000);
				Keyboard[4]&=~B8(00000100);
				break;
			case VK_F1:
				Keyboard[4]&=~B8(10000000);
				Keyboard[4]&=~B8(00000100);
				break;
			case VK_F2:
				break;
			case VK_F3:				// FCNT 3
				Keyboard[4]&=~B8(10000000);
				Keyboard[4]&=~B8(00100000);
				break;
			case VK_F4:				// FCNT 4
				Keyboard[4]&=~B8(10000000);
				Keyboard[4]&=~B8(00010000);
				break;
			case VK_F5:
				break;
			case VK_F6:
				break;
			case VK_F7:
				break;
			case VK_F8:				// FCNT 8
				Keyboard[4]&=~B8(10000000);
				Keyboard[3]&=~B8(00100000);
				break;
			case 0xdc:		// |\ :)
				Keyboard[4]&=~B8(10000000);
				Keyboard[5]&=~B8(00000100);		// sarebbe A opp. Z a seconda del simbolo...
				break;
			case 0xe2:		// <  :)
				Keyboard[4]&=~B8(10000000);
				Keyboard[7]&=~B8(00000100);
				break;
			case VK_ESCAPE:		// faccio come CLEAR ossia FCTN 4
				Keyboard[4]&=~B8(10000000);
				Keyboard[4]&=~B8(00010000);
				break;
			case VK_PAUSE:
				break;
			case VK_F12:			// simulo reset! FCNT =
				Keyboard[4]&=~B8(10000000);
				Keyboard[0]&=~B8(10000000);
				break;
			}
  

//   
//  if(IRQenable & B8(01000000))        //1=attivi
//    KBDIRQ=1;
  
  
		}
	else {			// KEYUP

		switch(ch) {
			case ' ':		//VK_SPACE
				Keyboard[1]|=B8(10000000);
				break;
			case 'A':
				Keyboard[5]|=B8(00000100);
				break;
			case 'B':
				Keyboard[7]|=B8(00001000);
				break;
			case 'C':
				Keyboard[7]|=B8(00100000);
				break;
			case 'D':
				Keyboard[5]|=B8(00100000);
				break;
			case 'E':
				Keyboard[6]|=B8(00100000);
				break;
			case 'F':
				Keyboard[5]|=B8(00010000);
				break;
			case 'G':
				Keyboard[5]|=B8(00001000);
				break;
			case 'H':
				Keyboard[1]|=B8(00001000);
				break;
			case 'I':
				Keyboard[2]|=B8(00100000);
				break;
			case 'J':
				Keyboard[1]|=B8(00010000);
				break;
			case 'K':
				Keyboard[1]|=B8(00100000);
				break;
			case 'L':
				Keyboard[1]|=B8(01000000);
				break;
			case 'M':
				Keyboard[0]|=B8(00010000);
				break;
			case 'N':
				Keyboard[0]|=B8(00001000);
				break;
			case 'O':
				Keyboard[2]|=B8(01000000);
				break;
			case 'P':
				Keyboard[2]|=B8(00000100);
				break;
			case 'Q':
				Keyboard[6]|=B8(00000100);
				break;
			case 'R':
				Keyboard[6]|=B8(00010000);
				break;
			case 'S':
				Keyboard[5]|=B8(01000000);
				break;
			case 'T':
				Keyboard[6]|=B8(00001000);
				break;
			case 'U':
				Keyboard[2]|=B8(00010000);
				break;
			case 'V':
				Keyboard[7]|=B8(00010000);
				break;
			case 'W':
				Keyboard[6]|=B8(01000000);
				break;
			case 'X':
				Keyboard[7]|=B8(01000000);
				break;
			case 'Y':
				Keyboard[2]|=B8(00001000);
				break;
			case 'Z':
				Keyboard[7]|=B8(00000100);
				break;
			case VK_NUMPAD0:
			case '0':
				Keyboard[3]|=B8(00000100);
				break;
			case VK_NUMPAD1:
			case '1':
				Keyboard[4]|=B8(00000100);
				break;
			case VK_NUMPAD2:
			case '2':
				Keyboard[4]|=B8(01000000);
				break;
			case VK_NUMPAD3:
			case '3':
				Keyboard[4]|=B8(00100000);
				break;
			case VK_NUMPAD4:
			case '4':
				Keyboard[4]|=B8(00010000);
				break;
			case VK_NUMPAD5:
			case '5':
				Keyboard[4]|=B8(00001000);
				break;
			case VK_NUMPAD6:
			case '6':
				Keyboard[3]|=B8(00001000);
				break;
			case VK_NUMPAD7:
			case '7':
				Keyboard[3]|=B8(00010000);
				break;
			case VK_NUMPAD8:
			case '8':
				Keyboard[3]|=B8(00100000);
				break;
			case VK_NUMPAD9:
			case '9':
				Keyboard[3]|=B8(01000000);
				break;
			case VK_HOME:
				break;
			case VK_DOWN:
				Keyboard[4]|=B8(10000000);
				Keyboard[7]|=B8(01000000);
				break;
			case VK_RIGHT:
				Keyboard[4]|=B8(10000000);
				Keyboard[5]|=B8(00100000);
				break;
			case VK_UP:
				Keyboard[4]|=B8(10000000);
				Keyboard[6]|=B8(00100000);
				break;
			case VK_LEFT:
				Keyboard[4]|=B8(10000000);
				Keyboard[5]|=B8(01000000);
				break;
			case VK_LSHIFT:
			case VK_SHIFT:
			case VK_RSHIFT:
				Keyboard[5]|=B8(10000000);
				break;
			case VK_CONTROL:
				Keyboard[6]|=B8(10000000);
				break;
			case VK_MENU:     // alt ossia FCTN
				Keyboard[4]|=B8(10000000);
				break;
			case VK_CAPITAL:
				break;
			case VK_MULTIPLY:
				Keyboard[5]|=B8(10000000);
				Keyboard[3]|=B8(00100000);
				break;
			case 0xbb:
				Keyboard[4]|=B8(10000000);
				Keyboard[6]|=B8(00001000);			// sarebbe T o G a seconda
				break;
			case VK_ADD:
				Keyboard[5]|=B8(10000000);
				Keyboard[0]|=B8(10000000);
				break;
			case VK_SUBTRACT:
				Keyboard[5]|=B8(10000000);
				Keyboard[0]|=B8(00000100);
				break;
			case 0xbd:		// -_
			case VK_DIVIDE:
				Keyboard[0]|=B8(00000100);
				break;
			case 0xba:		// è [{
				Keyboard[4]|=B8(10000000);
				Keyboard[6]|=B8(00010000);			// sarebbe R o F a seconda
				break;
			case VK_DECIMAL:
			case 0xbe:		// .
				Keyboard[0]|=B8(01000000);
				break;
			case 0xbc:		// ,
				Keyboard[0]|=B8(00100000);
				break;
			case 0xdb:		// '?
				break;
			case 0xdd:		// ì^
				Keyboard[0]|=B8(10000000);
				break;
			case 0xde:		// à#  faccio "
				Keyboard[4]|=B8(10000000);
				Keyboard[2]|=B8(00000100);
				break;
			case 0xc0:		// ò@
				break;
			case 0xbf:		// ù
				Keyboard[4]|=B8(10000000);
				Keyboard[2]|=B8(01000000);
				break;
				break;
			case VK_RETURN:
				Keyboard[2]|=B8(10000000);
				break;
			case VK_BACK:
				Keyboard[4]|=B8(10000000);
//				Keyboard[3]|=B8(01000000);		// v.sopra
				Keyboard[5]|=B8(01000000);		// 
				break;
			case VK_DELETE:
				Keyboard[4]|=B8(10000000);
				Keyboard[4]|=B8(00000100);
				break;
			case VK_F1:
				Keyboard[4]|=B8(10000000);
				Keyboard[4]|=B8(00000100);
				break;
			case VK_F2:
				break;
			case VK_F3:
				Keyboard[4]|=B8(10000000);
				Keyboard[4]|=B8(00100000);
				break;
			case VK_F4:
				Keyboard[4]|=B8(10000000);
				Keyboard[4]|=B8(00010000);
				break;
			case VK_F5:
				break;
			case VK_F6:
				break;
			case VK_F7:
				break;
			case VK_F8:
				Keyboard[4]|=B8(10000000);
				Keyboard[3]|=B8(00100000);
				break;
			case 0xdc:		// |
				Keyboard[4]|=B8(10000000);
				Keyboard[5]|=B8(00000100);
				break;
			case 0xe2:		// <  :)
				Keyboard[4]|=B8(10000000);
				Keyboard[7]|=B8(00000100);
				break;
			case VK_ESCAPE:
				Keyboard[4]|=B8(10000000);
				Keyboard[4]|=B8(00010000);
				break;
			case VK_PAUSE:
				break;
			case VK_F12:
				Keyboard[4]=0xff;		// vabbe' :) v.sopra
				Keyboard[0]=0xff;
				break;
			}
//	  KBDIRQ=1;  // lo uso solo come semaforo qua!

		}
//	wsprintf(myBuf,"flag: %02x",GetValue(0x28d));
//	SetWindowText(hStatusWnd,myBuf);
	}

