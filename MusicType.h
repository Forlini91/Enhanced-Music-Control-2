#pragma once



enum MusicType : int {
	Explore = 0,
	Public,
	Dungeon,
	Custom,					//Set when playing music desired by a script.
	Battle,
	Undefined,				//Unknown use.
	Special = 8,
	Mt_NotKnown = 0xFF		//Starts initialized to this.
};

enum SpecialMusicType : int {
	Death,
	Success,
	Title,
	Sp_NotKnown = 0xFF
};

//Checks whatever the given value is between 0 (Explore) and 4 (Battle)
extern bool isMusicTypeValid (int musicType);