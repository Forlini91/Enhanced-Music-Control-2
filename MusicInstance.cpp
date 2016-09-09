#include "MusicInstance.h"

#include "Globals.h"
#include "IniData.h"
#include "MusicTimes.h"
#include "TimeManagement.h"
#include "DebugMode.h"


using namespace std;

map<MusicType, MusicInstance> musicInstances;



void MusicInstance::setData (const string& curPlaylistName, const string& curTrackPath, LONGLONG curPosition, int maxRestoreTime) {
	lastValidTime = now + milliseconds (maxRestoreTime);
	playlistName = curPlaylistName;
	trackPath = curTrackPath;
	position = curPosition;
	_EMCDEBUG ("%lld | MusicInstance >> Save instance: \"%s\" > \"%s\", %f", timeStamp, curPlaylistName.c_str (), curTrackPath.c_str (), (float)(position / ONE_SECOND));
}




void initMusicInstances () {
	musicInstances.emplace (constructInPlace (MusicType::mtExplore));
	musicInstances.emplace (constructInPlace (MusicType::mtPublic));
	musicInstances.emplace (constructInPlace (MusicType::mtDungeon));
	musicInstances.emplace (constructInPlace (MusicType::mtCustom));
	musicInstances.emplace (constructInPlace (MusicType::mtBattle));
}


bool saveMusicInstance (MusicType musicType, const string& playlistName, const string& trackPath, LONGLONG position, LONGLONG duration) {
	//Assume the caller will lock the hMusicPlayerMutex
	if (musicType <= 4){
		if (position <= duration*previousTrackRememberRatio) {
			map<MusicType, MusicInstance>::iterator result = musicInstances.find (musicType);
			if (result != musicInstances.end ()) {
				int maxRestoreTime = musicType != MusicType::mtBattle ? musicTimes.getMusicRememberTime () : musicTimes.getBattleMusicRememberTime ();
				result->second.setData (playlistName, trackPath, position, maxRestoreTime);
				return true;
			} else {
				_EMCDEBUG ("%lld | MusicInstance >> Can't find music type %d", timeStamp, musicType);
			}
		} else {
			_EMCDEBUG ("%lld | MusicInstance >> Position %.2f exceed time limit %.2f * %.2f", timeStamp, (position / ONE_SECOND), (duration / ONE_SECOND), previousTrackRememberRatio);
		}
	} else {
		_EMCDEBUG ("%lld | MusicInstance >> Can't save music type %d", timeStamp, musicType);
	}
	return false;
	//Assume the caller will unlock the hMusicPlayerMutex
}


bool getMusicInstance (MusicType musicType, string *varPlaylistName, string *varTrackPath, LONGLONG *varPosition) {
	if (musicType <= 4) {
		map<MusicType, MusicInstance>::iterator result = musicInstances.find (musicType);
		if (result != musicInstances.end ()) {
			MusicInstance& musicInstance = result->second;
			if (musicInstance.position >= 0) {
				PlaylistsMap::const_iterator cit = playlists.find (musicInstance.playlistName);
				if (cit != playlists.end ()){
					if (now < musicInstance.lastValidTime){
						if (cit->second.hasTrack (musicInstance.trackPath)) {
							*varPlaylistName = musicInstance.playlistName;
							*varTrackPath = musicInstance.trackPath;
							*varPosition = musicInstance.position;
							return true;
						} else {
							_EMCDEBUG ("%lld | MusicInstance >> Playlist \"%s\" doesn't have the track %s", timeStamp, musicInstance.playlistName.c_str (), musicInstance.trackPath.c_str ());
						}
					} else {
						_EMCDEBUG ("%lld | MusicInstance >> Music instance is no more valid: %lld", timeStamp, musicInstance.lastValidTime);
					}
				} else {
					_EMCDEBUG ("%lld | MusicInstance >> Playlist has been deleted: %s", timeStamp, musicInstance.playlistName.c_str ());
				}
				musicInstance.position = -ONE_SECOND;
			} else {
				_EMCDEBUG ("%lld | MusicInstance >> Music position is negative: %.2f", timeStamp, musicType, (musicInstance.position / ONE_SECOND));
			}
		} else {
			_EMCDEBUG ("%lld | MusicInstance >> Can't find data for music type %d", timeStamp, musicType);
		}
	} else {
		_EMCDEBUG ("%lld | MusicInstance >> Can't load music type %d", timeStamp, musicType);
	}
	return false;
}