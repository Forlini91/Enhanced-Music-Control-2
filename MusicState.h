#pragma once

#include "MusicType.h"
#include "TimeManagement.h"



enum BattleState {
	bsNoBattle = 0,		//Not in battle
	bsStartBattle,		//Battle just started, battle music still not playing
	bsEndBattle,		//Battle just ended, battle music still playing
	bsBattle			//In battle
};


class MusicState {

private:

	bool overridden = false;
	bool checkOverride ();

	MusicType worldType = MusicType::mtNotKnown;			//The world's music type.
	MusicType eventType = MusicType::mtNotKnown;			//The music type set by the player state (dead, combat, ect)
	SpecialMusicType specialType = SpecialMusicType::spNotKnown;	//The special music type playing.

	MusicType lastWorldType = MusicType::mtNotKnown;	//Last valid world type. Let's keep it, should the level-up return an invalid worldType.
	MusicType overrideCurType = MusicType::mtNotKnown;			//What is the music type forced by override?
	MusicType overridePrevType = MusicType::mtNotKnown;		//What was the music type when the override was issued? If there's no lock, this allow us to detect when world music change and disable the override

	BattleState battleState = BattleState::bsNoBattle;
	bool battleOverridde = false;			//If true, GetCurrentMusicType will not return Battle.
	bool locked = false;

	milliseconds battleDelay = TIME_ZERO;
	milliseconds pauseTime = TIME_ZERO;

public:

	int wrlType = 0;

	//Gets the current world music type, checking and eventually avoiding the post-level-up menu bug.
	MusicType getWorldType () const;

	MusicType* getWorldTypePtr ();

	MusicType setWorldType (MusicType newWorldType);


	//Gets the current state music type
	MusicType getEventType () const;

	MusicType* getEventTypePtr ();

	MusicType setEventType (MusicType newEventType);


	SpecialMusicType getSpecialType ();

	SpecialMusicType* getSpecialTypePtr ();

	SpecialMusicType setSpecialType (SpecialMusicType newSpecialType);



	//Gets the current playing music type


	MusicType getCurrentMusicType ();

	//Update the music type, fixing the bug with the Success music and updating the battle delay and pause between tracks.
	//Return true if the music is paused, false otherwise
	bool updateMusicState (bool stopped);

	bool isLocked ();

	MusicType getOverrideType ();

	void overrideType (MusicType musicType, bool locked);

	bool isBattleOverridden ();

	bool setBattleOverride (bool battleOverride);


	bool isInBattle ();

	bool isBattleMusicPlaying ();

	void reloadBattleState ();

};


extern MusicState musicState;
extern HANDLE hMusicStateMutex;			//Lock when using the object "music".
