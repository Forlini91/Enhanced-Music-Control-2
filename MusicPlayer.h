#pragma once

#include <string>
#include <tchar.h>		//Don't ask me why, it doesn't compile without this...
#include <dshow.h>
#include "PlayerData.h"

using namespace std;



//Finally, the class that will facilitate our playing of the music.
//We will try and keep it simple, similar to how Oblivion works.
//Queue a track, then play it.
class MusicPlayer {
public:


	~MusicPlayer ();


	bool initialize ();


	//Queues a track to play (only one track at a time).
	//Starts playback from beginning.
	bool queueTrack (const char* track);


	//Queues a track to play (only one track at a time).
	//Starts playback from the specified position.
	bool queueTrack (const char* track, LONGLONG position);


	//Playes the queued track.  Uses the given fade method
	//when starting and/or stopping playback.  If a fade is
	//specified, the fade will occur over defaultFade milliseconds.
	bool playQueuedTrack (FadeMethod fadeMethod);


	//Playes the queued track.  Uses the given fade method
	//when starting and/or stopping playback.  If a fade is
	//specified, the fade will occur over the length of time
	//(in milliseconds) specified.
	bool playQueuedTrack (FadeMethod fadeMethod, float FadeLength);


	//Plays the queued track.  This version, if a fadeMethod is described,
	//allows fine control over the length of time spent fading in and
	//fadign out, individually.
	bool playQueuedTrack (FadeMethod fadeMethod, float FadeOutLength, float FadeInLength);


	//Returns true if the player initializaed and is ready to recieve
	//a song to play.
	bool isInitialized ();


	//Returns true if a track is playing.
	bool isPlaying ();


	//Returns true is a track is paused.
	//Note:  It can be both playing and paused!  Paused != Stopped!
	bool isPaused ();


	//Returns true if a song is waiting to be played.
	bool isQueuedUp ();


	//Returns true if the player can be queued up.
	//IE:  Its not fading, faded, or uninitialized.
	bool isQueuable ();


	//Returns the position of the playing song.
	LONGLONG getSongPosition ();


	//Returns the track's path.
	const char* getSongPath ();


	//If any of these methods returns false (as in they failed), the error
	//can be obtained from here to assist the user in debugging.
	const string getErrorMessage ();
	

	//Stops playback.  Can be restarted with Restart().
	bool stop (int fadeOut);


	//Pauses the currently playing track.  This does nothing if the track
	//is stopped.  If any fade method other then NoFade is used, the track
	//is faded to a pause over 1 second.
	void pause (int fadeOut);


	//Resumes the currently paused track.  This does nothing if the track
	//is stopped (use Restart to play a track again).  If any fade method
	//other than NoFade is used, the track is faded in when it is resumed.
	void resume (int fadeIn);


	//Restarts the current track.  If a song is currently playing and the
	//fade method is not NoFade, the song will be faded to a stop then
	//restarted from the beginning.
	bool restart (int fadeOut, int fadeIn);


	//Change the current track position.  If a song is currently playing and the
	//fade method is not NoFade, the song will be faded to a stop then
	//restarted from the given position.
	bool setPosition (int fadeOut, int fadeIn, LONGLONG position);


	//Should be called by SentryThread once an iteration.  This function does
	//things like checks the state of the player, fades the volume, and keeps
	//thing in order.
	void doPlayerStuff (int sleepTime);

	LONGLONG getTrackDuration ();

	float getDefaultFadeLength ();

	float getCurrentFadeInLength ();

	bool setCurrentFadeInLength (float length);

	float getCurrentFadeOutLength ();

	bool setCurrentFadeOutLength (float length);

	float getMinPauseTime ();

	bool setMinPauseTime (float length);

	float getExtraPauseTime ();

	bool setExtraPauseTime (float length);

	float getFinalPauseTime (bool update);

	float getMinBattleDelay ();

	bool setMinBattleDelay (float length);

	float getExtraBattleDelay ();

	bool setExtraBattleDelay (float length);

	float getFinalBattleDelay (bool update);

	float getMinAfterBattleDelay ();

	bool setMinAfterBattleDelay (float length);

	float getExtraAfterBattleDelay ();

	bool setExtraAfterBattleDelay (float length);

	float getFinalAfterBattleDelay (bool update);

	float getMaxRestoreTime ();

	bool setMaxRestoreTime (float length);


	//Sets the maximum volume of the player.
	//Setting this while fading will not adversely affect the fading process.
	bool setMaxMusicVolume (long lVolume);

	double getMusicSpeed (void);

	bool setMusicSpeed (double speed);

	//Forces the player to halt, irregardless of it's status.  Use this when the
	//program exits to put an immediately halt to the player.  Like a self-destruct
	//button, its both usable only once and located in the central brain area.  :D
	void forceKill ();


private:

	//Holds the path to the track currently being played by the player.
	char playingTrack[MAX_PATH];

	//Holds the path to the track that will be played when PlayQueuedTrack
	//is called.  If no track is queued, calling that method will do nothing.
	char queuedTrack[MAX_PATH];

	//This would be the position the queued track should be started at when
	//its played.
	LONGLONG queuedTrackPosition;

	//Indicates if a track is queued and ready.
	bool trackQueuedUp;

	//The last error message given.
	string lastError;

	//True if the player initialized successfully.
	bool initialized;

	//If set to true, all functions of the program will immediately be stopped.
	//This includes playback, fading, and event handling.
	bool forceKilled;

	//The default fade time.  This value is used when the fader is tasked
	//with fading in or fading out.  In milliseconds.
	float defaultFadeLength;

	float currentFadeInLength;
	float currentFadeOutLength;

	float minPauseTime;
	float extraPauseTime;
	float effectivePauseTime;

	float minBattleTime;
	float extraBattleTime;
	float effectiveBattleTime;

	float minAfterBattleTime;
	float extraAfterBattleTime;
	float effectiveAfterBattleTime;

	//Restore period is used to restore the previous song if the music type
	//changes once, then suddenly changes back again.  Say the player enters
	//an Inn, then exits within a short period of time.  This will make it so
	//the previous music will play where it left off instead of restarting
	//a new track.
	float maxMusicRestoreTime;

	//The maximum volume the player will be allowed to play at.
	//Value between -10000 and 0.  Why the fuck is this a long?
	long maximumVolume;

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

	//All the state and task of the player
	PlayerData playerData;

	//Does the DShow event handling.
	//Returns a PlayerState value.  If no event affected the state
	//of the player, the return value will equal playerData.state, else
	//the event mandates a change in the state.  Almost allways, this
	//change will be psStopped, since all we're testing for is if the
	//player has stopped.
	PlayerState doDShowEvents ();

	//Fades the volume of the playing music in or out as needed.
	void doFading (int sleepTime);

	//Whenever a task was to be carried out after a fade, this function
	//is the thing that does it.  It does a lot of things depending on
	//what the task is, so tread carefully.
	void doTaskStuff ();

	//Resets DirectSHow for whenever we want to play a new file, basically.
	void ResetDShow ();

};