#include "ThreadRequest.h"

#include "MusicState.h"
#include "MusicPlayer.h"



extern MusicState music;
extern MusicPlayer musicPlayer;
extern HANDLE hSentryRequestMutex;
extern HANDLE hMusicTypeMutex;
extern HANDLE hThePlayerMutex;



void ThreadRequest::requestSetPlaylist (Playlist* playlist, MusicType targetMT, int queueMode, float delay) {
	WaitForSingleObject (hThePlayerMutex, INFINITE);
	float fadeOut = musicPlayer.getCurrentFadeOutLength ();
	float fadeIn = musicPlayer.getCurrentFadeInLength ();
	ReleaseMutex (hThePlayerMutex);

	WaitForSingleObject (hMusicTypeMutex, INFINITE);
	MusicType musicType = music.GetCurrentMusicType (false);
	ReleaseMutex (hMusicTypeMutex);

	WaitForSingleObject (hSentryRequestMutex, INFINITE);
	if (musicType == targetMT) {
		Request_Swap_FadeIn = fadeIn;
		Request_Swap_FadeOut = fadeOut;
	}

	if (musicType == targetMT && queueMode == 1) {
		//QueueMode delays the update if music.GetCurrentMusicType == targetMT
		//It will be updated next time the music player is in a stopped state.
		Request_Swap_Type = targetMT;
		Request_Playlist = playlist;
		Request_Swap_Delay = delay * 1000;
	} else {
		//Immediately alter the playlist.
		//First check to see if there is any swap requests waiting for this music type.
		if (Request_Swap_Type == targetMT) { //Clear it.
			Request_Swap_Type = MusicType::Mt_NotKnown;
		}
		if (queueMode < 2) {	//Now, request a song advance. SentryThread will carry it out if/when it can.
			Request_PlayNext = true;
			if (Request_PlayNext_MusicType > 4 || Request_PlayNext_MusicType != musicType) {
				Request_PlayNext_MusicType = targetMT;
			}
		}
	}
	ReleaseMutex (hSentryRequestMutex);
}



void ThreadRequest::requestPlayTrack (string& track) {
	WaitForSingleObject (hSentryRequestMutex, INFINITE);
	Request_PlayCustomTrack = true;
	Request_Track = track;
	ReleaseMutex (hSentryRequestMutex);
}



bool ThreadRequest::hasRequestedCustomTrack () {
	if (Request_PlayCustomTrack) {
		Request_PlayCustomTrack = false;
		return true;
	}
	return false;
}