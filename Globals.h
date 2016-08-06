#pragma once

#define arraySize(arr,type) sizeof(arr)/sizeof(type)

#define LockHandle(mutex) WaitForSingleObject(mutex,INFINITE)

#define UnlockHandle(mutex) ReleaseMutex(mutex)

#define LockMulti(handleSet, ...) HANDLE handleSet[] = {__VA_ARGS__};\
WaitForMultipleObjects (arraySize(handleSet,HANDLE), handleSet, 1, INFINITE);

#define UnlockMulti(handleSet) for (HANDLE mutex : handleSet) ReleaseMutex(mutex);

#define isBetween(val,min,max) (min <= val && val <= max)

#define clamp(val,min,max) (val < min ? min : (val > max ? max : val))

#define inConsole IsConsoleOpen () && IsConsoleMode ()

#define Console_PrintC(x, ...) if(inConsole)Console_Print(x, __VA_ARGS__)

#define BUILD_IN_PLACE(key, ...) piecewise_construct, forward_as_tuple(key), forward_as_tuple(__VA_ARGS__) 




//Quick code

//Believe me or not... this condition means either
//(***)		l1 < v < l2   OR   l1 < v < l2
//Why?
//Let's take A and B, which are respectily (l1-v) and (l2-v). A is positive if l1>v and B is positive if l2>v
//We want one of the above situations (*** above), but when is v between l1 and l2?
//Answer: exactly when one is positive and the other is negative
//Example: A<0 and B>0 (so, l1<v && v<l2) or A>0 and B<0 (so, l2<v && v<l2). But... how to check this? 
//Answer: "A*B<0". The result of multiplication is negative only if one is positive and the other is negative
//NOTE: the parameter comp should be < or <= depending if you want v in range (l1,l2) or [l1,l2]
#define isBetweenLimits(l1,v,l2,comp) ((l1-v)*(l2-v) comp 0)