#include "MusicState.h"

#include "MusicTimes.h"
#include "DebugMode.h"


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



MusicType MusicState::getCurrentMusicType () {
	if (worldType != MusicType::mtSpecial) {									//Not special (death/success)
		if (eventType == MusicType::mtSpecial) {
			return MusicType::mtSpecial;
		} else if (battleState >= 2) {
			return MusicType::mtBattle;
		}
	}

	if (checkOverride ()) {
		return overrideCurType;
	} else {
		return worldType;
	}
}



bool MusicState::updateMusicState (bool stopped) {
	if (worldType < MusicType::mtBattle) {
		if (lastWorldType != worldType) {
			_EMCDEBUG ("%lld | MusicState >> Save world music type: %d -> %d", timeStamp, lastWorldType, worldType);
			lastWorldType = worldType;
		}
	} else if (lastWorldType != MusicType::mtNotKnown && worldType > MusicType::mtNotKnown) {
		_EMCDEBUG ("%lld | MusicState >> Restore world music type: %d (%u) -> %d", timeStamp, worldType, wrlType, lastWorldType);
		worldType = lastWorldType;
		eventType = MusicType::mtSpecial;
		lastWorldType = MusicType::mtNotKnown;
	}

	//State machine for battle music:
	switch (battleState) {
		case BattleState::bsNoBattle:		//Not in battle
			if (isInBattle ()) {		//Battle is starting. Let's start the delay timer.
				WaitForSingleObject (hMusicTimesMutex, INFINITE);
					milliseconds startBattleDelay = milliseconds(musicTimes.getStartBattleDelay ());
					battleDelay = now + startBattleDelay;
					_MESSAGE ("%lld | MusicState >> Start battle delay: %lld + %lld  =>  %lld", timeStamp, timeStamp, startBattleDelay.count (), battleDelay.count ());
					musicTimes.recalculateStartBattleDelay ();
				ReleaseMutex (hMusicTimesMutex);
				battleState = BattleState::bsStartBattle;
			}
			break;
		case BattleState::bsStartBattle:
			if (!isInBattle ()) {		//Battle ended before delay passed. Return to NoBattle.
				battleState = BattleState::bsNoBattle;
				_MESSAGE ("%lld | MusicState >> Battle ended before start delay passed: %lld", timeStamp, battleDelay.count ());
			} else if (battleDelay < now) {	//Delay passed. Let's rock!!
				battleState = BattleState::bsBattle;
				_MESSAGE ("%lld | MusicState >> Start battle delay passed: %lld", timeStamp, battleDelay.count ());
			}
			break;
		case BattleState::bsEndBattle:
			if (isInBattle ()) {		//Battle restarted before delay passed. Return to Battle.
				battleState = BattleState::bsBattle;
				_MESSAGE ("%lld | MusicState >> Battle restarted before stop delay passed: %lld", timeStamp, battleDelay.count ());
			} else if (battleDelay < now) {	//Delay passed. Calm down music.
				battleState = BattleState::bsNoBattle;
				_MESSAGE ("%lld | MusicState >> Stop battle delay passed: %lld", timeStamp, battleDelay.count ());
			}
			break;
		case BattleState::bsBattle:		//In battle
			if (!isInBattle ()) {		//Battle is ending. Let's start the delay timer.
				WaitForSingleObject (hMusicTimesMutex, INFINITE);
					milliseconds stopBattleDelay = milliseconds(musicTimes.getStopBattleDelay ());
					battleDelay = now + stopBattleDelay;
					_MESSAGE ("%lld | MusicState >> Stop battle delay: %lld + %lld  ==>  %lld", timeStamp, timeStamp, stopBattleDelay.count (), battleDelay.count ());
					musicTimes.recalculateStopBattleDelay ();
				ReleaseMutex (hMusicTimesMutex);
				battleState = BattleState::bsEndBattle;
			}
			break;
	}


	if (stopped) {
		if (pauseTime == TIME_ZERO) {
			WaitForSingleObject (hMusicTimesMutex, INFINITE);
				milliseconds pause = milliseconds(musicTimes.getPauseTime ());
				pauseTime = now + pause;
				_MESSAGE ("%lld | MusicState >> Pause between track: %lld + %lld  ==>  %lld", timeStamp, timeStamp, pause.count (), pauseTime.count ());
				musicTimes.recalculatePauseTime ();
			ReleaseMutex (hMusicTimesMutex);
		}
		return now < pauseTime;
	} else if (pauseTime != TIME_ZERO) {
		_MESSAGE ("%lld | MusicState >> Reset pause variables", timeStamp);
		pauseTime = TIME_ZERO;
	}
	return false;
}



bool MusicState::isLocked () {
	return locked;
}



MusicType MusicState::getOverrideType () {
	return overrideCurType;
}



void MusicState::overrideType (MusicType musicType, bool lock) {
	if (musicType == MusicType::mtNotKnown) {
		overrideCurType = MusicType::mtNotKnown;
		overridePrevType = MusicType::mtNotKnown;
		overridden = false;
		locked = false;
	} else {
		overrideCurType = musicType;
		overridePrevType = worldType;
		overridden = true;
		locked = lock;
	}
}



bool MusicState::checkOverride () {
	if (overridden) {
		if (locked || worldType == overridePrevType) {	//nothing changed
			return true;
		} else if (worldType <= 255) {
			overridden = false;
			overrideCurType = MusicType::mtNotKnown;
			_MESSAGE ("%lld | MusicState >> Cell changed -> soft override disabled");
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



void MusicState::reloadBattleState () {
	if (battleState >= 2) {
		if (eventType != MusicType::mtBattle) {
			battleState = BattleState::bsNoBattle;
		}
	} else {
		if (eventType == MusicType::mtBattle) {
			battleState = BattleState::bsBattle;
		}
	}
}