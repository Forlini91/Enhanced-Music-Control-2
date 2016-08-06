#pragma once




#define SLEEP_TIME 25


using namespace std;



//This is the main thread body
void MainThread (void *throwaway);

void MainThread_Initialization ();

//This function updates musicPlayer's volume with the volume from Oblivion and the multipliers.
//This ensures that the player can use Oblivion's audio controls to control the music.
void MainThread_SyncMusicVolume ();

void MainThread_SetPlaylist ();


bool MainThread_FixLevelUp (bool PlayerIsPlaying);

void MainThread_SelectNewTrack (float fadeOut, float fadeIn);

void MainThread_SelectCustomTrack ();

void MainThread_StorePreviousTrack (bool PlayerIsPlaying);

void MainThread_RestorePreviousTrack ();

void MainThread_UpdateMusicType (bool newTypeBattle);




void MainThread_ResetBattleTimer ();

void MainThread_ResetBattleDelayTimer ();

void MainThread_ResetAfterBattleDelayTimer ();

void MainThread_ResetPauseTimer ();

