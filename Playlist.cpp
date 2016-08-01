#include "Playlist.h"

#include <time.h>
#include <algorithm>
#include "GameAPI.h"
#include "FilePath.h"


using namespace std;







Playlist::Playlist (const Playlist* copyFrom) : vanillaPlaylist (false) {
	copyFrom->copyTo (this);
}



//Initializes a new Playlist from a path (which can have a search pattern)
//and gives it a name.  The name will be used later on for custom playlists,
//but I won't impliment that at this time.
Playlist::Playlist (const string& name, const string& paths, bool randomOrder, bool vanillaPlaylist) : name (name), randomOrder(randomOrder), vanillaPlaylist (vanillaPlaylist) {
	if (setPaths (paths, randomOrder)) {
		_MESSAGE ("Playlist: \"%s\" > Created succesfully", name.c_str ());
	} else if (vanillaPlaylist) {
		_MESSAGE ("Playlist: \"%s\" > Vanilla playlist without tracks", name.c_str ());
	} else {
		_WARNING ("Playlist: \"%s\" > Not succesfully created", name.c_str ());
		throw exception ();
	}
}



bool Playlist::setPaths (const string& paths, bool randomOrder) {
	if (initialized = buildPaths (paths)) {
		Playlist::randomOrder = randomOrder;
		sortTracks (true);
		curIndex = -1;
	}
	return initialized;
}



bool Playlist::addPath (const string& path) {
	_MESSAGE ("Playlist: \"%s\" > Add path: \"%s\"", name.c_str (), path.c_str ());
	if (buildPath (path)) {
		_MESSAGE ("Playlist: \"%s\" > Added path: \"%s\"", name.c_str (), path.c_str ());
		initialized = true;
		sortTracks (true);
		curIndex = -1;
		return true;
	} else {
		_MESSAGE ("Playlist: \"%s\" > Path is not valid or empty: \"%s\"", name.c_str (), path.c_str ());
		return false;
	}
}



const string& Playlist::getName () const {
	return name;
}



const vector<string>& Playlist::getTracks () const {
	return tracks;
}



int Playlist::size () const {
	return initialized ? tracks.size () : 0;
}



const string Playlist::next () {
	if (initialized) {
		curIndex++;
		if (curIndex >= tracks.size ()) {
			sortTracks (false);	//Reorder the list.
			curIndex = -1;
		}
		_MESSAGE ("Track n° %d/%d", curIndex, tracks.size ());
		return tracks.at (curIndex);
	}
	return "";
}



const string Playlist::current () {
	if (initialized) {
		if (curIndex == -1) {
			curIndex = 0;
			return tracks.at (0);
		} else {
			return tracks.at (curIndex);
		}
	} else {
		return "";
	}
}



bool Playlist::isInitialized () const {
	return initialized;
}



bool Playlist::isVanilla () const {
	return vanillaPlaylist;
}



void Playlist::copyTo (Playlist* copyTo) const {
	copyTo->name = name;
	copyTo->paths = paths;
	copyTo->randomOrder = randomOrder;
	copyTo->tracks = tracks;
	copyTo->curIndex = curIndex;
	copyTo->initialized = initialized;
}



void Playlist::printTracks () const {
	if (initialized) {
		//s1 = Convert::ToString(name.GetBuffer()) + Convert::ToString("->Path  ") + Convert::ToString(TargetPath.GetStr().GetBuffer());
		//Console_Print(s1.c_str());
		for (const string& path : paths) {
			Console_Print ("Path: %s", path.c_str());
		}
		Console_Print ("Number of tracks: %d", (int)tracks.size ());
		string t;
		for (const string& track : tracks) {
			t = getFileName (track);
			Console_Print ("Track: %s", t.c_str());
		}
	} else {
		Console_Print ("%s has no track", name.c_str ());
	}
}







vector<string> Playlist::tokenizePaths (const string& paths) const {
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
		if (tracks.size () > 2) {
			random_shuffle (tracks.begin (), tracks.end ());
		}
	} else if (updated) {
		sort (tracks.begin (), tracks.end ());
	}
}



bool Playlist::buildPaths (const string& pathList) {
	bool once = false;
	tracks.clear ();
	paths.clear ();
	//The string may contains more than one path, separated by '>' or '|'.
	vector<string> pathVec = tokenizePaths (pathList);
	for (string path : pathVec) {
		if (buildPath (path)) {
			once = true;
		}
	}
	//Now, overwrite the values in tracks.
	curIndex = -1;
	return once && !tracks.empty ();
}



bool Playlist::buildPath (const string& path) {

	//This will make the path absolute in relation to Oblivion's main directory only if the path is currently relative, else it does nothing to the path.
	string filePathC = cleanPath (path, true);
	const char *charPath = filePathC.c_str ();
	
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
			} else if (!(endsWith (fileName, ".mp3") || endsWith (fileName, ".wav") || endsWith (fileName, ".wma"))) {
				continue;
			}
			
			once = true;
			fileName = cleanPath (fileName, false);
			tracks.push_back (folderPathC + fileName);
		} while (FindNextFileA (hFind, &FindFileData) != 0);
		FindClose (hFind);

		if (once) {
			paths.push_back (filePathC);
			return true;
		}
	} else if (exists (charPath) && isDirectory(charPath)) {
		paths.push_back (filePathC);
		tracks.push_back (filePathC);
		return true;
	}
	return false;
}