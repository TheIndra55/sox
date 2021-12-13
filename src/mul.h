#ifndef MUL_H
#define MUL_H

typedef struct
{
	int hertz;
	int startLoop;
	int endLoop;
	int channelCount;
	int reverbVol;
	int startSizeToLoad;
	int partialLoop;
	int loopAreaSize;
	int hasCinematic;
	int hasSubtitles;
	int loopStartFileOffset;
	int loopStartBundleOffset;
	int maxEEBytesPerRead;
	float mediaLength;
	float volLeft[12];
	float volRight[12];
} STRHEADER;

#endif
