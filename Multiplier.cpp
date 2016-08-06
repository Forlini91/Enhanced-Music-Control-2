#include "Multiplier.h"



#include <process.h>
#include "Globals.h"
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
	*value = clamp(newValue, 0, 1);
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



FadeThreadState Multiplier::fadeVolume (float newTargetValue, float newFadeTime) {
	//Assume the calling function waited for the mutex
	startValue = *value;
	targetValue = clamp (newTargetValue, 0, 1);
	fadeTime = newFadeTime > 0 ? newFadeTime : 0;
	if (isFading) {
		if (startValue == targetValue || fadeTime == 0) {
			_MESSAGE ("Command >> emcSetMusicVolume >> Stop fade thread");
			isFading = false;
			setValue (targetValue);
			return FadeThreadState::ft_Stopped;
		} else {
			_MESSAGE ("Command >> emcSetMusicVolume >> Update fade thread");
			isChanged = true;
			return FadeThreadState::ft_Updated;
		}
	} else {
		if (startValue == targetValue || fadeTime == 0) {
			setValue (targetValue);
			return FadeThreadState::ft_NotRunning;
		} else {
			_MESSAGE ("Command >> emcSetMusicVolume >> Start fade thread");
			isFading = true;
			_beginthread (FadeThread, 0, this);
			return FadeThreadState::ft_Started;
		}
	}
	//Assume the calling function will release the mutex
}



volatile float Multiplier::operator<<(volatile float *var) {
	value = var;
	return *value;
}