#pragma once

#include <string>
#include "TimeManagement.h"



//This is the very lowest volume that can be used in DShow.
//Setting the volume to this effectively mutes the playing track.
#define ABSOLUTE_MINIMUM_VOLUME -10000

//Below -4000, you can't really hear the audio anyways.
//So, this is the effective minimum, and is what is used during fades.
#define EFFECTIVE_MINIMUM_VOLUME -4000

//There is only one maximum. It would be funny if they made it '11'.
#define MAXIMUM_VOLUME 0



using namespace std;



//This enumeration will keep track of the player's state.
enum PlayerState : int {
	psStopped = 0,			//Player is stopped.
	psPlaying,				//Player is playing.
	psPaused,				//Player is paused.
	psFadingOut,			//Player is fading out.
	psFadingIn,				//Player is fading in.
	psFadedOut,				//Player has faded out and is ready to continue it's task.
	psFadedIn,				//Player has faded in and has completed it's task.
	psUninitialized = 0xFF	//Player is not ready for use.
};


//A player task is what the state above is performing for.
//For instance, if the PlayerTask is Stop and the State is
//FadingOut, the player is fading out to stop.
enum PlayerTask : short {
	ptNone = 0,				//The state applies to no specific task.
	ptQueue,				//The state applies to playing a Queued Track.
	ptRestart,				//The state applies to restarting the playing track.
	ptPause,				//The state applies to pausing the playing track.
	ptResume,				//The state applies to resuming a paused track.
	ptStop,					//The state applies to stopping the current track.
	ptPosition				//The state applies to change the track position.
};


enum FadeMethod : short {
	fmNoFade = 0,
	fmFadeIn,
	fmFadeOut,
	fmFadeOutThenIn,
};


/*
FadingIn -> FadedIn/Playing
*/





struct QueuedTrack {
	QueuedTrack (const string &name, LONGLONG position) : name (name), position (position), queued (true) {};
	QueuedTrack () : name(""), position(0), queued (false) {};

	bool get (string *name, LONGLONG *position);
	bool isQueued () const;

private:
	string name;
	LONGLONG position;
	bool queued;

};



//Finally, the class that will facilitate our playing of the music.
//We will try and keep it simple, similar to how Oblivion works.
//Queue a track, then play it.
class MusicPlayer {
public:


	~MusicPlayer ();


	//Initialize the music player. Returns true if succeeded
	bool initialize ();


	//Returns true if the player is initialized and is ready to recieve a song to play
	bool isInitialized () const;


	//Returns true if the player has played at least once.
	bool hasPlayedOnce () const;


	//Returns true if the music player is playing
	bool isPlaying () const;


	//Returns true is the music player is paused.
	bool isPaused () const;


	//Returns true if the music player is stopped.
	bool isStopped () const;


	//Checks if the player is changing track and can't be queued up.
	//Typically, the player can't be interupted while it fades, except for when the program is exiting.
	//It returns false if the player is playing, paused or stopped, true otherwise.
	bool isReady () const;


	
	//Queues a track to play (only one track at a time).
	//Starts playback from the specified position.
	bool queueTrack (const string &trackPath, LONGLONG position);


	
	//Plays the queued track.
	//If a fadeMethod is given, the fade will occure over the lenght of the given fade out and fade in times.
	bool playQueuedTrack (FadeMethod fadeMethod, int fadeOut, int fadeIn);



	//Gets the track's full pathname.
	const string& getTrack () const;

	//Gets the track's position
	LONGLONG getTrackPosition () const;

	//Change the current track position.
	//If a song is currently playing and the fade method is not NoFade, the song will be faded to a stop then restarted from the given position.
	bool setTrackPosition (LONGLONG position, int fadeOut, int fadeIn);

	//Gets the track's duration
	LONGLONG getTrackDuration () const;


	//Sets the maximum volume of the player.
	//Setting this while fading will not adversely affect the fading process.
	bool setMaxMusicVolume (float volume);

	//Gets the music speed
	double getMusicSpeed () const;

	//Sets the music speed
	bool setMusicSpeed (double speed);




	//Stops playback.  Can be restarted with Restart().
	bool stop (int fadeOut);

	//Pauses the currently playing track. This does nothing if the track is stopped. 
	//If any fade method other then NoFade is used, the track is faded to a pause.
	bool pause (int fadeOut);

	//Resumes the currently paused track.
	//This does nothing if the track is stopped (use Restart to play a track again).
	//If any fade method other than NoFade is used, the track is faded in when it is resumed.
	bool resume (int fadeIn);

	//Restarts the current track.
	//If a song is currently playing and the fade method is not NoFade, the song will be faded to a stop then restarted from the beginning.
	bool restart (int fadeOut, int fadeIn);

	//Should be called by MainThread once an iteration.
	// This function does things like checks the state of the player, fades the volume, and keeps thing in order.
	void updatePlayer (int sleepTime);

	//Forces the player to halt, irregardless of it's status.
	//Use this when the program exits to put an immediately halt to the player.
	//Like a self-destruct button, it's usable only once and located in the central brain area.
	void forceKill ();





private:

	//True if the player initialized successfully.
	bool initialized = false;

	//If set to true, all functions of the program will immediately be stopped.
	//This includes playback, fading, and event handling.
	bool forceKilled = false;


	//This is where the player keeps it's current state.
	PlayerState state = PlayerState::psStopped;

	//This is the task the player will perform.
	PlayerTask task = PlayerTask::ptNone;

	//This variable keeps track of the fading while switching.
	FadeMethod fadeMethod = FadeMethod::fmNoFade;;

	//This variable tells the player that it has loaded at least one song in the past.
	//This is important for the Restart function.
	//Since the player initializes to the Stopped state but no graph has been built, calling Restart() may do weird things.
	//This variable will prevent those weird things from occuring.
	bool playedOnce = false;



	QueuedTrack queuedTrack = QueuedTrack ();

	//Holds the path to the track played
	string trackPath;

	//The current track position
	LONGLONG trackDuration = 0;

	//The current track duration
	LONGLONG trackPosition = 0;

	//The new track position
	LONGLONG newTrackPosition = 0;





	//The maximum volume the player will be allowed to play at.
	//Value between -10000 and 0.  Why the fuck is this a long?
	//0 is actually full blast...  -10000 is infintessimally quiet.
	long maximumVolume = 0;

	
	//This variable indicates the length of time the player should fade out the volume for.
	int fadeOutPeriod = 1000;

	//This variable indicates the length of time the player should fade in the volume for.
	int fadeInPeriod = 1000;

	//The total time the thread has slept since fading started.
	int fadeTotalSleepTime = 0;





	//Fades the volume of the playing music in or out as needed.
	bool doFading (int sleepTime);

	//Whenever a task was to be carried out after a fade, this function is the thing that does it.
	//It does a lot of things depending on what the task is, so tread carefully.
	bool doTask ();

	//Does the DShow event handling and check the state.
	//If no event affected the state of the player, it returns the current state else the event return the new state.
	//Almost always, this change will be psStopped, since all we're testing for is if the player has stopped.
	PlayerState checkState ();

	bool playTrack (FadeMethod newFadeMethod, int fadeOut, int fadeIn);

	//Initialize DirectShow
	bool initializeDShow (bool reset);

	//Resets DirectShow for whenever we want to play a new file, basically.
	bool resetDShow ();

};



extern MusicPlayer musicPlayer;
extern HANDLE hMusicPlayerMutex;	//Lock when using the object "musicPlayer".