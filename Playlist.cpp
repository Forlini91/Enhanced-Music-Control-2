#include "Playlist.h"

#include <time.h>
#include <algorithm>
#include "GameAPI.h"
#include "FilePath.h"
#include "TimeManagement.h"
#include "DebugMode.h"

#define notDirectory(x) ((x.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)



using namespace std;





Playlist *vanillaPlaylists[8];		//The 8 vanilla playlists: {Explore,Public,Dungeon,Custom,Battle,Death,Success,Title}
PlaylistsMap playlists;				//All vanilla and custom playlists
const string emptyStr;



Playlist::Playlist (const Playlist *copyFrom) : name(copyFrom->name), vanillaPlaylist (false) {
	copyFrom->copyTo (this);
}



Playlist::Playlist (const char *name, bool vanillaPlaylist) : name (name), vanillaPlaylist (vanillaPlaylist) {}



Playlist::Playlist (const char *name, const string &paths, bool randomOrder, bool vanillaPlaylist) : name (name), randomOrder (randomOrder), vanillaPlaylist (vanillaPlaylist) {
	if (setPaths (paths, randomOrder)) {
		_MESSAGE ("%lld | Playlist: \"%s\" > Created succesfully", timeStamp, name);
	} else if (vanillaPlaylist) {
		_MESSAGE ("%lld | Playlist: \"%s\" > Vanilla playlist without tracks", timeStamp, name);
	} else {
		_WARNING ("%lld | Playlist: \"%s\" > Not succesfully created", timeStamp, name);
		throw exception ();
	}
}



Playlist::~Playlist () {
	destroy ();
	ReleaseMutex (hMutex);
	CloseHandle (hMutex);
}



void Playlist::destroy () {
	initialized = false;
	destroyed = true;
	tracks.clear ();
	paths.clear ();
}



bool Playlist::setPaths (const string &paths, bool randomOrder) {
	Playlist backup = Playlist (this);
	if (buildPaths (paths)) {
		_MESSAGE ("%lld | Playlist: \"%s\" > New paths: \"%s\"", timeStamp, name, paths.c_str ());
		initialized = true;
		destroyed = false;
		Playlist::randomOrder = randomOrder;
		sortTracks (true);
		curIndex = -1;
		return true;
	} else {
		_MESSAGE ("%lld | Playlist: \"%s\" > Path is not valid or empty: \"%s\"", timeStamp, name, paths.c_str ());
		backup.moveTo (this);
		initialized = false;
		return false;
	}
}



bool Playlist::addPath (const string &path) {
	Playlist backup = Playlist(this);
	if (buildPath (path, true)) {
		_MESSAGE ("%lld | Playlist: \"%s\" > Added path: \"%s\"", timeStamp, name, path.c_str ());
		initialized = true;
		destroyed = false;
		sortTracks (true);
		curIndex = -1;
		return true;
	} else {
		_MESSAGE ("%lld | Playlist: \"%s\" > const string &is not valid or empty: \"%s\"", timeStamp, name, path.c_str ());
		backup.moveTo (this);
		return false;
	}
}



const vector<string>& Playlist::getTracks () const {
	return tracks;
}



bool Playlist::hasTrack (const string &track) const {
	return find (tracks.cbegin (), tracks.cend (), track) != tracks.cend ();
}



int Playlist::size () const {
	return initialized ? tracks.size () : 0;
}



const string& Playlist::getCurrentTrack () const {
	if (initialized && curIndex >= 0) {
		return tracks.at (curIndex);
	} else {
		return emptyStr;
	}
}



const string& Playlist::getNextTrack () {
	if (initialized) {
		if (++curIndex >= tracks.size ()) {
			sortTracks (false);	//Reorder the list.
			curIndex = 0;
		}
		_MESSAGE ("%lld | Playlist: \"%s\" > Track n° %d/%d > %s", timeStamp, name, curIndex, tracks.size (), tracks.at (curIndex));
		return tracks.at (curIndex);
	}
	return emptyStr;
}



bool Playlist::restoreTrackIndex (const string &track) {
	vector<string>::iterator pos = find (tracks.begin (), tracks.end (), track);
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



bool Playlist::isDestroyed () const {
	return destroyed;
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
		//s1 = Convert::ToString(name.GetBuffer()) + Convert::ToString("->const string & ") + Convert::ToString(TargetPath.GetStr().GetBuffer());
		//Console_Print(s1.c_str());
		for (const string &path : paths) {
			Console_Print ("Path: %s", path.c_str());
		}
		Console_Print ("Number of tracks: %d", (int)tracks.size ());
		for (const string &track : tracks) {
			Console_Print ("Track: %s", getFileName (track).c_str ());
		}
	} else {
		Console_Print ("%s has no track", name);
	}
}







vector<string> Playlist::tokenizePaths (const string &paths) const {
	vector<string> pathVec;
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
		if (tracks.size () > 1) {
			_EMCDEBUG2 ("%lld | Playlist: \"%s\" > Shuffle", timeStamp, name);
			random_shuffle (tracks.begin (), tracks.end ());
		}
	} else if (updated) {
		_EMCDEBUG2 ("%lld | Playlist: \"%s\" > Sort", timeStamp, name);
		sort (tracks.begin (), tracks.end ());
	}
}



bool Playlist::buildPaths (const string &pathList) {
	bool once = false;
	tracks.clear ();
	paths.clear ();
	//The string may contains more than one path, separated by '>' or '|'.
	vector<string> pathsList = tokenizePaths (pathList);
	for (const string &path : pathsList) {
		if (buildPath (path, false)) {
			once = true;
		}
	}
	//Now, overwrite the values in tracks.
	curIndex = -1;
	return once && !tracks.empty ();
}



bool Playlist::buildPath (const string &path, bool add) {

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
			if (fileName == ".." || !notDirectory (FindFileData)) {
				continue;
			} else if (!isExtensionSupported(fileName)) {
				_EMCDEBUG2 ("%lld | File %s has unsupported extension", timeStamp, fileName.c_str ());
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

bool Playlist::operator!= (const Playlist &playlist) const {
	return name != playlist.name;
}

bool Playlist::operator!= (const Playlist *playlist) const {
	return name != playlist->name;
}