#pragma once

//This enumeration will keep track of the player's state.
enum PlayerState : int {
	psStopped = 0,			//Player is stopped and not playing.
	psPlaying,				//Player is playing.
	psPaused,				//Player is paused.
	psFadingOut,			//Player is in the middle of fading out.
	psFadedOut,				//Player has faded and is ready to continue it's task.
	psFadingIn,				//Player is fading in.
	psFadedIn,
	psUninitialized = 0xFF	//Player is not ready for use.
};


//A player task is what the state above is performing for.
//For instance, if the PlayerTask is Stop and the State is
//FadingOut, the player is fading out to stop.
enum PlayerTask : int {
	ptNone = 0,				//The state applies to no specific task.
	ptQueue,				//The state applies to playing a Queued Track.
	ptRestart,				//The state applies to restarting the playing track.
	ptPause,				//The state applies to pausing the playing track.
	ptResume,				//The state applies to resuming a paused track.
	ptStop,					//The state applies to stopping the current track.
	ptScript,				//The state was initiated by a script command.  (not yet supported)
	ptPosition				//The state applies to change the track position.
};

enum FadeMethod : int {
	fmNoFade = 0,
	fmFadeIn,
	fmFadeOut,
	fmFadeOutThenIn,
	fmUndefined = 0xFF
};


struct PlayerData {
	//This is where the player keeps it's current state.
	PlayerState state = PlayerState::psUninitialized;
	PlayerTask task = PlayerTask::ptNone;
	//Indicates if the player is in the process of changing tracks.
	//Typically, the player can't be interupted while it fades, however
	//there is an override for when the program is exitting.
	//This variable should allways be true when State is not Stopped,
	//Playing, or Paused.
	bool switching;
	//This variable keeps track of the fading while switching.
	FadeMethod fadeMethod;
	//This variable indicates the length of time the player should fade
	//out the volume for.
	int fadeOutPeriod;
	//This variable indicates the length of time the player should fade
	//in the volume for.
	int fadeInPeriod;

	//The total time the thread has slept since fading started.
	int fadeTotalSleepTime;

	//This variable tells the player that it has loaded at least one
	//song in the past.  This is important for the Restart function.
	//Since the player initializes to the Stopped state but no graph
	//has been built, calling Restart() may do weird things.  This
	//variable will prevent those weird things from occuring.
	bool playedOnce;

	LONGLONG currentTrackDuration;
	LONGLONG currentTrackPosition;
	LONGLONG newTrackPosition;
};