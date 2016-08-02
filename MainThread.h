#pragma once

#include "ThreadRequest.h"
#include "ThreadState.h"

using namespace std;



bool isInRange (float val, float min, float max);



void MainThread_ResetStartBattleTimer ();

void MainThread_ResetBattleTimer ();

void MainThread_ResetAfterBattleTimer ();

void MainThread_ResetPauseTimer ();

//This is a function to be used by sentry Thread.
//It updates musicPlayer's volume with the volume from Oblivion.
//This ensures that the player can use Oblivion's audio controls
//to control the music.
void MainThread_SyncMusicVolume (void);

bool MainThread_SwapPlaylist ();

void MainThread_StorePreviousTrack (bool PlayerIsPlaying);

bool MainThread_FixLevelUp (bool PlayerIsPlaying);

void MainThread_SelectCustomTrack ();

void MainThread_RestorePreviousTrack ();

void MainThread_SelectNewTrack (float fadeOut, float fadeIn);

void MainThread_UpdateMusicType (bool newTypeBattle);

void MainThread (void *throwaway);