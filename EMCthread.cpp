#include "EMCthread.h"

#include <string>
#include <unordered_map>

#include "GameAPI.h"
#include "GameMenus.h"

#include "Globals.h"
#include "VanillaPlaylistData.h"
#include "MainGameVarsPointer.h"
#include "MusicState.h"
#include "MusicPlayer.h"
#include "MusicTimes.h"
#include "Multiplier.h"
#include "Playlist.h"
#include "ActivePlaylist.h"
#include "MusicInstance.h"
#include "ThreadRequest.h"
#include "IniData.h"
#include "OblivionINI.h"
#include "EMC2INISettings.h"
#include <chrono>
#include <ctime>
#include "DebugMode.h"



using namespace std;
InterfaceManager* intfc = nullptr;



#define checkLevelMenu(x) \
	if (intfc && intfc->activeMenu && intfc->activeMenu->id == 1027) { \
		if (!x) { \
			_EMCDEBUG ("%lld | LevelUp menu opened. MusicTypes: %d/%d", timeStamp, curMusicType, curSpecialType); \
			x = true; \
				} \
		} else if (x) { \
		_EMCDEBUG ("%lld | LevelUp menu closed. MusicTypes: %d/%d", timeStamp, curMusicType, curSpecialType); \
		x = false; \
	} \




MusicType curMusicType = MusicType::mtNotKnown;
MusicType lastMusicType = MusicType::mtNotKnown;
SpecialMusicType curSpecialType = SpecialMusicType::spNotKnown;
SpecialMusicType lastSpecialType = SpecialMusicType::spNotKnown;
bool deadMusicPlayed = false;
bool successMusicPlayed = false;
bool noTrackPlayed = false;
volatile bool *bMusicEnabled;
float *fMusicVolumeDup;
float *fMasterVolume;



void EMCthread (void *throwaway) {
	_MESSAGE ("Thread >> Start");

	//The amount of time the thread will sleep before processing again.
	float fVolume;
	bool playerIsReady = false;
	bool playerIsStopped = true;
	bool playerInPauseTime = false;
	bool musicChanged = false;
	bool reqNextTrack = false;
	bool reqCustomTrack = false;
	bool reqHoldMusicPlayer = false;
	int fadeInTime;
	int fadeOutTime;
	set<PendingPlaylist> &pendingPlaylists = threadRequest.getPendingPlaylists ();
	bool inLevelUpMenu = false;



	_MESSAGE ("Thread >> Initialization...");
	WaitForSingleObject (hThreadMutex, INFINITE);
	//Allow this thread to manipulate COM objects.
	_EMCDEBUG ("Initialize >> COM library");
	CoInitialize (nullptr);

	//Wait for MusicPlayer initialization...
	_EMCDEBUG ("Initialize >> music player");
	if (!musicPlayer.initialize ()) {
		return;
	}

	while (!mainGameVars->gameVars) Sleep (100);

	bHasQuitGame = &mainGameVars->gameVars->bHasQuitGame;
	fMusicVolumeDup = &mainGameVars->gameVars->SoundRecords->fMusicVolumeDup;
	fMasterVolume = &mainGameVars->gameVars->SoundRecords->fMasterVolume;
	bMusicEnabled = INI::Audio::bMusicEnabled;
	intfc = InterfaceManager::GetSingleton ();

	_EMCDEBUG ("Initialize >> Ini");
	//Read and apply the ini settings
	iniSettings.Initialize (INI_PATH, nullptr);
	iniSettings.applySettings ();

	_EMCDEBUG ("Initialize >> Music types remember maps");
	initMusicInstances ();

	//Wait for game initialization...
	_EMCDEBUG ("Initialize >> Vanilla volume settings");
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

	_EMCDEBUG ("Initialize >> Vanilla playlists");
	srand (std::time (0));
	apl_Explore.initialize (0, &getEmplaced (emplacePlaylistPaths (obExplore, obExplorePath, true, true)));
	apl_Public.initialize (1, &getEmplaced (emplacePlaylistPaths (obPublic, obPublicPath, true, true)));
	apl_Dungeon.initialize (2, &getEmplaced (emplacePlaylistPaths (obDungeon, obDungeonPath, true, true)));
	apl_Custom.initialize (3, &getEmplaced (emplacePlaylistPaths (obCustom, obCustomPath, true, true)));
	apl_Battle.initialize (4, &getEmplaced (emplacePlaylistPaths (obBattle, obBattlePath, true, true)));
	apl_Death.initialize (5, &getEmplaced (emplacePlaylistPaths (obDeath, obDeathPathExt, true, true)));
	apl_Success.initialize (6, &getEmplaced (emplacePlaylistPaths (obSuccess, obSuccessPathExt, true, true)));
	apl_Title.initialize (7, &getEmplaced (emplacePlaylistPaths (obTitle, obTitlePathExt, true, true)));



	fVolume = getMusicVolume ();
	musicPlayer.setMaxMusicVolume (fVolume);
	fadeOutTime = musicTimes.getFadeOut ();
	fadeInTime = musicTimes.getFadeIn ();
	_MESSAGE ("%lld | Thread >> Initialized\n\n\n", timeStamp);
	ReleaseMutex (hThreadMutex);

	for (; musicState.getSpecialType () != SpecialMusicType::spTitle; Sleep (SLEEP_TIME)) {
		if (*bHasQuitGame != 0) {
			return;
		}
	}

	playerInPauseTime = musicState.updateMusicState (playerIsStopped);
	lastMusicType = MusicType::mtSpecial;
	lastSpecialType = SpecialMusicType::spTitle;
	_MESSAGE ("%lld | Thread >> Title menu", timeStamp);



	for (; *bHasQuitGame == 0; Sleep (SLEEP_TIME)) {
		updateNow ();
		_EMCDEBUG2 ("\n%lld | Thread >> New loop", timeStamp);

		//Get the current state
		fVolume = getMusicVolume ();
		WaitForSingleObject (hMusicPlayerMutex, INFINITE);	//Get current player state
		musicPlayer.setMaxMusicVolume (fVolume);
		playerIsReady = musicPlayer.isReady ();
		playerIsStopped = musicPlayer.isStopped ();
		ReleaseMutex (hMusicPlayerMutex);

		WaitForSingleObject (hMusicStateMutex, INFINITE);
		checkLevelMenu (inLevelUpMenu)
			playerInPauseTime = musicState.updateMusicState (playerIsStopped);
		if (lastMusicType == MusicType::mtSpecial && lastSpecialType == SpecialMusicType::spTitle && !delayTitleMusicEnd) {
			if (musicState.getWorldType () < 8) {
				musicState.setEventType (MusicType::mtNotKnown);
				_MESSAGE ("%lld | Thread >> Don't delay title music ends", timeStamp);
			}
		}
		curMusicType = musicState.getCurrentMusicType ();
		curSpecialType = musicState.getSpecialType ();
		if (curMusicType != lastMusicType || curSpecialType != lastSpecialType) {
			_MESSAGE ("%lld | Thread >> Current music type: %d -> %d | %d -> %d", timeStamp, lastMusicType, curMusicType, lastSpecialType, curSpecialType);
		}
		ReleaseMutex (hMusicStateMutex);


		//Take all requests
		WaitForSingleObject (hThreadMutex, INFINITE);
		reqHoldMusicPlayer = threadRequest.hasRequestedHoldMusicPlayer ();
		if (threadRequest.hasRequestedCustomTrack ()) {
			reqCustomTrack = true;
			musicChanged = true;
			threadRequest.hasRequestedNextTrack ();		//Clear the request;
		} else if (threadRequest.hasRequestedNextTrack ()) {
			musicChanged = true;
			_EMCDEBUG ("%lld | Thread >> Request of next track changes the music", timeStamp);
		} else if (playerIsStopped && !deadMusicPlayed && !noTrackPlayed) {
			musicChanged = true;
			_EMCDEBUG ("%lld | Thread >> No track playing and no block. Time to select a new track", timeStamp);
		} else if (curMusicType == MusicType::mtSpecial && curSpecialType != lastSpecialType) {
			musicChanged = true;
			_EMCDEBUG ("%lld | Thread >> Special music type changed: %d -> %d", timeStamp, lastSpecialType, curSpecialType);
		} else if (curMusicType != lastMusicType && !samePlaylist (curMusicType, lastMusicType)) {
			musicChanged = true;
			_EMCDEBUG ("%lld | Thread >> Music type changed (%d -> %d) and playlist is different", timeStamp, lastMusicType, curMusicType);
		}

		//Do we need to swap some playlists?
		for (set<PendingPlaylist>::const_iterator it = pendingPlaylists.cbegin (); it != pendingPlaylists.cend ();) {
			if ((playerIsStopped && it->afterThisTrack) || it->targetTime >= now) {
				if (applyPendingPlaylist (*it, &fadeOutTime, &fadeInTime)) {
					musicChanged = true;
					_EMCDEBUG ("%lld | Thread >> Applying the pending playlist \"%s\" changes the music", timeStamp, it->playlist->name);
				}
				it = pendingPlaylists.erase (it);
			} else {
				it++;
			}
		}
		ReleaseMutex (hThreadMutex);



		//Update the playing music if music must change
		if (playerIsReady && musicChanged && !reqHoldMusicPlayer && !playerInPauseTime) {
			_MESSAGE ("%lld | Thread >> Update music. World music type: %d -> %d, Special music type: %d -> %d, Custom track: %d, Nexs track: %d, Is stopped: %d", timeStamp, lastMusicType, curMusicType, lastSpecialType, curSpecialType, reqCustomTrack, reqNextTrack, playerIsStopped);
			if (reqCustomTrack) {		//Play custom track
				playCustomTrack ();
				reqCustomTrack = false;
			} else {
				playPlaylistTrack (fadeOutTime, fadeInTime);
			}

			musicChanged = false;
			successMusicPlayed = false;
			lastMusicType = curMusicType;
			lastSpecialType = curSpecialType;
			_EMCDEBUG ("%lld | Thread >> World/Special music type have been updated: %d, %d", timeStamp, lastMusicType, lastSpecialType);
			WaitForSingleObject (hMusicTimesMutex, INFINITE);
			fadeOutTime = musicTimes.getFadeOut ();
			fadeInTime = musicTimes.getFadeOut ();
			ReleaseMutex (hMusicTimesMutex);
		}

		WaitForSingleObject (hMusicPlayerMutex, INFINITE);
		musicPlayer.updatePlayer (SLEEP_TIME);
		ReleaseMutex (hMusicPlayerMutex);

	}
	_MESSAGE ("%lld | Thread >> Exited normally", timeStamp);
	_endthread ();
}



float multipliersResult = 1;
//This is a function to be used by threadState Thread.
//It updates musicPlayer's volume with the volume from Oblivion.
//This ensures that the player can use Oblivion's audio controls to control the musicState.
float getMusicVolume () {
	float fVolume = 0;
	if (*bMusicEnabled) {
		//Get the volume, a value between 0 and 1.
		fVolume = (*fMusicVolumeDup) * (*fMasterVolume);
		if (recalculateMultipliers) {
			recalculateMultipliers = false;
			multipliersResult = 1.0;
			for (MultipliersMap::iterator it = multipliersCustom.begin (); it != multipliersCustom.end (); it++) {
				Multiplier& mult = it->second;
				WaitForSingleObject (mult.hThread, INFINITE);
				if (!mult.isDestroyed) {
					multipliersResult *= mult.getValue ();
				}
				ReleaseMutex (mult.hThread);
			}
			_MESSAGE ("%lld | Volume >> Multipliers recalculated: %f", timeStamp, multipliersResult);
		}
		fVolume *= multipliersResult;
	}
	_EMCDEBUG2 ("%lld | Volume >> Max volume: %f", timeStamp, fVolume);
	return fVolume;
}



bool applyPendingPlaylist (const PendingPlaylist& pendingPlaylist, int *fadeOutTime, int *fadeInTime) {
	bool changed = false;
	ActivePlaylist *aplToSwap = pendingPlaylist.aplToSwap;
	Playlist *playlist = pendingPlaylist.playlist;
	WaitForSingleObject (hMusicPlayerMutex, INFINITE);
	const string &trackPath = musicPlayer.getTrack ();
	ReleaseMutex (hMusicPlayerMutex);
	WaitForSingleObject (playlist->hMutex, INFINITE);
	if (playlist->isInitialized () && *aplToSwap != playlist) {
		*aplToSwap = playlist;
		changed = true;
		_MESSAGE ("%lld | Thread >> Set pending playlist: %s (%d) => %s", timeStamp, aplToSwap->name, aplToSwap->musicType, playlist->name);
		if (!playlist->restoreTrackIndex (trackPath)) {	//If it return false, then playlist doesn't have the current track, and we're forced to update.
			*fadeOutTime = pendingPlaylist.fadeOutTime;
			*fadeInTime = pendingPlaylist.fadeInTime;
			ReleaseMutex (playlist->hMutex);
			return true;
		}
	}
	ReleaseMutex (playlist->hMutex);
	return false;
}



bool playCustomTrack () {
	saveMusicInstance ();

	HANDLE handles[] = {hThreadMutex, hMusicTimesMutex, hMusicPlayerMutex};
	WaitForMultipleObjects (3, handles, 1, INFINITE);
	bool succeed = false;
	if (musicPlayer.queueTrack (threadRequest.getCustomTrack (), 0)) {
		succeed = musicPlayer.playQueuedTrack (FadeMethod::fmFadeOutThenIn, musicTimes.getFadeOut (), musicTimes.getFadeIn ());
	}
	threadRequest.clearCustomTrack ();
	ReleaseMutex (hThreadMutex);
	ReleaseMutex (hMusicTimesMutex);
	ReleaseMutex (hMusicPlayerMutex);

	if (succeed) {
		deadMusicPlayed = false;
		return true;
	}
	return false;
}



bool playPlaylistTrack (int fadeOut, int fadeIn) {
	if (musicPlayer.hasPlayedOnce ()) saveMusicInstance ();
	bool trackChosen = false;

	string playlistName;
	string trackPath;
	LONGLONG position = 0;


	_EMCDEBUG ("%lld | Thread >> Play playlist track | Fadeout/FadeIn: %d/%d", timeStamp, fadeOut, fadeIn);
	if (curMusicType != MusicType::mtSpecial) {
		selectedActivePlaylist = getActivePlaylist (curMusicType);
		deadMusicPlayed = false;
		if (curMusicType == MusicType::mtBattle) {
			fadeOut = fadeOutBattle;
			fadeIn = fadeInBattle;
		}
	} else {
		selectedActivePlaylist = getActivePlaylist (curSpecialType);
		deadMusicPlayed = (selectedActivePlaylist == &apl_Death);
		fadeOut = fadeIn = 500;
	}

	if (selectedActivePlaylist == &apl_NULL) {
		_MESSAGE ("%lld | Thread >> Unknown world/special music type: %d/%d", timeStamp, curMusicType, curSpecialType);
		noTrackPlayed = true;
		return false;
	} else if (getMusicInstance (curMusicType, &playlistName, &trackPath, &position)) {	//If a previous music instance can be recovered for this music type, then take it
		_MESSAGE ("%lld | Thread >> Resume previous track: %s, position: %.2f", timeStamp, trackPath.c_str (), (position / ONE_SECOND));
	} else {		//Create a new music instance, if possible...
		_EMCDEBUG ("%lld | Thread >> Play new track", timeStamp);
		Playlist* playlist = selectedActivePlaylist->playlist;
		WaitForSingleObject (playlist->hMutex, INFINITE);
		playlistName = playlist->name;
		trackPath = playlist->getNextTrack ();
		if (trackPath.empty ()) {
			_MESSAGE ("%lld | Thread >> No tracks for playlist %s", timeStamp, playlistName.c_str ());
			noTrackPlayed = true;
			return false;
		}
		noTrackPlayed = false;
		ReleaseMutex (playlist->hMutex);
		_MESSAGE ("%lld | Play next track from active %s playlist \"%s\" >> %s", timeStamp, selectedActivePlaylist->name, playlistName.c_str (), trackPath.c_str ());
	}


	if (musicPlayer.queueTrack (trackPath, position)) {
		if (musicPlayer.playQueuedTrack (FadeMethod::fmFadeOutThenIn, fadeOut, fadeIn)) {
			_MESSAGE ("%lld | Track correctly queued and started. It will play \"%s\" playlist > %s", timeStamp, selectedActivePlaylist->name, playlistName.c_str ());
			return true;
		} else {
			musicPlayer.stop (1000);
			_MESSAGE ("%lld | Error while playing", timeStamp);
			return false;
		}
	} else {
		musicPlayer.stop (1000);
		_MESSAGE ("%lld | Error while queuing", timeStamp);
		return false;
	}
}



void saveMusicInstance () {
	_MESSAGE ("%lld | Thread >> Save current music instance", timeStamp);
	if (selectedActivePlaylist != &apl_NULL) {
		Playlist *curPlaylist = selectedActivePlaylist->playlist;
		if (curPlaylist != nullptr) {

			WaitForSingleObject (hMusicPlayerMutex, INFINITE);
			const string &trackPath = musicPlayer.getTrack ();
			LONGLONG position = musicPlayer.getTrackPosition ();
			LONGLONG duration = musicPlayer.getTrackDuration ();
			ReleaseMutex (hMusicPlayerMutex);
			_EMCDEBUG ("%lld | Thread >> Current music instance: \"%s\", %f, %.2f", timeStamp, trackPath.c_str (), (position / ONE_SECOND), (duration / ONE_SECOND));

			WaitForSingleObject (curPlaylist->hMutex, INFINITE);
			if (curPlaylist->hasTrack (trackPath)) {
				saveMusicInstance (lastMusicType, curPlaylist->name, trackPath, position, duration);
			}
			ReleaseMutex (curPlaylist->hMutex);
		} else {
			_MESSAGE ("%lld | Thread >> No playlist to save", timeStamp);
		}
	} else {
		_MESSAGE ("%lld | Thread >> No active playlist to save", timeStamp);
	}
}