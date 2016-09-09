#include "EMC2INISettings.h"

#include "Globals.h"
#include "IniData.h"
#include "Multiplier.h"
#include "Playlist.h"
#include "MusicPlayer.h"
#include "MusicTimes.h"


EMC2INISettings iniSettings;


using namespace std;


void EMC2INISettings::Initialize (const char *INIPath, void *Parameter) {
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

	//General
	RegisterSetting (&kINIDelayTitleMusicEnd);
	RegisterSetting (&kINIMaxNumMultipliers);
	RegisterSetting (&kINIMaxNumPlaylists);

	//Music player
	RegisterSetting (&kINIMusicSpeed);
	RegisterSetting (&kINITrackPauseMin);
	RegisterSetting (&kINITrackPauseExtra);
	RegisterSetting (&kINIBattleMusicStartDelayMin);
	RegisterSetting (&kINIBattleMusicStartDelayExtra);
	RegisterSetting (&kINIBattleMusicEndDelayMin);
	RegisterSetting (&kINIBattleMusicEndDelayExtra);
	RegisterSetting (&kINIPreviousTrackRemember);
	RegisterSetting (&kINIPreviousTrackRememberBattle);
	RegisterSetting (&kINIPreviousTrackRememberRatio);

	//Fade
	RegisterSetting (&kINIFadeOut);
	RegisterSetting (&kINIFadeIn);
	RegisterSetting (&kINIFadeInBattle);
	RegisterSetting (&kINIFadeOutBattle);

	//Notifications
	RegisterSetting (&kINIPrintTrack);

	if (CreateINI)
		Save ();

	PopulateFromSection ("General");
	PopulateFromSection ("MusicPlayer");
	PopulateFromSection ("Fade");
	PopulateFromSection ("Notifications");

}



void EMC2INISettings::applySettings () {
	_MESSAGE ("Initialization >> EMC2 ini data");
	float val;

	//General
	delayTitleMusicEnd = (kINIDelayTitleMusicEnd.GetData ().i > 0);

	val = kINIMaxNumMultipliers.GetData ().i;
	numMultipliers = clamp (val, 0, 10000);
	multipliersCustom.reserve (numMultipliers + 1);

	val = kINIMaxNumPlaylists.GetData ().i;
	numPlaylists = clamp (val, 0, 10000);
	playlists.reserve (numPlaylists + 1);


	//MusicPlayer
	val = kINIMusicSpeed.GetData ().f;
	val = clamp (val, 0, 10);		//Up to 10x speed
	musicPlayer.setMusicSpeed (static_cast<double>(val));

	val = kINITrackPauseMin.GetData ().f;
	val = clamp (val, 0, 3600);		//Up to one hour
	musicTimes.setMinPauseTime (val * 1000);

	val = kINITrackPauseExtra.GetData ().f;
	val = clamp (val, 0, 3600);		//Up to one hour
	musicTimes.setExtraPauseTime (val * 1000);

	val = kINIBattleMusicStartDelayMin.GetData ().f;
	val = clamp (val, 0, 300);		//Up to 5 minutes delay.
	musicTimes.setMinStartBattleDelay (val * 1000);

	val = kINIBattleMusicStartDelayExtra.GetData ().f;
	val = clamp (val, 0, 300);		//Up to 5 minutes delay
	musicTimes.setExtraStartBattleDelay (val * 1000);

	val = kINIBattleMusicEndDelayMin.GetData ().f;
	val = clamp (val, 0, 300);		//Up to 5 minutes delay
	musicTimes.setMinStopBattleDelay (val * 1000);

	val = kINIBattleMusicEndDelayExtra.GetData ().f;
	val = clamp (val, 0, 300);		//Up to 5 minutes delay
	musicTimes.setExtraStopBattleDelay (val * 1000);

	val = kINIPreviousTrackRemember.GetData ().f;
	val = clamp (val, 0, 600);		//Up to 10 minutes
	musicTimes.setMusicRememberTime (val * 1000);

	val = kINIPreviousTrackRememberBattle.GetData ().f;
	val = clamp (val, 0, 600);		//Up to 10 minutes
	musicTimes.setBattleMusicRememberTime (val * 1000);

	val = kINIPreviousTrackRememberRatio.GetData ().f;
	previousTrackRememberRatio = clamp (val, 0, 1);			//From 0 to 100% of track duration


	musicTimes.recalculatePauseTime ();
	musicTimes.recalculateStartBattleDelay ();
	musicTimes.recalculateStopBattleDelay ();
	
	

	//Fade
	val = kINIFadeOut.GetData ().f;
	val = clamp (val, 0, 100000);
	musicTimes.setFadeOut (val * 1000);

	val = kINIFadeIn.GetData ().f;
	val = clamp (val, 0, 100000);
	musicTimes.setFadeIn (val * 1000);

	val = kINIFadeOutBattle.GetData ().f;
	fadeInBattle = clamp (val, 0, 100000) * 1000;

	val = kINIFadeInBattle.GetData ().f;
	fadeOutBattle = clamp (val, 0, 100000) * 1000;


	//Notification
	printNewTrack = (kINIPrintTrack.GetData ().i > 0);

}