#pragma once

#include <string>



using namespace std;



bool playMusicFile (const char *path);

bool playMusicFile (const string &path);

void parsePlayTrackCommand (const char *path);

void parsePlayTrackCommand (int playlistCode);