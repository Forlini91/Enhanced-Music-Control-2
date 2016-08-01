#pragma once

#include "MusicType.h"

struct ThreadRequest {

	//Set to true to request SentryThread to advance the song by one.
	bool Request_PlayNext = false;

	bool Request_PlayCustom = false;

	//This variable will indicate to SentryThread which MusicType it
	//should do this for.  It will only perform the advance if
	//Request_PlayNext_MusicType == newType, UNLESS Request_PlayNext_MusicType
	//is == MusicTypes::Undefined.
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

	//This variable specifies the name of the list to swap it with.
	char Request_Swap_plName[512];

	//This variable specifies the name of the custom track to play.
	char Request_Track_Name[MAX_PATH];

	//If set to true, the sentry thread will not attempt to resume
	//the music player from a stopped state until it is cleared.
	bool Request_HoldMusic;

};