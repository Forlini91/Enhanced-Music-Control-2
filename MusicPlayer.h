#pragma once

#include <string>
#include <tchar.h>		//Don't ask me why, it doesn't compile without this...
#include <dshow.h>

//1 Second
#define ONCE_SECOND 10000000.0

//This is the very lowest volume that can be used in DShow.
//Setting the volume to this effectively mutes the playing track.
#define ABSOLUTE_MINIMUM_VOLUME -10000

//But below -4000, you can't really hear the audio anyways.
//So, this is the effective minimum, and is what is used
//during fades.
#define EFFECTIVE_MINIMUM_VOLUME -4000

//There is only one maximum.  It would be funny if they made it '11'.
#define MAXIMUM_VOLUME 0




using namespace std;



extern HANDLE hMusicPlayerMutex;



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


struct QueuedTrack {
	QueuedTrack (const string& name, REFERENCE_TIME position) : name (name), position (position), queued (true) {};
	QueuedTrack () : queued (false) {};

	bool get (string* name, REFERENCE_TIME* position);
	bool isQueued () const;

private:
	string name;
	REFERENCE_TIME position;
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


	//Returns true if a song is waiting to be played.
	bool isQueuedUp () const;


	//Queues a track to play (only one track at a time).
	//Starts playback from beginning.
	bool queueTrack (const string& track);


	//Queues a track to play (only one track at a time).
	//Starts playback from the specified position.
	bool queueTrack (const string& track, REFERENCE_TIME position);


	//Playes the queued track.  Uses the given fade method
	//when starting and/or stopping playback.  If a fade is
	//specified, the fade will occur over defaultFade milliseconds.
	bool playQueuedTrack (FadeMethod fadeMethod);


	//Playes the queued track.  Uses the given fade method
	//when starting and/or stopping playback.  If a fade is
	//specified, the fade will occur over the length of time
	//(in milliseconds) specified.
	bool playQueuedTrack (FadeMethod fadeMethod, int FadeLength);


	//Plays the queued track.  This version, if a fadeMethod is described,
	//allows fine control over the length of time spent fading in and
	//fadign out, individually.
	bool playQueuedTrack (FadeMethod fadeMethod, int FadeOutLength, int FadeInLength);



	//Gets the track's full pathname.
	const char* getTrack () const;

	//Gets the track's position
	REFERENCE_TIME getTrackPosition () const;

	//Change the current track position.  If a song is currently playing and the
	//fade method is not NoFade, the song will be faded to a stop then
	//restarted from the given position.
	bool setTrackPosition (REFERENCE_TIME position, int fadeOut, int fadeIn);

	//Gets the track's duration
	REFERENCE_TIME getTrackDuration () const;

	//Gets the current fade in lenght
	int getCurrentFadeInLength () const;

	//Sets the current fade in lenght
	bool setCurrentFadeInLength (int length);

	//Gets the current fade out lenght
	int getCurrentFadeOutLength () const;

	//Sets the current fade out lenght
	bool setCurrentFadeOutLength (int length);


	//Gets the min pause between tracks
	int getMinPauseTime () const;

	//Sets the min pause between tracks
	bool setMinPauseTime (int length);

	//Gets the extra pause between tracks
	int getExtraPauseTime () const;

	//Sets the extra pause between tracks
	bool setExtraPauseTime (int length);

	//Gets the calculated random pause (in the range [min, min+extra]) between tracks
	int getCalculatedPauseTime () const;

	//Calculate a new random pause (in the range [min, min+extra]) between tracks
	void recalculatePauseTime ();


	//Gets the extra delay before starting the battle music
	int getMinBattleDelay () const;

	//Sets the min delay before starting the battle music
	bool setMinBattleDelay (int length);

	//Gets the extra delay before starting the battle music
	int getExtraBattleDelay () const;

	//Sets the extra delay before starting the battle music
	bool setExtraBattleDelay (int length);

	//Gets the calculated random delay (in the range [min, min+extra]) before starting the battle music
	int getCalculatedBattleDelay () const;

	//Calculate a new random delay (in the range [min, min+extra]) before starting the battle music
	void recalculateBattleDelay ();


	//Gets the min delay before stopping the battle music
	int getMinAfterBattleDelay () const;

	//Sets the min delay before stopping the battle music
	bool setMinAfterBattleDelay (int length);

	//Gets the extra delay before stopping the battle music
	int getExtraAfterBattleDelay () const;

	//Sets the extra delay before stopping the battle music
	bool setExtraAfterBattleDelay (int length);

	//Gets the calculated random delay (in the range [min, min+extra]) before stopping the battle music
	int getCalculatedAfterBattleDelay () const;

	//Calculate a new random delay (in the range [min, min+extra]) before stopping the battle music
	void recalculateAfterBattleDelay ();


	//Gets the max restore time (how much time the player will remember the previous track to restore it)
	int getMaxRestoreTime () const;

	//Sets the max restore time (how much time the player will remember the previous track to restore it)
	bool setMaxRestoreTime (int length);


	//Sets the maximum volume of the player.
	//Setting this while fading will not adversely affect the fading process.
	bool setMaxMusicVolume (long lVolume);

	//Gets the music speed
	double getMusicSpeed (void) const;

	//Sets the music speed
	bool setMusicSpeed (double speed);




	//Stops playback.  Can be restarted with Restart().
	bool stop (int fadeOut);

	//Pauses the currently playing track.  This does nothing if the track
	//is stopped.  If any fade method other then NoFade is used, the track
	//is faded to a pause over 1 second.
	bool pause (int fadeOut);

	//Resumes the currently paused track.  This does nothing if the track
	//is stopped (use Restart to play a track again).  If any fade method
	//other than NoFade is used, the track is faded in when it is resumed.
	bool resume (int fadeIn);

	//Restarts the current track.  If a song is currently playing and the
	//fade method is not NoFade, the song will be faded to a stop then
	//restarted from the beginning.
	bool restart (int fadeOut, int fadeIn);

	//Should be called by SentryThread once an iteration.  This function does
	//things like checks the state of the player, fades the volume, and keeps
	//thing in order.
	void updatePlayer (int sleepTime);

	//Forces the player to halt, irregardless of it's status.  Use this when the
	//program exits to put an immediately halt to the player.  Like a self-destruct
	//button, its both usable only once and located in the central brain area.  :D
	void forceKill ();

	//If any of these methods returns false (as in they failed), the error
	//can be obtained from here to assist the user in debugging.
	const char* getErrorMessage () const;





private:

	//True if the player initialized successfully.
	bool initialized = false;

	//If set to true, all functions of the program will immediately be stopped.
	//This includes playback, fading, and event handling.
	bool forceKilled = false;

	//These contain pointers to our DShow interfaces.
	//GraphBuilder lets us generate graphs which are used to render the sound.
	IGraphBuilder *pGraphBuilder = NULL;

	//MediaControl allows us to control the playback of the graph.
	IMediaControl *pMediaControl = NULL;

	//MediaSeeking allows us to obtain position and playback information.
	IMediaSeeking *pMediaSeeking = NULL;

	//MediaEventEx allows us to handle events generated by the DShow interfaces.
	IMediaEventEx *pMediaEventEx = NULL;

	//BasicAudio allows us to affect the balance and amplitude of the playback.
	//In other words, its the volume control.
	IBasicAudio *pBasicAudio = NULL;



	//This is where the player keeps it's current state.
	PlayerState state = PlayerState::psStopped;

	//This is the task the player will perform.
	PlayerTask task = PlayerTask::ptNone;

	//This variable keeps track of the fading while switching.
	FadeMethod fadeMethod = FadeMethod::fmNoFade;;

	//The last error message given.
	string lastError;

	//This variable tells the player that it has loaded at least one song in the past.
	//This is important for the Restart function.
	//Since the player initializes to the Stopped state but no graph has been built, calling Restart() may do weird things.
	//This variable will prevent those weird things from occuring.
	bool playedOnce = false;



	QueuedTrack queuedTrack = QueuedTrack ();

	//Holds the path to the track played
	string trackName;

	//The current track position
	REFERENCE_TIME trackDuration = 0;

	//The current track duration
	REFERENCE_TIME trackPosition = 0;

	//The new track position
	REFERENCE_TIME newTrackPosition = 0;



	int currentFadeInLength = 1000;
	int currentFadeOutLength = 1000;

	int minPauseTime = 0;
	int extraPauseTime = 0;
	int calculatedPauseTime;

	int minBattleTime = 1000;
	int extraBattleTime = 0;
	int calculatedBattleTime;

	int minAfterBattleTime = 0;
	int extraAfterBattleTime = 0;
	int calculatedAfterBattleTime;

	//Restore period is used to restore the previous song if the music type
	//changes once, then suddenly changes back again.  Say the player enters
	//an Inn, then exits within a short period of time.  This will make it so
	//the previous music will play where it left off instead of restarting
	//a new track.
	int maxMusicRestoreTime = 20000;

	//The maximum volume the player will be allowed to play at.
	//Value between -10000 and 0.  Why the fuck is this a long?
	//0 is actually full blast...  -10000 is infintessimally quiet.
	long maximumVolume = 0;

	
	//This variable indicates the length of time the player should fade
	//out the volume for.
	int fadeOutPeriod = 1000;
	//This variable indicates the length of time the player should fade
	//in the volume for.
	int fadeInPeriod = 1000;

	//The total time the thread has slept since fading started.
	int fadeTotalSleepTime = 0;





	//Fades the volume of the playing music in or out as needed.
	bool doFading (int sleepTime);

	//Whenever a task was to be carried out after a fade, this function
	//is the thing that does it.  It does a lot of things depending on
	//what the task is, so tread carefully.
	bool doTask ();

	//Does the DShow event handling and check the state.
	//If no event affected the state of the player, it returns the current state else the event return the new state.
	//Almost always, this change will be psStopped, since all we're testing for is if the player has stopped.
	PlayerState checkState ();

	//Initialize DirectShow
	bool initializeDShow (bool reset);

	//Resets DirectShow for whenever we want to play a new file, basically.
	bool resetDShow ();

};



extern MusicPlayer musicPlayer;