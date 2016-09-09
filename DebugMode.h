
#define EMCDEBUG 1



#ifdef _DEBUG
#define _EMCDEBUG(...) _MESSAGE(__VA_ARGS__)
#elif EMCDEBUG == 1
#define _EMCDEBUG(...) _MESSAGE(__VA_ARGS__)
#define _EMCDEBUG2(...)
#elif EMCDEBUG >= 2
#define _EMCDEBUG(...) _MESSAGE(__VA_ARGS__)
#define _EMCDEBUG2(...) _MESSAGE(__VA_ARGS__)
#else
#define _EMCDEBUG(...)
#endif