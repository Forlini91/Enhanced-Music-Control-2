#include "MusicState.h"

#include "MusicTimes.h"



MusicState musicState;
HANDLE hMusicStateMutex = CreateMutex (nullptr, FALSE, nullptr);



MusicType MusicState::getWorldType () const {
	return worldType;
}



MusicType* MusicState::getWorldTypePtr () {
	return &worldType;
}



MusicType MusicState::setWorldType (MusicType newWorldType) {
	MusicType prevWorldType = worldType;
	worldType = newWorldType;
	return prevWorldType;
}



MusicType MusicState::getEventType () const {
	return eventType;
}



MusicType* MusicState::getEventTypePtr () {
	return &eventType;
}



MusicType MusicState::setEventType (MusicType newEventType) {
	MusicType prevEventType = eventType;
	eventType = newEventType;
	return prevEventType;
}



SpecialMusicType MusicState::getSpecialType () {
	return specialType;
}



SpecialMusicType* MusicState::getSpecialTypePtr () {
	return &specialType;
}



SpecialMusicType MusicState::setSpecialType (SpecialMusicType newSpecialType) {
	SpecialMusicType prevSpecialType = specialType;
	specialType = newSpecialType;
	return prevSpecialType;
}



MusicType MusicState::getCurrentMusicType (bool ignoreBattleDelay) {
	if (worldType != MusicType::mtSpecial) {									//Not special (death/success)
		if (eventType == MusicType::mtSpecial) {
			return MusicType::mtSpecial;
		} else if (ignoreBattleDelay) {
			if (isInBattle()) {
				return MusicType::mtBattle;
			}
		} else if (isBattleMusicPlaying ()) {
			return MusicType::mtBattle;
		}
	}

	if (checkOverride ()) {
		return getOverrideType();
	} else {
		return getWorldType ();
	}
}



bool MusicState::updateMusicState (bool stopped) {
	if (worldType < MusicType::mtNotKnown) {
		lastWorldType = worldType;
	} else if (lastWorldType != MusicType::mtNotKnown) {
		_MESSAGE ("Restore world music type: %d -> %d", worldType, lastWorldType);
		worldType = lastWorldType;
		eventType = MusicType::mtSpecial;
		lastWorldType = MusicType::mtNotKnown;
	}

	//State machine for battle music:
	switch (battleState) {
		case BattleState::bsNoBattle:		//Not in battle
			if (isInBattle ()) {		//Battle is starting. Let's start the delay timer.
				WaitForSingleObject (hMusicTimesMutex, INFINITE);
					battleDelay = now + milliseconds (musicTimes.getStartBattleDelay ());
					musicTimes.recalculateStartBattleDelay ();
				ReleaseMutex (hMusicTimesMutex);
				battleState = BattleState::bsStartBattle;
			}
			break;
		case BattleState::bsStartBattle:
			if (!isInBattle ()) {		//Battle ended before delay passed. Return to NoBattle.
				battleState = BattleState::bsNoBattle;
			} else if (battleDelay >= now) {	//Delay passed. Let's rock!!
				battleState = BattleState::bsBattle;
			}
			break;
		case BattleState::bsEndBattle:
			if (isInBattle ()) {		//Battle restarted before delay passed. Return to Battle.
				battleState = BattleState::bsBattle;
			} else if (battleDelay >= now) {	//Delay passed. Calm down music.
				battleState = BattleState::bsNoBattle;
			}
			break;
		case BattleState::bsBattle:		//In battle
			if (!isInBattle ()) {		//Battle is ending. Let's start the delay timer.
				WaitForSingleObject (hMusicTimesMutex, INFINITE);
					battleDelay = now + milliseconds(musicTimes.getStopBattleDelay ());
					musicTimes.recalculateStopBattleDelay ();
				ReleaseMutex (hMusicTimesMutex);
				battleState = BattleState::bsEndBattle;
			}
			break;
	}


	if (stopped) {
		if (pauseTime == TIME_ZERO) {
			WaitForSingleObject (hMusicTimesMutex, INFINITE);
				pauseTime = now + milliseconds (musicTimes.getPauseTime ());
				musicTimes.recalculatePauseTime ();
			ReleaseMutex (hMusicTimesMutex);
		}
		return pauseTime < now;
	} else if (pauseTime != TIME_ZERO) {
		pauseTime = TIME_ZERO;
	}
	return false;
}



bool MusicState::isLocked () {
	return locked;
}



MusicType MusicState::getOverrideType () {
	return override;
}



void MusicState::overrideType (MusicType musicType, bool lock) {
	if (musicType == MusicType::mtNotKnown) {
		override = MusicType::mtNotKnown;
		overridePrev = MusicType::mtNotKnown;
		overridden = false;
		locked = false;
	} else {
		override = musicType;
		overridePrev = worldType;
		overridden = true;
		locked = lock;
	}
}



bool MusicState::checkOverride () {
	if (overridden) {
		if (locked || worldType == overridePrev) {	//nothing changed
			return true;
		} else if (worldType <= 255) {
			overridden = false;
			override = MusicType::mtNotKnown;
		}
	}
	return false;
}



bool MusicState::isBattleOverridden () {
	return battleOverridde;
}


bool MusicState::setBattleOverride (bool battleOverride) {
	if (MusicState::battleOverridde != battleOverridde) {
		MusicState::battleOverridde = battleOverride;
		return true;
	}
	return false;
}


bool MusicState::isInBattle () {
	return eventType == MusicType::mtBattle && !battleOverridde;
}


bool MusicState::isBattleMusicPlaying () {
	return battleState >= 2;
}