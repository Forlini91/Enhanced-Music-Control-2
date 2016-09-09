#include "TimeManagement.h"


const milliseconds BEGIN_TIME = duration_cast<milliseconds>(system_clock::now ().time_since_epoch ());

const milliseconds TIME_ZERO;
milliseconds now;
long long timeStamp;



void updateNow () {
	now = duration_cast<milliseconds>(system_clock::now ().time_since_epoch ()) - BEGIN_TIME;
	timeStamp = now.count ();
}