#pragma once



enum MusicType : int {
	mtExplore = 0,
	mtPublic,
	mtDungeon,
	mtCustom,					//Set when playing music desired by a script.
	mtBattle,
	mtUndefined,				//Unknown use.
	mtSpecial = 8,
	mtNotKnown = 0xFF		//Starts initialized to this.
};

enum SpecialMusicType : int {
	spDeath,
	spSuccess,
	spTitle,
	spNotKnown = 0xFF
};

//Checks whatever the given value is between 0 (Explore) and 4 (Battle)
bool isMusicTypeValid (int musicType);

bool isBattleType (MusicType musicType);

bool isBattleType (int musicType);