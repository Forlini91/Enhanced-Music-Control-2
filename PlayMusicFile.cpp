#include "PlayMusicFile.h"

#include <string>
#include <vector>
#include <time.h>
#include "GameAPI.h"
#include "ThreadRequest.h"
#include "FilePath.h"
#include "VanillaPlaylistData.h"



using namespace std;



bool playMusicFile (const char* path) {
	return playMusicFile (string (path));
}



bool playMusicFile (const string& path) {
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
		HANDLE hFind = INVALID_HANDLE_VALUE;
		WIN32_FIND_DATAA FindFileData;
		hFind = FindFirstFileA (charPath, &FindFileData);
		if (hFind == INVALID_HANDLE_VALUE) {
			if (IsConsoleOpen () && IsConsoleMode ()) {
				Console_Print ("Play track >> No file match the given pattern: %s", path.c_str ());
			}
			_MESSAGE ("Command >> emcPlayTrack >> No file match the given pattern: %s", path.c_str ());
			return false;
		}

		string folderPathC = getFolderPath (filePathC);
		vector <string> list;
		do {
			string fileName = FindFileData.cFileName;
			if (fileName == ".." || (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
				continue;
			} else if (endsNotWithAll (fileName, supportedExtensions)) {
				continue;
			}

			fileName = cleanPath (fileName, false);
			list.push_back (folderPathC + fileName);
		} while (FindNextFileA (hFind, &FindFileData) != 0);
		FindClose (hFind);

		if (list.empty()) {		// "<=" just for sport!
			if (IsConsoleOpen () && IsConsoleMode ()) {
				Console_Print ("Play track >> No file match the given pattern: %s", path.c_str ());
			}
			_MESSAGE ("Command >> emcPlayTrack >> No file match the given pattern: %s", path.c_str ());
			return false;
		}

		int queueSize = list.size ();
		if (queueSize > 1) {
			srand ((unsigned)time (NULL));
			filePathC = list.at (rand () % queueSize);
		} else {
			filePathC = list.at (0);
		}
	} else if (exists (filePathC)) {
		filePathC = filePathC;
	} else {
		if (IsConsoleOpen () && IsConsoleMode ()) {
			Console_Print ("Play track >> No such file exists: %s", path.c_str ());
		}
		_MESSAGE ("Command >> emcPlayTrack >> No such file exists: %s", path.c_str ());
		return false;
	}

	if (IsConsoleOpen () && IsConsoleMode ()) {
		Console_Print ("Play track >> %s", filePathC.c_str ());
	}
	_MESSAGE ("Command >> emcPlayTrack >> %s", filePathC.c_str ());
	threadRequest.requestPlayCustomTrack (filePathC);
	return true;
}



void parsePlayTrackCommand (const char* path) {
	if (_stricmp (path, "explore") == 0) {
		playMusicFile (obExplorePath);
	} else if (_stricmp (path, "public") == 0) {
		playMusicFile (obPublicPath);
	} else if (_stricmp (path, "dungeon") == 0) {
		playMusicFile (obDungeonPath);
	} else if (_stricmp (path, "battle") == 0) {
		playMusicFile (obBattlePath);
	} else if (_stricmp (path, "random") == 0) {
		srand ((unsigned)time (NULL));
		switch (rand () % 4) {
			case 0: playMusicFile (obExplorePath); break;
			case 1: playMusicFile (obPublicPath); break;
			case 2: playMusicFile (obDungeonPath); break;
			case 3: playMusicFile (obBattlePath); break;
		}
	} else {
		playMusicFile (path);
	}
}



void parsePlayTrackCommand (int playlistCode) {
	switch (playlistCode) {
		case 0: playMusicFile (obExplorePath); break;
		case 1: playMusicFile (obPublicPath); break;
		case 2: playMusicFile (obDungeonPath); break;
		case 4: playMusicFile (obBattlePath); break;
	}
}