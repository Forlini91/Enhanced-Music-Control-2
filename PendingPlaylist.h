#pragma once

#include "Playlist.h"
#include "ActivePlaylist.h"
#include "TimeManagement.h"

using namespace std;

struct PendingPlaylist {
	ActivePlaylist * const aplToSwap;
	Playlist * const playlist;
	const bool afterThisTrack;
	const int fadeOutTime;
	const int fadeInTime;
	const milliseconds targetTime;

	PendingPlaylist (ActivePlaylist *aplToSwap, Playlist *playlist, bool afterThisTrack, int delay, int fadeOut, int fadeIn);

	bool operator<(const PendingPlaylist& other) const;
	bool operator<=(const PendingPlaylist& other) const;
	bool operator>(const PendingPlaylist& other) const;
	bool operator>=(const PendingPlaylist& other) const;
	bool operator==(const PendingPlaylist& other) const;
	bool operator!=(const PendingPlaylist& other) const;
};