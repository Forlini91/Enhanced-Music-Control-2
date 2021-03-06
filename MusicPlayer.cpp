#include "MusicPlayer.h"

#include "Globals.h"
#include "FilePath.h"
#include <dshow.h>
#include "IniData.h"
#include "DebugMode.h"

#define VOLUME_MULT 2000.0
const float minVolume = pow (2, ABSOLUTE_MINIMUM_VOLUME / VOLUME_MULT);

using namespace std;



MusicPlayer musicPlayer;
HANDLE hMusicPlayerMutex = CreateMutex (nullptr, FALSE, nullptr);



//These contain pointers to our DShow interfaces.
//GraphBuilder lets us generate graphs which are used to render the sound.
IGraphBuilder *pGraphBuilder = nullptr;

//MediaControl allows us to control the playback of the graph.
IMediaControl *pMediaControl = nullptr;

//MediaSeeking allows us to obtain position and playback information.
IMediaSeeking *pMediaSeeking = nullptr;

//MediaEventEx allows us to handle events generated by the DShow interfaces.
IMediaEventEx *pMediaEventEx = nullptr;

//BasicAudio allows us to affect the balance and amplitude of the playback.
//In other words, its the volume control.
IBasicAudio *pBasicAudio = nullptr;



bool QueuedTrack::get (string *name, LONGLONG *position) {
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



bool MusicPlayer::initialize () {
	if (isInitialized()) {		//Don't initialize again if already initialized.
		return true;
	} else if (initializeDShow (false)) {
		_MESSAGE ("Music player >> Initialized");
		initialized = true;
		return true;
	} else {
		return false;
	}
}



bool MusicPlayer::isInitialized () const {
	return initialized;
}


bool MusicPlayer::hasPlayedOnce () const {
	return playedOnce;
}



bool MusicPlayer::isStopped () const {
	return state == PlayerState::psStopped;
}


bool MusicPlayer::isPlaying () const {
	return state == PlayerState::psPlaying;
}


bool MusicPlayer::isPaused () const {
	return state == PlayerState::psPaused;
}



bool MusicPlayer::isReady () const {
	switch (state) {
		case PlayerState::psStopped:
		case PlayerState::psPlaying:
		case PlayerState::psPaused:
			return true;
		default:
			return false;
	}
}






bool MusicPlayer::queueTrack (const string &trackPath, LONGLONG position) {
	if (isInitialized() && isReady()) {
		if (trackPath.empty ()) {
			_MESSAGE ("%lld | MusicPlayer >> No track available", timeStamp);
		} else if (!exists (trackPath)) {
			_MESSAGE ("%lld | MusicPlayer >> Track can't be queued: it doesn't exists >> %s", timeStamp, trackPath.c_str ());
		} else if (isDirectory (trackPath)) {
			_MESSAGE ("%lld | MusicPlayer >> Track can't be queued: it's a directory >> %s", timeStamp, trackPath.c_str ());
		} else if (!isExtensionSupported (trackPath)) {
			_MESSAGE ("%lld | MusicPlayer >> Track can't be queued: unsupported extension >> %s", timeStamp, trackPath.c_str ());
		} else {
			_MESSAGE ("%lld | MusicPlayer >> Queue new track >> %s (position: %.2f)", timeStamp, trackPath.c_str (), position/ONE_SECOND);
			queuedTrack = QueuedTrack (trackPath, position);
			return true;
		}
	}
	return false;
}



bool MusicPlayer::playQueuedTrack (FadeMethod newFadeMethod, int fadeOut, int fadeIn) {
	//If the player is initialized, a track is queued up, and the player isn't already switching tracks...
	if (isInitialized() && isReady() && queuedTrack.isQueued()) {
		//First, is a file is already playing, we need to begin stopping it.
		//Depending on the fade method, this could be instant, or it could not be.
		//If it isn't instant, the DoPlayerStuff() function will carry out the stopping and begin the playback. Otherwise, we can do it here.
		
		//If the player isn't playing (or is paused) OR we won't have to fade out a currently playing track...
		if (isStopped () || isPaused () || newFadeMethod == FadeMethod::fmNoFade || newFadeMethod == FadeMethod::fmFadeIn) {
			return playTrack (newFadeMethod, fadeOut, fadeIn);
		} else {	//Stop current track
			_MESSAGE ("%lld | MusicPlayer >> Delayed play track >> Fade method: %d, FadeOut/In: %d/%d)", timeStamp, newFadeMethod, fadeOut, fadeIn);
			state = PlayerState::psFadingOut;
			task = PlayerTask::ptQueue;
			fadeMethod = newFadeMethod;
			fadeOutPeriod = fadeOut;
			fadeInPeriod = fadeIn;
			return true;
		}
	}
	return false;
}





const string& MusicPlayer::getTrack () const {
	return trackPath;
}



LONGLONG MusicPlayer::getTrackPosition () const {
	return hasPlayedOnce() ? trackPosition : 0;
}



bool MusicPlayer::setTrackPosition (LONGLONG position, int fadeOut, int fadeIn) {
	//Only stop if currently playing, paused, or stopped.
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
			state = PlayerState::psFadingIn;
			task = PlayerTask::ptNone;
			pMediaSeeking->SetPositions (&position, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning);
			pMediaControl->Run ();
			pBasicAudio->put_Volume (maximumVolume);
		}
		return true;
	}
	return false;
}



LONGLONG MusicPlayer::getTrackDuration () const {
	return hasPlayedOnce() ? trackDuration : 0;
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
	if (isInitialized() && SUCCEEDED (pMediaSeeking->GetRate (&speed))) {
		return speed;
	}
	return -1;
}



bool MusicPlayer::setMusicSpeed (double speed) {
	double prev = 0;
	if (isInitialized ()
			&& SUCCEEDED (pMediaSeeking->GetRate (&prev))
			&& prev != speed
			&& SUCCEEDED (pMediaSeeking->SetRate (speed))) {
		return true;
	}
	return false;
}





bool MusicPlayer::resume (int fadeIn) {
	if (isInitialized () && isPaused ()) {
		task = PlayerTask::ptNone;
		if (fadeIn > 0) {
			//We must fade back in.
			state = PlayerState::psFadingIn;
			fadeMethod = FadeMethod::fmFadeIn;
			fadeInPeriod = fadeIn;
			pBasicAudio->put_Volume (EFFECTIVE_MINIMUM_VOLUME);	//Start at min, then fade in
			pMediaControl->Run ();
		} else {
			//We can just resume.
			state = PlayerState::psPlaying;
			pBasicAudio->put_Volume (maximumVolume);			//Start at max
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
		return true;
	}
	return false;
}



bool MusicPlayer::restart (int fadeOut, int fadeIn) {
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

		if (state == PlayerState::psUninitialized) {
			//Return, since there is nothing we can do
			return;
		}

		//However, first, it will check to see if the play-back is stopped.
		PlayerState currentState = checkState ();
		if (currentState != state) {		//REDUNDANT, but leave it for now...
			//Clean up as needed.  If we, by chance, performed a ForceKill...
			//Now, if the player stopped since the last time we did player stuff.
			if (currentState == PlayerState::psStopped) {
				//Well, then lets stop.
				//First, lets see if we were fading in any direction.
				switch (state) {
					case PlayerState::psFadingOut:
						if (task != PlayerTask::ptPause) {
							//Since we were in the middle of fading when the track stopped, there was probably something we needed to do.
							//Lets set the state to Faded so that we can handle it later.
							state = PlayerState::psFadedOut;
							fadeTotalSleepTime = 0;			//Reset the sleep time for the next fade.
							pBasicAudio->put_Volume (ABSOLUTE_MINIMUM_VOLUME);
							_MESSAGE ("%lld | MusicPlayer >> Switch state: FadingOut -> FadedOut. task: !Pause -> FadedOut", timeStamp);
						} else {
							//Otherwise, there is nothing to pause anymore.
							//Set to stopped instead.
							state = PlayerState::psStopped;			//Set to Stopped.
							task = PlayerTask::ptNone;	//No longer any task.  Its stopped.
							fadeTotalSleepTime = 0;		//Reset the sleep time for the next fade.
							_MESSAGE ("%lld | MusicPlayer >> Switch state: FadingOut -> Stopped. task: Pause -> None", timeStamp);
						}
						break;
					case PlayerState::psFadingIn:
						//Set volume to maximum as we no longer have to worry about fading.
						state = PlayerState::psStopped;
						task = PlayerTask::ptNone;
						fadeTotalSleepTime = 0;			//Reset the sleep time for the next fade.
						pBasicAudio->put_Volume (maximumVolume);
						_MESSAGE ("%lld | MusicPlayer >> Switch state: FadingIn -> Stopped. task: Any -> None", timeStamp);
						break;
					case PlayerState::psFadedOut:
						//There is one thing we should look out for in this case.
						//If we faded and wanted to pause...  Well, there is nothing to pause anymore. 
						//Lets handle it so it is stopped instead.
						if (task == PlayerTask::ptPause) {
							state = PlayerState::psStopped;			//Set to Stopped.
							task = PlayerTask::ptNone;		//No longer any task.  Its stopped.
							_MESSAGE ("%lld | MusicPlayer >> Switch status: FadedOut -> Stopped. task: Pause -> None", timeStamp);
						}
						break;
					default:
						//Nothing else, we'll set the state accordingly.
						state = PlayerState::psStopped;
						task = PlayerTask::ptNone;
						_MESSAGE ("%lld | MusicPlayer >> Switch status: Any -> Stopped. task: Any -> None", timeStamp);
						break;
				}
			}
		}

		//Now do typical player things.
		LONGLONG stopped;		//For the pMediaSeeking->GetPositions function.
		switch (state) {
			case PlayerState::psStopped:
				break;
			case PlayerState::psPlaying:
				//Keep track of the track's position.
				pMediaSeeking->GetPositions (&trackPosition, &stopped);
				break;
			case PlayerState::psFadingOut:
			case PlayerState::psFadingIn:
				//Keep track of the tracks position.
				pMediaSeeking->GetPositions (&trackPosition, &stopped);
				doFading (sleepTime);	//Do fading stuff.
				break;
			case PlayerState::psFadedOut:
			case PlayerState::psFadedIn:
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
			_MESSAGE ("%lld | MusicPlayer >> Switch status: FadingIn -> FadedIn. task: Unchanged", timeStamp);
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
			_MESSAGE ("%lld | MusicPlayer >> Switch status: FadingOut -> FadedOut. task: Unchanged", timeStamp);
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
		case E_INVALIDARG: _MESSAGE ("%lld | MusicPlayer >> DesiredVolume was outside of the range -10000 to 0.", timeStamp); break;
		case E_FAIL: _MESSAGE ("%lld | MusicPlayer >> Failed to alter the volume. Underlying device suffered an error.", timeStamp); break;
		case E_NOTIMPL: _MESSAGE ("%lld | MusicPlayer >> Filter graph does not support volume manipulations via this interface.", timeStamp); break;
	}
	return percentComplete >= 1.0;
}



bool MusicPlayer::doTask () {
	switch (state) {
		case PlayerState::psFadedIn:
			//Applies to Queue, Resume and Restart.
			state = PlayerState::psPlaying;
			task = PlayerTask::ptNone;
			_MESSAGE ("%lld | MusicPlayer >> Switch status: FadingIn -> Playing. task: Any -> None", timeStamp);
			break;
		case PlayerState::psFadedOut:
			//Applies to Queue, Position, Pause, Restart, Stop.
			switch (task) {
				case PlayerTask::ptQueue:
					playTrack (fadeMethod, fadeInPeriod, fadeOutPeriod);
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
					_MESSAGE ("%lld | MusicPlayer >> Switch status: FadedOut -> FadingIn. task: Restart -> None", timeStamp);
					break;
				case PlayerTask::ptPause:
					//Now that we have faded out, we can pause the track.
					pMediaControl->Pause ();
					state = PlayerState::psPaused;
					task = PlayerTask::ptNone;
					_MESSAGE ("%lld | MusicPlayer >> Switch status: FadedOut -> Paused. task: Pause -> None", timeStamp);
					break;
				case PlayerTask::ptStop:
					pMediaControl->Stop ();
					state = PlayerState::psStopped;
					task = PlayerTask::ptNone;
					_MESSAGE ("%lld | MusicPlayer >> Switch status: FadedOut -> Stopped. task: Stopped -> None", timeStamp);
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
					_MESSAGE ("%lld | MusicPlayer >> Switch status: FadedOut -> FadingIn. task: Position -> None", timeStamp);
					break;
			}
	}
	return true;
}



PlayerState MusicPlayer::checkState () {
	PlayerState retVal = state;
	long evCode, param1, param2;
	while (SUCCEEDED (pMediaEventEx->GetEvent (&evCode, &param1, &param2, 0))) {
		// We process only the EC_COMPLETE message which is sent when the media is finished playing.
		switch (evCode) {
			case EC_COMPLETE:
				if (retVal != PlayerState::psStopped) {
					_MESSAGE ("%lld | MusicPlayer >> End of the track", timeStamp);
				}
				retVal = PlayerState::psStopped;	//Set state to Stopped when finished playing.
		}
	}
	return retVal;
}



bool MusicPlayer::playTrack (FadeMethod newFadeMethod, int fadeOut, int fadeIn) {
	//Reset the player's interfaces.
	if (!resetDShow ()) {
		return false;
	}

	string queuedTrackPath;
	LONGLONG queuedTrackPosition;
	if (!queuedTrack.get (&queuedTrackPath, &queuedTrackPosition)) {
		_MESSAGE ("%lld | MusicPlayer >> No queued track", timeStamp);
		return false;
	}

	_MESSAGE ("%lld | MusicPlayer >> Play track >> \"%s\" (fade Out/In: %d/%d)", timeStamp, queuedTrackPath.c_str (), fadeOut, fadeIn);
	size_t length = 0;
	WCHAR wFileName[MAX_PATH];
	mbstowcs_s (&length, wFileName, queuedTrackPath.c_str (), MAX_PATH);	//Convert the filepath to wide characters.
	if (FAILED (pGraphBuilder->RenderFile (wFileName, nullptr))) {	//Create the filter graph.
		_MESSAGE ("%lld | MusicPlayer >> Failed to create the DShow Graph to render the file.", timeStamp);
		state = PlayerState::psStopped;
		task = PlayerTask::ptNone;
		return false;
	}

	trackPath = queuedTrackPath;
	if (FAILED (pMediaSeeking->GetDuration (&trackDuration))) {
		_MESSAGE ("%lld | MusicPlayer >> Failed to get the track duration", timeStamp);
		state = PlayerState::psStopped;
		task = PlayerTask::ptNone;
		return false;
	}
	
	_MESSAGE ("%lld | MusicPlayer >> Get track position: %.2f/%.2f", timeStamp, (queuedTrackPosition / ONE_SECOND), (trackDuration / ONE_SECOND));
	if (queuedTrackPosition <= trackDuration) {
		trackPosition = queuedTrackPosition;
	} else {
		_EMCDEBUG ("%lld | MusicPlayer >> Track position is greater than duration. Start from beginning.", timeStamp);
		trackPosition = 0;
	}
	if (FAILED (pMediaSeeking->SetPositions (&trackPosition, AM_SEEKING_AbsolutePositioning, nullptr, AM_SEEKING_NoPositioning))) {
		if (trackDuration > 0) {	//Don't care if 0
			_MESSAGE ("%lld | MusicPlayer >> Failed to set the track position", timeStamp);
			state = PlayerState::psStopped;
			task = PlayerTask::ptNone;
			return false;
		}
	}

	if (newFadeMethod == FadeMethod::fmFadeIn || newFadeMethod == FadeMethod::fmFadeOutThenIn) {
		state = PlayerState::psFadingIn;
		if (task != PlayerTask::ptQueue) {
			fadeMethod = newFadeMethod;
			fadeOutPeriod = fadeOut;
			fadeInPeriod = fadeIn;
		}
		task = PlayerTask::ptNone;
		//Set the volume to -10000 (silent).
		pBasicAudio->put_Volume (EFFECTIVE_MINIMUM_VOLUME);
	} else {
		//If we don't need to fade in or anything, then we can just set playing.
		state = PlayerState::psPlaying;
		task = PlayerTask::ptNone;
		pBasicAudio->put_Volume (maximumVolume);
	}

	if (FAILED (pMediaControl->Run ())) {
		_MESSAGE ("%lld | MusicPlayer >> Failed to run music player", timeStamp);
		state = PlayerState::psStopped;
		task = PlayerTask::ptNone;
		return false;
	}

	if (printNewTrack) {
		Console_Print ("Now playing track > %s", trackPath.c_str ());
	}
	playedOnce = true;
	return true;
}



bool MusicPlayer::initializeDShow (bool reset) {
	//Get an instance of the graph builder.
	initialized = false;
	HRESULT hr = CoCreateInstance (CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pGraphBuilder);
	if (FAILED (hr) || !pGraphBuilder) {	//Check to see that the GraphBuilder is available.
		switch (hr) {			//There was a problem.  Fail.
			case REGDB_E_CLASSNOTREG:	_MESSAGE ("%lld | MusicPlayer >> Failed to create an instance of IGraphBuilder: class was not registered", timeStamp); break;
			case CLASS_E_NOAGGREGATION:	_MESSAGE ("%lld | MusicPlayer >> Failed to create an instance of IGraphBuilder: this class cannot be created as part of an aggregate", timeStamp); break;
			case E_NOINTERFACE:			_MESSAGE ("%lld | MusicPlayer >> Failed to create an instance of IGraphBuilder: there was a problem with the interface", timeStamp); break;
			case S_OK:					_MESSAGE ("%lld | MusicPlayer >> An instance of the interface was, supposedly, created... but its null, which means it wasn't", timeStamp); break;
			default:					_MESSAGE ("%lld | MusicPlayer >> An unexpected error was thrown during the creation of the interface: the pointer, for some reason, is just null", timeStamp); break;
		}
		if (reset) forceKill ();
		return false;
	}

	//Reinitialize the DShow interfaces.
	//Now, get the other instances we require from the GraphBuilder.
	hr = pGraphBuilder->QueryInterface (IID_IMediaControl, (void **)&pMediaControl);
	if (FAILED (hr) || !pMediaControl) {
		_MESSAGE ("%lld | MusicPlayer >> Failed to obtain Media Control interface from pGraphBuilder", timeStamp);
		if (reset) forceKill ();
		return false;
	}

	hr = pGraphBuilder->QueryInterface (IID_IMediaSeeking, (void**)&pMediaSeeking);
	if (FAILED (hr) || !pMediaSeeking) {
		_MESSAGE ("%lld | MusicPlayer >> Failed to obtain Media Seeking interface from pGraphBuilder", timeStamp);
		if (reset) forceKill ();
		return false;
	}

	hr = pGraphBuilder->QueryInterface (IID_IMediaEventEx, (void**)&pMediaEventEx);
	if (FAILED (hr) || !pMediaEventEx) {
		_MESSAGE ("%lld | MusicPlayer >> Failed to obtain Media Event Ex interface from pGraphBuilder", timeStamp);
		if (reset) forceKill ();
		return false;
	}

	hr = pGraphBuilder->QueryInterface (IID_IBasicAudio, (void**)&pBasicAudio);
	if (FAILED (hr) || !pBasicAudio) {
		_MESSAGE ("%lld | MusicPlayer >> Failed to obtain Basic Audio interface from pGraphBuilder", timeStamp);
		if (reset) forceKill ();
		return false;
	}

	if (!reset) {
		state = PlayerState::psStopped;
		task = PlayerTask::ptNone;
	}
	initialized = true;
	return true;
}



bool MusicPlayer::resetDShow () {
	//If we have already used these DShow interfaces...
	if (playedOnce) {
		if (isInitialized()) {
			pMediaControl->Stop ();
		}
		if (initializeDShow (true)) {
			pBasicAudio->put_Volume (maximumVolume);		//Reset the volume.
			return true;
		}
		return false;
	}
	return true;
}



void MusicPlayer::forceKill () {
	_MESSAGE ("%lld | MusicPlayer >> Force killed!", timeStamp);
	initialized = false;	//Uninitialize the class. This will make sure no functions can execute.
	if (pMediaControl) {
		pMediaControl->Stop ();
	}
	state = PlayerState::psUninitialized;
	task = PlayerTask::ptNone;
}