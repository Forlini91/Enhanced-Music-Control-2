#include "PlayMusicFile.h"

#include <deque>
#include <time.h>
#include "GameAPI.h"
#include "XShellPath\Path.h"
#include "ThreadRequest.h"


extern HANDLE hSentryRequestMutex;
extern ThreadRequest threadRequest;


bool playMusicFile (char* path) {
	 nsPath::CPath curTargetPath = nsPath::CPath (path);
	 curTargetPath.MakeAbsolute (nsPath::CPath (::GetOblivionDirectory ().c_str ()));
	 char *pathBuffer = curTargetPath.GetStr ().GetBuffer ();
	 char fullPath[MAX_PATH];

	 if (strchr (curTargetPath, '*') || strchr (pathBuffer, '?')) {
		 HANDLE hFind = INVALID_HANDLE_VALUE;
		 WIN32_FIND_DATAA FindFileData;
		 hFind = FindFirstFileA (pathBuffer, &FindFileData);
		 if (hFind == INVALID_HANDLE_VALUE) {
			 if (IsConsoleOpen () && IsConsoleMode ()) {
				 Console_Print ("Play track >> No file match the given pattern: %s", path);
			 }
			 _MESSAGE ("Command >> emcPlayTrack >> No file match the given pattern: %s", path);
			 return false;
		 }

		 std::deque <MusicFile> list;
		 do {
			 if (strcmp (FindFileData.cFileName, "..") == 0 || (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
				 continue;
			 }
			 MusicFile track;
			 strcpy_s (track.MusicPath, MAX_PATH, curTargetPath.GetPath () + nsPath::CPath (FindFileData.cFileName).GetStr ().GetString ());
			 list.push_back (track);
		 } while (FindNextFileA (hFind, &FindFileData) != 0);
		 FindClose (hFind);

		 int queueSize = list.size ();
		 if (queueSize <= 0) {		// "<=" just for sport!
			 if (IsConsoleOpen () && IsConsoleMode ()) {
				 Console_Print ("Play track >> No file match the given pattern: %s", path);
			 }
			 _MESSAGE ("Command >> emcPlayTrack >> No file match the given pattern: %s", path);
			 return false;
		 }
		 int elem;
		 if (queueSize > 1) {
			 srand ((unsigned)time (NULL));
			 elem = rand () % queueSize;
		 } else {
			 elem = 0;
		 }
		 strcpy_s (fullPath, MAX_PATH, list.at (elem).MusicPath);
		 list.clear ();
	 } else {
		 if (!curTargetPath.Exists () || curTargetPath.IsDirectory ()) {
			 if (IsConsoleOpen () && IsConsoleMode ()) {
				 Console_Print ("Play track >> No such file exists: %s", path);
			 }
			 _MESSAGE ("Command >> emcPlayTrack >> No such file exists: %s", path);
			 return false;
		 }
		 strcpy_s (fullPath, MAX_PATH, curTargetPath.GetStr ().GetString ());
	 }

	 if (IsConsoleOpen () && IsConsoleMode ()) {
		 Console_Print ("Play track >> %s", fullPath);
	 }
	 _MESSAGE ("Command >> emcPlayTrack >> %s", fullPath);
	 WaitForSingleObject (hSentryRequestMutex, INFINITE);
	 threadRequest.Request_PlayCustom = true;
	 strcpy_s (threadRequest.Request_Track_Name, MAX_PATH, fullPath);
	 ReleaseMutex (hSentryRequestMutex);
	 return true;
}