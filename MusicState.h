#pragma once

#include "MusicType.h"



class MusicState {

private:

	bool overridden = false;
	bool checkOverride ();

	MusicType worldType = Mt_NotKnown;			//The world's music type.
	MusicType eventType = Mt_NotKnown;			//The music type set by the player state (dead, combat, ect)
	SpecialMusicType specialType = Death;		//The special music type playing.

	MusicType worldTypeSaved = Mt_NotKnown;		//What was the music type when the level-up menu appeared?

public:

	MusicType override = Mt_NotKnown;		//What is the music type forced by override?
	MusicType overridePrev = Mt_NotKnown;	//What was the music type when the override was issued? If there's no lock, this allow us to detect when world music change and disable the override
	bool battlePlaying = false;
	bool battleOverridden = false;			//If true, GetCurrentMusicType will not return Battle, unless the override is overridden.
	bool locked = false;

	//Gets the current world music type, checking and eventually avoiding the post-level-up menu bug.
	MusicType getWorldType () const;

	MusicType* getWorldTypePtr ();

	MusicType setWorldType (MusicType newWorldType);

	MusicType saveWorldType (MusicType saveType);


	//Gets the current state music type
	MusicType getEventType () const;

	MusicType* getEventTypePtr ();

	MusicType setEventType (MusicType newEventType);


	SpecialMusicType getSpecialType ();

	SpecialMusicType* getSpecialTypePtr ();

	SpecialMusicType setSpecialType (SpecialMusicType newSpecialType);


	//Gets the current playing music type
	MusicType getCurrentMusicType (bool ignoreSavedWorld, bool ignoreBattleDelay);

	void overrideMusic (MusicType musicType, bool locked);

};


extern MusicState music;
extern HANDLE hMusicStateMutex;			//Lock when using the object "music".
