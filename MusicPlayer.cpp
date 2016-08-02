#include "MusicPlayer.h"

#include "GlobalSettings.h"
#include "FilePath.h"


using namespace std;



MusicPlayer::~MusicPlayer () {
	if (pGraphBuilder) {
		pGraphBuilder->Release ();
		pGraphBuilder = NULL;
	}
	if (pMediaControl) {
		pMediaControl->Release ();
		pMediaControl = NULL;
	}
	if (pMediaSeeking) {
		pMediaSeeking->Release ();
		pMediaSeeking = NULL;
	}
	if (pMediaEventEx) {
		pMediaEventEx->Release ();
		pMediaEventEx = NULL;
	}
	if (pBasicAudio) {
		pBasicAudio->Release ();
		pBasicAudio = NULL;
	}

	CoUninitialize ();
}



bool MusicPlayer::initialize (void) {
	//Don't initialize again if allready initialized.
	if (isInitialized()) {
		return true;
	}

	//Get an instance of the graph builder.
	HRESULT hr = CoCreateInstance (CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pGraphBuilder);
	if (!pGraphBuilder) {		//Check to see that the GraphBuilder is available.
		switch (hr) {			//There was a problem.  Fail.
			case REGDB_E_CLASSNOTREG:	lastError = "MusicPlayer >> Failed to create an instance of IGraphBuilder.  Class was not registered."; break;
			case CLASS_E_NOAGGREGATION:	lastError = "MusicPlayer >> Failed to create an instance of IGraphBuilder.  This class cannot be created as part of an aggregate."; break;
			case E_NOINTERFACE:			lastError = "MusicPlayer >> Failed to create an instance of IGraphBuilder.  There was a problem with the interface."; break;
			case S_OK:					lastError = "MusicPlayer >> An instance of the interface was, supposedly, created...  But its null, which means it wasn't."; break;
			default:					lastError = "MusicPlayer >> An unexpected error was thrown during the creation of the interface.  The pointer, for some reason, is just null.  Error Code:  " + to_string (hr); break;
		}
		return false;
	}

	//Now, get the other instances we require from the GraphBuilder.
	hr = pGraphBuilder->QueryInterface (IID_IMediaControl, (void **)&pMediaControl);
	if (!pMediaControl) {
		lastError = "MusicPlayer >> Failed to obtain Media Control interface from pGraphBuilder.";
		return false;
	}

	hr = pGraphBuilder->QueryInterface (IID_IMediaSeeking, (void**)&pMediaSeeking);
	if (!pMediaSeeking) {
		lastError = "MusicPlayer >> Failed to obtain Media Seeking interface from pGraphBuilder.";
		return false;
	}

	hr = pGraphBuilder->QueryInterface (IID_IMediaEventEx, (void**)&pMediaEventEx);
	if (!pMediaEventEx) {
		lastError = "MusicPlayer >> Failed to obtain Media Event Ex interface from pGraphBuilder.";
		return false;
	}

	hr = pGraphBuilder->QueryInterface (IID_IBasicAudio, (void**)&pBasicAudio);
	if (!pBasicAudio) {
		lastError = "MusicPlayer >> Failed to obtain Basuc Audio interface from pGraphBuilder.";
		return false;
	}

	initialized = true;
	return true;
}



bool MusicPlayer::isInitialized () const  {
	return initialized;
}


bool MusicPlayer::hasPlayedOnce () const {
	return playedOnce;
}


const string& MusicPlayer::getErrorMessage () const {
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



bool MusicPlayer::isSwitching () const  {
	return !(isPlaying () || isPaused () || isStopped ());
}





bool MusicPlayer::isQueuedUp () const {
	return trackQueuedUp;
}



bool MusicPlayer::queueTrack (const string& trackName) {
	return queueTrack (trackName, 0);
}



bool MusicPlayer::queueTrack (const string& trackName, LONGLONG Position) {
	if (isInitialized() && !isSwitching()) {
		//Queue up the track.  Since I don't want to be using the reference
		//provided (as it may become invalid at a later time) I will create a
		//copy of it.
		if (trackName.empty ()) {
			_MESSAGE ("MusicPlayer >> No track available");
			return false;
		} else {
			if (true) {
				double pos = (Position / ((double)10000000L));
				_MESSAGE ("MusicPlayer >> Queue new track >> %s (position: %.2f)", trackName.c_str(), pos);
				queuedTrack = trackName;
				queuedTrackPosition = Position;
				trackQueuedUp = true;
				return true;
			} else {
				_MESSAGE ("MusicPlayer >> Track can't be queued. Doesn't exists >> %s");
			}
		}
	}
	lastError = "MusicPlayer >> Either not initialized or in the middle of changing tracks.";
	return false;
}



bool MusicPlayer::playQueuedTrack (FadeMethod newFadeMethod) {
	return playQueuedTrack (newFadeMethod, currentFadeOutLength, currentFadeInLength);
}



bool MusicPlayer::playQueuedTrack (FadeMethod newFadeMethod, float FadeLength) {
	return playQueuedTrack (newFadeMethod, FadeLength, FadeLength);
}



bool MusicPlayer::playQueuedTrack (FadeMethod newFadeMethod, float FadeOutLength, float FadeInLength) {
	//If the player is initialized, a track is queued up, and the player
	//isn't allready switching tracks...
	if (isInitialized() && isQueuedUp() && !isSwitching()) {
		WCHAR wFileName[MAX_PATH];
		HRESULT hr;

		//...then we can play the new track!
		//So, this is pretty complicated.  First, is a file is allready
		//playing, we need to begin stopping it.  Depending on the fade
		//method, this could be instant, or it could not be.  If it isn't
		//instant, the DoPlayerStuff() function will carry out the stopping
		//and begin the playback.  Otherwise, we can do it here.
		//If the player isn't playing (or is paused) OR we won't have to fade out a currently playing track...
		_MESSAGE ("MusicPlayer >> Play track >> \"%s\" (fade Out/In: %f/%f)", queuedTrack.c_str (), FadeOutLength, FadeInLength);
		if ((state == PlayerState::psStopped || state == PlayerState::psPaused) || (newFadeMethod != FadeMethod::fmFadeOut && fadeMethod != FadeMethod::fmFadeOutThenIn)) {
			//...we can begin playing without issue.
			//Reset the player's interfaces.
			ResetDShow ();
			//Convert the filepath to wide characters.
			size_t length = 0;
			mbstowcs_s (&length, wFileName, queuedTrack.c_str (), MAX_PATH);
			//Create the filter graph.
			hr = pGraphBuilder->RenderFile (wFileName, NULL);
			if (hr != S_OK) {
				lastError = "MusicPlayer >> Failed to create the DShow Graph to render the file.";
				state = PlayerState::psStopped;
				task = PlayerTask::ptNone;
				return false;
			}
			//Get the duration of the current track.
			pMediaSeeking->GetDuration (&currentTrackDuration);
			//If the position is greater than the duration...
			if (currentTrackDuration > queuedTrackPosition) {
				//...then it is obviously a bad value.  Start from the beginning.
				queuedTrackPosition = 0;
			}
			//Set the position of the track.
			pMediaSeeking->SetPositions (&queuedTrackPosition, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);

			if (newFadeMethod == FadeMethod::fmFadeIn || newFadeMethod == FadeMethod::fmFadeOutThenIn) {
				//With these fade methods, we will need to perform a gradual fade in.
				//The fading is done by the DoPlayerStuff() function.

				//Indicate we have started a long switch.
				fadeMethod = newFadeMethod;
				fadeOutPeriod = FadeOutLength;
				fadeInPeriod = FadeInLength;

				//Set the volume to -10000 (silent).
				pBasicAudio->put_Volume (EFFECTIVE_MINIMUM_VOLUME);

				//Change the player's state.
				state = PlayerState::psFadingIn;
				task = PlayerTask::ptQueue;
			} else {
				//If we don't need to fade in or anything, then we can just set playing.
				state = PlayerState::psPlaying;
				task = PlayerTask::ptNone;
				pBasicAudio->put_Volume (maximumVolume);
			}
			trackQueuedUp = false;
			playedOnce = true;
			pMediaControl->Run ();
		} else {
			//Change the player's state.
			state = PlayerState::psFadingOut;
			task = PlayerTask::ptQueue;
			//We must fade out the currently playing track, then begin playing.
			//Indicate we are switching.
			fadeMethod = newFadeMethod;
			fadeOutPeriod = FadeOutLength;
			fadeInPeriod = FadeInLength;
			//This is technically all we need to do.  The DoPlayerStuff()
			//function has the ball now.  Will it run with it?  We can't
			//actually know, you see, since we can't call RenderFile right
			//now, we have no idea if it will succeed or not.  So, all we
			//can do is return true now.

			//TODO:  Add checks to ensure the file will play successfully
			//when it is rendered.
		}
		return true;
	}
	lastError = "MusicPlayer >> Either not initialized, no track is queued, or we're allready changing tracks.";
	return false;
}





const string& MusicPlayer::getTrack () const {
	return queuedTrack;
}



REFERENCE_TIME MusicPlayer::getTrackPosition () const {
	if (state != PlayerState::psStopped) {
		return currentTrackPosition;
	}
	return 0;
}



bool MusicPlayer::setTrackPosition (int fadeOut, int fadeIn, REFERENCE_TIME position) {
	//Only stop if currently playing, paused, or stopped.
	if (initialized && playedOnce && !isSwitching()) {
		fadeOutPeriod = fadeOut;
		fadeInPeriod = fadeIn;
		if (fadeOut > 0 && state == PlayerState::psPlaying) {
			//We must fade to a stop.
			state = PlayerState::psFadingOut;
			task = PlayerTask::ptPosition;
			fadeMethod = FadeMethod::fmFadeOut;
			newTrackPosition = position;
		} else {
			//We can just resume.  Reset the position.
			state = PlayerState::psPlaying;
			task = PlayerTask::ptNone;
			pMediaSeeking->SetPositions (&position, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
			pMediaControl->Run ();
			pBasicAudio->put_Volume (maximumVolume);
		}
		return true;
	}
	return false;
}



REFERENCE_TIME MusicPlayer::getTrackDuration () const {
	return currentTrackDuration;
}




float MusicPlayer::getCurrentFadeInLength () const {
	return currentFadeInLength;
}



bool MusicPlayer::setCurrentFadeInLength (float length) {
	if (currentFadeInLength != length) {
		currentFadeInLength = length;
		return true;
	}
	return false;
}



float MusicPlayer::getCurrentFadeOutLength () const {
	return currentFadeOutLength;
}



bool MusicPlayer::setCurrentFadeOutLength (float length) {
	if (currentFadeOutLength != length) {
		currentFadeOutLength = length;
		return true;
	}
	return false;
}



float MusicPlayer::getMinPauseTime () const {
	return minPauseTime;
}



bool MusicPlayer::setMinPauseTime (float length) {
	if (minPauseTime != length) {
		minPauseTime = length;
		return true;
	}
	return false;
}



float MusicPlayer::getExtraPauseTime () const {
	return extraPauseTime;
}



bool MusicPlayer::setExtraPauseTime (float length) {
	if (extraPauseTime != length) {
		extraPauseTime = length;
		return true;
	}
	return false;
}



float MusicPlayer::getCalculatedPauseTime () const {
	return calculatedPauseTime;
}



void MusicPlayer::recalculatePauseTime () {
	calculatedPauseTime = minPauseTime + (extraPauseTime * rand () / RAND_MAX);
}



float MusicPlayer::getMinBattleDelay () const {
	return minBattleTime;
}



bool MusicPlayer::setMinBattleDelay (float length) {
	if (minBattleTime != length) {
		minBattleTime = length;
		return true;
	}
	return false;
}



float MusicPlayer::getExtraBattleDelay () const {
	return extraBattleTime;
}



bool MusicPlayer::setExtraBattleDelay (float length) {
	if (extraBattleTime != length) {
		extraBattleTime = length;
		return true;
	}
	return false;
}



float MusicPlayer::getCalculatedBattleDelay () const {
	return calculatedBattleTime;
}



void MusicPlayer::recalculateBattleDelay () {
	calculatedBattleTime = minBattleTime + (extraBattleTime * rand () / RAND_MAX);
}



float MusicPlayer::getMinAfterBattleDelay () const {
	return minAfterBattleTime;
}



bool MusicPlayer::setMinAfterBattleDelay (float length) {
	if (minAfterBattleTime != length) {
		minAfterBattleTime = length;
		return true;
	}
	return false;
}



float MusicPlayer::getExtraAfterBattleDelay () const {
	return extraAfterBattleTime;
}



bool MusicPlayer::setExtraAfterBattleDelay (float length) {
	if (extraAfterBattleTime != length) {
		extraAfterBattleTime = length;
		return true;
	}
	return false;
}



float MusicPlayer::getCalculatedAfterBattleDelay () const {
	return calculatedAfterBattleTime;
}



void MusicPlayer::recalculateAfterBattleDelay () {
	calculatedAfterBattleTime = minAfterBattleTime + (extraAfterBattleTime * rand () / RAND_MAX);
}



float MusicPlayer::getMaxRestoreTime () const {
	return maxMusicRestoreTime;
}



bool MusicPlayer::setMaxRestoreTime (float length) {
	if (maxMusicRestoreTime != length) {
		maxMusicRestoreTime = length;
		return true;
	}
	return false;
}



bool MusicPlayer::setMaxMusicVolume (long lVolume) {
	if (isInitialized() && lVolume != maximumVolume) {
		if (lVolume >= ABSOLUTE_MINIMUM_VOLUME && lVolume <= MAXIMUM_VOLUME) {
			maximumVolume = lVolume;
			//If we allready aren't adjusting the volume or the player's volume isn't at a critical point...
			//if(playerData.state != PlayerState::psFadingIn && playerData.state != PlayerState::psFadingOut && playerData.state != PlayerState::psFadedOut && playerData.state != PlayerState::psFadedIn)
			if (isSwitching() || state == PlayerState::psFadedIn) {
				//...set the volume.
				pBasicAudio->put_Volume (lVolume);
			}
			return true;
		}
		lastError = "MusicPlayer >> The provided volume was out of range (-10000 to 0).";
	}
	return false;
}



double MusicPlayer::getMusicSpeed () const {
	double speed;
	pMediaSeeking->GetRate (&speed);
	return speed;
}



bool MusicPlayer::setMusicSpeed (double speed) {
	double prev = 0;
	pMediaSeeking->GetRate (&prev);
	if (prev != speed) {
		pMediaSeeking->SetRate (speed);
		return true;
	}
	return false;
}





bool MusicPlayer::resume (int fadeIn) {
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
		return true;
	}
	return false;
}



bool MusicPlayer::pause (int fadeOut) {
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
		return true;
	}
	return false;
}



bool MusicPlayer::stop (int fadeOut) {
	//Only stop if currently playing or paused.
	if (initialized && (isPlaying() || isPaused())) {
		if (fadeOut > 0 && isPlaying()) {
			//We must fade to a stop.
			state = PlayerState::psFadingOut;
			task = PlayerTask::ptStop;
			fadeMethod = FadeMethod::fmFadeOut;
			fadeOutPeriod = fadeOut;
			return true;
		} else {
			//We can just stop.
			pMediaControl->Stop ();
			state = PlayerState::psStopped;
			task = PlayerTask::ptNone;
		}
		return true;
	}
	return false;
}



bool MusicPlayer::restart (int fadeOut, int fadeIn) {
	//Only stop if currently playing, paused, or stopped.
	if (isInitialized() && hasPlayedOnce() && !isSwitching()) {
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
			currentTrackPosition = 0;
			pMediaSeeking->SetPositions (&currentTrackPosition, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
			pBasicAudio->put_Volume (maximumVolume);
			pMediaControl->Run ();
		}
		return true;
	}
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
				pMediaSeeking->GetPositions (&currentTrackPosition, &stopped);
				break;
			case PlayerState::psStopped:
				break;
			case PlayerState::psFadingIn:
			case PlayerState::psFadingOut:
				//Keep track of the tracks position.
				pMediaSeeking->GetPositions (&currentTrackPosition, &stopped);
				doFading (sleepTime);	//Do fading stuff.
				break;
			case PlayerState::psFadedIn:
			case PlayerState::psFadedOut:
				//Keep track of the track's position.
				pMediaSeeking->GetPositions (&currentTrackPosition, &stopped);
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
		//Its all pretty simple.  Both these simply set the state to playing.
		state = PlayerState::psPlaying;
		task = PlayerTask::ptNone;
		_MESSAGE ("MusicPlayer >> Switch status: FadingIn -> Playing. task: Any -> None");
	} else if (state == PlayerState::psFadedOut) {
		//Applies to Queue, Pause, Restart, and Stop.
		size_t length = 0;
		switch (task) {
			case PlayerTask::ptQueue:
				if (!ResetDShow ()) {
					return false;
				}
				//Now that we faded out, its time to begin the queued track.
				WCHAR wFileName[MAX_PATH];
				HRESULT hr;
				//Convert the filepath to wide characters.
				mbstowcs_s (&length, wFileName, queuedTrack.c_str (), MAX_PATH);
				//Create the filter graph.
				hr = pGraphBuilder->RenderFile (wFileName, NULL);
				if (hr != S_OK) {
					lastError = "MusicPlayer >> Failed to create the DShow Graph to render the file.";
					//In the case of an error, just set the player to Stopped.
					state = PlayerState::psStopped;
					task = PlayerTask::ptNone;
					break;
				}
				//Get the duration of the current track.
				pMediaSeeking->GetDuration (&currentTrackDuration);
				//Set the position of the track.
				pMediaSeeking->SetPositions (&queuedTrackPosition, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);

				if (fadeMethod == FadeMethod::fmFadeIn || fadeMethod == FadeMethod::fmFadeOutThenIn) {
					//With these fade methods, we will need to perform a gradual fade in.
					//The fading is done by the DoPlayerStuff() function.

					//Set the volume to -10000 (silent).
					//This is just in case...
					pBasicAudio->put_Volume (EFFECTIVE_MINIMUM_VOLUME);

					//Change the player's state.
					state = PlayerState::psFadingIn;
					_MESSAGE ("MusicPlayer >> Switch status: FadedOut -> FadingIn. task: Queue");
				} else {
					//If we don't need to fade in or anything, then we can just set playing.
					pBasicAudio->put_Volume (maximumVolume);
					state = PlayerState::psPlaying;
					task = PlayerTask::ptNone;
					_MESSAGE ("MusicPlayer >> Switch status: FadedOut -> Playing. task: Queue -> None");
				}

				trackQueuedUp = false;
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
				currentTrackPosition = 0;
				pMediaControl->Stop ();
				pMediaSeeking->SetPositions (&currentTrackPosition, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
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
				pMediaSeeking->SetPositions (&newTrackPosition, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
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



//Resets DirectSHow for whenever we want to play a new file, basically.
bool MusicPlayer::ResetDShow () {
	//If we have allready used these DShow interfaces...
	if (playedOnce) {
		//...we should reset them before using them again. Stop playback.
		pMediaControl->Stop ();

		//Get an instance of the graph builder.
		HRESULT hr = CoCreateInstance (CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pGraphBuilder);
		if (FAILED(hr) || !pGraphBuilder) {	//Check to see that the GraphBuilder is available.
			lastError = "MusicPlayer >> Failed to create an instance of IGraphBuilder.";
			forceKill ();
			return false;
		}

		//Reinitialize the DShow interfaces.
		//Now, get the other instances we require from the GraphBuilder.
		hr = pGraphBuilder->QueryInterface (IID_IMediaControl, (void **)&pMediaControl);
		if (FAILED (hr) || !pMediaControl) {
			lastError = "MusicPlayer >> Failed to obtain Media Control interface from pGraphBuilder.";
			forceKill ();
			return false;
		}

		hr = pGraphBuilder->QueryInterface (IID_IMediaSeeking, (void**)&pMediaSeeking);
		if (FAILED (hr) || !pMediaSeeking) {
			lastError = "MusicPlayer >> Failed to obtain Media Seeking interface from pGraphBuilder.";
			forceKill ();
			return false;
		}

		hr = pGraphBuilder->QueryInterface (IID_IMediaEventEx, (void**)&pMediaEventEx);
		if (FAILED (hr) || !pMediaEventEx) {
			lastError = "MusicPlayer >> Failed to obtain Media Event Ex interface from pGraphBuilder.";
			forceKill ();
			return false;
		}

		hr = pGraphBuilder->QueryInterface (IID_IBasicAudio, (void**)&pBasicAudio);
		if (FAILED (hr) || !pBasicAudio) {
			lastError = "MusicPlayer >> Failed to obtain Basic Audio interface from pGraphBuilder.";
			forceKill ();
			return false;
		}

		//Reset the volume.
		pBasicAudio->put_Volume (maximumVolume);
		return true;
	}
	return false;
}



void MusicPlayer::forceKill () {
	_MESSAGE ("MusicPlayer >> Force killed!");
	initialized = false;	//Uninitialize the class. This will make sure no functions can execute.
	if (state != PlayerState::psStopped) {	
		if (pMediaControl) {
			pMediaControl->Stop ();
		}
		state = PlayerState::psStopped;
		task = PlayerTask::ptNone;
	}
}