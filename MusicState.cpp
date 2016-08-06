#include "MusicState.h"



MusicState music;
HANDLE hMusicStateMutex = CreateMutex (nullptr, FALSE, nullptr);







MusicType MusicState::getWorldType () const {
	return (worldTypeSaved != MusicType::Mt_NotKnown) ? worldTypeSaved : worldType;
}



MusicType* MusicState::getWorldTypePtr () {
	return &worldType;
}



MusicType MusicState::setWorldType (MusicType newWorldType) {
	MusicType prevWorldType = worldType;
	worldType = newWorldType;
	return prevWorldType;
}



MusicType MusicState::saveWorldType (MusicType saveType) {
	MusicType prevSavedWorldType = worldTypeSaved;
	worldTypeSaved = saveType;
	return prevSavedWorldType;
}



MusicType MusicState::getEventType () const {
	if (battlePlaying) {
		return MusicType::Battle;
	} else if (eventType == MusicType::Battle) {
		return MusicType::Mt_NotKnown;
	} else {
		return eventType;
	}
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



MusicType MusicState::getCurrentMusicType (bool ignoreSavedWorld, bool ignoreBattleDelay) {
	if (worldType != MusicType::Special) {									//Not special (death/success)
		if (ignoreBattleDelay) {
			if (eventType == MusicType::Special) {
				return eventType;
			} else if (eventType == MusicType::Battle && !battleOverridden) {
				return eventType;
			}
		} else {
			MusicType realEventType = getEventType ();
			if (realEventType != MusicType::Mt_NotKnown) {
				return realEventType;
			}
		}
	}

	if (checkOverride ()) {
		return override;
	} else if (ignoreSavedWorld) {
		return worldType;
	} else {
		return getWorldType ();
	}
}



void MusicState::overrideMusic (MusicType musicType, bool lock) {
	if (musicType == MusicType::Mt_NotKnown) {
		override = MusicType::Mt_NotKnown;
		overridePrev = MusicType::Mt_NotKnown;
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
			override = MusicType::Mt_NotKnown;
		}
	}
	return false;
}