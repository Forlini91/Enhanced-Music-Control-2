#include "Multiplier.h"



#include <process.h>
#include "GlobalSettings.h"
#include "FadeThread.h"



Multiplier multObMaster;
Multiplier multObMasterIni;
Multiplier multObMusic;
Multiplier multObMusicIni;
Multiplier multObEffects;
Multiplier multObEffectsIni;
Multiplier multObVoice;
Multiplier multObVoiceIni;
Multiplier multObFoot;
Multiplier multObFootIni;
MultipliersMap multipliersCustom;




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



bool Multiplier::setValueLimit (float newValue, float limit) {
	if (isBetweenLimits(*value,limit,newValue,<=)) {
		//If limit is beween *value and newValue, then we need to cross it (but we don't want to cross it, so...).
		*value = limit;
		return true;
	} else {
		*value = newValue;
		return false;
	}
}



void Multiplier::fadeVolume (float newTargetValue, float newFadeTime) {
	//Assume the calling function waited for the mutex
	startValue = *value;
	targetValue = clamp (newTargetValue);
	fadeTime = newFadeTime;
	if (isFading) {
		_MESSAGE ("Command >> emcSetMusicVolume >> Update fade thread");
		if (startValue == targetValue) {
			isFading = false;
			setValue (targetValue);
		} else {
			isChanged = true;
		}
	} else {
		if (startValue == targetValue) {
			setValue (targetValue);
		} else {
			_MESSAGE ("Command >> emcSetMusicVolume >> Begin fade thread");
			isFading = true;
			_beginthread (FadeThread, 0, this);
		}
	}
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