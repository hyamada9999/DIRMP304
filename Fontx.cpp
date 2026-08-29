// Fontx.cpp : 
//

#include <stdio.h>
#include <tchar.h>
#include <ctype.h>
#include <mbctype.h>
#include <mbstring.h>
//#include <memory.h>
#include <windows.h>
//#include <time.h>
//#include <locale.h>
#include "Fontx.h"

#define	FONT_FILE_ZEN_16_16		_T("_FONT\\MYRCZ16B.FNT")
#define FONT_FILE_HAN_8_16		_T("_FONT\\MYRCH16B.FNT")

#define FONT_FILE_ZEN_6_8		_T("_FONT\\K6X8X.FNT")

#define	FONT_FILE_ZEN_12_12		_T("_FONT\\DRUG12Z1.FEF")
#define	FONT_FILE_HAN_6_12		_T("_FONT\\DRUG12HN.FEF")

#define FONT_FILE_HAN_8_8		_T("_FONT\\TKFNT8H.FNT")
#define FONT_FILE_ZEN_8_8		_T("_FONT\\MISAKI.FNT")

//#define OLED_WIDTH				100			// 表示器のドット幅

typedef struct _FontMetricsBlock {
	unsigned short	StartCode;		// 開始コード(SJIS文字コード)
	unsigned short	EndCode;		// 終了コード(SJIS文字コード)
	int				Num;			// コード数
	BYTE			*FontData;		// フォントデータ
} FONTMETRICSBLOCK;

typedef struct _FontMetrics {
	int		Width;		// フォント幅(dot)
	int		Height;		// フォント高さ(dot)
	int		BytesWidth;			// フォントイメージの横バイト数
	int		BytesPerChar;		// 1文字当たりバイト数(フォントデータ)
	int		BytesPerCharWrk;	// 1文字当たりバイト数(変換ワークデータ)
	int		Code;		// 文字コード
	int		Blocks;		// コードブロック数
	FONTMETRICSBLOCK	*Block;
} FONTMETRICS;


static FONTMETRICS		FontZen_16_16;
static FONTMETRICS		FontHan_8_16;
static FONTMETRICS		FontZen_6_8;
static FONTMETRICS		FontHan_6_8;
static FONTMETRICS		FontZen_12_12;
static FONTMETRICS		FontHan_6_12;
static FONTMETRICS		FontHan_8_8;
static FONTMETRICS		FontZen_8_8;


void TransAxisBytes(BYTE *from, BYTE *to);
BYTE *GetFontData(FONTMETRICS *Font, unsigned short sjis_code);

/*
 *	フォントファイルを読み取り、FONTMETRICSデータを作成する。
 */
BOOL GetFontMetrics(const TCHAR *FontFile, FONTMETRICS *FontMetrics){
	FILE	*fp;
	BYTE	buff[128];
	BYTE	work1[8], work[2];
	int		i, j, k;

//	if ((fp = _tfopen(FontFile, _T("rb"))) == NULL){
	if (_wfopen_s(&fp, FontFile, _T("rb")) != 0 || fp == NULL) {
			_tprintf(_T("フォントファイルオープンエラー\n"));
		return FALSE;
	}
	fread(buff, 17, 1, fp);
	if (strncmp((char *)buff, "FONTX2", 6) != 0){
	//if (_tcsncmp((char *)buff, _T("FONTX2"), 6) != 0){
		_tprintf(_T("FONTXでない\n"));
		return FALSE;
	}
	FontMetrics->Width = buff[14];
	FontMetrics->Height = buff[15];
	FontMetrics->Code = buff[16];
	if (FontMetrics->Code == 0){	// ANK
		FontMetrics->Blocks = 1;
		FontMetrics->BytesWidth = 1;
		FontMetrics->BytesPerCharWrk = 16;
		FontMetrics->BytesPerChar = FontMetrics->BytesWidth * FontMetrics->Height;
		FontMetrics->Block = (FONTMETRICSBLOCK *)malloc(FontMetrics->Blocks * sizeof(FONTMETRICSBLOCK));

		i = 0;
		FontMetrics->Block[i].StartCode = 0;
		FontMetrics->Block[i].EndCode = 0xFF;
		FontMetrics->Block[i].Num = FontMetrics->Block[i].EndCode - FontMetrics->Block[i].StartCode + 1;
		FontMetrics->Block[i].FontData = (BYTE *)malloc(FontMetrics->Block[i].Num * FontMetrics->BytesPerCharWrk);

		for (j=0; j<FontMetrics->Block[i].Num; j++){
			fread(buff, FontMetrics->BytesPerChar, 1, fp);

			// 上ブロック
			for (k=0; k<8; k++){
				work1[k] = buff[k];
			}
			TransAxisBytes(work1, FontMetrics->Block[i].FontData + (j * FontMetrics->BytesPerCharWrk));

			if (FontMetrics->Height > 8){
				// 下ブロック
				for (k=0; k<8; k++){
					work1[k] = buff[k + 8];
				}
				TransAxisBytes(work1, FontMetrics->Block[i].FontData + (j * FontMetrics->BytesPerCharWrk) + 8);
			}
		}

	} else {
		fread(buff, 1, 1, fp);
		FontMetrics->Blocks = buff[0];
		FontMetrics->BytesWidth = ((FontMetrics->Width + 7) / 8);
		FontMetrics->BytesPerCharWrk = 32;	// 12x12, 16x16フォント(FontDataバッファ中の１文字当たりバイト数)
		FontMetrics->BytesPerChar = FontMetrics->BytesWidth * FontMetrics->Height;
		FontMetrics->Block = (FONTMETRICSBLOCK *)malloc(FontMetrics->Blocks * sizeof(FONTMETRICSBLOCK));

		//_tprintf(_T("FontMetrics Width=%d\n"), FontMetrics->Width);
		//_tprintf(_T("FontMetrics BytesPerChar=%d\n"), FontMetrics->BytesPerChar);

		// コードブロックテーブル
		for (i=0; i<FontMetrics->Blocks; i++){
			fread(buff, 4, 1, fp);

			FontMetrics->Block[i].StartCode = (buff[0] | ((unsigned short)buff[1] << 8));
			FontMetrics->Block[i].EndCode = (buff[2] | ((unsigned short)buff[3] << 8));
			FontMetrics->Block[i].Num = FontMetrics->Block[i].EndCode - FontMetrics->Block[i].StartCode + 1;
			FontMetrics->Block[i].FontData = (BYTE *)malloc(FontMetrics->Block[i].Num * FontMetrics->BytesPerCharWrk);
			//_tprintf(_T("FontMetrics StartCode=%d num=%d\n"), FontMetrics->Block[i].StartCode, FontMetrics->Block[i].Num);
		}

		// フォントイメージ
		for (i=0; i<FontMetrics->Blocks; i++){
			for (j=0; j<FontMetrics->Block[i].Num; j++){
				fread(buff, FontMetrics->BytesPerChar, 1, fp);

				// 16x16フォントを１ブロック=8x8ドットの４ブロックに分割。
				// 
				// ABCDEFGH
				// IJKLMNOP
				// QRSTUVWX
				// abcdefgh
				// ijklmnop
				// qrstuvwx
				// 01234567
				// ｱｲｳｴｵｶｷｸ
				// 
				// フォントファイル中では ABCDEFGH, IJKLMNOP, ... で各バイトを構成している。これを
				// ｱ0qiaQIA, ｲ1rjbRJB, ... というバイト構成に変換する。ｱ,ｲ,...が上位ビットとなる。

				if (FontMetrics->BytesWidth == 2){
					// 左上ブロック
					for (k=0; k<8; k++){
						work1[k] = buff[2*k];
					}
					TransAxisBytes(work1, FontMetrics->Block[i].FontData + (j * FontMetrics->BytesPerCharWrk));
					// 右上ブロック
					for (k=0; k<8; k++){
						work1[k] = buff[2*k + 1];
					}
					TransAxisBytes(work1, FontMetrics->Block[i].FontData + (j * FontMetrics->BytesPerCharWrk) + 8);
					if (FontMetrics->Height > 8){
						// 左下ブロック
						for (k=0; k<8; k++){	//	12x12フォントでは4
							work1[k] = buff[16 + 2*k];
						}
						TransAxisBytes(work1, FontMetrics->Block[i].FontData + (j * FontMetrics->BytesPerCharWrk) + 16);
						// 右下ブロック
						for (k=0; k<8; k++){	//	12x12フォントでは4
							work1[k] = buff[16 + 2*k + 1];
						}
						TransAxisBytes(work1, FontMetrics->Block[i].FontData + (j * FontMetrics->BytesPerCharWrk) + 24);
					}
				} else if (FontMetrics->BytesWidth == 1){
					// 上ブロック
					for (k=0; k<8; k++){
						work1[k] = buff[k];
					}
					TransAxisBytes(work1, FontMetrics->Block[i].FontData + (j * FontMetrics->BytesPerCharWrk));
					// 下ブロック(?)
					if (FontMetrics->Height > 8){
						// 必要になったら作成
					}
				}

				//_tprintf(_T("FontMetrics i=%d j=%d\n"), i, j);
			}
		}
	}

	fclose(fp);

	return TRUE;
}

/*
 *	２バイト用フォントデータから１バイト用フォントデータを作成する。
 *	ソースとなる２バイトフォントは8x8以下を想定している。
 */
BOOL MakeFontMetricsHan(FONTMETRICS *FontMetricsFrom, FONTMETRICS *FontMetrics){
	int		i;
	unsigned int c;
	unsigned int m;
	BYTE			*fontbyte;

	FontMetrics->Width = FontMetricsFrom->Width;
	FontMetrics->Height = FontMetricsFrom->Height;
	FontMetrics->Code = 0;

	FontMetrics->Blocks = 1;
	FontMetrics->BytesWidth = 1;
	FontMetrics->BytesPerCharWrk = 16;
	FontMetrics->BytesPerChar = FontMetrics->BytesWidth * FontMetrics->Height;
	FontMetrics->Block = (FONTMETRICSBLOCK *)malloc(FontMetrics->Blocks * sizeof(FONTMETRICSBLOCK));

	i = 0;
	FontMetrics->Block[i].StartCode = 0;
	FontMetrics->Block[i].EndCode = 0xFF;
	FontMetrics->Block[i].Num = FontMetrics->Block[i].EndCode - FontMetrics->Block[i].StartCode + 1;
	FontMetrics->Block[i].FontData = (BYTE *)malloc(FontMetrics->Block[i].Num * FontMetrics->BytesPerCharWrk);

	for (c=0; c<=0xFF; c++){
		if ((c >= 0x20 && c <= 0x7E) || (c >= 0xA1 && c<= 0xDF)){
			m = _mbbtombc(c);		// 半角文字に対応する全角文字(SJIS)
			//if (c <= 0x7E) printf("_mbbtombc %x -> %x\n", c, m);
		} else {
			m = 0x20;
		}
		fontbyte = GetFontData(FontMetricsFrom, m);
		if (fontbyte != NULL){
			memcpy(FontMetrics->Block[i].FontData + (c * FontMetrics->BytesPerCharWrk), fontbyte, FontMetrics->BytesPerCharWrk);
		} else {
			memset(FontMetrics->Block[i].FontData + (c * FontMetrics->BytesPerCharWrk), 0, FontMetrics->BytesPerCharWrk);
		}
	}

	return TRUE;
}


/*
 *	長さ８のバイト配列のビット位置を縦横入れ替え
 */
void TransAxisBytes(BYTE *from, BYTE *to){
	int		i, j;

	for (i=0; i<8; i++) to[i] = 0;
	
	to[0] |= ((from[0] & 0x80) >> 7);
	to[1] |= ((from[0] & 0x40) >> 6);
	to[2] |= ((from[0] & 0x20) >> 5);
	to[3] |= ((from[0] & 0x10) >> 4);
	to[4] |= ((from[0] & 0x08) >> 3);
	to[5] |= ((from[0] & 0x04) >> 2);
	to[6] |= ((from[0] & 0x02) >> 1);
	to[7] |= ((from[0] & 0x01) >> 0);

	to[0] |= ((from[1] & 0x80) >> 6);
	to[1] |= ((from[1] & 0x40) >> 5);
	to[2] |= ((from[1] & 0x20) >> 4);
	to[3] |= ((from[1] & 0x10) >> 3);
	to[4] |= ((from[1] & 0x08) >> 2);
	to[5] |= ((from[1] & 0x04) >> 1);
	to[6] |= ((from[1] & 0x02) >> 0);
	to[7] |= ((from[1] & 0x01) << 1);

	to[0] |= ((from[2] & 0x80) >> 5);
	to[1] |= ((from[2] & 0x40) >> 4);
	to[2] |= ((from[2] & 0x20) >> 3);
	to[3] |= ((from[2] & 0x10) >> 2);
	to[4] |= ((from[2] & 0x08) >> 1);
	to[5] |= ((from[2] & 0x04) >> 0);
	to[6] |= ((from[2] & 0x02) << 1);
	to[7] |= ((from[2] & 0x01) << 2);

	to[0] |= ((from[3] & 0x80) >> 4);
	to[1] |= ((from[3] & 0x40) >> 3);
	to[2] |= ((from[3] & 0x20) >> 2);
	to[3] |= ((from[3] & 0x10) >> 1);
	to[4] |= ((from[3] & 0x08) >> 0);
	to[5] |= ((from[3] & 0x04) << 1);
	to[6] |= ((from[3] & 0x02) << 2);
	to[7] |= ((from[3] & 0x01) << 3);

	to[0] |= ((from[4] & 0x80) >> 3);
	to[1] |= ((from[4] & 0x40) >> 2);
	to[2] |= ((from[4] & 0x20) >> 1);
	to[3] |= ((from[4] & 0x10) >> 0);
	to[4] |= ((from[4] & 0x08) << 1);
	to[5] |= ((from[4] & 0x04) << 2);
	to[6] |= ((from[4] & 0x02) << 3);
	to[7] |= ((from[4] & 0x01) << 4);

	to[0] |= ((from[5] & 0x80) >> 2);
	to[1] |= ((from[5] & 0x40) >> 1);
	to[2] |= ((from[5] & 0x20) >> 0);
	to[3] |= ((from[5] & 0x10) << 1);
	to[4] |= ((from[5] & 0x08) << 2);
	to[5] |= ((from[5] & 0x04) << 3);
	to[6] |= ((from[5] & 0x02) << 4);
	to[7] |= ((from[5] & 0x01) << 5);

	to[0] |= ((from[6] & 0x80) >> 1);
	to[1] |= ((from[6] & 0x40) >> 0);
	to[2] |= ((from[6] & 0x20) << 1);
	to[3] |= ((from[6] & 0x10) << 2);
	to[4] |= ((from[6] & 0x08) << 3);
	to[5] |= ((from[6] & 0x04) << 4);
	to[6] |= ((from[6] & 0x02) << 5);
	to[7] |= ((from[6] & 0x01) << 6);

	to[0] |= ((from[7] & 0x80) >> 0);
	to[1] |= ((from[7] & 0x40) << 1);
	to[2] |= ((from[7] & 0x20) << 2);
	to[3] |= ((from[7] & 0x10) << 3);
	to[4] |= ((from[7] & 0x08) << 4);
	to[5] |= ((from[7] & 0x04) << 5);
	to[6] |= ((from[7] & 0x02) << 6);
	to[7] |= ((from[7] & 0x01) << 7);

}

BYTE *GetFontData(FONTMETRICS *Font, unsigned short sjis_code){
	int		i;

	for (i=0; i<Font->Blocks; i++){
		if (sjis_code >= Font->Block[i].StartCode && sjis_code <= Font->Block[i].EndCode){
			return (Font->Block[i].FontData + (sjis_code - Font->Block[i].StartCode) * Font->BytesPerCharWrk);
		}
	}

	return NULL;
}


BOOL InitFont(){
	if (GetFontMetrics(FONT_FILE_ZEN_16_16, &FontZen_16_16) == FALSE){
		_tprintf(_T("GetFontMetrics error %s\n"), FONT_FILE_ZEN_16_16);
		return FALSE;
	}
	if (GetFontMetrics(FONT_FILE_HAN_8_16, &FontHan_8_16) == FALSE){
		_tprintf(_T("GetFontMetrics error %s\n"), FONT_FILE_HAN_8_16);
		return FALSE;
	}
	if (GetFontMetrics(FONT_FILE_ZEN_6_8, &FontZen_6_8) == FALSE){
		_tprintf(_T("GetFontMetrics error %s\n"), FONT_FILE_ZEN_6_8);
		return FALSE;
	}
	MakeFontMetricsHan(&FontZen_6_8, &FontHan_6_8);

	if (GetFontMetrics(FONT_FILE_ZEN_12_12, &FontZen_12_12) == FALSE){
		_tprintf(_T("GetFontMetrics error %s\n"), FONT_FILE_ZEN_12_12);
		return FALSE;
	}
	if (GetFontMetrics(FONT_FILE_HAN_6_12, &FontHan_6_12) == FALSE){
		_tprintf(_T("GetFontMetrics error %s\n"), FONT_FILE_HAN_6_12);
		return FALSE;
	}
	if (GetFontMetrics(FONT_FILE_HAN_8_8, &FontHan_8_8) == FALSE){
		_tprintf(_T("GetFontMetrics error %s\n"), FONT_FILE_HAN_8_8);
		return FALSE;
	}
	if (GetFontMetrics(FONT_FILE_ZEN_8_8, &FontZen_8_8) == FALSE){
		_tprintf(_T("GetFontMetrics error %s\n"), FONT_FILE_ZEN_8_8);
		return FALSE;
	}
	return TRUE;
}

/*
 *	【MP3_08/09】文字列sjis_str(文字列長str_length:SJIS換算)のOLED表示データを作成する。
 *		line0, line1 : １行表示の場合、それぞれOLEDの上半分、下半分に表示するデータ、
 *					 : ２行表示の場合、すべての表示データをline0に作成。line1は未使用。
 *		size0, size1 : line0/line1のバッファサイズ
 *		*width : line0/line1内に作成された横方向ドット数
 *		*Scroll : 表示スタイル (0:１行表示スクロールなし、1:１行表示スクロールあり、2:２行表示スクロールなし、3:２行表示スクロールあり)
 */
void MakeOLEDdataOfString(BYTE *line0, int size0, BYTE *line1, int size1, unsigned char *sjis_str, int str_length, int *width, int *Scroll){
	int				i, j;
	int				x;
	unsigned short	sjis_char;
	BYTE			*fontbyte;
	BOOLEAN			HasZen;
	int				lines;
	FONTMETRICS		*pfzen;
	FONTMETRICS		*pfhan;
	int				line;
	int				shiftbit;		// １行表示するとき上下方向中間となるようにビットをシフトする
	BYTE			mask;

	_tprintf(_T("MakeOLEDdataOfString:[%s][%d]\n"), sjis_str, str_length);

	// ２バイト文字を含むか？
	HasZen = FALSE;
	for (i=0; i<str_length; i++){
		if (_ismbslead((unsigned char *)sjis_str, (unsigned char *)(sjis_str + i)) != 0){
			HasZen = TRUE;
			break;
		}
	}

	/*
	 * フォント＆スタイル決定ロジック
	 */
	if (str_length <= 12){
		pfzen = &FontZen_16_16;
		pfhan = &FontHan_8_16;
		*Scroll = 0;
		lines = 1;
		shiftbit = 0;
		mask = 0xFF;
		//_tprintf(_T("FontZen_16_16 FontHan_8_16 "));
	} else if (str_length <= 16){
		pfzen = &FontZen_12_12;
		pfhan = &FontHan_6_12;
		*Scroll = 0;
		lines = 1;
		shiftbit = 2;
		mask = 0x0F;
	} else if (HasZen == FALSE){	// ２バイト文字を含まない
		pfzen = NULL;
		if (str_length <= 24){
			pfhan = &FontHan_8_8;
			if (str_length <= 12){	// 高さ８ドットフォントで１行表示
				lines = 1;
				shiftbit = 4;
				*Scroll = 0;
			} else {				// 高さ８ドットフォントで２行表示
				lines = 2;
				shiftbit = 0;
				*Scroll = 2;
			}
		} else {					// 高さ８ドットフォントで２行表示
			pfhan = &FontHan_6_8;
			lines = 2;
			shiftbit = 0;
			*Scroll = 3;
		}
		mask = 0xFF;
	} else {		// 高さ12ドットフォントで１行表示。スクロールあり
		pfzen = &FontZen_12_12;
		pfhan = &FontHan_6_12;
		*Scroll = 1;
		lines = 1;
		shiftbit = 2;
		mask = 0x0F;
		//_tprintf(_T("FontZen_12_12 FontHan_6_12 "));
	}

	x = 0;

	if (lines == 1){
		for (i=0; i<str_length; i++){
			if (_ismbslead((unsigned char *)sjis_str, (unsigned char *)(sjis_str + i)) != 0){
				sjis_char = ((sjis_str[i] << 8) | sjis_str[i+1]);
				fontbyte = GetFontData(pfzen, sjis_char);
				if (fontbyte != NULL){
					for (j=0; j<pfzen->Width; j++){
						line0[x] = *(fontbyte + j);
						line1[x] = (*(fontbyte + 16 + j) & mask);
						if (shiftbit > 0){
							line1[x] = ((line1[x] << shiftbit) | (line0[x] >> (8 - shiftbit)));
							line0[x] <<= shiftbit;
						}
						x++;
						if (x == size0 || x == size1) break;
					}
				} else {
					_tprintf(_T(" 全角フォントデータ見つからない (%d,%x)\n"), i, sjis_char);
				}
				i++;
			} else {
				fontbyte = GetFontData(pfhan, sjis_str[i]);
				if (fontbyte != NULL){
					for (j=0; j<pfhan->Width; j++){
						line0[x] = *(fontbyte + j);
						if (pfhan->Height > 8){
							line1[x] = (*(fontbyte + 8 + j) & mask);
						} else {
							line1[x] = 0;
						}
						if (shiftbit > 0){
							line1[x] = ((line1[x] << shiftbit) | (line0[x] >> (8 - shiftbit)));
							line0[x] <<= shiftbit;
						}
						x++;
						if (x == size0 || x == size1) break;
					}
				} else {
					_tprintf(_T(" 半角フォントデータ見つからない (%d:%c)\n"), i, sjis_str[i]);
				}
			}

			if (x == size0 || x == size1) break;
		}

		// 右端の残り
		for (i=x; i<OLED_WIDTH; i++){
			line0[i] = 0;
			line1[i] = 0;
		}

		*width = x;
		if (*width < OLED_WIDTH) *width = OLED_WIDTH;
	} else {	// ２行表示（すべて半角が前提）
		line = 0;
		for (i=0; i<str_length; i++){
#if 0
			if (line == 0 && (x + pfhan->Width) > OLED_WIDTH){
				line = 1;
				x = 0;
			} else if (line == 1 && (x + pfhan->Width) > OLED_WIDTH) break;
#endif
			if ((x + pfhan->Width) > size0) break;
			fontbyte = GetFontData(pfhan, sjis_str[i]);
			if (fontbyte != NULL){
				for (j=0; j<pfhan->Width; j++){
					if (line == 0){
						line0[x] = *(fontbyte + j);
					} else {
						line1[x] = *(fontbyte + j);
					}
					x++;
					if (x == size0 || x == size1) break;
				}
			} else {
				_tprintf(_T(" 半角フォントデータ見つからない (%d:%c)\n"), i, sjis_str[i]);
			}

			if (x == size0 || x == size1) break;
		}

//		*width = OLED_WIDTH;
		*width = x;

		// 右端の残り
		for (i=x; i<size0; i++){
			line0[i] = 0;
		}
		for (i=0; i<size1; i++){
			line1[i] = 0;
		}
	}

	_tprintf(_T(" イメージ幅=%d\n"), *width);

}

/*
 *	【MP3_10】文字列sjis_str(文字列長str_length:SJIS換算)のLCD表示データを作成する。
 *		１バイト文字／２バイト文字ともに高さ12ドットのフォントを使う。
 *		１文字列を１行あたりline_lengthドット、最大２行で表現する。
 *		１行目をline0とline1の下位４ビットに、２行目をline1の上位４ビットとline2にセットする。(line0/1/2は最低line_lengthバイト長)
 */
int MakeLcdDataOfString_10(BYTE *line0, BYTE *line1, BYTE *line2, int line_length, unsigned char *sjis_str){
	int				i, j;
	int				x;
	FONTMETRICS		*pfzen;
	FONTMETRICS		*pfhan;
	unsigned short	sjis_char;
	BYTE			*fontbyte;
	BYTE			mask;
	int				str_length;
	int				line;

	str_length = _tcslen((char *)sjis_str);

	memset(line0, 0, line_length);
	memset(line1, 0, line_length);
	memset(line2, 0, line_length);

	pfhan = &FontHan_6_12;
	pfzen = &FontZen_12_12;
	mask = 0x0F;

	line = 0;
	x = 0;

	for (i=0; i<str_length; i++){
		if (_ismbslead((unsigned char *)sjis_str, (unsigned char *)(sjis_str + i)) != 0){
			sjis_char = ((sjis_str[i] << 8) | sjis_str[i+1]);
			fontbyte = GetFontData(pfzen, sjis_char);
			if (fontbyte != NULL){
				if ((x + pfzen->Width) > line_length){
					if (line == 0){
						line = 1;
						x = 0;
					} else break;
				}
				for (j=0; j<pfzen->Width; j++){
					if (line == 0){
						line0[x] = *(fontbyte + j);
						line1[x] = (*(fontbyte + 16 + j) & mask);
					} else {
						line1[x] |= ((*(fontbyte + j) & mask) << 4);
						line2[x] = (*(fontbyte + j) & ~mask) >> 4;
						line2[x] |= ((*(fontbyte + 16 + j) & mask) << 4);
					}
					x++;
				}
			}
			i++;
		} else {
			fontbyte = GetFontData(pfhan, sjis_str[i]);
			if (fontbyte != NULL){
				if ((x + pfzen->Width) > line_length){
					if (line == 0){
						line = 1;
						x = 0;
					} else break;
				}
				for (j=0; j<pfhan->Width; j++){
					if (line == 0){
						line0[x] = *(fontbyte + j);
						line1[x] = (*(fontbyte + 8 + j) & mask);
					} else {
						line1[x] |= ((*(fontbyte + j) & mask) << 4);
						line2[x] = (*(fontbyte + j) & ~mask) >> 4;
						line2[x] |= ((*(fontbyte + 8 + j) & mask) << 4);
					}
					x++;
				}
			}
		}

	}
			
	return x;
}

/*
 *	メニュー項目用のOLEDイメージを作成する。全角・半角を問わず、高さ８ドットのフォントで表現する。
 *	イメージのドット幅をsize0で指定し、これより長いときは切り捨てる(文字の途中であっても)。
 *	また、短いときは０で埋める。
 *	MP3_09用。
 */
void MakeOLEDdataShortMenu(BYTE *line0, int size0, unsigned char *sjis_str, int str_length){
	int				i, j;
	int				x;
	BOOLEAN			HasZen;
	FONTMETRICS		*pfzen;
	FONTMETRICS		*pfhan;
	unsigned short	sjis_char;
	BYTE			*fontbyte;

	// ２バイト文字を含むか？
	HasZen = FALSE;
	for (i=0; i<str_length; i++){
		if (_ismbslead((unsigned char *)sjis_str, (unsigned char *)(sjis_str + i)) != 0){
			HasZen = TRUE;
			break;
		}
	}

	pfhan = &FontHan_6_8;
	pfzen = &FontZen_8_8;

	x = 0;

	for (i=0; i<str_length; i++){
		if (_ismbslead((unsigned char *)sjis_str, (unsigned char *)(sjis_str + i)) != 0){
			sjis_char = ((sjis_str[i] << 8) | sjis_str[i+1]);
			fontbyte = GetFontData(pfzen, sjis_char);
			if (fontbyte != NULL){
				for (j=0; j<pfzen->Width; j++){
					line0[x] = *(fontbyte + j);
					x++;
					if (x == size0) break;
				}
			}
			i++;
		} else {
			fontbyte = GetFontData(pfhan, sjis_str[i]);
			if (fontbyte != NULL){
				for (j=0; j<pfhan->Width; j++){
					line0[x] = *(fontbyte + j);
					x++;
					if (x == size0) break;
				}
			}
		}

		if (x == size0) break;
	}
			
	// 右端の残り
	for (i=x; i<size0; i++){
		line0[i] = 0;
	}
}


/*
 *	メニュー項目用のOLEDイメージを作成する。全角・半角を問わず、高さ８ドットのフォントで表現する。
 *	イメージのドット幅をwidthで指定し、これより長いときは切り捨てる(文字の途中であっても)。
 *	また、短いときは０で埋める。
 */
void MakeOLEDdataOfMenu(BYTE *line, int width, unsigned char *sjis_str, int str_length){
	int				i, j;
	int				x;
	unsigned short	sjis_char;
	BYTE			*fontbyte;
	FONTMETRICS		*pfzen;
	FONTMETRICS		*pfhan;

	pfzen = &FontZen_8_8;
	pfhan = &FontHan_6_8;

	x = 0;

	for (i=0; i<str_length; i++){
		if (_ismbslead((unsigned char *)sjis_str, (unsigned char *)(sjis_str + i)) != 0){
			sjis_char = ((sjis_str[i] << 8) | sjis_str[i+1]);
			fontbyte = GetFontData(pfzen, sjis_char);
			if (fontbyte != NULL){
				for (j=0; j<pfzen->Width; j++){
					line[x] = *(fontbyte + j);
					x++;
					if (x == width) break;
				}
			} else {
				_tprintf(_T(" 全角フォントデータ見つからない (%d,%x)\n"), i, sjis_char);
			}
			i++;
		} else {
			fontbyte = GetFontData(pfhan, sjis_str[i]);
			if (fontbyte != NULL){
				for (j=0; j<pfhan->Width; j++){
					line[x] = *(fontbyte + j);
					x++;
					if (x == width) break;
				}
			} else {
				_tprintf(_T(" 半角フォントデータ見つからない (%d:%c)\n"), i, sjis_str[i]);
			}
		}
	}

	// 右端の残り
	for (i=x; i<width; i++){
		line[i] = 0;
	}
}


/*
 *	固定メッセージ用OLED表示データ生成
 *	メッセージは半角文字のみ対応。
 */
void MakeOLEDdataOfMessage(BYTE *line0, int size0, unsigned char *sjis_str, int *width){
	FONTMETRICS		*pfhan;
	int				str_length;
	int				i;
	int				j;
	int				x;
	BYTE			*fontbyte;

	pfhan = &FontHan_6_8;
	str_length = _tcslen((char *)sjis_str);
	x = 0;

	for (i=0; i<str_length; i++){
		fontbyte = GetFontData(pfhan, sjis_str[i]);
		if (fontbyte != NULL){
			for (j=0; j<pfhan->Width; j++){
				line0[x] = *(fontbyte + j);
				x++;
				if (x == size0) break;
			}
		} else {
			_tprintf(_T(" 半角フォントデータ見つからない (%d:%c)\n"), i, sjis_str[i]);
		}
	}

	*width = x;
}
