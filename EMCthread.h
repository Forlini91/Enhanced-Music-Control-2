#pragma once

#include "PendingPlaylist.h"

#define SLEEP_TIME 25



using namespace std;



//This is the main thread body
void EMCthread (void *throwaway);

void updateMusic ();

//This function updates musicPlayer's volume with the volume from Oblivion and the multipliers.
//This ensures that the player can use Oblivion's audio controls to control the music.
float getMusicVolume ();



bool applyPendingPlaylist (const PendingPlaylist& pendingPlaylist, int *fadeOutTime, int *fadeInTime);

bool playCustomTrack ();

bool playPlaylistTrack (int fadeOut, int fadeIn);

void updateMusicType (bool newTypeBattle);

void saveMusicInstance ();