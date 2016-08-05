#include "FadeThread.h"

#include <process.h>
#include "Globals.h"
#include "Multiplier.h"
#include "MainGameVarsPointer.h"




void FadeThread (void *voidMultiplier) {
	while (!mainGameVars->gameVars) {
		Sleep (100);
	}

	byte *bHasQuitGame = &mainGameVars->gameVars->bHasQuitGame;
	Multiplier *mult = static_cast<Multiplier*>(voidMultiplier);
	LockHandle (mult->hThread);
		if (*bHasQuitGame != 0 || mult->isDestroyed || !mult->isFading) {
			_MESSAGE ("Fade Thread >> Fade procedure not started");
		} else {
			int sleepTime = mult->fadeTime * 1000 / FADE_STEPS;
			float volStep = (mult->targetValue - mult->startValue) / FADE_STEPS;
			_MESSAGE ("Fade Thread >> Initialize: Start: %f, End: %f, Time: %f, Current: %f, Step: %f, Sleep: %d", mult->startValue, mult->targetValue, mult->fadeTime, mult->getValue (), volStep, sleepTime);
			UnlockHandle (mult->hThread);

			Sleep (sleepTime);
			while (true) {
				LockHandle (mult->hThread);
				if (*bHasQuitGame != 0 || mult->isDestroyed || !mult->isFading) {
					_MESSAGE ("Fade Thread >> Thread stopped by external events");
					break;
				} else if (mult->isChanged) {
					mult->isChanged = false;
					sleepTime = mult->fadeTime * 1000 / FADE_STEPS;
					volStep = (mult->targetValue - mult->startValue) / FADE_STEPS;
					_MESSAGE ("Fade Thread >> Update: Start: %f, End: %f, Time: %f, Current: %f, Step: %f, Sleep: %d", mult->startValue, mult->targetValue, mult->fadeTime, mult->getValue (), volStep, sleepTime);
				} else if (mult->setValueLimit (mult->getValue () + volStep, mult->targetValue)) {
					_MESSAGE ("Fade Thread >> Stop thread: target value reached");
					break;
				}
				UnlockHandle (mult->hThread);
				Sleep (sleepTime);
			}
		}
		mult->isFading = false;
	UnlockHandle (mult->hThread);
}