#define MAJOR_VERSION	0
#define MINOR_VERSION	7
#define MICRO_VERSION	0
#define BUILD_NUMBER	_T("250")
#define YEAR_NUMBER		_T("2022")

#define QUOTE_(x)		#x
#define QUOTE(x)		QUOTE_(x)

#define VERSION_STRING	QUOTE(MAJOR_VERSION.MINOR_VERSION.MICRO_VERSION)
#define VERSION_STRING_W	_T(QUOTE(MAJOR_VERSION.MINOR_VERSION.MICRO_VERSION))

#ifdef NDEBUG
#define DEBUG_STRING	_T("")
#else
#define DEBUG_STRING	_T("Debug")
#endif

#define BUILD_DATE		_T("5th March 2022")
