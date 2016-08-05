#include "ThreadRequest.h"


#include "GameAPI.h"
#include "Globals.h"
#include "MusicState.h"
#include "MusicPlayer.h"
#include "ThreadState.h"



ThreadRequest threadRequest;
HANDLE hThreadMutex;		//"Thread" Mutex.  Lock when using the object threadState.



bool ThreadRequest::hasRequests () {
	return PlayNextTrack && (
		Swap_MusicType_Next == threadState.newType || (
			Swap_MusicType_Next == MusicType::Undefined && !threadState.newTypeIsSpecial
		)
	);
}



bool ThreadRequest::hasRequestedSetPlaylist () const {
	return Swap_Playlist != nullptr && Swap_MusicType != MusicType::Mt_NotKnown;
}



bool ThreadRequest::hasRequestedCustomTrack () const {
	return !CustomTrack_Name.empty ();
}



bool ThreadRequest::hasRequestedHoldMusic () const {
	return HoldMusic;
}








void ThreadRequest::getSwapData (Playlist **newPlaylist, MusicType *musicType) {
	//Assume caller requested hThreadMutex
		*newPlaylist = Swap_Playlist;
		*musicType = Swap_MusicType;
		Swap_Playlist = nullptr;
		Swap_MusicType = MusicType::Mt_NotKnown;
	//Assume caller will release hThreadMutex
}



void ThreadRequest::getSwapFadeTimes (float *fadeIn, float *fadeOut) {
	//Assume caller requested hThreadMutex
		if (Swap_FadeIn >= 0) {
			*fadeIn = Swap_FadeIn;
			Swap_FadeIn = -1;
		}
		if (Swap_FadeOut >= 0) {
			*fadeOut = Swap_FadeOut;
			Swap_FadeOut = -1;
		}
	//Assume caller will release hThreadMutex
}



bool ThreadRequest::checkDelayedSetPlaylist (int timePassed) {
	//Assume caller requested hThreadMutex
		if (threadRequest.Swap_Delay > 0) {
			if ((threadRequest.Swap_Delay -= timePassed) <= 0) {
				_MESSAGE ("Thread >> Time to swap after the delay");
				return true;
			}
		}
		return false;
	//Assume caller will release hThreadMutex
}



string ThreadRequest::getCustomTrack () {
	string ret;
	//Assume caller requested hThreadMutex
	ret.swap (CustomTrack_Name);			//Get and clear
	//Assume caller will release hThreadMutex
	return ret;
}



void ThreadRequest::cleanRequests () {
	if (PlayNextTrack) {
		_MESSAGE ("Thread >> Perform request >> Music type: %d", threadRequest.Swap_MusicType_Next);
		PlayNextTrack = false;
	}
	Swap_MusicType_Next = MusicType::Mt_NotKnown;
}








void ThreadRequest::requestSetPlaylist (Playlist *playlist, MusicType musicType, int queueMode, float delay) {
	LockHandle (hMusicPlayerMutex);
		float fadeOut = musicPlayer.getCurrentFadeOutLength ();
		float fadeIn = musicPlayer.getCurrentFadeInLength ();
	UnlockHandle (hMusicPlayerMutex);

	LockHandle (hMusicStateMutex);
		MusicType currentMusicType = music.GetCurrentMusicType (false);
	UnlockHandle (hMusicStateMutex);

	LockHandle (hThreadMutex);
		if (currentMusicType == musicType) {
			Swap_FadeIn = fadeIn;
			Swap_FadeOut = fadeOut;
		}

		if (currentMusicType == musicType && queueMode == 1) {
			//QueueMode 1 delays the update if music.GetCurrentMusicType == musicType
			//It will be updated next time the music player is in a stopped state.
			Swap_MusicType = musicType;
			playlist = playlist;
			Swap_Delay = delay * 1000;
		} else {
			//Immediately alter the playlist.
			//First check to see if there is any swap requests waiting for this music type.
			if (Swap_MusicType == musicType) { //Clear it.
				Swap_MusicType = MusicType::Mt_NotKnown;
			}
			if (queueMode < 2) {	//Now, request a song advance. The thread will carry it out if/when it can.
				PlayNextTrack = true;
				if (Swap_MusicType_Next != currentMusicType || Swap_MusicType_Next > 4) {
					Swap_MusicType_Next = musicType;
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
			PlayNextTrack = true;
			Swap_MusicType_Next = musicType;
		UnlockHandle (hThreadMutex);
	}
}



void ThreadRequest::requestPlayCustomTrack (string &track) {
	LockHandle (hThreadMutex);
		CustomTrack_Name = track;
	UnlockHandle (hThreadMutex);
}



bool ThreadRequest::requestHoldMusic (bool hold) {
	if (HoldMusic != hold) {
		LockHandle (hThreadMutex);
			HoldMusic = hold;
		LockHandle (hThreadMutex);
		return true;
	}
	return false;
}



void ThreadRequest::requestNextTrack (bool noHold) {
	LockHandle (hThreadMutex);
		threadRequest.PlayNextTrack = true;
		threadRequest.Swap_MusicType_Next = MusicType::Undefined;
		if (noHold) {
			threadRequest.HoldMusic = false;
		}
	UnlockHandle (hThreadMutex);
}