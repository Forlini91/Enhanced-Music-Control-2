#pragma once

#include "atlstr.h"
#include <string>
#include <deque>
#include <vector>
#include <unordered_map>

#include "MusicType.h"



#define emplacePlaylist(name,vanillaPlaylist) playlists.emplace (constructInPlace(name, name, vanillaPlaylist))
#define emplacePlaylistPaths(name,paths,randomOrder,vanillaPlaylist) playlists.emplace (constructInPlace(name, name, paths, randomOrder, vanillaPlaylist))



using namespace std;



class Playlist {

private:

	//The array containing the paths of the tracks in the list.
	//Everytime curIndex cycles through the list, it will be reordered,
	vector<string> tracks;

	//The directory containing the music, if we ever need it.
	//nsPath::CPath TargetPath;
	vector<string> paths;

	//If true, tracks are shuffled on creation/recreation/repeat, else they always maintain the starting order
	bool randomOrder;

	//The index the playlist is currently at.  If no song has been played in
	//this list, the value will be -1.
	int curIndex = -1;

	//Is playlist initialized?
	bool initialized = false;

	//Is playlist destroyed?
	bool destroyed = false;

	//If true, this is a vanilla playlist
	const bool vanillaPlaylist;



	//Tokenizes the path.
	vector<string> tokenizePaths (const string &paths) const;

	//Randomly reorders the playlist, guaranteeing that the last played song
	//will not come first in the playlist.
	void sortTracks (bool updated);

	//This function builds the playlist from the value held in TargetPath.
	bool buildPaths (const string &paths);

	bool buildPath (const string &path, bool add);



public:

	HANDLE hMutex = CreateMutex (nullptr, FALSE, nullptr);

	//The name of this playlist.  This value may be used later to increase the capabilities
	//of this music system.  It can be accessed by friendly classes to help manage a collection.
	const char *name;


	Playlist (const Playlist *copyFrom);

	Playlist (const char *name, bool vanillaPlaylist);

	Playlist (const char *name, const string &paths, bool randomOrder, bool vanillaPlaylist);

	~Playlist ();

	void destroy ();



	//Set the given paths and build the tracks/paths vectors.
	bool setPaths (const string &paths, bool randomOrder);

	//Adds a SINGLE path to the playlist, and reinitializes it.
	bool addPath (const string &path);

	//Gets all tracks in the playlist
	const vector<string>& getTracks () const;

	//Check whatever the given track is present in the playlist
	bool hasTrack (const string &track) const;

	//Number of tracks in the playlist
	int size () const;

	//Should be checked before use to ensure you're not trying to handle a playlist which failed initialization.
	bool isInitialized () const;

	//Should be checked before use to ensure you're not trying to handle a destroyed playlist.
	bool isDestroyed () const;

	//Return true if this is a vanilla playlist (and so, it can't be altered).
	bool isVanilla () const;

	//Returns the current track. Return the first track if there's still no current track.
	const string& getCurrentTrack () const;

	//Advances the playlist by one, and returns the next track
	const string& getNextTrack ();

	//Search the given track in the playlist and restore the playlist indexes.
	bool restoreTrackIndex (const string &track);

	//Copy this playlist data to the target playlist.
	void copyTo (Playlist *copyTo) const;

	//Move this playlist data to the target playlist
	void moveTo (Playlist *copyTo);

	void printTracks () const;

	

	bool operator== (const Playlist &playlist) const;

	bool operator== (const Playlist *playlist) const;

	bool operator!= (const Playlist &playlist) const;

	bool operator!= (const Playlist *playlist) const;

};


typedef unordered_map<string, Playlist> PlaylistsMap;
typedef pair<PlaylistsMap::iterator, bool> PlaylistEmplaceResult;


extern Playlist *vanillaPlaylists[8];
extern PlaylistsMap playlists;