#include "MainThread.h"

#include <string>
#include <unordered_map>

#include "GameAPI.h"

#include "GlobalSettings.h"
#include "OblivionINI.h"
#include "MainGameVarsPointer.h"
#include "MusicState.h"
#include "MusicPlayer.h"
#include "Multiplier.h"
#include "Playlist.h"



using namespace std;


extern MainGameVarsPointer *mainGameVars;
extern MusicState music;
extern MusicPlayer musicPlayer;
extern HANDLE hMusicTypeMutex;		//"Music Type" mutex  Lock when changing anything in Music.
extern HANDLE hThePlayerMutex;		//"musicPlayer" Mutex.  Lock when using musicPlayer.
extern HANDLE hPlaylistMutex;		//"Playlist" Mutex.  Lock when manipulating any of the playlists.
extern HANDLE hSentryRequestMutex;
extern Playlist *plExplore;
extern Playlist *plPublic;
extern Playlist *plDungeon;
extern Playlist *plCustom;
extern Playlist *plBattle;
extern Playlist *plTitle;
extern Playlist *plDeath;
extern Playlist *plSuccess;
extern map <MusicType, Playlist**> varsMusicType;
extern unordered_map <string, Playlist> playlists;
extern unordered_map <string, Multiplier> multipliersVanilla;
extern unordered_map <string, Multiplier> multipliersCustom;
extern ThreadRequest threadRequest;
extern ThreadState threadState;

extern bool printNewTrack;
extern bool delayTitleMusicEnd;


extern void applyIniValues ();



bool isInRange (float val, float min, float max) {
	return (val >= min && val <= max);
}







void MainThread_ResetStartBattleTimer () {
	if (threadState.startBattleTimer > 0) {
		_MESSAGE ("Thread >> Reset BattleDelay at %.0f ms", threadState.startBattleTimer);
		threadState.startBattleTimer = 0;
		musicPlayer.getFinalBattleDelay (true);
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
		musicPlayer.getFinalAfterBattleDelay (true);
	}
}



void MainThread_ResetPauseTimer () {
	if (threadState.pauseTimer > 0) {
		_MESSAGE ("Thread >> Reset Pause time at %.0f ms", threadState.pauseTimer);
		threadState.pauseTimer = 0;
		musicPlayer.getFinalPauseTime (true);
	}
}



//This is a function to be used by threadState Thread.
//It updates musicPlayer's volume with the volume from Oblivion.
//This ensures that the player can use Oblivion's audio controls
//to control the music.
void MainThread_SyncMusicVolume (void) {
	//Get the volume, a value between 0 and 1.
	float fVolume;
	float fMaster;
	if (mainGameVars->gameVars && mainGameVars->gameVars->SoundRecords) {
		fVolume = mainGameVars->gameVars->SoundRecords->fMusicVolumeDup;
		fMaster = mainGameVars->gameVars->SoundRecords->fMasterVolume;
	} else {
		fVolume = *(INI::Audio::fDefaultMusicVolume);
		fMaster = *(INI::Audio::fDefaultMasterVolume);
	}

	fVolume *= fMaster;
	for (auto it = multipliersCustom.begin (); it != multipliersCustom.end (); it++) {
		Multiplier& mult = it->second;
		WaitForSingleObject (mult.hThread, INFINITE);
		if (!mult.isDestroyed) {
			fVolume *= mult.getValue ();
		}
		ReleaseMutex (mult.hThread);
	}
	if (fVolume > 1) fVolume = 1;

	//In the future, i will disable the music in better ways,
	//but for now this is quick, if messy.  Will be fixed by
	//the 1.0 release.
	if (fVolume == 0.0 || !*INI::Audio::bMusicEnabled) {
		musicPlayer.setMaxMusicVolume (ABSOLUTE_MINIMUM_VOLUME);
		return;
	}

	//Volume is logarithmic.  This makes it linear.
	long lVolume = 2000.0 * log10 (fVolume);
	//long lVolume = EFFECTIVE_MINIMUM_VOLUME - (EFFECTIVE_MINIMUM_VOLUME * fVolume);
	musicPlayer.setMaxMusicVolume (lVolume);
}



bool MainThread_SwapPlaylist (void) {
	_MESSAGE ("Thread >> Swap playlist: %d > %s", threadRequest.Request_Swap_Type, threadRequest.Request_Swap_plName);
	bool ret = false;
	WaitForSingleObject (hPlaylistMutex, INFINITE);
	auto it = playlists.find (threadRequest.Request_Swap_plName);
	if (it != playlists.end()) {
		if (threadRequest.Request_Swap_Type >= 0 && threadRequest.Request_Swap_Type <= 4) {
			Playlist** var = varsMusicType.at (threadRequest.Request_Swap_Type);
			*var = &it->second;
			ret = true;
		}
	}
	ReleaseMutex (hPlaylistMutex);
	//Remove the swap request.
	threadRequest.Request_Swap_Type = MusicType::Mt_NotKnown;
	return ret;
}



void MainThread_StorePreviousTrack (bool PlayerIsPlaying) {
	if (threadState.curType < 8 && PlayerIsPlaying) {
		strcpy_s (threadState.lastPlayedSong, MAX_PATH, musicPlayer.getSongPath ());
		threadState.lastPlayedPosition = musicPlayer.getSongPosition ();
		threadState.lastPlayedSet = true;
		_MESSAGE ("Thread >> Store previous track: %s", threadState.lastPlayedSong);
	} else {
		_MESSAGE ("Thread >> Don't store previous track");
	}
}



bool MainThread_FixLevelUp (bool PlayerIsPlaying) {
	//Do we need to perform a fix for the success music?
	if (!PlayerIsPlaying && threadState.SuccessMusicFix) {
		_MESSAGE ("Thread >> Success music ended");
		WaitForSingleObject (hMusicTypeMutex, INFINITE);
		music.world = threadState.curType = threadState.newType = music.worldSaved;
		music.worldSaved = music.state = MusicType::Mt_NotKnown;
		ReleaseMutex (hMusicTypeMutex);
		threadState.curSpecial = threadState.lastSpecial;
		threadState.SuccessMusicFix = false;
		return true;
	}
	return false;
}



void MainThread_SelectCustomTrack () {
	WaitForSingleObject (hSentryRequestMutex, INFINITE);
	char temp[MAX_PATH];
	strcpy_s (temp, MAX_PATH, threadRequest.Request_Track_Name);
	ReleaseMutex (hSentryRequestMutex);
	WaitForSingleObject (hPlaylistMutex, INFINITE);
	if (musicPlayer.queueTrack (temp)) {
		musicPlayer.playQueuedTrack (FadeMethod::fmFadeOutThenIn);
		threadState.noTrack = false;
	} else {
		musicPlayer.stop (1000);
		threadState.noTrack = true;
	}
	ReleaseMutex (hPlaylistMutex);
	MainThread_ResetPauseTimer ();
}



void MainThread_RestorePreviousTrack () {
	//Restore the last song.
	//Save the current song temporarily.
	char tempPath[MAX_PATH];
	strcpy_s (tempPath, MAX_PATH, musicPlayer.getSongPath ());
	long tempPos = musicPlayer.getSongPosition ();
	if (strcmp (tempPath, threadState.lastPlayedSong)) {
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
		strcpy_s (threadState.lastPlayedSong, MAX_PATH, tempPath);
		threadState.lastPlayedPosition = tempPos;
	} else {
		_MESSAGE ("Thread >> Don't resume previous track");
	}
	//Reset the timers.
	threadState.restoreTimer = 0;
	MainThread_ResetBattleTimer ();
	MainThread_ResetPauseTimer ();
}



bool MainThread_InitializeSpecialPlaylists (void) {
	plDeath->addPath (obDeathPath"_*.mp3");
	plDeath->addPath (obDeathPath"_*.wav");
	plDeath->addPath (obDeathPath"_*.wma");
	plSuccess->addPath (obSuccessPath"_*.mp3");
	plSuccess->addPath (obSuccessPath"_*.wav");
	plSuccess->addPath (obSuccessPath"_*.wma");
	plTitle->addPath (obTitlePath"_*.mp3");
	plTitle->addPath (obTitlePath"_*.wav");
	plTitle->addPath (obTitlePath"_*.wma");

	multipliersVanilla.emplace (BUILD_IN_PLACE (obMaster, &mainGameVars->gameVars->SoundRecords->fMasterVolume));
	multipliersVanilla.emplace (BUILD_IN_PLACE (obMasterIni, INI::Audio::fDefaultMasterVolume));
	multipliersVanilla.emplace (BUILD_IN_PLACE (obMusic, &mainGameVars->gameVars->SoundRecords->fMusicVolumeDup));
	multipliersVanilla.emplace (BUILD_IN_PLACE (obMusicIni, INI::Audio::fDefaultMusicVolume));
	multipliersVanilla.emplace (BUILD_IN_PLACE (obEffects, &mainGameVars->gameVars->SoundRecords->fEffectsVolume));
	multipliersVanilla.emplace (BUILD_IN_PLACE (obEffectsIni, INI::Audio::fDefaultEffectsVolume));
	multipliersVanilla.emplace (BUILD_IN_PLACE (obFoot, &mainGameVars->gameVars->SoundRecords->fFootVolume));
	multipliersVanilla.emplace (BUILD_IN_PLACE (obFootIni, INI::Audio::fDefaultFootVolume));
	multipliersVanilla.emplace (BUILD_IN_PLACE (obVoice, &mainGameVars->gameVars->SoundRecords->fVoiceVolume));
	multipliersVanilla.emplace (BUILD_IN_PLACE (obVoiceIni, INI::Audio::fDefaultVoiceVolume));
	_MESSAGE ("Thread >> Multipliers initialized");
	return true;
}



void MainThread_SelectNewTrack (float fadeOut, float fadeIn) {
	//Place the currently playing song into threadState.

	WaitForSingleObject (hPlaylistMutex, INFINITE);
	_MESSAGE ("Choose playlist: %d", threadState.newType);
	switch (threadState.newType) {
		case Explore: //Select a track from the Explore list.
			threadState.curPlaylist = plExplore;
			break;
		case Public: //Select a track from the Public list.
			threadState.curPlaylist = plPublic;
			break;
		case Dungeon: //Select a track from the Dungeon list.
			threadState.curPlaylist = plDungeon;
			break;
		case Custom: //Select a track from the Custom list.
			break;
		case Battle: //Select a track from the Battle list.
			threadState.curPlaylist = plBattle;
			fadeOut = 500;
			fadeIn = 1000;
			break;
		case Undefined:
			_MESSAGE ("Thread >> Undefined music type");
			break;
		case Special:
			fadeOut = fadeIn = 1000;
			switch (threadState.curSpecial) {
				case Death:
					threadState.curPlaylist = plDeath;
					threadState.HoldUntilMTChange = true;
					break;
				case Success:
					threadState.curPlaylist = plSuccess;
					fadeOut = 500;
					WaitForSingleObject (hMusicTypeMutex, INFINITE);
					music.worldSaved = threadState.prevType;
					ReleaseMutex (hMusicTypeMutex);
					threadState.SuccessMusicFix = true;
					break;
				case Title:
					threadState.curPlaylist = plTitle;
					threadState.loadFromTitle = true;
					break;
				default:
					_MESSAGE ("Thread >> Unknown special music type");
					break;
			}
			threadState.lastSpecial = threadState.curSpecial;
			break;
		default:
			_MESSAGE ("Thread >> Unknown music type");
			threadState.newType = MusicType::Mt_NotKnown;
			break;
	}
	if (threadState.curPlaylist != NULL) {
		string temp = threadState.curPlaylist->next ();	//Holds the song path temporarily.
		_MESSAGE ("Chosen track: %s", temp.c_str ());
		if (musicPlayer.queueTrack (temp.c_str ())) {
			musicPlayer.playQueuedTrack (FadeMethod::fmFadeOutThenIn, fadeOut, fadeIn);
			threadState.noTrack = false;
			if (printNewTrack) {
				Console_Print ("Now playing: %s", temp.c_str());
			}
		} else {
			musicPlayer.stop (1000);
			threadState.noTrack = true;
		}
	} else {
		_MESSAGE ("Thread >> No chosen playlist");
	}
	ReleaseMutex (hPlaylistMutex);
	MainThread_ResetPauseTimer ();
}



void MainThread_UpdateMusicType (bool newTypeBattle) {
	WaitForSingleObject (hMusicTypeMutex, INFINITE);
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
	ReleaseMutex (hMusicTypeMutex);
}



void MainThread (void *throwaway) {
	_MESSAGE ("Thread >> Start");

	//The amount of time the thread will sleep before processing again.
	int SleepTime = 25;
	bool playerIsPlaying = false;
	bool playerIsQueuable = false;
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

	do {
		//Make sure our player is set.  This also works as a kind of
		//error recovery after ForceKill().
		WaitForSingleObject (hThePlayerMutex, INFINITE);
		if (!musicPlayer.isInitialized ()) {
			if (!musicPlayer.initialize ()) {
				_MESSAGE ("Thread >> Player failed initialization.");
				_MESSAGE (musicPlayer.getErrorMessage ().c_str ());
				Sleep (SleepTime);
				continue;	//Skip the rest until the player is brought up.
			}
			musicPlayer.getFinalPauseTime (true);
			musicPlayer.getFinalBattleDelay (true);
			musicPlayer.getFinalAfterBattleDelay (true);
			fadeOutTime = musicPlayer.getCurrentFadeOutLength ();
			fadeInTime = musicPlayer.getCurrentFadeInLength ();
			_MESSAGE ("Thread >> Player initialized");
		}

		MainThread_SyncMusicVolume ();
		//These variables are collected now so we don't have to keep
		//claiming the player's mutex.
		playerIsPlaying = musicPlayer.isPlaying ();
		playerIsQueuable = musicPlayer.isQueuable ();
		ReleaseMutex (hThePlayerMutex);



		//Get the current status
		WaitForSingleObject (hMusicTypeMutex, INFINITE);
		//Start checking to see what needs to be done.
		if (!delayTitleMusicEnd && threadState.loadFromTitle && music.world < 8) {
			music.state = MusicType::Mt_NotKnown;
			threadState.loadFromTitle = false;
		}
		threadState.newType = music.GetCurrentMusicType (true);
		music.state = music.state;
		threadState.curSpecial = music.special;
		ReleaseMutex (hMusicTypeMutex);
		typeChanged = (threadState.newType != threadState.curType);
		newTypeBattle = (threadState.newType == MusicType::Battle);
		newTypeSpecial = (threadState.newType == MusicType::Special);



		//Update all timers
		if (threadState.restoreTimer <= musicPlayer.getMaxRestoreTime()) {
			threadState.restoreTimer += SleepTime;
		}
		if (newTypeBattle) {
			if (threadState.startBattleTimer < musicPlayer.getFinalBattleDelay(false)) {
				threadState.startBattleTimer += SleepTime;
			} else if (playerIsPlaying && threadState.curType != MusicType::Battle) {
				_MESSAGE ("Thread >> BattleDelay time reached. Can now switch to battle music.");
			}
			if (threadState.afterBattleTimer) {
				MainThread_ResetAfterBattleTimer ();
			}
			threadState.battleTimer += SleepTime;
		} else {
			if (threadState.startBattleTimer) {
				MainThread_ResetStartBattleTimer ();
			}
			int afterBattleDelay = musicPlayer.getFinalAfterBattleDelay (false);
			if (threadState.afterBattleTimer < afterBattleDelay) {
				if (threadState.curType == MusicType::Battle) {
					threadState.afterBattleTimer += SleepTime;
				} else {
					threadState.afterBattleTimer = afterBattleDelay;
					_MESSAGE ("Thread >> AfterBattleDelay time forced to max.");
				}
			} else if (playerIsPlaying && threadState.curType == MusicType::Battle) {
				_MESSAGE ("Thread >> AfterBattleDelay time reached. Can now switch to normal music.");
			}
		}



		//Take all requests
		WaitForSingleObject (hSentryRequestMutex, INFINITE);
		performRequest = (threadRequest.Request_PlayNext && (		// threadRequest.Request_PlayNext_MusicType != MusicType::Special && (
			(threadRequest.Request_PlayNext_MusicType == threadState.newType) ||
			(threadRequest.Request_PlayNext_MusicType == MusicType::Undefined && !newTypeSpecial)));
		performCustom = threadRequest.Request_PlayCustom;
		performHoldMusic = threadRequest.Request_HoldMusic;
		if (performRequest || newTypeSpecial) {				// || threadRequest.Request_PlayNext_MusicType == MusicType::Special
			if (threadRequest.Request_PlayNext) {
				_MESSAGE ("Thread >> Perform request >> Music type: %d", threadRequest.Request_PlayNext_MusicType);
			}
			threadRequest.Request_PlayNext = false;
			threadRequest.Request_PlayNext_MusicType = MusicType::Mt_NotKnown;
		}
		if (performCustom) {
			_MESSAGE ("Thread >> Play custom track: %s", threadRequest.Request_Track_Name);
			threadRequest.Request_PlayCustom = false;
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
			threadRequest.Request_Swap_Delay -= SleepTime;
			swapDelay = (threadRequest.Request_Swap_Delay <= 0);
			if (swapDelay) {
				_MESSAGE ("Thread >> Time to swap after the delay");
			}
		}
		if ((!playerIsPlaying && threadRequest.Request_Swap_Type != MusicType::Mt_NotKnown) || swapDelay) {
			swapSucceed = MainThread_SwapPlaylist ();
			swapDelay = false;
		}
		ReleaseMutex (hSentryRequestMutex);



		if (!threadState.TitlePlayed) {
			if (newTypeSpecial && threadState.curSpecial == SpecialMusicType::Title) {
				_MESSAGE ("Thread >> Title music is playing. Player will now accept other music types.");
				threadState.TitlePlayed = true;
				if (!threadState.Initialized) {
					threadState.Initialized = MainThread_InitializeSpecialPlaylists ();
					applyIniValues ();
				}
			}
		}

		if (typeChanged) {
			if (threadState.curType <= 4 && threadState.newType <= 4) {
				Playlist* p1 = *varsMusicType.at (threadState.curType);
				Playlist* p2 = *varsMusicType.at (threadState.newType);
				if (p1 == p2) {
					_MESSAGE ("Thread >> Tipe changed but playlist is the same");
					typeChanged = false;
					forceUpdateMusicType = true;
				}
			}
		}


		//Update the playing music
		//If (type changed) or (the special music changed while playing special music) or (no music is playing).
		if (threadState.TitlePlayed && playerIsQueuable) {
			if (performRequest || performCustom || swapSucceed
				|| (!playerIsPlaying && !threadState.HoldUntilMTChange && !threadState.noTrack)	//Ok, not playing, but if there's no track to play...
				|| (!typeChanged && newTypeSpecial && threadState.curSpecial != threadState.lastSpecial)	//Type not changed, but special track is coming...
				|| (typeChanged && ((!newTypeBattle && threadState.afterBattleTimer >= musicPlayer.getFinalAfterBattleDelay(false)) || (newTypeBattle && threadState.battleTimer >= musicPlayer.getFinalBattleDelay(false)))) //Type changed and (it's not a battle OR enough time passed since battle begun)
				) {
				if (threadState.newType != 65535) {
					_MESSAGE ("Thread >> Update music >> New/Current/Previous music types: %d/%d/%d, IsPlaying: %d, Hold: %d, Pause time: %.0f", threadState.newType, threadState.curType, threadState.prevType, playerIsPlaying, performHoldMusic, threadState.pauseTimer);
				}
				threadState.HoldUntilMTChange = false;
				if (MainThread_FixLevelUp (playerIsPlaying)) {
					typeChanged = false;
					newTypeBattle = (threadState.newType == MusicType::Battle);
					newTypeSpecial = (threadState.newType == MusicType::Special);
				}

				WaitForSingleObject (hThePlayerMutex, INFINITE);
				if (performCustom) {		//Play custom track
					MainThread_StorePreviousTrack (playerIsPlaying);
					MainThread_ResetBattleTimer ();
					MainThread_SelectCustomTrack ();
				} else if (!playerIsPlaying && (performHoldMusic || threadState.pauseTimer < musicPlayer.getFinalPauseTime(false))) { //Prevent the player from playing something.
					ReleaseMutex (hThePlayerMutex);
					if (!performHoldMusic) {
						if (threadState.pauseTimer <= 0) {
							_MESSAGE ("Thread >> Begin pause");
						}
						threadState.pauseTimer += SleepTime;
						if (threadState.pauseTimer >= musicPlayer.getFinalPauseTime (false)) {
							_MESSAGE ("Thread >> End pause");
						}
					}
					threadState.lastPlayedSet = false;
					Sleep (SleepTime);
					continue;
				} else if (threadState.lastPlayedSet && threadState.newType == threadState.prevType && !newTypeSpecial && threadState.curSpecial != SpecialMusicType::Success &&
					((threadState.restoreTimer <= musicPlayer.getMaxRestoreTime()) ||
					(threadState.curType == MusicType::Battle && !newTypeBattle && threadState.battleTimer < 60000))
					) {	//Resume the previous track 
					MainThread_RestorePreviousTrack ();
					//Or play a new one
				} else if (threadState.newType <= 255) {
					MainThread_StorePreviousTrack (playerIsPlaying);
					MainThread_ResetBattleTimer ();
					if (!(performRequest || swapSucceed)) {
						fadeOutTime = musicPlayer.getCurrentFadeOutLength ();
						fadeInTime = musicPlayer.getCurrentFadeInLength ();
					}
					MainThread_SelectNewTrack (fadeOutTime, fadeInTime);
				}
				ReleaseMutex (hThePlayerMutex);

				threadState.restoreTimer = 0;
				MainThread_UpdateMusicType (newTypeBattle);
				fadeOutTime = musicPlayer.getCurrentFadeOutLength ();
				fadeInTime = musicPlayer.getCurrentFadeInLength ();
				swapSucceed = false;
				forceUpdateMusicType = false;
			}
		}
		Sleep (SleepTime);
		WaitForSingleObject (hThePlayerMutex, INFINITE);
		musicPlayer.doPlayerStuff (SleepTime);
		ReleaseMutex (hThePlayerMutex);
		if (forceUpdateMusicType) {
			MainThread_UpdateMusicType (newTypeBattle);
			forceUpdateMusicType = false;
		}
	} while (!(mainGameVars && mainGameVars->gameVars && mainGameVars->gameVars->bHasQuitGame));
	_MESSAGE ("Thread >> Exited normally.");
	_endthread ();
}
