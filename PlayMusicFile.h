#pragma once

struct MusicFile {
	char MusicPath[MAX_PATH];
};

bool playMusicFile (char* path);