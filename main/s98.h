#define	MAXDEVICE	64
#define	MAXCHIPTYPE	16

// S98ファイルヘッダ構造体
//
struct S98Header {
  char Magic[3];
  char Format[1];
  unsigned long Timer;
  unsigned long Timer2;
  unsigned long Compress;
  unsigned long NamePtr;
  unsigned long DataPtr;
  unsigned long LoopPtr;
  unsigned long DeviceCount;
  unsigned long DeviceInfo;
};

enum
{
		HEADER_SIZE		= 0x20,
		HEADER_SIZE_MAX	= HEADER_SIZE+0x10*64,
		S98_VERSION		= 0x0003,
		TIMER_INFO		= 0x0004,
		TIMER_INFO2		= 0x0008,
		TAG_OFFSET		= 0x0010,
		DATA_OFFSET		= 0x0014,
		LOOP_OFFSET		= 0x0018,
		DEVICE_COUNT	= 0x001c,
};

enum
{
		TYPE	= 0x00,
		CLOCK	= 0x04,
		PAN		= 0x08,
};

enum
{
	NONE		= 0,
	YM2149,
	YM2203,
	YM2612,
	YM2608,
	YM2151,
	YM2413,
	YM3526,
	YM3812,
	YMF262,
	AY_3_8910	=15,
	SN76489,
};
