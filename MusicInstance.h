#pragma once

#include <map>
#include "MusicType.h"
#include "ActivePlaylist.h"
#include "TimeManagement.h"



struct MusicInstance {

	milliseconds lastValidTime;
	string playlistName;
	string trackPath;
	LONGLONG position = -ONE_SECOND;

	void setData (const string& curPlaylistName, const string& curTrackPath, LONGLONG curPosition, int maxRestoreTime);

};





void initMusicInstances ();

bool saveMusicInstance (MusicType musicType, const string &playlistName, const string& trackPath, LONGLONG position, LONGLONG duration);

bool getMusicInstance (MusicType musicType, string *varPlaylistName, string *varTrackPath, LONGLONG *varPosition);