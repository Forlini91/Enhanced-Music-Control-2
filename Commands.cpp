#include "Commands.h"

#include <time.h>
#include <map>
#include <unordered_map>
#include "StringVar.h"
#include "ArrayVar.h"

#include "GlobalSettings.h"
#include "Multiplier.h"
#include "MusicType.h"
#include "MusicState.h"
#include "MusicPlayer.h"
#include "Playlist.h"
#include "ActivePlaylist.h"
#include "ThreadRequest.h"
#include "ThreadState.h"
#include "FadeThread.h"
#include "PlayMusicFile.h"



using namespace std;



extern OBSEArrayVarInterface* g_arrayIntfc;
extern OBSEStringVarInterface* g_stringIntfc;
typedef OBSEArrayVarInterface::Array OBSEArray;
extern int numPlaylists;
extern int numMultipliers;





//No input
//Returns the MusicType Oblivion wants to be played.
bool Cmd_GetMusicType_Execute (COMMAND_ARGS) {
	int mode = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &mode)) {
		return true;
	}

	LockHandle (hMusicStateMutex);
		switch (mode) {
			case 0: *result = music.GetRealMusicType (); break;
			case 1: *result = music.GetWorldMusic (); break;
			case 2: *result = music.override; break;
			case 3: *result = music.special; break;
			case 4: *result = music.locked; break;
		}
	UnlockHandle (hMusicStateMutex);

	if (CONSOLE) {
		Console_Print ("Current music type >> %.0f", *result);
	}
	return true;
};





bool Cmd_SetMusicType_Execute (COMMAND_ARGS) {
	*result = 0;

	int musicType = 255;
	int locked = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &musicType, &locked)) {
		return true;
	}

	if (musicType == 3 || musicType > 4) {
		if (CONSOLE) {
			Console_Print ("Set music type >> Failed: music type is not valid > Type: %d, Lock: %d", musicType, locked);
		}
		_MESSAGE ("Command >> emcSetMusicType >> Failed: music type is not valid > Type: %d, Lock: %d", musicType, locked);
		return true;
	}

	LockHandle (hMusicStateMutex);
		if (locked < 0 || musicType < 0) {
			music.overrideMusic (MusicType::Mt_NotKnown, false);
		} else {
			music.overrideMusic ((MusicType) musicType, locked>0);
		}
	UnlockHandle (hMusicStateMutex);

	if (CONSOLE) {
		Console_Print ("Set music type >> Succeed");
	}
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
		if (CONSOLE) {
			Console_Print ("Playlist manager >> Failed. Name is empty");
		}
		_MESSAGE ("Command >> emcCreatePlaylist >> Failed. Name is empty");
		return true;
	}

	string paths = string (plPaths);
	if (paths.empty ()) {				//Oh, wait, can't create a playlist without path
		if (CONSOLE) {
			Console_Print ("Playlist manager >> Failed. Path is empty");
		}
		_MESSAGE ("Command >> emcCreatePlaylist >> Failed. Path is empty for playlist \"%s\"", pPlaylistName);
	}

	LockHandle (hPlaylistMutex);
		try {
			PlaylistEmplaceResult insResult = EMPLACE_PLAYLIST (pPlaylistName, paths, shuffle != 0, false);
			if (insResult.second) {		//Create new playlist
				if (playlists.size () >= numPlaylists) {
					if (CONSOLE) {
						Console_Print ("Playlist manager >> Failed. Max playlists limit reached: %d", numPlaylists);
					}
					_MESSAGE ("Command >> emcCreatePlaylist >> Failed. Max playlists limit reached: %d", numPlaylists);
					playlists.erase (insResult.first);
				} else {
					*result = 1;
					if (CONSOLE) {
						Console_Print ("Create playlist >> Succeed");
					}
					_MESSAGE ("Command >> emcCreatePlaylist >> Succeed: \"%s\" >> %s", pPlaylistName, plPaths);
				}
			} else {		//Alter/Delete existing playlist
				PlaylistsMap::iterator it = insResult.first;
				Playlist* playlist = &it->second;
				ActivePlaylist* activePlaylist = NULL;
				if (playlist->isVanilla ()) {						//Playlist is vanilla
					if (CONSOLE) {
						Console_Print ("Playlist manager >> Failed. This is a vanilla playlist and can't be altered");
					}
					_MESSAGE ("Command >> emcCreatePlaylist >> Failed. \"%s\" is a vanilla playlist and can't be altered", pPlaylistName);
				} else if ((activePlaylist = getActivePlaylist(playlist)) != NULL) {		//Playlist is assigned
					if (CONSOLE) {
						Console_Print ("Playlist manager >> Failed. This playlist can't be deleted, as it's assigned to music type %s", activePlaylist->name);
					}
					_MESSAGE ("Command >> emcCreatePlaylist >> Failed. \"%s\" can't be deleted, as it's assigned to music type %s", pPlaylistName, activePlaylist->name);
					*result = -1;
				} else if (paths.empty ()) {			//Empty path -> erase playlist
					playlists.erase (it);
					*result = 1;
				} else {							//Not empty path -> alter playlist
					Playlist backupPlaylist (playlist);			//Create a backup
					if (playlist->setPaths (paths, shuffle != 0)) {	//If recreation succeed...
						*result = 1;
					} else {					//Recreation failed
						backupPlaylist.copyTo (playlist);		//Restore the playlist from the backup.
						if (CONSOLE) {
							Console_Print ("Recreate playlist >> Failed for unknown reasons");
						}
						_MESSAGE ("Command >> emcCreatePlaylist >> Failed for unknown reasons");
					}
				}
			}
		} catch (exception e) {
			if (CONSOLE) {
				Console_Print ("Create playlist >> Failed. No track found.");
			}
			_MESSAGE ("Command >> emcCreatePlaylist >> Failed. No track found for playlist \"%s\" in %s", pPlaylistName, plPaths);
		}
	UnlockHandle (hPlaylistMutex);

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
	string path = string (plPath);
	if (playlistName.empty ()) {	//Empty name. Can't proceed
		if (CONSOLE) {
			Console_Print ("Playlist manager >> Failed. Name is empty");
		}
		_MESSAGE ("Command >> emcAddPath >> Failed. Name is empty");
		return true;
	} else if (path.empty ()) {
		if (CONSOLE) {
			Console_Print ("Playlist manager >> Failed. Path is empty");
		}
		_MESSAGE ("Command >> emcAddPath >> Failed. Path is empty for playlist \"%s\"", pPlaylistName);
		return true;
	}

	PlaylistsMap::iterator it = playlists.find (playlistName);
	if (it == playlists.end ()) {
		if (CONSOLE) {
			Console_Print ("Add path >> Failed. Playlist does not exist");
		}
		_MESSAGE ("Command >> emcAddPath >> Failed. Playlist does not exist");
	} else {
		Playlist* playlist = &it->second;
		ActivePlaylist* activePlaylist = NULL;
		//Make sure its not one of the default playlists.
		if (playlist->isVanilla ()) {
			if (CONSOLE) {
				Console_Print ("Add path >> Failed. This is a vanilla playlist and can't be altered");
			}
			_MESSAGE ("Command >> emcAddPath >> Failed. \"%s\" is a vanilla playlist and can't be altered", pPlaylistName);
		} else if ((activePlaylist = getActivePlaylist(playlist)) != NULL) {
			if (CONSOLE) {
				Console_Print ("Add path >> Failed. This playlist can't be altered, as it's assigned to music type %s", activePlaylist->name);
			}
			_MESSAGE ("Command >> emcAddPath >> Failed. \"%s\" can't be altered, as it's assigned to music type %s", pPlaylistName, activePlaylist->name);
			*result = -1;
		}
			
		Playlist backupPlaylist (playlist);		//Backup the playlis
		if (playlist->addPath (path)) {		//Try to add the new path
			if (CONSOLE) {
				Console_Print ("Add path >> Succeed. Playlist updated", pPlaylistName, plPath);
			}
			_MESSAGE ("Command >> emcAddPath >> Succeed. Playlist updated: \"%s\" >> %s", pPlaylistName, plPath);
			*result = 1;
		} else {			//Addition failed.
			backupPlaylist.copyTo (playlist);		//Restore the backed up playlist.
			if (CONSOLE) {
				Console_Print ("Add path >> Failed. No track found", pPlaylistName, plPath);
			}
			_MESSAGE ("Command >> emcAddPath >> Failed. No track found for playlist \"%s\" in %s", pPlaylistName, plPath);
		}
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

	string playlistName = string (pPlaylistName);
	if (!playlistName.empty ()) {
		if (playlists.find (playlistName) != playlists.end ()) {
			*result = 1;
		}
	}
	if (CONSOLE) {
		Console_Print ("Playlist exists >> %.0f", *result);
	}
	return true;
}





//Required Input: 1 String
//Returns 1 is the playlist is assigned to a MusicType, else 0.
bool Cmd_IsPlaylistActive_Execute (COMMAND_ARGS) {
	//result == 1 if it is an active playlist, 0 otherwise.
	*result = 0;

	//Parameters
	char pPlaylistName[512];
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &pPlaylistName)) {
		return true;
	}
	string playlistName = string (pPlaylistName);
	if (!playlistName.empty ()) {
		PlaylistsMap::iterator it = playlists.find (playlistName);
		if (it != playlists.end ()) {
			Playlist* playlist = &it->second;
			if (isPlaylistActive (playlist)) {
				*result = 1;
			}
			if (CONSOLE) {
				Console_Print ("Is playlist active >> %.0f", *result);
			}
			return true;
		}
	}
	if (CONSOLE) {
		Console_Print ("Playlist doesn't exists");
	}
	return true;
}





bool Cmd_SetPlaylist_Execute (COMMAND_ARGS) {
	*result = 0;

	LockHandle (hMusicPlayerMutex);
		bool isReady = musicPlayer.isReady();
	UnlockHandle (hMusicPlayerMutex);
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
		if (CONSOLE) {
			Console_Print ("Set playlist >> Failed. Name is empty");
		}
		_MESSAGE ("Command >> emcSetPlaylist >> Failed. Name is empty");
		return true;
	} else if (!isMusicTypeValid(pMusicType)) {
		if (CONSOLE) {
			Console_Print ("Set playlist >> Failed. Music type doesn't exists");
		}
		_MESSAGE ("Command >> emcSetPlaylist >> Failed. Music type %d doesn't exists", pMusicType);
		return true;
	}
	

	MusicType musicType = static_cast<MusicType>(pMusicType);
	LockHandle (hPlaylistMutex);
		PlaylistsMap::iterator it = playlists.find (playlistName);
		if (it == playlists.end()) {
			if (CONSOLE) {
				Console_Print ("Set playlist >> Failed. Playlist does not exist: \"%s\"", pPlaylistName);
			}
			_MESSAGE ("Command >> emcSetPlaylist >> Failed. Playlist does not exist: \"%s\"", pPlaylistName);
			return true;
		}

		//Let other threads know the playlists are being manipulated.
		Playlist* playlist = &it->second;
		ActivePlaylist* apl = getActivePlaylist (musicType);
		bool succeed = apl->playlist != playlist;
		if (succeed) {
			threadRequest.requestSetPlaylist (playlist, musicType, pQueueMode, pDelay);
			apl->playlist = playlist;
			if (CONSOLE) {
				Console_Print ("Set playlist >> Succeed. Target: %d, Name: \"%s\", Mode: %d, Delay: %f", pMusicType, pPlaylistName, pQueueMode, pDelay);
			}
			_MESSAGE ("Command >> emcSetPlaylist >> Succeed. Target: %d, Name: \"%s\", Mode: %d, Delay: %f", pMusicType, pPlaylistName, pQueueMode, pDelay);
			*result = 1;
		} else {
			if (CONSOLE) {
				Console_Print ("Set playlist >> Failed. Target: %d, Name: \"%s\", Mode: %d, Delay: %f", pMusicType, pPlaylistName, pQueueMode, pDelay);
			}
			_MESSAGE ("Command >> emcSetPlaylist >> Failed. Target: %d, Name: \"%s\", Mode: %d, Delay: %f", pMusicType, pPlaylistName, pQueueMode, pDelay);
		}
	UnlockHandle (hPlaylistMutex);
	return true;
}





//Optional Input: 1 Int
//Return value has no meaning.
bool restorePlaylist (int i) {
	ActivePlaylist* activePlaylist = getActivePlaylist (static_cast<MusicType>(i));
	Playlist* vanillaPlaylist = vanillaPlaylists[i];
	if (activePlaylist->playlist != vanillaPlaylist) {
		activePlaylist->playlist = vanillaPlaylist;
		return true;
	} else {
		return false;
	}
}
bool Cmd_RestorePlaylist_Execute (COMMAND_ARGS) {
	*result = 0;

	LockHandle (hMusicPlayerMutex);
		bool isReady = musicPlayer.isReady ();
	UnlockHandle (hMusicPlayerMutex);
	if (!isReady) {
		return true;
	}

	//Parameters
	int pMusicType = -1;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &pMusicType)) {
		return true;
	}

	if (pMusicType > 4) {
		if (CONSOLE) {
			Console_Print ("Restore playlist >> Failed. Invalid music type");
		}
		_MESSAGE ("Command >> emcRestorePlaylist >> Failed. Invalid music type %d", pMusicType);
		return true;
	}

	if (pMusicType < 0) {		//Restore all.
		bool restored = false;
		LockHandle (hPlaylistMutex);
			for (int i = 0; i < 5; i++) {
				if (restorePlaylist (i)) {
					restored = true;
				}
			}
		UnlockHandle (hPlaylistMutex);

		if (restored) {
			*result = 1;
			threadRequest.requestResetPlaylist (MusicType::Undefined);
			if (CONSOLE) {
				Console_Print ("Restore playlist >> Succeed. All music types restored");
			}
			_MESSAGE ("Command >> emcRestorePlaylist >> Succeed. All music types restored to default playlist");
		} else {
			if (CONSOLE) {
				Console_Print ("Restore playlist >> Failed");
			}
			_MESSAGE ("Command >> emcRestorePlaylist >> Failed. All music types are already using their default playlists");
		}
	} else {
		LockHandle (hPlaylistMutex);
			bool restored = restorePlaylist (pMusicType);
		UnlockHandle (hPlaylistMutex);

		if (restored) {
			*result = 1;
			threadRequest.requestResetPlaylist (static_cast<MusicType>(pMusicType));
			if (CONSOLE) {
				Console_Print ("Restore playlist >> Succeed. Music type restored");
			}
			_MESSAGE ("Command >> emcRestorePlaylist >> Succeed. Music type %d restored to default playlist", pMusicType);
		} else {
			if (CONSOLE) {
				Console_Print ("Restore playlist >> Failed. Music type %d is already using the default playlist", pMusicType);
			}
			_MESSAGE ("Command >> emcRestorePlaylist >> Failed. Music type %d is already using the default playlist", pMusicType);
		}
	}
	
	return true;
}






bool Cmd_IsMusicSwitching_Execute (COMMAND_ARGS) {
	LockHandle (hMusicPlayerMutex);
		*result = musicPlayer.isReady () ? 0 : 1;
	UnlockHandle (hMusicPlayerMutex);
	if (CONSOLE) {
		Console_Print ("Music switching >> %.0f", *result);
	}
	return true;
}





bool Cmd_GetAllPlaylists_Execute (COMMAND_ARGS) {
	*result = 0;
	int pOnlyActive = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &pOnlyActive)) {
		return true;
	}

	bool console = CONSOLE;
	const char* playlistName;
	OBSEArray* newArray = g_arrayIntfc->CreateArray (NULL, 0, scriptObj);
	if (pOnlyActive == 0) {
		LockHandle (hPlaylistMutex);
			for (PlaylistsMap::const_iterator it = playlists.cbegin (); it != playlists.cend (); it++) {
				playlistName = (it->second).name;
				g_arrayIntfc->AppendElement (newArray, playlistName);
				if (console) Console_Print (playlistName);
			}
		UnlockHandle (hPlaylistMutex);
	} else {
		LockHandle (hPlaylistMutex);
			for (ActivePlaylist* activePlaylist : activePlaylists) {
				playlistName = activePlaylist->playlist->name;
				g_arrayIntfc->AppendElement (newArray, playlistName);
				if (console) Console_Print ("%s -> %s", activePlaylist->name, playlistName);
			}
		UnlockHandle (hPlaylistMutex);
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
		if (CONSOLE) {
			Console_Print ("Invalid music type");
		}
		return true;
	}

	ActivePlaylist* activePlaylist;
	if (pMusicType < 0) {
		activePlaylist = threadState.activePlaylist;
	} else {
		activePlaylist = getActivePlaylist (static_cast<MusicType>(pMusicType));
	}
	if (activePlaylist != NULL && activePlaylist->playlist != NULL) {
		const char* playlistName = activePlaylist->playlist->name;
		g_stringIntfc->Assign (PASS_COMMAND_ARGS, playlistName);
		if (CONSOLE) {
			Console_Print ("Playlist >> %s", playlistName);
		}
	} else {
		if (CONSOLE) {
			Console_Print ("No playlist assigned to type %d", pMusicType);
		}
	}
	return true;
}





bool Cmd_GetPlaylistTracks_Execute (COMMAND_ARGS) {
	*result = 0;

	char pPlaylistName[512];
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &pPlaylistName)) {
		return true;
	}
	
	Playlist* playlist = NULL;
	string playlistName = string (pPlaylistName);
	LockHandle (hPlaylistMutex);
		if (playlistName.empty ()) {
			if (threadState.activePlaylist != NULL) {
				playlist = threadState.activePlaylist->playlist;
			}
		} else {
			PlaylistsMap::iterator it = playlists.find (playlistName);
			if (it != playlists.end ()) {
				playlist = &it->second;
			}
		}
		if (playlist != NULL) {
			OBSEArray* newArray = g_arrayIntfc->CreateArray (NULL, 0, scriptObj);
			for (const string& track : playlist->getTracks ()) {
				g_arrayIntfc->AppendElement (newArray, track.c_str ());
			}
			g_arrayIntfc->AssignCommandResult (newArray, result);
			if (CONSOLE) {
				playlist->printTracks ();
			}
		} else {
			if (CONSOLE) {
				Console_Print ("Playlist not found");
			}
		}
	UnlockHandle (hPlaylistMutex);
	return true;
}





bool Cmd_GetTrackName_Execute (COMMAND_ARGS) {
	LockHandle (hMusicPlayerMutex);
		const char* songPath = musicPlayer.getTrack ();
	UnlockHandle (hMusicPlayerMutex);
	if (CONSOLE) {
		Console_Print ("Currently track >> %s", songPath);
	}
	g_stringIntfc->Assign (PASS_COMMAND_ARGS, songPath);
	return true;
}





bool Cmd_GetTrackDuration_Execute (COMMAND_ARGS) {
	LockHandle (hMusicPlayerMutex);
		*result = musicPlayer.getTrackDuration () / ONCE_SECOND;
	UnlockHandle (hMusicPlayerMutex);
	if (CONSOLE) {
		Console_Print ("Track duration >> %f", *result);
	}
	return true;
}





bool Cmd_GetTrackPosition_Execute (COMMAND_ARGS) {
	LockHandle (hMusicPlayerMutex);
		*result = musicPlayer.getTrackPosition () / ONCE_SECOND;
		if (CONSOLE) {
			double duration = musicPlayer.getTrackDuration () / ONCE_SECOND;
			Console_Print ("Track position >> %.0f / %.0f", *result, duration);
		}
	UnlockHandle (hMusicPlayerMutex);
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
	REFERENCE_TIME longPos = pPosition * ONCE_SECOND;

	LockHandle (hMusicPlayerMutex);
		REFERENCE_TIME longDur = musicPlayer.getTrackDuration ();
		if (longPos > longDur) {
			longPos = longDur;
		}
		musicPlayer.setTrackPosition (pPosition, pFadeOut, pFadeIn);
	UnlockHandle (hMusicPlayerMutex);

	if (CONSOLE) {
		Console_Print ("Set track position >> Position %.2f, Fade out/in: %f/%f", pPosition, pFadeOut, pFadeIn);
	}
	_MESSAGE ("Command >> emcSetTrackPosition >> Position %.2f, Fade out/in: %f/%f", pPosition, pFadeOut, pFadeIn);
	*result = 1;
	return true;
}





//Required input: None
//Returns 1 if the override is active, else 0.
bool Cmd_IsBattleOverridden_Execute (COMMAND_ARGS) {
	LockHandle (hMusicStateMutex);
		*result = music.battleOverridden ? 1 : 0;
	UnlockHandle (hMusicStateMutex);
	if (CONSOLE) {
		Console_Print ("Battle overridde >> %.0f", *result);
	}
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
	LockHandle (hMusicStateMutex);
		if (music.battleOverridden != battleOverrideBool) {
			if (CONSOLE) {
				Console_Print ("Set battle override >> %d", battleOverrideBool);
			}
			_MESSAGE ("Command >> emcSetBattleOverride >> %d", battleOverrideBool);
			music.battleOverridden = battleOverrideBool;
			*result = 1;
		} else {
			if (CONSOLE) {
				Console_Print ("Set battle override >> %d (unchanged)", battleOverrideBool);
			}
			_MESSAGE ("Command >> emcSetBattleOverride >> %d (unchanged)", battleOverrideBool);
		}
	UnlockHandle (hMusicStateMutex);
	return true;
}





//Required input: None
//Returns 1 if the override is active, else 0.
bool Cmd_IsMusicOnHold_Execute (COMMAND_ARGS) {
	LockHandle (hThreadMutex);
		*result = threadRequest.Request_HoldMusic ? 1 : 0;
	UnlockHandle (hThreadMutex);
	if (CONSOLE) {
		Console_Print ("Hold music >> %.0f", *result);
	}
	return true;
}





//Required input: None
//Returns 1 if the hold was engaged, else 0.
bool Cmd_SetMusicHold_Execute (COMMAND_ARGS) {
	*result = 0;
	int musicHold;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &musicHold)) {
		return true;
	}

	bool musicHoldBool = (musicHold != 0);
	LockHandle (hThreadMutex);
		if (threadRequest.Request_HoldMusic != musicHoldBool) {
			if (CONSOLE) {
				Console_Print ("Set music hold >> %d", musicHoldBool);
			}
			_MESSAGE ("Command >> emcSetMusicHold >> %d", musicHoldBool);
			threadRequest.Request_HoldMusic = musicHoldBool;
			*result = 1;
		} else {
			if (CONSOLE) {
				Console_Print ("Set music hold >> %d (unchanged)", musicHoldBool);
			}
			_MESSAGE ("Command >> emcSetMusicHold >> %d (unchanged)", musicHoldBool);
		}
	UnlockHandle (hThreadMutex);
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
	LockHandle (mult->hThread);
		switch (target) {
			case 1: *result = mult->isFading ? mult->startValue : mult->getValue (); break;
			case 2: *result = mult->isFading ? mult->targetValue : mult->getValue (); break;
			case 3: *result = mult->isFading ? mult->fadeTime : -1; break;
			default: *result = mult->getValue ();
		}
	UnlockHandle (mult->hThread);
	if (CONSOLE) {
		Console_Print ("Master volume >> %f", *result);
	}
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
	LockHandle (mult->hThread);
		if (fadeTime > 0) {
			if (CONSOLE) {
				Console_Print ("Set master volume >> Fade volume from %f to %f, Method: %d, Time: %f", mult->getValue (), volume, method, fadeTime);
			}
			_MESSAGE ("Command >> emcSetMasterVolume >> Fade volume from %f to %f, Method: %d, Time: %f", mult->getValue (), volume, method, fadeTime);
			mult->fadeVolume (volume, fadeTime);
		} else {
			if (CONSOLE) {
				Console_Print ("Set master volume >> Set volume from %f to %f", mult->getValue (), volume, method);
			}
			_MESSAGE ("Command >> emcSetMasterVolume >> Set volume from %f to %f", mult->getValue (), volume, method);
			mult->isFading = false;	//signal STOP to the thread running on this variable (if any)
			mult->setValue (volume);
		}
	UnlockHandle (mult->hThread);
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
			if (CONSOLE) {
				Console_Print ("This multiplier doesn't exists");
			}
			return true;
		}
	}

	LockHandle (mult->hThread);
		switch (target) {
			case 1: *result = mult->isFading ? mult->startValue : mult->getValue (); break;
			case 2: *result = mult->isFading ? mult->targetValue : mult->getValue (); break;
			case 3: *result = mult->isFading ? mult->fadeTime : -1; break;
			case 4: *result = (method < 2 || mult->saveGame) ? 1 : 0; break;
			case 5: *result = (method < 2 || mult->saveSession) ? 1 : 0; break;
			default: *result = mult->getValue ();
		}
	UnlockHandle (mult->hThread);

	if (CONSOLE) {
		Console_Print ("Music multiplier >> %f", *result);
	}
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

	Multiplier* mult = NULL;
	if (method < 2) {
		mult = (method == 0) ? &multObMusic : &multObMusicIni;
		LockHandle (mult->hThread);
			if (fadeTime > 0) {
				if (CONSOLE) {
					Console_Print ("Set music volume >> Fade volume from %f to %f, Method: %d, Time: %f", mult->getValue (), volume, method, fadeTime);
				}
				_MESSAGE ("Command >> emcSetMusicVolume >> Fade volume from %f to %f, Method: %d, Time: %f", mult->getValue (), volume, method, fadeTime);
				mult->fadeVolume (volume, fadeTime);
			} else {
				if (CONSOLE) {
					Console_Print ("Set music volume >> Change volume from %f to %f, Method: %d", mult->getValue (), volume, method);
				}
				_MESSAGE ("Command >> emcSetMusicVolume >> Change volume from %f to %f, Method: %d", mult->getValue (), volume, method);
				mult->isFading = false;
				mult->setValue (volume);
			}
		UnlockHandle (mult->hThread);
	} else {
		string key = string (pKey);
		if (key.empty ()) {
			if (CONSOLE) {
				Console_Print ("Set music volume >> Can't create/destroy a custom multiplier without a key");
				return true;
			}
		}

		_MESSAGE ("Command >> emcSetMusicVolume >> Method: %d, Key: %s", method, pKey);
		if (volume == 1.0) {
			MultipliersMap::iterator pos = multipliersCustom.find (key);
			if (pos == multipliersCustom.end ()) {
				if (CONSOLE) {
					Console_Print ("Set music volume >> Can't create a new multiplier with volume 1.0");
				}
				_MESSAGE ("Command >> emcSetMusicVolume >> Can't create a new multiplier with volume 1.0");
				return true;
			} else if (fadeTime <= 0) {
				mult = &pos->second;
				LockHandle (mult->hThread);
					mult->isDestroyed = true;
				UnlockHandle (mult->hThread);
				if (CONSOLE) {
					Console_Print ("Set music volume >> Multiplier destroyed");
				}
				_MESSAGE ("Command >> emcSetMusicVolume >> Multiplier destroyed");
				*result = 1;
				return true;
			}
		} else {
			MultiplierEmplaced insResult = multipliersCustom.emplace (BUILD_IN_PLACE (key, 1.0));
			if (insResult.second) {
				if (multipliersCustom.size () > numMultipliers) {
					if (CONSOLE) {
						Console_Print ("Set music volume >> Failed. Max multipliers limit reached: %d", numMultipliers);
					}
					_MESSAGE ("Command >> emcSetMusicVolume >> Failed. Max multipliers limit reached: %d", numMultipliers);
					return true;
				}
				if (CONSOLE) {
					Console_Print ("Set music volume >> Create new multiplier");
				}
				_MESSAGE ("Command >> emcSetMusicVolume >> Create new multiplier");
			}
			mult = &insResult.first->second;
		}

		LockHandle (mult->hThread);
			mult->saveGame = (method == 3);
			mult->saveSession = (method == 4);
			mult->isDestroyed = false;
			if (fadeTime > 0) {
				if (CONSOLE) {
					_MESSAGE ("Set music volume >> Fade multiplier \"%s\" from %f to %f Time: %f", pKey, mult->getValue (), volume, fadeTime);
				}
				_MESSAGE ("Command >> emcSetMusicVolume >> Fade multiplier \"%s\" from %f to %f Time: %f", pKey, mult->getValue (), volume, fadeTime);
				mult->fadeVolume (volume, fadeTime);
			} else {
				if (CONSOLE) {
					Console_Print ("Set music volume >> Set multiplier \"%s\" from %f to %f", pKey, mult->getValue (), volume);
				}
				_MESSAGE ("Command >> emcSetMusicVolume >> Set multiplier \"%s\" from %f to %f", pKey, mult->getValue (), volume);
				mult->isFading = false;		//signal STOP to the thread running on this multiplier (if any)
				mult->setValue (volume);
			}
		UnlockHandle (mult->hThread);
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
	LockHandle (mult->hThread);
		switch (target) {
			case 1: *result = mult->isFading ? mult->startValue : mult->getValue (); break;
			case 2: *result = mult->isFading ? mult->targetValue : mult->getValue (); break;
			case 3: *result = mult->isFading ? mult->fadeTime : -1; break;
			default: *result = mult->getValue ();
		}
	UnlockHandle (mult->hThread);

	if (CONSOLE) {
		Console_Print ("Effects volume >> %f", *result);
	}
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
	LockHandle (mult->hThread);
		if (fadeTime > 0) {
			if (CONSOLE) {
				Console_Print ("Set effects volume >> Fade volume from %f to %f, Method: %d, Time: %f", mult->getValue (), volume, method, fadeTime);
			}
			_MESSAGE ("Command >> emcSetEffectsVolume >> Fade volume from %f to %f, Method: %d, Time: %f", mult->getValue (), volume, method, fadeTime);
			mult->fadeVolume (volume, fadeTime);
		} else {
			if (CONSOLE) {
				Console_Print ("Set effects volume >> Set volume from %f to %f, Method: %d", mult->getValue (), volume, method, fadeTime);
			}
			_MESSAGE ("Command >> emcSetEffectsVolume >> Set volume from %f to %f, Method: %d", mult->getValue (), volume, method, fadeTime);
			mult->isFading = false;	//signal STOP to the thread running on this variable (if any)
			mult->setValue (volume);
		}
	UnlockHandle (mult->hThread);

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
	LockHandle (mult->hThread);
		switch (target) {
			case 1: *result = mult->isFading ? mult->startValue : mult->getValue (); break;
			case 2: *result = mult->isFading ? mult->targetValue : mult->getValue (); break;
			case 3: *result = mult->isFading ? mult->fadeTime : -1; break;
			default: *result = mult->getValue ();
		}
	UnlockHandle (mult->hThread);

	if (CONSOLE) {
		Console_Print ("Foot volume >> %f", *result);
	}
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
	LockHandle (mult->hThread);
		if (fadeTime > 0) {
			if (CONSOLE) {
				Console_Print ("Set foot volume >> Fade volume from %f to %f, Method: %d, Time: %f", mult->getValue (), volume, method, fadeTime);
			}
			_MESSAGE ("Command >> emcSetFootVolume >> Fade volume from %f to %f, Method: %d, Time: %f", mult->getValue (), volume, method, fadeTime);
			mult->fadeVolume (volume, fadeTime);
		} else {
			if (CONSOLE) {
				Console_Print ("Set foot volume >> Set volume from %f to %f, Method: %d", mult->getValue (), volume, method);
			}
			_MESSAGE ("Command >> emcSetFootVolume >> Set volume from %f to %f, Method: %d", mult->getValue (), volume, method);
			mult->isFading = false;	//signal STOP to the thread running on this variable (if any)
			mult->setValue (volume);
		}
	UnlockHandle (mult->hThread);

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
	LockHandle (mult->hThread);
		switch (target) {
			case 1: *result = mult->isFading ? mult->startValue : mult->getValue (); break;
			case 2: *result = mult->isFading ? mult->targetValue : mult->getValue (); break;
			case 3: *result = mult->isFading ? mult->fadeTime : -1; break;
			default: *result = mult->getValue ();
		}
	UnlockHandle (mult->hThread);

	if (CONSOLE) {
		Console_Print ("Voice volume >> %f", *result);
	}
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
	LockHandle (mult->hThread);
		if (fadeTime > 0) {
			if (CONSOLE) {
				Console_Print ("Set voice volume >> Fade volume from %f to %f, Method: %d, Time: %f", mult->getValue (), volume, method, fadeTime);
			}
			_MESSAGE ("Command >> emcSetVoiceVolume >> Fade volume from %f to %f, Method: %d, Time: %f", mult->getValue (), volume, method, fadeTime);
			mult->fadeVolume (volume, fadeTime);
		} else {
			if (CONSOLE) {
				Console_Print ("Set voice volume >> Set volume from %f to %f, Method: %d", mult->getValue (), volume, method);
			}
			_MESSAGE ("Command >> emcSetVoiceVolume >> Set volume from %f to %f, Method: %d", mult->getValue (), volume, method);
			mult->isFading = false;	//signal STOP to the thread running on this variable (if any)
			mult->setValue (volume);
		}
	UnlockHandle (mult->hThread);

	*result = 1;
	return true;
}





bool Cmd_GetMusicSpeed_Execute (COMMAND_ARGS) {
	LockHandle (hMusicPlayerMutex);
		*result = musicPlayer.getMusicSpeed ();
	UnlockHandle (hMusicPlayerMutex);
	if (CONSOLE) {
		Console_Print ("Music speed >> %f", *result);
	}
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

	LockHandle (hMusicPlayerMutex);
		bool changed = musicPlayer.setMusicSpeed ((double)speed);
	UnlockHandle (hMusicPlayerMutex);
	if (changed) {
		_MESSAGE ("Command >> emcSetMusicSpeed >> Speed: %f", speed);
		*result = 1;
	}

	if (CONSOLE) {
		Console_Print ("Set music speed >> Speed: %f", speed);
	}
	return true;
}





bool Cmd_GetFadeTime_Execute (COMMAND_ARGS) {
	*result = 0;
	int fadeIn = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &fadeIn)) {
		return true;
	}

	LockHandle (hMusicPlayerMutex);
		if (fadeIn != 0) {
			*result = musicPlayer.getCurrentFadeInLength () / 1000;
		} else {
			*result = musicPlayer.getCurrentFadeOutLength () / 1000;
		}
	UnlockHandle (hMusicPlayerMutex);

	if (CONSOLE) {
		Console_Print ("Fade time >> %f", *result);
	}
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

	if (fadeIn != 0) {
		LockHandle (hMusicPlayerMutex);
			bool changed = musicPlayer.setCurrentFadeInLength (fadeTime * 1000.0);
		UnlockHandle (hMusicPlayerMutex);
		if (changed) {
			_MESSAGE ("Command >> emcSetFadeTime >> Fade In: %f", fadeTime);
			*result = 1;
		}
		if (CONSOLE) {
			Console_Print ("Set fade time >> Fade In: %f", fadeTime);
		}
	} else {
		LockHandle (hMusicPlayerMutex);
			bool changed = musicPlayer.setCurrentFadeOutLength (fadeTime * 1000.0);
		UnlockHandle (hMusicPlayerMutex);
		if (changed) {
			_MESSAGE ("Command >> emcSetFadeTime >> Fade Out: %f", fadeTime);
			*result = 1;
		}
		if (CONSOLE) {
			Console_Print ("Set fade time >> Fade Out: %f", fadeTime);
		}
	}
	return true;
}





bool Cmd_GetPauseTime_Execute (COMMAND_ARGS) {
	*result = 0;
	int extraPause = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &extraPause)) {
		return true;
	}

	LockHandle (hMusicPlayerMutex);
		switch (extraPause) {
			case 2: *result = (musicPlayer.getCalculatedPauseTime() / 1000); break;
			case 1: *result = (musicPlayer.getExtraPauseTime () / 1000); break;
			default: *result = (musicPlayer.getMinPauseTime () / 1000);
		}
	UnlockHandle (hMusicPlayerMutex);

	if (CONSOLE) {
		Console_Print ("Pause time >> %f", *result);
	}
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

	LockHandle (hMusicPlayerMutex);
		bool changed1 = musicPlayer.setMinPauseTime (pauseTime * 1000);
		bool changed2 = musicPlayer.setExtraPauseTime (extraPauseTime * 1000);
		if (changed1 || changed2 || forceUpdate != 0) {
			musicPlayer.recalculatePauseTime();
			_MESSAGE ("Command >> emcSetPauseTime >> Min pause: %f, Extra pause: %f", pauseTime, extraPauseTime);
			*result = 1;
		}
	UnlockHandle (hMusicPlayerMutex);

	if (CONSOLE) {
		Console_Print ("Set pause time >> Min pause: %f, Extra pause: %f", pauseTime, extraPauseTime);
	}
	return true;
}





bool Cmd_GetBattleDelay_Execute (COMMAND_ARGS) {
	*result = 0;
	int extraBattleDelay = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &extraBattleDelay)) {
		return true;
	}

	LockHandle (hMusicPlayerMutex);
		switch (extraBattleDelay) {
			case 2:	*result = (musicPlayer.getCalculatedBattleDelay () / 1000); break;
			case 1: *result = (musicPlayer.getExtraBattleDelay () / 1000); break;
			default: *result = (musicPlayer.getMinBattleDelay () / 1000);
		}
	UnlockHandle (hMusicPlayerMutex);

	if (CONSOLE) {
		Console_Print ("Battle delay >> %f", *result);
	}
	return true;
}





bool Cmd_SetBattleDelay_Execute (COMMAND_ARGS) {
	*result = 0;
	float battleDelay = 0;
	float extraBattleDelay = 0;
	int forceUpdate = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &battleDelay, &extraBattleDelay, &forceUpdate)) {
		return true;
	}
	if (battleDelay < 0.5) {
		battleDelay = 0.5;
	}
	if (extraBattleDelay < 0) {
		extraBattleDelay = 0;
	}

	LockHandle (hMusicPlayerMutex);
		bool changed1 = musicPlayer.setMinBattleDelay (battleDelay * 1000);
		bool changed2 = musicPlayer.setExtraBattleDelay (extraBattleDelay * 1000);
		if (changed1 || changed2 || forceUpdate != 0) {
			musicPlayer.recalculateBattleDelay ();
			_MESSAGE ("Command >> emcSetBattleDelay >> Min delay: %f, Extra delay: %f", battleDelay, extraBattleDelay);
			*result = 1;
		}
	UnlockHandle (hMusicPlayerMutex);

	if (CONSOLE) {
		Console_Print ("Set battle delay >> Min delay: %f, Extra delay: %f", battleDelay, extraBattleDelay);
	}
	return true;
}





bool Cmd_GetAfterBattleDelay_Execute (COMMAND_ARGS) {
	*result = 0;
	int extraAfterBattleDelay = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &extraAfterBattleDelay)) {
		return true;
	}

	LockHandle (hMusicPlayerMutex);
		switch (extraAfterBattleDelay) {
			case 2:	*result = (musicPlayer.getCalculatedAfterBattleDelay () / 1000); break;
			case 1: *result = (musicPlayer.getExtraAfterBattleDelay () / 1000); break;
			default: *result = (musicPlayer.getMinAfterBattleDelay () / 1000);
		}
	UnlockHandle (hMusicPlayerMutex);

	if (CONSOLE) {
		Console_Print ("After battle delay >> %f", *result);
	}
	return true;
}





bool Cmd_SetAfterBattleDelay_Execute (COMMAND_ARGS) {
	*result = 0;
	float afterBattleDelay = 0;
	float extraAfterBattleDelay = 0;
	int forceUpdate = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &afterBattleDelay, &extraAfterBattleDelay, &forceUpdate)) {
		return true;
	}
	if (afterBattleDelay < 0) {
		afterBattleDelay = 0;
	}
	if (extraAfterBattleDelay < 0) {
		extraAfterBattleDelay = 0;
	}

	LockHandle (hMusicPlayerMutex);
		bool changed1 = musicPlayer.setMinAfterBattleDelay (afterBattleDelay * 1000);
		bool changed2 = musicPlayer.setExtraAfterBattleDelay (extraAfterBattleDelay * 1000);
		if (changed1 || changed2 || forceUpdate != 0) {
			musicPlayer.recalculateAfterBattleDelay ();
			_MESSAGE ("Command >> emcSetAfterBattleDelay >> Min delay: %f, Extra delay: %f", afterBattleDelay, extraAfterBattleDelay);
			*result = 1;
		}
	UnlockHandle (hMusicPlayerMutex);

	if (CONSOLE) {
		Console_Print ("Set after battle delay >> Min delay: %f, Extra delay: %f", afterBattleDelay, extraAfterBattleDelay);
	}
	return true;
}





bool Cmd_GetMaxRestoreTime_Execute (COMMAND_ARGS) {
	LockHandle (hMusicPlayerMutex);
		*result = musicPlayer.getMaxRestoreTime () / 1000;
	UnlockHandle (hMusicPlayerMutex);
	if (CONSOLE) {
		Console_Print ("Max music restore time >> %f", *result);
	}
	return true;
}





bool Cmd_SetMaxRestoreTime_Execute (COMMAND_ARGS) {
	*result = 0;
	float restoreTime;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &restoreTime)) {
		return true;
	}
	if (restoreTime < 0) {
		restoreTime = 0;
	}

	LockHandle (hMusicPlayerMutex);
		bool changed = musicPlayer.setMaxRestoreTime (restoreTime * 1000);
	UnlockHandle (hMusicPlayerMutex);
	if (changed) {
		_MESSAGE ("Command >> emcSetMaxRestoreTime >> Restore: %f", restoreTime);
		*result = 1;
	}

	if (CONSOLE) {
		Console_Print ("Set max restore time >> Restore: %f", restoreTime);
	}
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
	
	threadRequest.requestHoldMusic (keepStopped > 0);

	LockHandle (hMusicPlayerMutex);
		if (musicPlayer.isStopped ()) {
			if (CONSOLE) {
				Console_Print ("Stop music >> Already stopped");
			}
			_MESSAGE ("Command >> emcMusicStop >> Already stopped");
		} else {
			if (fadeOut < 0) {
				fadeOut = musicPlayer.getCurrentFadeOutLength ();
			}
			musicPlayer.stop (fadeOut);
			fadeOut /= 100;
			if (CONSOLE) {
				Console_Print ("Stop music >> Fade Out: %f", fadeOut);
			}
			_MESSAGE ("Command >> emcMusicStop >> Fade Out: %f", fadeOut);
			*result = 1;
		}
	UnlockHandle (hMusicPlayerMutex);
	return true;
}





bool Cmd_MusicPause_Execute (COMMAND_ARGS) {
	*result = 0;
	float fadeOut = -1;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &fadeOut)) {
		return true;
	}

	LockHandle (hMusicPlayerMutex);
	if (musicPlayer.isInitialized ()) {
		if (musicPlayer.isPaused ()) {
			if (CONSOLE) {
				Console_Print ("Pause music >> Already paused");
			}
			_MESSAGE ("Command >> emcMusicPause >> Already paused");
		} else {
			if (fadeOut < 0) {
				fadeOut = musicPlayer.getCurrentFadeOutLength ();
			}
			musicPlayer.pause (fadeOut);
			fadeOut /= 1000;
			if (CONSOLE) {
				Console_Print ("Pause music >> Fade Out: %f", fadeOut);
			}
			_MESSAGE ("Command >> emcMusicPause >> Fade Out: %f", fadeOut);
			*result = 1;
		}
	}
	UnlockHandle (hMusicPlayerMutex);
	return true;
}





bool Cmd_MusicResume_Execute (COMMAND_ARGS) {
	*result = 0;
	float fadeIn = -1;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &fadeIn)) {
		return true;
	}

	LockHandle (hMusicPlayerMutex);
		if (musicPlayer.isPaused ()) {
			if (fadeIn < 0) {
				fadeIn = musicPlayer.getCurrentFadeInLength ();
			}
			musicPlayer.resume (fadeIn);
			fadeIn /= 1000;
			if (CONSOLE) {
				Console_Print ("Resume music >> Fade In: %f", fadeIn);
			}
			_MESSAGE ("Command >> emcMusicResume >> Fade In: %f", fadeIn);
			*result = 1;
		} else {
			if (CONSOLE) {
				Console_Print ("Resume music >> It's not paused");
			}
			_MESSAGE ("Command >> emcMusicResume >> It's not paused");
		}
	UnlockHandle (hMusicPlayerMutex);
	return true;
}





bool Cmd_MusicRestart_Execute (COMMAND_ARGS) {
	*result = 0;
	float fadeIn = -1;
	float fadeOut = -1;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &fadeOut, &fadeIn)) {
		return true;
	}

	LockHandle (hMusicPlayerMutex);
		*result = musicPlayer.isStopped () ? 1 : 0;
		if (fadeOut < 0) {
			fadeOut = musicPlayer.getCurrentFadeOutLength ();
		}
		if (fadeIn < 0) {
			fadeIn = musicPlayer.getCurrentFadeInLength ();
		}
		musicPlayer.restart (fadeOut, fadeIn);
		fadeOut /= 1000;
		fadeIn /= 1000;
		if (CONSOLE) {
			Console_Print ("Restart music >> Fade Out: %f | Fade In: %f", fadeOut, fadeIn);
		}
		_MESSAGE ("Command >> emcMusicRestart >> Fade Out %f | Fade In: %f", fadeOut, fadeIn);
	UnlockHandle (hMusicPlayerMutex);
	return true;
}





bool Cmd_MusicNextTrack_Execute (COMMAND_ARGS) {
	*result = 0;
	int noHold = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &noHold)) {
		return true;
	}

	LockHandle (hThreadMutex);
	threadRequest.Request_PlayNext = true;
	threadRequest.Request_PlayNext_MusicType = MusicType::Undefined;
	if (noHold) {
		threadRequest.Request_HoldMusic = false;
	}
	UnlockHandle (hThreadMutex);

	if (CONSOLE) {
		Console_Print ("Next track... to be decided!");
	}
	_MESSAGE ("Command >> emcMusicNextTrack");
	*result = 1;
	return true;
}