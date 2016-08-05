#include "ActivePlaylist.h"



ActivePlaylist::ActivePlaylist (const char* name, MusicType musicType, SpecialMusicType specialMusicType) : name (name), musicType (musicType), specialMusicType (specialMusicType) {}



void ActivePlaylist::initialize (int i, Playlist* vanillaPlaylist) {
	vanillaPlaylists[i] = defaultPlaylist = playlist = vanillaPlaylist;
}



bool ActivePlaylist::restorePlaylist () {
	if (playlist != defaultPlaylist) {
		playlist = defaultPlaylist;
		return true;
	}
	return false;
}



ActivePlaylist apl_Explore ("Explore", MusicType::Explore, SpecialMusicType::Sp_NotKnown);
ActivePlaylist apl_Public ("Public", MusicType::Public, SpecialMusicType::Sp_NotKnown);
ActivePlaylist apl_Dungeon ("Dungeon", MusicType::Dungeon, SpecialMusicType::Sp_NotKnown);
ActivePlaylist apl_Custom ("Custom", MusicType::Custom, SpecialMusicType::Sp_NotKnown);
ActivePlaylist apl_Battle ("Battle", MusicType::Battle, SpecialMusicType::Sp_NotKnown);
ActivePlaylist apl_Death ("Death", MusicType::Special, SpecialMusicType::Death);
ActivePlaylist apl_Success ("Success", MusicType::Special, SpecialMusicType::Success);
ActivePlaylist apl_Title ("Title", MusicType::Special, SpecialMusicType::Title);

ActivePlaylist* activePlaylists[8] = {
	&apl_Explore,
	&apl_Public,
	&apl_Dungeon,
	&apl_Custom,
	&apl_Battle,
	&apl_Death,
	&apl_Success,
	&apl_Title
};



void ActivePlaylist::operator= (Playlist* playlist) {
	ActivePlaylist::playlist = playlist;
}

void ActivePlaylist::operator= (const ActivePlaylist& apl) {
	ActivePlaylist::playlist = apl.playlist;
}

void ActivePlaylist::operator= (const ActivePlaylist* apl) {
	ActivePlaylist::playlist = apl->playlist;
}

void ActivePlaylist::operator+= (const string& path) {
	playlist->addPath (path);
}



bool operator== (const ActivePlaylist& apl1, const ActivePlaylist& apl2) {
	return apl1.playlist == apl2.playlist;
}

bool operator== (const ActivePlaylist& apl, const Playlist* playlist) {
	return apl.playlist == playlist;
}

bool operator== (const Playlist* playlist, const ActivePlaylist& apl) {
	return apl.playlist == playlist;
}

bool operator== (const ActivePlaylist& apl, const Playlist& playlist) {
	return *apl.playlist == playlist;
}

bool operator== (const Playlist& playlist, const ActivePlaylist& apl) {
	return *apl.playlist == playlist;
}






bool isPlaylistActive (Playlist* playlist) {
	for (ActivePlaylist* apl : activePlaylists) {
		if (playlist == apl->playlist) {
			return true;
		}
	}
	return false;
}



ActivePlaylist* getActivePlaylist (Playlist* playlist) {
	for (ActivePlaylist* apl : activePlaylists) {
		if (playlist == apl->playlist) {
			return apl;
		}
	}
	return nullptr;
}



ActivePlaylist* getActivePlaylist (MusicType musicType) {
	switch (musicType) {
		case MusicType::Explore: return &apl_Explore;
		case MusicType::Public: return &apl_Public;
		case MusicType::Dungeon: return &apl_Dungeon;
		case MusicType::Custom: return &apl_Custom;
		case MusicType::Battle: return &apl_Battle;
		default: return nullptr;
	}
}



ActivePlaylist* getActivePlaylist (SpecialMusicType specialMusicType) {
	switch (specialMusicType) {
		case SpecialMusicType::Death: return &apl_Death;
		case SpecialMusicType::Success: return &apl_Success;
		case SpecialMusicType::Title: return &apl_Title;
		default: return nullptr;
	}
}



bool samePlaylist (MusicType musicType1, MusicType musicType2) {
	return isMusicTypeValid (musicType1) && isMusicTypeValid (musicType2) &&
		getActivePlaylist (musicType1)->playlist == getActivePlaylist (musicType2)->playlist;
}