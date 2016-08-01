#include "FadeThread.h"

#include <process.h>
#include "Multiplier.h"
#include "MainGameVarsPointer.h"



extern MainGameVarsPointer *mainGameVars;



void FadeThread (void *voidMultiplier) {
	Multiplier *params = (Multiplier *) voidMultiplier;
	while (!mainGameVars || !mainGameVars->gameVars) {
		Sleep (100);
	}
	byte* bHasQuitGame = &mainGameVars->gameVars->bHasQuitGame;
	const int fadeSteps = 32;
	WaitForSingleObject (params->hThread, INFINITE);
	if (!params->isFading) {
		_MESSAGE ("Fade Thread >> Don't start: initialization error");
	} else if (params->startValue == params->targetValue) {
		_MESSAGE ("Fade Thread >> Don't start: fade from/to the same volume!");
	} else {
		int sleepTime = static_cast<int> (params->fadeTime * 1000 / fadeSteps);
		float volStep = (params->targetValue - params->startValue) / fadeSteps;
		_MESSAGE ("Fade Thread >> Initialize: Start: %f, End: %f, Time: %f, Current: %f, Step: %f, Sleep: %d", params->startValue, params->targetValue, params->fadeTime, params->getValue (), volStep, sleepTime);
		ReleaseMutex (params->hThread);
		while (true) {
			Sleep (sleepTime);
			if (*bHasQuitGame == 1) {
				_MESSAGE ("Fade Thread >> Stop: exit game");
				_endthread ();
				return;	//For safety...
			}
			WaitForSingleObject (params->hThread, INFINITE);
			if (params->isDestroyed) {
				_MESSAGE ("Fade Thread >> Stop: multiplier destroyed");
				break;
			} else if (!params->isFading) {		//This means a emcSet*Volume has been called again with fadetime == 0 (instant change), and the fade must stop.
				_MESSAGE ("Fade Thread >> Stop: multiplier changed");
				break;
			} else if (params->isChanged) {	//This means a emcSet*Volume has been called again with fadetime > 0. Update the values.
				params->isChanged = false;
				if (params->startValue == params->targetValue) {
					_MESSAGE ("Fade Thread >> Stop: fade from/to the same volume!");
					break;
				}
				sleepTime = static_cast<int> (params->fadeTime * 1000 / fadeSteps);
				volStep = (params->targetValue - params->startValue) / fadeSteps;
				_MESSAGE ("Fade Thread >> Update: Start: %f, End: %f, Time: %f, Current: %f, Step: %f, Sleep: %d", params->startValue, params->targetValue, params->fadeTime, params->getValue (), volStep, sleepTime);
			}
			params->setValue (params->getValue () + volStep);
			//_MESSAGE ("Update volume to: %f", *params->variable);
			float diff = params->getValue () - params->targetValue;
			if (volStep * diff >= 0) {		//Both positive or both negative -> target reached/surpassed.
				params->setValue (params->targetValue);
				_MESSAGE ("Fade Thread >> Stop: Target value reached");
				break;
			}
			ReleaseMutex (params->hThread);
		}
	}

	params->isFading = false;
	ReleaseMutex (params->hThread);
	_endthread ();
}