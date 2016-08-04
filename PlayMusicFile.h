#pragma once

#include <string>



using namespace std;



extern bool playMusicFile (const char* path);

extern bool playMusicFile (const string& path);

extern void parsePlayTrackCommand (const char* path);

extern void parsePlayTrackCommand (int playlistCode);