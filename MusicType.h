#pragma once



enum MusicType : unsigned int {
	mtExplore = 0,
	mtPublic = 1,
	mtDungeon = 2,
	mtCustom = 3,			//Set when playing music desired by a script.
	mtBattle = 4,
	mtUndefined = 5,		//Unknown use.
	mtSpecial = 8,
	mtNotKnown = 0xFF,		//Starts initialized to this.
	mtNull = 65535			//Used when there's no music type (just after title or after level-up)
};

enum SpecialMusicType : unsigned int {
	spDeath = 0,
	spSuccess = 1,
	spTitle = 2,
	spNotKnown = 0xFF
};