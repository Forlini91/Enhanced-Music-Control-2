#include "ThreadRequest.h"


#include "GameAPI.h"
#include "Globals.h"
#include "MusicState.h"
#include "MusicTimes.h"
#include "MusicPlayer.h"



ThreadRequest threadRequest;
HANDLE hThreadMutex = CreateMutex (nullptr, FALSE, nullptr);		//Lock when using the object threadState.






bool ThreadRequest::hasRequestedNextTrack () {
	if (nextTrack) {
		nextTrack = false;
		return true;
	}
	return false;
}




bool ThreadRequest::hasRequestedCustomTrack () const {
	return !customTrackPath.empty ();
}



bool ThreadRequest::hasRequestedHoldMusicPlayer () const {
	return holdMusicPlayer;
}



void ThreadRequest::clearRequestNextTrack () {
	nextTrack = false;
}






set<PendingPlaylist>& ThreadRequest::getPendingPlaylists () {
	return pendingPlaylists;
}

string ThreadRequest::getCustomTrack () {
	return customTrackPath;
}

void ThreadRequest::clearCustomTrack () {
	customTrackPath.clear ();
}









void ThreadRequest::requestPlayCustomTrack (const string &trackPath) {
	WaitForSingleObject (hThreadMutex, INFINITE);
		nextTrack = false;
		customTrackPath = trackPath;
	ReleaseMutex (hThreadMutex);
}



void ThreadRequest::requestSetPlaylist (ActivePlaylist *aplToSwap, Playlist *playlist, bool afterThisTrack, float delay) {
	WaitForSingleObject (hMusicTimesMutex, INFINITE);
		int fadeOut = musicTimes.getFadeOut ();
		int fadeIn = musicTimes.getFadeIn ();
	ReleaseMutex (hMusicTimesMutex);

	WaitForSingleObject (hThreadMutex, INFINITE);
		if (afterThisTrack || delay > 0) {
			pendingPlaylists.emplace (aplToSwap, playlist, afterThisTrack, delay, fadeOut, fadeIn);
		} else {
			aplToSwap->playlist = playlist;
			if (selectedActivePlaylist == aplToSwap) {
				setPlaylistFadeOut = fadeOut;
				setPlaylistFadeIn = fadeIn;
				nextTrack = true;
			}
		}
	ReleaseMutex (hThreadMutex);
}



bool ThreadRequest::requestHoldMusicPlayer (bool hold) {
	bool changed = false;
	WaitForSingleObject (hThreadMutex, INFINITE);
		if (holdMusicPlayer != hold) {
			holdMusicPlayer = hold;
			changed = true;
		}
	ReleaseMutex (hThreadMutex);
	return changed;
}



void ThreadRequest::requestNextTrack (bool noHold) {
	WaitForSingleObject (hThreadMutex, INFINITE);
		nextTrack = true;
		if (noHold) {
			holdMusicPlayer = false;
		}
	ReleaseMutex (hThreadMutex);
}