#include "sox_i.h"
#include "ima_rw.h"

#include "mul.h"

// Tomb Raider Legend MultiplexStream format, based on wav.c

typedef struct
{
	short* samples;
	unsigned char* packet; 
	int samplesPerBlock;
	int blockAlign;
	int state[16];

	int padding_length;
} priv_t;

static int startwrite(sox_format_t * ft)
{
	priv_t* mul = (priv_t*) ft->priv;

	lsx_ima_init_table();

	int channels = ft->signal.channels;
	mul->blockAlign = 36;
	mul->samplesPerBlock = lsx_ima_samples_in((size_t) 0, (size_t) channels, (size_t) mul->blockAlign, (size_t) 0);

	mul->samples = lsx_malloc(channels * mul->samplesPerBlock * sizeof(short));
	mul->packet = lsx_malloc((size_t)mul->blockAlign);

	// write mul header
	STRHEADER header = { 0 };
	header.hertz = ft->signal.rate;
	header.startLoop = -1;
	header.endLoop = lsx_ima_samples_in((size_t) ft->signal.length, (size_t) channels, (size_t) mul->blockAlign, (size_t) mul->samplesPerBlock) / 2;
	header.channelCount = ft->signal.channels;
	header.loopStartFileOffset = 2048;

	header.volLeft[0] = 1;
	header.volRight[0] = 1;

	lsx_writebuf(ft, &header, sizeof(STRHEADER));

	// game will always seek to 2048 next
	lsx_seeki(ft, 2048, SEEK_SET);

	return SOX_SUCCESS;
}

static size_t write_samples(sox_format_t * ft, const sox_sample_t *buf, size_t len)
{
	priv_t* mul = (priv_t*) ft->priv;
	
	int numSamples = (len / mul->samplesPerBlock);
	size_t tell = lsx_tell(ft);

	// write packet header
	lsx_writedw(ft, 0); // sound packet
	lsx_writedw(ft, (numSamples * mul->blockAlign) + 16); // length
	lsx_seeki(ft, 16 - 8, SEEK_CUR);

	// write packet internal header
	lsx_writedw(ft, numSamples * mul->blockAlign); // bytes remaining
	lsx_seeki(ft, 16 - 4, SEEK_CUR);

	for (int i = 0; i < numSamples; i++)
	{
		for (int j = 0; j < mul->samplesPerBlock; j++)
		{
			mul->samples[j] = (buf + (i * mul->samplesPerBlock))[j] >> 16;
		}

		lsx_ima_block_mash_i((unsigned) ft->signal.channels, mul->samples, mul->samplesPerBlock, mul->state, mul->packet, 9);
		lsx_writebuf(ft, mul->packet, mul->blockAlign);
	}

	mul->padding_length += lsx_tell(ft) - tell;

	// todo find right calculation for game padding so game doesnt shit when loading the buffer
	//if (mul->padding_length > 60000)
	{
		int remaining = 65536 - mul->padding_length - 16;

		// write a padding packet
		lsx_writedw(ft, 2);
		lsx_writedw(ft, remaining);
		lsx_seeki(ft, 16 - 8, SEEK_CUR);

		lsx_seeki(ft, remaining - 4, SEEK_CUR);
		lsx_writedw(ft, 0);

		mul->padding_length = 0;

		return len;
	}

	return len;
}

static int stopwrite(sox_format_t * ft)
{
	priv_t* mul = (priv_t*) ft->priv;

	free(mul->samples);
	free(mul->packet);

	return SOX_SUCCESS;
}

LSX_FORMAT_HANDLER(mul)
{
	static char const * const names[] = {"mul", NULL};
	static unsigned const write_encodings[] = {SOX_ENCODING_SIGN2, 16, 0, 0};

	static sox_format_handler_t const handler = {
		SOX_LIB_VERSION_CODE, "MultiplexStream", names, 0,
		NULL, NULL, NULL,
		startwrite, write_samples, stopwrite,
		NULL, write_encodings, NULL, sizeof(priv_t)};

	return &handler;
}
