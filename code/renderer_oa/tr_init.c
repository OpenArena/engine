/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

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
// tr_init.c -- functions that are not called every frame

#include "tr_local.h"

glconfig_t  glConfig;
qboolean    textureFilterAnisotropic = qfalse;
int         maxAnisotropy = 0;
float       displayAspect = 0.0f;
qboolean    vertexShaders = qfalse;
qboolean	postprocess = qfalse;
qboolean    textureCompressionSupport = qfalse;		// leilei - compressed textures
qboolean    palettedTextureSupport = qfalse;		// leilei - paletted textures

char 		depthimage;

glstate_t	glState;

static void GfxInfo_f( void );

extern int voodootype;

#ifdef USE_RENDERER_DLOPEN
cvar_t  *com_altivec;
#endif


cvar_t	*r_flareSize;
cvar_t	*r_flareFade;
cvar_t	*r_flareCoeff;

cvar_t	*r_railWidth;
cvar_t	*r_railCoreWidth;
cvar_t	*r_railSegmentLength;

cvar_t	*r_ignoreFastPath;

cvar_t	*r_verbose;
cvar_t	*r_ignore;

cvar_t	*r_detailTextures;
cvar_t	*r_detailTextureScale;
cvar_t	*r_detailTextureLayers;


cvar_t	*r_znear;
cvar_t	*r_zproj;
cvar_t	*r_stereoSeparation;

cvar_t	*r_skipBackEnd;

cvar_t	*r_stereoEnabled;
cvar_t	*r_anaglyphMode;

cvar_t	*r_monolightmaps;

cvar_t	*r_ignorehwgamma;
cvar_t	*r_measureOverdraw;

cvar_t	*r_inGameVideo;
cvar_t	*r_fastsky;
cvar_t	*r_drawSun;
cvar_t	*r_dynamiclight;
cvar_t	*r_dlightBacks;

cvar_t	*r_lodbias;
cvar_t	*r_lodscale;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_speeds;
cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_facePlaneCull;
cvar_t	*r_showcluster;
cvar_t	*r_nocurves;

cvar_t	*r_allowExtensions;

cvar_t	*r_ext_compressed_textures;
cvar_t	*r_ext_multitexture;
cvar_t	*r_ext_compiled_vertex_array;
cvar_t	*r_ext_texture_env_add;
cvar_t	*r_ext_texture_filter_anisotropic;
cvar_t	*r_ext_max_anisotropy;
cvar_t	*r_ext_vertex_shader;
cvar_t	*r_loadDDS;			// leilei - loading DDS textures
cvar_t	*r_ext_paletted_texture;	// leilei - Paletted Texture
cvar_t	*r_ext_gamma_control;		// leilei - 3dfx gamma control
cvar_t	*r_postprocess;

cvar_t	*r_ignoreGLErrors;
cvar_t	*r_logFile;

cvar_t	*r_stencilbits;
cvar_t	*r_depthbits;
cvar_t	*r_colorbits;
cvar_t	*r_primitives;
cvar_t	*r_texturebits;
cvar_t  *r_ext_multisample;

cvar_t	*r_drawBuffer;
cvar_t	*r_lightmap;
cvar_t	*r_vertexLight;
cvar_t	*r_uiFullScreen;
cvar_t	*r_shadows;
cvar_t	*r_flares;
cvar_t	*r_mode;
cvar_t	*r_nobind;
cvar_t	*r_singleShader;
cvar_t	*r_roundImagesDown;
cvar_t	*r_colorMipLevels;
cvar_t	*r_picmip;
cvar_t	*r_iconmip;
cvar_t	*r_iconBits;
cvar_t	*r_lightmapBits;
cvar_t	*r_showtris;
cvar_t	*r_showsky;
cvar_t	*r_shownormals;
cvar_t	*r_finish;
cvar_t	*r_clear;
cvar_t	*r_swapInterval;
cvar_t	*r_textureMode;
cvar_t	*r_offsetFactor;
cvar_t	*r_offsetUnits;
cvar_t	*r_gamma;
cvar_t	*r_intensity;
cvar_t	*r_lockpvs;
cvar_t	*r_noportals;
cvar_t	*r_portalOnly;

cvar_t	*r_subdivisions;
cvar_t	*r_lodCurveError;

cvar_t	*r_fullscreen;
cvar_t  *r_noborder;

cvar_t	*r_customwidth;
cvar_t	*r_customheight;
cvar_t	*r_customPixelAspect;

cvar_t	*r_overBrightBits;
cvar_t	*r_mapOverBrightBits;

cvar_t	*r_debugSurface;
cvar_t	*r_simpleMipMaps;

cvar_t	*r_showImages;

cvar_t	*r_ambientScale;
cvar_t	*r_directedScale;
cvar_t	*r_debugLight;
cvar_t	*r_debugSort;
cvar_t	*r_printShaders;
cvar_t	*r_saveFontData;

cvar_t	*r_marksOnTriangleMeshes;

cvar_t	*r_aviMotionJpegQuality;
cvar_t	*r_screenshotJpegQuality;

cvar_t	*r_maxpolys;
int		max_polys;
cvar_t	*r_maxpolyverts;
int		max_polyverts;

// tcpp
cvar_t	*r_lensReflection1;
cvar_t	*r_lensReflection2;
cvar_t	*r_lensReflectionBrightness;

// leilei

cvar_t	*r_flareMethod;		// method of flare intensity
cvar_t	*r_flareQuality;	// testing quality of the flares.
cvar_t	*r_flareSun;		// type of flare to use for the sun
cvar_t	*r_flareDelay;		// time delay for medium quality flare testing

cvar_t	*r_specMode;
//cvar_t	*r_waveMode;
cvar_t	*r_flaresDlight;
cvar_t	*r_flaresDlightShrink;
cvar_t	*r_flaresDlightFade;
cvar_t	*r_flaresDlightOpacity;
cvar_t	*r_flaresDlightScale;

cvar_t	*r_lightmapColorNorm;

//cvar_t	*r_flaresSurfradii;
cvar_t	*r_alternateBrightness;		// leilei - linux overbright fix
cvar_t	*r_mockvr;		// Leilei - for debugging PVR only!
cvar_t	*r_leifx;		// Leilei - leifx nostalgia filter
cvar_t	*r_shadeMethod;		// Leilei
cvar_t	*r_particles;		// Leilei - particle effects motif
#ifdef BROKEN_MDRPHYS
cvar_t	*r_mdrPhysics;		// Leilei - mdr physics
#endif

cvar_t	*r_skytess;	// leilei - lower detail of skies

cvar_t	*r_leidebug;		// Leilei - debug
cvar_t	*r_leidebugeye;		// Leilei - eye debug

cvar_t	*r_suggestiveThemes;		// leilei - mature content control

cvar_t	*r_textureDither;	// leilei - Dithered texture

cvar_t	*r_lerpbias;		// Leilei - lerping bias

// leilei - fallback shader hack




#ifdef USE_FALLBACK_GLSL
extern const char *fallbackShader_brightness_vp;
extern const char *fallbackShader_brightness_fp;
extern const char *fallbackShader_leifx_dither_vp;
extern const char *fallbackShader_leifx_dither_fp;
extern const char *fallbackShader_leifx_filter_vp;
extern const char *fallbackShader_leifx_filter_fp;
extern const char *fallbackShader_leifx_gamma_vp;
extern const char *fallbackShader_leifx_gamma_fp;
extern const char *fallbackShader_leifx_vgasignal_vp;
extern const char *fallbackShader_leifx_vgasignal_fp;
#endif



/*
** InitOpenGL
**
** This function is responsible for initializing a valid OpenGL subsystem.  This
** is done by calling GLimp_Init (which gives us a working OGL subsystem) then
** setting variables, checking GL constants, and reporting the gfx system config
** to the user.
*/
static void InitOpenGL( void )
{
	char renderer_buffer[1024];

	//
	// initialize OS specific portions of the renderer
	//
	// GLimp_Init directly or indirectly references the following cvars:
	//		- r_fullscreen
	//		- r_mode
	//		- r_(color|depth|stencil)bits
	//		- r_ignorehwgamma
	//		- r_gamma
	//

	if ( glConfig.vidWidth == 0 ) {
		GLint		temp;

		GLimp_Init();
		GLimp_InitExtraExtensions();

		strcpy( renderer_buffer, glConfig.renderer_string );
		Q_strlwr( renderer_buffer );

		// OpenGL driver constants
		qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &temp );
		glConfig.maxTextureSize = temp;

		// stubbed or broken drivers may have reported 0...
		if ( glConfig.maxTextureSize <= 0 ) {
			glConfig.maxTextureSize = 0;
		}
	}

	// set default state
	GL_SetDefaultState();
}

/*
==================
GL_CheckErrors
==================
*/
void GL_CheckErrors( void )
{
	int		err;
	char	s[64];

	err = qglGetError();
	if ( err == GL_NO_ERROR ) {
		return;
	}
	if ( r_ignoreGLErrors->integer ) {
		return;
	}
	switch( err ) {
	case GL_INVALID_ENUM:
		strcpy( s, "GL_INVALID_ENUM" );
		break;
	case GL_INVALID_VALUE:
		strcpy( s, "GL_INVALID_VALUE" );
		break;
	case GL_INVALID_OPERATION:
		strcpy( s, "GL_INVALID_OPERATION" );
		break;
	case GL_STACK_OVERFLOW:
		strcpy( s, "GL_STACK_OVERFLOW" );
		break;
	case GL_STACK_UNDERFLOW:
		strcpy( s, "GL_STACK_UNDERFLOW" );
		break;
	case GL_OUT_OF_MEMORY:
		strcpy( s, "GL_OUT_OF_MEMORY" );
		break;
	default:
		Com_sprintf( s, sizeof(s), "%i", err);
		break;
	}

	ri.Error( ERR_FATAL, "GL_CheckErrors: %s", s );
}


/*
** R_GetModeInfo
*/
typedef struct vidmode_s {
	const char *description;
	int width, height;
	float pixelAspect;		// pixel width / height
} vidmode_t;

vidmode_t r_vidModes[] = {
	{ "Mode  0: 320x240",		320,	240,	1 },
	{ "Mode  1: 400x300",		400,	300,	1 },
	{ "Mode  2: 512x384",		512,	384,	1 },
	{ "Mode  3: 640x480 (480p)",	640,	480,	1 },
	{ "Mode  4: 800x600",		800,	600,	1 },
	{ "Mode  5: 960x720",		960,	720,	1 },
	{ "Mode  6: 1024x768",		1024,	768,	1 },
	{ "Mode  7: 1152x864",		1152,	864,	1 },
	{ "Mode  8: 1280x1024",		1280,	1024,	1 },
	{ "Mode  9: 1600x1200",		1600,	1200,	1 },
	{ "Mode 10: 2048x1536",		2048,	1536,	1 },
	{ "Mode 11: 856x480",		856,	480,	1 },		// Q3 MODES END HERE AND EXTENDED MODES BEGIN
	{ "Mode 12: 1280x720 (720p)",	1280,	720,	1 },
	{ "Mode 13: 1280x768",		1280,	768,	1 },
	{ "Mode 14: 1280x800",		1280,	800,	1 },
	{ "Mode 15: 1280x960",		1280,	960,	1 },
	{ "Mode 16: 1360x768",		1360,	768,	1 },
	{ "Mode 17: 1366x768",		1366,	768,	1 }, // yes there are some out there on that extra 6
	{ "Mode 18: 1360x1024",		1360,	1024,	1 },
	{ "Mode 19: 1400x1050",		1400,	1050,	1 },
	{ "Mode 20: 1400x900",		1400,	900,	1 },
	{ "Mode 21: 1600x900",		1600,	900,	1 },
	{ "Mode 22: 1680x1050",		1680,	1050,	1 },
	{ "Mode 23: 1920x1080 (1080p)",	1920,	1080,	1 },
	{ "Mode 24: 1920x1200",		1920,	1200,	1 },
	{ "Mode 25: 1920x1440",		1920,	1440,	1 },
	{ "Mode 26: 2560x1600",		2560,	1600,	1 },
	{ "Mode 27: 3840x2160 (4K)",	3840,	2160,	1 }
};
static int	s_numVidModes = ARRAY_LEN( r_vidModes );

qboolean R_GetModeInfo( int *width, int *height, float *windowAspect, int mode )
{
	vidmode_t	*vm;
	float			pixelAspect;

	if ( mode < -1 ) {
		return qfalse;
	}
	if ( mode >= s_numVidModes ) {
		return qfalse;
	}

	if ( mode == -1 ) {
		*width = r_customwidth->integer;
		*height = r_customheight->integer;
		pixelAspect = r_customPixelAspect->value;
	}
	else {
		vm = &r_vidModes[mode];

		*width  = vm->width;
		*height = vm->height;
		pixelAspect = vm->pixelAspect;
	}

	*windowAspect = (float)*width / ( *height * pixelAspect );

	return qtrue;
}

/*
** R_ModeList_f
*/
static void R_ModeList_f( void )
{
	int i;

	ri.Printf( PRINT_ALL, "\n" );
	for ( i = 0; i < s_numVidModes; i++ ) {
		ri.Printf( PRINT_ALL, "%s\n", r_vidModes[i].description );
	}
	ri.Printf( PRINT_ALL, "\n" );
}


/*
==============================================================================

						SCREEN SHOTS

NOTE TTimo
some thoughts about the screenshots system:
screenshots get written in fs_homepath + fs_gamedir
vanilla q3 .. baseq3/screenshots/ *.tga
team arena .. missionpack/screenshots/ *.tga

two commands: "screenshot" and "screenshotJPEG"
we use statics to store a count and start writing the first screenshot/screenshot????.tga (.jpg) available
(with FS_FileExists / FS_FOpenFileWrite calls)
FIXME: the statics don't get a reinit between fs_game changes

==============================================================================
*/

/*
==================
RB_ReadPixels

Reads an image but takes care of alignment issues for reading RGB images.

Reads a minimum offset for where the RGB data starts in the image from
integer stored at pointer offset. When the function has returned the actual
offset was written back to address offset. This address will always have an
alignment of packAlign to ensure efficient copying.

Stores the length of padding after a line of pixels to address padlen

Return value must be freed with ri.Hunk_FreeTempMemory()
==================
*/

byte *RB_ReadPixels(int x, int y, int width, int height, size_t *offset, int *padlen)
{
	byte *buffer, *bufstart;
	int padwidth, linelen;
	GLint packAlign;

	qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

	linelen = width * 3;
	padwidth = PAD(linelen, packAlign);

	// Allocate a few more bytes so that we can choose an alignment we like
	buffer = ri.Hunk_AllocateTempMemory(padwidth * height + *offset + packAlign - 1);

	bufstart = PADP((intptr_t) buffer + *offset, packAlign);
	qglReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, bufstart);

	*offset = bufstart - buffer;
	*padlen = padwidth - linelen;

	return buffer;
}

/*
==================
RB_TakeScreenshot
==================
*/
void RB_TakeScreenshot(int x, int y, int width, int height, char *fileName)
{
	byte *allbuf, *buffer;
	byte *srcptr, *destptr;
	byte *endline, *endmem;
	byte temp;

	int linelen, padlen;
	size_t offset = 18, memcount;

	allbuf = RB_ReadPixels(x, y, width, height, &offset, &padlen);
	buffer = allbuf + offset - 18;

	Com_Memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24;	// pixel size

	// swap rgb to bgr and remove padding from line endings
	linelen = width * 3;

	srcptr = destptr = allbuf + offset;
	endmem = srcptr + (linelen + padlen) * height;

	while(srcptr < endmem) {
		endline = srcptr + linelen;

		while(srcptr < endline) {
			temp = srcptr[0];
			*destptr++ = srcptr[2];
			*destptr++ = srcptr[1];
			*destptr++ = temp;

			srcptr += 3;
		}

		// Skip the pad
		srcptr += padlen;
	}

	memcount = linelen * height;

	// gamma correct
	if ( glConfig.deviceSupportsGamma && !r_alternateBrightness->integer) {
		R_GammaCorrect(allbuf + offset, memcount);
	}

	ri.FS_WriteFile(fileName, buffer, memcount + 18);

	ri.Hunk_FreeTempMemory(allbuf);
}

/*
==================
RB_TakeScreenshotJPEG
==================
*/

void RB_TakeScreenshotJPEG(int x, int y, int width, int height, char *fileName)
{
	byte *buffer;
	size_t offset = 0, memcount;
	int padlen;

	buffer = RB_ReadPixels(x, y, width, height, &offset, &padlen);
	memcount = (width * 3 + padlen) * height;

	// gamma correct
	if(glConfig.deviceSupportsGamma)
		R_GammaCorrect(buffer + offset, memcount);

	RE_SaveJPG(fileName, r_screenshotJpegQuality->integer, width, height, buffer + offset, padlen);
	ri.Hunk_FreeTempMemory(buffer);
}

/*
==================
RB_TakeScreenshotCmd
==================
*/

const void *RB_TakeScreenshotCmd( const void *data )
{
	const screenshotCommand_t	*cmd;

	cmd = (const screenshotCommand_t *)data;

	if (cmd->jpeg)
		RB_TakeScreenshotJPEG( cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);
	else
		RB_TakeScreenshot( cmd->x, cmd->y, cmd->width, cmd->height, cmd->fileName);

	return (const void *)(cmd + 1);
}

/*
==================
R_TakeScreenshot
==================
*/
void R_TakeScreenshot( int x, int y, int width, int height, char *name, qboolean jpeg )
{
	static char	fileName[MAX_OSPATH]; // bad things if two screenshots per frame?
	screenshotCommand_t	*cmd;

	cmd = R_GetCommandBuffer( sizeof( *cmd ) );
	if ( !cmd ) {
		return;
	}
	cmd->commandId = RC_SCREENSHOT;

	cmd->x = x;
	cmd->y = y;
	cmd->width = width;
	cmd->height = height;
	Q_strncpyz( fileName, name, sizeof(fileName) );
	cmd->fileName = fileName;
	cmd->jpeg = jpeg;
}

/*
==================
R_ScreenshotFilename
==================
*/
void R_ScreenshotFilename( int lastNumber, char *fileName )
{
	int		a,b,c,d;

	if ( lastNumber < 0 || lastNumber > 9999 ) {
		Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot9999.tga" );
		return;
	}

	a = lastNumber / 1000;
	lastNumber -= a*1000;
	b = lastNumber / 100;
	lastNumber -= b*100;
	c = lastNumber / 10;
	lastNumber -= c*10;
	d = lastNumber;

	Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.tga"
	             , a, b, c, d );
}

/*
==================
R_ScreenshotFilename
==================
*/
void R_ScreenshotFilenameJPEG( int lastNumber, char *fileName )
{
	int		a,b,c,d;

	if ( lastNumber < 0 || lastNumber > 9999 ) {
		Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot9999.jpg" );
		return;
	}

	a = lastNumber / 1000;
	lastNumber -= a*1000;
	b = lastNumber / 100;
	lastNumber -= b*100;
	c = lastNumber / 10;
	lastNumber -= c*10;
	d = lastNumber;

	Com_sprintf( fileName, MAX_OSPATH, "screenshots/shot%i%i%i%i.jpg"
	             , a, b, c, d );
}

/*
====================
R_LevelShot

levelshots are specialized 128*128 thumbnails for
the menu system, sampled down from full screen distorted images
====================
*/
void R_LevelShot( void )
{
	char		checkname[MAX_OSPATH];
	byte		*buffer;
	byte		*source, *allsource;
	byte		*src, *dst;
	size_t			offset = 0;
	int			padlen;
	int			x, y;
	int			r, g, b;
	float		xScale, yScale;
	int			xx, yy;

	Com_sprintf(checkname, sizeof(checkname), "levelshots/%s.tga", tr.world->baseName);

	allsource = RB_ReadPixels(0, 0, glConfig.vidWidth, glConfig.vidHeight, &offset, &padlen);
	source = allsource + offset;

	buffer = ri.Hunk_AllocateTempMemory(128 * 128*3 + 18);
	Com_Memset (buffer, 0, 18);
	buffer[2] = 2;		// uncompressed type
	buffer[12] = 128;
	buffer[14] = 128;
	buffer[16] = 24;	// pixel size

	// resample from source
	xScale = glConfig.vidWidth / 512.0f;
	yScale = glConfig.vidHeight / 384.0f;
	for ( y = 0 ; y < 128 ; y++ ) {
		for ( x = 0 ; x < 128 ; x++ ) {
			r = g = b = 0;
			for ( yy = 0 ; yy < 3 ; yy++ ) {
				for ( xx = 0 ; xx < 4 ; xx++ ) {
					src = source + (3 * glConfig.vidWidth + padlen) * (int)((y*3 + yy) * yScale) +
					      3 * (int) ((x*4 + xx) * xScale);
					r += src[0];
					g += src[1];
					b += src[2];
				}
			}
			dst = buffer + 18 + 3 * ( y * 128 + x );
			dst[0] = b / 12;
			dst[1] = g / 12;
			dst[2] = r / 12;
		}
	}

	// gamma correct
	if ( glConfig.deviceSupportsGamma ) {
		R_GammaCorrect( buffer + 18, 128 * 128 * 3 );
	}

	ri.FS_WriteFile( checkname, buffer, 128 * 128*3 + 18 );

	ri.Hunk_FreeTempMemory(buffer);
	ri.Hunk_FreeTempMemory(allsource);

	ri.Printf( PRINT_ALL, "Wrote %s\n", checkname );
}

/*
==================
R_ScreenShot_f

screenshot
screenshot [silent]
screenshot [levelshot]
screenshot [filename]

Doesn't print the pacifier message if there is a second arg
==================
*/
void R_ScreenShot_f (void)
{
	char	checkname[MAX_OSPATH];
	static	int	lastNumber = -1;
	qboolean	silent;

	if ( !strcmp( ri.Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !strcmp( ri.Cmd_Argv(1), "silent" ) ) {
		silent = qtrue;
	}
	else {
		silent = qfalse;
	}

	if ( ri.Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_sprintf( checkname, MAX_OSPATH, "screenshots/%s.tga", ri.Cmd_Argv( 1 ) );
	}
	else {
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if ( lastNumber == -1 ) {
			lastNumber = 0;
		}
		// scan for a free number
		for ( ; lastNumber <= 9999 ; lastNumber++ ) {
			R_ScreenshotFilename( lastNumber, checkname );

			if (!ri.FS_FileExists( checkname )) {
				break; // file doesn't exist
			}
		}

		if ( lastNumber >= 9999 ) {
			ri.Printf (PRINT_ALL, "ScreenShot: Couldn't create a file\n");
			return;
		}

		lastNumber++;
	}

	R_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, qfalse );

	if ( !silent ) {
		ri.Printf (PRINT_ALL, "Wrote %s\n", checkname);
	}
}

void R_ScreenShotJPEG_f (void)
{
	char		checkname[MAX_OSPATH];
	static	int	lastNumber = -1;
	qboolean	silent;

	if ( !strcmp( ri.Cmd_Argv(1), "levelshot" ) ) {
		R_LevelShot();
		return;
	}

	if ( !strcmp( ri.Cmd_Argv(1), "silent" ) ) {
		silent = qtrue;
	}
	else {
		silent = qfalse;
	}

	if ( ri.Cmd_Argc() == 2 && !silent ) {
		// explicit filename
		Com_sprintf( checkname, MAX_OSPATH, "screenshots/%s.jpg", ri.Cmd_Argv( 1 ) );
	}
	else {
		// scan for a free filename

		// if we have saved a previous screenshot, don't scan
		// again, because recording demo avis can involve
		// thousands of shots
		if ( lastNumber == -1 ) {
			lastNumber = 0;
		}
		// scan for a free number
		for ( ; lastNumber <= 9999 ; lastNumber++ ) {
			R_ScreenshotFilenameJPEG( lastNumber, checkname );

			if (!ri.FS_FileExists( checkname )) {
				break; // file doesn't exist
			}
		}

		if ( lastNumber == 10000 ) {
			ri.Printf (PRINT_ALL, "ScreenShot: Couldn't create a file\n");
			return;
		}

		lastNumber++;
	}

	R_TakeScreenshot( 0, 0, glConfig.vidWidth, glConfig.vidHeight, checkname, qtrue );

	if ( !silent ) {
		ri.Printf (PRINT_ALL, "Wrote %s\n", checkname);
	}
}

//============================================================================

/*
==================
RB_TakeVideoFrameCmd
==================
*/
const void *RB_TakeVideoFrameCmd( const void *data )
{
	const videoFrameCommand_t	*cmd;
	byte				*cBuf;
	size_t				memcount, linelen;
	int				padwidth, avipadwidth, padlen, avipadlen;
	GLint packAlign;

	cmd = (const videoFrameCommand_t *)data;

	qglGetIntegerv(GL_PACK_ALIGNMENT, &packAlign);

	linelen = cmd->width * 3;

	// Alignment stuff for glReadPixels
	padwidth = PAD(linelen, packAlign);
	padlen = padwidth - linelen;
	// AVI line padding
	avipadwidth = PAD(linelen, AVI_LINE_PADDING);
	avipadlen = avipadwidth - linelen;

	cBuf = PADP(cmd->captureBuffer, packAlign);

	qglReadPixels(0, 0, cmd->width, cmd->height, GL_RGB,
	              GL_UNSIGNED_BYTE, cBuf);

	memcount = padwidth * cmd->height;

	// gamma correct
	if(glConfig.deviceSupportsGamma)
		R_GammaCorrect(cBuf, memcount);

	if(cmd->motionJpeg) {
		memcount = RE_SaveJPGToBuffer(cmd->encodeBuffer, linelen * cmd->height,
		                              r_aviMotionJpegQuality->integer,
		                              cmd->width, cmd->height, cBuf, padlen);
		ri.CL_WriteAVIVideoFrame(cmd->encodeBuffer, memcount);
	}
	else {
		byte *lineend, *memend;
		byte *srcptr, *destptr;

		srcptr = cBuf;
		destptr = cmd->encodeBuffer;
		memend = srcptr + memcount;

		// swap R and B and remove line paddings
		while(srcptr < memend) {
			lineend = srcptr + linelen;
			while(srcptr < lineend) {
				*destptr++ = srcptr[2];
				*destptr++ = srcptr[1];
				*destptr++ = srcptr[0];
				srcptr += 3;
			}

			Com_Memset(destptr, '\0', avipadlen);
			destptr += avipadlen;

			srcptr += padlen;
		}

		ri.CL_WriteAVIVideoFrame(cmd->encodeBuffer, avipadwidth * cmd->height);
	}

	return (const void *)(cmd + 1);
}

//============================================================================

/*
** GL_SetDefaultState
*/
void GL_SetDefaultState( void )
{
	qglClearDepth( 1.0f );

	qglCullFace(GL_FRONT);

	qglColor4f (1,1,1,1);

	// initialize downstream texture unit if we're running
	// in a multitexture environment
	if ( qglActiveTextureARB ) {
		GL_SelectTexture( 1 );
		GL_TextureMode( r_textureMode->string );
		GL_TexEnv( GL_MODULATE );
		qglDisable( GL_TEXTURE_2D );
		GL_SelectTexture( 0 );
	}

	qglEnable(GL_TEXTURE_2D);
	GL_TextureMode( r_textureMode->string );
	GL_TexEnv( GL_MODULATE );

	qglShadeModel( GL_SMOOTH );
	qglDepthFunc( GL_LEQUAL );

	// the vertex array is always enabled, but the color and texture
	// arrays are enabled and disabled around the compiled vertex array call
	qglEnableClientState (GL_VERTEX_ARRAY);

	//
	// make sure our GL state vector is set correctly
	//
	glState.glStateBits = GLS_DEPTHTEST_DISABLE | GLS_DEPTHMASK_TRUE;

	qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	qglDepthMask( GL_TRUE );
	qglDisable( GL_DEPTH_TEST );
	qglEnable( GL_SCISSOR_TEST );
	qglDisable( GL_CULL_FACE );
	qglDisable( GL_BLEND );
}

/*
================
R_PrintLongString

Workaround for ri.Printf's 1024 characters buffer limit.
================
*/
void R_PrintLongString(const char *string)
{
	char buffer[1024];
	const char *p;
	int size = strlen(string);

	p = string;
	while(size > 0) {
		Q_strncpyz(buffer, p, sizeof (buffer) );
		ri.Printf( PRINT_ALL, "%s", buffer );
		p += 1023;
		size -= 1023;
	}
}

/*
================
GfxInfo_f
================
*/
extern char	extensions_string_full [BIG_INFO_STRING]; // leilei - gl extensions crash workaround
void GfxInfo_f( void )
{
	const char *enablestrings[] = {
		"disabled",
		"enabled"
	};
	const char *fsstrings[] = {
		"windowed",
		"fullscreen"
	};

	ri.Printf( PRINT_ALL, "\nGL_VENDOR: %s\n", glConfig.vendor_string );
	ri.Printf( PRINT_ALL, "GL_RENDERER: %s\n", glConfig.renderer_string );
	ri.Printf( PRINT_ALL, "GL_VERSION: %s\n", glConfig.version_string );
	ri.Printf( PRINT_ALL, "GL_EXTENSIONS: " );
	R_PrintLongString( extensions_string_full );//R_PrintLongString( glConfig.extensions_string ); // leilei - gl extensions crash workaround
	ri.Printf( PRINT_ALL, "\n" );
	ri.Printf( PRINT_ALL, "GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize );
	ri.Printf( PRINT_ALL, "GL_MAX_TEXTURE_UNITS_ARB: %d\n", glConfig.numTextureUnits );
	ri.Printf( PRINT_ALL, "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
	ri.Printf( PRINT_ALL, "MODE: %d, %d x %d %s hz:", r_mode->integer, glConfig.vidWidth, glConfig.vidHeight, fsstrings[r_fullscreen->integer == 1] );
	if ( glConfig.displayFrequency ) {
		ri.Printf( PRINT_ALL, "%d\n", glConfig.displayFrequency );
	}
	else {
		ri.Printf( PRINT_ALL, "N/A\n" );
	}
	if ( glConfig.deviceSupportsGamma ) {
		ri.Printf( PRINT_ALL, "GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits );
	}
	else {
		ri.Printf( PRINT_ALL, "GAMMA: software w/ %d overbright bits\n", tr.overbrightBits );
	}

	// rendering primitives
	{
		int		primitives;

		// default is to use triangles if compiled vertex arrays are present
		ri.Printf( PRINT_ALL, "rendering primitives: " );
		primitives = r_primitives->integer;
		if ( primitives == 0 ) {
			if ( qglLockArraysEXT ) {
				primitives = 2;
			}
			else {
				primitives = 1;
			}
		}
		if ( primitives == -1 ) {
			ri.Printf( PRINT_ALL, "none\n" );
		}
		else if ( primitives == 2 ) {
			ri.Printf( PRINT_ALL, "single glDrawElements\n" );
		}
		else if ( primitives == 1 ) {
			ri.Printf( PRINT_ALL, "multiple glArrayElement\n" );
		}
		else if ( primitives == 3 ) {
			ri.Printf( PRINT_ALL, "multiple glColor4ubv + glTexCoord2fv + glVertex3fv\n" );
		}
	}

	ri.Printf( PRINT_ALL, "texturemode: %s\n", r_textureMode->string );
	ri.Printf( PRINT_ALL, "picmip: %d\n", r_picmip->integer );
	ri.Printf( PRINT_ALL, "texture bits: %d\n", r_texturebits->integer );
	ri.Printf( PRINT_ALL, "multitexture: %s\n", enablestrings[qglActiveTextureARB != 0] );
	ri.Printf( PRINT_ALL, "compiled vertex arrays: %s\n", enablestrings[qglLockArraysEXT != 0 ] );
	ri.Printf( PRINT_ALL, "texenv add: %s\n", enablestrings[glConfig.textureEnvAddAvailable != 0] );
	ri.Printf( PRINT_ALL, "compressed textures: %s\n", enablestrings[glConfig.textureCompression!=TC_NONE] );
	ri.Printf( PRINT_ALL, "glsl programs: %s\n", enablestrings[vertexShaders] );
	if ( r_vertexLight->integer || glConfig.hardwareType == GLHW_PERMEDIA2 ) {
		ri.Printf( PRINT_ALL, "HACK: using vertex lightmap approximation\n" );
	}
	if ( glConfig.hardwareType == GLHW_RAGEPRO ) {
		ri.Printf( PRINT_ALL, "HACK: ragePro approximations\n" );
	}
	if ( glConfig.hardwareType == GLHW_RIVA128 ) {
		ri.Printf( PRINT_ALL, "HACK: riva128 approximations\n" );
	}
	if ( r_finish->integer ) {
		ri.Printf( PRINT_ALL, "Forcing glFinish\n" );
	}
	if (voodootype) {

		if (voodootype == 1)
			ri.Printf( PRINT_ALL, "3Dfx Voodoo Graphics assumed\n" );
		else if (voodootype == 2)
			ri.Printf( PRINT_ALL, "3Dfx Voodoo2 assumed\n" );
		else if (voodootype == 3)
			ri.Printf( PRINT_ALL, "3dfx Voodoo3 assumed\n" );
		else if (voodootype == 4)
			ri.Printf( PRINT_ALL, "3dfx Voodoo 4 or 5 assumed\n" );

	}



}

/*
===============
R_Register
===============
*/
void R_Register( void )
{
#ifdef USE_RENDERER_DLOPEN
	com_altivec = ri.Cvar_Get("com_altivec", "1", CVAR_ARCHIVE);
#endif

#if defined( _WIN32 )
	// leilei -  Get some version info first, code torn from quake
	OSVERSIONINFO	vinfo;

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

#endif

	//
	// latched and archived variables
	//
	r_allowExtensions = ri.Cvar_Get( "r_allowExtensions", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_compressed_textures = ri.Cvar_Get( "r_ext_compressed_textures", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_multitexture = ri.Cvar_Get( "r_ext_multitexture", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_compiled_vertex_array = ri.Cvar_Get( "r_ext_compiled_vertex_array", "1", CVAR_ARCHIVE | CVAR_LATCH);
	r_ext_texture_env_add = ri.Cvar_Get( "r_ext_texture_env_add", "1", CVAR_ARCHIVE | CVAR_LATCH);

	r_ext_texture_filter_anisotropic = ri.Cvar_Get( "r_ext_texture_filter_anisotropic",
	                                   "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_max_anisotropy = ri.Cvar_Get( "r_ext_max_anisotropy", "2", CVAR_ARCHIVE | CVAR_LATCH );
	r_loadDDS = ri.Cvar_Get( "r_loadDDS", "1", CVAR_ARCHIVE | CVAR_LATCH );	// leilei - compressed texture support
	r_ext_paletted_texture = ri.Cvar_Get( "r_ext_paletted_texture", "0", CVAR_ARCHIVE | CVAR_LATCH );	// leilei - paletted texture support
	r_ext_gamma_control = ri.Cvar_Get( "r_ext_gamma_control", "1", CVAR_ARCHIVE | CVAR_LATCH );	// leilei - 3dfx gamma support

	r_postprocess = ri.Cvar_Get( "r_postprocess", "none", CVAR_ARCHIVE|CVAR_LATCH );
	r_ext_vertex_shader = ri.Cvar_Get( "r_ext_vertex_shader", "0", CVAR_ARCHIVE|CVAR_LATCH );
	r_picmip = ri.Cvar_Get ("r_picmip", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_roundImagesDown = ri.Cvar_Get ("r_roundImagesDown", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_colorMipLevels = ri.Cvar_Get ("r_colorMipLevels", "0", CVAR_LATCH );
	ri.Cvar_CheckRange( r_picmip, 0, 16, qtrue );
	r_detailTextures = ri.Cvar_Get( "r_detailtextures", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_texturebits = ri.Cvar_Get( "r_texturebits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_colorbits = ri.Cvar_Get( "r_colorbits", "0", CVAR_ARCHIVE | CVAR_LATCH );	// use desktop
	r_stencilbits = ri.Cvar_Get( "r_stencilbits", "8", CVAR_ARCHIVE | CVAR_LATCH );
	r_depthbits = ri.Cvar_Get( "r_depthbits", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_ext_multisample = ri.Cvar_Get( "r_ext_multisample", "0", CVAR_ARCHIVE | CVAR_LATCH );
	ri.Cvar_CheckRange( r_ext_multisample, 0, 4, qtrue );
	r_overBrightBits = ri.Cvar_Get ("r_overBrightBits", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_ignorehwgamma = ri.Cvar_Get( "r_ignorehwgamma", "0", CVAR_ARCHIVE | CVAR_LATCH);

#if defined( _WIN32 )

	if (vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT) // Win2000 and up
		r_mode = ri.Cvar_Get( "r_mode", "-2", CVAR_ARCHIVE | CVAR_LATCH ); // leilei - -2 is so convenient for modern day PCs
	else			       // WinNT4/WinME and below
		r_mode = ri.Cvar_Get( "r_mode", "3", CVAR_ARCHIVE | CVAR_LATCH ); // leilei - initialize 640x480 for <2000 users as there's a problem at figuring out desktop res on them

#else

	r_mode = ri.Cvar_Get( "r_mode", "-2", CVAR_ARCHIVE | CVAR_LATCH ); // leilei - -2 is so convenient for modern day PCs
#endif

	r_fullscreen = ri.Cvar_Get( "r_fullscreen", "1", CVAR_ARCHIVE );
	r_noborder = ri.Cvar_Get("r_noborder", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_customwidth = ri.Cvar_Get( "r_customwidth", "1600", CVAR_ARCHIVE | CVAR_LATCH );
	r_customheight = ri.Cvar_Get( "r_customheight", "1024", CVAR_ARCHIVE | CVAR_LATCH );
	r_customPixelAspect = ri.Cvar_Get( "r_customPixelAspect", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_simpleMipMaps = ri.Cvar_Get( "r_simpleMipMaps", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_vertexLight = ri.Cvar_Get( "r_vertexLight", "0", CVAR_ARCHIVE | CVAR_LATCH );
	r_uiFullScreen = ri.Cvar_Get( "r_uifullscreen", "0", 0);
	r_subdivisions = ri.Cvar_Get ("r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH);
	r_stereoEnabled = ri.Cvar_Get( "r_stereoEnabled", "0", CVAR_ARCHIVE | CVAR_LATCH);
	r_ignoreFastPath = ri.Cvar_Get( "r_ignoreFastPath", "1", CVAR_ARCHIVE | CVAR_LATCH );
	r_monolightmaps = ri.Cvar_Get("r_monolightmaps", "0", CVAR_ARCHIVE | CVAR_LATCH);
	ri.Cvar_CheckRange(r_monolightmaps, 0, 1, qfalse);


	//
	// temporary latched variables that can only change over a restart
	//
	r_fullbright = ri.Cvar_Get ("r_fullbright", "0", CVAR_LATCH|CVAR_CHEAT );
	r_mapOverBrightBits = ri.Cvar_Get ("r_mapOverBrightBits", "2", CVAR_LATCH );
	r_intensity = ri.Cvar_Get ("r_intensity", "1", CVAR_LATCH );
	r_singleShader = ri.Cvar_Get ("r_singleShader", "0", CVAR_CHEAT | CVAR_LATCH );

	//
	// archived variables that can change at any time
	//
	r_lodCurveError = ri.Cvar_Get( "r_lodCurveError", "250", CVAR_ARCHIVE|CVAR_CHEAT );
	r_lodbias = ri.Cvar_Get( "r_lodbias", "0", CVAR_ARCHIVE );
	r_flares = ri.Cvar_Get ("r_flares", "0", CVAR_ARCHIVE );
	r_znear = ri.Cvar_Get( "r_znear", "4", CVAR_CHEAT );
	ri.Cvar_CheckRange( r_znear, 0.001f, 200, qfalse );
	r_zproj = ri.Cvar_Get( "r_zproj", "64", CVAR_ARCHIVE );
	r_stereoSeparation = ri.Cvar_Get( "r_stereoSeparation", "64", CVAR_ARCHIVE );
	r_ignoreGLErrors = ri.Cvar_Get( "r_ignoreGLErrors", "1", CVAR_ARCHIVE );
	r_fastsky = ri.Cvar_Get( "r_fastsky", "0", CVAR_ARCHIVE );
	r_inGameVideo = ri.Cvar_Get( "r_inGameVideo", "1", CVAR_ARCHIVE );
	r_drawSun = ri.Cvar_Get( "r_drawSun", "1", CVAR_ARCHIVE ); // leilei - default 1 for allowing sky shaders to define a sun.
	r_dynamiclight = ri.Cvar_Get( "r_dynamiclight", "1", CVAR_ARCHIVE );
	r_dlightBacks = ri.Cvar_Get( "r_dlightBacks", "1", CVAR_ARCHIVE );
	r_finish = ri.Cvar_Get ("r_finish", "0", CVAR_ARCHIVE);
	r_textureMode = ri.Cvar_Get( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST", CVAR_ARCHIVE );
	r_swapInterval = ri.Cvar_Get( "r_swapInterval", "0",
	                              CVAR_ARCHIVE | CVAR_LATCH );
	r_gamma = ri.Cvar_Get( "r_gamma", "1", CVAR_ARCHIVE );
	r_facePlaneCull = ri.Cvar_Get ("r_facePlaneCull", "1", CVAR_ARCHIVE );

	r_railWidth = ri.Cvar_Get( "r_railWidth", "16", CVAR_ARCHIVE );
	r_railCoreWidth = ri.Cvar_Get( "r_railCoreWidth", "6", CVAR_ARCHIVE );
	r_railSegmentLength = ri.Cvar_Get( "r_railSegmentLength", "32", CVAR_ARCHIVE );

	r_primitives = ri.Cvar_Get( "r_primitives", "0", CVAR_ARCHIVE );

	r_ambientScale = ri.Cvar_Get( "r_ambientScale", "0.6", CVAR_CHEAT );
	r_directedScale = ri.Cvar_Get( "r_directedScale", "1", CVAR_CHEAT );

	r_anaglyphMode = ri.Cvar_Get("r_anaglyphMode", "0", CVAR_ARCHIVE);

	//
	// temporary variables that can change at any time
	//
	r_showImages = ri.Cvar_Get( "r_showImages", "0", CVAR_TEMP );

	r_debugLight = ri.Cvar_Get( "r_debuglight", "0", CVAR_TEMP );
	r_debugSort = ri.Cvar_Get( "r_debugSort", "0", CVAR_CHEAT );
	r_printShaders = ri.Cvar_Get( "r_printShaders", "0", 0 );
	r_saveFontData = ri.Cvar_Get( "r_saveFontData", "0", 0 );

	r_nocurves = ri.Cvar_Get ("r_nocurves", "0", CVAR_CHEAT );
	r_drawworld = ri.Cvar_Get ("r_drawworld", "1", CVAR_CHEAT );
	r_lightmap = ri.Cvar_Get ("r_lightmap", "0", 0 );

	r_portalOnly = ri.Cvar_Get ("r_portalOnly", "0", CVAR_CHEAT );


	r_flareSize = ri.Cvar_Get ("r_flareSize", "40", CVAR_CHEAT);
	r_flareFade = ri.Cvar_Get ("r_flareFade", "7", CVAR_CHEAT);
	r_flareCoeff = ri.Cvar_Get ("r_flareCoeff", FLARE_STDCOEFF, CVAR_CHEAT);

	r_skipBackEnd = ri.Cvar_Get ("r_skipBackEnd", "0", CVAR_CHEAT);

	r_measureOverdraw = ri.Cvar_Get( "r_measureOverdraw", "0", CVAR_CHEAT );
	r_lodscale = ri.Cvar_Get( "r_lodscale", "5", CVAR_CHEAT );
	r_norefresh = ri.Cvar_Get ("r_norefresh", "0", CVAR_CHEAT);
	r_drawentities = ri.Cvar_Get ("r_drawentities", "1", CVAR_CHEAT );
	r_ignore = ri.Cvar_Get( "r_ignore", "1", CVAR_CHEAT );
	r_nocull = ri.Cvar_Get ("r_nocull", "0", CVAR_CHEAT);
	r_novis = ri.Cvar_Get ("r_novis", "0", CVAR_CHEAT);
	r_showcluster = ri.Cvar_Get ("r_showcluster", "0", CVAR_CHEAT);
	r_speeds = ri.Cvar_Get ("r_speeds", "0", CVAR_CHEAT);
	r_verbose = ri.Cvar_Get( "r_verbose", "0", CVAR_CHEAT );
	r_logFile = ri.Cvar_Get( "r_logFile", "0", CVAR_CHEAT );
	r_debugSurface = ri.Cvar_Get ("r_debugSurface", "0", CVAR_CHEAT);
	r_nobind = ri.Cvar_Get ("r_nobind", "0", CVAR_CHEAT);
	r_showtris = ri.Cvar_Get ("r_showtris", "0", CVAR_CHEAT);
	r_showsky = ri.Cvar_Get ("r_showsky", "0", CVAR_CHEAT);
	r_shownormals = ri.Cvar_Get ("r_shownormals", "0", CVAR_CHEAT);
	r_clear = ri.Cvar_Get ("r_clear", "0", CVAR_CHEAT);
	r_offsetFactor = ri.Cvar_Get( "r_offsetfactor", "-1", CVAR_CHEAT );
	r_offsetUnits = ri.Cvar_Get( "r_offsetunits", "-2", CVAR_CHEAT );
	r_drawBuffer = ri.Cvar_Get( "r_drawBuffer", "GL_BACK", CVAR_CHEAT );
	r_lockpvs = ri.Cvar_Get ("r_lockpvs", "0", CVAR_CHEAT);
	r_noportals = ri.Cvar_Get ("r_noportals", "0", CVAR_CHEAT);
	r_shadows = ri.Cvar_Get( "cg_shadows", "1", 0 );

	r_marksOnTriangleMeshes = ri.Cvar_Get("r_marksOnTriangleMeshes", "0", CVAR_ARCHIVE);

	r_aviMotionJpegQuality = ri.Cvar_Get("r_aviMotionJpegQuality", "90", CVAR_ARCHIVE);
	r_screenshotJpegQuality = ri.Cvar_Get("r_screenshotJpegQuality", "90", CVAR_ARCHIVE);

	r_maxpolys = ri.Cvar_Get( "r_maxpolys", va("%d", MAX_POLYS), 0);
	r_maxpolyverts = ri.Cvar_Get( "r_maxpolyverts", va("%d", MAX_POLYVERTS), 0);



	r_specMode = ri.Cvar_Get( "r_specMode", "1" , CVAR_ARCHIVE );
	//r_waveMode = ri.Cvar_Get( "r_waveMode", "0" , CVAR_ARCHIVE );

	r_lensReflection1 = ri.Cvar_Get( "r_lensReflection1", "1" , CVAR_ARCHIVE);	// sharp reflection
	r_lensReflection2 = ri.Cvar_Get( "r_lensReflection2", "0" , CVAR_ARCHIVE); // fuzzy reflection
	r_lensReflectionBrightness = ri.Cvar_Get( "r_lensReflectionBrightness", "0.5" , CVAR_ARCHIVE);

	r_flareQuality = ri.Cvar_Get( "r_flareQuality", "4" , CVAR_ARCHIVE);	// use high flares for default (lower settings stutter vsync or clip too badly)
	r_flareMethod = ri.Cvar_Get( "r_flareMethod", "0" , CVAR_ARCHIVE);
	r_flaresDlight = ri.Cvar_Get( "r_flaresDlight", "0" , CVAR_ARCHIVE );	// dynamic light flares
	r_flaresDlightShrink = ri.Cvar_Get( "r_flaresDlightShrink", "1" , CVAR_ARCHIVE );	// dynamic light flares shrinking when close (reducing muzzleflash blindness)
	r_flaresDlightFade = ri.Cvar_Get( "r_flaresDlightFade", "0" , CVAR_ARCHIVE | CVAR_CHEAT );	// dynamic light flares fading (workaround clipping bug)
	r_flaresDlightOpacity = ri.Cvar_Get( "r_flaresDlightOpacity", "0.5" , CVAR_ARCHIVE );	// dynamic light flares (workaround poor visibility)
	r_flaresDlightScale = ri.Cvar_Get( "r_flaresDlightScale", "0.7" , CVAR_ARCHIVE );	// dynamic light flares (workaround poor visibility)
	r_flareSun = ri.Cvar_Get( "r_flareSun", "0" , CVAR_ARCHIVE);	// it's 0 because mappers expect 0.
	r_flareDelay = ri.Cvar_Get( "r_flareDelay", "100" , CVAR_CHEAT);	// update delay for flare pixel read checking.


	r_mockvr = ri.Cvar_Get( "r_mockvr", "0" , CVAR_CHEAT);
	r_leifx = ri.Cvar_Get( "r_leifx", "0" , CVAR_ARCHIVE | CVAR_LATCH);
	r_shadeMethod = ri.Cvar_Get( "r_shadeMethod", "0" , CVAR_ARCHIVE);		// leilei - Alternative lightingDiffuse

	r_skytess = ri.Cvar_Get( "r_skyTess", "8", CVAR_ARCHIVE );			// leilei - sky detail adjustment

	r_detailTextureScale = ri.Cvar_Get( "r_detailtextureScale", "0", CVAR_ARCHIVE | CVAR_LATCH ); // leilei - adjust scale of detail textures
	r_detailTextureLayers = ri.Cvar_Get( "r_detailtextureLayers", "0", CVAR_ARCHIVE | CVAR_LATCH ); // leilei - add more detail layers

	r_suggestiveThemes = ri.Cvar_Get( "r_suggestiveThemes", "1" , CVAR_ARCHIVE | CVAR_LATCH);

	r_leidebug = ri.Cvar_Get( "r_leidebug", "0" , CVAR_CHEAT);
	r_particles = ri.Cvar_Get( "r_particles", "0" , CVAR_ARCHIVE | CVAR_LATCH);
#ifdef BROKEN_MDRPHYS
	r_mdrPhysics = ri.Cvar_Get( "r_mdrPhysics", "0" , CVAR_ARCHIVE);
#endif
	r_leidebugeye = ri.Cvar_Get( "r_leidebugeye", "0" , CVAR_CHEAT);

	r_lerpbias = ri.Cvar_Get( "r_lerpbias", "-2" , CVAR_ARCHIVE);

	r_iconmip = ri.Cvar_Get ("r_iconmip", "0", CVAR_ARCHIVE | CVAR_LATCH );		// leilei - icon mip
	r_iconBits = ri.Cvar_Get ("r_iconBits", "0", CVAR_ARCHIVE | CVAR_LATCH );	// leilei - icon bits

	r_lightmapBits = ri.Cvar_Get ("r_lightmapBits", "0", CVAR_ARCHIVE | CVAR_LATCH );	// leilei - lightmap color bits
	r_lightmapColorNorm = ri.Cvar_Get ("r_lightmapColorNorm", "1", CVAR_ARCHIVE | CVAR_LATCH ); // leilei - lightmap color normalization

	r_textureDither = ri.Cvar_Get ("r_textureDither", "0", CVAR_ARCHIVE | CVAR_LATCH );	// leilei - dithered textures

	// make sure all the commands added here are also
	// removed in R_Shutdown
	ri.Cmd_AddCommand( "imagelist", R_ImageList_f );
	ri.Cmd_AddCommand( "shaderlist", R_ShaderList_f );
	ri.Cmd_AddCommand( "skinlist", R_SkinList_f );
	ri.Cmd_AddCommand( "modellist", R_Modellist_f );
	ri.Cmd_AddCommand( "modelist", R_ModeList_f );
	ri.Cmd_AddCommand( "imagelistmaponly", R_ImageListMapOnly_f );
	ri.Cmd_AddCommand( "screenshot", R_ScreenShot_f );
	ri.Cmd_AddCommand( "screenshotJPEG", R_ScreenShotJPEG_f );
	ri.Cmd_AddCommand( "gfxinfo", GfxInfo_f );
	ri.Cmd_AddCommand( "minimize", GLimp_Minimize );
	ri.Cmd_AddCommand( "paletteglsldump", R_GLSLPalette_f );
}


/*
 * R_GLSL_AllocProgram
 * Reserve memory for program
 */
static glslProgram_t *R_GLSL_AllocProgram(void)
{
	glslProgram_t	*program;

	if (tr.numPrograms == MAX_PROGRAMS)
		return NULL;

	program = ri.Hunk_Alloc(sizeof(*tr.programs[tr.numPrograms]), h_low);
	Com_Memset(program, 0, sizeof(*program));

	program->index							= tr.numPrograms;
	program->u_AlphaGen						= -1;
	program->u_AmbientLight					= -1;
	program->u_DynamicLight					= -1;
	program->u_LightDistance				= -1;
	program->u_ColorGen						= -1;
	program->u_ConstantColor				= -1;
	program->u_DirectedLight				= -1;
	program->u_EntityColor					= -1;
	program->u_Greyscale					= -1;
	program->u_FogColor						= -1;
	program->u_IdentityLight				= -1;
	program->u_LightDirection				= -1;
	program->u_ModelViewMatrix				= -1;
	program->u_ModelViewProjectionMatrix	= -1;
	program->u_ProjectionMatrix				= -1;
	program->u_TCGen0						= -1;
	program->u_TCGen1						= -1;
	program->u_TexEnv						= -1;
	program->u_Texture0						= -1;
	program->u_Texture1						= -1;
	program->u_Texture2						= -1;
	program->u_Texture3						= -1;
	program->u_Texture4						= -1;
	program->u_Texture5						= -1;
	program->u_Texture6						= -1;
	program->u_Texture7						= -1;
	program->u_Time							= -1;
	program->u_ViewOrigin					= -1;
	program->u_Normal					= -1;

	program->rubyTextureSize				= -1;
	program->rubyInputSize					= -1;
	program->rubyOutputSize					= -1;
	program->rubyFrameCount					= -1;

	tr.programs[tr.numPrograms] = program;
	tr.numPrograms++;

	return program;
}
/*
 * R_GLSL_Init
 * Load all default GLSL programs which are not loaded via the q3 shader system
 */

void R_GLSL_Init(void)
{
#ifdef GLSL_BACKEND
	glslProgram_t	*program;
	char			programVertexObjects[MAX_PROGRAM_OBJECTS][MAX_QPATH];
	char			programFragmentObjects[MAX_PROGRAM_OBJECTS][MAX_QPATH];
	char p[128];

	postprocess=qfalse;
	depthimage=0;
	if (!vertexShaders)
		return;

	ri.Printf(PRINT_ALL, "Initializing Programs\n");

	tr.numPrograms = 0;

	/* leave a space for NULL program */
	program = R_GLSL_AllocProgram();
	program->valid = qfalse;

	/* skip program */
	program = R_GLSL_AllocProgram();
	Q_strncpyz(program->name, "skip", sizeof(program->name));
	program->valid = qtrue;

	/* load default programs */
	tr.skipProgram = RE_GLSL_RegisterProgram("skip", (const char *)NULL, 0, (const char *)NULL, 0);
	//tr.defaultProgram = 0;
	Q_strncpyz(programVertexObjects[0], "glsl/generic_vp.glsl", sizeof(programVertexObjects[0]));
	Q_strncpyz(programFragmentObjects[0], "glsl/generic_fp.glsl", sizeof(programFragmentObjects[0]));
	Q_strncpyz(programFragmentObjects[1], "glsl/texturing.glsl", sizeof(programFragmentObjects[1]));
	tr.defaultProgram = RE_GLSL_RegisterProgram("generic", (const char *)programVertexObjects, 1, (const char *)programFragmentObjects, 2);

	Q_strncpyz(programVertexObjects[0], "glsl/vertexLitTexture_vp.glsl", sizeof(programVertexObjects[0]));
	Q_strncpyz(programFragmentObjects[0], "glsl/vertexLitTexture_fp.glsl", sizeof(programFragmentObjects[0]));
	tr.vertexLitProgram = RE_GLSL_RegisterProgram("vertexLitTexture", (const char *)programVertexObjects, 1, (const char *)programFragmentObjects, 1);

	Q_strncpyz(programVertexObjects[0], "glsl/lightmappedMultitexture_vp.glsl", sizeof(programVertexObjects[0]));
	Q_strncpyz(programFragmentObjects[0], "glsl/lightmappedMultitexture_fp.glsl", sizeof(programFragmentObjects[0]));
	tr.lightmappedMultitextureProgram = RE_GLSL_RegisterProgram("lightmappedMultitexture", (const char *)programVertexObjects, 1, (const char *)programFragmentObjects, 1);

	Q_strncpyz(programVertexObjects[0], "glsl/sky_vp.glsl", sizeof(programVertexObjects[0]));
	Q_strncpyz(programFragmentObjects[0], "glsl/sky_fp.glsl", sizeof(programFragmentObjects[0]));
	tr.skyProgram = RE_GLSL_RegisterProgram("sky", (const char *)programVertexObjects, 1, (const char *)programFragmentObjects, 1);

	Q_strncpyz(programVertexObjects[0], "glsl/leifx_dither_vp.glsl", sizeof(programVertexObjects[0]));
	Q_strncpyz(programFragmentObjects[0], "glsl/leifx_dither_fp.glsl", sizeof(programFragmentObjects[0]));
	tr.leiFXDitherProgram = RE_GLSL_RegisterProgram("leifx_dither", (const char *)programVertexObjects, 1, (const char *)programFragmentObjects, 1);
#ifdef USE_FALLBACK_GLSL
//	if (!tr.leiFXDitherProgram) // try fallback shader
//	tr.leiFXDitherProgram = RE_GLSL_RegisterProgram("leifx_dither", fallbackShader_leifx_dither_vp, 1, fallbackShader_leifx_dither_fp, 1);
#endif
	Q_strncpyz(programVertexObjects[0], "glsl/leifx_gamma_vp.glsl", sizeof(programVertexObjects[0]));
	Q_strncpyz(programFragmentObjects[0], "glsl/leifx_gamma_fp.glsl", sizeof(programFragmentObjects[0]));
	tr.leiFXGammaProgram = RE_GLSL_RegisterProgram("leifx_gamma", (const char *)programVertexObjects, 1, (const char *)programFragmentObjects, 1);

	Q_strncpyz(programVertexObjects[0], "glsl/leifx_filter_vp.glsl", sizeof(programVertexObjects[0]));
	Q_strncpyz(programFragmentObjects[0], "glsl/leifx_filter_fp.glsl", sizeof(programFragmentObjects[0]));
	tr.leiFXFilterProgram = RE_GLSL_RegisterProgram("leifx_filter", (const char *)programVertexObjects, 1, (const char *)programFragmentObjects, 1);

	Q_strncpyz(programVertexObjects[0], "glsl/brightness_vp.glsl", sizeof(programVertexObjects[0]));
	Q_strncpyz(programFragmentObjects[0], "glsl/brightness_fp.glsl", sizeof(programFragmentObjects[0]));
	if (!(tr.BrightnessProgram = RE_GLSL_RegisterProgram("brightness", (const char *)programVertexObjects, 1, (const char *)programFragmentObjects, 1)))
	{
		// load it through this code
	ri.Printf( PRINT_ALL, "----- Trying to load Brightness Shader internally!!! \n" );

		const GLchar *brightnessfp = \	
			"#version 120 \n"
			"uniform sampler2D u_Texture0; \n"
			"varying vec2 texture_coordinate;\n"
			"uniform float u_CC_Overbright; \n"
			"uniform float u_CC_Gamma; \n"
			"void main()\n"
			"{\n"
    			"gl_FragColor = texture2D(u_Texture0, texture_coordinate); \n"
			"vec3 color;\n"
			"vec3 colord;\n"
			"int coloredr;\n"
			"int coloredg;\n"
			"int coloredb;\n"
			"color.r = 1;\n"
			"color.g = 1;\n"
			"color.b = 1;\n"
			"int yeh = 0;\n"
			"float ohyes;\n"
			"// Overbrights\n"
    			"gl_FragColor *= (u_CC_Overbright + 1);\n"
			"// Gamma Correction\n"
			"float gamma = u_CC_Gamma;\n"
			"gl_FragColor.r = pow(gl_FragColor.r, 1.0 / gamma);\n"
			"gl_FragColor.g = pow(gl_FragColor.g, 1.0 / gamma);\n"
			"gl_FragColor.b = pow(gl_FragColor.b, 1.0 / gamma);\n"
			"}\n";

		const GLchar *brightnessvp = \
			"varying vec2 texture_coordinate;\n"
			"varying vec2 texture_coordinate2;\n"
			"varying vec2 texture_coordinate3;\n"
			"varying vec2 texture_coordinate4;\n"
			"varying vec2 texture_coordinate5;\n"
			"varying float scale;\n"
			"uniform float u_CC_Overbright;\n"
			"uniform float u_CC_Gamma;\n"
			"void main()\n"
			"{\n"
	    		"gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; \n"
	    		"texture_coordinate = vec2(gl_MultiTexCoord0); \n"
			"}\n";

			tr.BrightnessProgram = RE_GLSL_RegisterProgramRaw("brightnessInternal", (const char *)brightnessvp, 1, (const char *)brightnessfp, 1);
	}

	if (strcmp( (const char *)r_postprocess->string, "none" )) {
		sprintf(p,"glsl/%s_vp.glsl",r_postprocess->string);
		Q_strncpyz(programVertexObjects[0], (const char *)&p, sizeof(programVertexObjects[0]));
		sprintf(p,"glsl/%s_fp.glsl",r_postprocess->string);
		Q_strncpyz(programFragmentObjects[0], (const char *)&p, sizeof(programFragmentObjects[0]));
		if (ri.FS_FileIsInPAK (programVertexObjects[0],NULL)==1 && ri.FS_FileIsInPAK (programFragmentObjects[0],NULL)==1)
			tr.postprocessingProgram = RE_GLSL_RegisterProgram("postprocess", (const char *)programVertexObjects, 1, (const char *)programFragmentObjects, 1);
		else
			ri.Printf( PRINT_ALL, "WARNING: Cannot locate postprocessing glsl program %s\n" ,r_postprocess->string);
		if (tr.postprocessingProgram) postprocess=qtrue;
	}
#endif
}
extern qboolean		initparticles;

/*
===============
R_Init
===============
*/
void R_Init( void )
{
	int	err;
	int i;
	byte *ptr;

	ri.Printf( PRINT_ALL, "----- R_Init -----\n" );

	// clear all our internal state
	Com_Memset( &tr, 0, sizeof( tr ) );
	Com_Memset( &backEnd, 0, sizeof( backEnd ) );
	Com_Memset( &tess, 0, sizeof( tess ) );

	if(sizeof(glconfig_t) != 11332)
		ri.Error( ERR_FATAL, "Mod ABI incompatible: sizeof(glconfig_t) == %u != 11332", (unsigned int) sizeof(glconfig_t));

//	Swap_Init();

	if ( (intptr_t)tess.xyz & 15 ) {
		ri.Printf( PRINT_WARNING, "tess.xyz not 16 byte aligned\n" );
	}
	Com_Memset( tess.constantColor255, 255, sizeof( tess.constantColor255 ) );

	//
	// init function tables
	//
	for ( i = 0; i < FUNCTABLE_SIZE; i++ ) {
		tr.sinTable[i]		= sin( DEG2RAD( i * 360.0f / ( ( float ) ( FUNCTABLE_SIZE - 1 ) ) ) );
		tr.squareTable[i]	= ( i < FUNCTABLE_SIZE/2 ) ? 1.0f : -1.0f;
		tr.sawToothTable[i] = (float)i / FUNCTABLE_SIZE;
		tr.inverseSawToothTable[i] = 1.0f - tr.sawToothTable[i];

		if ( i < FUNCTABLE_SIZE / 2 ) {
			if ( i < FUNCTABLE_SIZE / 4 ) {
				tr.triangleTable[i] = ( float ) i / ( FUNCTABLE_SIZE / 4 );
			}
			else {
				tr.triangleTable[i] = 1.0f - tr.triangleTable[i-FUNCTABLE_SIZE / 4];
			}
		}
		else {
			tr.triangleTable[i] = -tr.triangleTable[i-FUNCTABLE_SIZE/2];
		}
	}

	R_InitFogTable();

	R_NoiseInit();

	R_Register();

	R_BloomInit();
	R_PostprocessingInit();
	R_AltBrightnessInit();	// leilei	- alternate brightness
	max_polys = r_maxpolys->integer;
	if (max_polys < MAX_POLYS)
		max_polys = MAX_POLYS;

	max_polyverts = r_maxpolyverts->integer;
	if (max_polyverts < MAX_POLYVERTS)
		max_polyverts = MAX_POLYVERTS;

	ptr = ri.Hunk_Alloc( sizeof( *backEndData ) + sizeof(srfPoly_t) * max_polys + sizeof(polyVert_t) * max_polyverts, h_low);
	backEndData = (backEndData_t *) ptr;
	backEndData->polys = (srfPoly_t *) ((char *) ptr + sizeof( *backEndData ));
	backEndData->polyVerts = (polyVert_t *) ((char *) ptr + sizeof( *backEndData ) + sizeof(srfPoly_t) * max_polys);
	R_InitNextFrame();

	InitOpenGL();

	R_InitImages();
#ifdef GLSL_BACKEND
	R_GLSL_Init();
#endif
	R_InitShaders();

	R_InitSkins();

	R_ModelInit();

	R_InitFreeType();


	err = qglGetError();
	if ( err != GL_NO_ERROR )
		ri.Printf (PRINT_ALL, "glGetError() = 0x%x\n", err);

	// print info
	GfxInfo_f();
	ri.Printf( PRINT_ALL, "----- finished R_Init -----\n" );
}

/*
===============
RE_Shutdown
===============
*/
void RE_Shutdown( qboolean destroyWindow )
{

	ri.Printf( PRINT_ALL, "RE_Shutdown( %i )\n", destroyWindow );

	ri.Cmd_RemoveCommand ("modellist");
	ri.Cmd_RemoveCommand ("screenshotJPEG");
	ri.Cmd_RemoveCommand ("screenshot");
	ri.Cmd_RemoveCommand ("imagelist");
	ri.Cmd_RemoveCommand ("imagelistmaponly");
	ri.Cmd_RemoveCommand ("shaderlist");
	ri.Cmd_RemoveCommand ("skinlist");
	ri.Cmd_RemoveCommand ("gfxinfo");
	ri.Cmd_RemoveCommand("minimize");
	ri.Cmd_RemoveCommand( "modelist" );
	ri.Cmd_RemoveCommand( "shaderstate" );


	if ( tr.registered ) {
		R_IssuePendingRenderCommands();
		R_DeleteTextures();
	}

	R_DoneFreeType();

	// shut down platform specific OpenGL stuff
	if ( destroyWindow ) {
		GLimp_Shutdown();

		Com_Memset( &glConfig, 0, sizeof( glConfig ) );
		Com_Memset( &glState, 0, sizeof( glState ) );
	}

	tr.registered = qfalse;
}


/*
=============
RE_EndRegistration

Touch all images to make sure they are resident
=============
*/
void RE_EndRegistration( void )
{
	R_IssuePendingRenderCommands();
	if (!ri.Sys_LowPhysicalMemory()) {
		RB_ShowImages();
	}
}


/*
@@@@@@@@@@@@@@@@@@@@@
GetRefAPI

@@@@@@@@@@@@@@@@@@@@@
*/
#ifdef USE_RENDERER_DLOPEN
Q_EXPORT refexport_t* QDECL GetRefAPI ( int apiVersion, refimport_t *rimp )
{
#else
refexport_t *GetRefAPI ( int apiVersion, refimport_t *rimp )
{
#endif

	static refexport_t	re;

	ri = *rimp;

	Com_Memset( &re, 0, sizeof( re ) );

	if ( apiVersion != REF_API_VERSION ) {
		ri.Printf(PRINT_ALL, "Mismatched REF_API_VERSION: expected %i, got %i\n",
		          REF_API_VERSION, apiVersion );
		return NULL;
	}

	// the RE_ functions are Renderer Entry points

	re.Shutdown = RE_Shutdown;

	re.BeginRegistration = RE_BeginRegistration;
	re.RegisterModel = RE_RegisterModel;
	re.RegisterSkin = RE_RegisterSkin;
	re.RegisterShader = RE_RegisterShader;
	re.RegisterShaderNoMip = RE_RegisterShaderNoMip;
	re.LoadWorld = RE_LoadWorldMap;
	re.SetWorldVisData = RE_SetWorldVisData;
	re.EndRegistration = RE_EndRegistration;

	re.BeginFrame = RE_BeginFrame;
	re.EndFrame = RE_EndFrame;

	re.MarkFragments = R_MarkFragments;
	re.LerpTag = R_LerpTag;
	re.ModelBounds = R_ModelBounds;

	re.ClearScene = RE_ClearScene;
	re.AddRefEntityToScene = RE_AddRefEntityToScene;
	re.AddPolyToScene = RE_AddPolyToScene;
	re.LFX_ParticleEffect = LFX_ParticleEffect;
	re.GetViewPosition = RE_GetViewPosition;
	re.LightForPoint = R_LightForPoint;
	re.AddLightToScene = RE_AddLightToScene;
	re.AddAdditiveLightToScene = RE_AddAdditiveLightToScene;
	re.RenderScene = RE_RenderScene;

	re.SetColor = RE_SetColor;
	re.DrawStretchPic = RE_StretchPic;
	re.DrawStretchRaw = RE_StretchRaw;
	re.UploadCinematic = RE_UploadCinematic;

	re.RegisterFont = RE_RegisterFont;
	re.RemapShader = R_RemapShader;
	re.GetEntityToken = R_GetEntityToken;
	re.inPVS = R_inPVS;

	re.TakeVideoFrame = RE_TakeVideoFrame;

	return &re;
}
