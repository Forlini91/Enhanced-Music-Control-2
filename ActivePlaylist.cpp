#include "ActivePlaylist.h"



ActivePlaylist::ActivePlaylist (const char *name, MusicType musicType, SpecialMusicType specialMusicType) : name (name), musicType (musicType), specialMusicType (specialMusicType) {}



void ActivePlaylist::initialize (int i, Playlist *vanillaPlaylist) {
	vanillaPlaylists[i] = defaultPlaylist = playlist = vanillaPlaylist;
}



bool ActivePlaylist::restorePlaylist () {
	if (playlist != defaultPlaylist) {
		playlist = defaultPlaylist;
		return true;
	}
	return false;
}



ActivePlaylist apl_Explore ("Explore", MusicType::mtExplore, SpecialMusicType::spNotKnown);
ActivePlaylist apl_Public ("Public", MusicType::mtPublic, SpecialMusicType::spNotKnown);
ActivePlaylist apl_Dungeon ("Dungeon", MusicType::mtDungeon, SpecialMusicType::spNotKnown);
ActivePlaylist apl_Custom ("Custom", MusicType::mtCustom, SpecialMusicType::spNotKnown);
ActivePlaylist apl_Battle ("Battle", MusicType::mtBattle, SpecialMusicType::spNotKnown);
ActivePlaylist apl_Death ("Death", MusicType::mtSpecial, SpecialMusicType::spDeath);
ActivePlaylist apl_Success ("Success", MusicType::mtSpecial, SpecialMusicType::spSuccess);
ActivePlaylist apl_Title ("Title", MusicType::mtSpecial, SpecialMusicType::spTitle);
ActivePlaylist apl_NULL ("<Invalid>", MusicType::mtNotKnown, SpecialMusicType::spNotKnown);

ActivePlaylist *activePlaylists[8] = {
	&apl_Explore,
	&apl_Public,
	&apl_Dungeon,
	&apl_Custom,
	&apl_Battle,
	&apl_Death,
	&apl_Success,
	&apl_Title
};

ActivePlaylist* selectedActivePlaylist = &apl_NULL;



void ActivePlaylist::operator= (Playlist *playlist) {
	ActivePlaylist::playlist = playlist;
}

void ActivePlaylist::operator= (const ActivePlaylist &apl) {
	ActivePlaylist::playlist = apl.playlist;
}

void ActivePlaylist::operator= (const ActivePlaylist *apl) {
	ActivePlaylist::playlist = apl->playlist;
}

void ActivePlaylist::operator+= (const string &path) {
	playlist->addPath (path);
}



bool operator== (const ActivePlaylist &apl1, const ActivePlaylist &apl2) {
	return apl1.playlist == apl2.playlist;
}

bool operator== (const ActivePlaylist &apl, const Playlist *playlist) {
	return apl.playlist == playlist;
}

bool operator== (const Playlist *playlist, const ActivePlaylist &apl) {
	return apl.playlist == playlist;
}

bool operator== (const ActivePlaylist &apl, const Playlist &playlist) {
	return *apl.playlist == playlist;
}

bool operator== (const Playlist& playlist, const ActivePlaylist& apl) {
	return *apl.playlist == playlist;
}



bool operator!= (const ActivePlaylist &apl1, const ActivePlaylist &apl2) {
	return apl1.playlist != apl2.playlist;
}

bool operator!= (const ActivePlaylist &apl, const Playlist *playlist) {
	return apl.playlist != playlist;
}

bool operator!= (const Playlist *playlist, const ActivePlaylist &apl) {
	return apl.playlist != playlist;
}

bool operator!= (const ActivePlaylist &apl, const Playlist &playlist) {
	return *apl.playlist != playlist;
}

bool operator!= (const Playlist& playlist, const ActivePlaylist& apl) {
	return *apl.playlist != playlist;
}






bool isPlaylistActive (const char *playlistName) {
	if (playlistName[0] != '\0') {	//Not empty
		for (ActivePlaylist* apl : activePlaylists) {
			if (strcmp (playlistName, apl->playlist->name) != 0) {
				return true;
			}
		}
	}
	return false;
}



inline bool isPlaylistActive (string &playlistName) {
	return isPlaylistActive (playlistName.c_str ());
}



bool isPlaylistActive (Playlist *playlist) {
	for (ActivePlaylist* apl : activePlaylists) {
		if (playlist == apl->playlist) {
			return true;
		}
	}
	return false;
}



ActivePlaylist* getActivePlaylist (const char *playlistName) {
	if (playlistName[0] != '\0') {	//Not empty
		for (ActivePlaylist* apl : activePlaylists) {
			if (strcmp (playlistName, apl->playlist->name) != 0) {
				return apl;
			}
		}
	}
	return nullptr;
}



inline ActivePlaylist* getActivePlaylist (string& playlistName) {
	return getActivePlaylist(playlistName.c_str ());
}



ActivePlaylist* getActivePlaylist (Playlist *playlist) {
	for (ActivePlaylist* apl : activePlaylists) {
		if (playlist == apl->playlist) {
			return apl;
		}
	}
	return nullptr;
}



ActivePlaylist* getActivePlaylist (MusicType musicType) {
	switch (musicType) {
		case MusicType::mtExplore: return &apl_Explore;
		case MusicType::mtPublic: return &apl_Public;
		case MusicType::mtDungeon: return &apl_Dungeon;
		case MusicType::mtCustom: return &apl_Custom;
		case MusicType::mtBattle: return &apl_Battle;
		default: return &apl_NULL;
	}
}



ActivePlaylist* getActivePlaylist (SpecialMusicType specialMusicType) {
	switch (specialMusicType) {
		case SpecialMusicType::spDeath: return &apl_Death;
		case SpecialMusicType::spSuccess: return &apl_Success;
		case SpecialMusicType::spTitle: return &apl_Title;
		default: return &apl_NULL;
	}
}



bool samePlaylist (MusicType musicType1, MusicType musicType2) {
	ActivePlaylist* apl1 = getActivePlaylist (musicType1);
	ActivePlaylist* apl2 = getActivePlaylist (musicType2);
	return apl1 != &apl_NULL && apl2 != &apl_NULL && apl1->playlist == apl2->playlist;
}