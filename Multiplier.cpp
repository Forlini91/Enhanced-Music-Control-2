#include "Multiplier.h"



#include <process.h>
#include "FadeThread.h"



Multiplier::~Multiplier () {
	if (userMultiplier) {
		delete value;
	}
	CloseHandle (hThread);

}



volatile float Multiplier::getValue () {
	return *value;
}



void Multiplier::setValue (float newValue) {
	*value = clamp(newValue);
}



bool Multiplier::fadeVolume (float targetValue, float fadeTime) {
	//Assume the calling function waited for the mutex
	startValue = *value;
	Multiplier::targetValue = clamp(targetValue);
	Multiplier::fadeTime = fadeTime;
	if (!isFading) {
		isFading = true;
		_MESSAGE ("Command >> emcSetMusicVolume >> Begin fade thread");
		_beginthread (FadeThread, 0, this);
	} else {
		_MESSAGE ("Command >> emcSetMusicVolume >> Update fade thread");
		isChanged = true;
	}
	return true;
	//Assume the calling function will release the mutex
}

float Multiplier::clamp (float value) {
	if (value < 0) {
		return 0;
	} else if (value > 1) {
		return 1;
	} else {
		return value;
	}
}