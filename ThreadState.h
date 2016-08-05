#pragma once

#include "MusicType.h"
#include "ActivePlaylist.h"


struct ThreadState {

	//The current playing playlist
	ActivePlaylist* activePlaylist = nullptr;

	//The new musicType.
	MusicType newType = MusicType::Mt_NotKnown;

	//The last known musicType (since the thread ran).
	MusicType curType = MusicType::Mt_NotKnown;

	//The previous desired music type.
	MusicType prevType = MusicType::Mt_NotKnown;

	//New special music type.
	SpecialMusicType curSpecial;

	//Last known special music type.
	SpecialMusicType lastSpecial = SpecialMusicType::Death;

	bool newTypeIsBattle;

	bool newTypeIsSpecial;



	//The last song played.
	//When the music type changes the currently playing song will be stored here before a new one is queued up.
	//This will allow us to resume the song later if we need to.
	const char* lastPlayedSong = nullptr;

	//The position of the song when it was stopped.
	long lastPlayedPosition = 0;

	//True if lastPlayedSong was set before (and therefore safe to use).
	bool lastPlayedSet = false;



	//Load the game
	bool loadFromTitle = false;

	//Indicates that the player should not play again until a change.
	//That means even if the Main Thread sees that MusicPlayer is no longer playing, it won't tell it to play something else.
	//This is used for the Death special type.
	bool HoldUntilMTChange = false;

	//This, clumsily, fixes the problems I've had with the success music
	//Now it will play through once, and SHOULD go back to the previously playing music, but I'm not certain how well it will work
	//I need to hook the event that occurs when the 'Exit' button on the Level Up menu is pressed.
	bool SuccessMusicFix = false;

	//This is the timer.
	//While the timer is below restorePeriod, this will be incrimented by the amount in SleepTime.
	//If the musictype changes back to lastType before this timer is > restorePeriod, the music will be changed to lastPlayedSong and started at lastPlayedPosition.
	float restoreTimer = 0;

	//Pause between 2 tracks of the same playlist.
	float pauseTimer = 0;

	//Must remain in combat for at least 1000ms for the combat music to actually begin.
	float battleTimer = 0;

	//Must remain in combat for at least <user>ms for the combat music to actually begin.
	float startBattleTimer = 0;

	//Must have passed at least <user>ms after combat for the combat music to actually end.
	float afterBattleTimer = 0;

	//Tried to play a track, but the path is empty...
	bool noTrack = false;

};



extern ThreadState threadState;
extern HANDLE hThreadMutex;