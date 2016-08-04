#include "ThreadRequest.h"


#include "GlobalSettings.h"
#include "MusicState.h"
#include "MusicPlayer.h"



ThreadRequest threadRequest;
HANDLE hThreadMutex;		//"Thread" Mutex.  Lock when using the object threadState.



void ThreadRequest::requestSetPlaylist (Playlist* playlist, MusicType musicType, int queueMode, float delay) {
	LockHandle (hMusicPlayerMutex);
		float fadeOut = musicPlayer.getCurrentFadeOutLength ();
		float fadeIn = musicPlayer.getCurrentFadeInLength ();
	UnlockHandle (hMusicPlayerMutex);

	LockHandle (hMusicStateMutex);
		MusicType currentMusicType = music.GetCurrentMusicType (false);
	UnlockHandle (hMusicStateMutex);

	LockHandle (hThreadMutex);
	if (currentMusicType == musicType) {
		Request_Swap_FadeIn = fadeIn;
		Request_Swap_FadeOut = fadeOut;
	}

	if (currentMusicType == musicType && queueMode == 1) {
		//QueueMode 1 delays the update if music.GetCurrentMusicType == musicType
		//It will be updated next time the music player is in a stopped state.
		Request_Swap_Type = musicType;
		Request_Playlist = playlist;
		Request_Swap_Delay = delay * 1000;
	} else {
		//Immediately alter the playlist.
		//First check to see if there is any swap requests waiting for this music type.
		if (Request_Swap_Type == musicType) { //Clear it.
			Request_Swap_Type = MusicType::Mt_NotKnown;
		}
		if (queueMode < 2) {	//Now, request a song advance. The thread will carry it out if/when it can.
			Request_PlayNext = true;
			if (Request_PlayNext_MusicType != currentMusicType || Request_PlayNext_MusicType > 4) {
				Request_PlayNext_MusicType = musicType;
			}
		}
	}
	UnlockHandle (hThreadMutex);
}



void ThreadRequest::requestResetPlaylist (MusicType musicType) {
	LockHandle (hMusicStateMutex);
		MusicType realMusicType = music.GetRealMusicType ();
	UnlockHandle (hMusicStateMutex);
	if (musicType == MusicType::Undefined || musicType == realMusicType) {
		LockHandle (hThreadMutex);
		Request_PlayNext = true;
		Request_PlayNext_MusicType = musicType;
		UnlockHandle (hThreadMutex);
	}
}



void ThreadRequest::requestPlayCustomTrack (string& track) {
	LockHandle (hThreadMutex);
		Request_PlayCustomTrack = true;
		Request_Track = track;
	UnlockHandle (hThreadMutex);
}



bool ThreadRequest::hasRequestedCustomTrack () {
	if (Request_PlayCustomTrack) {
		Request_PlayCustomTrack = false;
		return true;
	}
	return false;
}


void ThreadRequest::requestHoldMusic (bool keepStopped) {
	LockHandle (hThreadMutex);
		Request_HoldMusic = keepStopped;
	LockHandle (hThreadMutex);
}