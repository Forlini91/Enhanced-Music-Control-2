#include "FadeThread.h"

#include <process.h>
#include "Globals.h"
#include "Multiplier.h"
#include "MainGameVarsPointer.h"




void FadeThread (void *voidMultiplier) {
	Multiplier *mult = static_cast<Multiplier*>(voidMultiplier);
	WaitForSingleObject (mult->hThread, INFINITE);
		if (*bHasQuitGame == 0 && !mult->isDestroyed && mult->isFading) {
			int sleepTime = mult->fadeTime * 1000 / FADE_STEPS;
			float step = (mult->targetValue - mult->startValue) / FADE_STEPS;
			_MESSAGE ("Fade Thread >> Initialize: Start: %f, End: %f, Time: %f, Current: %f, Step: %f, Sleep: %d", mult->startValue, mult->targetValue, mult->fadeTime, mult->getValue (), step, sleepTime);

			for (; *bHasQuitGame != 0 || mult->isDestroyed || !mult->isFading; Sleep (sleepTime)) {
				WaitForSingleObject (mult->hThread, INFINITE);
					if (mult->isChanged) {
						mult->isChanged = false;
						sleepTime = mult->fadeTime * 1000 / FADE_STEPS;
						step = (mult->targetValue - mult->startValue) / FADE_STEPS;
						_MESSAGE ("Fade Thread >> Update: Start: %f, End: %f, Time: %f, Current: %f, Step: %f, Sleep: %d", mult->startValue, mult->targetValue, mult->fadeTime, mult->getValue (), step, sleepTime);
					} else if (mult->setValueLimit (mult->getValue () + step, mult->targetValue)) {
						_MESSAGE ("Fade Thread >> Stop thread: target value reached");
						mult->isFading = false;
						ReleaseMutex (mult->hThread);
						return;
					}
				ReleaseMutex (mult->hThread);
			}
			return;
		} else {
			_MESSAGE ("Fade Thread >> Fade procedure not started");
		}
		mult->isFading = false;
	ReleaseMutex (mult->hThread);
}