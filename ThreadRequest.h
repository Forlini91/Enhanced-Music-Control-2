#pragma once

#include <string>
#include "MusicType.h"
#include "Playlist.h"


using namespace std;



class ThreadRequest {

private:
	//Set playlist
	Playlist* Swap_Playlist = nullptr;				//The Playlist to set
	MusicType Swap_MusicType = Mt_NotKnown;			//MusicType is to be swapped
	MusicType Swap_MusicType_Next = Mt_NotKnown;	//MusicType to be swapped after the delay
	float Swap_Delay = 0;
	float Swap_FadeIn = -1;
	float Swap_FadeOut = -1;

	//Play custom track
	string CustomTrack_Name;				//This variable specifies the name of the custom track to play.

	bool PlayNextTrack = false;

	bool HoldMusic;



public:

	bool hasRequests ();

	bool hasRequestedSetPlaylist () const;

	bool hasRequestedCustomTrack () const;
	
	bool hasRequestedHoldMusic () const;
	
	void getSwapData (Playlist **playlist, MusicType *musicType);

	void getSwapFadeTimes (float *fadeIn, float *fadeOut);

	bool checkDelayedSetPlaylist (int timePassed);

	string getCustomTrack ();
	
	void cleanRequests ();




	void requestSetPlaylist (Playlist *playlist, MusicType musicType, int queueMode, float delay);

	void requestResetPlaylist (MusicType musicType);

	void requestPlayCustomTrack (string &track);
	
	bool requestHoldMusic (bool hold);

	void requestNextTrack (bool noHold);

};

extern ThreadRequest threadRequest;
extern HANDLE hThreadMutex;