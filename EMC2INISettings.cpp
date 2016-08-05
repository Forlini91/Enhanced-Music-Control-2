#include "EMC2INISettings.h"

#include "Globals.h"
#include "IniData.h"
#include "Multiplier.h"
#include "Playlist.h"


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

	RegisterSetting (&kINIDelayTitleMusicEnd);
	RegisterSetting (&kINIMaxNumMultipliers);
	RegisterSetting (&kINIMaxNumPlaylists);
	RegisterSetting (&kINIMusicSpeed);
	RegisterSetting (&kINITrackPauseMin);
	RegisterSetting (&kINITrackPauseExtra);
	RegisterSetting (&kINIBattleMusicStartDelayMin);
	RegisterSetting (&kINIBattleMusicStartDelayExtra);
	RegisterSetting (&kINIBattleMusicEndDelayMin);
	RegisterSetting (&kINIBattleMusicEndDelayExtra);
	RegisterSetting (&kINIPreviousTrackRemember);
	RegisterSetting (&kINIFadeOut);
	RegisterSetting (&kINIFadeIn);
	RegisterSetting (&kINIFadeInBattle);
	RegisterSetting (&kINIFadeOutBattle);
	RegisterSetting (&kINIPrintTrack);

	if (CreateINI)
		Save ();

	PopulateFromSection ("GenericSection");
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
	val = clamp (val, 0, 100);
	musicPlayer.setMusicSpeed (false, (double)val);

	val = kINITrackPauseMin.GetData ().f;
	val = clamp (val, 0, 100000);
	musicPlayer.setMinPauseTime (false, val * 1000);

	val = kINITrackPauseExtra.GetData ().f;
	val = clamp (val, 0, 100000);
	musicPlayer.setExtraPauseTime (false, val * 1000);

	val = kINIBattleMusicStartDelayMin.GetData ().f;
	val = clamp (val, 0, 100000);
	musicPlayer.setMinBattleDelay (false, val * 1000);

	val = kINIBattleMusicStartDelayExtra.GetData ().f;
	val = clamp (val, 0, 100000);
	musicPlayer.setExtraBattleDelay (false, val * 1000);

	val = kINIBattleMusicEndDelayMin.GetData ().f;
	val = clamp (val, 0, 100000);
	musicPlayer.setMinAfterBattleDelay (false, val * 1000);

	val = kINIBattleMusicEndDelayExtra.GetData ().f;
	val = clamp (val, 0, 100000);
	musicPlayer.setExtraAfterBattleDelay (false, val * 1000);

	val = kINIBattleMusicEndDelayExtra.GetData ().f;
	val = clamp (val, 0, 100000);
	musicPlayer.setExtraAfterBattleDelay (false, val * 1000);

	val = kINIPreviousTrackRemember.GetData ().f;
	val = clamp (val, 0, 100000);
	musicPlayer.setMaxRestoreTime (false, val * 1000);


	//Fade
	val = kINIFadeOut.GetData ().f;
	val = clamp (val, 0, 100000);
	musicPlayer.setCurrentFadeOutLength (false, val * 1000);

	val = kINIFadeIn.GetData ().f;
	val = clamp (val, 0, 100000);
	musicPlayer.setCurrentFadeInLength (false, val * 1000);

	val = kINIFadeOutBattle.GetData ().f;
	fadeInBattle = clamp (val, 0, 100000) * 1000;

	val = kINIFadeInBattle.GetData ().f;
	fadeOutBattle = clamp (val, 0, 100000) * 1000;


	//Notification
	printNewTrack = (kINIPrintTrack.GetData ().i > 0);

}