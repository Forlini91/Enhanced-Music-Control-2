#pragma once

#include <string>
#include "Playlist.h"
#include "MusicType.h"



using namespace std;

struct ActivePlaylist {

	const char* name;							//Name of the playlist "slot"
	const MusicType musicType;					//Music type of this "slot"
	const SpecialMusicType specialMusicType;	//SpecialMusicType of this "slot"
	Playlist* playlist = NULL;					//Current playlist associated to this "slot"


	ActivePlaylist (const char* name, MusicType musicType, SpecialMusicType specialMusicType);

	void operator= (Playlist* playlist);
	void operator= (const ActivePlaylist& apl);
	void operator= (const ActivePlaylist* apl);
	void operator+= (const string& path);

};



bool operator== (const ActivePlaylist& apl1, const ActivePlaylist& apl2);
bool operator== (const ActivePlaylist& apl, const Playlist* playlist);
bool operator== (const ActivePlaylist& apl, const Playlist* playlist);
bool operator== (const Playlist* playlist, const ActivePlaylist& activePlaylist);
bool operator== (const ActivePlaylist& apl, const Playlist& playlist);



extern ActivePlaylist apl_Explore;		//The current active Explore playlist
extern ActivePlaylist apl_Public;		//The current active Public playlist
extern ActivePlaylist apl_Dungeon;		//The current active Dungeon playlist
extern ActivePlaylist apl_Custom;		//The current active Custom playlist
extern ActivePlaylist apl_Battle;		//The current active Battle playlist
extern ActivePlaylist apl_Death;		//The current active Death playlist
extern ActivePlaylist apl_Success;		//The current active Success playlist
extern ActivePlaylist apl_Title;		//The current active Title playlist
extern ActivePlaylist* activePlaylists[8];		//All current active playlists


//Checks whatever the playlist is active
extern bool isPlaylistActive (Playlist* playlist);

//Returns the ActivePlaylist with the given playlist, if any, else returns NULL
extern ActivePlaylist* getActivePlaylist (Playlist* playlist);

//Returns the ActivePlaylist with the given MusicType, if any
extern ActivePlaylist* getActivePlaylist (MusicType musicType);

//Returns the ActivePlaylist with the given SpecialMusicType, if any
extern ActivePlaylist* getActivePlaylist (SpecialMusicType specialMusicType);

//Check whatever they are the same music type, or the associated active playlists are the same.
extern bool samePlaylist (MusicType musicType1, MusicType musicType2);