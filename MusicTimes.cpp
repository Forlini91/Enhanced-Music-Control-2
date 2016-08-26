#include "MusicTimes.h"


#include "Globals.h"

MusicTimes musicTimes;
HANDLE hMusicTimesMutex;
MusicTimesGetter musicTimesGetter[] = {&MusicTimes::getMusicRememberTime, &MusicTimes::getBattleMusicRememberTime};



int MusicTimes::getMinPauseTime () const {
	return minPauseTime;
}



bool MusicTimes::setMinPauseTime (int length) {
	if (minPauseTime != length) {
		minPauseTime = length;
		return true;
	}
	return false;
}



int MusicTimes::getExtraPauseTime () const {
	return extraPauseTime;
}



bool MusicTimes::setExtraPauseTime (int length) {
	if (extraPauseTime != length) {
		extraPauseTime = length;
		return true;
	}
	return false;
}



int MusicTimes::getPauseTime () const {
	return pauseTime;
}



void MusicTimes::recalculatePauseTime () {
	pauseTime = minPauseTime + (extraPauseTime * rand () / RAND_MAX);
}



int MusicTimes::getMinStartBattleDelay () const {
	return minStartBattleDelay;
}



bool MusicTimes::setMinStartBattleDelay (int length) {
	if (minStartBattleDelay != length) {
		minStartBattleDelay = length;
		return true;
	}
	return false;
}



int MusicTimes::getExtraStartBattleDelay () const {
	return extraStartBattleDelay;
}



bool MusicTimes::setExtraStartBattleDelay (int length) {
	if (extraStartBattleDelay != length) {
		extraStartBattleDelay = length;
		return true;
	}
	return false;
}



int MusicTimes::getStartBattleDelay () const {
	return startBattleDelay;
}



void MusicTimes::recalculateStartBattleDelay () {
	startBattleDelay = minStartBattleDelay + (extraStartBattleDelay * rand () / RAND_MAX);
}



int MusicTimes::getMinStopBattleDelay () const {
	return minStopBattleDelay;
}



bool MusicTimes::setMinStopBattleDelay (int length) {
	if (minStopBattleDelay != length) {
		minStopBattleDelay = length;
		return true;
	}
	return false;
}



int MusicTimes::getExtraStopBattleDelay () const {
	return extraStopBattleDelay;
}



bool MusicTimes::setExtraStopBattleDelay (int length) {
	if (extraStopBattleDelay != length) {
		extraStopBattleDelay = length;
		return true;
	}
	return false;
}



int MusicTimes::getStopBattleDelay () const {
	return stopBattleDelay;
}



void MusicTimes::recalculateStopBattleDelay () {
	stopBattleDelay = minStopBattleDelay + (extraStopBattleDelay * rand () / RAND_MAX);
}



int MusicTimes::getMusicRememberTime () const {
	return musicRememberTime;
}



bool MusicTimes::setMusicRememberTime (int length) {
	if (musicRememberTime != length) {
		musicRememberTime = length;
		return true;
	}
	return false;
}



int MusicTimes::getBattleMusicRememberTime () const {
	return battleMusicRememberTime;
}



bool MusicTimes::setBattleMusicRememberTime (int length) {
	if (battleMusicRememberTime != length) {
		battleMusicRememberTime = length;
		return true;
	}
	return false;
}




int MusicTimes::getFadeIn () const {
	return fadeIn;
}



bool MusicTimes::setFadeIn (int time) {
	if (fadeIn != time) {
		fadeIn = time;
		return true;
	}
	return false;
}



int MusicTimes::getFadeOut () const {
	return fadeOut;
}



bool MusicTimes::setFadeOut (int time) {
	if (fadeOut != time) {
		fadeOut = time;
		return true;
	}
	return false;
}