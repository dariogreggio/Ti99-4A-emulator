#define APPNAME "TMS9900Win"


// Windows Header Files:
#include <windows.h>
#include <stdint.h>

// Local Header Files
#include "tms9900win.h"
#include "resource.h"

// Makes it easier to determine appropriate code paths:
#if defined (WIN32)
	#define IS_WIN32 TRUE
#else
	#define IS_WIN32 FALSE
#endif
#define IS_NT      IS_WIN32 && (BOOL)(GetVersion() < 0x80000000)
#define IS_WIN32S  IS_WIN32 && (BOOL)(!(IS_NT) && (LOBYTE(LOWORD(GetVersion()))<4))
#define IS_WIN95 (BOOL)(!(IS_NT) && !(IS_WIN32S)) && IS_WIN32

// Global Variables:
HINSTANCE g_hinst;
HANDLE hAccelTable;
char szAppName[] = APPNAME; // The name of this application
char INIFile[] = APPNAME".ini";
char szTitle[]   = APPNAME; // The title bar text
int AppXSize=534,AppYSize=358,AppYSizeR,YXRatio=1;		// così il rapporto è ok, ev. ingandire o rimpicciolire
BOOL fExit,debug=1, doppiaDim;
HWND ghWnd,hStatusWnd;
HBRUSH hBrush;
HPEN hPen1;
HFONT hFont,hFont2;
COLORREF Colori[16]={		// 
	RGB(0,0,0),						 // nero (trasparente
	RGB(0,0,0),						 // nero
	RGB(0x00,0xc0,0x00),	 // verde medio
	RGB(0x80,0xff,0x80),	 // verde chiaro

	RGB(0x00,0x00,0x80),	 // blu scuro
	RGB(0x00,0x00,0xff),	 // blu
	RGB(0x80,0x00,0x00),	 // rosso scuro
	RGB(0x00,0xff,0xff),	 // azzurro/cyan

	RGB(0xc0,0x00,0x00),	 // rosso medio
	RGB(0xff,0x00,0x00),	 // rosso chiaro
	RGB(0x80,0x80,0x00),	 // giallo scuro
	RGB(0xff,0xff,0x00),	 // giallo
	
	RGB(0x00,0x80,0x00),	 // verde
	RGB(0xff,0x00,0xff),	 // porpora/magenta
	RGB(0x80,0x80,0x80),	 // grigio medio
	RGB(0xff,0xff,0xff),	 // bianco
	};
UINT hTimer;
extern BOOL ColdReset;
extern BYTE CPUPins;
extern BYTE ram_seg[RAM_SIZE];
extern BYTE rom_seg[],rom_seg2[],grom_seg[];
extern BYTE TMS9918Reg[8],TMS9918RegS,TMS9918Sel,TMS9918WriteStage;
extern BYTE TMS9919[1]; 
extern BYTE TMS9901[32];
extern SWORD VICRaster;
BYTE VideoRAM[VIDEORAM_SIZE];		// 
extern DWORD VideoAddress;
extern BYTE VideoMode;
extern BYTE TIMIRQ,VIDIRQ,KBDIRQ,SERIRQ,RTCIRQ;
extern HFILE spoolFile;
BITMAPINFO *bmI;

extern const unsigned char TI994A_BIN_GROM0[],TI994A_BIN_GROM1[],TI994A_BIN_GROM2[],
	TI994A_BIN2[],TI994A_BIN_U610[],TI994A_BIN_U611[];


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	MSG msg;

	if(!hPrevInstance) {
		if(!InitApplication(hInstance)) {
			return (FALSE);
		  }
	  }

	if (!InitInstance(hInstance, nCmdShow)) {
		return (FALSE);
  	}

	if(*lpCmdLine) {
		PostMessage(ghWnd,WM_USER+1,0,(LPARAM)lpCmdLine);
		}

	hAccelTable = LoadAccelerators (hInstance,MAKEINTRESOURCE(IDR_ACCELERATOR1));
	Emulate(0);

  return msg.wParam;
	}


//https://www.angelfire.com/art2/unicorndreams/msx/RR-VDP.html#VDP-StatusReg
//https://www.unige.ch/medecine/nouspikel/ti99/tms9918a.htm#Registers
/*Bit:
Weight: 	0
>80 	1
>40 	2
>20 	3
>10 	4
>08 	5
>04 	6
>02 	7
>01
VR0 	0 	0 	0 	0 	0 	0 	Bitmap 	Ext vid
VR1 	16K 	Blank 	Int on 	Multic 	Text 	0 	Size 4 	Mag 2x
VR2 	0 	0 	0 	0 	Screen image
VR3
bitmap 	Color table
Addr 	Address mask
VR4
bitmap 	0 	0 	0 	0 	0 	Char pattern table
0 	0 	0 	0 	0 	Addr 	Address mask
VR5 	0 	Sprite attributs table
VR6 	0 	0 	0 	0 	0 	Sprites pattern table
VR7 	Text color (in text mode) 	Backdrop color
Status 	Int 	5+ sp 	Coinc 	5th sprite number
*/
const WORD graphColors[16]={0/*transparent*/,1,2,3, 4,5,6,7,
  8,9,10,11, 12,13,14,15
	};
int UpdateScreen(HDC hDC,SWORD rowIni, SWORD rowFin) {
	register int i;
	UINT16 px,py;
	int row1;
	register BYTE *p1,*p2;
  BYTE ch1,ch2,color,color1,color0;
	BYTE *pVideoRAM;

  BYTE videoMode=((TMS9918Reg[1] & 0x18) >> 2) | ((TMS9918Reg[0] & 2) >> 1);
  WORD charAddress=((WORD)(TMS9918Reg[4] & 7)) << 11;
  WORD videoAddress=((WORD)(TMS9918Reg[2] & 0xf)) << 10;
  WORD colorAddress=((WORD)(TMS9918Reg[3])) << 6;
  WORD spriteAttrAddress=((WORD)(TMS9918Reg[5] & 0x7f)) << 7;
  WORD spritePatternAddress=((WORD)(TMS9918Reg[5] & 0x7)) << 11;

  
  if(!(TMS9918Reg[1] & B8(01000000))) {    // blanked
    for(py=0; py<VERT_SIZE; py++) {   // bordo/sfondo
			pVideoRAM=(BYTE*)&VideoRAM[0]+(py)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN*2)));
      for(px=0; px<HORIZ_SIZE/2; px++)
//        writedata16(graphColors[TMS9918Reg[7] & 0xf]);
				*pVideoRAM++=((TMS9918Reg[7] & 0xf) << 4) | (TMS9918Reg[7] & 0xf);
			}
		goto do_plot;
    }
  else if(rowIni>=0 && rowIni<=192) {
    
		switch(videoMode) {    
			case 0:     // graphics 1
				for(py=rowIni/8; py<rowFin/8; py++) {    // 192 
					p2=((BYTE*)&TMSVideoRAM[videoAddress]) + (py*32 /*HORIZ_SIZE/8*/);
					for(row1=0; row1<8; row1++) {    // 192 linee(32 righe char) 
						pVideoRAM=(BYTE*)&VideoRAM[0]+((py*8)+row1)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN*2)));
						p1=p2;
						for(px=0; px<HORIZ_SIZE/8; px++) {    // 256 pixel 
							ch1=*p1++;
							ch2=TMSVideoRAM[charAddress + (ch1*8) + (row1)];
							color=TMSVideoRAM[colorAddress+ch1/8];
							color1=color >> 4; color0=color & 0xf;

							*pVideoRAM++=((ch2 & B8(10000000) ? color1 : color0) << 4) | (ch2 & B8(01000000) ? color1 : color0);
							*pVideoRAM++=((ch2 & B8(00100000) ? color1 : color0) << 4) | (ch2 & B8(00010000) ? color1 : color0);
							*pVideoRAM++=((ch2 & B8(00001000) ? color1 : color0) << 4) | (ch2 & B8(00000100) ? color1 : color0);
							*pVideoRAM++=((ch2 & B8(00000010) ? color1 : color0) << 4) | (ch2 & B8(00000001) ? color1 : color0);
							}
						}
					}
handle_sprites:
				{ // OVVIAMENTE sarebbe meglio gestirli riga per riga...!
				struct SPRITE_ATTR *sa;
				BYTE ssize=TMS9918Reg[1] & 2 ? 32 : 8,smag=TMS9918Reg[1] & 1 ? 16 : 8;
				BYTE j;
      
				sa=((struct SPRITE_ATTR *)&TMSVideoRAM[spriteAttrAddress]);
				for(i=0; i<32; i++) {
					struct SPRITE_ATTR *sa2;
        
					if(sa->ypos>=LAST_SPRITE_YPOS)
						continue;
        
					j=smag*(ssize==32 ? 2 : 1);
					sa2=sa+1;
					for(j=i+1; j<32; j++) {
						if(sa2->ypos < LAST_SPRITE_YPOS) {
            
							if((sa2->ypos>=sa->ypos && sa2->ypos<=sa->ypos+j) &&
								(sa2->xpos>=sa->xpos && sa2->xpos<=sa->xpos+j)) {
								// controllare solo i pixel accesi, a 1!
								TMS9918RegS |= B8(00100000);
								}
							// poi ci sarebbe il flag 5 sprite per riga!
							}
						sa2++;
						}
        
					p1=((BYTE*)&TMSVideoRAM[spritePatternAddress]) + ((WORD)sa->name*ssize);
					j=ssize;
					if(sa->ypos > 0xe1)     // Y diventa negativo..
						;
					if(sa->eclock)     // X diventa negativo..
						;
	//        setAddrWindow(sa->xpos/2,sa->ypos/2,8/2,8/2);
					color1=sa->color; color0=TMS9918Reg[7] & 0xf;
        
					for(py=0; py<ssize; py++) {
						pVideoRAM=(BYTE*)&TMSVideoRAM[0]+(py+sa->ypos)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN*2)))+sa->xpos;
						ch1=*p1++;
						if(smag==16) {
	/*            writedata16x2(graphColors[ch2 & B8(10000000 ? color1 : color0],graphColors[ch2 & B8(10000000 ? color1 : color0]);
							writedata16x2(graphColors[ch2 & B8(1000000 ? color1 : color0],graphColors[ch2 & B8(1000000 ? color1 : color0]);
							writedata16x2(graphColors[ch2 & B8(100000 ? color1 : color0],graphColors[ch2 & B8(100000 ? color1 : color0]);
							writedata16x2(graphColors[ch2 & B8(10000 ? color1 : color0],graphColors[ch2 & B8(10000 ? color1 : color0]);
							writedata16x2(graphColors[ch2 & B8(1000 ? color1 : color0],graphColors[ch2 & B8(1000 ? color1 : color0]);
							writedata16x2(graphColors[ch2 & B8(100 ? color1 : color0],graphColors[ch2 & B8(100 ? color1 : color0]);
							writedata16x2(graphColors[ch2 & B8(10 ? color1 : color0],graphColors[ch2 & B8(10 ? color1 : color0]);
							writedata16x2(graphColors[ch2 & B8(1 ? color1 : color0],graphColors[ch2 & B8(1 ? color1 : color0]);*/
							*pVideoRAM++=((ch1 & B8(10000000) ? color1 : color0) << 4) | (ch1 & B8(10000000) ? color1 : color0);
							*pVideoRAM++=((ch1 & B8(01000000) ? color1 : color0) << 4) | (ch1 & B8(01000000) ? color1 : color0);
							*pVideoRAM++=((ch1 & B8(00100000) ? color1 : color0) << 4) | (ch1 & B8(00100000) ? color1 : color0);
							*pVideoRAM++=((ch1 & B8(00010000) ? color1 : color0) << 4) | (ch1 & B8(00010000) ? color1 : color0);
							*pVideoRAM++=((ch1 & B8(00001000) ? color1 : color0) << 4) | (ch1 & B8(00001000) ? color1 : color0);
							*pVideoRAM++=((ch1 & B8(00000100) ? color1 : color0) << 4) | (ch1 & B8(00000100) ? color1 : color0);
							*pVideoRAM++=((ch1 & B8(00000010) ? color1 : color0) << 4) | (ch1 & B8(00000010) ? color1 : color0);
							*pVideoRAM++=((ch1 & B8(00000001) ? color1 : color0) << 4) | (ch1 & B8(00000001) ? color1 : color0);
							}
						else {
	/*            writedata16(graphColors[ch2 & B8(10000000 ? color1 : color0]); 
							writedata16(graphColors[ch2 & B8(1000000 ? color1 : color0]); 
							writedata16(graphColors[ch2 & B8(100000 ? color1 : color0]); 
							writedata16(graphColors[ch2 & B8(10000 ? color1 : color0]); 
							writedata16(graphColors[ch2 & B8(1000 ? color1 : color0]); 
							writedata16(graphColors[ch2 & B8(100 ? color1 : color0]); 
							writedata16(graphColors[ch2 & B8(10 ? color1 : color0]); 
							writedata16(graphColors[ch2 & B8(1 ? color1 : color0]); */
							*pVideoRAM++=((ch1 & B8(10000000) ? color1 : color0) << 4) | (ch1 & B8(01000000) ? color1 : color0);
							*pVideoRAM++=((ch1 & B8(00100000) ? color1 : color0) << 4) | (ch1 & B8(00010000) ? color1 : color0);
							*pVideoRAM++=((ch1 & B8(00001000) ? color1 : color0) << 4) | (ch1 & B8(00000100) ? color1 : color0);
							*pVideoRAM++=((ch1 & B8(00000010) ? color1 : color0) << 4) | (ch1 & B8(00000001) ? color1 : color0);
							}
						j--;
						switch(j) {   // gestisco i "quadranti" sprite messi a cazzo...
							case 23:
								pVideoRAM=(BYTE*)&TMSVideoRAM[0]+(py+sa->ypos+8)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN*2)))+sa->xpos;
								break;
							case 15:
								p1=((BYTE*)&TMSVideoRAM[spritePatternAddress]) + ((WORD)sa->name*ssize) + 16;
	//              setAddrWindow(sa->xpos/2+8/2,sa->ypos/2,8/2,8/2);
								pVideoRAM=(BYTE*)&TMSVideoRAM[0]+(py+sa->ypos)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN*2)))+sa->xpos+8;
								break;
							case 7:
	//              setAddrWindow(sa->xpos/2+8/2,sa->ypos/2+8/2,8/2,8/2);
								pVideoRAM=(BYTE*)&TMSVideoRAM[0]+(py+sa->ypos+8)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN*2)))+sa->xpos+8;
								break;
							default:
								break;
							}
						}
          
					sa++;
					}
				}
				break;
			case 1:     // graphics 2
				for(py=rowIni; py<rowFin/3; py++) {    // 192 linee 
					p1=((BYTE*)&TMSVideoRAM[videoAddress]) + (py*32 /*HORIZ_SIZE/8*/);
					pVideoRAM=(BYTE*)&VideoRAM[0]+((py*8)+row1)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN*2)));
					for(px=0; px<32 /*HORIZ_SIZE/8*/; px++) {    // 256 pixel 
						ch1=*p1++;
						ch2=TMSVideoRAM[charAddress + (ch1*8) + (py & 7)];
						color=TMSVideoRAM[colorAddress+py];
						color1=color >> 4; color0=color & 0xf;

	/*          writedata16x2(graphColors[ch2 & B8(10000000 ? color1 : color0],
										graphColors[ch2 & B8(01000000 ? color1 : color0]);
						writedata16x2(graphColors[ch2 & B8(00100000 ? color1 : color0],
										graphColors[ch2 & B8(00010000 ? color1 : color0]);
						writedata16x2(graphColors[ch2 & B8(00001000 ? color1 : color0],
										graphColors[ch2 & B8(00000100 ? color1 : color0]);
						writedata16x2(graphColors[ch2 & B8(00000010 ? color1 : color0],
										graphColors[ch2 & B8(00000001 ? color1 : color0]);*/
							*pVideoRAM++=((ch2 & B8(10000000) ? color1 : color0) << 4) | (ch2 & B8(01000000) ? color1 : color0);
							*pVideoRAM++=((ch2 & B8(00100000) ? color1 : color0) << 4) | (ch2 & B8(00010000) ? color1 : color0);
							*pVideoRAM++=((ch2 & B8(00001000) ? color1 : color0) << 4) | (ch2 & B8(00000100) ? color1 : color0);
							*pVideoRAM++=((ch2 & B8(00000010) ? color1 : color0) << 4) | (ch2 & B8(00000001) ? color1 : color0);
						}
					}
				for(py=rowFin/3; py<(rowFin*2)/3; py++) {    //
					p1=((BYTE*)&TMSVideoRAM[videoAddress]) + (py*32 /*HORIZ_SIZE/8*/);
					pVideoRAM=(BYTE*)&VideoRAM[0]+((py*8)+row1)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN*2)));
					for(px=0; px<32 /*HORIZ_SIZE/8*/; px++) {    // 256 pixel 
						ch1=*p1++;
						ch2=TMSVideoRAM[charAddress +2048 + (ch1*8) + (py & 7)];
						color=TMSVideoRAM[colorAddress+2048+py];
						color1=color >> 4; color0=color & 0xf;

						*pVideoRAM++=((ch2 & B8(10000000) ? color1 : color0) << 4) | (ch2 & B8(01000000) ? color1 : color0);
						*pVideoRAM++=((ch2 & B8(00100000) ? color1 : color0) << 4) | (ch2 & B8(00010000) ? color1 : color0);
						*pVideoRAM++=((ch2 & B8(00001000) ? color1 : color0) << 4) | (ch2 & B8(00000100) ? color1 : color0);
						*pVideoRAM++=((ch2 & B8(00000010) ? color1 : color0) << 4) | (ch2 & B8(00000001) ? color1 : color0);
						}
					}
				for(py=(rowFin*2)/3; py<rowFin; py++) {    // 
					p1=((BYTE*)&TMSVideoRAM[videoAddress]) + (py*32 /*HORIZ_SIZE/8*/);
					pVideoRAM=(BYTE*)&VideoRAM[0]+((py*8)+row1)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN*2)));
					for(px=0; px<32 /*HORIZ_SIZE/8*/; px++) {    // 256 pixel 
						ch1=*p1++;
						ch2=TMSVideoRAM[charAddress +4096 + (ch1*8) + (py & 7)];
						color=TMSVideoRAM[colorAddress+4096+py];
						color1=color >> 4; color0=color & 0xf;

						*pVideoRAM++=((ch2 & B8(10000000) ? color1 : color0) << 4) | (ch2 & B8(01000000) ? color1 : color0);
						*pVideoRAM++=((ch2 & B8(00100000) ? color1 : color0) << 4) | (ch2 & B8(00010000) ? color1 : color0);
						*pVideoRAM++=((ch2 & B8(00001000) ? color1 : color0) << 4) | (ch2 & B8(00000100) ? color1 : color0);
						*pVideoRAM++=((ch2 & B8(00000010) ? color1 : color0) << 4) | (ch2 & B8(00000001) ? color1 : color0);
						}
					}
				goto handle_sprites;
				break;
			case 2:     // multicolor
				for(py=rowIni; py<rowFin; py+=4) {    // 48 linee diventano 96
					pVideoRAM=(BYTE*)&VideoRAM[0]+((py*8)+row1)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN*2)));
					p1=((BYTE*)&TMSVideoRAM[videoAddress]) + (py*16);
					ch1=*p1++;
					p2=((BYTE*)&TMSVideoRAM[charAddress+ch1]);
					for(px=0; px<HORIZ_SIZE; px+=2) {    // 64 pixel diventano 128
						ch2=*p2++;
						color=TMSVideoRAM[colorAddress+ch2];
						color1=color >> 4; color0=color & 0xf;
          
						// finire!!

	//          writedata16x2(graphColors[color1],graphColors[color0]);
						}
					}
				goto handle_sprites;
				break;
			case 4:     // text 32x24, ~120mS, O1 opp O2, 2/7/24
				color1=TMS9918Reg[7] >> 4; color0=TMS9918Reg[7] & 0xf;
				for(py=rowIni/8; py<rowFin/8; py++) {    // 192 linee(32 righe char) 
					p2=((BYTE*)&TMSVideoRAM[videoAddress]) + (py*40);
					// mettere bordo
					for(row1=0; row1<8; row1++) {    // 192 linee(32 righe char) 
						pVideoRAM=(BYTE*)&VideoRAM[0]+((py*8)+row1)*(((HORIZ_SIZE/2)+(HORIZ_OFFSCREEN*2)));
						p1=p2;
						for(px=0; px<40; px++) {    // 240 pixel 
							ch1=*p1++;
							ch2=TMSVideoRAM[charAddress + (ch1*8) + (row1)];

							*pVideoRAM++=((ch2 & B8(10000000) ? color1 : color0) << 4) | (ch2 & B8(01000000) ? color1 : color0);
							*pVideoRAM++=((ch2 & B8(00100000) ? color1 : color0) << 4) | (ch2 & B8(00010000) ? color1 : color0);
							*pVideoRAM++=((ch2 & B8(00001000) ? color1 : color0) << 4) | (ch2 & B8(00000100) ? color1 : color0);
							}
						}
					}
				break;
			}

do_plot:
		i=StretchDIBits(hDC,0,(rowIni*AppYSizeR)/(VERT_SIZE),doppiaDim ? AppXSize*2 : AppXSize,
			doppiaDim ? ((rowFin-rowIni)*AppYSizeR)/(VERT_SIZE/2) : ((rowFin-rowIni)*AppYSizeR)/(VERT_SIZE),
			0,rowFin,HORIZ_SIZE+HORIZ_OFFSCREEN*2,rowIni-rowFin,
			VideoRAM,bmI,DIB_RGB_COLORS,SRCCOPY);


		}
  }


VOID CALLBACK myTimerProc(HWND hwnd,UINT uMsg,UINT idEvent,DWORD dwTime) {
	int i;
	HDC hDC;
	static BYTE divider;

  divider++;
//  if(divider>=32) {   // 50 Hz per TOD  qua siamo a 50 Hz :)
//    divider=0;
//    }

  if(divider>=50) {   // ergo 1Hz
    divider=0;
		}


	}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	int wmId,wmEvent;
	PAINTSTRUCT ps;
	HDC hDC,hCompDC;
 	POINT pnt;
	HMENU hMenu;
	HRSRC hrsrc;
	HFILE myFile;
 	BOOL bGotHelp;
	int i,j,k,i1,j1,k1;
	long l;
	char myBuf[128];
	char *s,*s1;
	LOGBRUSH br;
	RECT rc;
	SIZE mySize;
	HFONT hOldFont;
	HPEN myPen,hOldPen;
	HBRUSH myBrush,hOldBrush;
	static int TimerCnt;
	HANDLE hROM;

	switch(message) { 
		case WM_COMMAND:
			wmId    = LOWORD(wParam); // Remember, these are...
			wmEvent = HIWORD(wParam); // ...different for Win32!

			switch (wmId) {
				case ID_APP_ABOUT:
					DialogBox(g_hinst,MAKEINTRESOURCE(IDD_ABOUT), hWnd, (DLGPROC)About);
					break;

				case ID_OPZIONI_DUMP:
					DialogBox(g_hinst,MAKEINTRESOURCE(IDD_DUMP), hWnd, (DLGPROC)Dump0);
					break;

				case ID_OPZIONI_DUMPDISP:
					DialogBox(g_hinst,MAKEINTRESOURCE(IDD_DUMP1), hWnd, (DLGPROC)Dump1);
					break;

				case ID_OPZIONI_RESET:
   				CPUPins |= DoReset;
					InvalidateRect(hWnd,NULL,TRUE);
  				SetWindowText(hStatusWnd,"<reset>");
					break;

				case ID_EMULATORE_CARTRIDGE:
					break;

				case ID_APP_EXIT:
					PostMessage(hWnd,WM_CLOSE,0,0l);
					break;

				case ID_FILE_NEW:
					break;

				case ID_FILE_OPEN:
					{
					OPENFILENAME ofn;       // common dialog box structure
					TCHAR szFile[260] = { 0 };       // if using TCHAR macros

					// Initialize OPENFILENAME
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = sizeof(szFile);
					ofn.lpstrFilter = "Binari\0*.BIN\0Tutti\0*.*\0";
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = NULL;
					ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

					if(GetOpenFileName(&ofn) == TRUE) {
						myFile=_lopen(ofn.lpstrFile /*"68kmemory.bin"*/,OF_READ);
						if(myFile != -1) {
							_lread(myFile,ram_seg,RAM_SIZE);
							_lclose(myFile);
							}
						else
							MessageBox(hWnd,"Impossibile caricare",
								szAppName,MB_OK|MB_ICONHAND);
						}
					}
					break;

				case ID_FILE_SAVE_AS:
					{
					OPENFILENAME ofn;       // common dialog box structure
					TCHAR szFile[260] = { 0 };       // if using TCHAR macros

					// Initialize OPENFILENAME
					ZeroMemory(&ofn, sizeof(ofn));
					ofn.lStructSize = sizeof(ofn);
					ofn.hwndOwner = hWnd;
					ofn.lpstrFile = szFile;
					ofn.nMaxFile = sizeof(szFile);
					ofn.lpstrFilter = "Binari\0*.BIN\0Tutti\0*.*\0";
					ofn.nFilterIndex = 1;
					ofn.lpstrFileTitle = NULL;
					ofn.nMaxFileTitle = 0;
					ofn.lpstrInitialDir = NULL;
					ofn.Flags = OFN_PATHMUSTEXIST;

					if(GetOpenFileName(&ofn) == TRUE) {
							// use ofn.lpstrFile
						myFile=_lcreat(ofn.lpstrFile /*"68kmemory.bin"*/,0);
						if(myFile != -1) {
							_lwrite(myFile,ram_seg,RAM_SIZE);
							_lclose(myFile);
							}
						else
							MessageBox(hWnd,"Impossibile caricare",
								szAppName,MB_OK|MB_ICONHAND);
						}

					}
					break;

				case ID_OPZIONI_DEBUG:
					debug=!debug;
					break;

				case ID_OPZIONI_TRACE:
//					_f.SR.Trace =!_f.SR.Trace ;
					break;

				case ID_OPZIONI_DIMENSIONEDOPPIA:
					doppiaDim=!doppiaDim;
					break;

				case ID_OPZIONI_AGGIORNA:
					InvalidateRect(hWnd,NULL,TRUE);
					break;

				case ID_EDIT_PASTE:
					break;

        case ID_HELP: // Only called in Windows 95
          bGotHelp = WinHelp(hWnd, APPNAME".HLP", HELP_FINDER,(DWORD)0);
          if(!bGotHelp) {
            MessageBox(GetFocus(),"Unable to activate help",
              szAppName,MB_OK|MB_ICONHAND);
					  }
					break;

				case ID_HELP_INDEX: // Not called in Windows 95
          bGotHelp = WinHelp(hWnd, APPNAME".HLP", HELP_CONTENTS,(DWORD)0);
		      if(!bGotHelp) {
            MessageBox(GetFocus(),"Unable to activate help",
              szAppName,MB_OK|MB_ICONHAND);
					  }
					break;

				case ID_HELP_FINDER: // Not called in Windows 95
          if(!WinHelp(hWnd, APPNAME".HLP", HELP_PARTIALKEY,
				 		(DWORD)(LPSTR)"")) {
						MessageBox(GetFocus(),"Unable to activate help",
							szAppName,MB_OK|MB_ICONHAND);
					  }
					break;

				case ID_HELP_USING: // Not called in Windows 95
					if(!WinHelp(hWnd, (LPSTR)NULL, HELP_HELPONHELP, 0)) {
						MessageBox(GetFocus(),"Unable to activate help",
							szAppName, MB_OK|MB_ICONHAND);
					  }
					break;

				default:
					return (DefWindowProc(hWnd, message, wParam, lParam));
			}
			break;

		case WM_NCRBUTTONUP: // RightClick on windows non-client area...
			if (IS_WIN95 && SendMessage(hWnd, WM_NCHITTEST, 0, lParam) == HTSYSMENU) {
				// The user has clicked the right button on the applications
				// 'System Menu'. Here is where you would alter the default
				// system menu to reflect your application. Notice how the
				// explorer deals with this. For this app, we aren't doing
				// anything
				return (DefWindowProc(hWnd, message, wParam, lParam));
			  }
			else {
				// Nothing we are interested in, allow default handling...
				return (DefWindowProc(hWnd, message, wParam, lParam));
			  }
      break;

      case WM_RBUTTONDOWN: // RightClick in windows client area...
        pnt.x = LOWORD(lParam);
        pnt.y = HIWORD(lParam);
        ClientToScreen(hWnd, (LPPOINT)&pnt);
        hMenu = GetSubMenu(GetMenu(hWnd),2);
        if(hMenu) {
          TrackPopupMenu(hMenu, 0, pnt.x, pnt.y, 0, hWnd, NULL);
          }
        break;

		case WM_PAINT:
			hDC=BeginPaint(hWnd,&ps);
			myPen=CreatePen(PS_SOLID,16,Colori[0]);
			br.lbStyle=BS_SOLID;
				br.lbColor=Colori[0];

			br.lbHatch=0;
			myBrush=CreateBrushIndirect(&br);
			SelectObject(hDC,myPen);
			SelectObject(hDC,myBrush);
//			SelectObject(hDC,hFont);
//			Rectangle(hDC,0,0,200,200);
			Rectangle(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom);

			UpdateScreen(hDC,0/*VERT_OFFSCREEN*/,VERT_SIZE+VERT_OFFSCREEN*2);
			DeleteObject(myPen);
			DeleteObject(myBrush);
			EndPaint(hWnd,&ps);
			break;        

		case WM_TIMER:
			TimerCnt++;
			break;

		case WM_SIZE:
			GetClientRect(hWnd,&rc);
			AppXSize=rc.right-rc.left;
			AppYSize=rc.bottom-rc.top;
			AppYSizeR=rc.bottom-rc.top-(GetSystemMetrics(SM_CYSIZEFRAME)*2);
			MoveWindow(hStatusWnd,0,rc.bottom-16,rc.right,16,TRUE);
			break;        

		case WM_KEYDOWN:
			decodeKBD(wParam,lParam,0);
			break;        

		case WM_KEYUP:
			decodeKBD(wParam,lParam,1);
			break;        

		case WM_LBUTTONDOWN:
			break;        

		case WM_LBUTTONUP:
			break;        

		case WM_MOUSEMOVE:
			break;        

		case WM_CREATE:
//			bInFront=GetPrivateProfileInt(APPNAME,"SempreInPrimoPiano",0,INIFile);

			bmI=GlobalAlloc(GPTR,sizeof(BITMAPINFO)+(sizeof(COLORREF)*16));
			bmI->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
			bmI->bmiHeader.biBitCount=4;
			bmI->bmiHeader.biClrImportant=bmI->bmiHeader.biClrUsed=16;
			bmI->bmiHeader.biCompression=BI_RGB;
			bmI->bmiHeader.biWidth=HORIZ_SIZE+HORIZ_OFFSCREEN*2;
			bmI->bmiHeader.biHeight=VERT_SIZE+VERT_OFFSCREEN*2;
			bmI->bmiHeader.biPlanes=1;
			bmI->bmiHeader.biXPelsPerMeter=bmI->bmiHeader.biYPelsPerMeter=0;
			for(i=0; i<bmI->bmiHeader.biClrUsed; i++) {
				bmI->bmiColors[i].rgbRed=GetRValue(Colori[i]);
				bmI->bmiColors[i].rgbGreen=GetGValue(Colori[i]);
				bmI->bmiColors[i].rgbBlue=GetBValue(Colori[i]);
				}
			hFont=CreateFont(12,6,0,0,FW_LIGHT,0,0,0,
				ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY,DEFAULT_PITCH | FF_MODERN, (LPSTR)"Courier New");
			hFont2=CreateFont(14,7,0,0,FW_LIGHT,0,0,0,
				ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
				DEFAULT_QUALITY,DEFAULT_PITCH | FF_SWISS, (LPSTR)"Arial");
			GetClientRect(hWnd,&rc);
			hStatusWnd = CreateWindow("static","",
				WS_BORDER | SS_LEFT | WS_CHILD,
				0,rc.bottom-16,AppXSize-GetSystemMetrics(SM_CXVSCROLL)-2*GetSystemMetrics(SM_CXSIZEFRAME),16,
				hWnd,(HMENU)1001,g_hinst,NULL);
			ShowWindow(hStatusWnd, SW_SHOW);
			GetClientRect(hWnd,&rc);
			AppYSize=rc.bottom-rc.top;
			AppYSizeR=AppYSize-16;
			SendMessage(hStatusWnd,WM_SETFONT,(WPARAM)hFont2,0);
			hPen1=CreatePen(PS_SOLID,1,RGB(255,255,255));
			br.lbStyle=BS_SOLID;
			br.lbColor=0x000000;
			br.lbHatch=0;
			hBrush=CreateBrushIndirect(&br);

//  memcpy(rom_seg,TI994A_BIN,0x1800);
  memcpy(grom_seg,TI994A_BIN_GROM0,0x1800);
  memcpy(grom_seg+0x2000,TI994A_BIN_GROM1,0x1800);
  memcpy(grom_seg+0x2000*2,TI994A_BIN_GROM2,0x1800);
  for(i=0; i<8192; i+=2) {
    rom_seg[i+1]=TI994A_BIN_U611[i/2];			// big-endian
    rom_seg[i]=TI994A_BIN_U610[i/2];
    }




			spoolFile=_lcreat("spoolfile.txt",0);
/*			{extern const unsigned char IBMBASIC[32768];
			spoolFile=_lcreat("ibmbasic.bin",0);
			_lwrite(spoolFile,IBMBASIC,32768);
			}*/

/*			_outp(0x37a,8);
			_outp(0x37a,0);
			*/

//			hTimer=SetTimer(NULL,0,1000/32,myTimerProc);  // (basato su Raster
			// non usato... fa schifo! era per refresh...
			hTimer=SetTimer(NULL,0,1000/50,myTimerProc);  // 
			initHW();
			ColdReset=1;
			break;

		case WM_QUERYENDSESSION:
#ifndef _DEBUG
			if(MessageBox(hWnd,"La chiusura del programma cancellerà la memoria del TMS9900. Continuare?",APPNAME,MB_OKCANCEL | MB_ICONSTOP | MB_DEFBUTTON2) == IDOK)
				return 1l;
			else 
				return 0l;
#else
			return 1l;
#endif
			break;

		case WM_CLOSE:
esciprg:          
#ifndef _DEBUG
			if(MessageBox(hWnd,"La chiusura del programma cancellerà la memoria del TMS9900. Continuare?",APPNAME,MB_OKCANCEL | MB_ICONSTOP | MB_DEFBUTTON2) == IDOK) {
			  DestroyWindow(hWnd);
			  }
#else
		  DestroyWindow(hWnd);
#endif
			return 0l;
			break;

		case WM_DESTROY:
//			WritePrivateProfileInt(APPNAME,"SempreInPrimoPiano",bInFront,INIFile);
			// Tell WinHelp we don't need it any more...
			KillTimer(hWnd,hTimer);
			_lclose(spoolFile);
	    WinHelp(hWnd,APPNAME".HLP",HELP_QUIT,(DWORD)0);
			GlobalFree(bmI);
			DeleteObject(hBrush);
			DeleteObject(hPen1);
			DeleteObject(hFont);
			DeleteObject(hFont2);
			PostQuitMessage(0);
			break;

   	case WM_INITMENU:
   	  CheckMenuItem((HMENU)wParam,ID_OPZIONI_DEBUG,MF_BYCOMMAND | (debug ? MF_CHECKED : MF_UNCHECKED));
//   	  CheckMenuItem((HMENU)wParam,ID_OPZIONI_TRACE,MF_BYCOMMAND | (_f.SR.Trace ? MF_CHECKED : MF_UNCHECKED));
   	  CheckMenuItem((HMENU)wParam,ID_OPZIONI_DIMENSIONEDOPPIA,MF_BYCOMMAND | (doppiaDim ? MF_CHECKED : MF_UNCHECKED));
			break;

		default:
			return (DefWindowProc(hWnd, message, wParam, lParam));
		}
	return (0);
	}



ATOM MyRegisterClass(CONST WNDCLASS *lpwc) {
	HANDLE  hMod;
	FARPROC proc;
	WNDCLASSEX wcex;

	hMod=GetModuleHandle("USER32");
	if(hMod != NULL) {

#if defined (UNICODE)
		proc = GetProcAddress (hMod, "RegisterClassExW");
#else
		proc = GetProcAddress (hMod, "RegisterClassExA");
#endif

		if(proc != NULL) {
			wcex.style         = lpwc->style;
			wcex.lpfnWndProc   = lpwc->lpfnWndProc;
			wcex.cbClsExtra    = lpwc->cbClsExtra;
			wcex.cbWndExtra    = lpwc->cbWndExtra;
			wcex.hInstance     = lpwc->hInstance;
			wcex.hIcon         = lpwc->hIcon;
			wcex.hCursor       = lpwc->hCursor;
			wcex.hbrBackground = lpwc->hbrBackground;
    	wcex.lpszMenuName  = lpwc->lpszMenuName;
			wcex.lpszClassName = lpwc->lpszClassName;

			// Added elements for Windows 95:
			wcex.cbSize = sizeof(WNDCLASSEX);
			wcex.hIconSm = LoadIcon(wcex.hInstance, "SMALL");
			
			return (*proc)(&wcex);//return RegisterClassEx(&wcex);
		}
	}
return (RegisterClass(lpwc));
}


BOOL InitApplication(HINSTANCE hInstance) {
  WNDCLASS  wc;
  HWND      hwnd;

  wc.style         = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc   = (WNDPROC)WndProc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 0;
  wc.hInstance     = hInstance;
  wc.hIcon         = LoadIcon(hInstance,MAKEINTRESOURCE(IDI_APP32));
  wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(GetStockObject(BLACK_BRUSH));

        // Since Windows95 has a slightly different recommended
        // format for the 'Help' menu, lets put this in the alternate menu like this:
  if(IS_WIN95) {
		wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU1);
    } else {
	  wc.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU1);
    }
  wc.lpszClassName = szAppName;

  if(IS_WIN95) {
	  if(!MyRegisterClass(&wc))
			return 0;
    }
	else {
	  if(!RegisterClass(&wc))
	  	return 0;
    }


  }

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
	
	g_hinst=hInstance;

	ghWnd = CreateWindow(szAppName, szTitle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPCHILDREN | WS_THICKFRAME,
		CW_USEDEFAULT, CW_USEDEFAULT, 
		AppXSize+GetSystemMetrics(SM_CXSIZEFRAME)*2,AppYSize+GetSystemMetrics(SM_CYSIZEFRAME)*2+GetSystemMetrics(SM_CYMENUSIZE)+GetSystemMetrics(SM_CYSIZE)+18,
		NULL, NULL, hInstance, NULL);

	if(!ghWnd) {
		return (FALSE);
	  }

	ShowWindow(ghWnd, nCmdShow);
	UpdateWindow(ghWnd);

	return (TRUE);
  }

//
//  FUNCTION: About(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for "About" dialog box
// 		This version allows greater flexibility over the contents of the 'About' box,
// 		by pulling out values from the 'Version' resource.
//
//  MESSAGES:
//
//	WM_INITDIALOG - initialize dialog box
//	WM_COMMAND    - Input received
//
//
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	static  HFONT hfontDlg;		// Font for dialog text
	static	HFONT hFinePrint;	// Font for 'fine print' in dialog
	DWORD   dwVerInfoSize;		// Size of version information block
	LPSTR   lpVersion;			// String pointer to 'version' text
	DWORD   dwVerHnd=0;			// An 'ignored' parameter, always '0'
	UINT    uVersionLen;
	WORD    wRootLen;
	BOOL    bRetCode;
	int     i;
	char    szFullPath[256];
	char    szResult[256];
	char    szGetName[256];
	DWORD	dwVersion;
	char	szVersion[40];
	DWORD	dwResult;

	switch (message) {
    case WM_INITDIALOG:
//			ShowWindow(hDlg, SW_HIDE);
			hfontDlg = CreateFont(14, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				VARIABLE_PITCH | FF_SWISS, "");
			hFinePrint = CreateFont(11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				VARIABLE_PITCH | FF_SWISS, "");
//			CenterWindow (hDlg, GetWindow (hDlg, GW_OWNER));
			GetModuleFileName(g_hinst, szFullPath, sizeof(szFullPath));

			// Now lets dive in and pull out the version information:
			dwVerInfoSize = GetFileVersionInfoSize(szFullPath, &dwVerHnd);
			if(dwVerInfoSize) {
				LPSTR   lpstrVffInfo;
				HANDLE  hMem;
				hMem = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize);
				lpstrVffInfo  = GlobalLock(hMem);
				GetFileVersionInfo(szFullPath, dwVerHnd, dwVerInfoSize, lpstrVffInfo);
				// The below 'hex' value looks a little confusing, but
				// essentially what it is, is the hexidecimal representation
				// of a couple different values that represent the language
				// and character set that we are wanting string values for.
				// 040904E4 is a very common one, because it means:
				//   US English, Windows MultiLingual characterset
				// Or to pull it all apart:
				// 04------        = SUBLANG_ENGLISH_USA
				// --09----        = LANG_ENGLISH
				// ----04E4 = 1252 = Codepage for Windows:Multilingual
				lstrcpy(szGetName, "\\StringFileInfo\\040904E4\\");	 
				wRootLen = lstrlen(szGetName); // Save this position
			
				// Set the title of the dialog:
				lstrcat (szGetName, "ProductName");
				bRetCode = VerQueryValue((LPVOID)lpstrVffInfo,
					(LPSTR)szGetName,
					(LPVOID)&lpVersion,
					(UINT *)&uVersionLen);
//				lstrcpy(szResult, "About ");
//				lstrcat(szResult, lpVersion);
//				SetWindowText (hDlg, szResult);

				// Walk through the dialog items that we want to replace:
				for (i = DLG_VERFIRST; i <= DLG_VERLAST; i++) {
					GetDlgItemText(hDlg, i, szResult, sizeof(szResult));
					szGetName[wRootLen] = (char)0;
					lstrcat (szGetName, szResult);
					uVersionLen   = 0;
					lpVersion     = NULL;
					bRetCode      =  VerQueryValue((LPVOID)lpstrVffInfo,
						(LPSTR)szGetName,
						(LPVOID)&lpVersion,
						(UINT *)&uVersionLen);

					if(bRetCode && uVersionLen && lpVersion) {
					// Replace dialog item text with version info
						lstrcpy(szResult, lpVersion);
						SetDlgItemText(hDlg, i, szResult);
					  }
					else {
						dwResult = GetLastError();
						wsprintf (szResult, "Error %lu", dwResult);
						SetDlgItemText (hDlg, i, szResult);
					  }
					SendMessage (GetDlgItem (hDlg, i), WM_SETFONT, 
						(UINT)((i==DLG_VERLAST)?hFinePrint:hfontDlg),TRUE);
				  } // for


				GlobalUnlock(hMem);
				GlobalFree(hMem);

			}
		else {
				// No version information available.
			} // if (dwVerInfoSize)

    SendMessage(GetDlgItem (hDlg, IDC_LABEL), WM_SETFONT,
			(WPARAM)hfontDlg,(LPARAM)TRUE);

			// We are  using GetVersion rather then GetVersionEx
			// because earlier versions of Windows NT and Win32s
			// didn't include GetVersionEx:
			dwVersion = GetVersion();

			if (dwVersion < 0x80000000) {
				// Windows NT
				wsprintf (szVersion, "Microsoft Windows NT %u.%u (Build: %u)",
					(DWORD)(LOBYTE(LOWORD(dwVersion))),
					(DWORD)(HIBYTE(LOWORD(dwVersion))),
          (DWORD)(HIWORD(dwVersion)) );
				}
			else
				if (LOBYTE(LOWORD(dwVersion))<4) {
					// Win32s
				wsprintf (szVersion, "Microsoft Win32s %u.%u (Build: %u)",
  				(DWORD)(LOBYTE(LOWORD(dwVersion))),
					(DWORD)(HIBYTE(LOWORD(dwVersion))),
					(DWORD)(HIWORD(dwVersion) & ~0x8000) );
				}
			else {
					// Windows 95
				wsprintf(szVersion,"Microsoft Windows 95 %u.%u",
					(DWORD)(LOBYTE(LOWORD(dwVersion))),
					(DWORD)(HIBYTE(LOWORD(dwVersion))) );
				}

			SetWindowText(GetDlgItem(hDlg, IDC_OSVERSION), szVersion);
//			SetWindowPos(hDlg,NULL,GetSystemMetrics(SM_CXSCREEN)/2,GetSystemMetrics(SM_CYSCREEN)/2,0,0,SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOREDRAW | SWP_NOZORDER);
//			ShowWindow(hDlg, SW_SHOW);
			return (TRUE);

		case WM_COMMAND:
			if(wParam==IDOK || wParam==IDCANCEL) {
  		  EndDialog(hDlg,0);
			  return (TRUE);
			  }
			else if(wParam==3) {
				MessageBox(hDlg,"Se trovate utile questo programma, mandate un contributo!!\nVia Rivalta 39 - 10141 Torino (Italia)\n[Dario Greggio]","ADPM Synthesis sas",MB_OK);
			  return (TRUE);
			  }
			break;
		}

	return FALSE;
	}


LRESULT CALLBACK Dump0(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	char myBuf[256],myBuf1[32];
	int i,j;

	switch (message) {
    case WM_INITDIALOG:
			SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_RESETCONTENT,0,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST1),WM_SETFONT,(WPARAM)hFont,0);
			for(i=0; i<256; i+=16) {
				wsprintf(myBuf,"%04X: ",i);
				for(j=0; j<16; j++) {
					wsprintf(myBuf1,"%02X ",ram_seg[i+j]);
					strcat(myBuf,myBuf1);
				  }
				SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_ADDSTRING,0,(LPARAM)myBuf);
			  }
			return (TRUE);

		case WM_COMMAND:
			if(wParam==IDOK) {
  		  EndDialog(hDlg,0);
			  return (TRUE);
			  }
			break;
		}

	return FALSE;
	}

LRESULT CALLBACK Dump1(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
	char myBuf[256],myBuf1[32];
	int i,j;

	switch (message) {
    case WM_INITDIALOG:
			SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_RESETCONTENT,0,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST1),WM_SETFONT,(WPARAM)hFont,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST2),LB_RESETCONTENT,0,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST2),WM_SETFONT,(WPARAM)hFont,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST3),LB_RESETCONTENT,0,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST3),WM_SETFONT,(WPARAM)hFont,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST4),LB_RESETCONTENT,0,0);
			SendMessage(GetDlgItem(hDlg,IDC_LIST4),WM_SETFONT,(WPARAM)hFont,0);
			for(i=0xd000; i<0xd000+47; i+=16) {
				wsprintf(myBuf,"%04X: ",i);
				for(j=0; j<16; j++) {
					wsprintf(myBuf1,"%02X ",GetValue(i+j));
					strcat(myBuf,myBuf1);
				  }
				SendMessage(GetDlgItem(hDlg,IDC_LIST1),LB_ADDSTRING,0,(LPARAM)myBuf);
			  }
			for(i=0xd400; i<=0xd41C; i+=16) {
				wsprintf(myBuf,"%04X: ",i);
				for(j=0; j<16; j++) {
					wsprintf(myBuf1,"%02X ",GetValue(i+j));
					strcat(myBuf,myBuf1);
				  }
				SendMessage(GetDlgItem(hDlg,IDC_LIST2),LB_ADDSTRING,0,(LPARAM)myBuf);
			  }
			i=0xdc00;
			wsprintf(myBuf,"%04X: ",i);
			for(j=0; j<16; j++) {
				wsprintf(myBuf1,"%02X ",GetValue(i+j));
				strcat(myBuf,myBuf1);
			  }
			SendMessage(GetDlgItem(hDlg,IDC_LIST3),LB_ADDSTRING,0,(LPARAM)myBuf);
			i=0xdd00;
			wsprintf(myBuf,"%04X: ",i);
			for(j=0; j<16; j++) {
				wsprintf(myBuf1,"%02X ",GetValue(i+j));
				strcat(myBuf,myBuf1);
			  }
			SendMessage(GetDlgItem(hDlg,IDC_LIST4),LB_ADDSTRING,0,(LPARAM)myBuf);
			return (TRUE);

		case WM_COMMAND:
			if(wParam==IDOK) {
  		  EndDialog(hDlg,0);
			  return (TRUE);
			  }
			break;
		}

	return FALSE;
	}



int WritePrivateProfileInt(char *s, char *s1, int n, char *f) {
  int i;
  char myBuf[16];
  
  wsprintf(myBuf,"%d",n);
  WritePrivateProfileString(s,s1,myBuf,f);
  }

int ShowMe() {
	int i;
	char buffer[16];

	buffer[0]='A'^ 0x17;
	buffer[1]='D'^ 0x17;
	buffer[2]='P'^ 0x17;
	buffer[3]='M'^ 0x17;
	buffer[4]='-'^ 0x17;
	buffer[5]='G'^ 0x17;
	buffer[6]='.'^ 0x17;
	buffer[7]='D'^ 0x17;
	buffer[8]='a'^ 0x17;
	buffer[9]='r'^ 0x17;
	buffer[10]=' '^ 0x17;
	buffer[11]='2'^ 0x17;
	buffer[12]='2' ^ 0x17;
	buffer[13]=0;
	for(i=0; i<13; i++) buffer[i]^=0x17;
	MessageBox(GetDesktopWindow(),buffer,APPNAME,MB_OK | MB_ICONEXCLAMATION);
	}


