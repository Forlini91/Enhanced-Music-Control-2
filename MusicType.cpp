#include "MusicType.h"



bool isMusicTypeValid (int musicType) {
	return musicType >= 0 && musicType <= 4;
}

bool isBattleType (MusicType musicType) {
	return musicType == MusicType::mtBattle;
}

bool isBattleType (int musicType) {
	return musicType == 4;
}