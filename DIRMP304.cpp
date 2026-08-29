// DIRMP304.cpp : 
//
/*
 *	ディレクトリ名の変更ロジック
 *		1.Ver4/6で、実名が２バイト文字を含むときエラーとする(事前に手変更)。
 *		2.実名から短縮名(8.3)を生成する
 *			1.実名が２バイト文字を含むときは連番形式の短縮名とする(Ver8以降)
 *			2.実名が２バイト文字を含まないときは、実名から最大８文字の英数字を短縮名とする
 *		3.短縮名が実名と異なるとき
 *			1.ディレクトリ名を短縮名に変更
 *			2.実名をRENDIR04.LOGに保存
 *			3.実名と同名のファイルを作成(RENDIR04.LOGの中を見なくても元のディレクトリ名がわかるように)
 *
 *	ディレクトリ名の表示名決定ロジック
 *		1.対象ディレクトリ下にRENDIR04.LOGが無い → ディレクトリの実名
 *		2.対象ディレクトリ下にRENDIR04.LOGがある
 *			1.Ver4/6で、RENDIR04.LOGから取得した名前が２バイト文字を含む → ディレクトリの実名
 *			2.これ以外の時 → RENDIR04.LOGから取得した名前
 *
 *	曲名決定ロジック
 *		1.表示名04.txtから取得できるとき → 取得名
 *		2.表示名04.txtから取得できないとき
 *			1.MP3ファイルのタグ情報から取得できるとき → 取得名
 *			2.MP3ファイルのタグ情報から取得できないとき
 *				1.オリジナルファイル名
 *				2.短縮名
 */

#include "stdafx.h"


#define		DIRLISTFILE				_T("\\DIRLSTWK.TXT")	// DIRコマンドの出力結果(dir /AD /ON /S /N /B) 
#define		FILELISTFILE			_T("\\FILLSTWK.TXT")	// DIRコマンドの出力結果(dir /B /ON ディレクトリパス)
#define		DIRSTRUCTUREFILE		_T("DIRLST04.DAT")		// DIR構成ファイル(Version=4,6)
#define		DIRSTRUCTUREFILE08		_T("DIRLST08.DAT")		// DIR構成ファイル(Version=8,9)
#define		DIRSTRUCTUREFILE10		_T("DIRLST10.DAT")		// DIR構成ファイル(Version=10)
#define		INDEXFILE10				_T("INDEX10.DAT")		// INDEXファイル(Version=10,12)
#define		SNDFILELIST				_T("SNDLST04.DAT")		// 音源ファイルリスト(Version=4,6)
#define		SNDFILELIST08			_T("SNDLST08.DAT")		// 音源ファイルリスト(Version=8,9)
#define		SNDFILELIST10			_T("SNDLST10.DAT")		// 音源ファイルリスト(Version=10)
#define		PITCHSHIFTER			_T("PSHFT131.DAT")		// VS10XX PITCH SHIFTER Rev1.31(Version=4)
#define		SPEANA					_T("SPEANA.DAT")		// VS10XX SPECTRUM ANALYZER(Version=8)
#define		HISTFILE				_T("HIST04.DAT")		// 履歴ファイル(全Version共通)
#define		VOLUMEFILE04			_T("VOLUME04.DAT")		// ボリュームファイル (Version=4)
#define		VOLUMEFILE06			_T("VOLUME06.DAT")		// ボリュームファイル (Version=6)
#define		VOLUMEFILE08			_T("VOLUME08.DAT")		// ボリュームファイル (Version=8)
#define		VOLUMEFILE09			_T("VOLUME09.DAT")		// ボリュームファイル (Version=9)
#define		VOLUMEFILE10			_T("VOLUME10.DAT")		// ボリュームファイル (Version=10)
#define		VOLUMEFILE12			_T("VOLUME12.DAT")		// ボリュームファイル (Version=12)
#define		ERRLOGFILE				_T("ERRLOG04.DAT")		// エラーログファイル
#define		ERRLOGFILE_BK			_T("ERRLOG04_BK.DAT")	// エラーログファイル（まっさらのエラーログファイルをこのファイル名で残しておく）
#define		DISPLAYNAMELIST			_T("表示名04.txt")		// "MP3ファイル名"と"1バイト文字による表示名"をタブで区切る(入力ファイル)
#define		MSGIMGFILE08			_T("MSGIMG08.DAT")		// メッセージイメージファイル (Version=8,9)

#define		DIRMP304LOG				_T("DIRMP304.LOG")		// 本PGの実行ログ
#define		RENAME04LOG				_T("RENDIR04.LOG")		// ディレクトリ名変更ログ (読み出す際に使用するファイル名)
#define		RENAME04LOGW			  L"RENDIR04.LOG"		// ディレクトリ名変更ログ (書き込む際に使用するファイル名。上と同名であること)
#define		RENFIL04LOG				_T("RENFIL04.LOG")		// MP3ファイル名変更ログ (短縮名と元の名をタブ区切り) (読み出す際に使用するファイル名)
#define		RENFIL04LOGW			  L"RENFIL04.LOG"		// MP3ファイル名変更ログ (短縮名と元の名をタブ区切り) (書き込む際に使用するファイル名。上と同名であること)

#define		MENUDAT10				_T("MNUDAT10.DAT")		// メニューデータファイル
#define		MENUDAT12				_T("MNUDAT12.DAT")		// メニューデータファイル

#define		CHARIMGDAT10			_T("CHRIMG10.DAT")		// 文字コード対LCDイメージファイル (Version=10)
#define		CHARIMG_FIRST			' '
#define		CHARIMG_LAST			0x7E

#define		MAXLEN_DIRNAME			12			// DIR名長
#define		MAXLEN_DISPLAYNAME		32			// DIRの表示名長
#define		MAXLEN_TITLE			256			// 曲名の取得最大長(音源ファイルリストに出力する長さ(ML_TITLENAME)より大。取得値に含まれるアルバム名を切り捨てる可能性があるので。

#define		BUFLEN					128
#define		MAX_DIRS				1000

// ディレクトリ構成解析作業用
typedef struct _DirList {
	TCHAR	fulldirbase[BUFLEN];		// DIRフルパス(ドライブ名付き)
	TCHAR	fulldir[BUFLEN];			// DIRフルパス
	TCHAR	parentdir[BUFLEN];			// 親DIRフルパス
	TCHAR	dirname[MAXLEN_DIRNAME + 1];			// パス無しDIR名(スペース埋めしない)
	TCHAR	displayname[MAXLEN_DISPLAYNAME + 1];	// DIRの表示名(スペース埋めしない)
	USHORT	recno_subdir_start;		// サブDIR開始行
	USHORT	num_subdir;				// サブDIR数
	USHORT	recno_parentdir;		// 親DIR行
	USHORT	recno_grandparentdir;	// 親親DIR行
	USHORT	recno_nextrandomplay;	// ランダム再生時、次のレコードNo
	USHORT	row_dirlist_txt;		// DIRLIST.TXT内行
	USHORT	num_sndfile;			// 音源ファイル数
	// 以下、インデックス作成用に追加
	TCHAR	displaynameuc[MAXLEN_DISPLAYNAME + 1];		// DIRの表示名(ASCII文字を大文字化UPPER CASE INDEX生成に使う)
	TCHAR	displaynameparent[MAXLEN_DISPLAYNAME + 1];	// DIRの表示名(スペース埋めしない) 親DIR
	int		same_uc_flag;			// 同一のdisplaynameucが存在するとき 1
	TCHAR	filler[19];
} DIRLIST;

DIRLIST		DirList[MAX_DIRS];
USHORT		ShffuleWork[MAX_DIRS];
int			DirNameNum;
USHORT		numsnddir;
USHORT		DirListDetailRecNum;	// DIR構成ファイルの明細レコード総数

// DIR構成ファイル　レコード長・項目別バイトサイズ
#define	RECLEN_DIRLIST04			128UL		// イメージ部分を除くレコード長
#define	MAXLEN_FILEPATH				60
#define MAXLEN_DIRPATH				(MAXLEN_FILEPATH - 13)		// 13 = 8.3名 + 区切り
#define DL_FULLDIR					61			// 以下各項目のバイト数
#define DL_LENFULLDIR				1
#define DL_DIRNAME					13	
#define DL_RECNO_SUBDIR_START		2
#define DL_NUM_SUBDIR				1
#define DL_RECNO_PARENTDIR			2
#define DL_RECNO_GRANDPARENTDIR		2
#define DL_RECNO_NEXTRANDOMDIR		2
#define DL_NUM_SNDFILE				1
#define DL_DISPLAYNAME				33
#define DL_FILLER_04				10
#define DL_WIDTH_DIRNMI				2
#define DL_SCROLL					1
#define DL_FILLER_08				7
#define	DL_RECNO_MENUIMG_START		2
#define DL_FILLER_09				5
#define	DL_DIRNAMEIMG_HALF			512			// OLED表示イメージの上または下半分
#define DL_WIDTH_MENUIMG			128			// 

// DIR構成ファイル 出力値
typedef struct _DirStructure {
	char	FullDir[MAXLEN_FILEPATH + 1];	// DIRフルパス
	char	LenFullDir;						// DIRフルパスの長さ
	char	DirName[MAXLEN_DIRNAME + 1];	// DIR名
	USHORT	RecnoSubdirStart;				// サブDIR開始レコードNo
	BYTE	NumSubdir;						// サブDIR数
	USHORT	RecnoParentdir;					// 親DIRレコードNo
	USHORT	RecnoGrandParentdir;			// 親親DIRレコードNo
	USHORT	RecnoNextRandomPlay;			// ランダム再生時、次のレコードNo
	BYTE	NumSndFile;						// 音源ファイル数
	char	DisplayName[MAXLEN_DISPLAYNAME + 1];			// ディレクトリのLCD表示名(スペース埋めする)(MP3_08ではそのままつかわない)
	USHORT	WidthDirNameImage;				// DIR名イメージのビット幅 (MP3_08のみ)
	BYTE	Scroll;							// 表示をスクロールするか
	USHORT	RecnoMenuImgStart;				// メニューイメージの開始レコード番号
	char	Filler[RECLEN_DIRLIST04];
} DIRSTRUCTURE04;

// 固定長ディレクトリリストのヘッダレコード出力値
typedef struct _DirStructureHead04 {
	USHORT	RecNum;				// 総レコード数（ただしヘッダーレコードは含まない）
	USHORT	SndDirNum;			// MP3を持つDIR数
	TCHAR	filler[RECLEN_DIRLIST04];
} DIRSTRUCTUREHEAD04;

// インデックスファイル
DIRLIST		DirListWork[MAX_DIRS];

typedef struct _IndexStructure10 {
	char	Index[8];
	BYTE	NumSubDir;
	USHORT	RecNoStart;
} INDEXSTRUCTURE10;

INDEXSTRUCTURE10		IndexList[255];
int		NumIndexes;


// 音源ファイルリスト
#define	RECLEN_MP3LIST04			64UL	// イメージ部分を除くレコード長
#define ML_FILENAME					13		// 以下各項目のバイト数
#define ML_TRACKNO					3
#define ML_TITLENAME				33
#define ML_FILLER_04				15
#define ML_WIDTH_SNDNMI				2
#define ML_SCROLL					1
#define ML_FILLER_08				12
#define	ML_TITMENAMEIMG_HALF		512			// OLED表示イメージの上または下半分
#define ML_WIDTH_MENUIMG			128			// 

// MP3ファイルリスト構造体
typedef struct _Mp3List {
	TCHAR	FileName[13];
	TCHAR	TrackNo[3];						// 曲番号
	TCHAR	TitleName[MAXLEN_TITLE + 1];	// 表示名
	USHORT	width_sndnmi;					// 曲名イメージのビット幅 (MP3_08のみ)
	BYTE	scroll;
	TCHAR	filler[RECLEN_MP3LIST04];
} MP3LIST04;

// MP3ファイル名前変更構造体
typedef struct _Mp3Rename {
	wchar_t		BeforeNameW[BUFLEN];
	wchar_t		AfterNameW[BUFLEN];
} MP3RENAME;

// 表示名リスト
typedef struct _Mp3DisplayNameList {
	TCHAR	ShortName[16];					// 短縮名(8.3形式)
	TCHAR	OriginalName[BUFLEN + 1];		// 元の名
	TCHAR	DisplayName[MAXLEN_TITLE + 1];	// 表示名
} MP3DISPLAYNAMELIST04;

MP3DISPLAYNAMELIST04	Mp3DispnameList[256];
int						Mp3DispnameListNum;
MP3DISPLAYNAMELIST04	Mp3RenameList[256];
int						Mp3RenameListNum;

// 再生履歴ファイル
#define MAX_HIST_SIZE       100	// 記録件数

typedef struct _HistFile  {
	USHORT		LastDir;		// 最後に再生していたディレクトリ(0,1,2,...ディレクトリ数-1)
	USHORT		LastTrack;		// 最後に再生していた曲(1,2,...,曲数)
	ULONG		LastByte;		// 最後に再生していた曲の再生進捗（バイト数）

	USHORT		NextRandomDir;	// ランダム再生において次の再生するディレクトリ
} HISTFILE04;

#define RECLEN_HISTFILE04         10UL
#define HS_LASTDIR                2		// 以下各項目のバイト数
#define HS_LASTTRACK              2
#define HS_LASTBYTE               4
#define HS_NEXTRANDOMDIR          2

// ボリュームファイルレコード長(04/06/08/09共通)
#define RECLEN_VOLUMEFILE		2		

// エラーログファイル
#define RECLEN_ERRLOG04         8
#define MAX_ERRLOG_COUNT        1000

// 表示名イメージ作成ワークサイズ
#define	IMAGE_WORK_LEN			1024

// メッセージイメージファイル
#define MS_MSGLEN				2			// メッセージイメージドット長
#define MS_MSGIMG				512			// メッセージイメージ本体

#define OLED_WIDTH_08			100			// MP3_08/MP3_09のOLED幅
#define LCD_WIDTH_10			128			// MP3_10/MP3_12のLCD幅

// "PITCH SHIFTER"を使っているのはMP3_04だけ
// VLSI Solutionから公開されている"VS10XX PITCH SHIFTER"の固定データ
#define	PITCHSHIFTER131_SIZE		1373

const unsigned short PitchShifter131[PITCHSHIFTER131_SIZE] = { /* Compressed plugin */
  0x0007, 0x0001, 0x8030, 0x0006, 0x0006, 0x2800, 0xa2c0, 0x0000, /*    0 */
  0x0024, 0x2800, 0x68c0, 0x0007, 0x0001, 0x8010, 0x0006, 0x0006, /*    8 */
  0x2a10, 0x374e, 0x2a00, 0x0cce, 0x2a00, 0x270e, 0x0007, 0x0001, /*   10 */
  0x8033, 0x0006, 0x00d2, 0x3009, 0x3857, 0x3e18, 0x3822, 0x3e18, /*   18 */
  0x780a, 0x3e10, 0x3801, 0x48b2, 0x3855, 0x3e10, 0x3801, 0x0000, /*   20 */
  0x800a, 0x3e14, 0x3811, 0x3e14, 0xb813, 0x0006, 0x0013, 0x0023, /*   28 */
  0xffd1, 0x0006, 0x0192, 0x3009, 0x0800, 0x4080, 0x8c10, 0x0030, /*   30 */
  0x0552, 0x2800, 0x1c85, 0xf400, 0x4401, 0x0006, 0x0093, 0x3e10, /*   38 */
  0xb803, 0x3e01, 0x0c40, 0x4000, 0x8cc4, 0x4010, 0x8c02, 0x0000, /*   40 */
  0x0001, 0x6010, 0x4012, 0x0006, 0x0113, 0x2800, 0x1458, 0x6428, /*   48 */
  0x8c80, 0x0004, 0x0013, 0x3283, 0x0024, 0x0006, 0x0193, 0x3009, /*   50 */
  0x0203, 0x3009, 0x0c02, 0xfe26, 0x0024, 0x48b6, 0x8841, 0xfe42, /*   58 */
  0x0024, 0x4db6, 0x0024, 0xffb0, 0x4003, 0x48b2, 0x0024, 0xffa6, /*   60 */
  0x8203, 0x40b2, 0x8f82, 0x0030, 0x0555, 0x3d10, 0x4c80, 0xfe26, /*   68 */
  0x0024, 0x48b6, 0x8bc1, 0xfe42, 0x0024, 0x4db6, 0x0024, 0xffb0, /*   70 */
  0x4003, 0x48b2, 0x0024, 0xffa6, 0x1bc4, 0x40b2, 0x8c02, 0x4290, /*   78 */
  0x3401, 0x3009, 0x2f00, 0x2800, 0x1d55, 0x3009, 0x0c00, 0x4000, /*   80 */
  0x4401, 0x4100, 0x0024, 0x0000, 0x0001, 0x6014, 0x4010, 0x0004, /*   88 */
  0x0001, 0x2800, 0x1d58, 0x4010, 0x0024, 0x2800, 0x1d40, 0xf400, /*   90 */
  0x4010, 0x3e00, 0x8200, 0x3a10, 0x0200, 0x3a00, 0x3803, 0x0006, /*   98 */
  0x0152, 0x6104, 0x8b00, 0x6090, 0x8901, 0x6014, 0xa800, 0x0006, /*   a0 */
  0x6653, 0x2800, 0x2008, 0xb880, 0x4402, 0x3009, 0x2b80, 0x3009, /*   a8 */
  0x08c0, 0x4090, 0x4402, 0x3009, 0x2800, 0x0006, 0x0012, 0x3009, /*   b0 */
  0x2810, 0x3009, 0x0c01, 0x6214, 0x0024, 0x0006, 0x0092, 0x2800, /*   b8 */
  0x2258, 0x0000, 0x0100, 0x0004, 0x0001, 0x4214, 0x0024, 0x3009, /*   c0 */
  0x0801, 0x4112, 0x8c10, 0x6010, 0x020c, 0x6204, 0x020c, 0x3083, /*   c8 */
  0x1803, 0x2800, 0x2449, 0x36f0, 0x820c, 0x3009, 0x2c10, 0x36f4, /*   d0 */
  0x9813, 0x36f4, 0x1811, 0x36f0, 0x1801, 0x2210, 0x0000, 0x36f5, /*   d8 */
  0x4024, 0x36f0, 0x1801, 0x36f8, 0x580a, 0x36f8, 0x1822, 0x0030, /*   e0 */
  0x0717, 0x2100, 0x0000, 0x3f05, 0xdbd7, 0x0007, 0x0001, 0x809c, /*   e8 */
  0x0006, 0x00cc, 0x3009, 0x3853, 0x3e18, 0x3823, 0x3e18, 0x780a, /*   f0 */
  0x3e10, 0x3801, 0x48b2, 0x0024, 0x3e10, 0x3801, 0x0000, 0x800a, /*   f8 */
  0x3e10, 0xb803, 0x3e11, 0x3810, 0x3e04, 0x7812, 0x0006, 0x0010, /*  100 */
  0x0006, 0x0191, 0x3009, 0x0400, 0x4080, 0x8012, 0x0030, 0x0553, /*  108 */
  0x2800, 0x3445, 0x3009, 0x0840, 0x0006, 0x0093, 0x32f3, 0x0c40, /*  110 */
  0x4000, 0x4481, 0x4010, 0x8843, 0xf400, 0x4011, 0x0004, 0x0001, /*  118 */
  0x6014, 0x8cc4, 0x0030, 0x0550, 0x2800, 0x2e91, 0x3009, 0x0f82, /*  120 */
  0x0ffc, 0x0010, 0x3183, 0x0024, 0x0030, 0x0550, 0x6428, 0x184c, /*  128 */
  0xfe27, 0xe6e7, 0x48b6, 0x8447, 0xfe4e, 0x8c87, 0x4db6, 0x0024, /*  130 */
  0xffbe, 0x8bc3, 0x48b2, 0x0024, 0xffae, 0x8f82, 0x40b2, 0x0024, /*  138 */
  0xfe26, 0x2041, 0x48b6, 0x87c7, 0xfe4e, 0x0024, 0x4db6, 0x8c87, /*  140 */
  0xffbe, 0x0024, 0x48b2, 0x0024, 0xffae, 0x0024, 0x40b2, 0x8c02, /*  148 */
  0x4290, 0x2001, 0x3009, 0x2c00, 0x2800, 0x34d5, 0x36f1, 0x9807, /*  150 */
  0x2800, 0x34c0, 0xf400, 0x4452, 0x3b10, 0x0bc0, 0x3b00, 0x0024, /*  158 */
  0x0006, 0x0150, 0x3223, 0x0300, 0x6090, 0x8101, 0x6014, 0x4484, /*  160 */
  0x0004, 0x0001, 0x2800, 0x37c8, 0x6414, 0xa300, 0xb880, 0x810c, /*  168 */
  0x3009, 0x2380, 0x3009, 0x00c0, 0x4090, 0x4484, 0x6414, 0xa000, /*  170 */
  0x0006, 0x6651, 0x2800, 0x3951, 0x0006, 0x0010, 0x0ffc, 0x0013, /*  178 */
  0x3283, 0x0024, 0xf400, 0x4484, 0x3009, 0x0400, 0x6408, 0xa012, /*  180 */
  0x0000, 0x0400, 0x2800, 0x3b58, 0x6404, 0x8412, 0x0004, 0x0002, /*  188 */
  0x4428, 0x0024, 0x6404, 0x0024, 0x0000, 0x0413, 0x2800, 0x3dc8, /*  190 */
  0x3283, 0x0024, 0xf400, 0x4480, 0x6014, 0x0024, 0x0ffc, 0x0013, /*  198 */
  0x2800, 0x3d91, 0x0000, 0x0024, 0x3283, 0x0024, 0x3009, 0x2412, /*  1a0 */
  0x36f4, 0x5812, 0x36f1, 0x1810, 0x36f0, 0x9803, 0x36f0, 0x1801, /*  1a8 */
  0x2210, 0x0000, 0x36f0, 0x1801, 0x36f8, 0x580a, 0x36f8, 0x1823, /*  1b0 */
  0x0030, 0x0713, 0x2100, 0x0000, 0x3b04, 0xdbd3, 0x0007, 0x0001, /*  1b8 */
  0x8102, 0x0006, 0x0040, 0x3613, 0x0024, 0x3e12, 0x8024, 0x3e10, /*  1c0 */
  0xb803, 0x3e14, 0x3811, 0x3e14, 0xb813, 0x3e13, 0x780e, 0x3e03, /*  1c8 */
  0xc024, 0x0001, 0x000a, 0x0006, 0x0c15, 0x3504, 0x0024, 0x0020, /*  1d0 */
  0x0b91, 0x3880, 0x0024, 0x3880, 0x4024, 0xbd86, 0x3410, 0x0008, /*  1d8 */
  0x2b91, 0x0006, 0x02d2, 0x0000, 0x0053, 0x0000, 0x058e, 0x2400, /*  1e0 */
  0x458e, 0xfe25, 0x0829, 0x5017, 0x0829, 0x000e, 0x7b91, 0x5016, /*  1e8 */
  0x020c, 0x4db6, 0x0001, 0x4d16, 0x1bcf, 0xf1d6, 0x980e, 0xf7d0, /*  1f0 */
  0x1bcd, 0x36f4, 0x9813, 0x36f4, 0x1811, 0x36f0, 0x9803, 0x2000, /*  1f8 */
  0x0000, 0x36f2, 0x8024, 0x0007, 0x0001, 0x8122, 0x0006, 0x0102, /*  200 */
  0x3613, 0x0024, 0x3e12, 0xb817, 0x3e12, 0x3815, 0x3e05, 0xb814, /*  208 */
  0x3635, 0x0024, 0x0000, 0x800a, 0x3e10, 0x3801, 0x3e10, 0xb803, /*  210 */
  0x3e11, 0x3805, 0x3e11, 0xb810, 0x3e04, 0x7812, 0x34d3, 0x0024, /*  218 */
  0x3400, 0x0024, 0xf200, 0x1102, 0xf200, 0x0024, 0xf200, 0x0024, /*  220 */
  0x3cf0, 0x0024, 0x291d, 0x2400, 0x0000, 0x1600, 0x0000, 0x4001, /*  228 */
  0x6012, 0x108c, 0x3ce0, 0x0024, 0x2800, 0x4f49, 0x0000, 0x0386, /*  230 */
  0x0000, 0x4000, 0x3423, 0x0024, 0x3ce0, 0x0024, 0x0000, 0x0041, /*  238 */
  0x3413, 0x0024, 0x34b0, 0x8024, 0xfe22, 0x1100, 0x48b6, 0x0024, /*  240 */
  0xad66, 0x0024, 0xfe02, 0x0024, 0x293d, 0xc480, 0x48b2, 0x0024, /*  248 */
  0x4c8a, 0x104c, 0x0000, 0x0041, 0x34f0, 0x0024, 0xfe02, 0x0024, /*  250 */
  0x0000, 0xf001, 0x6eba, 0x0024, 0xf040, 0x0024, 0x6012, 0x0024, /*  258 */
  0x0000, 0x0041, 0x2800, 0x5748, 0x0000, 0xf002, 0xf040, 0x104c, /*  260 */
  0x3400, 0xc024, 0xfe26, 0x0024, 0x48b6, 0x0024, 0xfe02, 0x0024, /*  268 */
  0x293d, 0xc480, 0x48b2, 0x0024, 0x4480, 0x33c0, 0x0000, 0xf004, /*  270 */
  0x2800, 0x5758, 0x0000, 0x0024, 0x003f, 0x1004, 0x0000, 0x0041, /*  278 */
  0x0006, 0x0090, 0xb884, 0x108c, 0x6896, 0xa004, 0x34e0, 0x0024, /*  280 */
  0xfe02, 0x0024, 0x291d, 0x0580, 0x48b2, 0x0024, 0x0006, 0x0110, /*  288 */
  0x3423, 0x23c0, 0x34f0, 0x0024, 0x3410, 0x2380, 0xb880, 0xa140, /*  290 */
  0x3009, 0x23c0, 0x3009, 0x2340, 0x3009, 0x0000, 0x4080, 0x0024, /*  298 */
  0x0000, 0x0411, 0x2800, 0x5cd5, 0x0000, 0x0024, 0x2800, 0x64c0, /*  2a0 */
  0x3ce4, 0x4024, 0x4080, 0x138c, 0x0000, 0x0001, 0x2800, 0x6158, /*  2a8 */
  0x0006, 0x0011, 0x0000, 0x0400, 0x6090, 0x108c, 0x3ce0, 0x0400, /*  2b0 */
  0x6014, 0x0024, 0x0006, 0x6652, 0x2800, 0x6091, 0x0004, 0x0001, /*  2b8 */
  0x6014, 0x0024, 0x0000, 0x0024, 0x2800, 0x64d1, 0x0000, 0x0024, /*  2c0 */
  0x3009, 0x0800, 0x2800, 0x64c0, 0x3009, 0x2400, 0x0000, 0x0400, /*  2c8 */
  0x6090, 0x108c, 0x6090, 0x0024, 0x3ce0, 0x0400, 0x6014, 0x0024, /*  2d0 */
  0x0006, 0x6652, 0x2800, 0x6451, 0x0004, 0x0001, 0x6014, 0x0024, /*  2d8 */
  0x0000, 0x0024, 0x2800, 0x64d1, 0x0000, 0x0024, 0x3009, 0x0800, /*  2e0 */
  0x3009, 0x2400, 0x3613, 0x108c, 0x291a, 0x8040, 0x34e4, 0x3810, /*  2e8 */
  0x0000, 0x0810, 0x291a, 0x7f40, 0x3009, 0x1bcc, 0x36f4, 0x5812, /*  2f0 */
  0x36f1, 0x9810, 0x36f1, 0x1805, 0x36f0, 0x9803, 0x36f0, 0x1801, /*  2f8 */
  0x3405, 0x9014, 0x36f3, 0x0024, 0x36f2, 0x1815, 0x2000, 0x0000, /*  300 */
  0x36f2, 0x9817, 0x0007, 0x0001, 0x81a3, 0x0006, 0x0102, 0x3613, /*  308 */
  0x0024, 0x3e12, 0xb817, 0x3e12, 0x3815, 0x3e05, 0xb814, 0x3615, /*  310 */
  0x0024, 0x0000, 0x800a, 0x3e10, 0x3801, 0x3e10, 0xb803, 0x3e11, /*  318 */
  0x3805, 0x3e14, 0x3811, 0x0006, 0x9510, 0x0006, 0x01d1, 0x3e04, /*  320 */
  0xb813, 0x3009, 0x0000, 0x3009, 0x0401, 0x6014, 0x0024, 0x0030, /*  328 */
  0x0312, 0x2800, 0x7095, 0x0000, 0x0024, 0x3200, 0x044c, 0x3009, /*  330 */
  0x0401, 0x6014, 0x0024, 0x0006, 0x0292, 0x2800, 0x7095, 0x0006, /*  338 */
  0x9251, 0x3009, 0x0400, 0x3009, 0x0801, 0x6014, 0x0024, 0x0000, /*  340 */
  0x0024, 0x2800, 0x8685, 0x0000, 0x0024, 0x0030, 0x0311, 0x3100, /*  348 */
  0x0024, 0x4080, 0x0024, 0x0010, 0x0000, 0x2800, 0x7255, 0x0000, /*  350 */
  0x0024, 0x3900, 0x0024, 0x3100, 0x0024, 0x4080, 0x0024, 0x0001, /*  358 */
  0xffc0, 0x2800, 0x78d8, 0x0030, 0x00d2, 0x3201, 0x0024, 0xb408, /*  360 */
  0x0024, 0x0006, 0x01d3, 0x2800, 0x7515, 0x0001, 0xf400, 0x0001, /*  368 */
  0x0c04, 0x4408, 0x0401, 0x3613, 0x2004, 0x0006, 0x9250, 0x6810, /*  370 */
  0xac44, 0x3009, 0x2c81, 0x3e10, 0x0000, 0x2900, 0x4880, 0x3e00, /*  378 */
  0x2c00, 0x36e3, 0x0000, 0x6090, 0x0024, 0x3009, 0x2000, 0x3009, /*  380 */
  0x0005, 0x459a, 0x0024, 0x2900, 0x0b80, 0x0000, 0x8688, 0x3201, /*  388 */
  0x0024, 0xb408, 0x0024, 0x0000, 0x0081, 0x2800, 0x7a55, 0x0006, /*  390 */
  0x08d3, 0x0001, 0x0c04, 0x0001, 0xf400, 0x4408, 0x8c00, 0x6012, /*  398 */
  0x184c, 0x0004, 0x0003, 0x2800, 0x7c15, 0x0000, 0x0024, 0x4448, /*  3a0 */
  0x0024, 0xb885, 0xe1e0, 0x0000, 0x0041, 0x3100, 0x0024, 0xfe02, /*  3a8 */
  0x0024, 0x293d, 0xc480, 0x48b2, 0x0024, 0x0006, 0x08d1, 0x0006, /*  3b0 */
  0x01d0, 0xffc0, 0x1bcc, 0x48b2, 0x0024, 0x4cc2, 0x0024, 0x4cc2, /*  3b8 */
  0x0024, 0x3009, 0x2001, 0x0000, 0x0081, 0x3009, 0x0400, 0x6012, /*  3c0 */
  0x0024, 0x0006, 0x9253, 0x2800, 0x8295, 0x0030, 0x0312, 0x3200, /*  3c8 */
  0x404c, 0x3613, 0x2001, 0x3e10, 0x4c01, 0xf212, 0x0024, 0x3e00, /*  3d0 */
  0x4024, 0x2900, 0x4880, 0x0000, 0x83c8, 0x3200, 0x404c, 0x3613, /*  3d8 */
  0x2001, 0x3e10, 0x4c01, 0x2900, 0x4880, 0x3e00, 0x4024, 0x3023, /*  3e0 */
  0x0c00, 0x36e3, 0x2340, 0x3009, 0x0000, 0x0006, 0x9510, 0x3009, /*  3e8 */
  0x2000, 0x3009, 0x0c00, 0x6090, 0x0024, 0x3009, 0x2c00, 0x3009, /*  3f0 */
  0x0c05, 0x2900, 0x0b80, 0x459a, 0x0024, 0x36f4, 0x9813, 0x36f4, /*  3f8 */
  0x1811, 0x36f1, 0x1805, 0x36f0, 0x9803, 0x36f0, 0x1801, 0x3405, /*  400 */
  0x9014, 0x36f3, 0x0024, 0x36f2, 0x1815, 0x2000, 0x0000, 0x36f2, /*  408 */
  0x9817, 0x0007, 0x0001, 0x8224, 0x0006, 0x00ce, 0x3613, 0x0024, /*  410 */
  0x3e12, 0xb817, 0x3e12, 0x3815, 0x3e05, 0xb814, 0x3615, 0x0024, /*  418 */
  0x0000, 0x800a, 0x3e10, 0xb803, 0x4194, 0xb805, 0x3e11, 0x0024, /*  420 */
  0x3e14, 0x7812, 0x3e14, 0xf80d, 0x3e03, 0xf80e, 0x0000, 0x0003, /*  428 */
  0x2800, 0xa055, 0x0006, 0x9251, 0x002b, 0x1102, 0xb58a, 0x8404, /*  430 */
  0x6ed6, 0x0024, 0x0010, 0x0003, 0x2800, 0xa048, 0x0030, 0x0311, /*  438 */
  0x3100, 0x8024, 0x6236, 0x0024, 0x0000, 0x0024, 0x2800, 0x9e05, /*  440 */
  0x0000, 0x0024, 0x3100, 0x8024, 0x4284, 0x0024, 0x0006, 0x0c53, /*  448 */
  0x2800, 0x9e09, 0x0000, 0x0024, 0xf100, 0x0012, 0xb888, 0x4491, /*  450 */
  0x3300, 0x8024, 0x0006, 0x0c13, 0x6404, 0x2c02, 0x0000, 0x0024, /*  458 */
  0x2800, 0x9518, 0x4094, 0x0024, 0x2400, 0x9482, 0x3613, 0x0024, /*  460 */
  0x3220, 0x3840, 0x2900, 0x4080, 0x32e0, 0x7801, 0x3243, 0x1bc1, /*  468 */
  0x6498, 0x0024, 0x3920, 0x1800, 0x36f3, 0x0024, 0x0006, 0x0c13, /*  470 */
  0xb888, 0x0c02, 0x0006, 0x0c53, 0x3b10, 0x8024, 0x3004, 0x8024, /*  478 */
  0x3300, 0x884c, 0x0006, 0x0c13, 0x6404, 0x2c02, 0x0000, 0x0024, /*  480 */
  0x2800, 0x9a18, 0x4094, 0x4491, 0x2400, 0x9982, 0x3613, 0x0024, /*  488 */
  0x3220, 0x3840, 0x2900, 0x4080, 0x32e0, 0x7801, 0x3243, 0x1bc1, /*  490 */
  0x6498, 0x0024, 0x3920, 0x1800, 0x36f3, 0x0024, 0x0000, 0x0083, /*  498 */
  0x0006, 0x0c13, 0x3300, 0x984c, 0x0006, 0x0c93, 0x3b00, 0x8024, /*  4a0 */
  0x0006, 0x08d3, 0x3009, 0x0c02, 0x6236, 0x0024, 0x0000, 0x0082, /*  4a8 */
  0x2800, 0x9d85, 0x0000, 0x0024, 0xb884, 0xac02, 0x0006, 0x0293, /*  4b0 */
  0x3009, 0x2c02, 0x2800, 0xa040, 0x3009, 0x1bcc, 0x0006, 0x08d2, /*  4b8 */
  0x3613, 0x0802, 0x4294, 0x0024, 0x0006, 0x0293, 0x2800, 0xa005, /*  4c0 */
  0x6894, 0x0024, 0xb884, 0xa802, 0x3009, 0x2c02, 0x3009, 0x1bcc, /*  4c8 */
  0x36f3, 0xd80e, 0x36f4, 0xd80d, 0x36f4, 0x5812, 0x36f1, 0x1805, /*  4d0 */
  0x36f0, 0x9803, 0x3405, 0x9014, 0x36f3, 0x0024, 0x36f2, 0x1815, /*  4d8 */
  0x2000, 0x0000, 0x36f2, 0x9817, 0x0007, 0x0001, 0x828b, 0x0006, /*  4e0 */
  0x004a, 0x3613, 0x0024, 0x3e12, 0xb817, 0x3e12, 0x3815, 0x3e05, /*  4e8 */
  0xb814, 0x3615, 0x0024, 0x3e10, 0xb811, 0x0006, 0x7311, 0x3e14, /*  4f0 */
  0xb813, 0x0006, 0x0c52, 0x0006, 0x0c13, 0xb884, 0xb800, 0x3900, /*  4f8 */
  0x8024, 0x0006, 0x0002, 0x0000, 0x8911, 0x3e10, 0x4024, 0x3009, /*  500 */
  0x3850, 0x3009, 0x3810, 0x291a, 0x8040, 0x0000, 0x0c90, 0x0000, /*  508 */
  0x0010, 0x291a, 0x7f40, 0x3009, 0x1bcc, 0x3a10, 0x9bd0, 0x0006, /*  510 */
  0x1002, 0xb880, 0x1bc1, 0x3a00, 0x8024, 0x0006, 0x7312, 0x0006, /*  518 */
  0x0002, 0x3b00, 0x9813, 0x3a04, 0x4024, 0x36f4, 0x8024, 0x36f0, /*  520 */
  0x9811, 0x3405, 0x9014, 0x36f3, 0x0024, 0x36f2, 0x1815, 0x2000, /*  528 */
  0x0000, 0x36f2, 0x9817, 0x0007, 0x0001, 0x5800, 0x0006, 0x0005, /*  530 */
  0x0000, 0x0c00, 0x0000, 0x0100, 0x0100, 0x0006, 0x8006, 0x0000, /*  538 */
  0x0007, 0x0001, 0x580b, 0x0006, 0x0018, 0xffd8, 0x0052, 0xff62, /*  540 */
  0x0116, 0xfe3a, 0x02c3, 0xfbd5, 0x0633, 0xf6b8, 0x0e89, 0xe5ee, /*  548 */
  0x511e, 0x511e, 0xe5ee, 0x0e89, 0xf6b8, 0x0633, 0xfbd5, 0x02c3, /*  550 */
  0xfe3a, 0x0116, 0xff62, 0x0052, 0xffd8,
};


// VS10XX SPECTRUM ANALYZER
#define SPEANA_PLUGIN_SIZE 1009

const unsigned short SpeAna[SPEANA_PLUGIN_SIZE] = { /* Compressed plugin */
  0x0007, 0x0001, 0x8050, 0x0006, 0x0004, 0x2800, 0x2f40, 0x0000, /*    0 */
  0x0024, 0x0007, 0x0001, 0x8052, 0x0006, 0x00d6, 0x3e12, 0xb817, /*    8 */
  0x3e12, 0x3815, 0x3e05, 0xb814, 0x3615, 0x0024, 0x0000, 0x800a, /*   10 */
  0x3e10, 0x3801, 0x0001, 0xe000, 0x3e10, 0xb803, 0x0000, 0x0303, /*   18 */
  0x3e11, 0x3805, 0x3e11, 0xb807, 0x3e14, 0x3812, 0xb884, 0x130c, /*   20 */
  0x3410, 0x4024, 0x4112, 0x10d0, 0x4010, 0x008c, 0x4010, 0x0024, /*   28 */
  0xf400, 0x4012, 0x3000, 0x3840, 0x3009, 0x3801, 0x0000, 0x0041, /*   30 */
  0xfe02, 0x0024, 0x2900, 0x82c0, 0x48b2, 0x0024, 0x36f3, 0x0844, /*   38 */
  0x6306, 0x8845, 0xae3a, 0x8840, 0xbf8e, 0x8b41, 0xac32, 0xa846, /*   40 */
  0xffc8, 0xabc7, 0x3e01, 0x7800, 0xf400, 0x4480, 0x6090, 0x0024, /*   48 */
  0x6090, 0x0024, 0xf400, 0x4015, 0x3009, 0x3446, 0x3009, 0x37c7, /*   50 */
  0x3009, 0x1800, 0x3009, 0x3844, 0x48b3, 0xe1e0, 0x4882, 0x4040, /*   58 */
  0xfeca, 0x0024, 0x5ac2, 0x0024, 0x5a52, 0x0024, 0x4cc2, 0x0024, /*   60 */
  0x48ba, 0x4040, 0x4eea, 0x4801, 0x4eca, 0x9800, 0xff80, 0x1bc1, /*   68 */
  0xf1eb, 0xe3e2, 0xf1ea, 0x184c, 0x4c8b, 0xe5e4, 0x48be, 0x9804, /*   70 */
  0x488e, 0x41c6, 0xfe82, 0x0024, 0x5a8e, 0x0024, 0x525e, 0x1b85, /*   78 */
  0x4ffe, 0x0024, 0x48b6, 0x41c6, 0x4dd6, 0x48c7, 0x4df6, 0x0024, /*   80 */
  0xf1d6, 0x0024, 0xf1d6, 0x0024, 0x4eda, 0x0024, 0x0000, 0x0fc3, /*   88 */
  0x2900, 0x82c0, 0x4e82, 0x0024, 0x4084, 0x130c, 0x0004, 0xe100, /*   90 */
  0x3440, 0x4024, 0x4010, 0x0024, 0xf400, 0x4012, 0x3200, 0x4024, /*   98 */
  0xb132, 0x0024, 0x4214, 0x0024, 0xf224, 0x0024, 0x6230, 0x0024, /*   a0 */
  0x0001, 0x0001, 0x2800, 0x28c9, 0x0000, 0x0024, 0xf400, 0x40c2, /*   a8 */
  0x3200, 0x0024, 0xff82, 0x0024, 0x48b2, 0x0024, 0xb130, 0x0024, /*   b0 */
  0x6202, 0x0024, 0x003f, 0xf001, 0x2800, 0x2bd1, 0x0000, 0x1046, /*   b8 */
  0xfe64, 0x0024, 0x48be, 0x0024, 0x2800, 0x2cc0, 0x3a01, 0x8024, /*   c0 */
  0x3200, 0x0024, 0xb010, 0x0024, 0xc020, 0x0024, 0x3a00, 0x0024, /*   c8 */
  0x36f4, 0x1812, 0x36f1, 0x9807, 0x36f1, 0x1805, 0x36f0, 0x9803, /*   d0 */
  0x36f0, 0x1801, 0x3405, 0x9014, 0x36f3, 0x0024, 0x36f2, 0x1815, /*   d8 */
  0x2000, 0x0000, 0x36f2, 0x9817, 0x0007, 0x0001, 0x80bd, 0x0006, /*   e0 */
  0x021c, 0x3613, 0x0024, 0x3e12, 0xb817, 0x3e12, 0x3815, 0x3e05, /*   e8 */
  0xb814, 0x3645, 0x0024, 0x0000, 0x800a, 0x3e10, 0xb803, 0x0000, /*   f0 */
  0x0242, 0x3e11, 0x3805, 0x3e11, 0xb807, 0x3e14, 0x7812, 0x3e14, /*   f8 */
  0xf80d, 0x3e03, 0xf80e, 0x6124, 0x0024, 0x0030, 0x01d2, 0x2800, /*  100 */
  0x3595, 0x4182, 0x0024, 0x3204, 0x4024, 0x3100, 0x8024, 0x0030, /*  108 */
  0x03d1, 0x3900, 0x8024, 0x3200, 0x8024, 0x6294, 0x0024, 0x2800, /*  110 */
  0x7000, 0x3a00, 0x8024, 0x3613, 0x0024, 0x2800, 0x3885, 0x0004, /*  118 */
  0xe6d2, 0x0004, 0xe051, 0x0000, 0x2fd2, 0x3100, 0x8803, 0x6238, /*  120 */
  0x1bcc, 0x0000, 0x0024, 0x2800, 0x4e85, 0x4194, 0x0024, 0x0004, /*  128 */
  0xe6d2, 0x3613, 0x0024, 0x0000, 0x0302, 0x0004, 0xe011, 0x3009, /*  130 */
  0x3850, 0x0004, 0xe010, 0x3009, 0x3840, 0x0000, 0x1800, 0x2900, /*  138 */
  0x8480, 0xb882, 0xb801, 0x0001, 0xe010, 0x0000, 0x1700, 0x2900, /*  140 */
  0x8780, 0xb882, 0x0024, 0x3900, 0x9bc1, 0x0000, 0x2fd1, 0x3009, /*  148 */
  0x1bc0, 0x3009, 0x1bd0, 0x3009, 0x0404, 0x0004, 0xe051, 0x2800, /*  150 */
  0x3e40, 0x3901, 0x0024, 0x4448, 0x0402, 0x4294, 0x0024, 0x6498, /*  158 */
  0x2402, 0x001f, 0x4002, 0x6424, 0x0024, 0x0004, 0xe011, 0x2800, /*  160 */
  0x3d91, 0x0000, 0x058e, 0x2400, 0x4dce, 0x0000, 0x0013, 0x0004, /*  168 */
  0xe051, 0x0004, 0xfa04, 0x3100, 0x8024, 0xf224, 0x44c5, 0x4458, /*  170 */
  0x0024, 0xf400, 0x4115, 0x3500, 0xc024, 0x623c, 0x0024, 0x0000, /*  178 */
  0x0024, 0x2800, 0x4e11, 0x0000, 0x0024, 0x4384, 0x184c, 0x3100, /*  180 */
  0x3800, 0x291b, 0xef80, 0xf200, 0x0024, 0x003f, 0xfec3, 0x4084, /*  188 */
  0x4491, 0x3113, 0x1bc0, 0xa234, 0x0024, 0x0000, 0x2003, 0x6236, /*  190 */
  0x2402, 0x0000, 0x1003, 0x2800, 0x4748, 0x0000, 0x0024, 0x003f, /*  198 */
  0xf803, 0x3100, 0x8024, 0xb236, 0x0024, 0x2800, 0x4d40, 0x3900, /*  1a0 */
  0xc024, 0x6236, 0x0024, 0x0000, 0x0803, 0x2800, 0x4988, 0x0000, /*  1a8 */
  0x0024, 0x003f, 0xfe03, 0x3100, 0x8024, 0xb236, 0x0024, 0x2800, /*  1b0 */
  0x4d40, 0x3900, 0xc024, 0x6236, 0x0024, 0x0000, 0x0403, 0x2800, /*  1b8 */
  0x4bc8, 0x0000, 0x0024, 0x003f, 0xff03, 0x3100, 0x8024, 0xb236, /*  1c0 */
  0x0024, 0x2800, 0x4d40, 0x3900, 0xc024, 0x6236, 0x0402, 0x003f, /*  1c8 */
  0xff83, 0x2800, 0x4d48, 0x0000, 0x0024, 0xb236, 0x0024, 0x3900, /*  1d0 */
  0xc024, 0xb884, 0x07cc, 0x3900, 0x88cc, 0x3313, 0x0024, 0x0004, /*  1d8 */
  0xe091, 0x4194, 0x2413, 0x0004, 0xe0d1, 0x2800, 0x7015, 0x0004, /*  1e0 */
  0xe6c2, 0x3423, 0x0024, 0x3c10, 0x8024, 0x3100, 0xc024, 0x4304, /*  1e8 */
  0x0024, 0x39f0, 0x8024, 0x3100, 0x8024, 0x3cf0, 0x8024, 0x0004, /*  1f0 */
  0xe6c2, 0xb884, 0x33c2, 0x3c20, 0x8024, 0x34d0, 0xc024, 0x6238, /*  1f8 */
  0x0024, 0x0000, 0x0024, 0x2800, 0x6698, 0x4396, 0x0024, 0x2400, /*  200 */
  0x6643, 0x0000, 0x0024, 0x3423, 0x0024, 0x34e4, 0x4024, 0x3123, /*  208 */
  0x0024, 0x3100, 0xc024, 0x4304, 0x0024, 0x4284, 0x2402, 0x0001, /*  210 */
  0xe002, 0x2800, 0x6449, 0x0000, 0x0024, 0x3613, 0x104c, 0x3004, /*  218 */
  0xb850, 0x4088, 0x1043, 0x4336, 0x1390, 0x4234, 0x0024, 0x4234, /*  220 */
  0x0024, 0x0000, 0x2003, 0x2900, 0x72c0, 0xf400, 0x4091, 0x3423, /*  228 */
  0x1bd0, 0x3404, 0x4024, 0x3113, 0x0024, 0x3100, 0x8024, 0x6236, /*  230 */
  0x0024, 0x0004, 0x0003, 0x2800, 0x5c58, 0x0000, 0x0024, 0x0000, /*  238 */
  0x03c4, 0x4396, 0x87cc, 0xb248, 0x0402, 0xfe08, 0x0024, 0x48be, /*  240 */
  0x0024, 0x4264, 0x4184, 0xb234, 0x0024, 0x0004, 0x0003, 0x3900, /*  248 */
  0x8024, 0x3404, 0x4024, 0x3123, 0x0024, 0x3100, 0x8024, 0x6236, /*  250 */
  0x0024, 0x0000, 0x4003, 0x2800, 0x5e88, 0x0000, 0x0024, 0xb884, /*  258 */
  0x878c, 0x3900, 0x8024, 0x34e4, 0x4024, 0x3123, 0x0024, 0x31e0, /*  260 */
  0x8024, 0x6236, 0x0402, 0x0000, 0x0024, 0x2800, 0x6448, 0x4284, /*  268 */
  0x0024, 0x0000, 0x0024, 0x2800, 0x6455, 0x0000, 0x0024, 0x3413, /*  270 */
  0x184c, 0x3410, 0x8024, 0x3e10, 0x8024, 0x34e0, 0xc024, 0x2900, /*  278 */
  0x1480, 0x3e10, 0xc024, 0xf400, 0x40d1, 0x003f, 0xff44, 0x36e3, /*  280 */
  0x048c, 0x3100, 0x8024, 0xfe44, 0x0024, 0x48ba, 0x0024, 0x3901, /*  288 */
  0x0024, 0x0000, 0x00c3, 0x3423, 0x0024, 0xf400, 0x4511, 0x34e0, /*  290 */
  0x8024, 0x4234, 0x0024, 0x39f0, 0x8024, 0x3100, 0x8024, 0x6294, /*  298 */
  0x0024, 0x3900, 0x8024, 0x0004, 0xe011, 0x6894, 0x04c3, 0xa234, /*  2a0 */
  0x0403, 0x6238, 0x0024, 0x0000, 0x0024, 0x2800, 0x7001, 0x0000, /*  2a8 */
  0x0024, 0xb884, 0x90cc, 0x39f0, 0x8024, 0x3100, 0x8024, 0xb884, /*  2b0 */
  0x3382, 0x3c20, 0x8024, 0x34d0, 0xc024, 0x6238, 0x0024, 0x0004, /*  2b8 */
  0xe112, 0x2800, 0x7018, 0x4396, 0x0024, 0x2400, 0x6fc3, 0x0000, /*  2c0 */
  0x0024, 0x0003, 0xf002, 0x3201, 0x0024, 0xb424, 0x0024, 0x0028, /*  2c8 */
  0x0002, 0x2800, 0x6ec5, 0x6246, 0x0024, 0x0004, 0x0003, 0x2800, /*  2d0 */
  0x6e81, 0x4434, 0x0024, 0x0000, 0x1003, 0x6434, 0x0024, 0x2800, /*  2d8 */
  0x6ec0, 0x3a00, 0x8024, 0x3a00, 0x8024, 0x3213, 0x104c, 0xf400, /*  2e0 */
  0x4511, 0x34f0, 0x8024, 0x6294, 0x0024, 0x3900, 0x8024, 0x36f3, /*  2e8 */
  0xd80e, 0x36f4, 0xd80d, 0x36f4, 0x5812, 0x36f1, 0x9807, 0x36f1, /*  2f0 */
  0x1805, 0x36f0, 0x9803, 0x3405, 0x9014, 0x36f3, 0x0024, 0x36f2, /*  2f8 */
  0x1815, 0x2000, 0x0000, 0x36f2, 0x9817, 0x0007, 0x0001, 0x13e8, /*  300 */
  0x0006, 0x000f, 0x0032, 0x004f, 0x007e, 0x00c8, 0x013d, 0x01f8, /*  308 */
  0x0320, 0x04f6, 0x07e0, 0x0c80, 0x13d8, 0x1f7f, 0x3200, 0x4f5f, /*  310 */
  0x61a8, 0x0006, 0x8008, 0x0000, 0x0007, 0x0001, 0x81cb, 0x0006, /*  318 */
  0x0080, 0x3e12, 0xb814, 0x0000, 0x800a, 0x3e10, 0x3801, 0x3e10, /*  320 */
  0xb803, 0x3e11, 0x7806, 0x3e11, 0xf813, 0x3e13, 0xf80e, 0x3e13, /*  328 */
  0x4024, 0x3e04, 0x7810, 0x449a, 0x0040, 0x0000, 0x0803, 0x2800, /*  330 */
  0x7d44, 0x30f0, 0x4024, 0x0fff, 0xfec2, 0xa020, 0x0024, 0x0fff, /*  338 */
  0xff02, 0xa122, 0x0024, 0x4036, 0x0024, 0x0000, 0x1fc2, 0xb326, /*  340 */
  0x0024, 0x0010, 0x4002, 0x4326, 0x4495, 0x4024, 0x40d2, 0x0000, /*  348 */
  0x0180, 0xa100, 0x4090, 0x0010, 0x0042, 0x4204, 0x0024, 0xbc82, /*  350 */
  0x4091, 0x459a, 0x0024, 0x0000, 0x0054, 0x2800, 0x7c44, 0xbd86, /*  358 */
  0x4093, 0x2400, 0x7c05, 0xfe01, 0x5e0c, 0x5c43, 0x5f2d, 0x5e46, /*  360 */
  0x0024, 0x5c56, 0x0024, 0x5e53, 0x5e0c, 0x5c43, 0x5f2d, 0x5e46, /*  368 */
  0x0024, 0x5c56, 0x0024, 0x5e52, 0x0024, 0x4cb2, 0x4405, 0x0010, /*  370 */
  0x4004, 0x654a, 0x9810, 0x0000, 0x0144, 0xa54a, 0x1bd1, 0x0004, /*  378 */
  0xe013, 0x3301, 0xc444, 0x687e, 0x2005, 0xad76, 0x8445, 0x4ed6, /*  380 */
  0x8784, 0x36f3, 0x64c2, 0xac72, 0x8785, 0x4ec2, 0xa443, 0x3009, /*  388 */
  0x2440, 0x3009, 0x2741, 0x36f3, 0xd80e, 0x36f1, 0xd813, 0x36f1, /*  390 */
  0x5806, 0x36f0, 0x9803, 0x36f0, 0x1801, 0x2000, 0x0000, 0x36f2, /*  398 */
  0x9814, 0x0007, 0x0001, 0x820b, 0x0006, 0x000e, 0x4c82, 0x0024, /*  3a0 */
  0x0000, 0x0024, 0x2000, 0x0005, 0xf5c2, 0x0024, 0x0000, 0x0980, /*  3a8 */
  0x2000, 0x0000, 0x6010, 0x0024, 0x0007, 0x0001, 0x8212, 0x0006, /*  3b0 */
  0x0018, 0x4080, 0x184c, 0x3e03, 0x784f, 0x2800, 0x8685, 0x3e03, /*  3b8 */
  0xb810, 0x4090, 0x0024, 0x2400, 0x8640, 0x0000, 0x0024, 0x3810, /*  3c0 */
  0x4024, 0x36f3, 0x9810, 0x36f3, 0x580f, 0x2000, 0x0000, 0x0000, /*  3c8 */
  0x0024, 0x0007, 0x0001, 0x821e, 0x0006, 0x0018, 0x4080, 0x184c, /*  3d0 */
  0x3e03, 0x784f, 0x2800, 0x8985, 0x3e03, 0xb810, 0x4090, 0x0024, /*  3d8 */
  0x2400, 0x8940, 0x0000, 0x0024, 0x3009, 0x2041, 0x36f3, 0x9810, /*  3e0 */
  0x36f3, 0x580f, 0x2000, 0x0000, 0x0000, 0x0024, 0x000a, 0x0001, /*  3e8 */
  0x0050,
};


FILE	*FpLog;
BOOL	RenameErrFlg;
TCHAR	TempDir[MAX_PATH];
int		Version;			// MP3_xxのxxの部分
int		TitleNameRule;
BYTE	image0[IMAGE_WORK_LEN];		// OLEDイメージデータ作成ワーク
BYTE	image1[IMAGE_WORK_LEN];
BYTE	image2[IMAGE_WORK_LEN];
BYTE	shortimage0[DL_WIDTH_MENUIMG];
BYTE	shortimage1[ML_WIDTH_MENUIMG];

BOOL ReadDirectoryStructure();
//void SetRecnoMenuStart();
BOOL SetRandomOrder();
BOOL WriteDirStructureList();
void CreateIndexList();
USHORT MakeSndFileList(TCHAR *inFolder, TCHAR *outAlbumName);
void GetMP3AlbumInfo(TCHAR *folder, TCHAR *sndfile, TCHAR *outAlbumName, TCHAR *outTitleName);
void AdjustDirAndFileNamesW(wchar_t *CurrentDirW);
BOOL CheckName(TCHAR *name);
BOOL CheckNameW(wchar_t *name);
void MakeShortDirName(TCHAR *name, TCHAR *shortname);
void MakeShortDirNameW(wchar_t *name, wchar_t *shortname, int *dirno);
BOOL HasMultibyteChar(TCHAR *str);
BOOL HasMultibyteCharW(wchar_t *str);
BOOL ConvertToSingleByteString(TCHAR *fromstr, TCHAR *tostr);
void GetDirNameBoforeRename(TCHAR *dir, TCHAR *oldname);
void CreateMsgFile08();
void CreateMenuFile08();
void CreateVolumeFile(TCHAR *folder, TCHAR *filename);
void CreateMenuData10();
void CreateCharImgData10();
void CreateCtrlGuidanceImage10();
void MakeDisplaynameUC(TCHAR *displayname, TCHAR *displaynameUC);

// Fontx.cpp
BOOL	InitFont();
void MakeOLEDdataOfString(BYTE *line0, int size0, BYTE *line1, int size1, unsigned char *sjis_str, int str_length, int *width, int *Scroll);
void MakeOLEDdataShortMenu(BYTE *line0, int size0, unsigned char *sjis_str, int str_length);
void MakeOLEDdataOfMenu(BYTE *line, int width, unsigned char *sjis_str, int str_length);
void MakeOLEDdataOfMessage(BYTE *line0, int size0, unsigned char *sjis_str, int *width);
int MakeLcdDataOfString_10(BYTE *line0, BYTE *line1, BYTE *line2, int line_length, unsigned char *sjis_str);

int _tmain(int argc, _TCHAR* argv[])
{
	FILE	*fout;
	TCHAR	buff[128];
	USHORT	num;
	int		i;
	DIRSTRUCTURE04		rec04;
	DIRSTRUCTUREHEAD04	rech04;
	HISTFILE04		hrec04;
	wchar_t			CurDirW[MAX_PATH];
	TCHAR			CurDir[MAX_PATH];
	USHORT			recno_dirnmi;

	GetCurrentDirectory(MAX_PATH, CurDir);
	GetCurrentDirectoryW(MAX_PATH, CurDirW);

	printf("ディレクトリ名とファイル名を一括して8.3形式に変更\n");
	printf("【カレントディレクトリを確認すること】:%s\n", CurDir);
	printf("(\"go\" + Enterで続行、その他で中止)：");
	if (_fgetts(buff, 128, stdin) == NULL) return 0;
	if (_tcsncmp(buff, _T("go"), 2) != 0) return 0;

once_more_input_ver:
	printf("MP3プレーヤのバージョン : 4(MP3_04), 6(MP3_06), 8(MP3_08/09), 10(MP3_10), 12(MP3_12)：");
	if (_fgetts(buff, 128, stdin) == NULL) return 0;
	i = atoi(buff);
	if (i == 4) Version = 4;
	else if (i == 6) Version = 6;
	else if (i == 8) Version = 8;
	else if (i == 9) Version = 8;		// 9は8と同一フォーマット
	else if (i == 10) Version = 10;
	else if (i == 12) Version = 12;
	else goto once_more_input_ver;

	if (Version == 8 || Version == 9 || Version == 10 || Version == 12){
		if (InitFont() == FALSE){
			printf("フォントデータの取得でエラー\n");
			return 0;
		}
		printf("タイトル表示名決定ルール：0(タグ情報優先), 1([名前変更前]ファイル名優先)：");
		do {
			if (_fgetts(buff, 128, stdin) == NULL) return 0;
			TitleNameRule = atoi(buff);
			if (TitleNameRule == 0 || TitleNameRule == 1) break;
		} while (1);
	}
	GetTempPath(MAX_PATH, TempDir);
	printf("Tempdir=%s\n", TempDir);

	FpLog = _tfopen(DIRMP304LOG, _T("w"));

	/*
	 *	カレントディレクトリから再帰的にフォルダ名とファイル名を変更＆不要ファイルを削除
	 */
	setlocale(LC_ALL,"japanese");
	RenameErrFlg = FALSE;
	AdjustDirAndFileNamesW(CurDirW);
	if (RenameErrFlg == TRUE){
		printf("名前変更出来ないディレクトリまたはMP3ファイルについては手で名前変更すること!\n");
		if (Version == 4 || Version == 6){
			printf("MP3_04, MP3_06ではディレクトリ名を半角英数カナに変更する。\n");
		}
		printf("その後で本PGを再実行する\n");
		return 0;
	}

	/*
	 * ディレクトリ構造を読み取る
	 */
	ReadDirectoryStructure();

	/*
	 *	ランダム再生の順序を決める
	 */
	SetRandomOrder();

	/*
	 *	ディレクトリ構成ファイルを書き出す
	 */
	WriteDirStructureList();

	/*
	 *	インデックスファイル
	 */
	if (Version == 10 || Version == 12){
		CreateIndexList();
	}


	// PITCHSHIFTER
	if (Version == 4){
		fout = _tfopen(PITCHSHIFTER, _T("wb"));
		num = PITCHSHIFTER131_SIZE;
		fwrite(&num, 2, 1, fout);
		for (i=0; i<num; i++){
			fwrite(&PitchShifter131[i], 2, 1, fout);
		}
		fclose(fout);
	}

	// SPEANA
	if (Version == 8){
		fout = _tfopen(SPEANA, _T("wb"));
		num = SPEANA_PLUGIN_SIZE;
		fwrite(&num, 2, 1, fout);
		for (i=0; i<num; i++){
			fwrite(&SpeAna[i], 2, 1, fout);
		}
		fclose(fout);
	}

	// HISTFILE
	fout = _tfopen(HISTFILE, _T("wb"));
	memset(&hrec04, 0, sizeof(hrec04));
	hrec04.LastDir = 0xFFFF;
	hrec04.LastTrack = 0xFFFF;
	hrec04.NextRandomDir = DirList[0].recno_nextrandomplay;
	fwrite(&hrec04.LastDir, HS_LASTDIR, 1, fout);
	fwrite(&hrec04.LastTrack, HS_LASTTRACK, 1, fout);
	fwrite(&hrec04.LastByte, HS_LASTBYTE, 1, fout);
	fwrite(&hrec04.NextRandomDir, HS_NEXTRANDOMDIR, 1, fout);
	fclose(fout);

	// メインメニュー(メッセージ)イメージファイル
	if (Version == 8 || Version == 9){
		//CreateMsgFile08();
		CreateMenuFile08();
	}

	// ERRLOGFILE
	fout = _tfopen(ERRLOGFILE, _T("wb"));
	memset(buff, 0, RECLEN_ERRLOG04);
	for (i=0; i<MAX_ERRLOG_COUNT; i++){
		fwrite(buff, RECLEN_ERRLOG04, 1, fout);
	}
	fclose(fout);

	/***
	printf("\n");
	printf("#define ERRLOG04FILE              \"/%s\"\n", ERRLOGFILE);
	printf("#define RECLEN_ERRLOG04           %dUL\n", RECLEN_ERRLOG04);
	printf("#define MAX_ERRLOG_COUNT          %d\n", MAX_ERRLOG_COUNT);
	***/

	_stprintf(buff, _T("copy %s %s"), ERRLOGFILE, ERRLOGFILE_BK);
	_tsystem(buff);


	fclose(FpLog);

	// MP3_10用のメニューイメージデータと文字イメージデータ
	if (Version == 10){
		CreateMenuData10();
		CreateCharImgData10();
		//CreateCtrlGuidanceImage10();		// 生成したデータはMP3_10_CTRL_VER1のソースにハードコードした。
	} else if (Version == 12){
		CreateMenuData10();
	}

	return 0;
}

/*
 *	ディレクトリ構造を読み取り、内部構造体にセットする。
 *	MP3ファイルの存在するディレクトリについて「音源ファイル(MP3)リスト」を作成していく。
 */
BOOL ReadDirectoryStructure(){
	FILE	*fin;
	TCHAR	buff[128];
	USHORT	rowin;
	USHORT	idxout;
	TCHAR	fulldirbase[BUFLEN];
	TCHAR	albumname[MAXLEN_DISPLAYNAME + 1];
	TCHAR	albumlongname[MAXLEN_TITLE + 1];
	TCHAR	*c;
	TCHAR	*fullp;
	TCHAR	*parentp;
	TCHAR	*name;
	int		i;
	int		len;

	printf("ReadDirectoryStructure\n");
	// DIRコマンド /AD=ディレクトリに関して、/ON=名前順、/S=サブディレクトリも、/B=ファイル名
	// << 同一ディレクトリ下のサブディレクトリは連続することが以下のプログラムの前提になっている。>>
	_stprintf(buff, _T("dir /AD /ON /S /N /B > %s%s"), TempDir, DIRLISTFILE);
	_tsystem(buff);

	_stprintf(buff, _T("%s%s"), TempDir, DIRLISTFILE);
	fin = _tfopen(buff, _T("rt"));
	if (fin == NULL){
		printf("cannot open input file\n");
		return FALSE;
	}

	numsnddir = 0;		// MP3ファイルを持つDIR数
	rowin = 0;
	// ルートディレクトリのエントリを作成
	idxout = 0;
	DirList[idxout].recno_subdir_start = 1;
	idxout++;

	while (_fgetts(buff, 128, fin) != NULL){
		// 改行コードを取る
		c = _tcschr(buff, '\r');
		c = _tcschr(buff, '\n');
		if (c != NULL){
			*c = '\0';
		}
		_tcscpy(fulldirbase, buff);
		// ドライブ名を取る
		if (buff[1] == ':') fullp = &buff[2];
		else fullp = buff;
		if (_tcslen(fullp) > MAXLEN_DIRPATH) continue;

		// fullpはディレクトリのフルパスである。
		// これをパス無しサブディレクトリ名と親ディレクトリパスとに分離する
		c = _tcsrchr(fullp, '\\');
		if (c != NULL){
			_tcscpy(DirList[idxout].fulldir, fullp);
			*c = '\0';
			name = c + 1;
			if (name[0] == '_') continue;		// _ で始まるディレクトリは無視
			if (_tcsncicmp(name, "system", 6) == 0){	// "system"で始まるディレクトリは無視
				continue;
			}
			printf("%s\n", name);
			parentp = fullp;
			// パス無しサブディレクトリ名（これはLCD表示用のデータなので前もってスペース埋めしておく）
			//memset(DirList[idxout].dirname, ' ', MAXLEN_DIRNAME);
			// ここではスペース埋めしない。同じくスペース埋めしないdisplaynameにセットすることがあるため。スペース埋めはファイル出力時に。(2017/07/22)
			memset(DirList[idxout].dirname, 0, MAXLEN_DIRNAME);
			len = _tcslen(name);
			_tcsncpy(DirList[idxout].dirname, name, (len < MAXLEN_DIRNAME ? len : MAXLEN_DIRNAME));
			DirList[idxout].dirname[MAXLEN_DIRNAME] = '\0';
			// 親ディレクトリパス
			_tcscpy(DirList[idxout].fulldirbase, fulldirbase);		// DIRフルパス(ドライブ名付き)
			_tcscpy(DirList[idxout].parentdir, parentp);			// 親ディレクトリパス

			// DirListから親DIRを探す
			for (i=0; i<idxout; i++){
				if (_tcscmp(DirList[idxout].parentdir, DirList[i].fulldir) == 0){
					if (DirList[i].num_subdir == 0){
						DirList[i].recno_subdir_start = idxout;	// 親DIRの最初のサブDIR
					}
					DirList[i].num_subdir++;		// 親DIRのサブDIR数
					DirList[idxout].recno_parentdir = i;		// 親DIRの位置
					DirList[idxout].recno_grandparentdir = DirList[i].recno_parentdir;	// 親親DIRの位置
					break;
				}
			}
			if (i == idxout){
				// 親が見つからないときはルート直下のDIRのはず
				if (DirList[0].num_subdir == 0){
					DirList[0].recno_subdir_start = idxout;	// 親DIRの最初のサブDIR
				}
				DirList[0].num_subdir++;
				DirList[idxout].recno_parentdir = 0;
				DirList[idxout].recno_grandparentdir = 0;
			}
			DirList[idxout].row_dirlist_txt = rowin;		// 入力ファイル内の行位置
			// 音源ファイルリストを生成＆アルバム名（ディレクトリの表示名）を決定する
			memset(albumname, 0, MAXLEN_DISPLAYNAME);
			memset(albumlongname, 0, MAXLEN_TITLE);
			//memset(DirList[idxout].displayname, ' ', MAXLEN_DISPLAYNAME);		// スペース埋めしない
			DirList[idxout].num_sndfile = MakeSndFileList(DirList[idxout].fulldirbase, albumlongname);		// 音源ファイルリストを生成
			//↑で取得したアルバム名はDIR名として使わないことにした。"バッハ/カンタータ BWV5, 56, 180"のようなタイトル名("作品"のタイトルでなく"CD"のタイトル)
			// のことがある。
			//↓でアルバム名を取得する
			memset(albumname, 0, MAXLEN_DISPLAYNAME + 1);
			GetDirNameBoforeRename(DirList[idxout].fulldirbase, albumname);
			if (albumname[0] == '\0'){		// rename前の名前を取得できないとき
				memcpy(DirList[idxout].displayname, DirList[idxout].dirname, _tcslen(DirList[idxout].dirname));
			} else {	
				if ((Version == 4 || Version == 6) && HasMultibyteChar(albumname) == TRUE){		// rename前の名前が２バイト文字を含む
					memcpy(DirList[idxout].displayname, DirList[idxout].dirname, _tcslen(DirList[idxout].dirname));		// 現在のディレクトリ名をそのまま使う
				} else {
					memcpy(DirList[idxout].displayname, albumname, MAXLEN_DISPLAYNAME);		// RENAME前のディレクトリ名をアルバム表示名とする
				}
			}
			DirList[idxout].displayname[MAXLEN_DISPLAYNAME] = '\0';
			printf("[%s]\n", DirList[idxout].displayname);
			MakeDisplaynameUC(DirList[idxout].displayname, DirList[idxout].displaynameuc);
			memcpy(DirList[idxout].displaynameparent, DirList[DirList[idxout].recno_parentdir].displayname, MAXLEN_DISPLAYNAME + 1);		// 親DIRの表示名

			if (DirList[idxout].num_sndfile > 0){
				ShffuleWork[numsnddir] = idxout;		// ランダム再生順決定テーブル
				numsnddir++;
			}

			rowin++;
			idxout++;
		}
	}
	fclose(fin);

	DirListDetailRecNum = idxout;

	return TRUE;
}


/*
 *	ランダム再生のオーダーを決める
 */
BOOL SetRandomOrder(){
	int				shuffle_int;
	int				shuffle_cnt;
	int				i;
	USHORT			dirno_last;

	printf("random order start\n");
	// ランダム再生順決定
	if (numsnddir > 20){
		shuffle_cnt = numsnddir*1000;
		shuffle_int = 10;
	} else {
		shuffle_cnt = 0;
		shuffle_int = 0;
	}
	srand( (unsigned)time( NULL ) );
	for (i=0; i<shuffle_cnt; i++){
		int idx1, idx2;
		USHORT	d;

		idx1 = (rand() % numsnddir);
		while(1) {
			idx2 = (rand() % numsnddir);
			if (abs(idx2 - idx1) > shuffle_int) break;	// なるべく離れたもの同士を交換する
		}
		d = ShffuleWork[idx1];
		ShffuleWork[idx1] = ShffuleWork[idx2];
		ShffuleWork[idx2] = d;
	}
	//for (i=0; i<numsnddir; i++) printf("%d ", ShffuleWork[i]);
	//printf("\n");
	dirno_last = 0;
	for (i=0; i<numsnddir; i++){
		DirList[dirno_last].recno_nextrandomplay = ShffuleWork[i];
		dirno_last = ShffuleWork[i];
	}
	DirList[dirno_last].recno_nextrandomplay = DirList[0].recno_nextrandomplay;		// 循環させる

	return TRUE;
}


// 短縮名(inFilename) -> 元の名前(rename前のLong Name) -> 表示名(outDisplayname)　の順で変換リストから検索する。
// 見つからないとき空文字列を返す。
BOOL GetDisplayNameFromList(TCHAR *inFilename, TCHAR *outDisplayname, TCHAR *outOriginalFilename){
	int		i, j;

	// 元の名前を探す
	for (i=0; i<Mp3RenameListNum; i++){
		if (_tcscmp(inFilename, Mp3RenameList[i].ShortName) == 0){
			_tcscpy(outOriginalFilename, Mp3RenameList[i].OriginalName);
			break;
		}
	}
	if (i < Mp3RenameListNum){
		//printf("Rename前の名前あり\n");
		if (_tcscmp(Mp3RenameList[i].DisplayName, _T("")) == 0){		// 表示名なし
			//printf("    表示名なし\n");
			// 現在の表示名リストから探す(元の名で表示名リストを検索)
			for (j=0; j<Mp3DispnameListNum; j++){
				if (_tcscmp(Mp3RenameList[i].OriginalName, Mp3DispnameList[j].OriginalName) == 0){
					_tcscpy(outDisplayname, Mp3DispnameList[j].DisplayName);
					//printf("        表示名検索あり\n");
					return TRUE;
				}
			}
					//printf("        表示名検索なし\n");
		} else {	// すでに表示名あり
			//printf("    表示名あり\n");
			_tcscpy(outDisplayname, Mp3RenameList[i].DisplayName);
			return TRUE;
		}
	} else {
		//printf("Rename前の名前なし\n");
		_tcscpy(outOriginalFilename, inFilename);
		//outOriginalFilename[0] = '\0';
	}

	outDisplayname[0] = '\0';
	return FALSE;
}

// 表示名を名前変更リストに記録する。
// 名前変更ログに表示名も記録するため。
// この関数は未使用。
void AddDisplayNameToRenameList(TCHAR *ShortName, TCHAR *DisplayName){
	int		i;

	for (i=0; i<Mp3RenameListNum; i++){
		if (_tcscmp(ShortName, Mp3RenameList[i].ShortName) == 0){
			_tcscpy(Mp3RenameList[i].DisplayName, DisplayName);
			break;
		}
	}
}


// MP3ファイル名前変更ログリストの作成
// RENFIL04LOG は、短縮名 対 オリジナルLongName を記録している。
// オリジナルLongNameはスペースを含む可能性あり。
BOOL GetMp3RenameList(TCHAR *inFolder){
	TCHAR	filepath[256];
	//wchar_t	filepathW[256];
	TCHAR	buff[256];
	wchar_t	buffW[256];
	TCHAR	shortname[16];
	TCHAR	originalname[BUFLEN + 1];
	TCHAR	displayname[MAXLEN_TITLE + 1];
	FILE	*fin;
	TCHAR	*c;

	Mp3RenameListNum = 0;
	_stprintf(filepath, _T("%s\\%s"), inFolder, RENFIL04LOG);
	fin = _tfopen(filepath, _T("rt, ccs=UNICODE"));
	if (fin != NULL){
		//printf("%sのオープン %s\n", RENFIL04LOG);
		while (fgetws(buffW, 256, fin) != NULL){
			wcstombs(buff, buffW, 255);
			// 改行コードを取る
			c = _tcschr(buff, '\n');
			if (c != NULL){
				*c = '\0';
			}
			//
			displayname[0] = '\0';
			c = _tcschr(buff, '\t');
			if (c != NULL){
				*c = '\0';
				_tcscpy(Mp3RenameList[Mp3RenameListNum].ShortName, buff);
				_tcscpy(Mp3RenameList[Mp3RenameListNum].OriginalName, c + 1);
				_tcscpy(Mp3RenameList[Mp3RenameListNum].DisplayName, _T(""));
				//wprintf(L"%s %s %s\n", Mp3RenameList[Mp3RenameListNum].ShortName,
				//	Mp3RenameList[Mp3RenameListNum].OriginalName,
				//	Mp3RenameList[Mp3RenameListNum].DisplayName);
				Mp3RenameListNum++;
			}
		}
		fclose(fin);

		return TRUE;
	} else {
		printf("%sのオープンでエラー %s\n", RENFIL04LOG);
		return FALSE;
	}
}


// MP3ファイル名 -> 表示名　変換リストを作成。
// 各フォルダの"表示名_MP3_04.txt"を元に作成する。
BOOL GetMp3DisplayNameReplaceList(TCHAR *inFolder){
	TCHAR	filepath[256];
	TCHAR	buff[256];
	TCHAR	filename[BUFLEN + 1];
	TCHAR	titlename[BUFLEN + 1];
	FILE	*fin;
	TCHAR	*c, *c1, *c2, *c3;

	// 表示名リストを取得する
	Mp3DispnameListNum = 0;
	_stprintf(filepath, _T("%s\\%s"), inFolder, DISPLAYNAMELIST);
	//printf(" infile=%s\n", filepath);
	fin = _tfopen(filepath, _T("rt"));
	if (fin != NULL){
		while (_fgetts(buff, 256, fin) != NULL){
			//printf(" %s\n", buff);
			// 改行コードを取る
			c = _tcschr(buff, '\n');
			if (c != NULL){
				*c = '\0';
			}
			c = _tcschr(buff, '\t');
			if (c != NULL){
				*c = '\0';
				c1 = c;
			//if (_stscanf(buff, _T("%s %s"), filename, titlename) == 2){
			//	c1 = buff + _tcslen(filename);
				while (*c1 == ' ' || *c1 == '\t' || *c1 == '\0'){
					c1++;
				}
				//_tcscpy(filename, buff);
				//_tcscpy(titlename, c1);
				//_tprintf(_T("oritinal name=%s -> %s\n"), buff, c1);
				_tcscpy(Mp3DispnameList[Mp3DispnameListNum].ShortName, _T(""));
				_tcscpy(Mp3DispnameList[Mp3DispnameListNum].OriginalName, buff);
				_tcscpy(Mp3DispnameList[Mp3DispnameListNum].DisplayName, c1);
				Mp3DispnameListNum++;
			}
		}
		fclose(fin);

		return TRUE;
	} else {
		return FALSE;
	}
}


/*
 *	音源ファイル(MP3)リスト作成
 *		outAlbumName : アルバム名取得バッファ(NULL終端の可変長(最大=MAXLEN_DISPLAYNAME)文字列で取得)
 *		FindFirstFile/FindNextFileは必ずしも名前順でないので、dirコマンドによるリストで作成する。
 */
USHORT MakeSndFileList(TCHAR *inFolder, TCHAR *outAlbumName){
	FILE	*fin;
	FILE	*fout;
	TCHAR	outfile[128];
	TCHAR	filename[128];
	TCHAR	albumname[MAXLEN_TITLE + 1];
	TCHAR	titlename[MAXLEN_TITLE + 1];
	TCHAR	originalfilename[MAXLEN_TITLE + 1];
	TCHAR	artistname[128];
	TCHAR	cFileName[MAX_PATH];
	TCHAR	buff[256];
	TCHAR	*c;
	HANDLE	handle;
	WIN32_FIND_DATA		finddata;
	USHORT	num = 0;
	MP3LIST04	rec04[0x100];
	BOOL	VolFileExist;
	int		trackno;
	TCHAR	tracknostr[8];
	int		i, j, len;
	USHORT	recno_sndnmi;
	BOOLEAN	TitleNameFlag;

	trackno = 0;
	recno_sndnmi = 0;
	num = 0;
	TitleNameFlag = FALSE;

	//printf("GetMp3DisplayNameReplaceList %s\n", inFolder);
	GetMp3DisplayNameReplaceList(inFolder);
	//printf("GetMp3RenameList\n");
	GetMp3RenameList(inFolder);
	//printf("GetMp3RenameList end\n");

	// 検索条件
#if 0
	_stprintf(filename, "%s\\*.*", inFolder);
	handle = FindFirstFile(filename, &finddata);	
	if (handle != INVALID_HANDLE_VALUE){
#else
	_stprintf(buff, _T("dir /B /A-D /ON %s > %s%s 2>NUL"), inFolder, TempDir, FILELISTFILE);
	_tsystem(buff);

	_stprintf(buff, _T("%s%s"), TempDir, FILELISTFILE);
	fin = _tfopen(buff, _T("rt"));
	if (fin != NULL){
#endif
		VolFileExist = FALSE;

		// 出力
#if 0
		do {
			_tcsncpy(cFileName, finddata.cFileName, MAX_PATH);
#else
		while (_fgetts(buff, 128, fin) != NULL){
			// 改行コードを取る
			c = _tcschr(buff, '\n');
			if (c != NULL){
				*c = '\0';
			}
			_tcsncpy(cFileName, buff, MAX_PATH);
#endif

			c = _tcsrchr(cFileName, '.');
			if (c != NULL){
				if (_tcsicmp(c, _T(".MP3")) == 0){
					if (CheckName(cFileName) == FALSE){		// 短縮名のMP3ファイルしか存在しないはずだが
						continue;
					}
					if (num == 0x00FF){
						printf("MP3ファイル数制限超過 %s\n", inFolder);
						break;
					}

					// アルバム情報
					memset(albumname, 0, MAXLEN_TITLE + 1);
					memset(titlename, 0, MAXLEN_TITLE + 1);
					memset(originalfilename, 0, MAXLEN_TITLE + 1);
					//if (GetDisplayNameFromList(cFileName, titlename, originalfilename) == FALSE){
					GetDisplayNameFromList(cFileName, titlename, originalfilename);
					//wprintf(L"Original name=%s\n", originalfilename);
					if (titlename[0] == '\0'){
						GetMP3AlbumInfo(inFolder, cFileName, albumname, titlename);		// 表示名ファイルから取得できないときだけMP3ファイルから取得する
						if (num == 0){
							len = _tcslen(albumname);
							// 最初の音源ファイルからアルバム情報を取得。しかし現状、このアルバム情報は使っていない。
							memcpy(outAlbumName, albumname, (len<MAXLEN_DISPLAYNAME ? len : MAXLEN_DISPLAYNAME));
							outAlbumName[(len<MAXLEN_DISPLAYNAME ? len : MAXLEN_DISPLAYNAME)] = '\0';
						}
					}
					trackno++;

					memset(&rec04[num], 0, sizeof(MP3LIST04));
					// MP3ファイル名
					_tcsncpy(rec04[num].FileName, cFileName, 13);
					rec04[num].FileName[12] = '\0';
					// 曲番
					_stprintf(tracknostr, _T("%03d"), trackno);
					len = _tcslen(tracknostr);
					if (len <= 2){
						memcpy(rec04[num].TrackNo, tracknostr, len);
					} else {	// ３桁のときは下２桁だけ表示
						rec04[num].TrackNo[0] = tracknostr[len - 2];
						rec04[num].TrackNo[1] = tracknostr[len - 1];
					}
					// 表示名
					if (titlename[0] == '\0' ||															// MP3ファイルのTAGからタイトルを取得できないとき、あるいは
						((Version == 4 || Version == 6) && HasMultibyteChar(titlename) == TRUE)){		// Ver=4/6で２バイト文字を含むとき

						if (TitleNameRule == 1 && originalfilename[0] != '\0'){
							memcpy(rec04[num].TitleName, originalfilename, _tcslen(originalfilename));		// 名前変更前のオリジナルファイル名をタイトルにする
							TitleNameFlag = TRUE;
						} else {
							memcpy(rec04[num].TitleName, rec04[num].FileName, _tcslen(rec04[num].FileName));	// ファイル名そのものをタイトルにする
						}
					} else {
						if (TitleNameRule == 1 && originalfilename[0] != '\0'){
							memcpy(rec04[num].TitleName, originalfilename, _tcslen(originalfilename));		// 名前変更前のオリジナルファイル名をタイトルにする
						} else {
							len = _tcslen(titlename);
							if (len > MAXLEN_TITLE) len = MAXLEN_TITLE;
							memcpy(rec04[num].TitleName, titlename, len);
						}
						TitleNameFlag = TRUE;
					}
					num++;
				} else if ((Version == 4 && _tcsicmp(cFileName, VOLUMEFILE04) == 0) ||
						(Version == 6 && _tcsicmp(cFileName, VOLUMEFILE06) == 0) ||
						(Version == 8 && _tcsicmp(cFileName, VOLUMEFILE08) == 0) ||
						(Version == 9 && _tcsicmp(cFileName, VOLUMEFILE09) == 0) ||
						(Version == 10 && _tcsicmp(cFileName, VOLUMEFILE10) == 0) ||
						(Version == 12 && _tcsicmp(cFileName, VOLUMEFILE12) == 0)
						){
					VolFileExist = TRUE;
				}
			}
#if 0
		} while (FindNextFile(handle, &finddata));
#else
		}
#endif

		// MP3のタグからタイトル名を取得したとき、タイトル名の先頭にアルバム名を含んでいることがあり、その場合はすべての曲について
		// 同様である。その共通部分を外した部分を曲のタイトル名とする。
		if (TitleNameFlag == TRUE && num > 1){
			int				idx;
			unsigned int	c;
			BOOLEAN			FoundFlag = FALSE;
			TCHAR			titlename[MAXLEN_TITLE + 1];

			for (idx=0; idx<MAXLEN_TITLE; idx++){
				c = rec04[0].TitleName[idx];
				if (c == '\0') break;

				for (i=1; i<num; i++){
					if (c != rec04[i].TitleName[idx]){
						FoundFlag = TRUE;
						break;
					}
				}

				if (FoundFlag == TRUE) break;
			}

			if (FoundFlag == TRUE && idx > 0){
				for (i=0; i<num; i++){
					_tcscpy(titlename, rec04[i].TitleName + idx);
					memset(rec04[i].TitleName, 0, MAXLEN_TITLE);
					memcpy(rec04[i].TitleName, titlename, MAXLEN_TITLE);
				}
			}
		}

		// 出力ファイル
		if (Version == 8 || Version == 9){
			_stprintf(outfile, _T("%s\\%s"), inFolder, SNDFILELIST08);
		} else if (Version == 10 || Version == 12){
			_stprintf(outfile, _T("%s\\%s"), inFolder, SNDFILELIST10);
		} else {
			_stprintf(outfile, _T("%s\\%s"), inFolder, SNDFILELIST);
		}
		fout = _tfopen(outfile, _T("wb"));
		// 一括で出力
		for (i=0; i<num; i++){
			// OLED表示データ作成
			if (Version == 8 || Version == 9){
				int		w;
				int		scroll;

				memset(image0, 0, IMAGE_WORK_LEN);
				memset(image1, 0, IMAGE_WORK_LEN);
				MakeOLEDdataOfString(image0, IMAGE_WORK_LEN, image1, IMAGE_WORK_LEN, (unsigned char *)rec04[i].TitleName, _tcslen(rec04[i].TitleName), &w, &scroll);
				rec04[i].width_sndnmi = w;
				rec04[i].scroll = scroll;

				memset(shortimage1, 0, ML_WIDTH_MENUIMG);
				MakeOLEDdataShortMenu(shortimage1, ML_WIDTH_MENUIMG, (unsigned char *)rec04[i].TitleName, _tcslen(rec04[i].TitleName));
			} else if (Version == 10 || Version == 12){
				memset(image0, 0, LCD_WIDTH_10);
				memset(image1, 0, LCD_WIDTH_10);
				memset(image2, 0, LCD_WIDTH_10);
				MakeLcdDataOfString_10(image0, image1, image2, LCD_WIDTH_10, (unsigned char *)rec04[i].TitleName);
			}
			for (j=0; j<ML_TITLENAME; j++){
				if (rec04[i].TitleName[j] == '\0') rec04[i].TitleName[j] = ' ';
			}
			rec04[i].TitleName[ML_TITLENAME - 1] = '\0';

			//fwrite(&rec04, RECLEN_MP3LIST04, 1, fout);		// 2016/04/10 (構造体のアラインメントによってどのバイト位置に出力されるかわからない)
			fwrite(&rec04[i].FileName, ML_FILENAME, 1, fout);
			fwrite(&rec04[i].TrackNo, ML_TRACKNO, 1, fout);
			fwrite(&rec04[i].TitleName, ML_TITLENAME, 1, fout);
			if (Version == 8 || Version == 9){
				fwrite(&rec04[i].width_sndnmi, ML_WIDTH_SNDNMI, 1, fout);
				fwrite(&rec04[i].scroll, ML_SCROLL, 1, fout);
				fwrite(&rec04[i].filler, ML_FILLER_08, 1, fout);
				fwrite(image0, ML_TITMENAMEIMG_HALF, 1, fout);
				fwrite(image1, ML_TITMENAMEIMG_HALF, 1, fout);
				fwrite(shortimage1, ML_WIDTH_MENUIMG, 1, fout);
			} else if (Version == 10 || Version == 12){
				fwrite(&rec04[i].filler, ML_FILLER_04, 1, fout);
				fwrite(image0, LCD_WIDTH_10, 1, fout);
				fwrite(image1, LCD_WIDTH_10, 1, fout);
				fwrite(image2, LCD_WIDTH_10, 1, fout);
			} else {
				fwrite(&rec04[i].filler, ML_FILLER_04, 1, fout);
			}
		}
		fclose(fout);

		//検索条件クローズ
#if 0
		FindClose(handle);
#else
		fclose(fin);
#endif

		// ボリュームファイル生成
		if (VolFileExist == FALSE && num > 0){
			if (Version == 4){
				CreateVolumeFile(inFolder, VOLUMEFILE04);
			} else if (Version == 6){
				CreateVolumeFile(inFolder, VOLUMEFILE06);
			} else if (Version == 8 || Version == 9){
				CreateVolumeFile(inFolder, VOLUMEFILE08);
				CreateVolumeFile(inFolder, VOLUMEFILE09);
			} else if (Version == 10){
				CreateVolumeFile(inFolder, VOLUMEFILE10);
			} else if (Version == 12){
				CreateVolumeFile(inFolder, VOLUMEFILE12);
			}
#if 0
			UCHAR	vrec04[RECLEN_VOLUMEFILE];
			vrec04[0] = 0xFF;	// ボリューム値
			vrec04[1] = 0;		// ディレクトリの再生回数
			
			if (Version == 4) _stprintf(outfile, _T("%s\\%s"), inFolder, VOLUMEFILE04);
			else if (Version == 6) _stprintf(outfile, _T("%s\\%s"), inFolder, VOLUMEFILE06);
			else if (Version == 8) _stprintf(outfile, _T("%s\\%s"), inFolder, VOLUMEFILE08);
			else if (Version == 9) _stprintf(outfile, _T("%s\\%s"), inFolder, VOLUMEFILE09);
			fout = _tfopen(outfile, _T("wb"));
			fwrite(vrec04, RECLEN_VOLUMEFILE, 1, fout);
			fclose(fout);
#endif
		}
	}

	return num;
}

/*
 *	ボリュームファイル作成
 */
void CreateVolumeFile(TCHAR *folder, TCHAR *filename){
	FILE	*fout;
	TCHAR	outfile[128];
	UCHAR	vrec04[RECLEN_VOLUMEFILE];
	vrec04[0] = 0xFF;	// ボリューム値
	vrec04[1] = 0;		// ディレクトリの再生回数

	_stprintf(outfile, _T("%s\\%s"), folder, filename);
	fout = _tfopen(outfile, _T("wb"));
	fwrite(vrec04, RECLEN_VOLUMEFILE, 1, fout);
	fclose(fout);
}


/*
 *	MP3ファイルのタグ情報からアルバム名、曲名を取得する
 *		outAlbumName : アルバム名取得バッファ(NULL終端の可変長文字列で取得)
 */
void GetMP3AlbumInfo(TCHAR *folder, TCHAR *sndfile, TCHAR *outAlbumName, TCHAR *outTitleName){
	TCHAR	titlenameBuf[MAXLEN_TITLE + 1];
	TCHAR	albumnameBuf[MAXLEN_TITLE + 1];
	TCHAR	sndfilepath[128];
	unsigned char	data[1024];
	FILE	*fin;
	int		i, j, len;
	BOOL	flg1;
	int		num_chars, b;

	memset(titlenameBuf, 0, MAXLEN_TITLE);
	memset(albumnameBuf, 0, MAXLEN_TITLE);

	_stprintf(sndfilepath, _T("%s\\%s"), folder, sndfile);
	fin = _tfopen(sndfilepath, _T("rb"));
	if (fin == NULL){
		_ftprintf(FpLog, _T("fopen error %s\\%s\n"), folder, sndfile);
		return;
	}
	fread(data, 1, 1024, fin);
	fclose(fin);

	if (data[0] == 'I' && data[1] == 'D' && data[2] == '3'){	// ID3
		if (data[3] == 0x02){		// v2.2
			for (i=10; i<1024; i++){
				if (data[i] == 'T'){
					if (data[i + 1] == 'T' && data[i + 2] == '2'){		// TT2
						if (data[i + 6] == 0x00){	// ISO-8859-1
							//_tprintf(_T("ISO-8859-1 TT2 : %s\n"), sndfilepath);
							_tcsncpy(titlenameBuf, (const TCHAR *)&data[i + 7], MAXLEN_TITLE);
							titlenameBuf[MAXLEN_TITLE] = '\0';
							// search the end of TT2
							for (j=(i + 7); j<1024; j++){
								if (data[j] == 0x00){
									i = j;
									break;
								}
							}
						} else {		// UNICODE
							if (data[i + 7] == 0xff && data[i + 8] == 0xfe){
								b = WideCharToMultiByte(932, 0, (WCHAR *)&data[i + 9], -1, (char *)titlenameBuf, MAXLEN_TITLE, NULL, NULL);
							} else if (data[i + 7] == 0xfe && data[i + 8] == 0xff){
								b = 0;
							} else {
								b = WideCharToMultiByte(932, 0, (WCHAR *)&data[i + 7], -1, (char *)titlenameBuf, MAXLEN_TITLE, NULL, NULL);
							}
							if (b > 0){
								//printf("UNICODE TT2 :%s [%s]\n", sndfilepath, titlenameBuf); 
							} else {
								//_tprintf(_T("UNICODE TT2 : %s [unknow]\n"), sndfilepath);
							}
							// search the end of TT2
							num_chars = 0;
							for (j=(i + 7); j<1024; j+=2){
								num_chars++;
								if (data[j] == 0x00 && data[j + 1] == 0x00){
									i = j + 1;
									break;
								}
							}
						}
					} else if (data[i + 1] == 'A' && data[i + 2] == 'L'){	// TAL
						if (data[i + 6] == 0x00){	// ISO-8859-1
							//_tprintf(_T("ISO-8859-1 TAL : %s\n"), sndfilepath);
							_tcsncpy(outAlbumName, (const TCHAR *)&data[i + 7], MAXLEN_DISPLAYNAME);
							outAlbumName[MAXLEN_DISPLAYNAME] = '\0';
							// search the end of TT2
							for (j=(i + 7); j<1024; j++){
								if (data[j] == 0x00){
									i = j;
									break;
								}
							}
						} else {		// UNICODE
							//_tprintf(_T("UNICODE TAL : %s\n"), sndfilepath);
							if (data[i + 7] == 0xff && data[i + 8] == 0xfe){
								b = WideCharToMultiByte(932, 0, (WCHAR *)&data[i + 9], -1, (char *)albumnameBuf, MAXLEN_TITLE, NULL, NULL);
							} else if (data[i + 7] == 0xfe && data[i + 8] == 0xff){
								b = 0;
							} else {
								b = WideCharToMultiByte(932, 0, (WCHAR *)&data[i + 7], -1, (char *)albumnameBuf, MAXLEN_TITLE, NULL, NULL);
							}
							/*
							if (b > 0){
								printf("UNICODE TAL :%s [%s]\n", sndfilepath, albumnameBuf); 
							} else {
								_tprintf(_T("UNICODE TAL : %s [unknow]\n"), sndfilepath);
							}
							*/
							// search the end of TAL
							for (j=(i + 7); j<1024; j+=2){
								if (data[j] == 0x00 && data[j + 1] == 0x00){
									i = j + 1;
									break;
								}
							}
						}
					}
				}
			}
		} else if (data[3] == 0x03){	// v2.3
			for (i=10; i<1024; i++){
				if (data[i] == 'T'){
					if (data[i + 1] == 'I' && data[i + 2] == 'T'  && data[i + 3] == '2'){		// TIT2
						len = data[i + 7] - 1;

						//_tprintf(_T("TIT2 : %s\n"), sndfilepath);
						memcpy(titlenameBuf, (const TCHAR *)&data[i + 11], (len < MAXLEN_TITLE ? len : MAXLEN_TITLE));
						titlenameBuf[MAXLEN_TITLE] = '\0';
						break;
					}
				}
			}
		}
	}

	flg1 = FALSE;

	memcpy(outTitleName, titlenameBuf, MAXLEN_TITLE);
	memcpy(outAlbumName, albumnameBuf, MAXLEN_TITLE);

	/*
	for (i=0; i<LIMIT_TITLE; i++){
		if ((titlenameBuf[i] & 0x80) != 0){		// 日本語文字を含む
			flg1 = TRUE;
			break;
		}
	}
	if (Version == 4 || Version == 6){	// Version=4/6のときは日本語文字を含まないタイトルだけ採用
		if (flg1 == FALSE){
			memcpy(outTitleName, titlenameBuf, LIMIT_TITLE);
		}
	} else if (Version == 8){
		memcpy(outTitleName, titlenameBuf, LIMIT_TITLE);
	}
	*/

	if (outTitleName[0] == 0){
		_ftprintf(FpLog, _T("MP3のタグから曲名不明　%s\n"), folder, sndfilepath);
	} else {
		//_tprintf(_T("%s : %s\n"), sndfilepath, outTitleName);
	}
}

// MP3RENAMEのソート
int CompareMP3RENAME(const void *p1, const void *p2){
	MP3RENAME	*a1 = (MP3RENAME *)p1;
	MP3RENAME	*a2 = (MP3RENAME *)p2;
	return wcscmp(a1->BeforeNameW, a2->BeforeNameW);
}

/*
 *	再帰的にDIR名とMP3ファイル名の変更　を行う。
 *	ファイル名、ディレクトリ名はUNICODEで扱うこと。
 */
void AdjustDirAndFileNamesW(wchar_t *CurrentDirW){
	wchar_t	FindNamePath[BUFLEN];
	MP3RENAME	RenameList[255];
	wchar_t	filenameW[BUFLEN];
	wchar_t	oldfilepathW[BUFLEN];
//	wchar_t	newnameW[BUFLEN];
	wchar_t	newfilepathW[BUFLEN];
	HANDLE	handle;
	WIN32_FIND_DATAW	finddataw;
	TCHAR	msg[256];		// ログ出力用バッファ
	wchar_t	msgW[256];		// ログ出力用バッファ
	int		trackno = 1;
	int		subdirno = 1;
	int		num = 0;
	FILE	*fren;
	int		i;
	BOOL	hasMBC;

	i = wcslen(CurrentDirW);
	if (CurrentDirW[i-1] == '\\') CurrentDirW[i-1] = '\0';
	//printf("AdjustDirAndFileNamesW %s\n", CurrentDir);
	//mbstowcs(CurrentDirW, CurrentDir, BUFLEN);

	// MP3ﾌｧｲﾙの名前変更ログ
	swprintf(filenameW, L"%s\\%s", CurrentDirW, RENFIL04LOGW);
	// 存在しないファイルを"a"で開くとSJISで書き込まれてしまう、という不思議な動作のため
	// 存在しないときは"w"で、存在するときは"a"で開く。
	if (GetFileAttributesW(filenameW) == -1){
		fren = _wfopen(filenameW, L"wt, ccs=UNICODE");		// 
	} else {
		fren = _wfopen(filenameW, L"a+t, ccs=UNICODE");		// 追加出力
	}

	// MP3ファイルのリストを取得
	swprintf(FindNamePath, L"%s\\*.MP3", CurrentDirW);
	handle = FindFirstFileW(FindNamePath, &finddataw);
	if (handle != INVALID_HANDLE_VALUE){
		i = 0;
		do {
			wcscpy(RenameList[i].BeforeNameW, finddataw.cFileName);
			i++;
		} while (FindNextFileW(handle, &finddataw));
		num = i;
		FindClose(handle);

		// RenameListをBeforeNameでソート
		qsort(RenameList, num, sizeof(MP3RENAME), CompareMP3RENAME);
	}


	// 名前変更
	for (trackno=0; trackno<num; trackno++){
		swprintf(oldfilepathW, L"%s\\%s", CurrentDirW, RenameList[trackno].BeforeNameW);
		swprintf(RenameList[trackno].AfterNameW, L"%03d.MP3", trackno + 1);
		swprintf(newfilepathW, L"%s\\%s", CurrentDirW, RenameList[trackno].AfterNameW);

		if (wcsicmp(RenameList[trackno].BeforeNameW, RenameList[trackno].AfterNameW) != 0){
			if (MoveFileW(oldfilepathW, newfilepathW) != 0){
				swprintf(msgW, L"%s\t%s\n", RenameList[trackno].AfterNameW, RenameList[trackno].BeforeNameW);	// 短縮名、元の名
				fwprintf(fren, msgW);
			} else {
				RenameErrFlg = TRUE;
				// メッセージ生成＆ログ出力＆画面出力
				swprintf(msgW, L"MP3 名前変更不可 [%s]\n", oldfilepathW);
				fwprintf(FpLog, msgW);
				wcstombs(msg, msgW, 255);
				msg[255] = '\0';
				printf(msg);
			}
		}
	}

	fclose(fren);

	// サブディレクトリの名前変更
	// サブディレクトリのリスト取得
	i = 0;
	num = 0;
	swprintf(FindNamePath, L"%s\\*.*", CurrentDirW);
	handle = FindFirstFileW(FindNamePath, &finddataw);
	if (handle != INVALID_HANDLE_VALUE){
		do {
			if (finddataw.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
				if (wcscmp(finddataw.cFileName, L".") != 0 &&
					wcscmp(finddataw.cFileName, L"..") != 0 &&
					finddataw.cFileName[0] != '_' &&						// _ で始まるディレクトリはそのまま
					_wcsnicmp(finddataw.cFileName, L"System", 6) != 0		// Systemで始まるディレクトリはそのまま
					){

					wcscpy(RenameList[i].BeforeNameW, finddataw.cFileName);
					i++;
				}
			}
		} while (FindNextFileW(handle, &finddataw));
		num = i;
		FindClose(handle);

		// RenameListをBeforeNameでソート
		qsort(RenameList, num, sizeof(MP3RENAME), CompareMP3RENAME);
	}

	//
	for (trackno=0; trackno < num; trackno++){
		if (CheckNameW(RenameList[trackno].BeforeNameW) == TRUE) continue;		// 名前変更不要

		hasMBC = HasMultibyteCharW(RenameList[trackno].BeforeNameW);

		swprintf(oldfilepathW, L"%s\\%s", CurrentDirW, RenameList[trackno].BeforeNameW);
		if ((Version == 4 || Version == 6) && hasMBC == TRUE){
			swprintf(msgW, L"ディレクトリ　マルチバイト文字含む %s\n", oldfilepathW);
			fwprintf(FpLog, msgW);
			wcstombs(msg, msgW, 255);
			msg[255] = '\0';
			printf(msg);
			RenameErrFlg = TRUE;
		} else {
			wchar_t		altc = L'1';		// ファイル名代替文字
			int			len;

			// 短縮名を生成
			MakeShortDirNameW(RenameList[trackno].BeforeNameW, RenameList[trackno].AfterNameW, &subdirno);
			len = wcslen(RenameList[trackno].AfterNameW);
			// 名前の競合がないかを確認
			do {
				swprintf(newfilepathW, L"%s\\%s", CurrentDirW, RenameList[trackno].AfterNameW);
				if (GetFileAttributesW(newfilepathW) == -1) break;		// 競合なし
				if (hasMBC == TRUE){
					swprintf(RenameList[trackno].AfterNameW, L"%05ld", subdirno++);
				} else {
					RenameList[trackno].AfterNameW[len - 1] = altc++;	// 最後の文字を代替文字に置き換え
				}
			} while (1);
			// 名前変更
			if (MoveFileW(oldfilepathW, newfilepathW) != 0){
				// ディレクトリ名変更が成功したら古い名前をログに記録 -> LCDの表示に使う
				FILE	*fprn;
				wchar_t	renamelog[BUFLEN];
				swprintf(renamelog, L"%s\\%s", newfilepathW, RENAME04LOGW);
				fprn = _wfopen(renamelog, L"wt, ccs=UNICODE");
				if (fprn != NULL){
					swprintf(msgW, L"%s\n", RenameList[trackno].BeforeNameW);
					fwprintf(fprn, msgW);
					fclose(fprn);
				}
				// ディレクトリの古い名前と同名のファイルを作成する。
				// MP3_08では日本語名を記号名に変更するので、元々のディレクトリ名がわかるようにする。
				swprintf(renamelog, L"%s\\%s", newfilepathW, RenameList[trackno].BeforeNameW);
				fprn = _wfopen(renamelog, L"wt, ccs=UNICODE");
				if (fprn != NULL){
					fwprintf(fprn, L"%s\n", RenameList[trackno].AfterNameW);
					fclose(fprn);
				}
			} else {	// 失敗
				swprintf(msgW, L"ディレクトリ　名前変更不可[%lx] [%s]\n", GetLastError(), oldfilepathW);
				fwprintf(FpLog, msgW);
				wcstombs(msg, msgW, 255);
				msg[255] = '\0';
				printf(msg);
				RenameErrFlg = TRUE;
			}
		}
	}

	// サブディレクトリの再帰的処理
	if (num > 0){
		swprintf(FindNamePath, L"%s\\*.*", CurrentDirW);
		handle = FindFirstFileW(FindNamePath, &finddataw);
		if (handle != INVALID_HANDLE_VALUE){
			do {
				if (finddataw.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
					if (wcscmp(finddataw.cFileName, L".") != 0 &&
						wcscmp(finddataw.cFileName, L"..") != 0 &&
						finddataw.cFileName[0] != '_'){		// _ で始まるディレクトリは無視

						swprintf(newfilepathW, L"%s\\%s", CurrentDirW, finddataw.cFileName);
						AdjustDirAndFileNamesW(newfilepathW);
					}
				}
			} while (FindNextFileW(handle, &finddataw));
			FindClose(handle);
		}
	}

}

/*
 *	名前nameが(1)8.3形式で(2)マルチバイト文字を含まないときTRUE
 */
BOOL CheckNameW(wchar_t *name){
	wchar_t	*c;
	int		len;
	int		i;
	BOOL	hasMBC;

	c = wcsrchr(name, '.');
	if (c != NULL){
		// 拡張子の長さ確認
		if (wcslen(c + 1) > 3) return FALSE;
		// Basenameの長さ
		len = (int)(c - name);
	} else {	// 拡張子なし
		len = wcslen(name);
	}
	// basenameの長さ
	if (len > 8) return FALSE;
	// basename中に'.' ' 'を含まない
	for (i=0; i<len; i++){
		if (name[i] == '.' || name[i] == ' ') return FALSE;
	}

	hasMBC = HasMultibyteCharW(name);
	if (hasMBC == TRUE) return FALSE;
	else return(TRUE);
}

/*
 *	名前nameが(1)8.3形式で(2)マルチバイト文字を含まないときTRUE
 */
BOOL CheckName(TCHAR *name){
	TCHAR	*c;
	int		len;
//	int		i;
	BOOL	hasMBC;

	c = _tcsrchr(name, '.');
	if (c != NULL){
		// 拡張子の長さ確認
		if (_tcslen(c + 1) > 3) return FALSE;
		// Basenameの長さ
		len = (int)(c - name);
	} else {	// 拡張子なし
		len = _tcslen(name);
	}
	// basenameの長さ
	if (len > 8) return FALSE;

	hasMBC = HasMultibyteChar(name);
	if (hasMBC == TRUE) return FALSE;
	else return(TRUE);
}

/*
 *	マルチバイト文字を含むときTRUEを返す。
 */
BOOL HasMultibyteCharW(wchar_t *str){
	int		i;
	int		len;

	len = wcslen(str);
	for (i=0; i<len; i++){
		if (iswascii(str[i]) == 0) return TRUE;
		//if (str[i] > 0xFF) return TRUE;
	}
	return FALSE;
}

/*
 *	マルチバイト文字を含むときTRUEを返す。
 */
BOOL HasMultibyteChar(TCHAR *str){
	int		i;
	int		len;

	len = _tcslen(str);

	for (i=0; i<len; i++){
		// マルチバイト文字を含む
		if (_ismbslead((unsigned char *)str, (unsigned char *)(str + i)) != 0){
			return TRUE;
		}
	}

	return FALSE;
}

#if 0		// 使用廃止 2016/08/12
/*
 *	文字列中のマルチバイト文字をシングルバイト文字に変換可能ならば変換し、変換不可能な文字はそのまま残すことで新しい文字列を作成する。
 *		１文字でも変換できたときTRUEを返す。FALSEを返したときは全く変換できなかった。
 */
BOOL ConvertToSingleByteString(TCHAR *fromstr, TCHAR *tostr){
	int		i, j;
	int		len;
	unsigned int	c1, c2;
	BOOL	ret;

	ret = FALSE;
	len = _tcslen(fromstr);
	j = 0;

	for (i=0; i<len; i++){
		// マルチバイト文字を含む
		if (_ismbslead((unsigned char *)fromstr, (unsigned char *)(fromstr + i)) != 0){
			c1 = ((unsigned char)fromstr[i] << 8) | (unsigned char)fromstr[i + 1];
			c2 = _mbctombb(c1);
			if (c2 != c1){
				tostr[j++] = c2;
				ret = TRUE;
			} else {
				tostr[j++] = fromstr[i];
				tostr[j++] = fromstr[i + 1];
			}
			i++;
		} else {
			tostr[j++] = fromstr[i];
		}
	}
	tostr[j++] = '\0';

	return ret;
}
#endif


/*
 *	ディレクトリの名前nameの短縮名を生成する
 */
void MakeShortDirNameW(wchar_t *name, wchar_t *shortname, int *dirno){
	int		i, j;
	int		len;
	BOOL	hasMBC;
//	DWORD	attr;

	hasMBC = HasMultibyteCharW(name);

	if (hasMBC == TRUE){
		// マルチバイト文字を含むときは、新しい名前を作る
		swprintf(shortname, L"%05ld", (*dirno)++);
	} else {
		wchar_t		c = '1';

		len = wcslen(name);

		for (i=0,j=0; i<len && j<8; i++){
			if (iswalnum(name[i]) != 0){
				shortname[j++] = name[i];
			}
		}
		shortname[j++] = '\0';
		len = j - 1;
	}
}

/*
 *	ディレクトリの名前nameの短縮名を生成する
 */
void MakeShortDirName(TCHAR *name, TCHAR *shortname){
	int		i, j;
	int		len;
//	BOOL	hasExtMBC;
	BOOL	hasMBC;
//	TCHAR	*c;

	hasMBC = HasMultibyteChar(name);

	if (hasMBC == TRUE){
		// マルチバイト文字を含むときは、新しい名前を作る
		_stprintf(shortname, _T("%05ld"), DirNameNum++);
	} else {
		len = _tcslen(name);

		for (i=0,j=0; i<len && j<8; i++){
			if (_istalnum((unsigned char)name[i]) != 0){
				shortname[j++] = name[i];
			}
		}
		shortname[j++] = '\0';
	}
}

/*
 *	rename前のディレクトリ名をSJISに変換して取得
 */
void GetDirNameBoforeRename(TCHAR *dir, TCHAR *oldname){
	FILE	*fprn;
	TCHAR	renamelog[BUFLEN];
	TCHAR	buf[BUFLEN];
	wchar_t	bufW[BUFLEN];
	TCHAR	*c;
	int		len;

	_stprintf(renamelog, _T("%s\\%s"), dir, RENAME04LOG);
	fprn = _tfopen(renamelog, _T("rt, ccs=UNICODE"));
	if (fprn != NULL){
		fgetws(bufW, BUFLEN, fprn);		// UNICODEで保存されている名前
		wcstombs(buf, bufW, BUFLEN);	// SJISに変換する
		c = _tcschr(buf, '\n');
		if (c != NULL){
			*c = '\0';
		}
		len = _tcslen(buf);
		memcpy(oldname, buf, (len<MAXLEN_DISPLAYNAME ? len : MAXLEN_DISPLAYNAME));
		oldname[(len<MAXLEN_DISPLAYNAME ? len : MAXLEN_DISPLAYNAME)] = '\0';

		fclose(fprn);
	}
}

/*
 *	DIR構成ファイルを書き出す
 */
BOOL WriteDirStructureList(){
	FILE	*fout;
	DIRSTRUCTURE04		rec04;
	DIRSTRUCTUREHEAD04	rech04;
	USHORT			recno_dirnmi;
	int		i;

	// DIR構成ファイル出力
	// ０レコード目はヘッダレコード
	if (Version == 8 || Version == 9){
		fout = _tfopen(DIRSTRUCTUREFILE08, _T("wb"));
	} else if (Version == 10 || Version == 12){
		fout = _tfopen(DIRSTRUCTUREFILE10, _T("wb"));
	} else {
		fout = _tfopen(DIRSTRUCTUREFILE, _T("wb"));
	}
	rech04.RecNum = DirListDetailRecNum;
	rech04.SndDirNum = numsnddir;
	memset(rech04.filler, '\0', RECLEN_DIRLIST04);
	fwrite(&rech04.RecNum, 2, 1, fout);		// ヘッダ
	fwrite(&rech04.SndDirNum, 2, 1, fout);
	fwrite(&rech04.filler, RECLEN_DIRLIST04 - 4, 1, fout);
	if (Version == 8 || Version == 9){
		fwrite(image0, DL_DIRNAMEIMG_HALF, 1, fout);		// ヘッダレコードもダミーのイメージエリアを持つ(MP3_08/09でファイル内位置の計算を簡単にするため)
		fwrite(image1, DL_DIRNAMEIMG_HALF, 1, fout);
		fwrite(shortimage0, DL_WIDTH_MENUIMG, 1, fout);
	} else if (Version == 10 || Version == 12){
		fwrite(image0, 512 - RECLEN_DIRLIST04, 1, fout);	// ヘッダレコードもダミーのイメージエリアを持つ(MP3_10でファイル内位置の計算を簡単にするため)
	}

	// DIR名イメージファイル
	recno_dirnmi = 0;
	// １レコード目からがDIRレコード（その最初のレコードはルート）
	for (i=0; i<DirListDetailRecNum; i++){
		//_tprintf(_T("%-12s %3d %04d %03d %04d %04d %04d %02d\n"), 
		//	DirList[i].dirname, i, DirList[i].recno_subdir_start, DirList[i].num_subdir, 
		//	DirList[i].recno_parentdir, DirList[i].recno_grandparentdir, DirList[i].recno_nextrandomplay, DirList[i].num_sndfile);

		memcpy(rec04.FullDir, DirList[i].fulldir, MAXLEN_FILEPATH + 1);
		rec04.LenFullDir = _tcslen(rec04.FullDir);		// strlen(rec04.fulldir);
		//memcpy(rec04.DirName, DirList[i].dirname, MAXLEN_DIRNAME + 1);
		memset(rec04.DirName, ' ', MAXLEN_DIRNAME);
		memcpy(rec04.DirName, DirList[i].dirname, _tcslen(DirList[i].dirname));
		rec04.DirName[MAXLEN_DIRNAME] = '\0';
		if (DirList[i].recno_subdir_start == 0){	// サブディレクトリが無い
			rec04.RecnoSubdirStart = 0;
		} else {
			rec04.RecnoSubdirStart = DirList[i].recno_subdir_start + 1;		// ヘッダレコードの分を加算
		}
		if (DirList[i].num_subdir > 0x00FF){
			printf("サブDIR数制限超過 %s %d\n", rec04.FullDir, DirList[i].num_subdir);
			rec04.NumSubdir = 0x00FF;		// 最初の２５５だけ有効に。
			// 制限超過していてもランダム再生の対象にはなる。
			// DIR選択機能で選択ができない。
		} else {
			rec04.NumSubdir = DirList[i].num_subdir;
		}
		rec04.RecnoParentdir = DirList[i].recno_parentdir + 1;
		rec04.RecnoGrandParentdir = DirList[i].recno_grandparentdir + 1;
		rec04.RecnoNextRandomPlay = DirList[i].recno_nextrandomplay;
		rec04.NumSndFile = DirList[i].num_sndfile;
		memset(rec04.DisplayName, ' ', DL_DISPLAYNAME);		// スペース埋めする
		memcpy(rec04.DisplayName, DirList[i].displayname, DL_DISPLAYNAME);
		rec04.DisplayName[DL_DISPLAYNAME] = '\0';
		memset(rec04.Filler, '\0', RECLEN_DIRLIST04);

		if (Version == 8 || Version == 9){
			// ディレクトリの表示名からOLED表示データを作成
			int	imgwidth;
			int scroll;

			memset(image0, 0, IMAGE_WORK_LEN);
			memset(image1, 0, IMAGE_WORK_LEN);
			MakeOLEDdataOfString(image0, IMAGE_WORK_LEN, image1, IMAGE_WORK_LEN, (unsigned char *)DirList[i].displayname, _tcslen(DirList[i].displayname), &imgwidth, &scroll);
			rec04.WidthDirNameImage = imgwidth;
			rec04.Scroll = scroll;

			memset(shortimage0, 0, DL_WIDTH_MENUIMG);
			MakeOLEDdataShortMenu(shortimage0, DL_WIDTH_MENUIMG, (unsigned char *)DirList[i].displayname, _tcslen(DirList[i].displayname));
		} else if (Version == 10 || Version == 12){
			memset(image0, 0, LCD_WIDTH_10);
			memset(image1, 0, LCD_WIDTH_10);
			memset(image2, 0, LCD_WIDTH_10);
			MakeLcdDataOfString_10(image0, image1, image2, LCD_WIDTH_10, (unsigned char *)DirList[i].displayname);
		}

		fwrite(&rec04.FullDir, DL_FULLDIR, 1, fout);
		fwrite(&rec04.LenFullDir, DL_LENFULLDIR, 1, fout);
		fwrite(&rec04.DirName, DL_DIRNAME, 1, fout);
		fwrite(&rec04.RecnoSubdirStart, DL_RECNO_SUBDIR_START, 1, fout);
		fwrite(&rec04.NumSubdir, DL_NUM_SUBDIR, 1, fout);
		fwrite(&rec04.RecnoParentdir, DL_RECNO_PARENTDIR, 1, fout);
		fwrite(&rec04.RecnoGrandParentdir, DL_RECNO_GRANDPARENTDIR, 1, fout);
		fwrite(&rec04.RecnoNextRandomPlay, DL_RECNO_NEXTRANDOMDIR, 1, fout);
		fwrite(&rec04.NumSndFile, DL_NUM_SNDFILE, 1, fout);
		fwrite(&rec04.DisplayName, DL_DISPLAYNAME, 1, fout);
		if (Version == 8 || Version == 9){
			fwrite(&rec04.WidthDirNameImage, DL_WIDTH_DIRNMI, 1, fout);
			fwrite(&rec04.Scroll, DL_SCROLL, 1, fout);
			//if (Version == 8){
				fwrite(&rec04.Filler, DL_FILLER_08, 1, fout);
			//} else if (Version == 9) {
			//	fwrite(&rec04.RecnoMenuImgStart, DL_RECNO_MENUIMG_START, 1, fout);
			//	fwrite(&rec04.Filler, DL_FILLER_09, 1, fout);
			//}
			fwrite(image0, DL_DIRNAMEIMG_HALF, 1, fout);
			fwrite(image1, DL_DIRNAMEIMG_HALF, 1, fout);
			fwrite(shortimage0, DL_WIDTH_MENUIMG, 1, fout);
		} else if (Version == 10 || Version == 12){
			fwrite(&rec04.Filler, DL_FILLER_04, 1, fout);
			fwrite(image0, LCD_WIDTH_10, 1, fout);
			fwrite(image1, LCD_WIDTH_10, 1, fout);
			fwrite(image2, LCD_WIDTH_10, 1, fout);
		} else {
			fwrite(&rec04.Filler, DL_FILLER_04, 1, fout);
		}
	}
	fclose(fout);

	return TRUE;
}

// displayname中のASCII小文字を大文字に変換した文字列を作成する
void MakeDisplaynameUC(TCHAR *displayname, TCHAR *displaynameUC){
	int		i;

	for (i=0; i<MAXLEN_DISPLAYNAME; i++){
		if (_ismbslead((unsigned char *)displayname, (unsigned char *)(displayname + i))){
			displaynameUC[i] = displayname[i];
			displaynameUC[i+1] = displayname[i+1];
			i++;
		} else if (displayname[i] >= 'a' && displayname[i] <= 'z'){
			displaynameUC[i] = toupper(displayname[i]);
		} else {
			displaynameUC[i] = displayname[i];
		}
	}
	displaynameUC[MAXLEN_DISPLAYNAME] = '\0';
	//printf("displaynameUC=%s\n", displaynameUC);
}

// DIRLISTのソート用比較関数
int CmpDirList4Index(const void *p1, const void *p2){
	DIRLIST	*d1 = (DIRLIST *)p1;
	DIRLIST	*d2 = (DIRLIST *)p2;

	// BWVとそれ以外のBとの比較
	if (d1->displaynameuc[0] == 'B' && d2->displaynameuc[0] == 'B'){
		if (!(d1->displaynameuc[1] == 'W' && d1->displaynameuc[2] == 'V') &&
			(d2->displaynameuc[1] == 'W' && d2->displaynameuc[2] == 'V')){
				return -1;
		} else if ((d1->displaynameuc[1] == 'W' && d1->displaynameuc[2] == 'V') &&
			!(d2->displaynameuc[1] == 'W' && d2->displaynameuc[2] == 'V')){
				return +1;
		}
	}
	return _tcscmp(d1->displaynameuc, d2->displaynameuc);		// 大文字に揃えて比較する
}

/*
 *	インデックスファイルを作成する。
 *	インデックスファイルの構造はDIR構成ファイルに類似。
 *	ルートディレクトリ - インデックス - 実ディレクトリ　の３階層からなるDIR構成ファイルと考えられる。
 */
void CreateIndexList(){
	int		i;
	TCHAR	index[8], indexlast[8];
	int		NumSubDirs;
	FILE	*fout;
	DIRSTRUCTURE04		rec04;
	DIRSTRUCTUREHEAD04	rech04;
	TCHAR	displaynameuc2[MAXLEN_DISPLAYNAME + 1];

	memcpy(DirListWork, DirList, sizeof(DIRLIST) * MAX_DIRS);
	qsort(DirListWork, DirListDetailRecNum, sizeof(DIRLIST), CmpDirList4Index);

	// displaynameucの重複チェック(たとえばdisk1のようなディレクトリ名は複数ありえる)
	memset(displaynameuc2, 0, MAXLEN_DISPLAYNAME + 1);
	for (i=0; i<DirListDetailRecNum; i++){
		if (DirListWork[i].num_sndfile > 0){
			if (_tcscmp(displaynameuc2, DirListWork[i].displaynameuc) == 0){
				printf("displaynameuc=%s %s matched\n", DirListWork[i].displaynameuc, displaynameuc2);
				DirListWork[i].same_uc_flag = 1;
				DirListWork[i - 1].same_uc_flag = 1;
			} else {
				DirListWork[i].same_uc_flag = 0;
				printf("displaynameuc=%s %s\n", DirListWork[i].displaynameuc, displaynameuc2);
			}
			_tcscpy(displaynameuc2, DirListWork[i].displaynameuc);
		}
	}
	// 重複のあるdisplaynameについて、親ディレクトリ名|displayname を表示名とする。
	for (i=0; i<DirListDetailRecNum; i++){
		if (DirListWork[i].num_sndfile > 0){
			if (DirListWork[i].same_uc_flag == 1){
				int len = _tcslen(DirListWork[i].displayname);

				if (MAXLEN_DISPLAYNAME > (len+1)){
					DirListWork[i].displayname[len] = '|';
					memcpy(&DirListWork[i].displayname[len + 1], DirListWork[i].displaynameparent, MAXLEN_DISPLAYNAME - (len + 1));
				}
				printf("dupname=%s\n", DirListWork[i].displayname);
			}
		}
	}

	// displaynameを設定しなおしたので、ソートしなおす
	qsort(DirListWork, DirListDetailRecNum, sizeof(DIRLIST), CmpDirList4Index);

	NumIndexes = 0;
	NumSubDirs = 0;

	// 頭文字(index)のリスト(IndexList)を作成する
	for (i=0; i<DirListDetailRecNum; i++){
		if (DirListWork[i].num_sndfile > 0){
			printf("index displayname=%s\n", DirListWork[i].displaynameuc);
			if (_ismbslead((unsigned char *)DirListWork[i].displaynameuc, (unsigned char *)DirListWork[i].displaynameuc)){		// 全角文字
				index[0] = DirListWork[i].displaynameuc[0];
				index[1] = DirListWork[i].displaynameuc[1];
				index[2] = '\0';
			} else if (DirListWork[i].displaynameuc[0] == 'B' && DirListWork[i].displaynameuc[1] == 'W' && DirListWork[i].displaynameuc[2] == 'V'){		// BWVxxxのディレクトリは多いので別インデックスに分ける。
				index[0] = DirListWork[i].displaynameuc[0];
				index[1] = DirListWork[i].displaynameuc[1];
				index[2] = '\0';
			} else {
				index[0] = DirListWork[i].displaynameuc[0];
				index[1] = '\0';
			}
			//printf("index=%s %x %x %x\n", index, index[0], index[1], index[2]);
			if (_tcscmp(index, indexlast) == 0){
				IndexList[NumIndexes - 1].NumSubDir++;		// 頭文字(index)ごとにサブディレクトリ件数をカウント
			} else {		// 頭文字(index)をリスト(IndexList)に追加する
				_tcscpy(IndexList[NumIndexes].Index, index);
				IndexList[NumIndexes].NumSubDir = 1;
				IndexList[NumIndexes].RecNoStart = NumSubDirs;	// DirListWork内の位置
				_tcscpy(indexlast, index);
				NumIndexes++;
			}
			NumSubDirs++;

			// fillerの最初のバイトにインデックステーブルの位置を保持
			DirListWork[i].filler[0] = NumIndexes - 1;
		}
	}

	fout = _tfopen(INDEXFILE10, _T("wb"));

	// ヘッダレコード
	printf("インデックス数=%d\n", NumIndexes);
	rech04.RecNum = NumIndexes + NumSubDirs;			// インデックス数＋サブディレクトリ数＝
	rech04.SndDirNum = NumSubDirs;						// MP3ファイルを持つディレクトリ総数
	memset(rech04.filler, '\0', RECLEN_DIRLIST04);
	fwrite(&rech04.RecNum, 2, 1, fout);		// ヘッダ
	fwrite(&rech04.SndDirNum, 2, 1, fout);
	fwrite(&rech04.filler, RECLEN_DIRLIST04 - 4, 1, fout);
	// フィラー(ヘッダレコードのイメージデータ部分）
	if (Version == 10 || Version == 12){
		memset(image0, 0, LCD_WIDTH_10);
		memset(image1, 0, LCD_WIDTH_10);
		memset(image2, 0, LCD_WIDTH_10);
		fwrite(image0, LCD_WIDTH_10, 1, fout);
		fwrite(image1, LCD_WIDTH_10, 1, fout);
		fwrite(image2, LCD_WIDTH_10, 1, fout);
	}

	// ルートディレクトリレコード
	memset(&rec04, 0, sizeof(rec04));
	rec04.RecnoSubdirStart = 2;			// ヘッダレコード、ルートディレクトリレコードの次から(インデックスのレコードが)スタート
	rec04.NumSubdir = NumIndexes;		// インデックス数
	rec04.RecnoParentdir = 1;
	rec04.RecnoGrandParentdir = 1;
	fwrite(&rec04.FullDir, DL_FULLDIR, 1, fout);
	fwrite(&rec04.LenFullDir, DL_LENFULLDIR, 1, fout);
	fwrite(&rec04.DirName, DL_DIRNAME, 1, fout);
	fwrite(&rec04.RecnoSubdirStart, DL_RECNO_SUBDIR_START, 1, fout);
	fwrite(&rec04.NumSubdir, DL_NUM_SUBDIR, 1, fout);
	fwrite(&rec04.RecnoParentdir, DL_RECNO_PARENTDIR, 1, fout);
	fwrite(&rec04.RecnoGrandParentdir, DL_RECNO_GRANDPARENTDIR, 1, fout);
	fwrite(&rec04.RecnoNextRandomPlay, DL_RECNO_NEXTRANDOMDIR, 1, fout);
	fwrite(&rec04.NumSndFile, DL_NUM_SNDFILE, 1, fout);
	fwrite(&rec04.DisplayName, DL_DISPLAYNAME, 1, fout);
	fwrite(&rec04.Filler, DL_FILLER_04, 1, fout);
	// フィラー
	if (Version == 10 || Version == 12){
		memset(image0, 0, LCD_WIDTH_10);
		memset(image1, 0, LCD_WIDTH_10);
		memset(image2, 0, LCD_WIDTH_10);
		fwrite(image0, LCD_WIDTH_10, 1, fout);
		fwrite(image1, LCD_WIDTH_10, 1, fout);
		fwrite(image2, LCD_WIDTH_10, 1, fout);
	}

	// インデックスレコード
	for (i=0; i<NumIndexes; i++){
		memset(&rec04, 0, sizeof(rec04));
		memcpy(rec04.DirName, IndexList[i].Index, MAXLEN_DIRNAME + 1);
		rec04.RecnoSubdirStart = 2 + NumIndexes + IndexList[i].RecNoStart;
		rec04.NumSubdir = IndexList[i].NumSubDir;
		rec04.RecnoParentdir = 1;		// ルートディレクトリ
		rec04.RecnoGrandParentdir = 1;
		rec04.NumSndFile = 0;
		memset(rec04.DisplayName, ' ', DL_DISPLAYNAME);		// スペース埋めする
		memcpy(rec04.DisplayName, IndexList[i].Index, _tcslen(IndexList[i].Index));
		rec04.DisplayName[DL_DISPLAYNAME] = '\0';
		memset(rec04.Filler, '\0', RECLEN_DIRLIST04);
		// イメージ
		if (Version == 10 || Version == 12){
			printf("INDEX CHA=%s\n", IndexList[i].Index);
			memset(image0, 0, LCD_WIDTH_10);
			memset(image1, 0, LCD_WIDTH_10);
			memset(image2, 0, LCD_WIDTH_10);
			MakeLcdDataOfString_10(image0, image1, image2, LCD_WIDTH_10, (unsigned char *)IndexList[i].Index);	// 6x12, 12x12フォントでイメージデータ作成
		}

		fwrite(&rec04.FullDir, DL_FULLDIR, 1, fout);
		fwrite(&rec04.LenFullDir, DL_LENFULLDIR, 1, fout);
		fwrite(&rec04.DirName, DL_DIRNAME, 1, fout);
		fwrite(&rec04.RecnoSubdirStart, DL_RECNO_SUBDIR_START, 1, fout);
		fwrite(&rec04.NumSubdir, DL_NUM_SUBDIR, 1, fout);
		fwrite(&rec04.RecnoParentdir, DL_RECNO_PARENTDIR, 1, fout);
		fwrite(&rec04.RecnoGrandParentdir, DL_RECNO_GRANDPARENTDIR, 1, fout);
		fwrite(&rec04.RecnoNextRandomPlay, DL_RECNO_NEXTRANDOMDIR, 1, fout);
		fwrite(&rec04.NumSndFile, DL_NUM_SNDFILE, 1, fout);
		fwrite(&rec04.DisplayName, DL_DISPLAYNAME, 1, fout);
		// イメージ
		if (Version == 10 || Version == 12){
			fwrite(&rec04.Filler, DL_FILLER_04, 1, fout);
			fwrite(image0, LCD_WIDTH_10, 1, fout);
			fwrite(image1, LCD_WIDTH_10, 1, fout);
			fwrite(image2, LCD_WIDTH_10, 1, fout);
		}
	}

	// 実ディレクトリ
	for (i=0; i<DirListDetailRecNum; i++){
		if (DirListWork[i].num_sndfile > 0){
			memset(&rec04, 0, sizeof(rec04));
			memcpy(rec04.FullDir, DirListWork[i].fulldir, MAXLEN_FILEPATH + 1);
			rec04.LenFullDir = _tcslen(rec04.FullDir);		// strlen(rec04.fulldir);
			//memcpy(rec04.DirName, DirListWork[i].dirname, MAXLEN_DIRNAME + 1);
			memset(rec04.DirName, ' ', MAXLEN_DIRNAME);
			memcpy(rec04.DirName, DirListWork[i].dirname, _tcslen(DirListWork[i].dirname));
			rec04.DirName[MAXLEN_DIRNAME] = '\0';
			rec04.RecnoSubdirStart = 0;
			rec04.NumSubdir = 0;	// サブディレクトリは無い
			rec04.RecnoParentdir = DirListWork[i].filler[0] + 1;		// ヘッダレコードの分を加算
			rec04.RecnoGrandParentdir = 1;		// ルートディレクトリ
			rec04.NumSndFile = DirListWork[i].num_sndfile;
			memset(rec04.DisplayName, ' ', DL_DISPLAYNAME);		// スペース埋めする
			memcpy(rec04.DisplayName, DirListWork[i].displayname, DL_DISPLAYNAME);
			rec04.DisplayName[DL_DISPLAYNAME] = '\0';
			memset(rec04.Filler, '\0', RECLEN_DIRLIST04);
			// イメージ
			if (Version == 10 || Version == 12){
				memset(image0, 0, LCD_WIDTH_10);
				memset(image1, 0, LCD_WIDTH_10);
				memset(image2, 0, LCD_WIDTH_10);
				MakeLcdDataOfString_10(image0, image1, image2, LCD_WIDTH_10, (unsigned char *)DirListWork[i].displayname);
			}

			fwrite(&rec04.FullDir, DL_FULLDIR, 1, fout);
			fwrite(&rec04.LenFullDir, DL_LENFULLDIR, 1, fout);
			fwrite(&rec04.DirName, DL_DIRNAME, 1, fout);
			fwrite(&rec04.RecnoSubdirStart, DL_RECNO_SUBDIR_START, 1, fout);
			fwrite(&rec04.NumSubdir, DL_NUM_SUBDIR, 1, fout);
			fwrite(&rec04.RecnoParentdir, DL_RECNO_PARENTDIR, 1, fout);
			fwrite(&rec04.RecnoGrandParentdir, DL_RECNO_GRANDPARENTDIR, 1, fout);
			fwrite(&rec04.RecnoNextRandomPlay, DL_RECNO_NEXTRANDOMDIR, 1, fout);
			fwrite(&rec04.NumSndFile, DL_NUM_SNDFILE, 1, fout);
			fwrite(&rec04.DisplayName, DL_DISPLAYNAME, 1, fout);
			if (Version == 10 || Version == 12){
				fwrite(&rec04.Filler, DL_FILLER_04, 1, fout);
				fwrite(image0, LCD_WIDTH_10, 1, fout);
				fwrite(image1, LCD_WIDTH_10, 1, fout);
				fwrite(image2, LCD_WIDTH_10, 1, fout);
			}
		}
	}

	fclose(fout);
}

// MP3_08用メッセージファイルの１メッセージを生成
void WriteLineMsgFile08(FILE *fp, TCHAR *msg){
	BYTE	line0[MS_MSGIMG];
	int		w;

	memset(line0, 0, MS_MSGIMG);
	MakeOLEDdataOfMessage(line0, MS_MSGIMG, (unsigned char *)msg, &w);
	fwrite(line0, 1, MS_MSGIMG, fp);
}

// MP3_08用メニューファイルの１項目を生成
void WriteLineMenuFile08(FILE *fp, TCHAR *msg, int Width){
	BYTE	line0[MS_MSGIMG];
	int		w;

	memset(line0, 0, MS_MSGIMG);
	MakeOLEDdataOfMessage(line0, MS_MSGIMG, (unsigned char *)msg, &w);
	fwrite(line0, 1, Width, fp);
}

// MP3_08/09用メニューイメージファイル作成
void CreateMenuFile08(){
	FILE	*fp;

	fp = _tfopen(MSGIMGFILE08, _T("wb"));
	if (fp != NULL){
		WriteLineMenuFile08(fp, " RANDOM    ", OLED_WIDTH_08/2);
		WriteLineMenuFile08(fp, " SELECT    ", OLED_WIDTH_08/2);
		WriteLineMenuFile08(fp, "           ", OLED_WIDTH_08/2);
		WriteLineMenuFile08(fp, "           ", OLED_WIDTH_08/2);
		fclose(fp);
	}
}

// MP3_10用文字コード対LCDイメージ　ファイル
void CreateCharImgData10(){
	BYTE	line0a[6*(CHARIMG_LAST - CHARIMG_FIRST + 1)];
	BYTE	line1a[6*(CHARIMG_LAST - CHARIMG_FIRST + 1)];
	BYTE	line2a[6*(CHARIMG_LAST - CHARIMG_FIRST + 1)];
	BYTE	lineasc[256];
	int		width;
	int		i;
	FILE	*fout;

	fout = _tfopen(CHARIMGDAT10, _T("wb"));

	// ASCII文字
	for (i=CHARIMG_FIRST; i<=CHARIMG_LAST; i++){
		lineasc[i - CHARIMG_FIRST] = i;
	}
	lineasc[CHARIMG_LAST - CHARIMG_FIRST] = '\0';
	width = MakeLcdDataOfString_10(line0a, line1a, line2a, 6*(CHARIMG_LAST - CHARIMG_FIRST + 1), (unsigned char *)lineasc);
	fwrite(line0a, 1, width, fout);
	fwrite(line1a, 1, width, fout);

	fclose(fout);
}

// MP3_10/MP3_12 用メニューデータ作成
void CreateMenuData10(){
	BYTE	line0a[MS_MSGIMG];
	BYTE	line1a[MS_MSGIMG];
	BYTE	line2a[MS_MSGIMG];
	BYTE	line0b[MS_MSGIMG];
	BYTE	line1b[MS_MSGIMG];
	BYTE	line2b[MS_MSGIMG];
	BYTE	lineasc[256];
	int		width;
	int		i;
	FILE	*fout;

	if (Version == 10){
		fout = _tfopen(MENUDAT10, _T("wb"));
	} else if (Version == 12){
		fout = _tfopen(MENUDAT12, _T("wb"));
	}

	// メインメニュー
	// 1,2行目
	width = MakeLcdDataOfString_10(line0a, line1a, line2a, MS_MSGIMG, (unsigned char *)"フォルダ    ");
	width = MakeLcdDataOfString_10(line0b, line1b, line2b, MS_MSGIMG, (unsigned char *)"インデックス");
	for (i=0; i<LCD_WIDTH_10; i++){
		line1a[i] &= 0x0F;
		line1a[i] |= (line0b[i] << 4);
		line0b[i] >>= 4;
		line0b[i] |= (line1b[i] << 4);
	}
	fwrite(line0a, 1, LCD_WIDTH_10, fout);
	fwrite(line1a, 1, LCD_WIDTH_10, fout);
	fwrite(line0b, 1, LCD_WIDTH_10, fout);

	// 3,4行目
	width = MakeLcdDataOfString_10(line0a, line1a, line2a, MS_MSGIMG, (unsigned char *)"ランダム再生");
	if (Version == 10){
		width = MakeLcdDataOfString_10(line0b, line1b, line2b, MS_MSGIMG, (unsigned char *)"OPT→電源オフ");
	} else if (Version == 12){
		width = MakeLcdDataOfString_10(line0b, line1b, line2b, MS_MSGIMG, (unsigned char *)"             ");
	}
	for (i=0; i<LCD_WIDTH_10; i++){
		line1a[i] &= 0x0F;
		line1a[i] |= (line0b[i] << 4);
		line0b[i] >>= 4;
		line0b[i] |= (line1b[i] << 4);
	}
	fwrite(line0a, 1, LCD_WIDTH_10, fout);
	fwrite(line1a, 1, LCD_WIDTH_10, fout);
	fwrite(line0b, 1, LCD_WIDTH_10, fout);

	if (Version == 12){
		// 5行目
		width = MakeLcdDataOfString_10(line0a, line1a, line2a, MS_MSGIMG, (unsigned char *)" ");
		width = MakeLcdDataOfString_10(line0b, line1b, line2b, MS_MSGIMG, (unsigned char *)" ");	// 6行目は表示できない
		for (i=0; i<LCD_WIDTH_10; i++){
			line1a[i] &= 0x0F;
			line1a[i] |= (line0b[i] << 4);
			line0b[i] >>= 4;
			line0b[i] |= (line1b[i] << 4);
		}
		fwrite(line0a, 1, LCD_WIDTH_10, fout);
		fwrite(line1a, 1, LCD_WIDTH_10, fout);
	}

	// オプションメニュー
	// 1,2行目
	if (Version == 10){
		width = MakeLcdDataOfString_10(line0a, line1a, line2a, MS_MSGIMG, (unsigned char *)"項目数変更");
		width = MakeLcdDataOfString_10(line0b, line1b, line2b, MS_MSGIMG, (unsigned char *)"電源オフ");
	} else if (Version == 12){
		width = MakeLcdDataOfString_10(line0a, line1a, line2a, MS_MSGIMG, (unsigned char *)"GLCDリセット");
		width = MakeLcdDataOfString_10(line0b, line1b, line2b, MS_MSGIMG, (unsigned char *)"スピーカ ON/OFF");
	}
	for (i=0; i<LCD_WIDTH_10; i++){
		line1a[i] &= 0x0F;
		line1a[i] |= (line0b[i] << 4);
		line0b[i] >>= 4;
		line0b[i] |= (line1b[i] << 4);
	}
	fwrite(line0a, 1, LCD_WIDTH_10, fout);
	fwrite(line1a, 1, LCD_WIDTH_10, fout);
	fwrite(line0b, 1, LCD_WIDTH_10, fout);

	// 3,4行目
	if (Version == 10){
		width = MakeLcdDataOfString_10(line0a, line1a, line2a, MS_MSGIMG, (unsigned char *)" ");
	} else if (Version == 12){
		width = MakeLcdDataOfString_10(line0a, line1a, line2a, MS_MSGIMG, (unsigned char *)"ステレオ／モノラル");
	}
	width = MakeLcdDataOfString_10(line0b, line1b, line2b, MS_MSGIMG, (unsigned char *)" ");
	for (i=0; i<LCD_WIDTH_10; i++){
		line1a[i] &= 0x0F;
		line1a[i] |= (line0b[i] << 4);
		line0b[i] >>= 4;
		line0b[i] |= (line1b[i] << 4);
	}
	fwrite(line0a, 1, LCD_WIDTH_10, fout);
	fwrite(line1a, 1, LCD_WIDTH_10, fout);
	fwrite(line0b, 1, LCD_WIDTH_10, fout);

	if (Version == 12){
		// 5行目
		width = MakeLcdDataOfString_10(line0a, line1a, line2a, MS_MSGIMG, (unsigned char *)" ");
		width = MakeLcdDataOfString_10(line0b, line1b, line2b, MS_MSGIMG, (unsigned char *)" ");	// 6行目は表示できない
		for (i=0; i<LCD_WIDTH_10; i++){
			line1a[i] &= 0x0F;
			line1a[i] |= (line0b[i] << 4);
			line0b[i] >>= 4;
			line0b[i] |= (line1b[i] << 4);
		}
		fwrite(line0a, 1, LCD_WIDTH_10, fout);
		fwrite(line1a, 1, LCD_WIDTH_10, fout);
	}

	fclose(fout);
}

// MP3_10用ガイダンスイメージ(CTRL Ver.1)
void CreateCtrlGuidanceImage10(){
	BYTE	line0a[MS_MSGIMG];
	BYTE	line1a[MS_MSGIMG];
	BYTE	line2a[MS_MSGIMG];
	BYTE	line0b[MS_MSGIMG];
	BYTE	line1b[MS_MSGIMG];
	BYTE	line2b[MS_MSGIMG];
	int		width;
	int		i;
	FILE	*fout;

	width = MakeLcdDataOfString_10(line0a, line1a, line2a, MS_MSGIMG, (unsigned char *)"UP  PRV NXT PLY");
	width = MakeLcdDataOfString_10(line0b, line1b, line2b, MS_MSGIMG, (unsigned char *)"DWN SEL DES OPT");

	fout = _tfopen("GUID10.TXT", _T("w"));
	for (i=0; i<LCD_WIDTH_10; i++){
		fprintf(fout, "0x%02x, ", line0a[i]);
	}
	fprintf(fout, "\n");
	for (i=0; i<LCD_WIDTH_10; i++){
		fprintf(fout, "0x%02x, ", line1a[i]);
	}
	fprintf(fout, "\n");
	for (i=0; i<LCD_WIDTH_10; i++){
		fprintf(fout, "0x%02x, ", line0b[i]);
	}
	fprintf(fout, "\n");
	for (i=0; i<LCD_WIDTH_10; i++){
		fprintf(fout, "0x%02x, ", line1b[i]);
	}
	fprintf(fout, "\n");
	fclose(fout);
}


// MP3_08用メッセージファイル作成
void CreateMsgFile08(){
	FILE	*fp;
//	TCHAR	msg[1024];
//	UINT	w;

	fp = _tfopen(MSGIMGFILE08, _T("wb"));
	if (fp != NULL){
		// msg:0
		//_tcscpy(msg, _T("PLAY:RANDOM DOWN:SELECT"));
		//WriteLineMsgFile08(fp, msg);
		if (Version == 8 || Version == 9){
			WriteLineMsgFile08(fp, _T("PLAY    :RANDOM "));
			WriteLineMsgFile08(fp, _T("DOWN-DIR:SELECT "));
		} else {
			WriteLineMsgFile08(fp, _T("PLAY:RANDOM     "));
			WriteLineMsgFile08(fp, _T("DOWN:SELECT     "));
		}
		// msg:1
		//_tcscpy(msg, _T("NO DIR LIST FILE"));
		//WriteLineMsgFile08(fp, msg);
		WriteLineMsgFile08(fp, _T("NO DIR LIST FILE"));
		WriteLineMsgFile08(fp, _T("                "));
		// msg:2
		//_tcscpy(msg, _T("NO MP3 LIST FILE"));
		WriteLineMsgFile08(fp, _T("NO MP3 LIST FILE"));
		WriteLineMsgFile08(fp, _T("                "));
		// msg:3
		//_tcscpy(msg, _T("MP3 FILE OPEN ERROR"));
		WriteLineMsgFile08(fp, _T("MP3 OPEN ERROR  "));
		WriteLineMsgFile08(fp, _T("                "));
		// msg:4
		//_tcscpy(msg, _T("SPEAKER OFF"));
		//WriteLineMsgFile08(fp, msg);
		WriteLineMsgFile08(fp, _T("SPEAKER OFF     "));
		WriteLineMsgFile08(fp, _T("                "));
		// msg:5
		//_tcscpy(msg, _T("SPEAKER ON"));
		//WriteLineMsgFile08(fp, msg);
		WriteLineMsgFile08(fp, _T("SPEAKER ON      "));
		WriteLineMsgFile08(fp, _T("                "));
		// msg:6
		//_tcscpy(msg, _T("DISPLAY OFF"));
		//WriteLineMsgFile08(fp, msg);
		WriteLineMsgFile08(fp, _T("DISPLAY OFF     "));
		WriteLineMsgFile08(fp, _T("                "));
		// msg:7
		//_tcscpy(msg, _T("DISPLAY ON"));
		//WriteLineMsgFile08(fp, msg);
		WriteLineMsgFile08(fp, _T("DISPLAY ON      "));
		WriteLineMsgFile08(fp, _T("                "));
		// msg:8
		//_tcscpy(msg, _T("PAUSE"));
		//WriteLineMsgFile08(fp, msg);
		WriteLineMsgFile08(fp, _T("PAUSE           "));
		WriteLineMsgFile08(fp, _T("                "));
		// msg:9
		//_tcscpy(msg, _T("POWER OFF"));
		//WriteLineMsgFile08(fp, msg);
		WriteLineMsgFile08(fp, _T("POWER OFF       "));
		WriteLineMsgFile08(fp, _T("                "));

		fclose(fp);
	}
}

