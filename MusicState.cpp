#include "MusicState.h"



MusicState music;
HANDLE hMusicStateMutex;	//"Music Type" mutex  Lock when using the object musicState.



MusicType MusicState::GetCurrentMusicType (bool forceWorld) {
	if (state != MusicType::Mt_NotKnown)
		if (world != MusicType::Special)
			if (state != MusicType::Battle || !battleOverridden)
				return state;

	if (checkOverride ()) {
		return override;
	} else {
		return forceWorld ? world : GetWorldMusic ();
	}
}



MusicType MusicState::GetWorldMusic () const {
	return (worldSaved != MusicType::Mt_NotKnown) ? worldSaved : world;
}



MusicType MusicState::GetStateMusic () const {
	if (battlePlaying) {
		return MusicType::Battle;
	} else if (state == MusicType::Battle) {
		return MusicType::Mt_NotKnown;
	} else {
		return state;
	}
}



MusicType MusicState::GetRealMusicType () {
	if (world != MusicType::Special) {
		MusicType state2 = GetStateMusic ();
		if (state2 != MusicType::Mt_NotKnown) {
			return state2;
		}
	}
	if (checkOverride()) {
		return override;
	} else {
		return GetWorldMusic ();
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
		overridePrev = world;
		overridden = true;
		locked = lock;
	}
}



bool MusicState::checkOverride () {
	if (overridden) {
		if (locked || world == overridePrev) {	//nothing changed
			return true;
		} else if (world <= 255) {
			overridden = false;
			override = MusicType::Mt_NotKnown;
		}
	}
	return false;
}



bool isMusicTypeValid (int musicType) {
	return musicType >= 0 && musicType <= 4;
}