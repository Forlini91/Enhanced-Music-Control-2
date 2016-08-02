#include "Commands.h"

#include <time.h>
#include <map>
#include <unordered_map>
#include "StringVar.h"
#include "ArrayVar.h"

#include "GlobalSettings.h"
#include "MusicState.h"
#include "MusicPlayer.h"
#include "Multiplier.h"
#include "ThreadRequest.h"
#include "ThreadState.h"
#include "FadeThread.h"
#include "PlayMusicFile.h"

using namespace std;



extern OBSEArrayVarInterface* g_arrayIntfc;
extern OBSEStringVarInterface* g_stringIntfc;
typedef OBSEArrayVarInterface::Array OBSEArray;

extern HANDLE hMusicTypeMutex;
extern HANDLE hPlaylistMutex;
extern HANDLE hThePlayerMutex;
extern HANDLE hSentryRequestMutex;
extern MusicPlayer musicPlayer;
extern MusicState music;
extern ThreadRequest threadRequest;
extern ThreadState threadState;
extern Playlist *plExplore;
extern Playlist *plPublic;
extern Playlist *plDungeon;
extern Playlist *plCustom;
extern Playlist *plBattle;
extern Playlist *plTitle;
extern Playlist *plDeath;
extern Playlist *plSuccess;
extern string vanillaPlaylistNames[];
extern unordered_map<string, Playlist> playlists;
extern unordered_map<string, Multiplier> multipliersVanilla;
extern unordered_map<string, Multiplier> multipliersCustom;
extern map <MusicType, string> musicTypes;
extern map <SpecialMusicType, string> specialMusicTypes;
extern map <MusicType, Playlist**> varsMusicType;
extern map <SpecialMusicType, Playlist**> varsSpecialMusicType;
extern int numPlaylists;
extern int numMultipliers;


extern MusicType getAssignedMusicType (const Playlist* playlist);
extern SpecialMusicType getAssignedSpecialMusicType (const Playlist* playlist);
string* getAssignedMusicTypeName (const Playlist* playlist) {
	MusicType musicType = getAssignedMusicType (playlist);
	if (musicType == MusicType::Mt_NotKnown) {
		return NULL;
	} else if (musicType != MusicType::Special) {
		return &musicTypes.at (musicType);
	} else {
		return &specialMusicTypes.at (getAssignedSpecialMusicType (playlist));
	}
}





//No input
//Returns the MusicType Oblivion wants to be played.
bool Cmd_GetMusicType_Execute (COMMAND_ARGS) {
	int mode = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &mode)) {
		return true;
	}

	WaitForSingleObject (hMusicTypeMutex, INFINITE);
	switch (mode) {
		case 0: *result = music.GetRealMusicType (); break;
		case 1: *result = music.GetWorldMusic (); break;
		case 2: *result = music.override; break;
		case 3: *result = music.special; break;
		case 4: *result = music.locked; break;
	}
	ReleaseMutex (hMusicTypeMutex);
	if (CONSOLE) {
		Console_Print ("Current music type >> %.0f", *result);
	}
	return true;
};





bool Cmd_SetMusicType_Execute (COMMAND_ARGS) {
	*result = 0;

	MusicType musicType = MusicType::Mt_NotKnown;
	int locked = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &musicType, &locked)) {
		return true;
	}

	if (musicType < -1 || musicType == 3 || musicType > 4) {
		if (CONSOLE) {
			Console_Print ("Set music type >> Failed: music type is not valid > Type: %d, Lock: %d", musicType, locked);
		}
		_MESSAGE ("Command >> emcSetMusicType >> Failed: music type is not valid > Type: %d, Lock: %d", musicType, locked);
		return true;
	}

	WaitForSingleObject (hMusicTypeMutex, INFINITE);
	if (locked < 0 || musicType < 0) {
		music.overrideMusic (MusicType::Mt_NotKnown, false);
	} else {
		music.overrideMusic (musicType, locked>0);
	}
	ReleaseMutex (hMusicTypeMutex);

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
	char plName[512];
	char plPaths[512];
	int shuffle = 1;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &plName, &plPaths, &shuffle)) {
		return true;
	}

	string name = string(plName);
	if (name.empty ()) {	//Empty name. Can't proceed
		if (CONSOLE) {
			Console_Print ("Playlist manager >> Failed. Name is empty");
		}
		_MESSAGE ("Command >> emcCreatePlaylist >> Failed. Name is empty");
		return true;
	}

	string paths = string (plPaths);
	auto it = playlists.find (name);
	if (it == playlists.end ()) {		//Create new playlist
		if (paths.empty ()) {				//Oh, wait, can't create a playlist without path
			if (CONSOLE) {
				Console_Print ("Playlist manager >> Failed. Path is empty");
			}
			_MESSAGE ("Command >> emcCreatePlaylist >> Failed. Path is empty for playlist \"%s\"", plName);
		} else if (playlists.size () >= numPlaylists) {
			if (CONSOLE) {
				Console_Print ("Playlist manager >> Failed. Max playlists limit reached: %d", numPlaylists);
			}
			_MESSAGE ("Command >> emcCreatePlaylist >> Failed. Max playlists limit reached: %d", numPlaylists);
		} else {
			WaitForSingleObject (hPlaylistMutex, INFINITE);
			try {
				auto insResult = playlists.emplace (BUILD_IN_PLACE (name, name, paths, shuffle != 0, false)).first;
				*result = 1;
				if (CONSOLE) {
					Console_Print ("Create playlist >> Succeed");
				}
				_MESSAGE ("Command >> emcCreatePlaylist >> Succeed: \"%s\" >> %s", plName, plPaths);
			} catch (exception e) {
				if (CONSOLE) {
					Console_Print ("Create playlist >> Failed. No track found.");
				}
				_MESSAGE ("Command >> emcCreatePlaylist >> Failed. No track found for playlist \"%s\" in %s", plName, plPaths);
			}
			ReleaseMutex (hPlaylistMutex);
		}
	} else {		//Alter/Delete existing playlist
		Playlist* playlist = &it->second;
		string* musicType;
		if (playlist->isVanilla ()) {						//Playlist is vanilla
			if (CONSOLE) {
				Console_Print ("Playlist manager >> Failed. This is a vanilla playlist and can't be altered");
			}
			_MESSAGE ("Command >> emcCreatePlaylist >> Failed. \"%s\" is a vanilla playlist and can't be altered", plName);
		} else if ((musicType = getAssignedMusicTypeName (playlist)) != NULL) {		//Playlist is assigned
			if (CONSOLE) {
				Console_Print ("Playlist manager >> Failed. This playlist can't be deleted, as it's assigned to music type %s", musicType->c_str());
			}
			_MESSAGE ("Command >> emcCreatePlaylist >> Failed. \"%s\" can't be deleted, as it's assigned to music type %s", plName, musicType->c_str ());
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
	char plName[512];
	char plPath[512];
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &plName, &plPath)) {
		return true;
	}

	string name = string (plName);
	string path = string (plPath);
	if (name.empty ()) {	//Empty name. Can't proceed
		if (CONSOLE) {
			Console_Print ("Playlist manager >> Failed. Name is empty");
		}
		_MESSAGE ("Command >> emcAddPath >> Failed. Name is empty");
		return true;
	} else if (path.empty ()) {
		if (CONSOLE) {
			Console_Print ("Playlist manager >> Failed. Path is empty");
		}
		_MESSAGE ("Command >> emcAddPath >> Failed. Path is empty for playlist \"%s\"", plName);
		return true;
	}

	auto it = playlists.find (name);
	if (it == playlists.end ()) {
		if (CONSOLE) {
			Console_Print ("Add path >> Failed. Playlist does not exist");
		}
		_MESSAGE ("Command >> emcAddPath >> Failed. Playlist does not exist");
	} else {
		Playlist* playlist = &it->second;
		string* musicType;
		//Make sure its not one of the default playlists.
		if (playlist->isVanilla ()) {
			if (CONSOLE) {
				Console_Print ("Add path >> Failed. This is a vanilla playlist and can't be altered");
			}
			_MESSAGE ("Command >> emcAddPath >> Failed. \"%s\" is a vanilla playlist and can't be altered", plName);
		} else if ((musicType = getAssignedMusicTypeName (playlist)) != NULL) {
			if (CONSOLE) {
				Console_Print ("Add path >> Failed. This playlist can't be altered, as it's assigned to a music type");
			}
			_MESSAGE ("Command >> emcAddPath >> Failed. \"%s\" can't be altered, as it's assigned to a music type ", plName);
			*result = -1;
		}
			
		Playlist backupPlaylist (playlist);		//Backup the playlis
		if (playlist->addPath (path)) {		//Try to add the new path
			if (CONSOLE) {
				Console_Print ("Add path >> Succeed. Playlist updated", plName, plPath);
			}
			_MESSAGE ("Command >> emcAddPath >> Succeed. Playlist updated: \"%s\" >> %s", plName, plPath);
			*result = 1;
		} else {			//Addition failed.
			backupPlaylist.copyTo (playlist);		//Restore the backed up playlist.
			if (CONSOLE) {
				Console_Print ("Add path >> Failed. No track found", plName, plPath);
			}
			_MESSAGE ("Command >> emcAddPath >> Failed. No track found for playlist \"%s\" in %s", plName, plPath);
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
	char plName[512];
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &plName)) {
		return true;
	}

	string name = string (plName);
	if (!name.empty ()) {
		if (playlists.find (name) != playlists.end ()) {
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
	char plName[512];
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &plName)) {
		return true;
	}
	string name = string (plName);
	if (!name.empty ()) {
		auto it = playlists.find (name);
		if (it != playlists.end ()) {
			Playlist* playlist = &it->second;
			if (playlist == plExplore || playlist == plPublic || playlist == plDungeon || playlist == plBattle) {
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

	WaitForSingleObject (hThePlayerMutex, INFINITE);
	bool isSwitching = musicPlayer.isSwitching ();
	ReleaseMutex (hThePlayerMutex);
	if (isSwitching) {
		return true;
	}

	//Parameters
	MusicType targetMT = MusicType::Explore;
	char plName[512];
	int queueMode = 0;
	float delay = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &targetMT, &plName, &queueMode, &delay)) {
		return true;
	}

	string name = string (plName);
	if (name.empty ()) {
		if (CONSOLE) {
			Console_Print ("Set playlist >> Failed. Name is empty");
		}
		_MESSAGE ("Command >> emcSetPlaylist >> Failed. Name is empty");
		return true;
	}

	auto it = playlists.find (name);
	if (it == playlists.end()) {
		if (CONSOLE) {
			Console_Print ("Set playlist >> Failed. Playlist does not exist: \"%s\"", plName);
		}
		_MESSAGE ("Command >> emcSetPlaylist >> Failed. Playlist does not exist: \"%s\"", plName);
		return true;
	}

	//Let other threads know the playlists are being manipulated.
	bool succeed = false;
	Playlist* playlist = &it->second;
	Playlist** variable = varsMusicType.at (targetMT);
	if (*variable != playlist) {
		WaitForSingleObject (hPlaylistMutex, INFINITE);
		threadRequest.requestSetPlaylist (playlist, targetMT, queueMode, delay);
		ReleaseMutex (hPlaylistMutex);
		*variable = playlist;
		succeed = true;
	}

	if (succeed){
		if (CONSOLE) {
			Console_Print ("Set playlist >> Succeed. Target: %d, Name: \"%s\", Mode: %d, Delay: %f", targetMT, plName, queueMode, delay);
		}
		_MESSAGE ("Command >> emcSetPlaylist >> Succeed. Target: %d, Name: \"%s\", Mode: %d, Delay: %f", targetMT, plName, queueMode, delay);
		*result = 1;
	} else {
		if (CONSOLE) {
			Console_Print ("Set playlist >> Failed. Target: %d, Name: \"%s\", Mode: %d, Delay: %f", targetMT, plName, queueMode, delay);
		}
		_MESSAGE ("Command >> emcSetPlaylist >> Failed. Target: %d, Name: \"%s\", Mode: %d, Delay: %f", targetMT, plName, queueMode, delay);
	}
	return true;
}





//Optional Input: 1 Int
//Return value has no meaning.
bool restorePlaylist (int i) {
	MusicType musicType = (MusicType) i;
	string& origPLName = vanillaPlaylistNames[i];
	WaitForSingleObject (hPlaylistMutex, INFINITE);
	Playlist** variable = varsMusicType.at (musicType);
	Playlist* defaultPL = &playlists.at (origPLName);
	if (*variable != defaultPL) {
		*variable = defaultPL;
//		curPlaylistNames[i] = origPLName;
		return true;
	} else {
		return false;
	}
}
bool Cmd_RestorePlaylist_Execute (COMMAND_ARGS) {
	*result = 0;

	WaitForSingleObject (hThePlayerMutex, INFINITE);
	bool isSwitching = musicPlayer.isSwitching ();
	ReleaseMutex (hThePlayerMutex);
	if (isSwitching) {
		return true;
	}

	//Parameters
	int targetMT = -1;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &targetMT)) {
		return true;
	}

	if (targetMT < 0) {		//Restore all.
		WaitForSingleObject (hPlaylistMutex, INFINITE);
		bool restored = false;
		for (int i = 0; i < 5; i++) {
			if (restorePlaylist (i)) {
				restored = true;
			}
		}
		ReleaseMutex (hPlaylistMutex);

		if (restored) {
			*result = 1;
			if (CONSOLE) {
				Console_Print ("Restore playlist >> Succeed. All music types restored");
			}
			_MESSAGE ("Command >> emcRestorePlaylist >> Succeed. All music types restored to default playlist");
		} else {
			if (CONSOLE) {
				Console_Print ("Restore playlist >> Failed");
			}
			_MESSAGE ("Command >> emcRestorePlaylist >> Failed. All music types are already using their default playlists", targetMT);
			return true;
		}
	} else if (targetMT <= 4) {
		WaitForSingleObject (hPlaylistMutex, INFINITE);
		bool restored = restorePlaylist (targetMT);
		ReleaseMutex (hPlaylistMutex);

		if (restored) {
			*result = 1;
			if (CONSOLE) {
				Console_Print ("Restore playlist >> Succeed. Music type restored");
			}
			_MESSAGE ("Command >> emcRestorePlaylist >> Succeed. Music type %d restored to default playlist", targetMT);
		} else {
			if (CONSOLE) {
				Console_Print ("Restore playlist >> Failed");
			}
			_MESSAGE ("Command >> emcRestorePlaylist >> Failed. Music type %d is already using the default playlist", targetMT);
			return true;
		}
	}

	if (targetMT < 0 || targetMT == music.GetRealMusicType ()) {
		WaitForSingleObject (hSentryRequestMutex, INFINITE);
		threadRequest.Request_PlayNext = true;
		threadRequest.Request_PlayNext_MusicType = (targetMT > -1) ? MusicType (targetMT) : MusicType::Undefined;
		ReleaseMutex (hSentryRequestMutex);
	}
	return true;
}





//No Input
//Returns a value indicating if the music can be manipulated.
bool Cmd_IsMusicSwitching_Execute (COMMAND_ARGS) {
	WaitForSingleObject (hThePlayerMutex, INFINITE);
	*result = musicPlayer.isSwitching () ? 0 : 1;
	ReleaseMutex (hThePlayerMutex);
	if (CONSOLE) {
		Console_Print ("Music switching >> %.0f", *result);
	}
	return true;
}





bool Cmd_GetAllPlaylists_Execute (COMMAND_ARGS) {
	*result = 0;
	OBSEArray* newArray = g_arrayIntfc->CreateArray (NULL, 0, scriptObj);
	WaitForSingleObject (hPlaylistMutex, INFINITE);
	for (auto it = playlists.cbegin (); it != playlists.cend (); it++) {
		g_arrayIntfc->AppendElement (newArray, (it->second).getName ().c_str ());
	}
	g_arrayIntfc->AssignCommandResult (newArray, result);
	if (CONSOLE) {
		for (auto it = playlists.cbegin (); it != playlists.cend (); it++) {
			Console_Print ((it->second).getName().c_str());
		}
	}
	ReleaseMutex (hPlaylistMutex);
	return true;
}





bool Cmd_GetPlaylist_Execute (COMMAND_ARGS) {
	*result = 0;
	int targetMT = -1;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &targetMT)) {
		return true;
	}

	Playlist* playlist = threadState.curPlaylist;
	const char* name = (playlist != NULL) ? playlist->getName ().c_str () : "Unknown ?!?";
	if (CONSOLE) {
		Console_Print ("Current playlist >> %s", playlist->getName().c_str());
	}
	if (playlist != NULL) {
		g_stringIntfc->Assign (PASS_COMMAND_ARGS, name);
	}
	return true;
}





bool Cmd_GetPlaylistTracks_Execute (COMMAND_ARGS) {
	*result = 0;
	//Parameters
	char plName[512];
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &plName)) {
		return true;
	}
	
	Playlist* playlist;
	string name = string (plName);
	WaitForSingleObject (hPlaylistMutex, INFINITE);
	if (name.empty()) {
		playlist = threadState.curPlaylist;
		if (playlist == NULL) {
			ReleaseMutex (hPlaylistMutex);
			if (CONSOLE) {
				Console_Print ("There's no active playlist");
			}
			return true;
		}
	} else {
		auto it = playlists.find (name);
		if (it == playlists.end ()) {
			ReleaseMutex (hPlaylistMutex);
			if (CONSOLE) {
				Console_Print ("There's no playlist named \"%s\"", plName);
			}
			return true;
		} else {
			playlist = &it->second;
		}
	}

	OBSEArray* newArray = g_arrayIntfc->CreateArray (NULL, 0, scriptObj);
	for (const string& track : playlist->getTracks ()) {
		g_arrayIntfc->AppendElement (newArray, track.c_str ());
	}
	g_arrayIntfc->AssignCommandResult (newArray, result);
	if (CONSOLE) {
		playlist->printTracks ();
	}
	ReleaseMutex (hPlaylistMutex);
	return true;
}





bool Cmd_GetTrackName_Execute (COMMAND_ARGS) {
	WaitForSingleObject (hThePlayerMutex, INFINITE);
	const string& songPath = musicPlayer.getTrack ();
	ReleaseMutex (hThePlayerMutex);
	if (CONSOLE) {
		Console_Print ("Currently track >> %s", songPath.c_str ());
	}
	g_stringIntfc->Assign (PASS_COMMAND_ARGS, songPath.c_str ());
	return true;
}





bool Cmd_GetTrackDuration_Execute (COMMAND_ARGS) {
	WaitForSingleObject (hThePlayerMutex, INFINITE);
	*result = musicPlayer.getTrackDuration () / ((double)10000000);
	ReleaseMutex (hThePlayerMutex);
	if (CONSOLE) {
		Console_Print ("Track duration >> %f", *result);
	}
	return true;
}





bool Cmd_GetTrackPosition_Execute (COMMAND_ARGS) {
	WaitForSingleObject (hThePlayerMutex, INFINITE);
	*result = musicPlayer.getTrackPosition () / ((double)10000000L);
	double duration = musicPlayer.getTrackDuration () / ((double)10000000L);
	ReleaseMutex (hThePlayerMutex);
	if (CONSOLE) {
		Console_Print ("Track position >> %.0f / %.0f", *result, duration);
	}
	return true;
}





bool Cmd_SetTrackPosition_Execute (COMMAND_ARGS) {
	*result = 0;
	float position = 0;
	int fadeTimeOut = 1000;
	int fadeTimeIn = 1000;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &position, &fadeTimeOut, &fadeTimeIn)) {
		return true;
	}

	REFERENCE_TIME longPos = position * ONCE_SECOND;
	WaitForSingleObject (hThePlayerMutex, INFINITE);
	REFERENCE_TIME longDur = musicPlayer.getTrackDuration ();
	if (longPos > longDur) {
		longPos = longDur;
	}
	musicPlayer.setTrackPosition (fadeTimeOut, fadeTimeIn, longPos);
	ReleaseMutex (hThePlayerMutex);
	if (CONSOLE) {
		Console_Print ("Set track position >> Position %.2f, Fade out/in: %f/%f", position, fadeTimeOut, fadeTimeIn);
	}
	_MESSAGE ("Command >> emcSetTrackPosition >> Position %.2f, Fade out/in: %f/%f", position, fadeTimeOut, fadeTimeIn);
	*result = 1;
	return true;
}





//Required input: None
//Returns 1 if the override is active, else 0.
bool Cmd_IsBattleOverridden_Execute (COMMAND_ARGS) {
	WaitForSingleObject (hMusicTypeMutex, INFINITE);
	*result = music.battleOverridden ? 1 : 0;
	ReleaseMutex (hMusicTypeMutex);
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
	WaitForSingleObject (hMusicTypeMutex, INFINITE);
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
	ReleaseMutex (hMusicTypeMutex);
	return true;
}





//Required input: None
//Returns 1 if the override is active, else 0.
bool Cmd_IsMusicOnHold_Execute (COMMAND_ARGS) {
	WaitForSingleObject (hSentryRequestMutex, INFINITE);
	*result = threadRequest.Request_HoldMusic ? 1 : 0;
	ReleaseMutex (hSentryRequestMutex);
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
	WaitForSingleObject (hSentryRequestMutex, INFINITE);
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
	ReleaseMutex (hSentryRequestMutex);
	return true;
}





bool Cmd_GetMasterVolume_Execute (COMMAND_ARGS) {
	*result = 0;
	int method = 0;
	int target = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &method, &target)) {
		return true;
	}

	Multiplier *mult = &multipliersVanilla.at(method == 0 ? obMaster : obMasterIni);
	WaitForSingleObject (mult->hThread, INFINITE);
	switch (target) {
		case 0: *result = mult->getValue (); break;
		case 1: *result = (mult->isFading ? mult->startValue : mult->getValue ()); break;
		case 2: *result = (mult->isFading ? mult->targetValue : mult->getValue ()); break;
		case 3: *result = (mult->isFading ? mult->fadeTime : -1); break;
		default: *result = mult->getValue ();
	}
	ReleaseMutex (mult->hThread);
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

	Multiplier *mult = &multipliersVanilla.at(method == 0 ? obMaster : obMasterIni);
	WaitForSingleObject (mult->hThread, INFINITE);
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
	bool custom = method >= 2;
	if (!custom) {
		mult = &multipliersVanilla.at (method == 0 ? obMusic : obMusicIni);
	} else {
		auto pos = multipliersCustom.find (key);
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

	WaitForSingleObject (mult->hThread, INFINITE);
	switch (target) {
		case 1: *result = (mult->isFading ? mult->startValue : mult->getValue ()); break;
		case 2: *result = (mult->isFading ? mult->targetValue : mult->getValue ()); break;
		case 3: *result = (mult->isFading ? mult->fadeTime : -1); break;
		case 4: *result = (!custom || mult->saveGame) ? 1 : 0; break;
		case 5: *result = (!custom || mult->saveSession) ? 1 : 0; break;
		default: *result = mult->getValue ();
	}
	ReleaseMutex (mult->hThread);
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

	char ckey[MAX_PATH];
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &volume, &method, &fadeTime, &ckey)) {
		return true;
	}

	Multiplier* mult = NULL;
	if (method < 2) {
		mult = &multipliersVanilla.at (method == 0 ? obMusic : obMusicIni);
		WaitForSingleObject (mult->hThread, INFINITE);
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
		ReleaseMutex (mult->hThread);
	} else {
		string key = string (ckey);
		if (key.empty ()) {
			if (CONSOLE) {
				Console_Print ("Set music volume >> Can't create/destroy a custom multiplier without a key");
				return true;
			}
		}

		_MESSAGE ("Command >> emcSetMusicVolume >> Method: %d, Key: %s", method, ckey);
		if (volume == 1.0) {
			auto pos = multipliersCustom.find (key);
			if (pos == multipliersCustom.end ()) {
				if (CONSOLE) {
					Console_Print ("Set music volume >> Can't create a new multiplier with volume 1.0");
				}
				_MESSAGE ("Command >> emcSetMusicVolume >> Can't create a new multiplier with volume 1.0");
				return true;
			} else if (fadeTime <= 0) {
				mult = &pos->second;
				WaitForSingleObject (mult->hThread, INFINITE);
				mult->isDestroyed = true;
				ReleaseMutex (mult->hThread);
				if (CONSOLE) {
					Console_Print ("Set music volume >> Multiplier destroyed");
				}
				_MESSAGE ("Command >> emcSetMusicVolume >> Multiplier destroyed");
			} return true;
		} else {
			if (multipliersCustom.size () >= numMultipliers) {
				if (CONSOLE) {
					Console_Print ("Set music volume >> Failed. Max multipliers limit reached: %d", numMultipliers);
				}
				_MESSAGE ("Command >> emcSetMusicVolume >> Failed. Max multipliers limit reached: %d", numMultipliers);
				return true;
			}
			auto insResult = multipliersCustom.emplace (BUILD_IN_PLACE (key, 1.0));
			mult = &insResult.first->second;
			if (insResult.second) {
				if (CONSOLE) {
					Console_Print ("Set music volume >> Create new multiplier");
				}
				_MESSAGE ("Command >> emcSetMusicVolume >> Create new multiplier");
			}
		}

		WaitForSingleObject (mult->hThread, INFINITE);
		mult->saveGame = (method == 3);
		mult->saveSession = (method == 4);
		mult->isDestroyed = false;
		if (fadeTime > 0) {
			if (CONSOLE) {
				_MESSAGE ("Set music volume >> Fade multiplier \"%s\" from %f to %f Time: %f", ckey, mult->getValue (), volume, fadeTime);
			}
			_MESSAGE ("Command >> emcSetMusicVolume >> Fade multiplier \"%s\" from %f to %f Time: %f", ckey, mult->getValue (), volume, fadeTime);
			mult->fadeVolume (volume, fadeTime);
		} else {
			if (CONSOLE) {
				Console_Print ("Set music volume >> Set multiplier \"%s\" from %f to %f", ckey, mult->getValue (), volume);
			}
			_MESSAGE ("Command >> emcSetMusicVolume >> Set multiplier \"%s\" from %f to %f", ckey, mult->getValue (), volume);
			mult->isFading = false;		//signal STOP to the thread running on this multiplier (if any)
			mult->setValue (volume);
		}
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

	Multiplier *mult = &multipliersVanilla.at (method == 0 ? obEffects : obEffectsIni);
	WaitForSingleObject (mult->hThread, INFINITE);
	switch (target) {
		case 0: *result = mult->getValue (); break;
		case 1: *result = (mult->isFading ? mult->startValue : mult->getValue ()); break;
		case 2: *result = (mult->isFading ? mult->targetValue : mult->getValue ()); break;
		case 3: *result = (mult->isFading ? mult->fadeTime : -1); break;
		default: *result = mult->getValue ();
	}
	ReleaseMutex (mult->hThread);
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

	Multiplier *mult = &multipliersVanilla.at(method == 0 ? obEffects : obEffectsIni);
	WaitForSingleObject (mult->hThread, INFINITE);
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

	Multiplier *mult = &multipliersVanilla.at (method == 0 ? obFoot : obFootIni);
	WaitForSingleObject (mult->hThread, INFINITE);
	switch (target) {
		case 0: *result = mult->getValue (); break;
		case 1: *result = (mult->isFading ? mult->startValue : mult->getValue ()); break;
		case 2: *result = (mult->isFading ? mult->targetValue : mult->getValue ()); break;
		case 3: *result = (mult->isFading ? mult->fadeTime : -1); break;
		default: *result = mult->getValue ();
	}
	ReleaseMutex (mult->hThread);
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

	Multiplier *mult = &multipliersVanilla.at (method == 0 ? obFoot : obFootIni);
	WaitForSingleObject (mult->hThread, INFINITE);
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

	Multiplier *mult = &multipliersVanilla.at (method == 0 ? obVoice : obVoiceIni);
	WaitForSingleObject (mult->hThread, INFINITE);
	switch (target) {
		case 0: *result = mult->getValue (); break;
		case 1: *result = (mult->isFading ? mult->startValue : mult->getValue ()); break;
		case 2: *result = (mult->isFading ? mult->targetValue : mult->getValue ()); break;
		case 3: *result = (mult->isFading ? mult->fadeTime : -1); break;
		default: *result = mult->getValue ();
	}
	ReleaseMutex (mult->hThread);
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

	Multiplier *mult = &multipliersVanilla.at (method == 0 ? obVoice : obVoiceIni);
	WaitForSingleObject (mult->hThread, INFINITE);
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
	ReleaseMutex (mult->hThread);
	*result = 1;
	return true;
}





bool Cmd_GetMusicSpeed_Execute (COMMAND_ARGS) {
	WaitForSingleObject (hThePlayerMutex, INFINITE);
	*result = musicPlayer.getMusicSpeed ();
	ReleaseMutex (hThePlayerMutex);
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
	WaitForSingleObject (hThePlayerMutex, INFINITE);
	bool changed = musicPlayer.setMusicSpeed ((double)speed);
	ReleaseMutex (hThePlayerMutex);
	if (CONSOLE) {
		Console_Print ("Set music speed >> Speed: %f", speed);
	}
	if (changed) {
		_MESSAGE ("Command >> emcSetMusicSpeed >> Speed: %f", speed);
		*result = 1;
	}
	return true;
}





bool Cmd_GetFadeTime_Execute (COMMAND_ARGS) {
	*result = 0;
	int fadeIn = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &fadeIn)) {
		return true;
	}

	WaitForSingleObject (hThePlayerMutex, INFINITE);
	*result = (fadeIn != 0 ? musicPlayer.getCurrentFadeInLength () : musicPlayer.getCurrentFadeOutLength ()) / 1000;
	ReleaseMutex (hThePlayerMutex);
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
		WaitForSingleObject (hThePlayerMutex, INFINITE);
		bool changed = musicPlayer.setCurrentFadeInLength (fadeTime * 1000.0);
		ReleaseMutex (hThePlayerMutex);
		if (changed) {
			_MESSAGE ("Command >> emcSetFadeTime >> Fade In: %f", fadeTime);
			*result = 1;
		}
		if (CONSOLE) {
			Console_Print ("Set fade time >> Fade In: %f", fadeTime);
		}
	} else {
		WaitForSingleObject (hThePlayerMutex, INFINITE);
		bool changed = musicPlayer.setCurrentFadeOutLength (fadeTime * 1000.0);
		ReleaseMutex (hThePlayerMutex);
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
	WaitForSingleObject (hThePlayerMutex, INFINITE);
	switch (extraPause) {
		case 2: *result = (musicPlayer.getCalculatedPauseTime() / 1000); break;
		case 1: *result = (musicPlayer.getExtraPauseTime () / 1000); break;
		default: *result = (musicPlayer.getMinPauseTime () / 1000);
	}
	ReleaseMutex (hThePlayerMutex);
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

	WaitForSingleObject (hThePlayerMutex, INFINITE);
	bool changed1 = musicPlayer.setMinPauseTime (pauseTime * 1000);
	bool changed2 = musicPlayer.setExtraPauseTime (extraPauseTime * 1000);
	if (changed1 || changed2 || forceUpdate != 0) {
		musicPlayer.recalculatePauseTime();
		_MESSAGE ("Command >> emcSetPauseTime >> Min pause: %f, Extra pause: %f", pauseTime, extraPauseTime);
		*result = 1;
	}
	ReleaseMutex (hThePlayerMutex);
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
	WaitForSingleObject (hThePlayerMutex, INFINITE);
	switch (extraBattleDelay) {
		case 2:	*result = (musicPlayer.getCalculatedBattleDelay () / 1000); break;
		case 1: *result = (musicPlayer.getExtraBattleDelay () / 1000); break;
		default: *result = (musicPlayer.getMinBattleDelay () / 1000);
	}
	ReleaseMutex (hThePlayerMutex);
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

	WaitForSingleObject (hThePlayerMutex, INFINITE);
	bool changed1 = musicPlayer.setMinBattleDelay (battleDelay * 1000);
	bool changed2 = musicPlayer.setExtraBattleDelay (extraBattleDelay * 1000);
	if (changed1 || changed2 || forceUpdate != 0) {
		musicPlayer.recalculateBattleDelay ();
		_MESSAGE ("Command >> emcSetBattleDelay >> Min delay: %f, Extra delay: %f", battleDelay, extraBattleDelay);
		*result = 1;
	}
	ReleaseMutex (hThePlayerMutex);
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
	WaitForSingleObject (hThePlayerMutex, INFINITE);
	switch (extraAfterBattleDelay) {
		case 2:	*result = (musicPlayer.getCalculatedAfterBattleDelay () / 1000); break;
		case 1: *result = (musicPlayer.getExtraAfterBattleDelay () / 1000); break;
		default: *result = (musicPlayer.getMinAfterBattleDelay () / 1000);
	}
	ReleaseMutex (hThePlayerMutex);
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

	WaitForSingleObject (hThePlayerMutex, INFINITE);
	bool changed1 = musicPlayer.setMinAfterBattleDelay (afterBattleDelay * 1000);
	bool changed2 = musicPlayer.setExtraAfterBattleDelay (extraAfterBattleDelay * 1000);
	if (changed1 || changed2 || forceUpdate != 0) {
		musicPlayer.recalculateAfterBattleDelay ();
		_MESSAGE ("Command >> emcSetAfterBattleDelay >> Min delay: %f, Extra delay: %f", afterBattleDelay, extraAfterBattleDelay);
		*result = 1;
	}
	ReleaseMutex (hThePlayerMutex);
	if (CONSOLE) {
		Console_Print ("Set after battle delay >> Min delay: %f, Extra delay: %f", afterBattleDelay, extraAfterBattleDelay);
	}
	return true;
}





bool Cmd_GetMaxRestoreTime_Execute (COMMAND_ARGS) {
	WaitForSingleObject (hThePlayerMutex, INFINITE);
	*result = musicPlayer.getMaxRestoreTime () / 1000;
	ReleaseMutex (hThePlayerMutex);
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

	WaitForSingleObject (hThePlayerMutex, INFINITE);
	bool changed = musicPlayer.setMaxRestoreTime (restoreTime * 1000);
	ReleaseMutex (hThePlayerMutex);
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
	if (_stricmp (path, "explore") == 0) {
		playMusicFile ("Data/Music/Explore/*");
	} else if (_stricmp (path, "public") == 0) {
		playMusicFile ("Data/Music/Public/*");
	} else if (_stricmp (path, "dungeon") == 0) {
		playMusicFile ("Data/Music/Dungeon/*");
	} else if (_stricmp (path, "battle") == 0) {
		playMusicFile ("Data/Music/Battle/*");
	} else if (_stricmp (path, "random") == 0) {
		srand ((unsigned)time (NULL));
		switch (rand () % 4) {
			case 0: playMusicFile ("Data/Music/Explore/*"); break;
			case 1: playMusicFile ("Data/Music/Public/*"); break;
			case 2: playMusicFile ("Data/Music/Dungeon/*"); break;
			case 3: playMusicFile ("Data/Music/Battle/*"); break;
		}
	} else {
		playMusicFile (path);
	}
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

	WaitForSingleObject (hSentryRequestMutex, INFINITE);
	threadRequest.Request_HoldMusic = (keepStopped > 0);
	ReleaseMutex (hSentryRequestMutex);

	WaitForSingleObject (hThePlayerMutex, INFINITE);
	if (musicPlayer.isInitialized ()) {
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
	}
	ReleaseMutex (hThePlayerMutex);
	return true;
}





bool Cmd_MusicPause_Execute (COMMAND_ARGS) {
	*result = 0;
	float fadeOut = -1;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &fadeOut)) {
		return true;
	}

	WaitForSingleObject (hThePlayerMutex, INFINITE);
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
	ReleaseMutex (hThePlayerMutex);
	return true;
}





bool Cmd_MusicResume_Execute (COMMAND_ARGS) {
	*result = 0;
	float fadeIn = -1;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &fadeIn)) {
		return true;
	}

	WaitForSingleObject (hThePlayerMutex, INFINITE);
	if (musicPlayer.isInitialized ()) {
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
	}
	ReleaseMutex (hThePlayerMutex);
	return true;
}





bool Cmd_MusicRestart_Execute (COMMAND_ARGS) {
	*result = 0;
	float fadeIn = -1;
	float fadeOut = -1;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &fadeOut, &fadeIn)) {
		return true;
	}

	WaitForSingleObject (hThePlayerMutex, INFINITE);
	if (musicPlayer.isInitialized ()) {
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
	}
	ReleaseMutex (hThePlayerMutex);
	return true;
}





bool Cmd_MusicNextTrack_Execute (COMMAND_ARGS) {
	*result = 0;
	int noHold = 0;
	if (!ExtractArgs (PASS_EXTRACT_ARGS, &noHold)) {
		return true;
	}

	WaitForSingleObject (hSentryRequestMutex, INFINITE);
	threadRequest.Request_PlayNext = true;
	threadRequest.Request_PlayNext_MusicType = MusicType::Undefined;
	if (noHold) {
		threadRequest.Request_HoldMusic = false;
	}
	ReleaseMutex (hSentryRequestMutex);

	if (CONSOLE) {
		Console_Print ("Next track... to be decided!");
	}
	_MESSAGE ("Command >> emcMusicNextTrack");
	*result = 1;
	return true;
}