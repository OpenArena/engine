/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// tr_bloom.c: General post-processing shader stuff including bloom, leifx, and everything else
//		that's postprocessed.  Maintained by leilei and Hitchiker

#include "tr_local.h"


static cvar_t *r_bloom;
static cvar_t *r_bloom_sample_size;
static cvar_t *r_bloom_fast_sample;
static cvar_t *r_bloom_alpha;
static cvar_t *r_bloom_darken;
static cvar_t *r_bloom_intensity;
static cvar_t *r_bloom_diamond_size;
static cvar_t *r_bloom_cascade;
static cvar_t *r_bloom_cascade_blur;
static cvar_t *r_bloom_cascade_intensity;
static cvar_t *r_bloom_cascade_alpha;
static cvar_t *r_bloom_cascade_dry;
static cvar_t *r_bloom_dry;
extern cvar_t	*r_overBrightBits;
extern cvar_t	*r_gamma;
static cvar_t *r_bloom_reflection;		// LEILEI
static cvar_t *r_bloom_sky_only;		// LEILEI





extern float ScreenFrameCount;

static struct {
#ifdef GLSL_POSTPROCESSING
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} effect;
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} screen;
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} depth;
	struct {
		int		width, height;
	} work;
	qboolean started;
#endif
} postproc;



extern int	force32upload;
int		leifxmode;
int		leifxpass;
int		fakeit = 0;

extern int vresWidth;
extern int vresHeight;
/*
==============================================================================

						LIGHT BLOOMS

==============================================================================
*/

static float Diamond8x[8][8] = {
	{ 0.0f, 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f, 0.0f, },
	{ 0.0f, 0.0f, 0.2f, 0.3f, 0.3f, 0.2f, 0.0f, 0.0f, },
	{ 0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f, },
	{ 0.1f, 0.3f, 0.6f, 0.9f, 0.9f, 0.6f, 0.3f, 0.1f, },
	{ 0.1f, 0.3f, 0.6f, 0.9f, 0.9f, 0.6f, 0.3f, 0.1f, },
	{ 0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f, },
	{ 0.0f, 0.0f, 0.2f, 0.3f, 0.3f, 0.2f, 0.0f, 0.0f, },
	{ 0.0f, 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f, 0.0f  }
};


static float Star8x[8][8] = {
	{ 0.4f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f, 0.1f, 0.4f, },
	{ 0.1f, 0.6f, 0.2f, 0.0f, 0.0f, 0.2f, 0.6f, 0.1f, },
	{ 0.0f, 0.2f, 0.7f, 0.6f, 0.6f, 0.7f, 0.2f, 0.0f, },
	{ 0.0f, 0.0f, 0.6f, 0.9f, 0.9f, 0.6f, 0.0f, 0.0f, },
	{ 0.0f, 0.0f, 0.6f, 0.9f, 0.9f, 0.6f, 0.0f, 0.0f, },
	{ 0.0f, 0.2f, 0.7f, 0.6f, 0.6f, 0.7f, 0.2f, 0.0f, },
	{ 0.1f, 0.6f, 0.2f, 0.0f, 0.0f, 0.2f, 0.6f, 0.1f, },
	{ 0.4f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f, 0.1f, 0.4f, }
};


static float Diamond6x[6][6] = {
	{ 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f, },
	{ 0.0f, 0.3f, 0.5f, 0.5f, 0.3f, 0.0f, },
	{ 0.1f, 0.5f, 0.9f, 0.9f, 0.5f, 0.1f, },
	{ 0.1f, 0.5f, 0.9f, 0.9f, 0.5f, 0.1f, },
	{ 0.0f, 0.3f, 0.5f, 0.5f, 0.3f, 0.0f, },
	{ 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f  }
};

static float Diamond4x[4][4] = {
	{ 0.3f, 0.4f, 0.4f, 0.3f, },
	{ 0.4f, 0.9f, 0.9f, 0.4f, },
	{ 0.4f, 0.9f, 0.9f, 0.4f, },
	{ 0.3f, 0.4f, 0.4f, 0.3f  }
};

static struct {
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} effect;
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} effect2;
	struct {
		image_t	*texture;
		int		width, height;
		float	readW, readH;
	} screen;
	struct {
		int		width, height;
	} work;
	qboolean started;
} bloom;



static void ID_INLINE R_Bloom_Quad( int width, int height, float texX, float texY, float texWidth, float texHeight )
{
	int x = 0;
	int y = 0;
	x = 0;
	y += glConfig.vidHeight - height;
	width += x;
	height += y;

	texWidth += texX;
	texHeight += texY;

	qglBegin( GL_QUADS );
	qglTexCoord2f(	texX,						texHeight	);
	qglVertex2f(	x,							y	);

	qglTexCoord2f(	texX,						texY	);
	qglVertex2f(	x,							height	);

	qglTexCoord2f(	texWidth,					texY	);
	qglVertex2f(	width,						height	);

	qglTexCoord2f(	texWidth,					texHeight	);
	qglVertex2f(	width,						y	);
	qglEnd ();
}


// LEILEI - Bloom Reflection


static void ID_INLINE R_Bloom_Quad_Lens(float offsert, int width, int height, float texX, float texY, float texWidth, float texHeight)
{
	int x = 0;
	int y = 0;
	x = 0;
	y += glConfig.vidHeight - height;
	width += x;
	height += y;
	offsert = offsert * 9; // bah
	texWidth -= texX;
	texHeight -= texY;

	qglBegin( GL_QUADS );
	qglTexCoord2f(	texX,						texHeight	);
	qglVertex2f(	width + offsert,							height + offsert	);

	qglTexCoord2f(	texX,						texY	);
	qglVertex2f(	width + offsert,							y	- offsert);

	qglTexCoord2f(	texWidth,					texY	);
	qglVertex2f(	x - offsert,						y	- offsert);

	qglTexCoord2f(	texWidth,					texHeight	);
	qglVertex2f(	x - offsert,						height	+ offsert);
	qglEnd ();
}


// LEILEI - end

/*
=================
R_Bloom_InitTextures
=================
*/
static void R_Bloom_InitTextures( void )
{
	byte	*data;

	// find closer power of 2 to screen size
	for (bloom.screen.width = 1; bloom.screen.width< glConfig.vidWidth; bloom.screen.width *= 2);
	for (bloom.screen.height = 1; bloom.screen.height < glConfig.vidHeight; bloom.screen.height *= 2);

	bloom.screen.readW = glConfig.vidWidth / (float)bloom.screen.width;
	bloom.screen.readH = glConfig.vidHeight / (float)bloom.screen.height;

	// find closer power of 2 to effect size
	bloom.work.width = r_bloom_sample_size->integer;
	bloom.work.height = bloom.work.width * ( glConfig.vidWidth / glConfig.vidHeight );

	for (bloom.effect.width = 1; bloom.effect.width < bloom.work.width; bloom.effect.width *= 2);
	for (bloom.effect.height = 1; bloom.effect.height < bloom.work.height; bloom.effect.height *= 2);

	bloom.effect.readW = bloom.work.width / (float)bloom.effect.width;
	bloom.effect.readH = bloom.work.height / (float)bloom.effect.height;

	bloom.effect2.readW=bloom.effect.readW;
	bloom.effect2.readH=bloom.effect.readH;
	bloom.effect2.width=bloom.effect.width;
	bloom.effect2.height=bloom.effect.height;


	// disable blooms if we can't handle a texture of that size
	if( bloom.screen.width > glConfig.maxTextureSize ||
	        bloom.screen.height > glConfig.maxTextureSize ||
	        bloom.effect.width > glConfig.maxTextureSize ||
	        bloom.effect.height > glConfig.maxTextureSize ||
	        bloom.work.width > glConfig.vidWidth ||
	        bloom.work.height > glConfig.vidHeight
	  ) {
		ri.Cvar_Set( "r_bloom", "0" );
		Com_Printf( S_COLOR_YELLOW"WARNING: 'R_InitBloomTextures' too high resolution for light bloom, effect disabled\n" );
		return;
	}

	// leilei - let's not do that bloom disabling anymore
	force32upload = 1;

	data = ri.Hunk_AllocateTempMemory( bloom.screen.width * bloom.screen.height * 4 );
	Com_Memset( data, 0, bloom.screen.width * bloom.screen.height * 4 );
	bloom.screen.texture = R_CreateImage( "***bloom screen texture***", data, bloom.screen.width, bloom.screen.height, 0, 0, GL_CLAMP_TO_EDGE  );
	ri.Hunk_FreeTempMemory( data );

	data = ri.Hunk_AllocateTempMemory( bloom.effect.width * bloom.effect.height * 4 );
	Com_Memset( data, 0, bloom.effect.width * bloom.effect.height * 4 );
	bloom.effect.texture = R_CreateImage( "***bloom effect texture***", data, bloom.effect.width, bloom.effect.height, 0, 0, GL_CLAMP_TO_EDGE  );
	bloom.effect2.texture = R_CreateImage( "***bloom effect texture 2***", data, bloom.effect.width, bloom.effect.height, 0, 0, GL_CLAMP_TO_EDGE  );
	ri.Hunk_FreeTempMemory( data );
	bloom.started = qtrue;
	force32upload = 0;
}



/*
=================
R_InitBloomTextures
=================
*/
void R_InitBloomTextures( void )
{
	if( !r_bloom->integer )
		return;
	memset( &bloom, 0, sizeof( bloom ));
	R_Bloom_InitTextures ();
}
/*
=================
R_Bloom_DrawEffect
=================
*/
static void R_Bloom_DrawEffect( void )
{
	float alpha=r_bloom_alpha->value;
	GL_Bind( bloom.effect.texture );
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
	if(r_bloom_cascade->integer) {
		alpha=r_bloom_cascade_alpha->value;
	}
	qglColor4f( alpha,alpha,alpha, 1.0f );
	R_Bloom_Quad( glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
}


/*
=================
R_Bloom_LensEffect


LEILEI's Silly hack
=================
*/
static void R_Bloom_LensEffect( void )
{
	GL_Bind( bloom.effect.texture );
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE );

	qglColor4f( 0.78f, 0.23f, 0.34f, 0.07f );
	R_Bloom_Quad_Lens(16, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.78f, 0.39f, 0.21f, 0.07f );
	R_Bloom_Quad_Lens(32, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.78f, 0.59f, 0.21f, 0.07f );
	R_Bloom_Quad_Lens(48, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.71f, 0.75f, 0.21f, 0.07f );
	R_Bloom_Quad_Lens(64, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.52f, 0.78f, 0.21f, 0.07f );
	R_Bloom_Quad_Lens(80, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.32f, 0.78f, 0.21f, 0.07f );
	R_Bloom_Quad_Lens(96, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.21f, 0.78f, 0.28f, 0.07f );
	R_Bloom_Quad_Lens(112, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.21f, 0.78f, 0.47f, 0.07f );
	R_Bloom_Quad_Lens(128, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.21f, 0.77f, 0.66f, 0.07f );
	R_Bloom_Quad_Lens(144, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.21f, 0.67f, 0.78f, 0.07f );
	R_Bloom_Quad_Lens(160, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.21f, 0.47f, 0.78f, 0.07f );
	R_Bloom_Quad_Lens(176, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.21f, 0.28f, 0.78f, 0.07f );
	R_Bloom_Quad_Lens(192, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.35f, 0.21f, 0.78f, 0.07f );
	R_Bloom_Quad_Lens(208, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.53f, 0.21f, 0.78f, 0.07f );
	R_Bloom_Quad_Lens(224, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.72f, 0.21f, 0.75f, 0.07f );
	R_Bloom_Quad_Lens(240, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );
	qglColor4f( 0.78f, 0.21f, 0.59f, 0.07f );
	R_Bloom_Quad_Lens(256, glConfig.vidWidth, glConfig.vidHeight, 0, 0, bloom.effect.readW, bloom.effect.readW );

}



/*
 =================
 R_Bloom_Cascaded
 =================
 Tcpp: sorry for my poor English skill.
 */
static void R_Bloom_Cascaded( void )
{
	int scale;
	int oldWorkW, oldWorkH;
	int newWorkW, newWorkH;
	float bloomShiftX=r_bloom_cascade_blur->value/(float)bloom.effect.width;
	float bloomShiftY=r_bloom_cascade_blur->value/(float)bloom.effect.height;
	float intensity=r_bloom_cascade_intensity->value;
	float intensity2;

	qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	//Take the backup texture and downscale it
	GL_Bind( bloom.screen.texture );
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	R_Bloom_Quad( bloom.work.width, bloom.work.height, 0, 0, bloom.screen.readW, bloom.screen.readH );
	//Copy downscaled framebuffer into a texture
	GL_Bind( bloom.effect.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );

	/* Copy the result to the effect texture */
	GL_Bind( bloom.effect2.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );

	// do blurs..
	scale=32;
	while(bloom.work.width<scale)
		scale>>=1;
	while(bloom.work.height<scale)
		scale>>=1;

	// prepare the first level.
	newWorkW=bloom.work.width/scale;
	newWorkH=bloom.work.height/scale;

	GL_Bind( bloom.effect2.texture );
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	intensity2=intensity/(float)scale;
	qglColor4f( intensity2, intensity2, intensity2, 1.0 );
	R_Bloom_Quad( newWorkW, newWorkH,
	              0, 0,
	              bloom.effect2.readW, bloom.effect2.readH );

	// go through levels.
	while(scale>1) {
		float oldScaleInv=1.f/(float)scale;
		scale>>=1;
		oldWorkH=newWorkH;
		oldWorkW=newWorkW;
		newWorkW=bloom.work.width/scale;
		newWorkH=bloom.work.height/scale;

		// get effect texture.
		GL_Bind( bloom.effect.texture );
		qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, oldWorkW, oldWorkH);

		// maginfy the previous level.
		if(r_bloom_cascade_blur->value<.01f) {
			// don't blur.
			qglColor4f( 1.f, 1.f, 1.f, 1.0 );
			GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
			R_Bloom_Quad( newWorkW, newWorkH,
			              0, 0,
			              bloom.effect.readW*oldScaleInv, bloom.effect.readH*oldScaleInv );
		}
		else {
			// blur.
			qglColor4f( .25f, .25f, .25f, 1.0 );
			GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
			R_Bloom_Quad( newWorkW, newWorkH,
			              -bloomShiftX, -bloomShiftY,
			              bloom.effect.readW*oldScaleInv, bloom.effect.readH*oldScaleInv );

			GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
			R_Bloom_Quad( newWorkW, newWorkH,
			              bloomShiftX, -bloomShiftY,
			              bloom.effect.readW*oldScaleInv, bloom.effect.readH*oldScaleInv );
			R_Bloom_Quad( newWorkW, newWorkH,
			              -bloomShiftX, bloomShiftY,
			              bloom.effect.readW*oldScaleInv, bloom.effect.readH*oldScaleInv );
			R_Bloom_Quad( newWorkW, newWorkH,
			              bloomShiftX, bloomShiftY,
			              bloom.effect.readW*oldScaleInv, bloom.effect.readH*oldScaleInv );
		}

		// add the input.
		intensity2=intensity/(float)scale;
		qglColor4f( intensity2, intensity2, intensity2, 1.0 );

		GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
		GL_Bind( bloom.effect2.texture );

		R_Bloom_Quad( newWorkW, newWorkH,
		              0, 0,
		              bloom.effect2.readW, bloom.effect2.readH );


	}

	GL_Bind( bloom.effect.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );

}

/*
=================
R_Bloom_GeneratexDiamonds
=================
*/
static void R_Bloom_WarsowEffect( void )
{
	int		i, j, k;
	float	intensity, scale, *diamond;


	qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	//Take the backup texture and downscale it
	GL_Bind( bloom.screen.texture );
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	R_Bloom_Quad( bloom.work.width, bloom.work.height, 0, 0, bloom.screen.readW, bloom.screen.readH );
	//Copy downscaled framebuffer into a texture
	GL_Bind( bloom.effect.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
	// darkening passes with repeated filter
	if( r_bloom_darken->integer ) {
		int i;
		GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO );

		for( i = 0; i < r_bloom_darken->integer; i++ ) {
			R_Bloom_Quad( bloom.work.width, bloom.work.height,
			              0, 0,
			              bloom.effect.readW, bloom.effect.readH );
		}
		qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
	}
	/* Copy the result to the effect texture */
	GL_Bind( bloom.effect.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );

	// bluring passes, warsow uses a repeated semi blend on a selectable diamond grid
	qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE_MINUS_SRC_COLOR );
	if( r_bloom_diamond_size->integer > 7 || r_bloom_diamond_size->integer <= 3 ) {
		if( r_bloom_diamond_size->integer != 8 )
			ri.Cvar_Set( "r_bloom_diamond_size", "8" );
	}
	else if( r_bloom_diamond_size->integer > 5 ) {
		if( r_bloom_diamond_size->integer != 6 )
			ri.Cvar_Set( "r_bloom_diamond_size", "6" );
	}
	else if( r_bloom_diamond_size->integer > 3 ) {
		if( r_bloom_diamond_size->integer != 4 )
			ri.Cvar_Set( "r_bloom_diamond_size", "4" );
	}

	switch( r_bloom_diamond_size->integer ) {
	case 4:
		k = 2;
		diamond = &Diamond4x[0][0];
		scale = r_bloom_intensity->value * 0.8f;
		break;
	case 6:
		k = 3;
		diamond = &Diamond6x[0][0];
		scale = r_bloom_intensity->value * 0.5f;
		break;
	case 9:
		k = 4;
		diamond = &Star8x[0][0];
		scale = r_bloom_intensity->value * 0.3f;
		break;

	default:
	case 8:
		k = 4;
		diamond = &Diamond8x[0][0];
		scale = r_bloom_intensity->value * 0.3f;
		break;
	}

	for( i = 0; i < r_bloom_diamond_size->integer; i++ ) {
		for( j = 0; j < r_bloom_diamond_size->integer; j++, diamond++ ) {
			float x, y;
			intensity =  *diamond * scale;
			if( intensity < 0.01f )
				continue;
			qglColor4f( intensity, intensity, intensity, 1.0 );
			x = (i - k) * ( 2 / 640.0f ) * bloom.effect.readW;
			y = (j - k) * ( 2 / 480.0f ) * bloom.effect.readH;

			R_Bloom_Quad( bloom.work.width, bloom.work.height, x, y, bloom.effect.readW, bloom.effect.readH );
		}
	}
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
}

/*
=================
R_Bloom_BackupScreen
Backup the full original screen to a texture for downscaling and later restoration
=================
*/
static void R_Bloom_BackupScreen( void )
{
	GL_Bind( bloom.screen.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight );
}

/*
=================
R_Bloom_RestoreScreen
Restore the temporary framebuffer section we used with the backup texture
=================
*/
static void R_Bloom_RestoreScreen( void )
{
	float dry=r_bloom_dry->value;
	if(r_bloom_cascade->integer)
		dry=r_bloom_cascade_dry->value;
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	GL_Bind( bloom.screen.texture );
	qglColor4f( dry,dry,dry, 1 );
	if(dry<.99f) {
		R_Bloom_Quad( bloom.screen.width, bloom.screen.height, 0, 0,
		              1.f,
		              1.f );
	}
	else {
		R_Bloom_Quad( bloom.work.width, bloom.work.height, 0, 0,
		              bloom.work.width / (float)bloom.screen.width,
		              bloom.work.height / (float)bloom.screen.height );
	}
}
/*
=================
R_Bloom_RestoreScreen_Postprocessed
Restore the temporary framebuffer section we used with the backup texture
=================
*/
extern int mpasses;

static void R_Bloom_RestoreScreen_Postprocessed( void )
{
#ifdef GLSL_POSTPROCESSING
	glslProgram_t	*program = NULL;
	if (leifxmode) {
		if (leifxmode == 1) {
			if (vertexShaders) R_GLSL_UseProgram(tr.leiFXDitherProgram);
			program=tr.programs[tr.leiFXDitherProgram];
		}
		if (leifxmode == 2) {
			if (vertexShaders) R_GLSL_UseProgram(tr.leiFXGammaProgram);
			program=tr.programs[tr.leiFXGammaProgram];
		}
		if (leifxmode == 3) {
			if (vertexShaders) R_GLSL_UseProgram(tr.leiFXFilterProgram);
			program=tr.programs[tr.leiFXFilterProgram];
		}
		if (leifxmode == 666) {
			if (vertexShaders) R_GLSL_UseProgram(tr.BrightnessProgram);
			program=tr.programs[tr.BrightnessProgram];
		}
	}
	else {
		if (vertexShaders) R_GLSL_UseProgram(tr.postprocessingProgram);
		// Feed GLSL postprocess program
		program=tr.programs[tr.postprocessingProgram];
	}
	if (!program) {
		//If leifsmode has been set to something invalid we have to bail out or we will dereference a null pointer
		Com_Printf( S_COLOR_YELLOW"WARNING: 'leifxmode' has an invalid value\n" );
		return;
	}
	if (program->u_ScreenSizeX > -1) R_GLSL_SetUniform_u_ScreenSizeX(program, glConfig.vidWidth);

	if (program->u_ScreenSizeY > -1) R_GLSL_SetUniform_u_ScreenSizeY(program, glConfig.vidHeight);

	if (program->u_ScreenToNextPixelX > -1) R_GLSL_SetUniform_u_ScreenToNextPixelX(program, (float)1.0/(float)glConfig.vidWidth);

	if (program->u_ScreenToNextPixelY > -1) R_GLSL_SetUniform_u_ScreenToNextPixelY(program, (float)1.0/(float)glConfig.vidHeight);

	//if (program->u_Time > -1) R_GLSL_SetUniform_Time(program, backEnd.refdef.time);
	if (program->u_Time > -1) R_GLSL_SetUniform_Time(program, ScreenFrameCount);


	// Brightness stuff
	if (program->u_CC_Brightness > -1) R_GLSL_SetUniform_u_CC_Brightness(program, 1.0);
	if (program->u_CC_Gamma > -1) R_GLSL_SetUniform_u_CC_Gamma(program, r_gamma->value);
	if (program->u_CC_Overbright > -1) R_GLSL_SetUniform_u_CC_Overbright(program, r_overBrightBits->value);
	if (program->u_CC_Saturation > -1) R_GLSL_SetUniform_u_CC_Saturation(program, 1.0);
	if (program->u_CC_Contrast > -1) R_GLSL_SetUniform_u_CC_Contrast(program, 1.0);

	//
	if (leifxmode == 3) {
		R_GLSL_SetUniform_u_CC_Brightness(program, leifxpass);
	}

	if (program->u_zFar > -1) R_GLSL_SetUniform_u_zFar(program, tr.viewParms.zFar);
	GL_SelectTexture(0);
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	GL_Bind( postproc.screen.texture );
	GL_SelectTexture(7);
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	GL_Bind( postproc.depth.texture );
	qglColor4f( 1, 1, 1, 1 );

//	if (leifxmode == 778)
//		return;
	R_Bloom_Quad( glConfig.vidWidth, glConfig.vidHeight, 0, 0,
		              postproc.screen.readW,postproc.screen.readH );
	if (vertexShaders) R_GLSL_UseProgram(0);
	GL_SelectTexture(0);
#endif
}

/*
=================
R_Bloom_DownsampleView
Scale the copied screen back to the sample size used for subsequent passes
=================
*/
/*static void R_Bloom_DownsampleView( void )
{
	// TODO, Provide option to control the color strength here /
//	qglColor4f( r_bloom_darken->value, r_bloom_darken->value, r_bloom_darken->value, 1.0f );
	qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	GL_Bind( bloom.screen.texture );
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
	//Downscale it
	R_Bloom_Quad( bloom.work.width, bloom.work.height, 0, 0, bloom.screen.readW, bloom.screen.readH );
#if 1
	GL_Bind( bloom.effect.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
	// darkening passes
	if( r_bloom_darken->integer ) {
		int i;
		GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO );

		for( i = 0; i < r_bloom_darken->integer; i++ ) {
			R_Bloom_Quad( bloom.work.width, bloom.work.height,
				0, 0,
				bloom.effect.readW, bloom.effect.readH );
		}
		qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
	}
#endif
	// Copy the result to the effect texture /
	GL_Bind( bloom.effect.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
}

static void R_Bloom_CreateEffect( void ) {
	int dir, x;
	int range;

	//First step will zero dst, rest will one add
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ZERO );
//	GL_Bind( bloom.screen.texture );
	GL_Bind( bloom.effect.texture );
	range = 4;
	for (dir = 0;dir < 2;dir++)
	{
		// blend on at multiple vertical offsets to achieve a vertical blur
		// TODO: do offset blends using GLSL
		for (x = -range;x <= range;x++)
		{
			float xoffset, yoffset, r;
			if (!dir){
				xoffset = 0;
				yoffset = x*1.5;
			} else {
				xoffset = x*1.5;
				yoffset = 0;
			}
			xoffset /= bloom.work.width;
			yoffset /= bloom.work.height;
			// this r value looks like a 'dot' particle, fading sharply to
			// black at the edges
			// (probably not realistic but looks good enough)
			//r = ((range*range+1)/((float)(x*x+1)))/(range*2+1);
			//r = (dir ? 1.0f : brighten)/(range*2+1);
			r = 2.0f /(range*2+1)*(1 - x*x/(float)(range*range));
//			r *= r_bloom_darken->value;
			qglColor4f(r, r, r, 1);
			R_Bloom_Quad( bloom.work.width, bloom.work.height,
				xoffset, yoffset,
				bloom.effect.readW, bloom.effect.readH );
//				bloom.screen.readW, bloom.screen.readH );
			GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );
		}
	}
	GL_Bind( bloom.effect.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, bloom.work.width, bloom.work.height );
}*/

/*
=================
R_BloomScreen
=================
*/
void R_BloomScreen( void )
{
	if( !r_bloom->integer )
		return;
	if(r_bloom_sky_only->integer)
		return;
	if ( backEnd.doneBloom )
		return;
	if ( !backEnd.doneSurfaces )
		return;
	backEnd.doneBloom = qtrue;
	if( !bloom.started ) {
		R_Bloom_InitTextures();
		if( !bloom.started )
			return;
	}

	if ( !backEnd.projection2D )
		RB_SetGL2D();
#if 0
	// set up full screen workspace
	GL_TexEnv( GL_MODULATE );
	qglScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglMatrixMode( GL_PROJECTION );
	qglLoadIdentity ();
	qglOrtho( 0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1 );
	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity ();

	GL_Cull( CT_TWO_SIDED );
#endif

	qglColor4f( 1, 1, 1, 1 );

	//Backup the old screen in a texture
	R_Bloom_BackupScreen();
	// create the bloom texture using one of a few methods
	if(r_bloom_cascade->integer)
		R_Bloom_Cascaded();
	else
		R_Bloom_WarsowEffect ();
//	R_Bloom_CreateEffect();
	// restore the screen-backup to the screen
	R_Bloom_RestoreScreen();
	// Do the final pass using the bloom texture for the final effect
	R_Bloom_DrawEffect ();

// 	LEILEI - Lens Bloom Hack
//		The concept of this is to inverse the coordinates on both X and Y,
//		then scale outward using the offset value applied, as well as the modulated color
//		applied to give a rainbow streak effect.  Most effective with high bloom darkness
//		values.
	if(r_bloom_reflection->integer)
		R_Bloom_LensEffect ();
}



void R_BloomInit( void )
{
	memset( &bloom, 0, sizeof( bloom ));

	r_bloom = ri.Cvar_Get( "r_bloom", "0", CVAR_ARCHIVE );
	r_bloom_alpha = ri.Cvar_Get( "r_bloom_alpha", "0.3", CVAR_ARCHIVE );
	r_bloom_diamond_size = ri.Cvar_Get( "r_bloom_diamond_size", "8", CVAR_ARCHIVE );
	r_bloom_intensity = ri.Cvar_Get( "r_bloom_intensity", "1.3", CVAR_ARCHIVE );
	r_bloom_darken = ri.Cvar_Get( "r_bloom_darken", "4", CVAR_ARCHIVE );
	r_bloom_sample_size = ri.Cvar_Get( "r_bloom_sample_size", "256", CVAR_ARCHIVE|CVAR_LATCH ); // Tcpp: I prefer 256 to 128
	r_bloom_fast_sample = ri.Cvar_Get( "r_bloom_fast_sample", "0", CVAR_ARCHIVE|CVAR_LATCH );
	r_bloom_cascade = ri.Cvar_Get( "r_bloom_cascade", "0", CVAR_ARCHIVE );
	r_bloom_cascade_blur = ri.Cvar_Get( "r_bloom_cascade_blur", ".4", CVAR_ARCHIVE );
	r_bloom_cascade_intensity= ri.Cvar_Get( "r_bloom_cascade_intensity", "20", CVAR_ARCHIVE );
	r_bloom_cascade_alpha = ri.Cvar_Get( "r_bloom_cascade_alpha", "0.15", CVAR_ARCHIVE );
	r_bloom_cascade_dry = ri.Cvar_Get( "r_bloom_cascade_dry", "0.8", CVAR_ARCHIVE );
	r_bloom_dry = ri.Cvar_Get( "r_bloom_dry", "1", CVAR_ARCHIVE );
	r_bloom_reflection = ri.Cvar_Get( "r_bloom_reflection", "0", CVAR_ARCHIVE );
	r_bloom_sky_only = ri.Cvar_Get( "r_bloom_sky_only", "0", CVAR_ARCHIVE );
}





// =================================================================
// GLSL POST PROCESSING
// =================================================================





/*
=================
R_Postprocess_InitTextures
=================
*/
static void R_Postprocess_InitTextures( void )
{
#ifdef GLSL_POSTPROCESSING
	byte	*data;

	force32upload = 1;
	// find closer power of 2 to screen size
	for (postproc.screen.width = 1; postproc.screen.width< glConfig.vidWidth; postproc.screen.width *= 2) {}


//	for (postproc.screen.height = 1;postproc.screen.height < vidinted;postproc.screen.height *= 2);
//else
	for (postproc.screen.height = 1; postproc.screen.height < glConfig.vidHeight; postproc.screen.height *= 2) {}

	postproc.screen.readW = glConfig.vidWidth / (float)postproc.screen.width;
	postproc.screen.readH = glConfig.vidHeight / (float)postproc.screen.height;




	// find closer power of 2 to effect size
	postproc.work.width = r_bloom_sample_size->integer;
	postproc.work.height = postproc.work.width * ( glConfig.vidWidth / glConfig.vidHeight );

	for (postproc.effect.width = 1; postproc.effect.width < postproc.work.width; postproc.effect.width *= 2) {}
	for (postproc.effect.height = 1; postproc.effect.height < postproc.work.height; postproc.effect.height *= 2) {}

	postproc.effect.readW = postproc.work.width / (float)postproc.effect.width;
	postproc.effect.readH = postproc.work.height / (float)postproc.effect.height;

	// disable blooms if we can't handle a texture of that size
	if( 	postproc.screen.width > glConfig.maxTextureSize ||
	        postproc.screen.height > glConfig.maxTextureSize
	  ) {
		ri.Cvar_Set( "r_postprocess", "none" );
		Com_Printf( S_COLOR_YELLOW"WARNING: 'R_InitPostprocessTextures' too high resolution for postprocessing, effect disabled\n" );
		postprocess=0;
		return;
	}

	force32upload = 1;

	data = ri.Hunk_AllocateTempMemory( postproc.screen.width * postproc.screen.height * 4 );
	Com_Memset( data, 0, postproc.screen.width * postproc.screen.height * 4 );
	postproc.screen.texture = R_CreateImage( "***postproc screen texture***", data, postproc.screen.width, postproc.screen.height, 0, 0, GL_CLAMP_TO_EDGE  );
	ri.Hunk_FreeTempMemory( data );
	// GLSL Depth Buffer

	data = ri.Hunk_AllocateTempMemory( postproc.screen.width * postproc.screen.height *4);
	Com_Memset( data, 0, postproc.screen.width * postproc.screen.height * 4 );
	depthimage=1;
	postproc.depth.texture = R_CreateImage( "***depthbuffer texture***",  data, postproc.screen.width, postproc.screen.height, 0, 0, GL_CLAMP_TO_EDGE   );
	depthimage=0;
	ri.Hunk_FreeTempMemory( data );

	postproc.started = qtrue;
	force32upload = 0;
#endif
}

/*
=================
R_InitPostprocessTextures
=================
*/
void R_InitPostprocessTextures( void )
{
#ifdef GLSL_POSTPROCESSING
	if( !postprocess )
		return;
	memset( &postproc, 0, sizeof( postproc ));
	R_Postprocess_InitTextures ();
#endif
}


/*
=================
R_Postprocess_BackupScreen
Backup the full original screen to a texture for downscaling and later restoration
=================
*/
static void R_Postprocess_BackupScreen( void )
{
#ifdef GLSL_POSTPROCESSING
	GL_Bind( postproc.screen.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight );

	GL_Bind( postproc.depth.texture );
	qglCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, glConfig.vidWidth, glConfig.vidHeight );

#endif
}


/*
=================
R_PostprocessScreen
=================
*/
void R_PostprocessScreen( void )
{
#ifdef GLSL_POSTPROCESSING
	if( !postprocess )
		return;
	if ( backEnd.donepostproc )
		return;
	if ( !backEnd.doneSurfaces )
		return;
	if ( !vertexShaders )
		return;		// leilei - cards without support for this should not ever activate this
	backEnd.donepostproc = qtrue;
	if( !postproc.started ) {
		R_Postprocess_InitTextures();
		if( !postproc.started )
			return;
	}

	if ( !backEnd.projection2D )
		RB_SetGL2D();

	qglColor4f( 1, 1, 1, 1 );

	//Backup the old screen in a texture
	R_Postprocess_BackupScreen();
	//Redraw texture using GLSL program
	R_Bloom_RestoreScreen_Postprocessed();
#endif
}



void R_PostprocessingInit(void)
{
#ifdef GLSL_POSTPROCESSING
	memset( &postproc, 0, sizeof( postproc ));
#endif
}


// =================================================================
// LEIFX FILTER EFFECTS
//
//		Some require external GLSL scripts (placed in baseoa/glsl)
//	Some don't but are unstable on non-GL2 hardware anyway.
//
// =================================================================



static void ID_INLINE R_LeiFX_Pointless_Quad( int width, int height, float texX, float texY, float texWidth, float texHeight )
{
	int x = 0;
	int y = 0;
	x = 0;
	y += glConfig.vidHeight - height;
	width += x;
	height += y;

	texWidth += texX;
	texHeight += texY;

	qglBegin( GL_QUADS );

	qglTexCoord2f(	texX,						texHeight	);
	qglVertex2f(	x,							y	);

	qglTexCoord2f(	texX,						texY	);
	qglVertex2f(	x,							height	);

	qglTexCoord2f(	texWidth,					texY	);
	qglVertex2f(	width,						height	);

	qglTexCoord2f(	texWidth,					texHeight	);
	qglVertex2f(	width,						y	);
	qglEnd ();
}

void R_LeiFX_Stupid_Hack (void)
{
	GL_State( GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_SRC_COLOR);
	GL_Bind( tr.whiteImage );
	GL_Cull( CT_TWO_SIDED );

	qglColor4f( 1, 1, 1, 1 );
	R_LeiFX_Pointless_Quad( 0, 0, 0, 0, 1, 1 );

}
void R_LeiFXPostprocessDitherScreen( void )
{
#ifdef GLSL_POSTPROCESSING
	if( !r_leifx->integer)
		return;
	if ( backEnd.doneleifx)
		return;
	if ( !vertexShaders )
		return;		// leilei - cards without support for this should not ever activate this
//	if ( !backEnd.doneSurfaces )
//		return;
//	backEnd.doneleifx = qtrue;
	if( !postproc.started ) {
		force32upload = 1;
		R_Postprocess_InitTextures();
		if( !postproc.started )
			return;
	}

	if ( !backEnd.projection2D )
		RB_SetGL2D();

//	postprocess = 1;

	leifxmode = 1;

	// The stupidest hack in america
	R_LeiFX_Stupid_Hack();


	if (r_leifx->integer > 1) {
		leifxmode = 1;			// reduct and dither - 1 pass
		R_Postprocess_BackupScreen();
		R_Bloom_RestoreScreen_Postprocessed();
	}

	force32upload = 0;
#endif
}


void R_LeiFXPostprocessFilterScreen( void )
{
#ifdef GLSL_POSTPROCESSING
	if( !r_leifx->integer)
		return;
	if ( backEnd.doneleifx)
		return;
	if ( !vertexShaders )
		return;		// leilei - cards without support for this should not ever activate this
//	if ( !backEnd.doneSurfaces )
//		return;
	if( !postproc.started ) {
		force32upload = 1;
		R_Postprocess_InitTextures();
		if( !postproc.started )
			return;
	}

	if ( !backEnd.projection2D ) {
		RB_SetGL2D();
	}
	force32upload = 1;

//	postprocess = 1;

	/*	if (r_leifx->integer == 3){
			leifxmode = 2;			// gamma - 1 pass
		// The stupidest hack in america
		R_LeiFX_Stupid_Hack();
			R_Postprocess_BackupScreen();
			R_Bloom_RestoreScreen_Postprocessed();
			}
	*/						// Gamma disabled because r_alternateBrightness 2 makes it redundant now.
	if (r_leifx->integer > 3) {
		leifxmode = 3;			// filter - 4 pass
		// The stupidest hack in america
		R_LeiFX_Stupid_Hack();
		leifxpass = 0;
		R_Postprocess_BackupScreen();
		R_Bloom_RestoreScreen_Postprocessed();
		leifxpass = 1;
		R_Postprocess_BackupScreen();
		R_Bloom_RestoreScreen_Postprocessed();
		leifxpass = 2;
		R_Postprocess_BackupScreen();
		R_Bloom_RestoreScreen_Postprocessed();
		leifxpass = 3;
		R_Postprocess_BackupScreen();
		R_Bloom_RestoreScreen_Postprocessed();
	}
	backEnd.doneleifx = qtrue;

	force32upload = 0;

#endif
}


// =================================================================
// ALTERNATE SCREEN BRIGHTNESS
// =================================================================

extern cvar_t *r_alternateBrightness;
// shamelessly ripped off from tr_bloom.c
static void ID_INLINE R_Brighter_Quad( int width, int height, float texX, float texY, float texWidth, float texHeight )
{
	int x = 0;
	int y = 0;
	x = 0;
	y += glConfig.vidHeight - height;
	width += x;
	height += y;

	texWidth += texX;
	texHeight += texY;

	qglBegin( GL_QUADS );
	qglTexCoord2f(	texX,						texHeight	);
	qglVertex2f(	x,							y	);

	qglTexCoord2f(	texX,						texY	);
	qglVertex2f(	x,							height	);

	qglTexCoord2f(	texWidth,					texY	);
	qglVertex2f(	width,						height	);

	qglTexCoord2f(	texWidth,					texHeight	);
	qglVertex2f(	width,						y	);
	qglEnd ();
}



void R_BrightItUp (int dst, int src, float intensity)
{
	GL_State( GLS_DEPTHTEST_DISABLE | dst | src);
	GL_Bind( tr.whiteImage );
	GL_Cull( CT_TWO_SIDED );

	float alpha=intensity;	// why
	qglColor4f( alpha,alpha,alpha, 1.0f );
	if (!fakeit)
		R_Brighter_Quad( glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1, 1 );

}


void R_BrightScreen( void )
{
	int mode = 0;	// 0 = none; 1 = blend; 2 = shader
	if( !r_alternateBrightness->integer)
		return;
	if ( backEnd.doneAltBrightness)
		return;

	if (r_alternateBrightness->integer == 1)
		mode = 1;	// force use blend
#ifdef GLSL_POSTPROCESSING
	if (r_alternateBrightness->integer == 2) {
		// Automatically determine from capabilities
		if ( vertexShaders ) {
			mode = 2;
		}
		else {
			mode = 1;
		}
	}

	// the modern pixel shader way
	if (mode == 2) {

		if ( !vertexShaders )
			return;		// leilei - cards without support for this should not ever activate this
		if( !postproc.started ) {
			force32upload = 1;
			R_Postprocess_InitTextures();
			if( !postproc.started )
				return;
		}

		if ( !backEnd.projection2D )
			RB_SetGL2D();

		force32upload = 1;

		leifxmode = 666;			
		R_LeiFX_Stupid_Hack();
		R_Postprocess_BackupScreen();
		R_Bloom_RestoreScreen_Postprocessed();
		backEnd.doneAltBrightness = qtrue;

		force32upload = 0;

	}

	// the fixed function quad blending way
	else if (mode == 1)
#endif
	{
		int eh, ah;
		if ((r_overBrightBits->integer)) {

			ah = r_overBrightBits->integer;
			if (ah < 1) {
				ah = 1;
			}
			if (ah > 2) {
				ah = 2;    // clamp so it never looks stupid
			}

			// Blend method
			// do a loop for every overbright bit enabled

			for (eh=0; eh<ah; eh++)
				R_BrightItUp(GLS_SRCBLEND_DST_COLOR, GLS_DSTBLEND_ONE, 1.0f);

			backEnd.doneAltBrightness = qtrue;
		}
	}

}


void R_AltBrightnessInit( void )
{

	r_alternateBrightness = ri.Cvar_Get( "r_alternateBrightness", "0", CVAR_ARCHIVE | CVAR_LATCH);
}



// =================================================================
// WATER BUFFER TEST
// =================================================================

#if 0
static struct {
	// NO!
} water;

// leilei - experimental water effect
static void R_Water_InitTextures( void )
{
	// NO!
}
#endif



void R_InitWaterTextures( void )
{
	// NO!
}


#if 0
static void R_Water_BackupScreen( void )
{
	// NO!
}

static void R_WaterWorks( void )
{
	// NO!
}


static void R_Water_RestoreScreen( void )
{
	// NO!
}
#endif

void R_WaterInit( void )
{
	// NO!
}


void R_WaterScreen( void )
{
	// NO!
}

