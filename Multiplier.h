#pragma once

struct Multiplier {

private:
	volatile float* value;
	const bool userMultiplier;


public:

	Multiplier () : value (new float (1)), userMultiplier (true) {};
	Multiplier (float value) : value (new float (value)), userMultiplier (true) {};
	Multiplier (volatile float* var) : value (var), userMultiplier (false) {};
	~Multiplier ();

	float startValue = 1;
	float targetValue = 1;
	float fadeTime = 0;
	HANDLE hThread = CreateMutex (NULL, FALSE, NULL);
	bool isFading = false;
	bool isChanged = false;
	bool isDestroyed = false;
	bool saveGame = false;
	bool saveSession = false;

	volatile float getValue ();

	void setValue (float newValue);

	bool fadeVolume (float targetValue, float fadeTime);

	float clamp (float volume);
};