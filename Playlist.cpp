#include "Playlist.h"

#include <time.h>
#include <algorithm>
#include "GameAPI.h"
#include "FilePath.h"


using namespace std;





Playlist *vanillaPlaylists[8];		//The 8 vanilla playlists: {Explore,Public,Dungeon,Custom,Battle,Death,Success,Title}
PlaylistsMap playlists;				//All vanilla and custom playlists
HANDLE hPlaylistMutex = CreateMutex (nullptr, FALSE, nullptr);		//Lock when using the object "playlists".
const string emptyStr;



Playlist::Playlist (const Playlist *copyFrom) : name(copyFrom->name), vanillaPlaylist (false) {
	copyFrom->copyTo (this);
}



Playlist::Playlist (const char *name, bool vanillaPlaylist) : name (name), vanillaPlaylist (vanillaPlaylist) {}



Playlist::Playlist (const char *name, Paths paths, bool randomOrder, bool vanillaPlaylist) : name (name), randomOrder (randomOrder), vanillaPlaylist (vanillaPlaylist) {
	if (setPaths (paths, randomOrder)) {
		_MESSAGE ("Playlist: \"%s\" > Created succesfully", name);
	} else if (vanillaPlaylist) {
		_MESSAGE ("Playlist: \"%s\" > Vanilla playlist without tracks", name);
	} else {
		_WARNING ("Playlist: \"%s\" > Not succesfully created", name);
		throw exception ();
	}
}



bool Playlist::setPaths (Paths paths, bool randomOrder) {
	Playlist backup = Playlist (this);
	if (buildPaths (paths)) {
		_MESSAGE ("Playlist: \"%s\" > New paths: \"%s\"", name, paths.c_str ());
		initialized = true;
		Playlist::randomOrder = randomOrder && tracks.size () >= 2;
		sortTracks (true);
		curIndex = -1;
		return true;
	} else {
		_MESSAGE ("Playlist: \"%s\" > Path is not valid or empty: \"%s\"", name, paths.c_str ());
		backup.moveTo (this);
		initialized = false;
		return false;
	}
}



bool Playlist::addPath (Path path) {
	Playlist backup = Playlist(this);
	_MESSAGE ("Playlist: \"%s\" > Add path: \"%s\"", name, path.c_str ());
	if (buildPath (path, true)) {
		_MESSAGE ("Playlist: \"%s\" > Added path: \"%s\"", name, path.c_str ());
		initialized = true;
		sortTracks (true);
		curIndex = -1;
		return true;
	} else {
		_MESSAGE ("Playlist: \"%s\" > Path is not valid or empty: \"%s\"", name, path.c_str ());
		backup.moveTo (this);
		return false;
	}
}



const TracksList& Playlist::getTracks () const {
	return tracks;
}



int Playlist::size () const {
	return initialized ? tracks.size () : 0;
}



Track Playlist::getCurrentTrack () const {
	if (initialized && curIndex >= 0) {
		return tracks.at (curIndex);
	} else {
		return emptyStr;
	}
}



Track Playlist::getNextTrack () {
	if (initialized) {
		curIndex++;
		if (curIndex >= tracks.size ()) {
			sortTracks (false);	//Reorder the list.
			curIndex = -1;
		}
		_MESSAGE ("Track n° %d/%d", curIndex, tracks.size ());
		return tracks.at (curIndex);
	}
	return emptyStr;
}



bool Playlist::restoreTrackPosition (Track track) {
	TracksList::iterator pos = find (tracks.begin (), tracks.end (), track);
	if (pos != tracks.end ()) {
		curIndex = distance (tracks.begin (), pos);
		return true;
	} else {
		return false;
	}
}



bool Playlist::isInitialized () const {
	return initialized;
}



bool Playlist::isVanilla () const {
	return vanillaPlaylist;
}



void Playlist::copyTo (Playlist *copyTo) const {
	copyTo->paths = paths;
	copyTo->randomOrder = randomOrder;
	copyTo->tracks = tracks;
	copyTo->curIndex = curIndex;
	copyTo->initialized = initialized;
}



void Playlist::moveTo (Playlist *moveTo) {
	moveTo->paths = move (paths);
	moveTo->randomOrder = randomOrder;
	moveTo->tracks = move (tracks);
	moveTo->curIndex = curIndex;
	moveTo->initialized = initialized;
}



void Playlist::printTracks () const {
	if (initialized) {
		//s1 = Convert::ToString(name.GetBuffer()) + Convert::ToString("->Path  ") + Convert::ToString(TargetPath.GetStr().GetBuffer());
		//Console_Print(s1.c_str());
		for (Path path : paths) {
			Console_Print ("Path: %s", path.c_str());
		}
		Console_Print ("Number of tracks: %d", (int)tracks.size ());
		for (Track track : tracks) {
			Console_Print ("Track: %s", getFileName (track).c_str ());
		}
	} else {
		Console_Print ("%s has no track", name);
	}
}







PathsList Playlist::tokenizePaths (Paths paths) const {
	PathsList pathVec;
	int i = 0;
	int j = 0;
	int n = paths.size ();

	while (i < n) {
		while (j < n && paths.at (j) != '>' && paths.at (j) != '|') {
			j++;
		}
		pathVec.emplace_back (paths, i, j-i);
		i = ++j;
	}
	return pathVec;
}



void Playlist::sortTracks (bool updated) {
	if (randomOrder){
		if (tracks.size () > 2) {
			random_shuffle (tracks.begin (), tracks.end ());
		}
	} else if (updated) {
		sort (tracks.begin (), tracks.end ());
	}
}



bool Playlist::buildPaths (Paths pathList) {
	bool once = false;
	tracks.clear ();
	paths.clear ();
	//The string may contains more than one path, separated by '>' or '|'.
	PathsList pathsList = tokenizePaths (pathList);
	for (Paths path : pathsList) {
		if (buildPath (path, false)) {
			once = true;
		}
	}
	//Now, overwrite the values in tracks.
	curIndex = -1;
	return once && !tracks.empty ();
}



bool Playlist::buildPath (Path path, bool add) {

	//This will make the path absolute in relation to Oblivion's main directory only if the path is currently relative, else it does nothing to the path.
	string filePathC = cleanPath (path, true);
	const char *charPath = filePathC.c_str ();

	if (isDirectory (filePathC)) {
		if (filePathC.back () != '\\') {
			filePathC += "\\*";
			charPath = filePathC.c_str ();
		} else {
			filePathC.push_back ('*');
			charPath = filePathC.c_str ();
		}
	}
	
	if (strchr (charPath, '*') || strchr (charPath, '?')) {
		WIN32_FIND_DATAA FindFileData;
		HANDLE hFind = FindFirstFileA (charPath, &FindFileData);
		if (hFind == INVALID_HANDLE_VALUE) {
			return false;
		}

		string folderPathC = getFolderPath (filePathC);
		bool once = false;
		do {
			string fileName = FindFileData.cFileName;
			//This will prevent ".." from being seen as a file.
			if (fileName == ".." || (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
				continue;
			} else if (endsNotWithAll (fileName, supportedExtensions)) {
				continue;
			}
			
			once = true;
			fileName = folderPathC + cleanPath (fileName, false);
			if (!add || find (tracks.begin (), tracks.end (), fileName) == tracks.end ()) {
				tracks.push_back (fileName);
			}
		} while (FindNextFileA (hFind, &FindFileData) != 0);
		FindClose (hFind);

		if (once) {
			paths.push_back (filePathC);
			return true;
		}
	} else if (exists (charPath)) {
		paths.push_back (filePathC);
		tracks.push_back (filePathC);
		return true;
	}
	return false;
}


bool Playlist::operator== (const Playlist &playlist) const {
	return name == playlist.name;
}

bool Playlist::operator== (const Playlist *playlist) const {
	return name == playlist->name;
}