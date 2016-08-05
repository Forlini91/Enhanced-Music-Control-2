#pragma once

#include "MusicType.h"



class MusicState {

public:

	MusicType world = Mt_NotKnown;		//The world's music type.
	MusicType state = Mt_NotKnown;		//The music type set by the player state (dead, combat, ect)
	SpecialMusicType special = Death;		//The special music type playing.

	MusicType override = Mt_NotKnown;		//What is the music type forced by override?
	MusicType overridePrev = Mt_NotKnown;	//What was the music type when the override was issued? If there's no lock, this allow us to detect when world music change and disable the override
	MusicType worldSaved = Mt_NotKnown;		//What was the music type when the level-up menu appeared?
	bool battlePlaying = false;
	bool battleOverridden = false;			//If true, GetCurrentMusicType will not return Battle, unless the override is overridden.
	bool locked = false;

	//Gets the current playing music type
	MusicType GetCurrentMusicType (bool forceWorld);

	//Gets the current world music type, checking and eventually avoiding the post-level-up menu bug.
	MusicType GetWorldMusic () const;

	//Gets the current state music type
	MusicType GetStateMusic () const;

	MusicType GetRealMusicType ();

	void overrideMusic (MusicType musicType, bool locked);

private:

	bool overridden = false;
	bool checkOverride ();

};


extern MusicState music;
extern HANDLE hMusicStateMutex;
