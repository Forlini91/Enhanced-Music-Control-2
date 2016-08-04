#pragma once

#include <string>
#include "MusicType.h"
#include "Playlist.h"


using namespace std;



extern HANDLE hThreadMutex;



class ThreadRequest {

public:

	//This variable specifies the name of the list to swap it with.
	Playlist* Request_Playlist;

	//This variable specifies the name of the custom track to play.
	string Request_Track;

	//Set to true to request to play the next track.
	bool Request_PlayNext = false;

	//Set to true to request to play a custom track.
	bool Request_PlayCustomTrack = false;

	//This variable will indicate to which MusicType it should play next. 
	//It will only perform the advance if
	//Request_PlayNext_MusicType == newType, UNLESS Request_PlayNext_MusicType == MusicTypes::Undefined.
	MusicType Request_PlayNext_MusicType = Mt_NotKnown;

	//The following variables control the Playlist Swap system.
	//If the ChangeMusic command is called with queueMode true,
	//then the playlist may be queued up.  The next time the player
	//is stopped, it will perform the swap then.

	//This variable determines which MusicType is to be swapped.
	//Setting to Mt_NotKnown indicates no swap is standing by.
	MusicType Request_Swap_Type = Mt_NotKnown;
	float Request_Swap_Delay = 0;
	float Request_Swap_FadeIn = -1;
	float Request_Swap_FadeOut = -1;

	//If set to true, the sentry thread will not attempt to resume
	//the music player from a stopped state until it is cleared.
	bool Request_HoldMusic;


public:
	void requestSetPlaylist (Playlist* playlist, MusicType musicType, int queueMode, float delay);

	void requestResetPlaylist (MusicType musicType);

	void requestPlayCustomTrack (string& track);
	
	bool hasRequestedCustomTrack ();

	void requestHoldMusic (bool keepStopped);

};

extern ThreadRequest threadRequest;