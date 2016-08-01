#pragma once

#include "INIManager.h"


using namespace std;


SME::INI::INISetting	kINIMusicSpeed (
	"MusicSpeed",
	"General",
	"What's the music speed? (1.0 is the normal speed, above 1 is faster, below 1 is slower)",
	1.0F);


SME::INI::INISetting	kINIFadeOut (
	"FadeOut",
	"General",
	"Fade out time",
	1.0F);


SME::INI::INISetting	kINIFadeIn (
	"FadeIn",
	"General",
	"Fade in time",
	1.0F);


SME::INI::INISetting	kINITrackPause (
	"MinPause",
	"General",
	"Min duration of pause between tracks.",
	0.0F);


SME::INI::INISetting	kINITrackPauseExtra (
	"ExtraPause",
	"General",
	"Extra random duration of pause between tracks. Result = MinPause + (Random value between 0 and ExtraPause)",
	0.0F);


SME::INI::INISetting	kINIBattleMusicStartDelay (
	"MinBattleStart",
	"General",
	"Min delay before combat music start",
	1.0F);


SME::INI::INISetting	kINIBattleMusicStartDelayExtra ("ExtraBattleStart",
	"General",
	"Extra delay before combat music start",
	0.0F);


SME::INI::INISetting	kINIBattleMusicEndDelay (
	"MinBattleEnd",
	"General",
	"Min delay before combat music ends",
	1.0F);


SME::INI::INISetting	kINIBattleMusicEndDelayExtra (
	"ExtraBattleEnd",
	"General",
	"Extra delay before combat music ends",
	0.0F);


SME::INI::INISetting	kINIPreviousTrackRemember (
	"PreviousTrackRemember",
	"General",
	"How many seconds EMC will remember the previous track when changing track?",
	20.0F);


SME::INI::INISetting	kINIDelayTitleMusicEnd (
	"DelayTitleMusicEnd",
	"General",
	"The title music continue playing during the loading screen",
	(SInt32)1);


SME::INI::INISetting	kINIMaxNumPlaylists (
	"MaxNumPlaylists",
	"Internal",
	"Max number of expected playlists. Exceeding this number may cause BIIG problems!",
	(SInt32)100);


SME::INI::INISetting	kINIMaxNumMultipliers (
	"MaxNumMultipliers",
	"Internal",
	"Max number of expected custom multipliers. Exceeding this number may cause BIIG problems!",
	(SInt32)150);


SME::INI::INISetting	kINIPrintTrack (
	"PrintTrack",
	"Notifications",
	"Print the playing tracks on the console",
	(SInt32)1);








class EMC2INISettings : SME::INI::INIManager {

public:

	void Initialize (const char* INIPath, void* Parameter) {
		this->INIFilePath = INIPath;
		_MESSAGE ("INI Path: %s", INIPath);

		fstream INIStream (INIPath, fstream::in);
		
		bool CreateINI = false;
		if (INIStream.fail ()) {
			_MESSAGE ("INI File not found; Creating one...");
			CreateINI = true;
		}

		INIStream.close ();
		INIStream.clear ();

		RegisterSetting (&kINIMusicSpeed);
		RegisterSetting (&kINIFadeOut);
		RegisterSetting (&kINIFadeIn);
		RegisterSetting (&kINITrackPause);
		RegisterSetting (&kINITrackPauseExtra);
		RegisterSetting (&kINIBattleMusicStartDelay);
		RegisterSetting (&kINIBattleMusicStartDelayExtra);
		RegisterSetting (&kINIBattleMusicEndDelay);
		RegisterSetting (&kINIBattleMusicEndDelayExtra);
		RegisterSetting (&kINIPreviousTrackRemember);
		RegisterSetting (&kINIDelayTitleMusicEnd);
		RegisterSetting (&kINIMaxNumPlaylists);
		RegisterSetting (&kINIMaxNumMultipliers);
		RegisterSetting (&kINIPrintTrack);

		if (CreateINI)
			Save ();

		PopulateFromSection ("GenericSection");
	}

};