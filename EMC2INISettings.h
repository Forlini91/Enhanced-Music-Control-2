#pragma once

#include "INIManager.h"
#include "MusicPlayer.h"



#define INI_PATH "Data\\OBSE\\Plugins\\EnhancedMusicControl 2.ini"		//Path of the EMC2 ini



using namespace SME::INI;


class EMC2INISettings : INIManager {

public:
	INISetting kINIMusicSpeed = INISetting (
		"MusicSpeed",
		"General",
		"What's the music speed? (1.0 is the normal speed, above 1 is faster, below 1 is slower)",
		1.0F);

	INISetting kINIFadeOut = INISetting (
		"FadeOut",
		"General",
		"Fade out time",
		1.0F);


	INISetting kINIFadeIn = INISetting (
		"FadeIn",
		"General",
		"Fade in time",
		1.0F);


	INISetting kINITrackPause = INISetting (
		"MinPause",
		"General",
		"Min duration of pause between tracks.",
		0.0F);


	INISetting kINITrackPauseExtra = INISetting (
		"ExtraPause",
		"General",
		"Extra random duration of pause between tracks. Result = MinPause + (Random value between 0 and ExtraPause)",
		0.0F);


	INISetting kINIBattleMusicStartDelay = INISetting (
		"MinBattleStart",
		"General",
		"Min delay before combat music start",
		1.0F);


	INISetting kINIBattleMusicStartDelayExtra = INISetting (
		"ExtraBattleStart",
		"General",
		"Extra delay before combat music start",
		0.0F);


	INISetting kINIBattleMusicEndDelay = INISetting (
		"MinBattleEnd",
		"General",
		"Min delay before combat music ends",
		1.0F);


	INISetting kINIBattleMusicEndDelayExtra = INISetting (
		"ExtraBattleEnd",
		"General",
		"Extra delay before combat music ends",
		0.0F);


	INISetting kINIPreviousTrackRemember = INISetting (
		"PreviousTrackRemember",
		"General",
		"How many seconds EMC will remember the previous track when changing track?",
		20.0F);


	INISetting kINIDelayTitleMusicEnd = INISetting (
		"DelayTitleMusicEnd",
		"General",
		"The title music continue playing during the loading screen",
		(SInt32)1);


	INISetting kINIMaxNumPlaylists = INISetting (
		"MaxNumPlaylists",
		"Internal",
		"Max number of expected playlists. Exceeding this number may cause BIIG problems!",
		(SInt32)100);


	INISetting kINIMaxNumMultipliers = INISetting (
		"MaxNumMultipliers",
		"Internal",
		"Max number of expected custom multipliers. Exceeding this number may cause BIIG problems!",
		(SInt32)150);


	INISetting kINIPrintTrack = INISetting (
		"PrintTrack",
		"Notifications",
		"Print the playing tracks on the console",
		(SInt32)1);


	void Initialize (const char* INIPath, void* Parameter);

	void applySettings (bool &delayTitleMusicEnd, bool &printNewTrack, int &numPlaylists, int &numMultipliers);

};


extern EMC2INISettings iniSettings;