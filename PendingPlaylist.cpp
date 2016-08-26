#include "PendingPlaylist.h"


PendingPlaylist::PendingPlaylist (ActivePlaylist *aplToSwap, Playlist *playlist, bool afterThisTrack, int delay, int fadeOut, int fadeIn)
	: aplToSwap (aplToSwap), playlist (playlist), afterThisTrack (afterThisTrack), targetTime (now + milliseconds (delay)), fadeOutTime (fadeOutTime), fadeInTime (fadeInTime) {}


bool PendingPlaylist::operator<(const PendingPlaylist& other) const {
	return targetTime < other.targetTime;
}

bool PendingPlaylist::operator<=(const PendingPlaylist& other) const {
	return targetTime <= other.targetTime;
}

bool PendingPlaylist::operator>(const PendingPlaylist& other) const {
	return targetTime > other.targetTime;
}

bool PendingPlaylist::operator>=(const PendingPlaylist& other) const {
	return targetTime >= other.targetTime;
}

bool PendingPlaylist::operator==(const PendingPlaylist& other) const {
	return targetTime == other.targetTime;
}

bool PendingPlaylist::operator!=(const PendingPlaylist& other) const {
	return targetTime != other.targetTime;
}