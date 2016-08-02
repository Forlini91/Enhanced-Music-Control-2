#pragma once

#include "CommandTable.h"
#include "GameAPI.h"
#include "Multiplier.h"
#include "MusicType.h"
#include "Playlist.h"






//No input
//Returns the MusicType Oblivion wants to be played.
bool Cmd_GetMusicType_Execute (COMMAND_ARGS);
static ParamInfo kParams_GetMusicType[1] = {
	{"mode", kParamType_Integer, 1},
};
static CommandInfo kGetMusicTypeCommand = {
	"emcGetMusicType",
	"",
	0,
	"Gets the music type Oblivion wishes to play",
	0,
	1,
	kParams_GetMusicType,
	Cmd_GetMusicType_Execute
};



bool Cmd_SetMusicType_Execute (COMMAND_ARGS);
static ParamInfo kParams_SetMusicType[2] = {
	{"music type", kParamType_Integer, 0},
	{"lock mode", kParamType_Integer, 1},
};
static CommandInfo kSetMusicTypeCommand = {
	"emcSetMusicType",
	"",
	0,
	"Temporary override the current world music type with the given one. To disable the override, pass -1 as first parameter.",
	0,
	2,
	kParams_SetMusicType,
	Cmd_SetMusicType_Execute
};



//Required Input: 2 Strings
//Returns 1 on success, 0 on failure.
bool Cmd_CreatePlaylist_Execute (COMMAND_ARGS);
static ParamInfo kParams_CreatePlaylist[3] = {
	{"playlist name", kParamType_String, 0},
	{"music path(s)", kParamType_String, 0},
	{"random order", kParamType_Integer, 1},
};
static CommandInfo kCreatePlaylistCommand = {
	"emcCreatePlaylist",
	"",
	0,
	"Creates a new playlist or update an existing one with the specified name and path. Return 1 if succeed, -1 if playlist exists but it's active so it can't be recreated, 0 if name or path are bad",
	0,
	3,
	kParams_CreatePlaylist,
	Cmd_CreatePlaylist_Execute
};



//Required Input: 2 Strings
//Returns 1 on success, 0 on failure.
bool Cmd_AddPathToPlaylist_Execute (COMMAND_ARGS);
static ParamInfo kParams_AddPathToPlaylist[2] = {
	{"playlist name", kParamType_String, 0},
	{"music path", kParamType_String, 0},
};
static CommandInfo kAddPathToPlaylistCommand = {
	"emcAddPathToPlaylist",
	"emcAddPath",
	0,
	"Adds a single, new path to an existing playlist. Return 1 if succeed, 0 otherwise.",
	0,
	2,
	kParams_AddPathToPlaylist,
	Cmd_AddPathToPlaylist_Execute
};



//Required Input: 1 String
//Returns 1 is the playlist was found, else 0.
bool Cmd_PlaylistExists_Execute (COMMAND_ARGS);
static ParamInfo kParams_PlaylistExists[1] = {
	{"playlist name", kParamType_String, 0},
};
static CommandInfo kPlaylistExistsCommand = {
	"emcPlaylistExists",
	"",
	0,
	"Returns 1 if the specified playlist exists, 0 otherwise.",
	0,
	1,
	kParams_PlaylistExists,
	Cmd_PlaylistExists_Execute
};



//Required Input: 1 String
//Returns 1 is the playlist is assigned to a MusicType, else 0.
bool Cmd_IsPlaylistActive_Execute (COMMAND_ARGS);
static ParamInfo kParams_IsPlaylistActive[1] = {
	{"playlist name", kParamType_String, 0},
};
static CommandInfo kIsPlaylistActiveCommand = {
	"emcIsPlaylistActive",
	"",
	0,
	"Returns 1 if the specified playlist is currently assigned to a MusicType, 0 otherwise.",
	0,
	1,
	kParams_IsPlaylistActive,
	Cmd_IsPlaylistActive_Execute
};



void Cmd_SetPlaylist_Execute2 (const char* plName, MusicType targetMT, int queueMode, float delay, int i, Playlist **playlist);
bool Cmd_SetPlaylist_Execute (COMMAND_ARGS);
static ParamInfo kParams_SetPlaylist[4] = {
	{"music type", kParamType_Integer, 0},
	{"playlist name", kParamType_String, 0},
	{"queue mode", kParamType_Integer, 1},
	{"delay", kParamType_Float, 1},
};
static CommandInfo kSetPlaylistCommand = {
	"emcSetPlaylist",
	"",
	0,
	"Set the playlist for the specified MusicType. Return 1 if succeed, 0 otherwise.",
	0,
	4,
	kParams_SetPlaylist,
	Cmd_SetPlaylist_Execute
};



//Optional Input: 1 Int
//Return value has no meaning.
bool Cmd_RestorePlaylist_Execute (COMMAND_ARGS);
static ParamInfo kParams_RestorePlaylist[1] = {
	{"music type", kParamType_Integer, 1},
};
static CommandInfo kRestorePlaylistCommand = {
	"emcRestorePlaylist",
	"",
	0,
	"Restores the specified music type to the default Oblivion playlist (pass -1 restores all to default). Return 1 if succeed, 0 otherwise.",
	0,
	1,
	kParams_RestorePlaylist,
	Cmd_RestorePlaylist_Execute
};



//No Input
//Returns a value indicating if the music can be manipulated.
bool Cmd_IsMusicSwitching_Execute (COMMAND_ARGS);
static CommandInfo kIsMusicSwitchingCommand = {
	"emcIsMusicSwitching",
	"emcIsMS",
	0,
	"Return 1 if the music player is busy and can't recieve music altering commands, 0 otherwise.",
	0,
	0,
	NULL,
	Cmd_IsMusicSwitching_Execute
};



bool Cmd_GetAllPlaylists_Execute (COMMAND_ARGS);
static CommandInfo kGetAllPlaylistsCommand = {
	"emcGetAllPlaylists",
	"",
	0,
	"Gets the names of all playlists",
	0,
	0,
	NULL,
	Cmd_GetAllPlaylists_Execute
};



bool Cmd_GetPlaylist_Execute (COMMAND_ARGS);
static ParamInfo kParams_GetPlaylist[1] = {
	{"music type", kParamType_Integer, 1},
};
static CommandInfo kGetPlaylistCommand = {
	"emcGetPlaylist",
	"",
	0,
	"Gets the name of the playlist assigned to the given MusicType. If no MusicType is given, use the current one.",
	0,
	1,
	kParams_GetPlaylist,
	Cmd_GetPlaylist_Execute
};



bool Cmd_GetPlaylistTracks_Execute (COMMAND_ARGS);
static ParamInfo kParams_GetPlaylistTracks[1] = {
	{"playlist name", kParamType_String, 1},
};
static CommandInfo kGetPlaylistTracksCommand = {
	"emcGetPlaylistTracks",
	"emcGetTracks",
	0,
	"Gets the names of the given playlist tracks. If no playlist is given, use the current one.",
	0,
	1,
	kParams_GetPlaylistTracks,
	Cmd_GetPlaylistTracks_Execute
};



bool Cmd_GetTrackName_Execute (COMMAND_ARGS);
static CommandInfo kGetTrackNameCommand = {
	"emcGetTrackName",
	"emcGetTrack",
	0,
	"Gets the name of the current track.",
	0,
	0,
	NULL,
	Cmd_GetTrackName_Execute
};



bool Cmd_GetTrackDuration_Execute (COMMAND_ARGS);
static CommandInfo kGetTrackDurationCommand = {
	"emcGetTrackDuration",
	"",
	0,
	"Gets the duration of the current playing track.",
	0,
	0,
	NULL,
	Cmd_GetTrackDuration_Execute
};



bool Cmd_GetTrackPosition_Execute (COMMAND_ARGS);
static CommandInfo kGetTrackPositionCommand = {
	"emcGetTrackPosition",
	"",
	0,
	"Gets the position of the current playing track.",
	0,
	0,
	NULL,
	Cmd_GetTrackPosition_Execute
};



bool Cmd_SetTrackPosition_Execute (COMMAND_ARGS);
static ParamInfo kParams_SetTrackPosition[3] = {
	{"at second", kParamType_Float, 0},
	{"fade out", kParamType_Integer, 1},
	{"fade in", kParamType_Integer, 1},
};
static CommandInfo kSetTrackPositionCommand = {
	"emcSetTrackPosition",
	"",
	0,
	"Set the position of the current playing track. Return 1 if succeed, 0 otherwise.",
	0,
	3,
	kParams_SetTrackPosition,
	Cmd_SetTrackPosition_Execute
};



//Required input: None
//Returns 1 if the override is active, else 0.
bool Cmd_IsBattleOverridden_Execute (COMMAND_ARGS);
static CommandInfo kIsBattleOverriddenCommand = {
	"emcIsBattleOverridden",
	"",
	0,
	"Returns 1 if the battle music override is active, 0 otherwise.",
	0,
	0,
	NULL,
	Cmd_IsBattleOverridden_Execute
};



//Required input: None
//Returns 1 if the override was set, else 0.
bool Cmd_SetBattleOverride_Execute (COMMAND_ARGS);
static ParamInfo kParams_SetBattleOverride[3] = {
	{"override", kParamType_Integer, 0},
};
static CommandInfo kSetBattleOverrideCommand = {
	"emcSetBattleOverride",
	"",
	0,
	"Set the battle music override. Return 1 if override has been changed, 0 otherwise.",
	0,
	1,
	kParams_SetBattleOverride,
	Cmd_SetBattleOverride_Execute
};



//Required input: None
//Returns 1 if the override is active, else 0.
bool Cmd_IsMusicOnHold_Execute (COMMAND_ARGS);
static CommandInfo kIsMusicOnHoldCommand = {
	"emcIsMusicOnHold",
	"",
	0,
	"Returns 1 if the music player is currently on hold, 0 otherwise.",
	0,
	0,
	NULL,
	Cmd_IsMusicOnHold_Execute
};



//Required input: None
//Returns 1 if the hold was engaged, else 0.
bool Cmd_SetMusicHold_Execute (COMMAND_ARGS);
static ParamInfo kParams_SetMusicHold[3] = {
	{"hold", kParamType_Integer, 0},
};
static CommandInfo kSetMusicHoldCommand = {
	"emcSetMusicHold",
	"",
	0,
	"Hold the music player. It won't enqueue a new tracks after the current one. Return 1 if the hold status has been changed, 0 otherwise.",
	0,
	1,
	kParams_SetMusicHold,
	Cmd_SetMusicHold_Execute
};








static ParamInfo kParams_GetXVolume[2] = {
	{"read from ini", kParamType_Integer, 1},
	{"which value", kParamType_Integer, 1},
};
static ParamInfo kParams_SetXVolume[3] = {
	{"volume", kParamType_Float, 0},
	{"save to ini", kParamType_Integer, 1},
	{"fade time", kParamType_Float, 1},
};



bool Cmd_GetMasterVolume_Execute (COMMAND_ARGS);
static CommandInfo kGetMasterVolumeCommand = {
	"emcGetMasterVolume",
	"",
	0,
	"Get the master volume.",
	0,
	2,
	kParams_GetXVolume,
	Cmd_GetMasterVolume_Execute
};



bool Cmd_SetMasterVolume_Execute (COMMAND_ARGS);
static CommandInfo kSetMasterVolumeCommand = {
	"emcSetMasterVolume",
	"",
	0,
	"Set the master volume.",
	0,
	3,
	kParams_SetXVolume,
	Cmd_SetMasterVolume_Execute
};



bool Cmd_GetMusicVolume_Execute (COMMAND_ARGS);
static ParamInfo kParams_GetMusicVolume[3] = {
	{"read mode", kParamType_Integer, 1},
	{"which value", kParamType_Integer, 1},
	{"multiplier ID", kParamType_String, 1},
};
static CommandInfo kGetMusicVolumeCommand = {
	"emcGetMusicVolume",
	"",
	0,
	"Get the music volume.",
	0,
	3,
	kParams_GetMusicVolume,
	Cmd_GetMusicVolume_Execute
};



bool Cmd_SetMusicVolume_Execute (COMMAND_ARGS);
static ParamInfo kParams_SetMusicVolume[4] = {
	{"volume", kParamType_Float, 0},
	{"save mode", kParamType_Integer, 1},
	{"fade time", kParamType_Float, 1},
	{"multiplier ID", kParamType_String, 1},
};
static CommandInfo kSetMusicVolumeCommand = {
	"emcSetMusicVolume",
	"",
	0,
	"Set the music volume.",
	0,
	4,
	kParams_SetMusicVolume,
	Cmd_SetMusicVolume_Execute
};



bool Cmd_GetEffectsVolume_Execute (COMMAND_ARGS);
static CommandInfo kGetEffectsVolumeCommand = {
	"emcGetEffectsVolume",
	"",
	0,
	"Get the effects volume.",
	0,
	2,
	kParams_GetXVolume,
	Cmd_GetEffectsVolume_Execute
};



bool Cmd_SetEffectsVolume_Execute (COMMAND_ARGS);
static CommandInfo kSetEffectsVolumeCommand = {
	"emcSetEffectsVolume",
	"",
	0,
	"Set the effects volume.",
	0,
	3,
	kParams_SetXVolume,
	Cmd_SetEffectsVolume_Execute
};



bool Cmd_GetFootVolume_Execute (COMMAND_ARGS);
static CommandInfo kGetFootVolumeCommand = {
	"emcGetFootVolume",
	"",
	0,
	"Get the foot volume.",
	0,
	2,
	kParams_GetXVolume,
	Cmd_GetFootVolume_Execute
};



bool Cmd_SetFootVolume_Execute (COMMAND_ARGS);
static CommandInfo kSetFootVolumeCommand = {
	"emcSetFootVolume",
	"",
	0,
	"Set the foot volume.",
	0,
	3,
	kParams_SetXVolume,
	Cmd_SetFootVolume_Execute
};



bool Cmd_GetVoiceVolume_Execute (COMMAND_ARGS);
static CommandInfo kGetVoiceVolumeCommand = {
	"emcGetVoiceVolume",
	"",
	0,
	"Get the voice volume.",
	0,
	2,
	kParams_GetXVolume,
	Cmd_GetVoiceVolume_Execute
};



bool Cmd_SetVoiceVolume_Execute (COMMAND_ARGS);
static CommandInfo kSetVoiceVolumeCommand = {
	"emcSetVoiceVolume",
	"",
	0,
	"Set the voice volume.",
	0,
	3,
	kParams_SetXVolume,
	Cmd_SetVoiceVolume_Execute
};



bool Cmd_GetMusicSpeed_Execute (COMMAND_ARGS);
static CommandInfo kGetMusicSpeedCommand = {
	"emcGetMusicSpeed",
	"",
	0,
	"Get the music speed.",
	0,
	0,
	NULL,
	Cmd_GetMusicSpeed_Execute
};



bool Cmd_SetMusicSpeed_Execute (COMMAND_ARGS);
static ParamInfo kParams_SetMusicSpeed[1] = {
	{"speed", kParamType_Float, 0},
};
static CommandInfo kSetMusicSpeedCommand = {
	"emcSetMusicSpeed",
	"",
	0,
	"Set the music speed.",
	0,
	1,
	kParams_SetMusicSpeed,
	Cmd_SetMusicSpeed_Execute
};



bool Cmd_GetFadeTime_Execute (COMMAND_ARGS);
static ParamInfo kParams_GetFadeTime[1] = {
	{"get fade in", kParamType_Integer, 0},
};
static CommandInfo kGetFadeTimeCommand = {
	"emcGetFadeTime",
	"",
	0,
	"Get the fade out or fade in time for the tracks.",
	0,
	1,
	kParams_GetFadeTime,
	Cmd_GetFadeTime_Execute
};



bool Cmd_SetFadeTime_Execute (COMMAND_ARGS);
static ParamInfo kParams_SetFadeTime[2] = {
	{"fade time", kParamType_Float, 0},
	{"set fade in", kParamType_Integer, 1},
};
static CommandInfo kSetFadeTimeCommand = {
	"emcSetFadeTime",
	"",
	0,
	"Set the fade out or fade in time for the tracks.",
	0,
	2,
	kParams_SetFadeTime,
	Cmd_SetFadeTime_Execute
};



bool Cmd_GetPauseTime_Execute (COMMAND_ARGS);
static ParamInfo kParams_GetPauseTime[1] = {
	{"get extra pause", kParamType_Integer, 1},
};
static CommandInfo kGetPauseTimeCommand = {
	"emcGetPauseTime",
	"",
	0,
	"Get the pause time between 2 tracks.",
	0,
	1,
	kParams_GetPauseTime,
	Cmd_GetPauseTime_Execute
};



bool Cmd_SetPauseTime_Execute (COMMAND_ARGS);
static ParamInfo kParams_SetPauseTime[3] = {
	{"min time", kParamType_Float, 0},
	{"extra time", kParamType_Float, 1},
	{"force update", kParamType_Integer, 1},
};
static CommandInfo kSetPauseTimeCommand = {
	"emcSetPauseTime",
	"",
	0,
	"Set the pause time between 2 tracks.",
	0,
	3,
	kParams_SetPauseTime,
	Cmd_SetPauseTime_Execute
};



bool Cmd_GetBattleDelay_Execute (COMMAND_ARGS);
static ParamInfo kParams_GetBattleDelay[1] = {
	{"get extra delay", kParamType_Integer, 1},
};
static CommandInfo kGetBattleDelayCommand = {
	"emcGetBattleDelay",
	"",
	0,
	"Get how many seconds the battle must last before the battle music start.",
	0,
	1,
	kParams_GetBattleDelay,
	Cmd_GetBattleDelay_Execute
};



bool Cmd_SetBattleDelay_Execute (COMMAND_ARGS);
static ParamInfo kParams_SetBattleDelay[3] = {
	{"min delay", kParamType_Float, 0},
	{"extra delay", kParamType_Float, 1},
	{"force update", kParamType_Integer, 1},
};
static CommandInfo kSetBattleDelayCommand = {
	"emcSetBattleDelay",
	"",
	0,
	"Set how many seconds the battle must last before the battle music start (Min: 0.5 secs).",
	0,
	3,
	kParams_SetBattleDelay,
	Cmd_SetBattleDelay_Execute
};



bool Cmd_GetAfterBattleDelay_Execute (COMMAND_ARGS);
static ParamInfo kParams_GetAfterBattleDelay[1] = {
	{"get extra delay", kParamType_Integer, 1},
};
static CommandInfo kGetAfterBattleDelayCommand = {
	"emcGetAfterBattleDelay",
	"",
	0,
	"Get how many seconds must pass after battle ended before the battle music ends.",
	0,
	1,
	kParams_GetAfterBattleDelay,
	Cmd_GetAfterBattleDelay_Execute
};



bool Cmd_SetAfterBattleDelay_Execute (COMMAND_ARGS);
static ParamInfo kParams_SetAfterBattleDelay[3] = {
	{"min delay", kParamType_Float, 0},
	{"extra delay", kParamType_Float, 1},
	{"force update", kParamType_Integer, 1},
};
static CommandInfo kSetAfterBattleDelayCommand = {
	"emcSetAfterBattleDelay",
	"",
	0,
	"Set how many seconds must pass after battle ended before the battle music ends.",
	0,
	3,
	kParams_SetAfterBattleDelay,
	Cmd_SetAfterBattleDelay_Execute
};



bool Cmd_GetMaxRestoreTime_Execute (COMMAND_ARGS);
static CommandInfo kGetMaxRestoreTimeCommand = {
	"emcGetMaxRestoreTime",
	"",
	0,
	"Get how many seconds the player remember the previous playlist.",
	0,
	0,
	NULL,
	Cmd_GetMaxRestoreTime_Execute
};



bool Cmd_SetMaxRestoreTime_Execute (COMMAND_ARGS);
static ParamInfo kParams_SetMaxRestoreTime[1] = {
	{"max restore time", kParamType_Float, 0},
};
static CommandInfo kSetMaxRestoreTimeCommand = {
	"emcSetMaxRestoreTime",
	"",
	0,
	"Set how many seconds the player remember the previous playlist.",
	0,
	1,
	kParams_SetMaxRestoreTime,
	Cmd_SetMaxRestoreTime_Execute
};



bool Cmd_PlayTrack_Execute (COMMAND_ARGS);
static ParamInfo kParams_PlayTrack[1] = {
	{"track full path name", kParamType_String, 0},
};
static CommandInfo kPlayTrackCommand = {
	"emcPlayTrack",
	"",
	0,
	"Play the given track then resume the previous playlist.",
	0,
	1,
	kParams_PlayTrack,
	Cmd_PlayTrack_Execute
};



bool Cmd_MusicStop_Execute (COMMAND_ARGS);
static ParamInfo kParams_MusicStop[2] = {
	{"fade out time", kParamType_Float, 1},
	{"keep stopped whatever happens", kParamType_Integer, 1},
};
static CommandInfo kMusicStopCommand = {
	"emcMusicStop",
	"",
	0,
	"The music player stops.",
	0,
	2,
	kParams_MusicStop,
	Cmd_MusicStop_Execute
};



bool Cmd_MusicPause_Execute (COMMAND_ARGS);
static ParamInfo kParams_MusicPause[1] = {
	{"fade out time", kParamType_Float, 1},
};
static CommandInfo kMusicPauseCommand = {
	"emcMusicPause",
	"",
	0,
	"The music player pause the current track.",
	0,
	1,
	kParams_MusicPause,
	Cmd_MusicPause_Execute
};



bool Cmd_MusicResume_Execute (COMMAND_ARGS);
static ParamInfo kParams_MusicResume[1] = {
	{"fade in time", kParamType_Float, 1},
};
static CommandInfo kMusicResumeCommand = {
	"emcMusicResume",
	"",
	0,
	"The music player resume the current track (only if paused).",
	0,
	1,
	kParams_MusicResume,
	Cmd_MusicResume_Execute
};



bool Cmd_MusicRestart_Execute (COMMAND_ARGS);
static ParamInfo kParams_MusicRestart[2] = {
	{"fade out time", kParamType_Float, 1},
	{"fade in time", kParamType_Float, 1},
};
static CommandInfo kMusicRestartCommand = {
	"emcMusicRestart",
	"",
	0,
	"The music player play (if stopped) and restart the last track.",
	0,
	2,
	kParams_MusicRestart,
	Cmd_MusicRestart_Execute
};



bool Cmd_MusicNextTrack_Execute (COMMAND_ARGS);
static ParamInfo kParams_MusicNextTrack[1] = {
	{"restart if paused or stopped", kParamType_Integer, 1},
};
static CommandInfo kMusicNextTrackCommand = {
	"emcMusicNextTrack",
	"",
	0,
	"The music player play the next track in the playlist.",
	0,
	1,
	kParams_MusicNextTrack,
	Cmd_MusicNextTrack_Execute
};