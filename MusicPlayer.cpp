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
	if (!initialized) {
		HRESULT hr;

		//Get an instance of the graph builder.
		hr = CoCreateInstance (CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pGraphBuilder);

		//Check to see that the GraphBuilder is available.
		if (!pGraphBuilder) {
			//There was a problem.  Fail.
			switch (hr) {
				case REGDB_E_CLASSNOTREG:
					lastError = "MusicPlayer >> Failed to create an instance of IGraphBuilder.  Class was not registered.";
					break;
				case CLASS_E_NOAGGREGATION:
					lastError = "MusicPlayer >> Failed to create an instance of IGraphBuilder.  This class cannot be created as part of an aggregate.";
					break;
				case E_NOINTERFACE:
					lastError = "MusicPlayer >> Failed to create an instance of IGraphBuilder.  There was a problem with the interface.";
					break;
				case S_OK:
					lastError = "MusicPlayer >> An instance of the interface was, supposedly, created...  But its null, which means it wasn't.";
					break;
				default:
					lastError = "MusicPlayer >> An unexpected error was thrown during the creation of the interface.  The pointer, for some reason, is just null.  Error Code:  " + to_string (hr);
					break;
			}
			return false;
		}

		//Now, get the other instances we require from the GraphBuilder.
		hr = pGraphBuilder->QueryInterface (IID_IMediaControl, (void **)&pMediaControl);
		hr = pGraphBuilder->QueryInterface (IID_IMediaSeeking, (void**)&pMediaSeeking);
		hr = pGraphBuilder->QueryInterface (IID_IMediaEventEx, (void**)&pMediaEventEx);
		hr = pGraphBuilder->QueryInterface (IID_IBasicAudio, (void**)&pBasicAudio);

		//Check they were all created okay.
		if (!pMediaControl || !pMediaSeeking || !pMediaEventEx || !pBasicAudio) {
			lastError = "MusicPlayer >> Failed to obtain interfaces from pGraphBuilder.";
			return false;
		}

		//Make all other preperations here.
		playerData.state = PlayerState::psStopped;
		playerData.task = PlayerTask::ptNone;
		playerData.switching = false;
		playerData.fadeMethod = FadeMethod::fmUndefined;
		playerData.playedOnce = false;
		defaultFadeLength = 1000;
		currentFadeInLength = defaultFadeLength;
		currentFadeOutLength = defaultFadeLength;
		minPauseTime = 0;
		extraPauseTime = 0;
		minBattleTime = 1000;
		extraBattleTime = 0;
		minAfterBattleTime = 0;
		extraAfterBattleTime = 0;
		maxMusicRestoreTime = 20000.0;
		playerData.fadeInPeriod = defaultFadeLength;
		playerData.fadeOutPeriod = defaultFadeLength;
		playerData.currentTrackDuration = 0;
		playerData.currentTrackPosition = 0;
		playerData.fadeTotalSleepTime = 0;
		maximumVolume = 0;		//0 is actually full blast...  -10000 is infintessimally quiet.
		forceKilled = false;
		trackQueuedUp = false;

		//lastError is set so there won't be any access to uninitialized variables.
		lastError = "MusicPlayer >> Feelin' fine.";
		initialized = true;
		return true;
	} else {
		lastError = "MusicPlayer >> Warning: Initialize was called while the class was allready initialized.";
		return false;
	}
}



bool MusicPlayer::queueTrack (const char *Track) {
	return queueTrack (Track, 0);
}



bool MusicPlayer::queueTrack (const char *Track, LONGLONG Position) {
	if (initialized && !playerData.switching) {
		//Queue up the track.  Since I don't want to be using the reference
		//provided (as it may become invalid at a later time) I will create a
		//copy of it.
		if (strnlen_s (Track, MAX_PATH) > 0) {
			if (true) {
				double pos = (Position / ((double)10000000L));
				_MESSAGE ("MusicPlayer >> Queue new track >> %s (position: %.2f)", Track, pos);
				strcpy_s (queuedTrack, MAX_PATH, Track);
				queuedTrackPosition = Position;
				trackQueuedUp = true;
				return true;
			} else {
				_MESSAGE ("MusicPlayer >> Track can't be queued. Doesn't exists >> %s");
			}
		}
		_MESSAGE ("MusicPlayer >> No track available");
		return false;
	}
	lastError = "MusicPlayer >> Either not initialized or in the middle of changing tracks.";
	return false;
}



bool MusicPlayer::playQueuedTrack (FadeMethod fadeMethod) {
	return playQueuedTrack (fadeMethod, currentFadeOutLength, currentFadeInLength);
}



bool MusicPlayer::playQueuedTrack (FadeMethod fadeMethod, float FadeLength) {
	return playQueuedTrack (fadeMethod, FadeLength, FadeLength);
}



bool MusicPlayer::playQueuedTrack (FadeMethod fadeMethod, float FadeOutLength, float FadeInLength) {
	//If the player is initialized, a track is queued up, and the player
	//isn't allready switching tracks...
	if (initialized && !playerData.switching) {
		WCHAR wFileName[MAX_PATH];
		HRESULT hr;

		//...then we can play the new track!
		//So, this is pretty complicated.  First, is a file is allready
		//playing, we need to begin stopping it.  Depending on the fade
		//method, this could be instant, or it could not be.  If it isn't
		//instant, the DoPlayerStuff() function will carry out the stopping
		//and begin the playback.  Otherwise, we can do it here.
		//If the player isn't playing (or is paused) OR we won't have to fade out a currently playing track...
		_MESSAGE ("MusicPlayer >> Play track >> \"%s\" (fade Out/In: %f/%f)", queuedTrack, FadeOutLength, FadeInLength);
		if ((playerData.state == PlayerState::psStopped || playerData.state == PlayerState::psPaused) || (fadeMethod != FadeMethod::fmFadeOut && fadeMethod != FadeMethod::fmFadeOutThenIn)) {
			//...we can begin playing without issue.
			//Reset the player's interfaces.
			ResetDShow ();
			//Convert the filepath to wide characters.
			size_t length = 0;
			mbstowcs_s (&length, wFileName, queuedTrack, MAX_PATH);
			//Create the filter graph.
			hr = pGraphBuilder->RenderFile (wFileName, NULL);
			if (hr != S_OK) {
				lastError = "MusicPlayer >> Failed to create the DShow Graph to render the file.";
				playerData.state = PlayerState::psStopped;
				playerData.task = PlayerTask::ptNone;
				playerData.switching = false;
				return false;
			}
			//Get the duration of the current track.
			pMediaSeeking->GetDuration (&playerData.currentTrackDuration);
			//If the position is greater than the duration...
			if (playerData.currentTrackDuration > queuedTrackPosition) {
				//...then it is obviously a bad value.  Start from the beginning.
				queuedTrackPosition = 0;
			}
			//Set the position of the track.
			pMediaSeeking->SetPositions (&queuedTrackPosition, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);

			if (fadeMethod == FadeMethod::fmFadeIn || fadeMethod == FadeMethod::fmFadeOutThenIn) {
				//With these fade methods, we will need to perform a gradual fade in.
				//The fading is done by the DoPlayerStuff() function.

				//Indicate we have started a long switch.
				playerData.switching = true;
				playerData.fadeMethod = fadeMethod;
				playerData.fadeOutPeriod = FadeOutLength;
				playerData.fadeInPeriod = FadeInLength;

				//Set the volume to -10000 (silent).
				pBasicAudio->put_Volume (EFFECTIVE_MINIMUM_VOLUME);

				//Change the player's state.
				playerData.state = PlayerState::psFadingIn;
				playerData.task = PlayerTask::ptQueue;
			} else {
				//If we don't need to fade in or anything, then we can just set playing.
				playerData.state = PlayerState::psPlaying;
				playerData.task = PlayerTask::ptNone;
				playerData.switching = false;
				pBasicAudio->put_Volume (maximumVolume);
			}
			trackQueuedUp = false;
			strcpy_s (playingTrack, MAX_PATH, queuedTrack);
			playerData.playedOnce = true;
			pMediaControl->Run ();
		} else {
			//We must fade out the currently playing track, then begin playing.
			//Indicate we are switching.
			playerData.switching = true;
			playerData.fadeMethod = fadeMethod;
			playerData.fadeOutPeriod = FadeOutLength;
			playerData.fadeInPeriod = FadeInLength;

			//Change the player's state.
			playerData.state = PlayerState::psFadingOut;
			playerData.task = PlayerTask::ptQueue;
			//This is technically all we need to do.  The DoPlayerStuff()
			//function has the ball now.  Will it run with it?  We can't
			//actually know, you see, since we can't call RenderFile right
			//now, we have no idea if it will succeed or not.  So, all we
			//can do is return true now.
			//
			//TODO:  Add checks to ensure the file will play successfully
			//when it is rendered.
		}
		return true;
	}
	lastError = "MusicPlayer >> Either not initialized, no track is queued, or we're allready changing tracks.";
	return false;
}



bool MusicPlayer::isInitialized () {
	return initialized;
}



bool MusicPlayer::isPlaying () {
	return initialized && playerData.state != PlayerState::psStopped;
}



bool MusicPlayer::isPaused () {
	return initialized && playerData.state == PlayerState::psPaused;
}



bool MusicPlayer::isQueuedUp () {
	return initialized && trackQueuedUp;
}



bool MusicPlayer::isQueuable () {
	//We can only accept queues if the player is in one of these 3 states.
	return (playerData.state == PlayerState::psStopped || playerData.state == PlayerState::psPaused || playerData.state == PlayerState::psPlaying);
}



LONGLONG MusicPlayer::getSongPosition () {
	if (playerData.state != PlayerState::psStopped) {
		return playerData.currentTrackPosition;
	}
	return 0;
}



const char* MusicPlayer::getSongPath () {
	return queuedTrack;
}



const string MusicPlayer::getErrorMessage () {
	return lastError;
}



bool MusicPlayer::stop (int fadeOut) {
	//Only stop if currently playing or paused.
	if (initialized && (playerData.state == PlayerState::psPlaying || playerData.state == PlayerState::psPaused)) {
		if (fadeOut && playerData.state == PlayerState::psPlaying) {
			//We must fade to a stop.
			playerData.switching = true;
			playerData.fadeMethod = FadeMethod::fmFadeOut;
			playerData.state = PlayerState::psFadingOut;
			playerData.task = PlayerTask::ptStop;
			playerData.fadeOutPeriod = fadeOut;
			return true;
		}
		//We can just stop.
		pMediaControl->Stop ();
		playerData.state = PlayerState::psStopped;
		playerData.task = PlayerTask::ptNone;
		playerData.switching = false;
		return true;
	}
	lastError = "MusicPlayer >> Player was either uninitialized, or not in a stoppable state.";
	return false;
}



void MusicPlayer::pause (int fadeOut) {
	if (initialized && playerData.state == PlayerState::psPlaying) {
		if (fadeOut > 0 && playerData.state == PlayerState::psPlaying) {
			//We must fade to a pause.
			playerData.switching = true;
			playerData.fadeMethod = FadeMethod::fmFadeOut;
			playerData.state = PlayerState::psFadingOut;
			playerData.task = PlayerTask::ptPause;
			playerData.fadeOutPeriod = fadeOut;
			return;
		}
		//We can just pause.
		pMediaControl->Pause ();
		playerData.state = PlayerState::psPaused;
		playerData.task = PlayerTask::ptNone;
		playerData.switching = false;
	}
}



void MusicPlayer::resume (int fadeIn) {
	if (initialized && playerData.state == PlayerState::psPaused) {
		if (fadeIn > 0) {
			//We must fade back in.
			playerData.switching = true;
			playerData.fadeMethod = FadeMethod::fmFadeIn;
			playerData.state = PlayerState::psFadingIn;
			playerData.task = PlayerTask::ptResume;
			playerData.fadeInPeriod = fadeIn;
			pBasicAudio->put_Volume (EFFECTIVE_MINIMUM_VOLUME);
			pMediaControl->Run ();
			return;
		}
		//We can just resume.
		pMediaControl->Run ();
		pBasicAudio->put_Volume (maximumVolume);
		playerData.state = PlayerState::psPlaying;
		playerData.task = PlayerTask::ptNone;
		playerData.switching = false;
	}
}



bool MusicPlayer::restart (int fadeOut, int fadeIn) {
	//Only stop if currently playing, paused, or stopped.
	if (initialized && (playerData.state == PlayerState::psPlaying || playerData.state == PlayerState::psPaused || playerData.state == PlayerState::psStopped)) {
		if (playerData.state == PlayerState::psStopped && !playerData.playedOnce) {
			//It is unsafe to resume the last played file.
			lastError = "MusicPlayer >> The player has not played anything yet.  Can not restart if there is no media to play.";
			return false;
		}

		if (fadeOut > 0 && playerData.state == PlayerState::psPlaying) {
			//We must fade to a stop.
			playerData.switching = true;
			playerData.task = PlayerTask::ptRestart;
			playerData.state = PlayerState::psFadingOut;
			playerData.fadeMethod = FadeMethod::fmFadeOut;
			playerData.fadeOutPeriod = fadeOut;
			playerData.fadeInPeriod = fadeIn;
			return true;
		}

		//We can just resume.  Reset the position.
		if (pMediaSeeking) {
			playerData.currentTrackPosition = 0;
			pMediaSeeking->SetPositions (&playerData.currentTrackPosition, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
		}
		pMediaControl->Run ();
		pBasicAudio->put_Volume (maximumVolume);
		playerData.state = PlayerState::psFadingIn;
		playerData.task = PlayerTask::ptNone;
		playerData.switching = true;
		return true;
	}
	lastError = "MusicPlayer >> Player was either uninitialized or in an unrestartable state.";
	return false;
}



bool MusicPlayer::setPosition (int fadeOut, int fadeIn, LONGLONG position) {
	//Only stop if currently playing, paused, or stopped.
	if (initialized && (playerData.state == PlayerState::psPlaying || playerData.state == PlayerState::psPaused || playerData.state == PlayerState::psStopped)) {
		if (playerData.state == PlayerState::psStopped && !playerData.playedOnce) {
			//It is unsafe to resume the last played file.
			lastError = "MusicPlayer >> The player has not played anything yet.  Can not restart if there is no media to play.";
			return false;
		}

		playerData.fadeOutPeriod = fadeOut;
		playerData.fadeInPeriod = fadeIn;
		if (fadeOut > 0 && playerData.state == PlayerState::psPlaying) {
			//We must fade to a stop.
			playerData.switching = true;
			playerData.fadeMethod = FadeMethod::fmFadeOut;
			playerData.state = PlayerState::psFadingOut;
			playerData.task = PlayerTask::ptPosition;
			playerData.newTrackPosition = position;
			return true;
		}
		//We can just resume.  Reset the position.
		if (pMediaSeeking) {
			pMediaSeeking->SetPositions (&position, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
		}
		pMediaControl->Run ();
		pBasicAudio->put_Volume (maximumVolume);
		playerData.state = PlayerState::psPlaying;
		playerData.task = PlayerTask::ptNone;
		return true;
	}
	lastError = "MusicPlayer >> Player was either uninitialized or in an unrestartable state.";
	return false;
}



void MusicPlayer::doPlayerStuff (int sleepTime) {
	if (initialized) {
		//This is the main function of the player.  Every iteration of
		//Setry Thread that this player is initialized, this function
		//should be called.  The player is, as most things, a simple
		//state machine.  Every time, it will check to see if the state
		//requires a task to be performed.  If so, it will perform it.
		//Otherwise, this function does alot of nothing.

		//So, a quick rundown:
		//1)  Checks playerData.state and performs actions as neccessary.
		//2)  Checks for events from the player, mostly if the player has stopped.

		//However, first, it will check to see if the play-back is stopped.
		PlayerState currentState = doDShowEvents ();
		if (currentState != playerData.state) {
			//Clean up as needed.  If we, by chance, performed a ForceKill...
			if (currentState == PlayerState::psUninitialized) {
				//Return, since there is nothing more to do.
				return;
			}
			//Now, if the player stopped since the last time we did player stuff.
			else if (currentState == PlayerState::psStopped) {
				//Well, then lets stop.
				//First, lets see if we were fading in any direction.
				switch (playerData.state) {
					case PlayerState::psFadingIn:
						//Set volume to maximum as we no longer have to worry about fading.
						pBasicAudio->put_Volume (maximumVolume);
						playerData.fadeTotalSleepTime = 0;			//Reset the sleep time for the next fade.
						playerData.task = PlayerTask::ptNone;
						playerData.state = currentState;
						playerData.switching = false;
						_MESSAGE ("MusicPlayer >> Switch state: FadingIn -> Stopped. task: Any -> None");
						break;
					case PlayerState::psFadingOut:
						//If we weren't working to pause the track...
						if (playerData.task != PlayerTask::ptPause) {
							//Set volume to minimum as we no longer have to worry about fading.
							pBasicAudio->put_Volume (ABSOLUTE_MINIMUM_VOLUME);
							//Since we were in the middle of fading when the track stopped,
							//there was probably something we needed to do.  Lets set the
							//state to Faded so that we can handle it later.
							playerData.state = PlayerState::psFadedOut;
							playerData.fadeTotalSleepTime = 0;			//Reset the sleep time for the next fade.
							_MESSAGE ("MusicPlayer >> Switch state: FadingOut -> FadedOut. task: !Pause -> <MAINTAIN>");
						} else {
							//Otherwise, there is nothing to pause anymore.
							//Set to stopped instead.
							playerData.task = PlayerTask::ptNone;	//No longer any task.  Its stopped.
							playerData.state = currentState;			//Set to Stopped.
							playerData.fadeTotalSleepTime = 0;		//Reset the sleep time for the next fade.
							playerData.switching = false;
							_MESSAGE ("MusicPlayer >> Switch state: FadingOut -> Stopped. task: Pause -> None");
						}
						break;
					case PlayerState::psFadedOut:
						//There is one thing we should look out for in this case.
						//If we faded and wanted to pause...  Well, there is
						//nothing to pause anymore.  Lets handle it so it is
						//stopped instead.
						if (playerData.task == PlayerTask::ptPause) {
							playerData.task = PlayerTask::ptNone;	//No longer any task.  Its stopped.
							playerData.state = currentState;			//Set to Stopped.
							playerData.switching = false;
							_MESSAGE ("MusicPlayer >> Switch status: FadedOut -> Stopped. task: Pause -> None");
						}
						break;
					default:
						//Nothing else, we'll set the state accordingly.
						playerData.task = PlayerTask::ptNone;
						playerData.state = currentState;
						playerData.switching = false;
						_MESSAGE ("MusicPlayer >> Switch status: Any -> Stopped. task: Any -> None");
						break;
				}
			}
		}
		//Now do typical player things.
		LONGLONG stopped;		//For the pMediaSeeking->GetPositions function.
		switch (playerData.state) {
			case PlayerState::psPlaying:
				//Keep track of the track's position.
				pMediaSeeking->GetPositions (&playerData.currentTrackPosition, &stopped);
				break;
			case PlayerState::psStopped:
				playerData.switching = false;
				break;
			case PlayerState::psFadingIn:
			case PlayerState::psFadingOut:
				//Keep track of the tracks position.
				pMediaSeeking->GetPositions (&playerData.currentTrackPosition, &stopped);
				//Do fading stuff.
				doFading (sleepTime);
				break;
			case PlayerState::psFadedIn:
			case PlayerState::psFadedOut:
				//Keep track of the track's position.
				pMediaSeeking->GetPositions (&playerData.currentTrackPosition, &stopped);
				//Now continue the task.
				doTaskStuff ();
				break;
		}
	}
}



LONGLONG MusicPlayer::getTrackDuration () {
	return playerData.currentTrackDuration;
}



float MusicPlayer::getDefaultFadeLength () {
	return defaultFadeLength;
}



float MusicPlayer::getCurrentFadeInLength () {
	return currentFadeInLength;
}



bool MusicPlayer::setCurrentFadeInLength (float length) {
	if (currentFadeInLength != length) {
		currentFadeInLength = length;
		return true;
	}
	return false;
}



float MusicPlayer::getCurrentFadeOutLength () {
	return currentFadeOutLength;
}



bool MusicPlayer::setCurrentFadeOutLength (float length) {
	if (currentFadeOutLength != length) {
		currentFadeOutLength = length;
		return true;
	}
	return false;
}



float MusicPlayer::getMinPauseTime () {
	return minPauseTime;
}



bool MusicPlayer::setMinPauseTime (float length) {
	if (minPauseTime != length) {
		minPauseTime = length;
		return true;
	}
	return false;
}



float MusicPlayer::getExtraPauseTime () {
	return extraPauseTime;
}



bool MusicPlayer::setExtraPauseTime (float length) {
	if (extraPauseTime != length) {
		extraPauseTime = length;
		return true;
	}
	return false;
}



float MusicPlayer::getFinalPauseTime (bool update) {
	if (update) {
		effectivePauseTime = minPauseTime + (extraPauseTime * rand () / RAND_MAX);
	}
	return effectivePauseTime;
}



float MusicPlayer::getMinBattleDelay () {
	return minBattleTime;
}



bool MusicPlayer::setMinBattleDelay (float length) {
	if (minBattleTime != length) {
		minBattleTime = length;
		return true;
	}
	return false;
}



float MusicPlayer::getExtraBattleDelay () {
	return extraBattleTime;
}



bool MusicPlayer::setExtraBattleDelay (float length) {
	if (extraBattleTime != length) {
		extraBattleTime = length;
		return true;
	}
	return false;
}



float MusicPlayer::getFinalBattleDelay (bool update) {
	if (update) {
		effectiveBattleTime = minBattleTime + (extraBattleTime * rand () / RAND_MAX);
	}
	return effectiveBattleTime;
}



float MusicPlayer::getMinAfterBattleDelay () {
	return minAfterBattleTime;
}



bool MusicPlayer::setMinAfterBattleDelay (float length) {
	if (minAfterBattleTime != length) {
		minAfterBattleTime = length;
		return true;
	}
	return false;
}



float MusicPlayer::getExtraAfterBattleDelay () {
	return extraAfterBattleTime;
}



bool MusicPlayer::setExtraAfterBattleDelay (float length) {
	if (extraAfterBattleTime != length) {
		extraAfterBattleTime = length;
		return true;
	}
	return false;
}



float MusicPlayer::getFinalAfterBattleDelay (bool update) {
	if (update) {
		effectiveAfterBattleTime = minAfterBattleTime + (extraAfterBattleTime * rand () / RAND_MAX);
	}
	return effectiveAfterBattleTime;
}



float MusicPlayer::getMaxRestoreTime () {
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
	if (initialized && lVolume != maximumVolume) {
		if (lVolume >= ABSOLUTE_MINIMUM_VOLUME && lVolume <= MAXIMUM_VOLUME) {
			maximumVolume = lVolume;
			//If we allready aren't adjusting the volume or the player's volume isn't at a critical point...
			//if(playerData.state != PlayerState::psFadingIn && playerData.state != PlayerState::psFadingOut && playerData.state != PlayerState::psFadedOut && playerData.state != PlayerState::psFadedIn)
			if (playerData.state == PlayerState::psPlaying || playerData.state == PlayerState::psStopped || playerData.state == PlayerState::psFadedIn || playerData.state == PlayerState::psPaused) {
				//...set the volume.
				pBasicAudio->put_Volume (lVolume);
			}
			return true;
		}
		lastError = "MusicPlayer >> The provided volume was out of range (-10000 to 0).";
	}
	return false;
}



double MusicPlayer::getMusicSpeed () {
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



void MusicPlayer::forceKill () {
	_MESSAGE ("MusicPlayer >> Force killed!");
	if (playerData.state != PlayerState::psStopped) {
		pMediaControl->Stop ();
		playerData.state = PlayerState::psStopped;
		playerData.task = PlayerTask::ptNone;
		playerData.switching = false;
	}
	//Uninitialize the class.
	initialized = false;	//This will make it so no functions can execute.
	playerData.state = PlayerState::psUninitialized;
	playerData.task = PlayerTask::ptNone;
}



PlayerState MusicPlayer::doDShowEvents () {
	HRESULT hr = S_OK;
	long evCode, param1, param2;
	PlayerState retVal = playerData.state;

	if (!pMediaEventEx) {
		lastError = "MusicPlayer >> pMediaEventEX was undefined.  ForceKill()ing to prevent disaster.";
		forceKill ();
		return PlayerState::psUninitialized;
	}

	while (SUCCEEDED (pMediaEventEx->GetEvent (&evCode, &param1, &param2, 0))) {
		switch (evCode) {
			// We process only the EC_COMPLETE message which is sent when the media is finished playing.
			case EC_COMPLETE:
				//Set state to Stopped when finished playing.
				retVal = PlayerState::psStopped;
				break;
		}
		hr = pMediaEventEx->FreeEventParams (evCode, param1, param2);
	}
	return retVal;
}



void MusicPlayer::doFading (int sleepTime) {
	playerData.fadeTotalSleepTime += sleepTime;
	float percentComplete;
	float linearVolume = (maximumVolume != ABSOLUTE_MINIMUM_VOLUME) ? pow (10.0, 0.0005 * maximumVolume) : 0.0;
	HRESULT hr;

	//Only two possible states.  FadingIn or FadingOut
	if (playerData.state == PlayerState::psFadingIn) {
		//FadingIn
		//long DesiredVolume = EFFECTIVE_MINIMUM_VOLUME + ((maximumVolume - EFFECTIVE_MINIMUM_VOLUME) * (float(playerData.FadeTotalSleepTime) / float(playerData.fadeInPeriod)));
		percentComplete = (playerData.fadeInPeriod ? float (playerData.fadeTotalSleepTime) / float (playerData.fadeInPeriod) : 1.0);
		long DesiredVolume = (percentComplete == 0.0 || linearVolume == 0.0) ? ABSOLUTE_MINIMUM_VOLUME : (2000 * log10 (percentComplete * linearVolume));
		if (percentComplete >= 1.0) {
			//Set the volume to maximumVolume
			hr = pBasicAudio->put_Volume (maximumVolume);
			//Exit from fading.
			playerData.state = PlayerState::psFadedIn;
			playerData.fadeTotalSleepTime = 0;
			_MESSAGE ("MusicPlayer >> Switch status: FadingIn -> FadedIn. task: Unchanged.");
		} else {
			hr = pBasicAudio->put_Volume (DesiredVolume);
		}
	} else {
		//FadingOut
		//long DesiredVolume = maximumVolume + ((EFFECTIVE_MINIMUM_VOLUME - maximumVolume) * (float(playerData.FadeTotalSleepTime) / float(playerData.fadeOutPeriod)));
		percentComplete = (playerData.fadeOutPeriod ? float (playerData.fadeTotalSleepTime) / float (playerData.fadeOutPeriod) : 1.0);
		long DesiredVolume = (percentComplete == 1.0 || linearVolume == 0.0) ? ABSOLUTE_MINIMUM_VOLUME : (2000 * log10 ((1 - percentComplete) * linearVolume));
		if (percentComplete >= 1.0) {
			//Set the volume to ABSOLUTE_MINIMUM_VOLUME
			hr = pBasicAudio->put_Volume (ABSOLUTE_MINIMUM_VOLUME);
			//Exit from fading.
			playerData.state = PlayerState::psFadedOut;
			playerData.fadeTotalSleepTime = 0;
			_MESSAGE ("MusicPlayer >> Switch status: FadingOut -> FadedOut. task: Unchanged");
		} else {
			hr = pBasicAudio->put_Volume (DesiredVolume);
		}
	}
	if (hr == E_INVALIDARG) {
		lastError = "MusicPlayer >> DesiredVolume was outside of the range -10000 to 0.";
	} else if (hr == E_FAIL) {
		lastError = "MusicPlayer >> Failed to alter the volume.  Underlying device suffered an error.";
	} else if (hr == E_NOTIMPL) {
		lastError = "MusicPlayer >> Filter graph does not support volume manipulations via this interface.";
	}
}



void MusicPlayer::doTaskStuff () {
	if (playerData.state == PlayerState::psFadedIn) {
		//Applies to Queue, Resume and Restart.
		//Its all pretty simple.  Both these simply set the state to playing.
		playerData.state = PlayerState::psPlaying;
		playerData.task = PlayerTask::ptNone;
		playerData.switching = false;
		_MESSAGE ("MusicPlayer >> Switch status: FadingIn -> Playing. task: Any -> None");
	} else if (playerData.state == PlayerState::psFadedOut) {
		//Applies to Queue, Pause, Restart, and Stop.
		size_t length = 0;
		switch (playerData.task) {
			case PlayerTask::ptQueue:
				ResetDShow ();
				//Now that we faded out, its time to begin the queued track.
				WCHAR wFileName[MAX_PATH];
				HRESULT hr;
				//Convert the filepath to wide characters.
				mbstowcs_s (&length, wFileName, queuedTrack, MAX_PATH);
				//Create the filter graph.
				hr = pGraphBuilder->RenderFile (wFileName, NULL);
				if (hr != S_OK) {
					lastError = "MusicPlayer >> Failed to create the DShow Graph to render the file.";
					//In the case of an error, just set the player to Stopped.
					playerData.state = PlayerState::psStopped;
					playerData.task = PlayerTask::ptNone;
					playerData.switching = false;
					break;
				}
				//Get the duration of the current track.
				pMediaSeeking->GetDuration (&playerData.currentTrackDuration);
				//Set the position of the track.
				pMediaSeeking->SetPositions (&queuedTrackPosition, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);

				if (playerData.fadeMethod == FadeMethod::fmFadeIn || playerData.fadeMethod == FadeMethod::fmFadeOutThenIn) {
					//With these fade methods, we will need to perform a gradual fade in.
					//The fading is done by the DoPlayerStuff() function.

					//Set the volume to -10000 (silent).
					//This is just in case...
					pBasicAudio->put_Volume (EFFECTIVE_MINIMUM_VOLUME);

					//Change the player's state.
					playerData.state = PlayerState::psFadingIn;
					_MESSAGE ("MusicPlayer >> Switch status: FadedOut -> FadingIn. task: Queue");
				} else {
					//If we don't need to fade in or anything, then we can just set playing.
					pBasicAudio->put_Volume (maximumVolume);
					playerData.state = PlayerState::psPlaying;
					playerData.task = PlayerTask::ptNone;
					playerData.switching = false;
					_MESSAGE ("MusicPlayer >> Switch status: FadedOut -> Playing. task: Queue -> None");
				}

				trackQueuedUp = false;
				strcpy_s (playingTrack, MAX_PATH, queuedTrack);
				playerData.playedOnce = true;
				pMediaControl->Run ();
				break;
			case PlayerTask::ptPause:
				//Now that we have faded out, we can pause the track.
				pMediaControl->Pause ();
				playerData.state = PlayerState::psPaused;
				playerData.task = PlayerTask::ptNone;
				playerData.switching = false;
				_MESSAGE ("MusicPlayer >> Switch status: FadedOut -> Paused. task: Pause -> None");
				break;
			case PlayerTask::ptRestart:
				//Now we can restart the track.
				//This requires stopping the playback, resetting the position, and running it again.
				playerData.currentTrackPosition = 0;
				pMediaControl->Stop ();
				pMediaSeeking->SetPositions (&playerData.currentTrackPosition, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
				pMediaControl->Run ();
				playerData.state = PlayerState::psFadingIn;
				playerData.task = PlayerTask::ptNone;
				_MESSAGE ("MusicPlayer >> Switch status: FadedOut -> FadingIn. task: Restart -> None");
				break;
			case PlayerTask::ptStop:
				pMediaControl->Stop ();
				playerData.state = PlayerState::psStopped;
				playerData.task = PlayerTask::ptNone;
				playerData.switching = false;
				_MESSAGE ("MusicPlayer >> Switch status: FadedOut -> Stopped. task: Stopped -> None");
				break;
			case PlayerTask::ptPosition:
				//Now we can change the track position.
				//This requires stopping the playback, setting the position, and running it again.
				pMediaControl->Stop ();
				pMediaSeeking->SetPositions (&playerData.newTrackPosition, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
				pMediaControl->Run ();
				playerData.newTrackPosition = 0;
				playerData.state = PlayerState::psFadingIn;
				playerData.task = PlayerTask::ptNone;
				_MESSAGE ("MusicPlayer >> Switch status: FadedOut -> FadingIn. task: Position -> None");
				break;
		}
	}
}



//Resets DirectSHow for whenever we want to play a new file, basically.
void MusicPlayer::ResetDShow () {
	//If we have allready used these DShow interfaces...
	if (playerData.playedOnce) {
		//...we should reset them before using them again.
		//Stop playback.
		pMediaControl->Stop ();

		//Reinitialize the DShow interfaces.
		HRESULT hr;

		//Get an instance of the graph builder.
		CoCreateInstance (CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pGraphBuilder);

		//Check to see that the GraphBuilder is available.
		if (!pGraphBuilder) {
			//There was a problem.  Fail.
			lastError = "MusicPlayer >> Failed to create an instance of IGraphBuilder.";
			forceKill ();
			return;
		}

		//Now, get the other instances we require from the GraphBuilder.
		hr = pGraphBuilder->QueryInterface (IID_IMediaControl, (void **)&pMediaControl);
		hr = pGraphBuilder->QueryInterface (IID_IMediaSeeking, (void**)&pMediaSeeking);
		hr = pGraphBuilder->QueryInterface (IID_IMediaEventEx, (void**)&pMediaEventEx);
		hr = pGraphBuilder->QueryInterface (IID_IBasicAudio, (void**)&pBasicAudio);

		//Check they were all created okay.
		if (!pMediaControl || !pMediaSeeking || !pMediaEventEx || !pBasicAudio) {
			lastError = "MusicPlayer >> Failed to obtain interfaces from pGraphBuilder.";
			forceKill ();
			return;
		}

		//Reset the volume.
		pBasicAudio->put_Volume (maximumVolume);
	}
}