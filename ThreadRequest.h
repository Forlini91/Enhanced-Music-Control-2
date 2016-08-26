#pragma once

#include <string>
#include <set>
#include "MusicType.h"
#include "Playlist.h"
#include "ActivePlaylist.h"
#include "PendingPlaylist.h"


using namespace std;



class ThreadRequest {

private:


	//Play custom track
	string customTrackPath;				//This variable specifies the name of the custom track to play.

	//Set playlist fade times (not used if delayed)
	set<PendingPlaylist> pendingPlaylists;
	float setPlaylistFadeIn = -1;
	float setPlaylistFadeOut = -1;

	//No music
	bool holdMusicPlayer;

	//Next track
	bool nextTrack = false;



public:

	bool hasRequestedCustomTrack () const;

	bool hasRequestedHoldMusicPlayer () const;

	bool hasRequestedNextTrack ();



	
	set<PendingPlaylist>& getPendingPlaylists ();

	string getCustomTrack ();

	void clearCustomTrack ();
	
	void clearRequestNextTrack ();





	void requestPlayCustomTrack (const string &trackPath);

	void requestSetPlaylist (ActivePlaylist *aplToSwap, Playlist *playlist, bool afterThisTrack, float delay);

	bool requestHoldMusicPlayer (bool hold);

	void requestNextTrack (bool noHold);

};

extern ThreadRequest threadRequest;
extern HANDLE hThreadMutex;