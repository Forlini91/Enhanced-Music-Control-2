#include "MainThread.h"

#include <string>
#include <unordered_map>

#include "GameAPI.h"

#include "GlobalSettings.h"
#include "VanillaPlaylistData.h"
#include "MainGameVarsPointer.h"
#include "OblivionINI.h"
#include "MusicState.h"
#include "MusicPlayer.h"
#include "Multiplier.h"
#include "Playlist.h"
#include "ActivePlaylist.h"
#include "ThreadState.h"
#include "ThreadRequest.h"



using namespace std;


volatile bool *bMusicEnabled = INI::Audio::bMusicEnabled;
volatile float *iniMasterVolume = INI::Audio::fDefaultMasterVolume;
volatile float *iniMusicVolume = INI::Audio::fDefaultMusicVolume;
extern bool printNewTrack;
extern bool delayTitleMusicEnd;



void MainThread_DelayedInitialization () {
	_MESSAGE ("Initialization >> Playlists");	//actually, just complete the initialization for Death, Success and Title
	apl_Death += obDeathPath_;
	apl_Success += obSuccessPath_;
	apl_Title += obTitlePath_;

	_MESSAGE ("Initialization >> Multipliers");
	multObMaster = Multiplier (&mainGameVars->gameVars->SoundRecords->fMasterVolume);
	multObMasterIni = Multiplier (INI::Audio::fDefaultMasterVolume);
	multObMusic = Multiplier (&mainGameVars->gameVars->SoundRecords->fMusicVolumeDup);
	multObMusicIni = Multiplier (INI::Audio::fDefaultMusicVolume);
	multObEffects = Multiplier (&mainGameVars->gameVars->SoundRecords->fEffectsVolume);
	multObEffectsIni = Multiplier (INI::Audio::fDefaultEffectsVolume);
	multObVoice = Multiplier (&mainGameVars->gameVars->SoundRecords->fVoiceVolume);
	multObVoiceIni = Multiplier (INI::Audio::fDefaultVoiceVolume);
	multObFoot = Multiplier (&mainGameVars->gameVars->SoundRecords->fFootVolume);
	multObFootIni = Multiplier (INI::Audio::fDefaultFootVolume);
}



void MainThread_ResetStartBattleTimer () {
	if (threadState.startBattleTimer > 0) {
		_MESSAGE ("Thread >> Reset BattleDelay at %.0f ms", threadState.startBattleTimer);
		threadState.startBattleTimer = 0;
		musicPlayer.recalculateBattleDelay ();
	}
}



void MainThread_ResetBattleTimer () {
	if (threadState.battleTimer > 0 && threadState.newType != MusicType::Battle) {
		_MESSAGE ("Thread >> Total battle duration: %.0f ms.", threadState.battleTimer);
		threadState.battleTimer = 0;
	}
}



void MainThread_ResetAfterBattleTimer () {
	if (threadState.afterBattleTimer > 0) {
		_MESSAGE ("Thread >> Reset AfterBattleDelay at %.0f ms", threadState.afterBattleTimer);
		threadState.afterBattleTimer = 0;
		musicPlayer.recalculateAfterBattleDelay ();
	}
}



void MainThread_ResetPauseTimer () {
	if (threadState.pauseTimer > 0) {
		_MESSAGE ("Thread >> Reset Pause time at %.0f ms", threadState.pauseTimer);
		threadState.pauseTimer = 0;
		musicPlayer.recalculatePauseTime ();
	}
}



//This is a function to be used by threadState Thread.
//It updates musicPlayer's volume with the volume from Oblivion.
//This ensures that the player can use Oblivion's audio controls
//to control the music.
void MainThread_SyncMusicVolume (void) {
	if (!*bMusicEnabled) {
		musicPlayer.setMaxMusicVolume (ABSOLUTE_MINIMUM_VOLUME);
		return;
	}

	//Get the volume, a value between 0 and 1.
	float fVolume;
	float fMaster;
	if (mainGameVars->gameVars && mainGameVars->gameVars->SoundRecords) {
		fVolume = mainGameVars->gameVars->SoundRecords->fMusicVolumeDup;
		fMaster = mainGameVars->gameVars->SoundRecords->fMasterVolume;
	} else {
		fVolume = *iniMusicVolume;
		fMaster = *iniMasterVolume;
	}

	fVolume *= fMaster;
	for (MultipliersMap::iterator it = multipliersCustom.begin (); it != multipliersCustom.end (); it++) {
		Multiplier& mult = it->second;
		LockHandle (mult.hThread);
		if (!mult.isDestroyed) {
			fVolume *= mult.getValue ();
		}
		UnlockHandle (mult.hThread);
	}
	if (fVolume > 1) fVolume = 1;

	//In the future, i will disable the music in better ways, but for now this is quick, if messy.
	if (fVolume <= 0.0) {
		musicPlayer.setMaxMusicVolume (ABSOLUTE_MINIMUM_VOLUME);
	} else {
		long lVolume = 2000.0 * log10 (fVolume);	//Volume is logarithmic.  This makes it linear.
		musicPlayer.setMaxMusicVolume (lVolume);
	}
}



bool MainThread_SwapPlaylist (void) {
	int musicType = static_cast<int>(threadRequest.Request_Swap_Type);
	_MESSAGE ("Thread >> Swap playlist: %d > %s", musicType, threadRequest.Request_Playlist->name);
	bool ret = false;
	LockHandle (hPlaylistMutex);
	if (isMusicTypeValid(musicType)) {
		ActivePlaylist* var = activePlaylists[musicType];
		*var = threadRequest.Request_Playlist;
		ret = true;
	}
	UnlockHandle (hPlaylistMutex);
	//Remove the swap request.
	threadRequest.Request_Swap_Type = MusicType::Mt_NotKnown;
	return ret;
}



void MainThread_StorePreviousTrack (bool PlayerIsPlaying) {
	if (threadState.curType < 8 && PlayerIsPlaying) {
		_MESSAGE ("Thread >> Store previous track: %s", musicPlayer.getTrack ());
		threadState.lastPlayedSong = musicPlayer.getTrack ();
		threadState.lastPlayedPosition = musicPlayer.getTrackPosition ();
		threadState.lastPlayedSet = true;
	} else {
		_MESSAGE ("Thread >> Don't store previous track");
	}
}



bool MainThread_FixLevelUp (bool PlayerIsPlaying) {
	//Do we need to perform a fix for the success music?
	if (!PlayerIsPlaying && threadState.SuccessMusicFix) {
		_MESSAGE ("Thread >> Success music ended");
		LockHandle (hMusicStateMutex);
		music.world = threadState.curType = threadState.newType = music.worldSaved;
		music.worldSaved = music.state = MusicType::Mt_NotKnown;
		UnlockHandle (hMusicStateMutex);
		threadState.curSpecial = threadState.lastSpecial;
		threadState.SuccessMusicFix = false;
		return true;
	}
	return false;
}



void MainThread_SelectCustomTrack () {
	LockHandle (hThreadMutex);
	string track = threadRequest.Request_Track;
	UnlockHandle (hThreadMutex);
	LockHandle (hPlaylistMutex);
	if (musicPlayer.queueTrack (track)) {
		musicPlayer.playQueuedTrack (FadeMethod::fmFadeOutThenIn);
		threadState.noTrack = false;
	} else {
		musicPlayer.stop (1000);
		threadState.noTrack = true;
	}
	UnlockHandle (hPlaylistMutex);
	MainThread_ResetPauseTimer ();
}



void MainThread_RestorePreviousTrack () {
	//Restore the last song.
	//Save the current song temporarily.
	const char* trackName = musicPlayer.getTrack ();
	REFERENCE_TIME tempPos = musicPlayer.getTrackPosition ();
	if (strcmp(threadState.lastPlayedSong,trackName) != 0) {
		//Restore the previous song.
		if (musicPlayer.queueTrack (threadState.lastPlayedSong, threadState.lastPlayedPosition)) {
			musicPlayer.playQueuedTrack (FadeMethod::fmFadeOutThenIn);
			_MESSAGE ("Thread >> Resume previous track: %s", threadState.lastPlayedSong);
			threadState.noTrack = false;
		} else {
			musicPlayer.stop (1000);
			threadState.noTrack = true;
		}
		//And now save the current song as last song.
		threadState.lastPlayedSong = trackName;
		threadState.lastPlayedPosition = tempPos;
	} else {
		_MESSAGE ("Thread >> Don't resume previous track");
	}
	//Reset the timers.
	threadState.restoreTimer = 0;
	MainThread_ResetBattleTimer ();
	MainThread_ResetPauseTimer ();
}



void MainThread_SelectNewTrack (float fadeOut, float fadeIn) {
	//Place the currently playing song into threadState.
	LockHandle (hPlaylistMutex);
	threadState.activePlaylist = NULL;
	switch (threadState.newType) {
		case Explore: //Select a track from the Explore list.
			threadState.activePlaylist = &apl_Explore; break;
		case Public: //Select a track from the Public list.
			threadState.activePlaylist = &apl_Public; break;
		case Dungeon: //Select a track from the Dungeon list.
			threadState.activePlaylist = &apl_Dungeon; break;
		case Custom: //Select a track from the Custom list.
			threadState.activePlaylist = &apl_Custom; break;
		case Battle: //Select a track from the Battle list.
			threadState.activePlaylist = &apl_Battle;
			fadeOut = 500;
			fadeIn = 1000;
			break;
		case Undefined:
			_MESSAGE ("Thread >> Undefined music type"); break;
			threadState.activePlaylist = NULL;
		case Special:
			fadeOut = fadeIn = 1000;
			switch (threadState.curSpecial) {
				case Death:
					threadState.activePlaylist = &apl_Death;
					threadState.HoldUntilMTChange = true;
					break;
				case Success:
					threadState.activePlaylist = &apl_Success;
					threadState.SuccessMusicFix = true;
					LockHandle (hMusicStateMutex);
						music.worldSaved = threadState.prevType;
					UnlockHandle (hMusicStateMutex);
					fadeOut = 500;
					break;
				case Title:
					threadState.activePlaylist = &apl_Title;
					threadState.loadFromTitle = true;
					break;
				default:
					_MESSAGE ("Thread >> Unknown special music type");
					threadState.activePlaylist = NULL;
					break;
			}
			threadState.lastSpecial = threadState.curSpecial;
			break;
		default:
			_MESSAGE ("Thread >> Unknown music type");
			threadState.activePlaylist = NULL;
			threadState.newType = MusicType::Mt_NotKnown;
			break;
	}
	if (threadState.activePlaylist != NULL) {
		Playlist* playlist = threadState.activePlaylist->playlist;
		const char* trackPath = playlist->next ().c_str();
		_MESSAGE ("Play track from active %s playlist \"%s\" >> %s", threadState.activePlaylist->name, playlist->name, trackPath);
		if (musicPlayer.queueTrack (trackPath)) {
			musicPlayer.playQueuedTrack (FadeMethod::fmFadeOutThenIn, fadeOut, fadeIn);
			threadState.noTrack = false;
			if (printNewTrack) {
				Console_Print ("Now playing %s playlist \"%s\" >> %s", threadState.activePlaylist->name, playlist->name, trackPath);
			}
		} else {
			musicPlayer.stop (1000);
			threadState.noTrack = true;
		}
	} else {
		_MESSAGE ("Thread >> No chosen active playlist for type/special type: %d/%d", threadState.newType, threadState.curSpecial);
	}
	UnlockHandle (hPlaylistMutex);
	MainThread_ResetPauseTimer ();
}



void MainThread_UpdateMusicType (bool newTypeBattle) {
	LockHandle (hMusicStateMutex);
	if (threadState.curType != threadState.newType && threadState.newType < MusicType::Mt_NotKnown) {
		_MESSAGE ("Thread >> Update music types");
		//Shift things around.
		if (threadState.SuccessMusicFix && threadState.newType < 3) {
			threadState.prevType = music.worldSaved;
			music.worldSaved = threadState.newType;
		} else {
			threadState.prevType = threadState.curType;
		}
		threadState.curType = threadState.newType;
		//Reset the restoreTimer.
	}
	music.battlePlaying = newTypeBattle;
	UnlockHandle (hMusicStateMutex);
}



void MainThread (void *throwaway) {
	_MESSAGE ("Thread >> Start");

	//The amount of time the thread will sleep before processing again.
	bool notPlayedOnce = true;
	bool playerIsStopped = true;
	bool playerIsReady = true;
	bool typeChanged = false;
	bool newTypeBattle = false;
	bool newTypeSpecial = false;
	bool performRequest = false;
	bool performCustom = false;
	bool performHoldMusic = false;
	bool swapSucceed = false;
	bool swapDelay = false;
	bool forceUpdateMusicType = false;
	float fadeOutTime = 1000;
	float fadeInTime = 1000;





	//Allow this thread to manipulate COM objects.
	CoInitialize (NULL);

	LockHandle (hMusicPlayerMutex);
		do {
			if (!musicPlayer.initialize ()) {
				_MESSAGE ("Thread >> Player failed initialization.");
				_MESSAGE (musicPlayer.getErrorMessage ());
			} else if (mainGameVars->gameVars) {
				_MESSAGE ("Thread >> Initialize data");
				MainThread_DelayedInitialization ();
				musicPlayer.recalculatePauseTime ();
				musicPlayer.recalculateBattleDelay ();
				musicPlayer.recalculateAfterBattleDelay ();
				fadeOutTime = musicPlayer.getCurrentFadeOutLength ();
				fadeInTime = musicPlayer.getCurrentFadeInLength ();
				_MESSAGE ("Thread >> Player initialized");
				break;
			}
			Sleep (SLEEP_TIME);
		} while (true);
	UnlockHandle (hMusicPlayerMutex);





	byte* bHasQuitGame = &mainGameVars->gameVars->bHasQuitGame;
	while(*bHasQuitGame == 0) {
		LockHandle (hMusicPlayerMutex);
			MainThread_SyncMusicVolume ();
			//These variables are collected now so we don't have to keep
			//claiming the player's mutex.
			playerIsStopped = musicPlayer.isStopped ();
			playerIsReady = musicPlayer.isReady ();
		UnlockHandle (hMusicPlayerMutex);



		//Get the current status
		LockHandle (hMusicStateMutex);
			//Start checking to see what needs to be done.
			if (!delayTitleMusicEnd && threadState.loadFromTitle && music.world < 8) {
				music.state = MusicType::Mt_NotKnown;
				threadState.loadFromTitle = false;
			}
			threadState.newType = music.GetCurrentMusicType (true);
			music.state = music.state;
			threadState.curSpecial = music.special;
		UnlockHandle (hMusicStateMutex);

		typeChanged = !samePlaylist (threadState.newType, threadState.curType);
		forceUpdateMusicType = true;

		newTypeBattle = (threadState.newType == MusicType::Battle);
		newTypeSpecial = (threadState.newType == MusicType::Special);



		//Update all timers
		if (threadState.restoreTimer <= musicPlayer.getMaxRestoreTime()) {
			threadState.restoreTimer += SLEEP_TIME;
		}

		if (newTypeBattle) {
			if (threadState.startBattleTimer < musicPlayer.getCalculatedBattleDelay()) {
				threadState.startBattleTimer += SLEEP_TIME;
			} else if (!playerIsStopped && threadState.curType != MusicType::Battle) {
				_MESSAGE ("Thread >> BattleDelay time reached. Can now switch to battle music.");
			}
			if (threadState.afterBattleTimer) {
				MainThread_ResetAfterBattleTimer ();
			}
			threadState.battleTimer += SLEEP_TIME;
		} else {
			if (threadState.startBattleTimer) {
				MainThread_ResetStartBattleTimer ();
			}
			int afterBattleDelay = musicPlayer.getCalculatedAfterBattleDelay ();
			if (threadState.afterBattleTimer < afterBattleDelay) {
				if (threadState.curType == MusicType::Battle) {
					threadState.afterBattleTimer += SLEEP_TIME;
				} else {
					threadState.afterBattleTimer = afterBattleDelay;
					_MESSAGE ("Thread >> AfterBattleDelay time forced to max.");
				}
			} else if (!playerIsStopped && threadState.curType == MusicType::Battle) {
				_MESSAGE ("Thread >> AfterBattleDelay time reached. Can now switch to normal music.");
			}
		}



		//Take all requests
		LockHandle (hThreadMutex);
			performRequest = (threadRequest.Request_PlayNext && (		// threadRequest.Request_PlayNext_MusicType != MusicType::Special && (
				(threadRequest.Request_PlayNext_MusicType == threadState.newType) ||
				(threadRequest.Request_PlayNext_MusicType == MusicType::Undefined && !newTypeSpecial)));
			if (performCustom = threadRequest.hasRequestedCustomTrack ()) {
				_MESSAGE ("Thread >> Play custom track: %s", threadRequest.Request_Track.c_str ());
			}
			performHoldMusic = threadRequest.Request_HoldMusic;
			if (performRequest || newTypeSpecial) {				// || threadRequest.Request_PlayNext_MusicType == MusicType::Special
				if (threadRequest.Request_PlayNext) {
					_MESSAGE ("Thread >> Perform request >> Music type: %d", threadRequest.Request_PlayNext_MusicType);
				}
				threadRequest.Request_PlayNext = false;
				threadRequest.Request_PlayNext_MusicType = MusicType::Mt_NotKnown;
			}
			if (threadRequest.Request_Swap_FadeOut >= 0) {
				fadeOutTime = threadRequest.Request_Swap_FadeOut;
				threadRequest.Request_Swap_FadeOut = -1;
			}
			if (threadRequest.Request_Swap_FadeIn >= 0) {
				fadeInTime = threadRequest.Request_Swap_FadeIn;
				threadRequest.Request_Swap_FadeIn = -1;
			}

			//Do we need to do a playlist swap?
			if (threadRequest.Request_Swap_Delay > 0) {
				threadRequest.Request_Swap_Delay -= SLEEP_TIME;
				swapDelay = (threadRequest.Request_Swap_Delay <= 0);
				if (swapDelay) {
					_MESSAGE ("Thread >> Time to swap after the delay");
				}
			}
			if ((playerIsStopped && threadRequest.Request_Swap_Type != MusicType::Mt_NotKnown) || swapDelay) {
				swapSucceed = MainThread_SwapPlaylist ();
				swapDelay = false;
			}
		UnlockHandle (hThreadMutex);



		if (notPlayedOnce) {
			if (musicPlayer.hasPlayedOnce ()) {
				_MESSAGE ("Thread >> Title music is playing. Player will now accept other music types.");
				notPlayedOnce = false;
			}
		}


		//Update the playing music
		//If (type changed) or (the special music changed while playing special music) or (no music is playing).
		if (!notPlayedOnce && playerIsReady) {
			if (performRequest || performCustom || swapSucceed
				|| (playerIsStopped && !threadState.HoldUntilMTChange && !threadState.noTrack)	//Ok, not playing, but if there's no track to play...
				|| (!typeChanged && newTypeSpecial && threadState.curSpecial != threadState.lastSpecial)	//Type not changed, but special track is coming...
				|| (typeChanged && ((!newTypeBattle && threadState.afterBattleTimer >= musicPlayer.getCalculatedAfterBattleDelay ()) || (newTypeBattle && threadState.battleTimer >= musicPlayer.getCalculatedBattleDelay ()))) //Type changed and (it's not a battle OR enough time passed since battle begun)
				) {
				if (threadState.newType != 65535) {
					_MESSAGE ("Thread >> Update music >> New/Current/Previous music types: %d/%d/%d, IsPlaying: %d, Hold: %d, Pause time: %.0f", threadState.newType, threadState.curType, threadState.prevType, !playerIsStopped, performHoldMusic, threadState.pauseTimer);
				}
				threadState.HoldUntilMTChange = false;
				if (MainThread_FixLevelUp (!playerIsStopped)) {
					typeChanged = false;
					newTypeBattle = (threadState.newType == MusicType::Battle);
					newTypeSpecial = (threadState.newType == MusicType::Special);
				}

				LockHandle (hMusicPlayerMutex);
					if (performCustom) {		//Play custom track
						MainThread_StorePreviousTrack (!playerIsStopped);
						MainThread_ResetBattleTimer ();
						MainThread_SelectCustomTrack ();
					} else if (playerIsStopped && (performHoldMusic || threadState.pauseTimer < musicPlayer.getCalculatedPauseTime ())) { //Prevent the player from playing something.
						UnlockHandle (hMusicPlayerMutex);
						if (!performHoldMusic) {
							if (threadState.pauseTimer <= 0) {
								_MESSAGE ("Thread >> Begin pause");
							}
							threadState.pauseTimer += SLEEP_TIME;
							if (threadState.pauseTimer >= musicPlayer.getCalculatedPauseTime ()) {
								_MESSAGE ("Thread >> End pause");
							}
						}
						threadState.lastPlayedSet = false;
						Sleep (SLEEP_TIME);
						continue;
					} else if (threadState.lastPlayedSet && threadState.newType == threadState.prevType && !newTypeSpecial && threadState.curSpecial != SpecialMusicType::Success &&
						((threadState.restoreTimer <= musicPlayer.getMaxRestoreTime()) ||
						(threadState.curType == MusicType::Battle && !newTypeBattle && threadState.battleTimer < 60000))
						) {	//Resume the previous track 
						MainThread_RestorePreviousTrack ();
						//Or play a new one
					} else if (threadState.newType <= 255) {
						MainThread_StorePreviousTrack (!playerIsStopped);
						MainThread_ResetBattleTimer ();
						if (!(performRequest || swapSucceed)) {
							fadeOutTime = musicPlayer.getCurrentFadeOutLength ();
							fadeInTime = musicPlayer.getCurrentFadeInLength ();
						}
						MainThread_SelectNewTrack (fadeOutTime, fadeInTime);
					}
				UnlockHandle (hMusicPlayerMutex);

				threadState.restoreTimer = 0;
				MainThread_UpdateMusicType (newTypeBattle);
				fadeOutTime = musicPlayer.getCurrentFadeOutLength ();
				fadeInTime = musicPlayer.getCurrentFadeInLength ();
				swapSucceed = false;
				forceUpdateMusicType = false;
			}
		}
		Sleep (SLEEP_TIME);
		LockHandle (hMusicPlayerMutex);
		musicPlayer.updatePlayer (SLEEP_TIME);
		UnlockHandle (hMusicPlayerMutex);
		if (forceUpdateMusicType) {
			MainThread_UpdateMusicType (newTypeBattle);
			forceUpdateMusicType = false;
		}
	}
	_MESSAGE ("Thread >> Exited normally.");
	_endthread ();
}
