#pragma once

#include "atlstr.h"
#include <string>
#include <deque>
#include <vector>
#include <unordered_map>

#include "MusicType.h"




using namespace std;



class Playlist {

private:

	//The array containing the paths of the tracks in the list.
	//Everytime curIndex cycles through the list, it will be reordered,
	vector <string> tracks;

	//The directory containing the music, if we ever need it.
	//nsPath::CPath TargetPath;
	vector <string> paths;

	//If true, tracks are shuffled on creation/recreation/repeat, else they always maintain the starting order
	bool randomOrder;

	//The index the playlist is currently at.  If no song has been played in
	//this list, the value will be -1.
	int curIndex = -1;

	//Is playlist initialized?
	bool initialized = false;

	//If true, this is a vanilla playlist
	const bool vanillaPlaylist;



	//Tokenizes the path.
	vector<string> tokenizePaths (const string& path) const;

	//Randomly reorders the playlist, guaranteeing that the last played song
	//will not come first in the playlist.
	void sortTracks (bool updated);

	//This function builds the playlist from the value held in TargetPath.
	bool buildPaths (const string& paths);

	bool buildPath (const string& path, bool add);



public:

	//The name of this playlist.  This value may be used later to increase the capabilities
	//of this music system.  It can be accessed by friendly classes to help manage a collection.
	const char* name;


	Playlist (const Playlist* copyFrom);

	Playlist (const char* name, bool vanillaPlaylist);

	Playlist (const char* name, const string& paths, bool randomOrder, bool vanillaPlaylist);

	//Set the given paths and build the tracks/paths vectors.
	bool setPaths (const string& paths, bool randomOrder);

	//Adds a SINGLE path to the playlist, and reinitializes it.
	bool addPath (const string& path);

	//Gets all tracks in the playlist
	const vector<string>& Playlist::getTracks () const;

	//Number of tracks in the playlist
	int size () const;

	//Should be checked before use to ensure you're trying to run with a bad playlist.
	bool isInitialized () const;

	//Return true if this is a vanilla playlist (and so, it can't be altered).
	bool isVanilla () const;

	//Advances the playlist by one, and returns a (const char *) pointing to the
	//file path.
	const string next ();

	//Returns the current song.  If there is no current song, it advances the
	//track by one so there IS a song.
	const string current ();

	//Createsa  copy of the current playlist into the Target.
	void copyTo (Playlist *copyTo) const;

	void printTracks () const;

	

	bool operator== (const Playlist& playlist) const;

	bool operator== (const Playlist* playlist) const;

};


typedef unordered_map<string, Playlist> PlaylistsMap;
typedef pair<PlaylistsMap::iterator, bool> PlaylistEmplaceResult;


extern HANDLE hPlaylistMutex;
extern Playlist* vanillaPlaylists[8];
extern PlaylistsMap playlists;