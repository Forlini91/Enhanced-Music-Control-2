#pragma once

#include "INIManager.h"
#include "MusicPlayer.h"



#define INI_PATH "Data\\OBSE\\Plugins\\EnhancedMusicControl 2.ini"		//Path of the EMC2 ini



using namespace SME::INI;


class EMC2INISettings : INIManager {

public:


	INISetting kINIDelayTitleMusicEnd = INISetting (
		"DelayTitleMusicEnd",
		"General",
		"The title music continue playing during the loading screen",
		(SInt32)1);


	INISetting kINIMaxNumMultipliers = INISetting (
		"MaxNumMultipliers",
		"General",
		"Max number of expected custom multipliers. Exceeding this number may cause BIIG problems!",
		(SInt32)150);


	INISetting kINIMaxNumPlaylists = INISetting (
		"MaxNumPlaylists",
		"General",
		"Max number of expected playlists. Exceeding this number may cause BIIG problems!",
		(SInt32)100);


	INISetting kINIMusicSpeed = INISetting (
		"MusicSpeed",
		"MusicPlayer",
		"What's the music speed? (1.0 is the normal speed, above 1 is faster, below 1 is slower)",
		1.0F);


	INISetting kINITrackPauseMin = INISetting (
		"PauseMin",
		"MusicPlayer",
		"Min duration of pause between tracks.",
		0.0F);


	INISetting kINITrackPauseExtra = INISetting (
		"PauseExtra",
		"MusicPlayer",
		"Extra random duration of pause between tracks. Result = MinPause + (Random value between 0 and ExtraPause)",
		0.0F);


	INISetting kINIBattleMusicStartDelayMin = INISetting (
		"BattleStartMin",
		"MusicPlayer",
		"Min delay before combat music start",
		1.0F);


	INISetting kINIBattleMusicStartDelayExtra = INISetting (
		"BattleStartExtra",
		"MusicPlayer",
		"Extra delay before combat music start",
		0.0F);


	INISetting kINIBattleMusicEndDelayMin = INISetting (
		"BattleEndMin",
		"MusicPlayer",
		"Min delay before combat music ends",
		1.0F);


	INISetting kINIBattleMusicEndDelayExtra = INISetting (
		"BattleEndExtra",
		"MusicPlayer",
		"Extra delay before combat music ends",
		0.0F);


	INISetting kINIPreviousTrackRemember = INISetting (
		"PreviousTrackRemember",
		"MusicPlayer",
		"How many seconds EMC will remember the previous track when changing track?",
		20.0F);


	INISetting kINIFadeOut = INISetting (
		"FadeOut",
		"Fade",
		"Fade out time",
		1.0F);


	INISetting kINIFadeIn = INISetting (
		"FadeIn",
		"Fade",
		"Fade in time",
		1.0F);


	INISetting kINIFadeInBattle = INISetting (
		"FadeInBattle",
		"Fade",
		"Fade in when battle music start",
		500.0F);


	INISetting kINIFadeOutBattle = INISetting (
		"FadeOutBattle",
		"Fade",
		"Fade in when battle music ends",
		1000.0F);


	INISetting kINIPrintTrack = INISetting (
		"PrintTrack",
		"Notifications",
		"Print the playing tracks on the console",
		(SInt32)1);


	void Initialize (const char *INIPath, void *Parameter);

	void applySettings ();

};


extern EMC2INISettings iniSettings;