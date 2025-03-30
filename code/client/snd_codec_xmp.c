/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2005 Stuart Dalton (badcdev@gmail.com)
Copyright (C) 2014 leilei

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

int	xmpspeed = 22050; // assume 22050hz unless........
#ifdef USE_CODEC_XMP

// includes for the Q3 sound system
#include "client.h"
#include "snd_codec.h"

// includes for XMP
#include <xmp.h>




// leilei - XMP
xmp_context xmpsong;
//int sound_init(int, int);
void sound_play(void *, int);
void sound_deinit(void);



static float fadey;
extern cvar_t *s_xmp_startPattern;
static int startpat;
static int gopat;
extern	int	samplingrate;		// from snd_dma

void S_XMP_StartSong ( void )
{



}

void S_XMP_EndSong ( void )
{



}


// TODO: A callback to set this appropriately from game context (monster sight, level triggers)
void S_XMP_CheckForFade ( void )
{
	gopat = s_xmp_startPattern->integer;

	if ((gopat != startpat))
	{
		fadey -= 0.025f;
	//	Com_Printf("AHHHFADING TO PATTERN %i!!! %f\n", startpat, fadey);
		xmp_set_player(xmpsong, XMP_PLAYER_VOLUME, (fadey * 128));
	}

	if ((startpat != gopat) && (fadey < 0.01))
	{
		startpat = gopat;
		xmp_set_position(xmpsong, startpat);
		xmp_set_player(xmpsong, XMP_PLAYER_VOLUME, 128);
		fadey = 1;
	}

}

int	aintistreaming = 0;
char *streamilename;
/*
=================
S_XMP_CodecLoad
=================
*/
void *S_XMP_CodecLoad(const char *filename, snd_info_t *info)
{
	// This is not a codec for sound effects
	return NULL;
}

/*
=================
S_XMP_CodecOpenStream
=================
*/

// FIXME: there's a memory leak here if you start the same song many many many many times?
snd_stream_t *S_XMP_CodecOpenStream(const char *filename)
{
	// First let's close whatever song we had....

	// Open
	snd_stream_t *rv = S_CodecUtilOpen(filename, &xmp_codec);
	if(!rv) {
		return NULL;
	}

//	Com_Printf("OPENSTREAM %s\n", filename);

	{
		fileHandle_t file;

		// Try to open the file
		FS_FOpenFileRead(filename, &file, qtrue);
		if(!file) {
			Com_Printf( S_COLOR_RED "ERROR: No.\"%s\"\n",filename);
			return NULL;
		}


		// Allocate some memory
		long	thelength = FS_ReadFile(filename, NULL);


		void *buffer = Hunk_AllocateTempMemory(thelength);
		if(!buffer) {
			FS_FCloseFile(file);
			Com_Printf( S_COLOR_RED "ERROR: Out of memory reading \"%s\"\n", filename);
			return NULL;
		}

		FS_Read(buffer, thelength, file);


		// OK!
		struct xmp_module_info mi;

		xmpsong = xmp_create_context();
		int itsloaded = 0;

		itsloaded = xmp_load_module_from_memory(xmpsong, buffer, 0);

		// Free our memory and close the file.
		Hunk_FreeTempMemory(buffer);
		FS_FCloseFile(file);		// unfortunately these do not help with the leak

		if (itsloaded == 0)
		{
			itsloaded = xmp_start_player(xmpsong, xmpspeed, 0);	
			startpat=s_xmp_startPattern->integer;
			gopat = startpat;
			fadey = 1.0;
			xmp_set_player(xmpsong, XMP_PLAYER_VOLUME, 128);
			xmp_set_position(xmpsong, startpat);
		}

		if (itsloaded == 0) {
			//	Com_Printf("XMP loaded our buffer of the file %s which is %i long \n", filename, thelength);
			xmp_get_module_info(xmpsong, &mi);
			//	Com_Printf("Song Name: %s\n", mi.mod->name);
			//	Com_Printf("CODECLOAD %s\n", filename);
		}

	}


	rv->info.size = 1337;	// This doesn't matter!
	rv->pos = 0;

	rv->info.rate = xmpspeed;
	rv->info.width = 2;
	rv->info.channels = 2;
	rv->info.samples = 12;

	return rv;
}

/*
=================
S_XMP_CodecCloseStream
=================
*/
void S_XMP_CodecCloseStream(snd_stream_t *stream)
{
	xmp_end_player(xmpsong);
	xmp_release_module(xmpsong);
	xmp_free_context(xmpsong);
	S_CodecUtilClose(&stream);
}


/*
=================
S_OGG_CodecReadStream
=================
*/
int S_XMP_CodecReadStream(snd_stream_t *stream, int bytes, void *buffer)
{
	// buffer handling
	struct xmp_module_info mi;
	struct xmp_frame_info fi;

	// check if input is valid
	if(!(stream && buffer)) {
		return 0;
	}

	if(bytes <= 0) {
		return 0;
	}

	int yeah=xmp_play_buffer(xmpsong, buffer, bytes, 0);

	if (yeah == 0) {	// if we can play it...
		xmp_get_frame_info(xmpsong, &fi);
		xmp_get_module_info(xmpsong, &mi);
	}
	else {
		return 0;
	}

	S_XMP_CheckForFade();

	return bytes;
}


snd_codec_t xmp_codec = {
	"umx",
	S_XMP_CodecLoad,
	S_XMP_CodecOpenStream,
	S_XMP_CodecReadStream,
	S_XMP_CodecCloseStream,
	NULL
};

snd_codec_t xmp_mod_codec = {
	"mod",
	S_XMP_CodecLoad,
	S_XMP_CodecOpenStream,
	S_XMP_CodecReadStream,
	S_XMP_CodecCloseStream,
	NULL
};

snd_codec_t xmp_it_codec = {
	"it",
	S_XMP_CodecLoad,
	S_XMP_CodecOpenStream,
	S_XMP_CodecReadStream,
	S_XMP_CodecCloseStream,
	NULL
};

snd_codec_t xmp_s3m_codec = {
	"s3m",
	S_XMP_CodecLoad,
	S_XMP_CodecOpenStream,
	S_XMP_CodecReadStream,
	S_XMP_CodecCloseStream,
	NULL
};

snd_codec_t xmp_xm_codec = {
	"xm",
	S_XMP_CodecLoad,
	S_XMP_CodecOpenStream,
	S_XMP_CodecReadStream,
	S_XMP_CodecCloseStream,
	NULL
};

#endif // USE_CODEC_XMP
