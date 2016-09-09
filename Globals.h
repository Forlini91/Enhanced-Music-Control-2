#pragma once


extern IDebugLog gLog;

#define WaitForSingleObject WaitForSingleObject			//This only exist for syntax coloration purpose!
#define WaitForMultipleObjects WaitForMultipleObjects	//This only exist for syntax coloration purpose!
#define ReleaseMutex ReleaseMutex						//This only exist for syntax coloration purpose!



#define isBetween(val,minV,maxV) (minV <= val && val <= maxV)

#define clamp(val,minV,maxV) (val < minV ? minV : (val > maxV ? maxV : val))

#define inConsole IsConsoleOpen () && IsConsoleMode ()

#define Console_PrintC(x, ...) if(inConsole)Console_Print(x, __VA_ARGS__)

#define constructInPlace(key, ...) piecewise_construct, forward_as_tuple(key), forward_as_tuple(__VA_ARGS__) 

#define getEmplaced(x) x.first->second



//Quick code

//Believe me or not... this condition means either
//(***)			l1 < v < l2   OR   l2 < v < l1
//(C++ code)	(l1 < v && v < l2) || (l2 < v && v < l1)
//Why?
//Let's take A and B, which are respectily (l1-v) and (l2-v). A is positive if l1>v and B is positive if l2>v
//We want v in the middle, so either of the above (***) situations, but when is v in the middle?
//Answer: when exactly one betweehn A or B is positive and the other is negative
//Example: A<0 and B>0 (so, l1<v && v<l2) or A>0 and B<0 (so, l2<v && v<l2). But... how to check this quickly? 
//Answer: "A*B<0". The result of multiplication is negative only if one is positive and the other is negative
//NOTE: the parameter comp should be < or <= depending if you want v in range (l1,l2) or [l1,l2]
#define isBetweenLimits(l1,v,l2,comp) ((l1-v)*(l2-v) comp 0)