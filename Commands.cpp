#include "Commands.h"

#include <time.h>
#include <map>
#include <unordered_map>
#include "StringVar.h"
#include "ArrayVar.h"

#include "Globals.h"
#include "Multiplier.h"
#include "MusicType.h"
#include "MusicState.h"
#include "MusicPlayer.h"
#include "MusicTimes.h"
#include "Playlist.h"
#include "ActivePlaylist.h"
#include "ThreadRequest.h"
#include "FadeThread.h"
#include "PlayMusicFile.h"
#include "IniData.h"
#include "OBSEInterfaces.h"



using namespace std;




//No input
//Returns the MusicType Oblivion wants to be played.
bool Cmd_GetMusicType_Execute (COMMAND_ARGS) {
	int mode = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &mode)) {
		return true;
	}

	WaitForSingleObject (hMusicStateMutex, INFINITE);
		switch (mode) {
			case 0: *result = musicState.getCurrentMusicType (false); break;
			case 1: *result = musicState.getWorldType (); break;
			case 2: *result = musicState.getOverrideType (); break;
			case 3: *result = musicState.getSpecialType (); break;
			case 4: *result = musicState.isLocked() ? 1 : 0; break;
		}
	ReleaseMutex (hMusicStateMutex);

	Console_PrintC ("Current music type >> %.0f", *result);
	return true;
};





bool Cmd_SetMusicType_Execute (COMMAND_ARGS) {
	*result = 0;

	int musicType = 255;
	int locked = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &musicType, &locked)) {
		return true;
	}

	if (musicType > 4) {
		Console_PrintC ("Set music type >> Failed: music type is not valid > Type: %d, Lock: %d", musicType, locked);
		_MESSAGE ("Command >> emcSetMusicType >> Failed: music type is not valid > Type: %d, Lock: %d", musicType, locked);
		return true;
	}

	WaitForSingleObject (hMusicStateMutex, INFINITE);
		if (locked < 0 || musicType < 0) {
			musicState.overrideType (MusicType::mtNotKnown, false);
		} else {
			musicState.overrideType (MusicType(musicType), locked>0);
		}
	ReleaseMutex (hMusicStateMutex);

	Console_PrintC ("Set music type >> Type: %d, Lock: %d", musicType, locked);
	_MESSAGE ("Command >> emcSetMusicType >> Type: %d, Lock: %d", musicType, locked);
	*result = 1;
	return true;
}



bool Cmd_CreatePlaylist_Execute (COMMAND_ARGS) {
	//Specifically, if the playlist does allready exist (by name)
	//it will return 1.  Else, it will create it.  If it succeeds
	//in it's creation, it will return 1.  Otherwise, 0.
	*result = 0;

	//Parameters
	char pPlaylistName[512];
	char plPaths[512];
	int shuffle = 1;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &pPlaylistName, &plPaths, &shuffle)) {
		return true;
	}

	if (strlen(pPlaylistName) <= 0) {	//Empty name. Can't proceed
		Console_PrintC ("Playlist manager >> Failed: Name is empty");
		_MESSAGE ("Command >> emcCreatePlaylist >> Failed: Name is empty");
		return true;
	}

	const string &paths = plPaths;
			PlaylistEmplaceResult insResult = emplacePlaylist (pPlaylistName, false);
			if (insResult.second || insResult.first->second.isDestroyed()) {		//Create new playlist (destroyed playlists are considered as not existent)
				if (playlists.size () >= numPlaylists) {		//won't happend with destroyed playlists
					Console_PrintC ("Playlist manager >> Failed: Max playlists limit reached: %d", numPlaylists);
					_MESSAGE ("Command >> emcCreatePlaylist >> Failed: Max playlists limit reached: %d", numPlaylists);
					playlists.erase (insResult.first);
				} else if (insResult.first->second.setPaths (paths, shuffle != 0)) {
					Console_PrintC ("Create playlist >> Succeed (create)");
					_MESSAGE ("Command >> emcCreatePlaylist >> Succeed (create): \"%s\" >> %s", pPlaylistName, plPaths);
					*result = 1;
				} else {
					Console_PrintC ("Create playlist >> Failed: No track found.");
					_MESSAGE ("Command >> emcCreatePlaylist >> Failed: No track found for playlist \"%s\" in paths %s", pPlaylistName, plPaths);
				}
			} else {		//Alter/Delete existing playlist
				PlaylistsMap::iterator it = insResult.first;
				Playlist* playlist = &it->second;
				WaitForSingleObject (playlist->hMutex, INFINITE);
					if (playlist->isVanilla ()) {						//Playlist is vanilla
						Console_PrintC ("Playlist manager >> Failed: This is a vanilla playlist and can't be altered");
						_MESSAGE ("Command >> emcCreatePlaylist >> Failed: \"%s\" is a vanilla playlist and can't be altered", pPlaylistName);
					} else if (paths.empty ()) {	//Empty path -> destroy playlist
						ActivePlaylist* activePlaylist;
						while ((activePlaylist = getActivePlaylist (playlist)) != nullptr) {		//Playlist is assigned
							activePlaylist->restorePlaylist ();
						}
						playlist->destroy ();
						threadRequest.requestNextTrack (false);
						*result = 1;
					} else {						//Not empty path -> alter playlist
						const string &currentTrack = playlist->getCurrentTrack ();
						if (playlist->setPaths (paths, shuffle != 0)) {		//If recreation succeed...
							if (!currentTrack.empty () && isPlaylistActive (playlist)) {
								playlist->restoreTrackIndex (currentTrack);
							}
							Console_PrintC ("Recreate playlist >> Succeed (rebuild)");
							_MESSAGE ("Command >> emcCreatePlaylist >> Succeed (rebuild): \"%s\" >> %s", pPlaylistName, plPaths);
							*result = 1;
						} else {					//Recreation failed
							Console_PrintC ("Recreate playlist >> Failed: No tracks found");
							_MESSAGE ("Command >> emcCreatePlaylist >> Failed: No tracks found");
						}
					}
				ReleaseMutex (playlist->hMutex);
			}
	return true;
}





//Required Input: 2 Strings
//Returns 1 on success, 0 on failure.
bool Cmd_AddPathToPlaylist_Execute (COMMAND_ARGS) {
	//If the playlist does not exist, it will return 0.
	//If the recreation fails, it will return 0.
	//If everything goes to plan, it returns 1.
	*result = 0;

	//Parameters
	char pPlaylistName[512];
	char plPath[512];
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &pPlaylistName, &plPath)) {
		return true;
	}

	string playlistName = string (pPlaylistName);
	const string &path = plPath;
	if (playlistName.empty ()) {	//Empty name. Can't proceed
		Console_PrintC ("Playlist manager >> Failed: Name is empty");
		_MESSAGE ("Command >> emcAddPath >> Failed: Name is empty");
		return true;
	} else if (path.empty ()) {
		Console_PrintC ("Playlist manager >> Failed: Path is empty");
		_MESSAGE ("Command >> emcAddPath >> Failed: Path is empty for playlist \"%s\"", pPlaylistName);
		return true;
	}

	PlaylistsMap::iterator it = playlists.find (playlistName);
	if (it == playlists.end () || it->second.isDestroyed()) {
		Console_PrintC ("Add path >> Failed: Playlist does not exist");
		_MESSAGE ("Command >> emcAddPath >> Failed: Playlist does not exist");
	} else {
		Playlist* playlist = &it->second;
		WaitForSingleObject (playlist->hMutex, INFINITE);
			//Make sure its not one of the default playlists.
			if (playlist->isVanilla ()) {
				Console_PrintC ("Add path >> Failed: This is a vanilla playlist and can't be altered");
				_MESSAGE ("Command >> emcAddPath >> Failed: \"%s\" is a vanilla playlist and can't be altered", pPlaylistName);
			} else {
				const string &currentTrack = playlist->getCurrentTrack ();
				if (playlist->addPath (path)) {		//Try to add the new path
					if (!currentTrack.empty () && isPlaylistActive (playlist)) {
						playlist->restoreTrackIndex (currentTrack);
					}
					Console_PrintC ("Add path >> Succeed: Playlist updated", pPlaylistName, plPath);
					_MESSAGE ("Command >> emcAddPath >> Succeed: Playlist updated: \"%s\" >> %s", pPlaylistName, plPath);
					*result = 1;
				} else {			//Addition failed.
					Console_PrintC ("Add path >> Failed: No track found", pPlaylistName, plPath);
					_MESSAGE ("Command >> emcAddPath >> Failed: No track found for playlist \"%s\" in %s", pPlaylistName, plPath);
				}
			}
		ReleaseMutex (playlist->hMutex);
	}
	return true;
}





//Required Input: 1 String
//Returns 1 is the playlist was found, else 0.
bool Cmd_PlaylistExists_Execute (COMMAND_ARGS) {
	//result == 1 if exists, 0 otherwise.
	*result = 0;

	//Parameters
	char pPlaylistName[512];
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &pPlaylistName)) {
		return true;
	}

	if (strlen(pPlaylistName) > 0) {
		PlaylistsMap::iterator pos = playlists.find (string (pPlaylistName));
		if (pos != playlists.end () && (pos->second.isVanilla() || pos->second.isInitialized ())) {
			*result = 1;
		}
	}
	Console_PrintC ("Playlist exists >> %.0f", *result);
	return true;
}





//Required Input: 1 String
//Returns 1 is the playlist is assigned to a MusicType, else 0.
bool Cmd_IsPlaylistActive_Execute (COMMAND_ARGS) {
	//result == 1 if it is an active playlist, 0 otherwise.
	*result = 0;

	//Parameters
	char pPlaylistName[512];
	int musicType = -1;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &pPlaylistName, &musicType)) {
		return true;
	}

	if (musicType < 0) {
		if (isPlaylistActive (pPlaylistName)) {
			*result = 1;
			Console_PrintC ("Playlist is active");
		} else {
			Console_PrintC ("Playlist is not active");
		}
	} else if (musicType <= 4) {
		if (strcmp (pPlaylistName, activePlaylists[musicType]->playlist->name) != 0) {
			*result = 1;
			Console_PrintC ("Playlist is assigned to music type %d", musicType);
		} else {
			Console_PrintC ("Playlist is not assigned to music type %d", musicType);
		}
	} else {
		Console_PrintC ("Music type %d is not valid", musicType);
	}
	return true;
}





bool Cmd_SetPlaylist_Execute (COMMAND_ARGS) {
	*result = 0;

	WaitForSingleObject (hMusicPlayerMutex, INFINITE);
	bool isReady = musicPlayer.isReady ();
	ReleaseMutex (hMusicPlayerMutex);
	if (!isReady) {
		return true;
	}

	//Parameters
	int pMusicType = 0;
	char pPlaylistName[512];
	int pQueueMode = 0;
	float pDelay = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &pMusicType, &pPlaylistName, &pQueueMode, &pDelay)) {
		return true;
	}

	string playlistName = string (pPlaylistName);
	if (playlistName.empty ()) {
		Console_PrintC ("Set playlist >> Failed: Name is empty");
		_MESSAGE ("Command >> emcSetPlaylist >> Failed: Name is empty");
		return true;
	} else if (pMusicType > 4) {
		Console_PrintC ("Set playlist >> Failed: Music type doesn't exists");
		_MESSAGE ("Command >> emcSetPlaylist >> Failed: Music type %d doesn't exists", pMusicType);
		return true;
	}

	PlaylistsMap::iterator it = playlists.find (playlistName);
	if (it == playlists.end () || it->second.isDestroyed ()) {
		Console_PrintC ("Set playlist >> Failed: Playlist does not exist: \"%s\"", pPlaylistName);
		_MESSAGE ("Command >> emcSetPlaylist >> Failed: Playlist does not exist: \"%s\"", pPlaylistName);
		return true;
	}

	ActivePlaylist* apl;
	if (pMusicType >= 0) {
		//Let other threads know the playlists are being manipulated.
		MusicType musicType = static_cast<MusicType>(pMusicType);
		apl = getActivePlaylist (musicType);		//Can't return nullptr, as 0 <= pMusicType <= 4
	} else {
		apl = selectedActivePlaylist;
	}

	Playlist* playlist = &it->second;
	if (apl->playlist != playlist || pQueueMode != 0 || pDelay > 0) {
		threadRequest.requestSetPlaylist (apl, playlist, pQueueMode != 0, pDelay * 1000);
		Console_PrintC ("Set playlist >> Succeed: Target: %d, Name: \"%s\", Mode: %d, Delay: %f", pMusicType, pPlaylistName, pQueueMode, pDelay);
		_MESSAGE ("Command >> emcSetPlaylist >> Succeed: Target: %d, Name: \"%s\", Mode: %d, Delay: %f", pMusicType, pPlaylistName, pQueueMode, pDelay);
		*result = 1;
	} else {
		Console_PrintC ("Set playlist >> Failed: Target: %d, Name: \"%s\", Mode: %d, Delay: %f", pMusicType, pPlaylistName, pQueueMode, pDelay);
		_MESSAGE ("Command >> emcSetPlaylist >> Failed: Target: %d, Name: \"%s\", Mode: %d, Delay: %f", pMusicType, pPlaylistName, pQueueMode, pDelay);
	}
	return true;
}





bool Cmd_RestorePlaylist_Execute (COMMAND_ARGS) {
	*result = 0;

	WaitForSingleObject (hMusicPlayerMutex, INFINITE);
		bool isReady = musicPlayer.isReady ();
	ReleaseMutex (hMusicPlayerMutex);
	if (!isReady) {
		return true;
	}

	//Parameters
	int pMusicType = -1;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &pMusicType)) {
		return true;
	}

	WaitForSingleObject (hMusicStateMutex, INFINITE);
		MusicType currentMusicType = musicState.getCurrentMusicType (false);
	ReleaseMutex (hMusicStateMutex);
	if (pMusicType < 0) {		//Restore all.
		bool restored = false;
		ActivePlaylist* activePlaylist;
		for (int i = 0; i <= 4; i++) {
			activePlaylist = getActivePlaylist (static_cast<MusicType>(i));
			if (activePlaylist->restorePlaylist ()){
				restored = true;
			}
		}

		if (restored) {
			threadRequest.requestNextTrack (false);
			*result = 1;
			Console_PrintC ("Restore playlist >> Succeed: All music types restored");
			_MESSAGE ("Command >> emcRestorePlaylist >> Succeed: All music types restored to default playlist");
		} else {
			Console_PrintC ("Restore playlist >> Failed: All music types are already using their default playlists");
			_MESSAGE ("Command >> emcRestorePlaylist >> Failed: All music types are already using their default playlists");
		}
	} else if (pMusicType <= 4) {
		ActivePlaylist* activePlaylist = getActivePlaylist (static_cast<MusicType>(pMusicType));
		if (activePlaylist->restorePlaylist ()) {
			if (currentMusicType == pMusicType) {
				threadRequest.requestNextTrack (false);
			}
			*result = 1;
			Console_PrintC ("Restore playlist >> Succeed: Music type restored");
			_MESSAGE ("Command >> emcRestorePlaylist >> Succeed: Music type %d restored to default playlist", pMusicType);
		} else {
			Console_PrintC ("Restore playlist >> Failed: Music type %d is already using the default playlist", pMusicType);
			_MESSAGE ("Command >> emcRestorePlaylist >> Failed: Music type %d is already using the default playlist", pMusicType);
		}
	} else {
		Console_PrintC ("Restore playlist >> Failed: Invalid music type");
		_MESSAGE ("Command >> emcRestorePlaylist >> Failed: Invalid music type %d", pMusicType);
	}
	
	return true;
}






bool Cmd_IsMusicSwitching_Execute (COMMAND_ARGS) {
	*result = musicPlayer.isReady () ? 0 : 1;
	Console_PrintC ("Music switching >> %.0f", *result);
	return true;
}





bool Cmd_GetAllPlaylists_Execute (COMMAND_ARGS) {
	*result = 0;
	int pOnlyActive = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &pOnlyActive)) {
		return true;
	}

	bool console = inConsole;
	const char* playlistName;
	OBSEArray* newArray = g_arrayIntfc->CreateArray (nullptr, 0, scriptObj);
		if (pOnlyActive == 0) {
			for (PlaylistsMap::const_iterator it = playlists.cbegin (); it != playlists.cend (); it++) {
				playlistName = (it->second).name;
				g_arrayIntfc->AppendElement (newArray, playlistName);
				if (console) Console_Print (playlistName);
			}
		} else {
			for (ActivePlaylist* activePlaylist : activePlaylists) {
				playlistName = activePlaylist->playlist->name;
				g_arrayIntfc->AppendElement (newArray, playlistName);
				if (console) Console_Print ("%s -> %s", activePlaylist->name, playlistName);
			}
		}
	g_arrayIntfc->AssignCommandResult (newArray, result);
	return true;
}





bool Cmd_GetPlaylist_Execute (COMMAND_ARGS) {
	*result = 0;

	int pMusicType = -1;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &pMusicType)) {
		return true;
	}

	if (pMusicType > 4) {
		Console_PrintC ("Invalid music type");
		return true;
	}

	ActivePlaylist* activePlaylist;
	if (pMusicType < 0) {
		activePlaylist = selectedActivePlaylist;
	} else {
		activePlaylist = getActivePlaylist (static_cast<MusicType>(pMusicType));
	}

	if (activePlaylist != nullptr) {
		const char* playlistName = activePlaylist->playlist->name;
		g_stringIntfc->Assign (PASS_COMMAND_ARGS, playlistName);
		Console_PrintC ("Playlist >> %s", playlistName);
	} else {
		Console_PrintC ("No playlist assigned to type %d", pMusicType);
	}
	return true;
}





bool Cmd_GetPlaylistTracks_Execute (COMMAND_ARGS) {
	*result = 0;

	char pPlaylistName[512];
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &pPlaylistName)) {
		return true;
	}
	
	Playlist* playlist = nullptr;
	string playlistName = string (pPlaylistName);
	
	if (playlistName.empty ()) {
		playlist = selectedActivePlaylist->playlist;
	} else {
		PlaylistsMap::iterator it = playlists.find (playlistName);
		if (it != playlists.end ()) {
			playlist = &it->second;
		}
	}
	if (playlist != nullptr) {
		OBSEArray* newArray = g_arrayIntfc->CreateArray (nullptr, 0, scriptObj);
		for (const string& track : playlist->getTracks ()) {
			g_arrayIntfc->AppendElement (newArray, track.c_str ());
		}
		g_arrayIntfc->AssignCommandResult (newArray, result);
		if (inConsole) {
			playlist->printTracks ();
		}
	} else {
		Console_PrintC ("Playlist not found");
	}
	return true;
}





bool Cmd_GetTrack_Execute (COMMAND_ARGS) {
	const char* songPath = musicPlayer.getTrack ().c_str();
	Console_PrintC ("Currently track >> %s", songPath);
	g_stringIntfc->Assign (PASS_COMMAND_ARGS, songPath);
	return true;
}





bool Cmd_GetTrackDuration_Execute (COMMAND_ARGS) {
	*result = musicPlayer.getTrackDuration () / ONE_SECOND;
	Console_PrintC ("Track duration >> %f", *result);
	return true;
}





bool Cmd_GetTrackPosition_Execute (COMMAND_ARGS) {
	*result = musicPlayer.getTrackPosition () / ONE_SECOND;
	double duration = musicPlayer.getTrackDuration () / ONE_SECOND;
	Console_PrintC ("Track position >> %.0f / %.0f", *result, duration);
	return true;
}





bool Cmd_SetTrackPosition_Execute (COMMAND_ARGS) {
	*result = 0;
	float pPosition = 0;
	int pFadeOut = 1;
	int pFadeIn = 1;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &pPosition, &pFadeOut, &pFadeIn)) {
		return true;
	}

	pFadeOut *= 1000;
	pFadeIn *= 1000;
	if (pPosition < 0) pPosition = 0;

	WaitForSingleObject (hMusicPlayerMutex, INFINITE);
		LONGLONG longPos = pPosition * ONE_SECOND;
		LONGLONG longDur = musicPlayer.getTrackDuration ();
		if (longPos > longDur - ONE_SECOND) {
			longPos = longDur - ONE_SECOND;
		}
		musicPlayer.setTrackPosition (longPos, pFadeOut, pFadeIn);
	ReleaseMutex (hMusicPlayerMutex);

	Console_PrintC ("Set track position >> Position %.2f, Fade out/in: %f/%f", longPos, pFadeOut, pFadeIn);
	_MESSAGE ("Command >> emcSetTrackPosition >> Position %.2f, Fade out/in: %f/%f", longPos, pFadeOut, pFadeIn);
	*result = 1;
	return true;
}





//Required input: None
//Returns 1 if the override is active, else 0.
bool Cmd_IsBattleOverridden_Execute (COMMAND_ARGS) {
	*result = musicState.isBattleOverridden() ? 1 : 0;
	Console_PrintC ("Battle overridden >> %.0f", *result);
	return true;
}





//Required input: None
//Returns 1 if the override was set, else 0.
bool Cmd_SetBattleOverride_Execute (COMMAND_ARGS) {
	*result = 0;
	int battleOverride = -1;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &battleOverride)) {
		return true;
	}

	bool battleOverrideBool = (battleOverride != 0);
	WaitForSingleObject (hMusicStateMutex, INFINITE);
		if (musicState.setBattleOverride (battleOverrideBool)) {
			Console_PrintC ("Set battle override >> %d", battleOverrideBool);
			_MESSAGE ("Command >> emcSetBattleOverride >> %d", battleOverrideBool);
			*result = 1;
		} else {
			Console_PrintC ("Set battle override >> %d (unchanged)", battleOverrideBool);
		}
	ReleaseMutex (hMusicStateMutex);
	return true;
}





bool Cmd_IsMusicOnHold_Execute (COMMAND_ARGS) {
	*result = threadRequest.hasRequestedHoldMusicPlayer () ? 1 : 0;
	Console_PrintC ("Hold music >> %.0f", *result);
	return true;
}





bool Cmd_SetMusicHold_Execute (COMMAND_ARGS) {
	*result = 0;
	int pHoldMusic;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &pHoldMusic)) {
		return true;
	}

	bool holdMusic = (pHoldMusic != 0);
	if (threadRequest.requestHoldMusicPlayer (holdMusic)) {
		Console_PrintC ("Set music hold >> %d", holdMusic);
		_MESSAGE ("Command >> emcSetMusicHold >> %d", holdMusic);
		*result = 1;
	} else {
		Console_PrintC ("Set music hold >> %d (unchanged)", holdMusic);
	}
	return true;
}





bool Cmd_GetMasterVolume_Execute (COMMAND_ARGS) {
	*result = 0;
	int method = 0;
	int target = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &method, &target)) {
		return true;
	}

	Multiplier *mult = (method == 0) ? &multObMaster : &multObMasterIni;
	WaitForSingleObject (mult->hThread, INFINITE);
		switch (target) {
			case 1: *result = mult->isFading ? mult->startValue : mult->getValue (); break;
			case 2: *result = mult->isFading ? mult->targetValue : mult->getValue (); break;
			case 3: *result = mult->isFading ? mult->fadeTime : -1; break;
			default: *result = mult->getValue ();
		}
	ReleaseMutex (mult->hThread);
	Console_PrintC ("Master volume >> %f", *result);
	return true;
}





bool Cmd_SetMasterVolume_Execute (COMMAND_ARGS) {
	*result = 0;
	float volume = 0;
	int method = 0;
	float fadeTime = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &volume, &method, &fadeTime)) {
		return true;
	}

	Multiplier *mult = (method == 0) ? &multObMaster : &multObMasterIni;
	WaitForSingleObject (mult->hThread, INFINITE);
		Console_PrintC ("Set master volume >> Set/Fade volume from %f to %f, Method: %d, Time: %f", mult->getValue (), volume, method, fadeTime);
		_MESSAGE ("Command >> emcSetMasterVolume >> Set/Fade volume from %f to %f, Method: %d, Time: %f", mult->getValue (), volume, method, fadeTime);
		mult->fadeVolume (volume, fadeTime);
	ReleaseMutex (mult->hThread);
	*result = 1;
	return true;
}





bool Cmd_GetMusicVolume_Execute (COMMAND_ARGS) {
	*result = 0;
	int method = 0;
	int target = 0;
	char key[MAX_PATH];
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &method, &target, &key)) {
		return true;
	}

	Multiplier* mult;
	if (method < 2) {
		mult = (method == 0) ? &multObMusic : &multObMusicIni;
	} else {
		MultipliersMap::iterator pos = multipliersCustom.find (key);
		if (pos != multipliersCustom.end ()) {
			mult = &pos->second;
		} else {
			*result = -1.0;
			Console_PrintC ("This multiplier doesn't exists");
			return true;
		}
	}

	WaitForSingleObject (mult->hThread, INFINITE);
		switch (target) {
			case 1: *result = mult->isFading ? mult->startValue : mult->getValue (); break;
			case 2: *result = mult->isFading ? mult->targetValue : mult->getValue (); break;
			case 3: *result = mult->isFading ? mult->fadeTime : -1; break;
			case 4: *result = (method < 2 || mult->saveGame) ? 1 : 0; break;
			case 5: *result = (method < 2 || mult->saveSession) ? 1 : 0; break;
			default: *result = mult->getValue ();
		}
	ReleaseMutex (mult->hThread);

	Console_PrintC ("Music multiplier >> %f", *result);
	return true;
}





bool Cmd_SetMusicVolume_Execute (COMMAND_ARGS) {
	*result = 0;
	float volume = 1.0;
	int method = 0;
	float fadeTime = 0;

	char pKey[MAX_PATH];
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &volume, &method, &fadeTime, &pKey)) {
		return true;
	}

	Multiplier* mult = nullptr;
	if (method < 2) {
		mult = (method == 0) ? &multObMusic : &multObMusicIni;
		WaitForSingleObject (mult->hThread, INFINITE);
			Console_PrintC ("Set music volume >> Set/Fade volume from %f to %f, Method: %d, Time: %f", mult->getValue (), volume, method, fadeTime);
			_MESSAGE ("Command >> emcSetMusicVolume >> Set/Fade volume from %f to %f, Method: %d, Time: %f", mult->getValue (), volume, method, fadeTime);
			mult->fadeVolume (volume, fadeTime);
		ReleaseMutex (mult->hThread);
	} else {
		string key = string (pKey);
		if (key.empty ()) {
			Console_PrintC ("Set music volume >> Can't create/destroy a custom multiplier without a key");
			return true;
		}

		_MESSAGE ("Command >> emcSetMusicVolume >> Method: %d, Key: %s", method, pKey);
		if (volume == 1.0) {
			MultipliersMap::iterator pos = multipliersCustom.find (key);
			if (pos == multipliersCustom.end () || pos->second.isDestroyed) {
				Console_PrintC ("Set music volume >> Can't create a new multiplier with volume 1.0");
				_MESSAGE ("Command >> emcSetMusicVolume >> Can't create a new multiplier with volume 1.0");
				return true;
			} else if (fadeTime <= 0) {
				mult = &pos->second;
				WaitForSingleObject (mult->hThread, INFINITE);
					mult->isDestroyed = true;
				ReleaseMutex (mult->hThread);
				Console_PrintC ("Set music volume >> Multiplier destroyed");
				_MESSAGE ("Command >> emcSetMusicVolume >> Multiplier destroyed");
				*result = 1;
				return true;
			}
		} else {
			MultiplierEmplaced insResult = multipliersCustom.emplace (constructInPlace (key, 1.0));	//If not present add it... in one shot, for best performance
			mult = &insResult.first->second;
			if (insResult.second || mult->isDestroyed) {
				if (multipliersCustom.size () > numMultipliers) {	//The capacity is (numMultipliers + 1)... the +1 avoid to reallocate the map when emplace surpass the limit. Still, it should be hard to reach...
					Console_PrintC ("Set music volume >> Failed: Max multipliers limit reached: %d", numMultipliers);
					_MESSAGE ("Command >> emcSetMusicVolume >> Failed: Max multipliers limit reached: %d", numMultipliers);
					multipliersCustom.erase (insResult.first);	//Remove it, so we will fall <= numMultipliers
					return true;
				}
				Console_PrintC ("Set music volume >> Create new multiplier");
				_MESSAGE ("Command >> emcSetMusicVolume >> Create new multiplier");
			}
		}

		Console_PrintC ("Set music volume >> Set/Fade multiplier \"%s\" from %f to %f Time: %f", pKey, mult->getValue (), volume, fadeTime);
		_MESSAGE ("Command >> emcSetMusicVolume >> Set/Fade multiplier \"%s\" from %f to %f Time: %f", pKey, mult->getValue (), volume, fadeTime);
		WaitForSingleObject (mult->hThread, INFINITE);
			mult->saveGame = (method == 3);
			mult->saveSession = (method == 4);
			mult->isDestroyed = false;
			mult->fadeVolume (volume, fadeTime);
		ReleaseMutex (mult->hThread);
	}

	*result = 1;
	return true;
}





bool Cmd_GetEffectsVolume_Execute (COMMAND_ARGS) {
	*result = 0;
	int method = 0;
	int target = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &method, &target)) {
		return true;
	}

	Multiplier *mult = (method == 0) ? &multObEffects : &multObEffectsIni;
	WaitForSingleObject (mult->hThread, INFINITE);
		switch (target) {
			case 1: *result = mult->isFading ? mult->startValue : mult->getValue (); break;
			case 2: *result = mult->isFading ? mult->targetValue : mult->getValue (); break;
			case 3: *result = mult->isFading ? mult->fadeTime : -1; break;
			default: *result = mult->getValue ();
		}
	ReleaseMutex (mult->hThread);

	Console_PrintC ("Effects volume >> %f", *result);
	return true;
}





bool Cmd_SetEffectsVolume_Execute (COMMAND_ARGS) {
	*result = 0;
	float volume = 0;
	int method = 0;
	float fadeTime = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &volume, &method, &fadeTime)) {
		return true;
	}

	Multiplier *mult = (method == 0) ? &multObEffects : &multObEffectsIni;
	WaitForSingleObject (mult->hThread, INFINITE);
		Console_PrintC ("Set effects volume >> Set/Fade volume from %f to %f, Method: %d, Time: %f", mult->getValue (), volume, method, fadeTime);
		_MESSAGE ("Command >> emcSetEffectsVolume >> Set/Fade volume from %f to %f, Method: %d, Time: %f", mult->getValue (), volume, method, fadeTime);
		mult->fadeVolume (volume, fadeTime);
	ReleaseMutex (mult->hThread);

	*result = 1;
	return true;
}





bool Cmd_GetFootVolume_Execute (COMMAND_ARGS) {
	*result = 0;
	int method = 0;
	int target = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &method, &target)) {
		return true;
	}

	Multiplier *mult = (method == 0) ? &multObFoot : &multObFootIni;
	WaitForSingleObject (mult->hThread, INFINITE);
		switch (target) {
			case 1: *result = mult->isFading ? mult->startValue : mult->getValue (); break;
			case 2: *result = mult->isFading ? mult->targetValue : mult->getValue (); break;
			case 3: *result = mult->isFading ? mult->fadeTime : -1; break;
			default: *result = mult->getValue ();
		}
	ReleaseMutex (mult->hThread);

	Console_PrintC ("Foot volume >> %f", *result);
	return true;
}





bool Cmd_SetFootVolume_Execute (COMMAND_ARGS) {
	*result = 0;
	float volume = 0;
	int method = 0;
	float fadeTime = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &volume, &method, &fadeTime)) {
		return true;
	}

	Multiplier *mult = (method == 0) ? &multObFoot : &multObFootIni;
	WaitForSingleObject (mult->hThread, INFINITE);
		Console_PrintC ("Set foot volume >> Set/Fade volume from %f to %f, Method: %d, Time: %f", mult->getValue (), volume, method, fadeTime);
		_MESSAGE ("Command >> emcSetFootVolume >> Set/Fade volume from %f to %f, Method: %d, Time: %f", mult->getValue (), volume, method, fadeTime);
		mult->fadeVolume (volume, fadeTime);
	ReleaseMutex (mult->hThread);

	*result = 1;
	return true;
}





bool Cmd_GetVoiceVolume_Execute (COMMAND_ARGS) {
	*result = 0;
	int method = 0;
	int target = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &method, &target)) {
		return true;
	}

	Multiplier *mult = (method == 0) ? &multObVoice : &multObVoiceIni;
	WaitForSingleObject (mult->hThread, INFINITE);
		switch (target) {
			case 1: *result = mult->isFading ? mult->startValue : mult->getValue (); break;
			case 2: *result = mult->isFading ? mult->targetValue : mult->getValue (); break;
			case 3: *result = mult->isFading ? mult->fadeTime : -1; break;
			default: *result = mult->getValue ();
		}
	ReleaseMutex (mult->hThread);

	Console_PrintC ("Voice volume >> %f", *result);
	return true;
}





bool Cmd_SetVoiceVolume_Execute (COMMAND_ARGS) {
	*result = 0;
	float volume = 0;
	int method = 0;
	float fadeTime = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &volume, &method, &fadeTime)) {
		return true;
	}

	Multiplier *mult = (method == 0) ? &multObVoice : &multObVoiceIni;
	WaitForSingleObject (mult->hThread, INFINITE);
		Console_PrintC ("Set voice volume >> Set/Fade volume from %f to %f, Method: %d, Time: %f", mult->getValue (), volume, method, fadeTime);
		_MESSAGE ("Command >> emcSetVoiceVolume >> Set/Fade volume from %f to %f, Method: %d, Time: %f", mult->getValue (), volume, method, fadeTime);
		mult->fadeVolume (volume, fadeTime);
	ReleaseMutex (mult->hThread);

	*result = 1;
	return true;
}





bool Cmd_GetMusicSpeed_Execute (COMMAND_ARGS) {
	*result = musicPlayer.getMusicSpeed ();
	Console_PrintC ("Music speed >> %f", *result);
	return true;
}





bool Cmd_SetMusicSpeed_Execute (COMMAND_ARGS) {
	*result = 0;
	float speed;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &speed)) {
		return true;
	}

	if (speed <= 0.1) {
		speed = 0.1;
	} else if (speed > 100) {
		speed = 100;
	}

	WaitForSingleObject (hMusicPlayerMutex, INFINITE);
		if (musicPlayer.setMusicSpeed ((double)speed)) {
			_MESSAGE ("Command >> emcSetMusicSpeed >> Speed: %f", speed);
			*result = 1;
		}
	ReleaseMutex (hMusicPlayerMutex);

	Console_PrintC ("Set music speed >> Speed: %f", speed);
	return true;
}





bool Cmd_GetFadeTime_Execute (COMMAND_ARGS) {
	*result = 0;
	int fadeIn = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &fadeIn)) {
		return true;
	}

	WaitForSingleObject (hMusicTimesMutex, INFINITE);
		if (fadeIn != 0) {
			*result = musicTimes.getFadeIn () / 1000;
		} else {
			*result = musicTimes.getFadeOut () / 1000;
		}
	ReleaseMutex (hMusicTimesMutex);

	Console_PrintC ("Fade time >> %f", *result);
	return true;
}





bool Cmd_SetFadeTime_Execute (COMMAND_ARGS) {
	*result = 0;
	float fadeTime = 0;
	int fadeIn = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &fadeTime, &fadeIn)) {
		return true;
	}
	if (fadeTime < 0) {
		fadeTime = 0;
	}

	WaitForSingleObject (hMusicTimesMutex, INFINITE);
		if (fadeIn != 0) {
			if (musicTimes.setFadeIn (fadeTime * 1000.0)) {
				_MESSAGE ("Command >> emcSetFadeTime >> Fade In: %f", fadeTime);
				*result = 1;
			}
			Console_PrintC ("Set fade time >> Fade In: %f", fadeTime);
		} else {
			if (musicTimes.setFadeOut (fadeTime * 1000.0)) {
				_MESSAGE ("Command >> emcSetFadeTime >> Fade Out: %f", fadeTime);
				*result = 1;
			}
			Console_PrintC ("Set fade time >> Fade Out: %f", fadeTime);
		}
	ReleaseMutex (hMusicTimesMutex);
	return true;
}





bool Cmd_GetPauseTime_Execute (COMMAND_ARGS) {
	*result = 0;
	int extraPause = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &extraPause)) {
		return true;
	}

	switch (extraPause) {
		case 0: *result = (musicTimes.getMinPauseTime () / 1000); break;
		case 1: *result = (musicTimes.getExtraPauseTime () / 1000); break;
		default: *result = (musicTimes.getPauseTime() / 1000);
	}

	Console_PrintC ("Pause time >> %f", *result);
	return true;
}





bool Cmd_SetPauseTime_Execute (COMMAND_ARGS) {
	*result = 0;
	float pauseTime = 0;
	float extraPauseTime = 0;
	int forceUpdate = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &pauseTime, &extraPauseTime, &forceUpdate)) {
		return true;
	}
	if (pauseTime < 0) {
		pauseTime = 0;
	}
	if (extraPauseTime < 0) {
		extraPauseTime = 0;
	}

	WaitForSingleObject (hMusicTimesMutex, INFINITE);
		bool changed1 = musicTimes.setMinPauseTime (pauseTime * 1000);
		bool changed2 = musicTimes.setExtraPauseTime (extraPauseTime * 1000);
		if (changed1 || changed2 || forceUpdate != 0) {
			musicTimes.recalculatePauseTime ();
			_MESSAGE ("Command >> emcSetPauseTime >> Min pause: %f, Extra pause: %f", pauseTime, extraPauseTime);
			*result = 1;
		}
	ReleaseMutex (hMusicTimesMutex);

	Console_PrintC ("Set pause time >> Min pause: %f, Extra pause: %f", pauseTime, extraPauseTime);
	return true;
}





bool Cmd_GetStartBattleDelay_Execute (COMMAND_ARGS) {
	*result = 0;
	int battleDelayData = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &battleDelayData)) {
		return true;
	}

	switch (battleDelayData) {
		case 0: *result = (musicTimes.getMinStartBattleDelay () / 1000); break;
		case 1: *result = (musicTimes.getExtraStartBattleDelay () / 1000); break;
		default: *result = (musicTimes.getStartBattleDelay () / 1000);
	}

	Console_PrintC ("Start battle delay >> %f", *result);
	return true;
}





bool Cmd_SetStartBattleDelay_Execute (COMMAND_ARGS) {
	*result = 0;
	float minDelay = 0;
	float extraDelay = 0;
	int forceUpdate = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &minDelay, &extraDelay, &forceUpdate)) {
		return true;
	}
	if (minDelay < 0.5) {
		minDelay = 0.5;
	}
	if (extraDelay < 0) {
		extraDelay = 0;
	}

	WaitForSingleObject (hMusicTimesMutex, INFINITE);
		bool changed1 = musicTimes.setMinStartBattleDelay (minDelay * 1000);
		bool changed2 = musicTimes.setExtraStartBattleDelay (extraDelay * 1000);
		if (changed1 || changed2 || forceUpdate != 0) {
			musicTimes.recalculateStartBattleDelay ();
			_MESSAGE ("Command >> emcSetStartBattleDelay >> Min delay: %f, Extra delay: %f", minDelay, extraDelay);
			*result = 1;
		}
	ReleaseMutex (hMusicTimesMutex);

	Console_PrintC ("Set start battle delay >> Min delay: %f, Extra delay: %f", minDelay, extraDelay);
	return true;
}





bool Cmd_GetStopBattleDelay_Execute (COMMAND_ARGS) {
	*result = 0;
	int battleDelayData = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &battleDelayData)) {
		return true;
	}

	
	switch (battleDelayData) {
		case 0: *result = (musicTimes.getMinStopBattleDelay () / 1000); break;
		case 1: *result = (musicTimes.getExtraStopBattleDelay () / 1000); break;
		default: *result = (musicTimes.getStopBattleDelay () / 1000);
	}

	Console_PrintC ("Stop battle delay >> %f", *result);
	return true;
}





bool Cmd_SetStopBattleDelay_Execute (COMMAND_ARGS) {
	*result = 0;
	float minDelay = 0;
	float extraDelay = 0;
	int forceUpdate = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &minDelay, &extraDelay, &forceUpdate)) {
		return true;
	}
	if (minDelay < 0) {
		minDelay = 0;
	}
	if (extraDelay < 0) {
		extraDelay = 0;
	}

	WaitForSingleObject (hMusicTimesMutex, INFINITE);
		bool changed1 = musicTimes.setMinStopBattleDelay (minDelay * 1000);
		bool changed2 = musicTimes.setExtraStopBattleDelay (extraDelay * 1000);
		if (changed1 || changed2 || forceUpdate != 0) {
			musicTimes.recalculateStopBattleDelay ();
			_MESSAGE ("Command >> emcSetStopBattleDelay >> Min delay: %f, Extra delay: %f", minDelay, extraDelay);
			*result = 1;
		}
	ReleaseMutex (hMusicTimesMutex);

	Console_PrintC ("Set stop battle delay >> Min delay: %f, Extra delay: %f", minDelay, extraDelay);
	return true;
}





bool Cmd_GetMusicRememberTime_Execute (COMMAND_ARGS) {
	*result = 0;
	int battleTime = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &battleTime)) {
		return true;
	}

	if (battleTime == 0) {
		*result = musicTimes.getMusicRememberTime () / 1000;
	} else {
		*result = musicTimes.getBattleMusicRememberTime () / 1000;
	}
	Console_PrintC ("Max music restore time >> %f", *result);
	return true;
}





bool Cmd_SetMusicRememberTime_Execute (COMMAND_ARGS) {
	*result = 0;
	float restoreTime;
	int battleTime = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &restoreTime, &battleTime)) {
		return true;
	}
	if (restoreTime < 0) {
		restoreTime = 0;
	}

	WaitForSingleObject (hMusicTimesMutex, INFINITE);
		if (battleTime == 0) {
			if (musicTimes.setMusicRememberTime (restoreTime * 1000)) {
				_MESSAGE ("Command >> emcSetMaxRestoreTime >> Restore: %f", restoreTime);
				*result = 1;
			}
			Console_PrintC ("Set max restore time >> Restore: %f", restoreTime);
		} else {
			if (musicTimes.setBattleMusicRememberTime (restoreTime * 1000)) {
				_MESSAGE ("Command >> emcSetMaxRestoreTime >> Restore battle: %f", restoreTime);
				*result = 1;
			}
			Console_PrintC ("Set max restore time >> Restore battle: %f", restoreTime);
		}
	ReleaseMutex (hMusicTimesMutex);
	return true;
}





bool Cmd_PlayTrack_Execute (COMMAND_ARGS) {
	*result = 0;
	char path[MAX_PATH];
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &path)) {
		return true;
	}

	_MESSAGE ("Command >> emcPlayTrack >> %s", path);
	parsePlayTrackCommand (path);
	*result = 1;
	return true;
}





bool Cmd_MusicStop_Execute (COMMAND_ARGS) {
	*result = 0;
	float fadeOut = -1;
	int keepStopped = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &fadeOut, &keepStopped)) {
		return true;
	}
	
	if (fadeOut < 0) {
		fadeOut = musicTimes.getFadeOut ();		//Read only, mutex not needed (this is a command, only other commands can modify it, but can't run concurrently)
	}
	threadRequest.requestHoldMusicPlayer (keepStopped > 0);

	WaitForSingleObject (hMusicPlayerMutex, INFINITE);
		if (musicPlayer.stop (fadeOut)) {
			fadeOut /= 1000;
			Console_PrintC ("Stop music >> Fade Out: %f", fadeOut);
			_MESSAGE ("Command >> emcMusicStop >> Fade Out: %f", fadeOut);
			*result = 1;
		} else {
			Console_PrintC ("Stop music >> Can't be stopped if not playing/paused!");
			_MESSAGE ("Command >> emcMusicStop >> Can't be stopped if not playing/paused!");
		}
	ReleaseMutex (hMusicPlayerMutex);
	return true;
}





bool Cmd_MusicPause_Execute (COMMAND_ARGS) {
	*result = 0;
	float fadeOut = -1;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &fadeOut)) {
		return true;
	}

	if (fadeOut < 0) {
		fadeOut = musicTimes.getFadeOut ();		//Read only, mutex not needed
	}

	WaitForSingleObject (hMusicPlayerMutex, INFINITE);
		if (musicPlayer.pause (fadeOut)) {
			fadeOut /= 1000;
			Console_PrintC ("Pause music >> Fade Out: %f", fadeOut);
			_MESSAGE ("Command >> emcMusicPause >> Fade Out: %f", fadeOut);
			*result = 1;
		} else {
			Console_PrintC ("Pause music >> Can't be paused while not playing!");
			_MESSAGE ("Command >> emcMusicPause >> Can't be paused while not playing!");
		}
	ReleaseMutex (hMusicPlayerMutex);
	return true;
}





bool Cmd_MusicResume_Execute (COMMAND_ARGS) {
	*result = 0;
	float fadeIn = -1;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &fadeIn)) {
		return true;
	}

	if (fadeIn < 0) {
		fadeIn = musicTimes.getFadeIn ();		//Read only, mutex not needed
	}

	WaitForSingleObject (hMusicPlayerMutex, INFINITE);
		if (musicPlayer.resume (fadeIn)) {
			fadeIn /= 1000;
			Console_PrintC ("Resume music >> Fade In: %f", fadeIn);
			_MESSAGE ("Command >> emcMusicResume >> Fade In: %f", fadeIn);
			*result = 1;
		} else {
			Console_PrintC ("Resume music >> Can't resume if not paused");
			_MESSAGE ("Command >> emcMusicResume >> Can't resume if not paused");
		}
	ReleaseMutex (hMusicPlayerMutex);
	return true;
}





bool Cmd_MusicRestart_Execute (COMMAND_ARGS) {
	*result = 0;
	float fadeIn = -1;
	float fadeOut = -1;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &fadeOut, &fadeIn)) {
		return true;
	}

	if (fadeOut < 0) {
		fadeOut = musicTimes.getFadeOut ();
	}
	if (fadeIn < 0) {
		fadeIn = musicTimes.getFadeIn ();
	}

	WaitForSingleObject (hMusicPlayerMutex, INFINITE);
		*result = musicPlayer.isStopped () ? 1 : 0;
		musicPlayer.restart (fadeOut, fadeIn);
	ReleaseMutex (hMusicPlayerMutex);

	fadeOut /= 1000;
	fadeIn /= 1000;
	Console_PrintC ("Restart music >> Fade Out: %f | Fade In: %f | Was stopped: %d", fadeOut, fadeIn, *result);
	_MESSAGE ("Command >> emcMusicRestart >> Fade Out %f | Fade In: %f | Was stopped: %d", fadeOut, fadeIn, *result);
	return true;
}





bool Cmd_MusicNextTrack_Execute (COMMAND_ARGS) {
	*result = 0;
	int noHold = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &noHold)) {
		return true;
	}

	threadRequest.requestNextTrack (noHold != 0);
	Console_PrintC ("Next track requested!");
	_MESSAGE ("Command >> emcMusicNextTrack");
	*result = 1;
	return true;
}