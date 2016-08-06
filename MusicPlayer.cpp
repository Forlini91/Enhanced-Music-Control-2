#include "MusicPlayer.h"

#include "Globals.h"
#include "FilePath.h"


#define VOLUME_MULT 2000.0
const float minVolume = pow (2, ABSOLUTE_MINIMUM_VOLUME / VOLUME_MULT);

using namespace std;



MusicPlayer musicPlayer;
HANDLE hMusicPlayerMutex = CreateMutex (nullptr, FALSE, nullptr);



bool QueuedTrack::get (string *name, REFERENCE_TIME *position) {
	if (isQueued ()) {
		*name = QueuedTrack::name;
		*position = QueuedTrack::position;
		queued = false;
		return true;
	} else {
		return false;
	}
}


bool QueuedTrack::isQueued () const {
	return queued;
}



MusicPlayer::~MusicPlayer () {
	if (pGraphBuilder) {
		pGraphBuilder->Release ();
		pGraphBuilder = nullptr;
	}
	if (pMediaControl) {
		pMediaControl->Release ();
		pMediaControl = nullptr;
	}
	if (pMediaSeeking) {
		pMediaSeeking->Release ();
		pMediaSeeking = nullptr;
	}
	if (pMediaEventEx) {
		pMediaEventEx->Release ();
		pMediaEventEx = nullptr;
	}
	if (pBasicAudio) {
		pBasicAudio->Release ();
		pBasicAudio = nullptr;
	}

	CoUninitialize ();
}



bool MusicPlayer::initialize (void) {
	if (isInitialized()) {		//Don't initialize again if already initialized.
		return true;
	} else if (initializeDShow (false)) {
		_MESSAGE ("Music player initialized");
		initialized = true;
		return true;
	} else {
		return false;
	}
}



bool MusicPlayer::isInitialized () const  {
	return initialized;
}


bool MusicPlayer::hasPlayedOnce () const {
	return playedOnce;
}


const char* MusicPlayer::getErrorMessage () const {
	return lastError;
}



bool MusicPlayer::isPlaying () const {
	return state == PlayerState::psPlaying;
}



bool MusicPlayer::isPaused () const  {
	return state == PlayerState::psPaused;
}



bool MusicPlayer::isStopped () const  {
	return state == PlayerState::psStopped;
}



bool MusicPlayer::isReady () const {
	return isPlaying() || isPaused() || isStopped();
}





bool MusicPlayer::isQueuedUp () const {
	return queuedTrack.isQueued();
}



bool MusicPlayer::queueTrack (const string &trackName) {
	return queueTrack (trackName, 0);
}



bool MusicPlayer::queueTrack (const string &trackName, REFERENCE_TIME position) {
	if (isInitialized() && isReady()) {
		//Queue up the track.  Since I don't want to be using the reference
		//provided (as it may become invalid at a later time) I will create a
		//copy of it.
		if (trackName.empty ()) {
			_MESSAGE ("MusicPlayer >> No track available");
			return false;
		} else if (exists (trackName) && !isDirectory (trackName) && isExtensionSupported (trackName)) {
			double pos = position / ONCE_SECOND;
			_MESSAGE ("MusicPlayer >> Queue new track >> %s (position: %.2f)", trackName.c_str(), pos);
			queuedTrack = QueuedTrack (trackName, position);
			return true;
		} else {
			_MESSAGE ("MusicPlayer >> Track can't be queued. Doesn't exists >> %s", trackName.c_str ());
		}
	}
	return false;
}



bool MusicPlayer::playQueuedTrack (FadeMethod newFadeMethod) {
	return playQueuedTrack (newFadeMethod, currentFadeOutLength, currentFadeInLength);
}



bool MusicPlayer::playQueuedTrack (FadeMethod newFadeMethod, int FadeLength) {
	return playQueuedTrack (newFadeMethod, FadeLength, FadeLength);
}



bool MusicPlayer::playQueuedTrack (FadeMethod newFadeMethod, int FadeOutLength, int FadeInLength) {
	//If the player is initialized, a track is queued up, and the player
	//isn't allready switching tracks...
	if (isInitialized() && isQueuedUp() && isReady()) {
		//...then we can play the new track!
		//So, this is pretty complicated.  First, is a file is allready
		//playing, we need to begin stopping it.  Depending on the fade
		//method, this could be instant, or it could not be.  If it isn't
		//instant, the DoPlayerStuff() function will carry out the stopping
		//and begin the playback.  Otherwise, we can do it here.
		//If the player isn't playing (or is paused) OR we won't have to fade out a currently playing track...
		if (isStopped () || isPaused () || (newFadeMethod != FadeMethod::fmFadeOut && newFadeMethod != FadeMethod::fmFadeOutThenIn)) {
			//...we can begin playing without issue.
			//Reset the player's interfaces.
			if (!resetDShow ()) {
				_MESSAGE (getErrorMessage ());
				return false;
			}

			string queuedTrackName;
			REFERENCE_TIME queuedTrackPosition;
			if (!queuedTrack.get (&queuedTrackName, &queuedTrackPosition)) {
				return false;
			}

			_MESSAGE ("MusicPlayer >> Play track >> \"%s\" (fade Out/In: %f/%f)", queuedTrackName.c_str (), FadeOutLength, FadeInLength);
			size_t length = 0;
			WCHAR wFileName[MAX_PATH];
			mbstowcs_s (&length, wFileName, queuedTrackName.c_str (), MAX_PATH);	//Convert the filepath to wide characters.
			HRESULT hr = pGraphBuilder->RenderFile (wFileName, nullptr);	//Create the filter graph.
			if (hr != S_OK) {
				lastError = "MusicPlayer >> Failed to create the DShow Graph to render the file.";
				state = PlayerState::psStopped;
				task = PlayerTask::ptNone;
				return false;
			}

			trackName = queuedTrackName;
			pMediaSeeking->GetDuration (&trackDuration);
			if (trackDuration <= queuedTrackPosition) {
				trackPosition = queuedTrackPosition;
			} else {
				trackPosition = 0;
			}
			pMediaSeeking->SetPositions (&trackPosition, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning);

			if (newFadeMethod == FadeMethod::fmFadeIn || newFadeMethod == FadeMethod::fmFadeOutThenIn) {
				state = PlayerState::psFadingIn;
				task = PlayerTask::ptQueue;
				//The fading is done by the DoPlayerStuff() function.
				fadeMethod = newFadeMethod;
				fadeOutPeriod = FadeOutLength;
				fadeInPeriod = FadeInLength;
				//Set the volume to -10000 (silent).
				pBasicAudio->put_Volume (EFFECTIVE_MINIMUM_VOLUME);
			} else {
				//If we don't need to fade in or anything, then we can just set playing.
				state = PlayerState::psPlaying;
				task = PlayerTask::ptNone;
				pBasicAudio->put_Volume (maximumVolume);
			}
			playedOnce = true;
			pMediaControl->Run ();
		} else {
			//Change the player's state.
			state = PlayerState::psFadingOut;
			task = PlayerTask::ptQueue;
			//We must fade out the currently playing track, then begin playing.
			fadeMethod = newFadeMethod;
			fadeOutPeriod = FadeOutLength;
			fadeInPeriod = FadeInLength;
			//TODO:  Add checks to ensure the file will play successfully
			//when it is rendered.
		}
		return true;
	}
	lastError = "MusicPlayer >> Either not initialized, no track is queued, or we're allready changing tracks.";
	return false;
}





const char* MusicPlayer::getTrack () const {
	return trackName.c_str ();
}



REFERENCE_TIME MusicPlayer::getTrackPosition () const {
	return hasPlayedOnce() ? trackDuration : 0;
}



bool MusicPlayer::setTrackPosition (bool lock, REFERENCE_TIME position, int fadeOut, int fadeIn) {
	//Only stop if currently playing, paused, or stopped.
	if (lock) LockHandle (hMusicPlayerMutex);
	if (isInitialized() && hasPlayedOnce() && isReady()) {
		fadeOutPeriod = fadeOut;
		fadeInPeriod = fadeIn;
		if (fadeOut > 0 && isPlaying()) {
			//We must fade to a stop.
			state = PlayerState::psFadingOut;
			task = PlayerTask::ptPosition;
			fadeMethod = FadeMethod::fmFadeOut;
			newTrackPosition = position;
		} else {
			//We can just resume.  Reset the position.
			state = PlayerState::psPlaying;
			task = PlayerTask::ptNone;
			pMediaSeeking->SetPositions (&position, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning);
			pMediaControl->Run ();
			pBasicAudio->put_Volume (maximumVolume);
		}
		if (lock) UnlockHandle (hMusicPlayerMutex);
		return true;
	}
	if (lock) UnlockHandle (hMusicPlayerMutex);
	return false;
}



REFERENCE_TIME MusicPlayer::getTrackDuration () const {
	return hasPlayedOnce() ? trackDuration : 0;
}




int MusicPlayer::getCurrentFadeInLength () const {
	return currentFadeInLength;
}



bool MusicPlayer::setCurrentFadeInLength (bool lock, int length) {
	if (lock) LockHandle (hMusicPlayerMutex);
	if (currentFadeInLength != length) {
		currentFadeInLength = length;
		if (lock) UnlockHandle (hMusicPlayerMutex);
		return true;
	}
	if (lock) UnlockHandle (hMusicPlayerMutex);
	return false;
}



int MusicPlayer::getCurrentFadeOutLength () const {
	return currentFadeOutLength;
}



bool MusicPlayer::setCurrentFadeOutLength (bool lock, int length) {
	if (lock) LockHandle (hMusicPlayerMutex);
	if (currentFadeOutLength != length) {
		currentFadeOutLength = length;
		if (lock) UnlockHandle (hMusicPlayerMutex);
		return true;
	}
	if (lock) UnlockHandle (hMusicPlayerMutex);
	return false;
}



int MusicPlayer::getMinPauseTime () const {
	return minPauseTime;
}



bool MusicPlayer::setMinPauseTime (bool lock, int length) {
	if (lock) LockHandle (hMusicPlayerMutex);
	if (minPauseTime != length) {
		minPauseTime = length;
		if (lock) UnlockHandle (hMusicPlayerMutex);
		return true;
	}
	if (lock) UnlockHandle (hMusicPlayerMutex);
	return false;
}



int MusicPlayer::getExtraPauseTime () const {
	return extraPauseTime;
}



bool MusicPlayer::setExtraPauseTime (bool lock, int length) {
	if (lock) LockHandle (hMusicPlayerMutex);
	if (extraPauseTime != length) {
		extraPauseTime = length;
		if (lock) UnlockHandle (hMusicPlayerMutex);
		return true;
	}
	if (lock) UnlockHandle (hMusicPlayerMutex);
	return false;
}



int MusicPlayer::getCalculatedPauseTime () const {
	return calculatedPauseTime;
}



void MusicPlayer::recalculatePauseTime (bool lock) {
	if (lock) LockHandle (hMusicPlayerMutex);
	calculatedPauseTime = minPauseTime + (extraPauseTime * rand () / RAND_MAX);
	if (lock) UnlockHandle (hMusicPlayerMutex);
}



int MusicPlayer::getMinBattleDelay () const {
	return minBattleTime;
}



bool MusicPlayer::setMinBattleDelay (bool lock, int length) {
	if (lock) LockHandle (hMusicPlayerMutex);
	if (minBattleTime != length) {
		minBattleTime = length;
		if (lock) UnlockHandle (hMusicPlayerMutex);
		return true;
	}
	if (lock) UnlockHandle (hMusicPlayerMutex);
	return false;
}



int MusicPlayer::getExtraBattleDelay () const {
	return extraBattleTime;
}



bool MusicPlayer::setExtraBattleDelay (bool lock, int length) {
	if (lock) LockHandle (hMusicPlayerMutex);
	if (extraBattleTime != length) {
		extraBattleTime = length;
		if (lock) UnlockHandle (hMusicPlayerMutex);
		return true;
	}
	if (lock) UnlockHandle (hMusicPlayerMutex);
	return false;
}



int MusicPlayer::getCalculatedBattleDelay () const {
	return calculatedBattleTime;
}



void MusicPlayer::recalculateBattleDelay (bool lock) {
	if (lock) LockHandle (hMusicPlayerMutex);
	calculatedBattleTime = minBattleTime + (extraBattleTime * rand () / RAND_MAX);
	if (lock) UnlockHandle (hMusicPlayerMutex);
}



int MusicPlayer::getMinAfterBattleDelay () const {
	return minAfterBattleTime;
}



bool MusicPlayer::setMinAfterBattleDelay (bool lock, int length) {
	if (lock) LockHandle (hMusicPlayerMutex);
	if (minAfterBattleTime != length) {
		minAfterBattleTime = length;
		if (lock) UnlockHandle (hMusicPlayerMutex);
		return true;
	}
	if (lock) UnlockHandle (hMusicPlayerMutex);
	return false;
}



int MusicPlayer::getExtraAfterBattleDelay () const {
	return extraAfterBattleTime;
}



bool MusicPlayer::setExtraAfterBattleDelay (bool lock, int length) {
	if (lock) LockHandle (hMusicPlayerMutex);
	if (extraAfterBattleTime != length) {
		extraAfterBattleTime = length;
		if (lock) UnlockHandle (hMusicPlayerMutex);
		return true;
	}
	if (lock) UnlockHandle (hMusicPlayerMutex);
	return false;
}



int MusicPlayer::getCalculatedAfterBattleDelay () const {
	return calculatedAfterBattleTime;
}



void MusicPlayer::recalculateAfterBattleDelay (bool lock) {
	if (lock) LockHandle (hMusicPlayerMutex);
	calculatedAfterBattleTime = minAfterBattleTime + (extraAfterBattleTime * rand () / RAND_MAX);
	if (lock) UnlockHandle (hMusicPlayerMutex);
}



int MusicPlayer::getMaxRestoreTime () const {
	return maxMusicRestoreTime;
}



bool MusicPlayer::setMaxRestoreTime (bool lock, int length) {
	if (lock) LockHandle (hMusicPlayerMutex);
	if (maxMusicRestoreTime != length) {
		maxMusicRestoreTime = length;
		if (lock) UnlockHandle (hMusicPlayerMutex);
		return true;
	}
	if (lock) UnlockHandle (hMusicPlayerMutex);
	return false;
}



bool MusicPlayer::setMaxMusicVolume (float fVolume) {
	if (isInitialized()) {
		fVolume = clamp (fVolume, 0, 1);
		long newMaximumVolume;
		if (fVolume <= minVolume) {
			newMaximumVolume = ABSOLUTE_MINIMUM_VOLUME;
		} else if (fVolume >= 1) {
			newMaximumVolume = MAXIMUM_VOLUME;
		} else {
			newMaximumVolume = VOLUME_MULT * log10 (fVolume);	//Volume is logarithmic.  This makes it linear.
		}
		if (maximumVolume != newMaximumVolume) {
			maximumVolume = newMaximumVolume;
			//If we aren't already adjusting the volume or the player's volume isn't at a critical point...
			if (isReady () || state == PlayerState::psFadedIn) {
				//...set the volume.
				pBasicAudio->put_Volume (newMaximumVolume);
			}
			return true;
		}
		return false;
	}
	return false;
}



double MusicPlayer::getMusicSpeed () const {
	double speed;
	pMediaSeeking->GetRate (&speed);
	return speed;
}



bool MusicPlayer::setMusicSpeed (bool lock, double speed) {
	if (lock) LockHandle (hMusicPlayerMutex);
	double prev = 0;
	pMediaSeeking->GetRate (&prev);
	if (prev != speed) {
		pMediaSeeking->SetRate (speed);
		if (lock) UnlockHandle (hMusicPlayerMutex);
		return true;
	}
	if (lock) UnlockHandle (hMusicPlayerMutex);
	return false;
}





bool MusicPlayer::resume (bool lock, int fadeIn) {
	if (lock) LockHandle (hMusicPlayerMutex);
	if (isInitialized () && isPaused ()) {
		if (fadeIn > 0) {
			//We must fade back in.
			state = PlayerState::psFadingIn;
			task = PlayerTask::ptResume;
			fadeMethod = FadeMethod::fmFadeIn;
			fadeInPeriod = fadeIn;
			pBasicAudio->put_Volume (EFFECTIVE_MINIMUM_VOLUME);
			pMediaControl->Run ();
		} else {
			//We can just resume.
			state = PlayerState::psPlaying;
			task = PlayerTask::ptNone;
			pBasicAudio->put_Volume (maximumVolume);
			pMediaControl->Run ();
		}
		if (lock) UnlockHandle (hMusicPlayerMutex);
		return true;
	}
	if (lock) UnlockHandle (hMusicPlayerMutex);
	return false;
}



bool MusicPlayer::pause (bool lock, int fadeOut) {
	if (lock) LockHandle (hMusicPlayerMutex);
	if (isInitialized() && isPlaying()) {
		if (fadeOut > 0) {
			//We must fade to a pause.
			state = PlayerState::psFadingOut;
			task = PlayerTask::ptPause;
			fadeMethod = FadeMethod::fmFadeOut;
			fadeOutPeriod = fadeOut;
		} else {
			//We can just pause.
			pMediaControl->Pause ();
			state = PlayerState::psPaused;
			task = PlayerTask::ptNone;
		}
		if (lock) UnlockHandle (hMusicPlayerMutex);
		return true;
	}
	if (lock) UnlockHandle (hMusicPlayerMutex);
	return false;
}



bool MusicPlayer::stop (bool lock, int fadeOut) {
	if (lock) LockHandle (hMusicPlayerMutex);
	if (isInitialized() && (isPlaying() || isPaused())) {
		if (fadeOut > 0 && isPlaying()) {
			//We must fade to a stop.
			state = PlayerState::psFadingOut;
			task = PlayerTask::ptStop;
			fadeMethod = FadeMethod::fmFadeOut;
			fadeOutPeriod = fadeOut;
		} else {
			//We can just stop.
			pMediaControl->Stop ();
			state = PlayerState::psStopped;
			task = PlayerTask::ptNone;
		}
		if (lock) UnlockHandle (hMusicPlayerMutex);
		return true;
	}
	if (lock) UnlockHandle (hMusicPlayerMutex);
	return false;
}



bool MusicPlayer::restart (bool lock, int fadeOut, int fadeIn) {
	if (lock) LockHandle (hMusicPlayerMutex);
	if (isInitialized() && hasPlayedOnce() && isReady()) {
		if (fadeOut > 0 && isPlaying()) {
			//We must fade to a stop.
			state = PlayerState::psFadingOut;
			task = PlayerTask::ptRestart;
			fadeMethod = FadeMethod::fmFadeOut;
			fadeOutPeriod = fadeOut;
			fadeInPeriod = fadeIn;
		} else {
			//We can just resume.  Reset the position.
			state = PlayerState::psFadingIn;
			task = PlayerTask::ptNone;
			trackPosition = 0;
			pMediaSeeking->SetPositions (&trackPosition, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning);
			pBasicAudio->put_Volume (maximumVolume);
			pMediaControl->Run ();
		}
		if (lock) UnlockHandle (hMusicPlayerMutex);
		return true;
	}
	if (lock) UnlockHandle (hMusicPlayerMutex);
	return false;
}







void MusicPlayer::updatePlayer (int sleepTime) {
	if (isInitialized()) {
		//This is the main function of the player.
		//For every iteration of the main thread this function should be called.
		//The player is, as most things, a simple state machine.
		//Every time, it will check to see if the state requires a task to be performed.
		//If so, it will perform it, otherwise this function does alot of nothing.

		//So, a quick rundown:
		//1)  Checks state and performs actions as neccessary.
		//2)  Checks for events from the player, mostly if the player has stopped.

		//However, first, it will check to see if the play-back is stopped.
		PlayerState currentState = checkState ();
		if (currentState == PlayerState::psUninitialized) {
			//Return, since there is nothing more to do.
			return;
		} else if (currentState != state) {
			//Clean up as needed.  If we, by chance, performed a ForceKill...
			//Now, if the player stopped since the last time we did player stuff.
			if (currentState == PlayerState::psStopped) {
				//Well, then lets stop.
				//First, lets see if we were fading in any direction.
				switch (state) {
					case PlayerState::psFadingIn:
						//Set volume to maximum as we no longer have to worry about fading.
						state = currentState;
						task = PlayerTask::ptNone;
						fadeTotalSleepTime = 0;			//Reset the sleep time for the next fade.
						pBasicAudio->put_Volume (maximumVolume);
						_MESSAGE ("MusicPlayer >> Switch state: FadingIn -> Stopped. task: Any -> None");
						break;
					case PlayerState::psFadingOut:
						if (task != PlayerTask::ptPause) {
							//Since we were in the middle of fading when the track stopped,
							//there was probably something we needed to do.  Lets set the
							//state to Faded so that we can handle it later.
							state = PlayerState::psFadedOut;
							fadeTotalSleepTime = 0;			//Reset the sleep time for the next fade.
							pBasicAudio->put_Volume (ABSOLUTE_MINIMUM_VOLUME);
							_MESSAGE ("MusicPlayer >> Switch state: FadingOut -> FadedOut. task: !Pause -> <MAINTAIN>");
						} else {
							//Otherwise, there is nothing to pause anymore.
							//Set to stopped instead.
							state = currentState;			//Set to Stopped.
							task = PlayerTask::ptNone;	//No longer any task.  Its stopped.
							fadeTotalSleepTime = 0;		//Reset the sleep time for the next fade.
							_MESSAGE ("MusicPlayer >> Switch state: FadingOut -> Stopped. task: Pause -> None");
						}
						break;
					case PlayerState::psFadedOut:
						//There is one thing we should look out for in this case.
						//If we faded and wanted to pause...  Well, there is
						//nothing to pause anymore.  Lets handle it so it is
						//stopped instead.
						if (task == PlayerTask::ptPause) {
							state = currentState;			//Set to Stopped.
							task = PlayerTask::ptNone;		//No longer any task.  Its stopped.
							_MESSAGE ("MusicPlayer >> Switch status: FadedOut -> Stopped. task: Pause -> None");
						}
						break;
					default:
						//Nothing else, we'll set the state accordingly.
						state = currentState;
						task = PlayerTask::ptNone;
						_MESSAGE ("MusicPlayer >> Switch status: Any -> Stopped. task: Any -> None");
						break;
				}
			}
		}

		//Now do typical player things.
		REFERENCE_TIME stopped;		//For the pMediaSeeking->GetPositions function.
		switch (state) {
			case PlayerState::psPlaying:
				//Keep track of the track's position.
				pMediaSeeking->GetPositions (&trackPosition, &stopped);
				break;
			case PlayerState::psStopped:
				break;
			case PlayerState::psFadingIn:
			case PlayerState::psFadingOut:
				//Keep track of the tracks position.
				pMediaSeeking->GetPositions (&trackPosition, &stopped);
				doFading (sleepTime);	//Do fading stuff.
				break;
			case PlayerState::psFadedIn:
			case PlayerState::psFadedOut:
				//Keep track of the track's position.
				pMediaSeeking->GetPositions (&trackPosition, &stopped);
				doTask ();	//Now continue the task.
				break;
		}
	}
}




bool MusicPlayer::doFading (int sleepTime) {
	fadeTotalSleepTime += sleepTime;
	float linearVolume = (maximumVolume != ABSOLUTE_MINIMUM_VOLUME) ? pow (10.0, 0.0005 * maximumVolume) : 0.0;
	float percentComplete;
	HRESULT hr;

	//Only two possible states.  FadingIn or FadingOut
	if (state == PlayerState::psFadingIn) {
		percentComplete = (fadeInPeriod != 0 ? float (fadeTotalSleepTime) / fadeInPeriod : 1.0);
		if (percentComplete >= 1.0) {
			_MESSAGE ("MusicPlayer >> Switch status: FadingIn -> FadedIn. task: Unchanged.");
			state = PlayerState::psFadedIn;					//Exit from fading.
			fadeTotalSleepTime = 0;
			hr = pBasicAudio->put_Volume (maximumVolume);	//Set the volume to maximumVolume
		} else if (linearVolume == 0.0) {
			hr = pBasicAudio->put_Volume (ABSOLUTE_MINIMUM_VOLUME);
		} else {
			hr = pBasicAudio->put_Volume (2000 * log10 (percentComplete * linearVolume));
		}
	} else { //if (state == PlayerState::psFadingOut) {
		percentComplete = (fadeOutPeriod ? float (fadeTotalSleepTime) / fadeOutPeriod : 1.0);
		if (percentComplete >= 1.0) {
			_MESSAGE ("MusicPlayer >> Switch status: FadingOut -> FadedOut. task: Unchanged");
			state = PlayerState::psFadedOut;	//Exit from fading.
			fadeTotalSleepTime = 0;
			hr = pBasicAudio->put_Volume (ABSOLUTE_MINIMUM_VOLUME);		//Set the volume to ABSOLUTE_MINIMUM_VOLUME
		} else if (linearVolume == 0.0) {
			hr = pBasicAudio->put_Volume (ABSOLUTE_MINIMUM_VOLUME);
		} else {
			hr = pBasicAudio->put_Volume (2000 * log10 ((1 - percentComplete) * linearVolume));
		}
	}

	switch (hr) {
		case E_INVALIDARG: lastError = "MusicPlayer >> DesiredVolume was outside of the range -10000 to 0."; break;
		case E_FAIL: lastError = "MusicPlayer >> Failed to alter the volume.  Underlying device suffered an error."; break;
		case E_NOTIMPL: lastError = "MusicPlayer >> Filter graph does not support volume manipulations via this interface."; break;
	}
	return percentComplete >= 1.0;
}



bool MusicPlayer::doTask () {
	if (state == PlayerState::psFadedIn) {
		//Applies to Queue, Resume and Restart.
		state = PlayerState::psPlaying;
		task = PlayerTask::ptNone;
		_MESSAGE ("MusicPlayer >> Switch status: FadingIn -> Playing. task: Any -> None");
	} else if (state == PlayerState::psFadedOut) {
		//Applies to Queue, Pause, Restart, and Stop.
		size_t length = 0;
		string queuedTrackName;
		HRESULT hr;
		switch (task) {
			case PlayerTask::ptQueue:
				if (!resetDShow ()) {
					_MESSAGE (getErrorMessage ());
					return false;
				}

				REFERENCE_TIME queuedTrackPosition;
				if (!queuedTrack.get (&queuedTrackName, &queuedTrackPosition)) {
					return false;
				}

				//Convert the filepath to wide characters.
				_MESSAGE ("MusicPlayer >> Play track >> \"%s\" (fade Out/In: %f/%f)", queuedTrackName.c_str (), fadeOutPeriod, fadeInPeriod);
				WCHAR wFileName[MAX_PATH];
				mbstowcs_s (&length, wFileName, queuedTrackName.c_str (), MAX_PATH);
				hr = pGraphBuilder->RenderFile (wFileName, nullptr);	//Create the filter graph.
				if (hr != S_OK) {
					lastError = "MusicPlayer >> Failed to create the DShow Graph to render the file.";
					state = PlayerState::psStopped;
					task = PlayerTask::ptNone;
					break;
				}
				//Get the duration of the current track and set the new position of the track.
				pMediaSeeking->GetDuration (&trackDuration);
				pMediaSeeking->SetPositions (&queuedTrackPosition, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning);
				if (fadeMethod == FadeMethod::fmFadeIn || fadeMethod == FadeMethod::fmFadeOutThenIn) {
					_MESSAGE ("MusicPlayer >> Switch status: FadedOut -> FadingIn. task: Queue");
					state = PlayerState::psFadingIn;
					//The fading is done by the doFading() function.
					//Set the volume to -10000 (silent).
					pBasicAudio->put_Volume (EFFECTIVE_MINIMUM_VOLUME);
				} else {
					_MESSAGE ("MusicPlayer >> Switch status: FadedOut -> Playing. task: Queue -> None");
					//If we don't need to fade in or anything, then we can just set playing.
					state = PlayerState::psPlaying;
					task = PlayerTask::ptNone;
					pBasicAudio->put_Volume (maximumVolume);
				}

				playedOnce = true;
				pMediaControl->Run ();
				break;
			case PlayerTask::ptPause:
				//Now that we have faded out, we can pause the track.
				pMediaControl->Pause ();
				state = PlayerState::psPaused;
				task = PlayerTask::ptNone;
				_MESSAGE ("MusicPlayer >> Switch status: FadedOut -> Paused. task: Pause -> None");
				break;
			case PlayerTask::ptRestart:
				//Now we can restart the track.
				//This requires stopping the playback, resetting the position, and running it again.
				trackPosition = 0;
				pMediaControl->Stop ();
				pMediaSeeking->SetPositions (&trackPosition, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning);
				pMediaControl->Run ();
				state = PlayerState::psFadingIn;
				task = PlayerTask::ptNone;
				_MESSAGE ("MusicPlayer >> Switch status: FadedOut -> FadingIn. task: Restart -> None");
				break;
			case PlayerTask::ptStop:
				pMediaControl->Stop ();
				state = PlayerState::psStopped;
				task = PlayerTask::ptNone;
				_MESSAGE ("MusicPlayer >> Switch status: FadedOut -> Stopped. task: Stopped -> None");
				break;
			case PlayerTask::ptPosition:
				//Now we can change the track position.
				//This requires stopping the playback, setting the position, and running it again.
				pMediaControl->Stop ();
				pMediaSeeking->SetPositions (&newTrackPosition, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning);
				pMediaControl->Run ();
				newTrackPosition = 0;
				state = PlayerState::psFadingIn;
				task = PlayerTask::ptNone;
				_MESSAGE ("MusicPlayer >> Switch status: FadedOut -> FadingIn. task: Position -> None");
				break;
		}
	}
	return true;
}



PlayerState MusicPlayer::checkState () {
	if (!pMediaEventEx) {
		lastError = "MusicPlayer >> pMediaEventEX was undefined.  ForceKill()ing to prevent disaster.";
		forceKill ();
		return PlayerState::psUninitialized;
	}

	PlayerState retVal = state;
	long evCode, param1, param2;
	while (SUCCEEDED (pMediaEventEx->GetEvent (&evCode, &param1, &param2, 0))) {
		// We process only the EC_COMPLETE message which is sent when the media is finished playing.
		if (evCode == EC_COMPLETE) {
			retVal = PlayerState::psStopped;	//Set state to Stopped when finished playing.
		}
	}
	return retVal;
}



bool MusicPlayer::initializeDShow (bool reset) {
	//Get an instance of the graph builder.
	HRESULT hr = CoCreateInstance (CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pGraphBuilder);
	if (FAILED (hr) || !pGraphBuilder) {	//Check to see that the GraphBuilder is available.
		switch (hr) {			//There was a problem.  Fail.
			case REGDB_E_CLASSNOTREG:	lastError = "MusicPlayer >> Failed to create an instance of IGraphBuilder.  Class was not registered."; break;
			case CLASS_E_NOAGGREGATION:	lastError = "MusicPlayer >> Failed to create an instance of IGraphBuilder.  This class cannot be created as part of an aggregate."; break;
			case E_NOINTERFACE:			lastError = "MusicPlayer >> Failed to create an instance of IGraphBuilder.  There was a problem with the interface."; break;
			case S_OK:					lastError = "MusicPlayer >> An instance of the interface was, supposedly, created...  But its null, which means it wasn't."; break;
			default:					lastError = "MusicPlayer >> An unexpected error was thrown during the creation of the interface.  The pointer, for some reason, is just null."; break;
		}
		if (reset) forceKill ();
		return false;
	}

	//Reinitialize the DShow interfaces.
	//Now, get the other instances we require from the GraphBuilder.
	hr = pGraphBuilder->QueryInterface (IID_IMediaControl, (void **)&pMediaControl);
	if (FAILED (hr) || !pMediaControl) {
		lastError = "MusicPlayer >> Failed to obtain Media Control interface from pGraphBuilder.";
		if (reset) forceKill ();
		return false;
	}

	hr = pGraphBuilder->QueryInterface (IID_IMediaSeeking, (void**)&pMediaSeeking);
	if (FAILED (hr) || !pMediaSeeking) {
		lastError = "MusicPlayer >> Failed to obtain Media Seeking interface from pGraphBuilder.";
		if (reset) forceKill ();
		return false;
	}

	hr = pGraphBuilder->QueryInterface (IID_IMediaEventEx, (void**)&pMediaEventEx);
	if (FAILED (hr) || !pMediaEventEx) {
		lastError = "MusicPlayer >> Failed to obtain Media Event Ex interface from pGraphBuilder.";
		if (reset) forceKill ();
		return false;
	}

	hr = pGraphBuilder->QueryInterface (IID_IBasicAudio, (void**)&pBasicAudio);
	if (FAILED (hr) || !pBasicAudio) {
		lastError = "MusicPlayer >> Failed to obtain Basic Audio interface from pGraphBuilder.";
		if (reset) forceKill ();
		return false;
	}

	return true;
}



bool MusicPlayer::resetDShow () {
	//If we have allready used these DShow interfaces...
	if (playedOnce) {
		if (pMediaControl) {
			pMediaControl->Stop ();
		}
		if (initializeDShow (true)) {
			pBasicAudio->put_Volume (maximumVolume);		//Reset the volume.
			return true;
		}
	}
	return false;
}



void MusicPlayer::forceKill () {
	_MESSAGE ("MusicPlayer >> Force killed!");
	initialized = false;	//Uninitialize the class. This will make sure no functions can execute.
	if (state != PlayerState::psUninitialized) {
		if (state != PlayerState::psStopped) {
			if (pMediaControl) {
				pMediaControl->Stop ();
			}
		}
		state = PlayerState::psUninitialized;
		task = PlayerTask::ptNone;
	}
}