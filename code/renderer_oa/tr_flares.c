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
// tr_flares.c

#include "tr_local.h"
#include "../qcommon/cm_local.h"

/*
=============================================================================

LIGHT FLARES

A light flare is an effect that takes place inside the eye when bright light
sources are visible.  The size of the flare relative to the screen is nearly
constant, irrespective of distance, but the intensity should be proportional to the
projected area of the light source.

A surface that has been flagged as having a light flare will calculate the depth
buffer value that its midpoint should have when the surface is added.

After all opaque surfaces have been rendered, the depth buffer is read back for
each flare in view.  If the point has not been obscured by a closer surface, the
flare should be drawn.

Surfaces that have a repeated texture should never be flagged as flaring, because
there will only be a single flare added at the midpoint of the polygon.

To prevent abrupt popping, the intensity of the flare is interpolated up and
down as it changes visibility.  This involves scene to scene state, unlike almost
all other aspects of the renderer, and is complicated by the fact that a single
frame may have multiple scenes.

RB_RenderFlares() will be called once per view (twice in a mirrored scene, potentially
up to five or more times in a frame with 3D status bar icons).

=============================================================================
*/


// flare states maintain visibility over multiple frames for fading
// layers: view, mirror, menu
typedef struct flare_s {
	struct		flare_s	*next;		// for active chain

	int			addedFrame;

	qboolean	inPortal;				// true if in a portal view of the scene
	int			frameSceneNum;
	void		*surface;
	int			fogNum;

	int			fadeTime;

	qboolean	visible;			// state of last test
	float		drawIntensity;		// may be non 0 even if !visible due to fading

	int			windowX, windowY;
	float		eyeZ;

	vec3_t		origin;
	vec3_t		color;
	int		radius;				// leilei - for dynamic light flares
	qboolean 	peek;
	int		ftype;			// leilei - flare types
	// 0 - off
	// 1 - nromal flare
	// 2 - hexagonal polygons (tcpp)
	// 3 - glow polygons	(tcpp)
	// 4 - hex and glow polygons	(tcpp)
	// 5 - lens reflections like it's 1997
	// 6 - fully modulated lens reflections
	// 7 - unmodulated lens reflections
	// 8 - anamorphic like it's 2009
	struct shader_s		*theshader;	// leilei - custom flare shaders
	int		type;			// 0 - map, 1 - dlight, 2 - sun
	float		delay;			// update delay time
} flare_t;

#define		MAX_FLARES		256 // was 128

flare_t		r_flareStructs[MAX_FLARES];
flare_t		*r_activeFlares, *r_inactiveFlares;


vec3_t		sunorg;		// sun flare hack
int flareCoeff;


/*
==================
R_SetFlareCoeff
==================
*/
static void R_SetFlareCoeff( void )
{

	if(r_flareCoeff->value == 0.0f)
		flareCoeff = atof(FLARE_STDCOEFF);
	else
		flareCoeff = r_flareCoeff->value;
}


qboolean 	forceit;	// for low quality flare testing


static int 	pvrhack		=	0;			// leilei = powervr workarounds
/*
==================
R_ClearFlares
==================
*/
void R_ClearFlares( void )
{
	int		i;

	Com_Memset( r_flareStructs, 0, sizeof( r_flareStructs ) );
	r_activeFlares = NULL;
	r_inactiveFlares = NULL;

	for ( i = 0 ; i < MAX_FLARES ; i++ ) {
		r_flareStructs[i].next = r_inactiveFlares;
		r_inactiveFlares = &r_flareStructs[i];
	}
	R_SetFlareCoeff();
}


/*
==================
RB_AddFlare

This is called at surface tesselation time
==================
*/

float			flaredsize;	// leilei - dirty flare fix for widescreens



void RB_AddFlare(srfFlare_t *surface, int fogNum, vec3_t point, vec3_t color, vec3_t normal, int radii, int efftype, float scaled, int type)
{
	int				i;
	flare_t			*f;
	vec3_t			local;
	float			d = 1;
	vec4_t			eye, clip, normalized, window;

	backEnd.pc.c_flareAdds++;

	// fade the intensity of the flare down as the
	// light surface turns away from the viewer
	if(normal && (normal[0] || normal[1] || normal[2]) ) {
		VectorSubtract( backEnd.viewParms.or.origin, point, local );
		VectorNormalizeFast(local);
		d = DotProduct(local, normal);

		// If the viewer is behind the flare don't add it.
		if(d < 0)
			return;
	}

	flaredsize = backEnd.viewParms.viewportHeight;

	R_TransformModelToClip( point, backEnd.or.modelMatrix,
	                        backEnd.viewParms.projectionMatrix, eye, clip );




	// check to see if the point is completely off screen
	for ( i = 0 ; i < 3 ; i++ ) {
		if ( clip[i] >= clip[3] || clip[i] <= -clip[3] ) {
			return;
		}
	}


	R_TransformClipToWindow( clip, &backEnd.viewParms, normalized, window );

	if ( window[0] < 0 || window[0] >= backEnd.viewParms.viewportWidth
	        || window[1] < 0 || window[1] >= backEnd.viewParms.viewportHeight ) {
		return;	// shouldn't happen, since we check the clip[] above, except for FP rounding
	}

	// see if a flare with a matching surface, scene, and view exists
	for ( f = r_activeFlares ; f ; f = f->next ) {
		if ( f->surface == surface && f->frameSceneNum == backEnd.viewParms.frameSceneNum
		        && f->inPortal == backEnd.viewParms.isPortal ) {
			break;
		}
	}

	// allocate a new one
	if (!f ) {
		if ( !r_inactiveFlares ) {
			// the list is completely full
			return;
		}
		f = r_inactiveFlares;
		r_inactiveFlares = r_inactiveFlares->next;
		f->next = r_activeFlares;
		r_activeFlares = f;

		f->surface = surface;
		f->frameSceneNum = backEnd.viewParms.frameSceneNum;
		f->inPortal = backEnd.viewParms.isPortal;
		f->addedFrame = -1;
	}

	if ( f->addedFrame != backEnd.viewParms.frameCount - 1 ) {
		f->visible = qfalse;
		f->fadeTime = backEnd.refdef.time - 2000;
	}

	f->addedFrame = backEnd.viewParms.frameCount;
	f->fogNum = fogNum;
	f->ftype = efftype;
	VectorCopy(point, f->origin);
	VectorCopy( color, f->color );


	if ( (r_flaresDlightFade->integer) && (type == 1) ) {	// leilei - dynamic light flares fading instantly
		f->fadeTime = -666;
	}





	if (!pvrhack)	// leilei - don't do this on powervr
		VectorScale( f->color, d, f->color );

	// save info needed to test
	f->windowX = backEnd.viewParms.viewportX + window[0];
	f->windowY = backEnd.viewParms.viewportY + window[1];


	f->radius = radii * scaled * 0.17;
	f->eyeZ = eye[2];
	f->theshader = tr.flareShader;
	f->type = type;

	if (f->type == 0)
		f->theshader = surface->shadder;
	else
		f->theshader = tr.flareShader;


	if ( (type == 1) && (r_flaresDlightScale->value) ) {	// leilei - dynamic light flare scale
		float ef = r_flaresDlightScale->value;
		if (ef > 1.0f) ef = 1.0f;
		if (ef < 0.01f) ef = 0.01f;

		f->radius *= ef;
	}

	if ( (type == 1) && (r_flaresDlightOpacity->value) ) {	// leilei - dynamic light flare scale
		float ef = r_flaresDlightOpacity->value;
		if (ef > 1.0f) ef = 1.0f;
		if (ef < 0.1f) ef = 0.1f;

		f->color[0] *= ef;
		f->color[1] *= ef;
		f->color[2] *= ef;
	}


//	if ( (r_flaresDlightShrink->integer) && (type == 1) ) 	// leilei - dynamic light flares shrinking when close
//	{


//	}


}

/*
==================
RB_AddDlightFlares
==================
*/
void RB_AddDlightFlares( void )
{
	dlight_t		*l;
	int				i, j, k;
	fog_t			*fog = NULL;

	if ( !r_flaresDlight->integer ) {	// leilei - dynamic light flares will be separate from flares
		return;
	}

	l = backEnd.refdef.dlights;

	if(tr.world)
		fog = tr.world->fogs;


	for (i=0 ; i<backEnd.refdef.num_dlights ; i++, l++) {

		if(fog) {
			// find which fog volume the light is in
			for ( j = 1 ; j < tr.world->numfogs ; j++ ) {
				fog = &tr.world->fogs[j];
				for ( k = 0 ; k < 3 ; k++ ) {
					if ( l->origin[k] < fog->bounds[0][k] || l->origin[k] > fog->bounds[1][k] ) {
						break;
					}
				}
				if ( k == 3 ) {
					break;
				}
			}
			if ( j == tr.world->numfogs ) {
				j = 0;
			}
		}
		else
			j = 0;

		RB_AddFlare( (void *)l, j, l->origin, l->color, NULL, l->radius * 0.6, r_flaresDlight->integer, 1.0f, 1);
	}
}

/*
===============================================================================

FLARE BACK END

===============================================================================
*/


void CM_Trace( trace_t *results, const vec3_t start, const vec3_t end, vec3_t mins, vec3_t maxs,
               clipHandle_t model, const vec3_t origin, int brushmask, int capsule, sphere_t *sphere );


/*
==================
RB_TestFlareFast

faster simple one.
==================
*/
static void RB_TestFlareFast( flare_t *f, int dotrace )
{
	float			depth;
	qboolean		visible;
	float			fade;
	float			screenZ;

	backEnd.pc.c_flareTests++;


	// doing a readpixels is as good as doing a glFinish(), so
	// don't bother with another sync
	glState.finishCalled = qfalse;
	if (f->type == 2) dotrace = 0; // sun cant trace
	// leilei - do trace, then complain
	if (dotrace) {
		trace_t  yeah;
		CM_Trace( &yeah, f->origin, backEnd.or.viewOrigin, NULL, NULL, 0, f->origin, 1, 0, NULL );
		if (yeah.fraction < 1) {
			visible = 0;
			return;
		}
		else {
			visible = 1;
		}
	}

	// leilei - delay hack, to speed up the renderer

	if (backEnd.refdef.time > f->delay) {

		// read back the z buffer contents

		qglReadPixels( f->windowX, f->windowY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth );
		f->delay = backEnd.refdef.time + r_flareDelay->value;


		screenZ = backEnd.viewParms.projectionMatrix[14] /
		          ( ( 2*depth - 1 ) * backEnd.viewParms.projectionMatrix[11] - backEnd.viewParms.projectionMatrix[10] );

		visible = ( -f->eyeZ - -screenZ ) < 24;



		if ( visible ) {
			if ( !f->visible ) {
				f->visible = qtrue;
				f->fadeTime = backEnd.refdef.time - 1;

			}
			{
				fade = ( ( backEnd.refdef.time - f->fadeTime ) / 1000.0f ) * r_flareFade->value;
			}
		}
		else {
			if ( f->visible ) {
				f->visible = qfalse;
				f->fadeTime = backEnd.refdef.time - 1;
			}
			fade = 1.0f - ( ( backEnd.refdef.time - f->fadeTime ) / 1000.0f ) * r_flareFade->value;
		}

		if ( fade < 0 ) {
			fade = 0;
		}
		if ( fade > 1 ) {
			fade = 1;
		}

		f->drawIntensity = fade;

	}
	else
		// leilei - continue drawing the flare from where we last checked
	{
		if (f->visible) {
			f->drawIntensity  = 1;
		}
		else {
			f->drawIntensity  = 0;
		}
	}

}

/*
==================
RB_TestFlare
==================
*/
static void RB_TestFlare( flare_t *f, int dotrace )
{
	float			depth;
	qboolean		visible;
	float			fade;
	float			screenZ;

	backEnd.pc.c_flareTests++;


	// doing a readpixels is as good as doing a glFinish(), so
	// don't bother with another sync
	glState.finishCalled = qfalse;


	if (f->type == 2) dotrace = 0; // sun cant trace
	// leilei - do trace, then complain
	if (dotrace) {
		trace_t  yeah;
		CM_Trace( &yeah, f->origin, backEnd.or.viewOrigin, NULL, NULL, 0, f->origin, 1, 0, NULL );
		if (yeah.fraction < 1) {
			visible = 0;
			return;
		}
		else {
			visible = 1;
		}
	}

	// read back the z buffer contents

	qglReadPixels( f->windowX, f->windowY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth );


	screenZ = backEnd.viewParms.projectionMatrix[14] /
	          ( ( 2*depth - 1 ) * backEnd.viewParms.projectionMatrix[11] - backEnd.viewParms.projectionMatrix[10] );

	visible = ( -f->eyeZ - -screenZ ) < 24;



	if ( visible ) {
		if ( !f->visible ) {
			f->visible = qtrue;
			f->fadeTime = backEnd.refdef.time - 1;

		}
		{
			fade = ( ( backEnd.refdef.time - f->fadeTime ) / 1000.0f ) * r_flareFade->value;
		}
	}
	else {
		if ( f->visible ) {
			f->visible = qfalse;
			f->fadeTime = backEnd.refdef.time - 1;
		}
		fade = 1.0f - ( ( backEnd.refdef.time - f->fadeTime ) / 1000.0f ) * r_flareFade->value;
	}

	if ( fade < 0 ) {
		fade = 0;
	}
	if ( fade > 1 ) {
		fade = 1;
	}

	f->drawIntensity = fade;

}

static void RB_TestFlareTraceOnly( flare_t *f )
{
	qboolean		visible;
	float			fade;

	backEnd.pc.c_flareTests++;


	// doing a readpixels is as good as doing a glFinish(), so
	// don't bother with another sync
	glState.finishCalled = qfalse;

	// read from a traceline
	trace_t  yeah;
	CM_Trace( &yeah, f->origin, backEnd.or.viewOrigin, NULL, NULL, 0, f->origin, 1, 0, NULL );
	if (yeah.fraction < 1) {
		visible = 0;
		return;
	}
	else {
		visible = 1;
	}

	if ( visible ) {
		if ( !f->visible ) {
			f->visible = qtrue;
			f->fadeTime = backEnd.refdef.time - 1;

		}
		{
			fade = ( ( backEnd.refdef.time - f->fadeTime ) / 1000.0f ) * r_flareFade->value;
		}
	}
	else {
		if ( f->visible ) {
			f->visible = qfalse;
			f->fadeTime = backEnd.refdef.time - 1;
		}
		fade = 1.0f - ( ( backEnd.refdef.time - f->fadeTime ) / 1000.0f ) * r_flareFade->value;
	}

	if ( fade < 0 ) {
		fade = 0;
	}
	if ( fade > 1 ) {
		fade = 1;
	}

	f->drawIntensity = fade;

}



/*
==================
RB_RenderFlare
==================
*/

void RB_RenderFlare( flare_t *f )
{
	float			size;
	vec3_t			color;
	int				iColor[3];
	float distance, intensity, factor;
	byte fogFactors[3] = {255, 255, 255};
	int ind=0;
	int alphcal;
	backEnd.pc.c_flareRenders++;

	flaredsize = backEnd.viewParms.viewportHeight * (f->radius * 0.06);
	float flaredsize2 = backEnd.viewParms.viewportHeight;

	// We don't want too big values anyways when dividing by distance.
	if(f->eyeZ > -1.0f)
		distance = 1.0f;
	else
		distance = -f->eyeZ;

	if ( (r_flaresDlightShrink->integer) && (f->type == 1) ) {	// leilei - dynamic light flares shrinking when close

		float newd = distance / (48.0f);
		if (newd > 1) newd = 1.0f;

		flaredsize *= newd;
	}


	if(!f->radius)
		f->radius = 0.0f;	// leilei - don't do a radius if there is no radius at all!

	// calculate the flare size..

	/*
	 * This is an alternative to intensity scaling. It changes the size of the flare on screen instead
	 * with growing distance. See in the description at the top why this is not the way to go.
	*/
	// size will change ~ 1/r.
	if (r_flareMethod->integer == 1 || r_flareMethod->integer == 4 ) {			// The "not the way to go" method.
		// seen in EF
		size = flaredsize * (r_flareSize->value / (distance * -2.0f));
	}
	else if (r_flareMethod->integer == 2) {			// Raven method
		size = flaredsize * ( r_flareSize->value/640.0f + 8 / -f->eyeZ );
	}
	else {

		size = flaredsize * ( (r_flareSize->value) /640.0f + 8 / distance );



	}





	/*
	 * As flare sizes stay nearly constant with increasing distance we must decrease the intensity
	 * to achieve a reasonable visual result. The intensity is ~ (size^2 / distance^2) which can be
	 * got by considering the ratio of
	 * (flaresurface on screen) : (Surface of sphere defined by flare origin and distance from flare)
	 * An important requirement is:
	 * intensity <= 1 for all distances.
	 *
	 * The formula used here to compute the intensity is as follows:
	 * intensity = flareCoeff * size^2 / (distance + size*sqrt(flareCoeff))^2
	 * As you can see, the intensity will have a max. of 1 when the distance is 0.
	 * The coefficient flareCoeff will determine the falloff speed with increasing distance.
	 */

	factor = distance + size * sqrt(flareCoeff);



	if (r_flareMethod->integer == 4)		// leilei - EF didn't scale intensity on distance. Speed I guess
		intensity = 1;
	else
		intensity = flareCoeff * size * size / (factor * factor);

	if (r_flareMethod->integer == 1) {	// leilei - stupid hack to fix the not the way method
		if (intensity > 1) intensity = 1;

	}

	if (pvrhack)
		VectorScale(f->color, 1, color );
	else
		VectorScale(f->color, f->drawIntensity * intensity, color);






// Calculations for fogging
	if(tr.world && f->fogNum > 0 && f->fogNum < tr.world->numfogs) {
		tess.numVertexes = 1;
		VectorCopy(f->origin, tess.xyz[0]);
		tess.fogNum = f->fogNum;

		RB_CalcModulateColorsByFog(fogFactors);

		// We don't need to render the flare if colors are 0 anyways.
		if(!(fogFactors[0] || fogFactors[1] || fogFactors[2]))
			return;
	}



	iColor[0] = color[0] * fogFactors[0];
	iColor[1] = color[1] * fogFactors[1];
	iColor[2] = color[2] * fogFactors[2];
	if (pvrhack)
		alphcal = f->drawIntensity * tr.identityLight * 255;	// Calculate alphas from intensity instead
	else
		alphcal = 255;				// Don't mess with alpha.




	float halfer = 1;
	if (f->ftype == 5 || f->ftype == 6 || f->ftype == 7 || f->ftype == 166) {
		RB_BeginSurface( tr.flareShaderAtlas, f->fogNum );
		halfer = 0.5f;
	}
	else {


		if (r_flareQuality->integer) {	// leilei - high quality flares get no depth testing
			int index;

			for(index = 0; index <f->theshader->numUnfoggedPasses; index++) {
				f->theshader->stages[index]->adjustColorsForFog = ACFF_NONE;
				f->theshader->stages[index]->stateBits |= GLS_DEPTHTEST_DISABLE;
			}
		}


		RB_BeginSurface( f->theshader, f->fogNum );


		halfer = 1;
	}

	// FIXME: use quadstamp?
	tess.xyz[tess.numVertexes][0] = f->windowX - size;
	tess.xyz[tess.numVertexes][1] = f->windowY - size;
	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = 0;
	tess.vertexColors[tess.numVertexes][0] = iColor[0];
	tess.vertexColors[tess.numVertexes][1] = iColor[1];
	tess.vertexColors[tess.numVertexes][2] = iColor[2];
	tess.vertexColors[tess.numVertexes][3] = alphcal;
	tess.numVertexes++;

	tess.xyz[tess.numVertexes][0] = f->windowX - size;
	tess.xyz[tess.numVertexes][1] = f->windowY + size;
	tess.texCoords[tess.numVertexes][0][0] = 0;
	tess.texCoords[tess.numVertexes][0][1] = 1 * halfer;
	tess.vertexColors[tess.numVertexes][0] = iColor[0];
	tess.vertexColors[tess.numVertexes][1] = iColor[1];
	tess.vertexColors[tess.numVertexes][2] = iColor[2];
	tess.vertexColors[tess.numVertexes][3] = alphcal;
	tess.numVertexes++;

	tess.xyz[tess.numVertexes][0] = f->windowX + size;
	tess.xyz[tess.numVertexes][1] = f->windowY + size;
	tess.texCoords[tess.numVertexes][0][0] = 1 * halfer;
	tess.texCoords[tess.numVertexes][0][1] = 1 * halfer;
	tess.vertexColors[tess.numVertexes][0] = iColor[0];
	tess.vertexColors[tess.numVertexes][1] = iColor[1];
	tess.vertexColors[tess.numVertexes][2] = iColor[2];
	tess.vertexColors[tess.numVertexes][3] = alphcal;
	tess.numVertexes++;

	tess.xyz[tess.numVertexes][0] = f->windowX + size;
	tess.xyz[tess.numVertexes][1] = f->windowY - size;
	tess.texCoords[tess.numVertexes][0][0] = 1 * halfer;
	tess.texCoords[tess.numVertexes][0][1] = 0;
	tess.vertexColors[tess.numVertexes][0] = iColor[0];
	tess.vertexColors[tess.numVertexes][1] = iColor[1];
	tess.vertexColors[tess.numVertexes][2] = iColor[2];
	tess.vertexColors[tess.numVertexes][3] = alphcal;
	tess.numVertexes++;

	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 1;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 0;
	tess.indexes[tess.numIndexes++] = 2;
	tess.indexes[tess.numIndexes++] = 3;

	ind+=4;

	// reflections -- tcpparena

	if(f->ftype == 2 || f->ftype == 4) {

		// renders sharp lens flare.

		float cx, cy;
		float dx, dy;
		float size2;
		const float poses[]=	{-.15f, 0.6f, -.1f, -.6f, -1.8f};
		const float sizes[]=	{0.14f, 0.2f, 0.1f, 0.2f, 1.0f};
		int brightness1[]=	{8,25, 40, 26, 10};		// red
		int brightness2[]=	{15,23, 25, 30, 5};		// green
		int brightness3[]=	{12,20, 30, 28, 10};	// blue
		const float r3_2=0.866025403784439f;
		int n;
		cx=backEnd.viewParms.viewportX+(backEnd.viewParms.viewportWidth>>1);
		cy=backEnd.viewParms.viewportY+(backEnd.viewParms.viewportHeight>>1);
		for(n=0; n<5; n++) {
			dx=(f->windowX-cx)*poses[n]+cx;
			dy=(f->windowY-cy)*poses[n]+cy;
			size2=sizes[n]*backEnd.viewParms.viewportWidth*.25f;

			brightness1[n]=(int)(brightness1[n]*r_lensReflectionBrightness->value);
			brightness2[n]=(int)(brightness2[n]*r_lensReflectionBrightness->value);
			brightness3[n]=(int)(brightness3[n]*r_lensReflectionBrightness->value);

			tess.xyz[tess.numVertexes][0] = dx-size2;
			tess.xyz[tess.numVertexes][1] = dy;
			tess.texCoords[tess.numVertexes][0][0] = .5f;
			tess.texCoords[tess.numVertexes][0][1] = .5f;
			tess.vertexColors[tess.numVertexes][0] = (iColor[0]*brightness1[n])>>8;
			tess.vertexColors[tess.numVertexes][1] = (iColor[1]*brightness2[n])>>8;
			tess.vertexColors[tess.numVertexes][2] = (iColor[2]*brightness3[n])>>8;
			tess.vertexColors[tess.numVertexes][3] = 255;
			tess.numVertexes++;

			tess.xyz[tess.numVertexes][0] = dx-size2*.5f;
			tess.xyz[tess.numVertexes][1] = dy-size2*r3_2;
			tess.texCoords[tess.numVertexes][0][0] = .5f;
			tess.texCoords[tess.numVertexes][0][1] = .5f;
			tess.vertexColors[tess.numVertexes][0] = (iColor[0]*brightness1[n])>>8;
			tess.vertexColors[tess.numVertexes][1] = (iColor[1]*brightness2[n])>>8;
			tess.vertexColors[tess.numVertexes][2] = (iColor[2]*brightness3[n])>>8;
			tess.vertexColors[tess.numVertexes][3] = 255;
			tess.numVertexes++;

			tess.xyz[tess.numVertexes][0] = dx+size2*.5f;
			tess.xyz[tess.numVertexes][1] = dy-size2*r3_2;
			tess.texCoords[tess.numVertexes][0][0] = .5f;
			tess.texCoords[tess.numVertexes][0][1] = .5f;
			tess.vertexColors[tess.numVertexes][0] = (iColor[0]*brightness1[n])>>8;
			tess.vertexColors[tess.numVertexes][1] = (iColor[1]*brightness2[n])>>8;
			tess.vertexColors[tess.numVertexes][2] = (iColor[2]*brightness3[n])>>8;
			tess.vertexColors[tess.numVertexes][3] = 255;
			tess.numVertexes++;

			tess.xyz[tess.numVertexes][0] = dx+size2;
			tess.xyz[tess.numVertexes][1] = dy;
			tess.texCoords[tess.numVertexes][0][0] = .5f;
			tess.texCoords[tess.numVertexes][0][1] = .5f;
			tess.vertexColors[tess.numVertexes][0] = (iColor[0]*brightness1[n])>>8;
			tess.vertexColors[tess.numVertexes][1] = (iColor[1]*brightness2[n])>>8;
			tess.vertexColors[tess.numVertexes][2] = (iColor[2]*brightness3[n])>>8;
			tess.vertexColors[tess.numVertexes][3] = 255;
			tess.numVertexes++;

			tess.xyz[tess.numVertexes][0] = dx+size2*.5f;
			tess.xyz[tess.numVertexes][1] = dy+size2*r3_2;
			tess.texCoords[tess.numVertexes][0][0] = .5f;
			tess.texCoords[tess.numVertexes][0][1] = .5f;
			tess.vertexColors[tess.numVertexes][0] = (iColor[0]*brightness1[n])>>8;
			tess.vertexColors[tess.numVertexes][1] = (iColor[1]*brightness2[n])>>8;
			tess.vertexColors[tess.numVertexes][2] = (iColor[2]*brightness3[n])>>8;
			tess.vertexColors[tess.numVertexes][3] = 255;
			tess.numVertexes++;

			tess.xyz[tess.numVertexes][0] = dx-size2*.5f;
			tess.xyz[tess.numVertexes][1] = dy+size2*r3_2;
			tess.texCoords[tess.numVertexes][0][0] = .5f;
			tess.texCoords[tess.numVertexes][0][1] = .5f;
			tess.vertexColors[tess.numVertexes][0] = (iColor[0]*brightness1[n])>>8;
			tess.vertexColors[tess.numVertexes][1] = (iColor[1]*brightness2[n])>>8;
			tess.vertexColors[tess.numVertexes][2] = (iColor[2]*brightness3[n])>>8;
			tess.vertexColors[tess.numVertexes][3] = 255;
			tess.numVertexes++;

			tess.indexes[tess.numIndexes++] = 2+ind;
			tess.indexes[tess.numIndexes++] = 1+ind;
			tess.indexes[tess.numIndexes++] = 0+ind;
			tess.indexes[tess.numIndexes++] = 3+ind;
			tess.indexes[tess.numIndexes++] = 2+ind;
			tess.indexes[tess.numIndexes++] = 0+ind;
			tess.indexes[tess.numIndexes++] = 4+ind;
			tess.indexes[tess.numIndexes++] = 3+ind;
			tess.indexes[tess.numIndexes++] = 0+ind;
			tess.indexes[tess.numIndexes++] = 5+ind;
			tess.indexes[tess.numIndexes++] = 4+ind;
			tess.indexes[tess.numIndexes++] = 0+ind;

			ind+=6;

		}
	}



	if(f->ftype == 3 || f->ftype == 4) {

		// renders fuzzy lens flare.

		float cx, cy;
		float dx, dy;
		float size2;
		const float poses[]=	{1.7f, -0.2f};
		const float sizes[]=	{1.2f, 0.2f};
		int brightness1[]=	{25, 40};	// red
		int brightness2[]=	{35, 10};	// green
		int brightness3[]=	{30, 15};	// blue
		int n;
		cx=backEnd.viewParms.viewportX+(backEnd.viewParms.viewportWidth>>1);
		cy=backEnd.viewParms.viewportY+(backEnd.viewParms.viewportHeight>>1);


		for(n=0; n<2; n++) {
			dx=(f->windowX-cx)*poses[n]+cx;
			dy=(f->windowY-cy)*poses[n]+cy;
			size2=sizes[n]*flaredsize2*.25f;

			brightness1[n]=(int)(brightness1[n]*r_lensReflectionBrightness->value);
			brightness2[n]=(int)(brightness2[n]*r_lensReflectionBrightness->value);
			brightness3[n]=(int)(brightness3[n]*r_lensReflectionBrightness->value);

			tess.xyz[tess.numVertexes][0] = dx-size2;
			tess.xyz[tess.numVertexes][1] = dy-size2;
			tess.texCoords[tess.numVertexes][0][0] = 0.f;
			tess.texCoords[tess.numVertexes][0][1] = 0.f;
			tess.vertexColors[tess.numVertexes][0] = (iColor[0]*brightness1[n])>>8;
			tess.vertexColors[tess.numVertexes][1] = (iColor[1]*brightness2[n])>>8;
			tess.vertexColors[tess.numVertexes][2] = (iColor[2]*brightness3[n])>>8;
			tess.vertexColors[tess.numVertexes][3] = alphcal;
			tess.numVertexes++;

			tess.xyz[tess.numVertexes][0] = dx-size2;
			tess.xyz[tess.numVertexes][1] = dy+size2;
			tess.texCoords[tess.numVertexes][0][0] = 0.f;
			tess.texCoords[tess.numVertexes][0][1] = 1.f;
			tess.vertexColors[tess.numVertexes][0] = (iColor[0]*brightness1[n])>>8;
			tess.vertexColors[tess.numVertexes][1] = (iColor[1]*brightness2[n])>>8;
			tess.vertexColors[tess.numVertexes][2] = (iColor[2]*brightness3[n])>>8;
			tess.vertexColors[tess.numVertexes][3] = alphcal;
			tess.numVertexes++;

			tess.xyz[tess.numVertexes][0] = dx+size2;
			tess.xyz[tess.numVertexes][1] = dy+size2;
			tess.texCoords[tess.numVertexes][0][0] = 1.f;
			tess.texCoords[tess.numVertexes][0][1] = 1.f;
			tess.vertexColors[tess.numVertexes][0] = (iColor[0]*brightness1[n])>>8;
			tess.vertexColors[tess.numVertexes][1] = (iColor[1]*brightness2[n])>>8;
			tess.vertexColors[tess.numVertexes][2] = (iColor[2]*brightness3[n])>>8;
			tess.vertexColors[tess.numVertexes][3] = alphcal;
			tess.numVertexes++;

			tess.xyz[tess.numVertexes][0] = dx+size2;
			tess.xyz[tess.numVertexes][1] = dy-size2;
			tess.texCoords[tess.numVertexes][0][0] = 1.f;
			tess.texCoords[tess.numVertexes][0][1] = 0.f;
			tess.vertexColors[tess.numVertexes][0] = (iColor[0]*brightness1[n])>>8;
			tess.vertexColors[tess.numVertexes][1] = (iColor[1]*brightness2[n])>>8;
			tess.vertexColors[tess.numVertexes][2] = (iColor[2]*brightness3[n])>>8;
			tess.vertexColors[tess.numVertexes][3] = alphcal;
			tess.numVertexes++;

			tess.indexes[tess.numIndexes++] = 0+ind;
			tess.indexes[tess.numIndexes++] = 1+ind;
			tess.indexes[tess.numIndexes++] = 2+ind;
			tess.indexes[tess.numIndexes++] = 0+ind;
			tess.indexes[tess.numIndexes++] = 2+ind;
			tess.indexes[tess.numIndexes++] = 3+ind;

			ind+=4;


		}
	}

	float drak;

	if(f->ftype == 5 || f->ftype == 6 || f->ftype == 7 || f->ftype == 166) {

		// renders special atlas lens flare like fuzzy but not fuzzy
		int modess;
		if (f->ftype == 6) modess = 1;		// mono rings
		else if (f->ftype == 7 || f->ftype == 166) modess = 2;	// force normal colored rings
		else	modess = 0;				// colorable colored rings
		float cx, cy;
		float dx, dy;
		float size2;
		float poses[]=	{-1.0f, -0.75f, -0.64f, -0.57f, -0.37f, -0.35f, -0.3f, 1.2f, -.21f, 0.15f, .38f, .56f, .52f, 0.6f, 1.2f, 1.37f};
		float sizes[]=	{1.15f, 0.7f, 0.2f, 0.35f, 0.24f, .86f, .357f, 2.3f, 0.15f, 0.09f, 0.21f, 0.7f, 0.37f, 	0.23f, 0.3f, 1.2f};
		float atlases[]= {4, 2, 1, 2, 2, 2, 2, 1, 1, 1, 2, 7, 8, 2, 3, 2};

		float downsize1 = 0.25f;
		float downsize2 = 0.25f;

		int brightness1[]=	{16, 5, 6, 18, 18, 38, 18, 12, 	24, 24, 18, 3, 3, 0, 12, 12};
		int brightness2[]=	{16, 32, 8, 17, 17, 37, 17, 11, 28, 28, 17, 3, 3, 0, 12, 10};
		int brightness3[]=	{27, 3, 24, 0, 0, 17, 0, 4, 28, 28, 0, 	17, 12, 12, 10, 10};


		int n;
		vec3_t	colarz;
		cx=backEnd.viewParms.viewportX+(backEnd.viewParms.viewportWidth>>1);
		cy=backEnd.viewParms.viewportY+(backEnd.viewParms.viewportHeight>>1);


		for(n=0; n<16; n++) {
			dx=(f->windowX-cx)*poses[n]+cx;
			dy=(f->windowY-cy)*poses[n]+cy;
			size2=sizes[n]*flaredsize2*.25f;

			drak = f->radius * 0.07;
			if (atlases[n] == 1) {
				downsize1 = 1;
				downsize2 = 1;
			};
			if (atlases[n] == 3) {
				downsize1 = 1;
				downsize2 = -1;
			};
			if (atlases[n] == 4) {
				downsize1 = -1;
				downsize2 = -1;
			};
			if (atlases[n] == 2) {
				downsize1 = -1;
				downsize2 = 1;
			};


			if (modess == 1) {
				brightness1[n] = brightness1[n] + brightness2[n] + brightness3[n] * 0.0100;
				brightness2[n] = brightness1[n];
				brightness3[n] = brightness1[n];
			}
			brightness1[n]=(int)(brightness1[n]*r_lensReflectionBrightness->value) * drak;
			brightness2[n]=(int)(brightness2[n]*r_lensReflectionBrightness->value) * drak;
			brightness3[n]=(int)(brightness3[n]*r_lensReflectionBrightness->value) * drak;

			if (modess == 2) {
				iColor[0] = 32.0f;
				iColor[1] = 32.0f;
				iColor[2] = 32.0f;
			}

			if (pvrhack) {
				if (modess == 2) {
					colarz[0] = ceil(iColor[0]*brightness1[n]);
					colarz[1] = ceil(iColor[1]*brightness2[n]);
					colarz[2] = ceil(iColor[2]*brightness3[n]);
				}
				else {
					colarz[0] = ceil(iColor[0]);
					colarz[1] = ceil(iColor[1]);
					colarz[2] = ceil(iColor[2]);
				}

				alphcal=r_lensReflectionBrightness->value * drak * 56;
			}
			else

			{
				colarz[0] = (iColor[0]*brightness1[n])>>8;
				colarz[1] = (iColor[1]*brightness2[n])>>8;
				colarz[2] = (iColor[2]*brightness3[n])>>8;
			}

			tess.xyz[tess.numVertexes][0] = dx-size2;
			tess.xyz[tess.numVertexes][1] = dy-size2;
			tess.texCoords[tess.numVertexes][0][0] = 0.f;
			tess.texCoords[tess.numVertexes][0][1] = 0.f;
			tess.vertexColors[tess.numVertexes][0] = colarz[0];
			tess.vertexColors[tess.numVertexes][1] = colarz[1];
			tess.vertexColors[tess.numVertexes][2] = colarz[2];
			tess.vertexColors[tess.numVertexes][3] = alphcal;
			tess.numVertexes++;

			tess.xyz[tess.numVertexes][0] = dx-size2;
			tess.xyz[tess.numVertexes][1] = dy+size2;
			tess.texCoords[tess.numVertexes][0][0] = 0.f;
			tess.texCoords[tess.numVertexes][0][1] = 0.5f  * downsize2;
			tess.vertexColors[tess.numVertexes][0] = colarz[0];
			tess.vertexColors[tess.numVertexes][1] = colarz[1];
			tess.vertexColors[tess.numVertexes][2] = colarz[2];
			tess.vertexColors[tess.numVertexes][3] = alphcal;
			tess.numVertexes++;

			tess.xyz[tess.numVertexes][0] = dx+size2;
			tess.xyz[tess.numVertexes][1] = dy+size2;
			tess.texCoords[tess.numVertexes][0][0] = 0.5f  * downsize1;
			tess.texCoords[tess.numVertexes][0][1] = 0.5f  * downsize2;
			tess.vertexColors[tess.numVertexes][0] = colarz[0];
			tess.vertexColors[tess.numVertexes][1] = colarz[1];
			tess.vertexColors[tess.numVertexes][2] = colarz[2];
			tess.vertexColors[tess.numVertexes][3] = alphcal;
			tess.numVertexes++;

			tess.xyz[tess.numVertexes][0] = dx+size2;
			tess.xyz[tess.numVertexes][1] = dy-size2;
			tess.texCoords[tess.numVertexes][0][0] = 0.5f * downsize1;
			tess.texCoords[tess.numVertexes][0][1] = 0.f;
			tess.vertexColors[tess.numVertexes][0] = colarz[0];
			tess.vertexColors[tess.numVertexes][1] = colarz[1];
			tess.vertexColors[tess.numVertexes][2] = colarz[2];
			tess.vertexColors[tess.numVertexes][3] = alphcal;
			tess.numVertexes++;


			tess.indexes[tess.numIndexes++] = 0+ind;
			tess.indexes[tess.numIndexes++] = 1+ind;
			tess.indexes[tess.numIndexes++] = 2+ind;
			tess.indexes[tess.numIndexes++] = 0+ind;
			tess.indexes[tess.numIndexes++] = 2+ind;
			tess.indexes[tess.numIndexes++] = 3+ind;

			ind+=4;


		}
	}

	if(f->ftype == 8 ) {

		// renders anamorphic flare
		// JUST LIKE TEH MOVEEZ!!!!!!!!!
		float cx, cy;
		float dx, dy;
		float size2;
		float size3;
		const float poses[]=	{0.9f, 1.0f, 1.08f};
		const float sizes[]=	{1.2f, 6.0f, 4.0f};
		const float sizes2[]=	{1.2f, 0.14f, 0.2f};
		int brightness1[]=	{16, 8,  5};	// red
		int brightness2[]=	{23, 21, 11};	// green
		int brightness3[]=	{30, 52, 32};	// blue
		int n;
		cx=backEnd.viewParms.viewportX+(backEnd.viewParms.viewportWidth>>1);
		cy=backEnd.viewParms.viewportY+(backEnd.viewParms.viewportHeight>>1);

		drak = f->radius * 0.02;
		for(n=0; n<3; n++) {
			dx=(f->windowX-cx)*poses[n]+cx;
			dy=(f->windowY-cy)*poses[n]+cy;
			size2=sizes[n]*flaredsize2 * drak*.25f;
			size3=sizes2[n]*flaredsize2 * (drak) *.25f;

			brightness1[n]=(int)(brightness1[n]*6 *r_lensReflectionBrightness->value);
			brightness2[n]=(int)(brightness2[n]*6 *r_lensReflectionBrightness->value);
			brightness3[n]=(int)(brightness3[n]*6 *r_lensReflectionBrightness->value);

			tess.xyz[tess.numVertexes][0] = dx-size2;
			tess.xyz[tess.numVertexes][1] = dy-size3;
			tess.texCoords[tess.numVertexes][0][0] = 0.f;
			tess.texCoords[tess.numVertexes][0][1] = 0.f;
			tess.vertexColors[tess.numVertexes][0] = (iColor[0]*brightness1[n])>>8;
			tess.vertexColors[tess.numVertexes][1] = (iColor[1]*brightness2[n])>>8;
			tess.vertexColors[tess.numVertexes][2] = (iColor[2]*brightness3[n])>>8;
			tess.vertexColors[tess.numVertexes][3] = alphcal;
			tess.numVertexes++;

			tess.xyz[tess.numVertexes][0] = dx-size2;
			tess.xyz[tess.numVertexes][1] = dy+size3;
			tess.texCoords[tess.numVertexes][0][0] = 0.f;
			tess.texCoords[tess.numVertexes][0][1] = 1.f;
			tess.vertexColors[tess.numVertexes][0] = (iColor[0]*brightness1[n])>>8;
			tess.vertexColors[tess.numVertexes][1] = (iColor[1]*brightness2[n])>>8;
			tess.vertexColors[tess.numVertexes][2] = (iColor[2]*brightness3[n])>>8;
			tess.vertexColors[tess.numVertexes][3] = alphcal;
			tess.numVertexes++;

			tess.xyz[tess.numVertexes][0] = dx+size2;
			tess.xyz[tess.numVertexes][1] = dy+size3;
			tess.texCoords[tess.numVertexes][0][0] = 1.f;
			tess.texCoords[tess.numVertexes][0][1] = 1.f;
			tess.vertexColors[tess.numVertexes][0] = (iColor[0]*brightness1[n])>>8;
			tess.vertexColors[tess.numVertexes][1] = (iColor[1]*brightness2[n])>>8;
			tess.vertexColors[tess.numVertexes][2] = (iColor[2]*brightness3[n])>>8;
			tess.vertexColors[tess.numVertexes][3] = alphcal;
			tess.numVertexes++;

			tess.xyz[tess.numVertexes][0] = dx+size2;
			tess.xyz[tess.numVertexes][1] = dy-size3;
			tess.texCoords[tess.numVertexes][0][0] = 1.f;
			tess.texCoords[tess.numVertexes][0][1] = 0.f;
			tess.vertexColors[tess.numVertexes][0] = (iColor[0]*brightness1[n])>>8;
			tess.vertexColors[tess.numVertexes][1] = (iColor[1]*brightness2[n])>>8;
			tess.vertexColors[tess.numVertexes][2] = (iColor[2]*brightness3[n])>>8;
			tess.vertexColors[tess.numVertexes][3] = alphcal;
			tess.numVertexes++;

			tess.indexes[tess.numIndexes++] = 0+ind;
			tess.indexes[tess.numIndexes++] = 1+ind;
			tess.indexes[tess.numIndexes++] = 2+ind;
			tess.indexes[tess.numIndexes++] = 0+ind;
			tess.indexes[tess.numIndexes++] = 2+ind;
			tess.indexes[tess.numIndexes++] = 3+ind;

			ind+=4;


		}
	}
	RB_EndSurface();


}

/*
==================
RB_RenderFlares

Because flares are simulating an occular effect, they should be drawn after
everything (all views) in the entire frame has been drawn.

Because of the way portals use the depth buffer to mark off areas, the
needed information would be lost after each view, so we are forced to draw
flares after each view.

The resulting artifact is that flares in mirrors or portals don't dim properly
when occluded by something in the main view, and portal flares that should
extend past the portal edge will be overwritten.
==================
*/
void RB_RenderFlares (void)
{
	flare_t		*f;
	flare_t		**prev;
	qboolean	draw;

	if ( !r_flares->integer && !r_flaresDlight->integer ) {
		return;
	}

	if ( (backEnd.refdef.rdflags & RDF_NOWORLDMODEL)) 	return;		// leilei - don't draw flares in the UI. this prevents
	// a very very very very nasty error relating to the trace checks
	// leading to recursive startup in te OA3 menu

	if(r_flareCoeff->modified) {
		R_SetFlareCoeff();
		r_flareCoeff->modified = qfalse;
	}

	// Reset currentEntity to world so that any previously referenced entities
	// don't have influence on the rendering of these flares (i.e. RF_ renderer flags).
	backEnd.currentEntity = &tr.worldEntity;
	backEnd.or = backEnd.viewParms.world;

	if (r_flaresDlight->integer)
		RB_AddDlightFlares();

	// perform z buffer readback on each flare in this view
	draw = qfalse;
	prev = &r_activeFlares;
	while ( ( f = *prev ) != NULL ) {
		// throw out any flares that weren't added last frame
		if ( f->addedFrame < backEnd.viewParms.frameCount - 1 ) {
			*prev = f->next;
			f->next = r_inactiveFlares;
			r_inactiveFlares = f;
			continue;
		}

		// don't draw any here that aren't from this scene / portal
		f->drawIntensity = 0;
		if ( f->frameSceneNum == backEnd.viewParms.frameSceneNum
		        && f->inPortal == backEnd.viewParms.isPortal ) {
			if (r_flareQuality->integer > 4)		// highest flare quality - only frequent readpixels, no trace
				RB_TestFlare( f, 0 );
			else if (r_flareQuality->integer == 4)		// high flare quality - frequent readpixels, trace
				RB_TestFlare( f, 1 );
			else if (r_flareQuality->integer == 3)		// medium flare quality - delayed readpixels, no trace (faster for some cpus than 2)
				RB_TestFlareFast( f, 0 );
			else if (r_flareQuality->integer == 2)		// low flare quality - delayed readpixels, trace
				RB_TestFlareFast( f, 1 );
			else if (r_flareQuality->integer == 1)		// lower flare quality - no readpixels, trace
				RB_TestFlareTraceOnly( f );
			else
				RB_TestFlareFast( f, 1 );		// lowest is actually a different surface flare defined elsewhere

			if ( f->drawIntensity ) {
				draw = qtrue;
			}
			else {
				// this flare has completely faded out, so remove it from the chain
				*prev = f->next;
				f->next = r_inactiveFlares;
				r_inactiveFlares = f;
				continue;
			}
		}

		prev = &f->next;
	}

	if ( !draw ) {
		return;		// none visible
	}

	if ( backEnd.viewParms.isPortal ) {
		qglDisable (GL_CLIP_PLANE0);
	}

	qglPushMatrix();
	qglLoadIdentity();
	qglMatrixMode( GL_PROJECTION );
	qglPushMatrix();
	qglLoadIdentity();
	qglOrtho( backEnd.viewParms.viewportX, backEnd.viewParms.viewportX + backEnd.viewParms.viewportWidth,
	          backEnd.viewParms.viewportY, backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight,
	          -99999, 99999 );

	for ( f = r_activeFlares ; f ; f = f->next ) {
		if ( f->frameSceneNum == backEnd.viewParms.frameSceneNum
		        && f->inPortal == backEnd.viewParms.isPortal
		        && f->drawIntensity ) {
			RB_RenderFlare( f );
		}
	}

	qglPopMatrix();
	qglMatrixMode( GL_MODELVIEW );
	qglPopMatrix();
}



void RB_DrawSunFlare( void )
{
	float		size;
	float		dist;
	vec3_t		origin, vec1, vec2;
	vec3_t		temp;
	int	fetype;



	if ( !backEnd.skyRenderedThisView ) {
		return;
	}
	if ( !r_flareSun->integer ) {
		return;
	}

	// leilei - don't draw the flare if we ain't got the sun
	if ( !r_drawSun->integer ) {
		return;
	}

	// don't draw the flare if we don't define the sun
	if ( r_drawSun->integer == 1 && (!tr.sunOn) ) {
		return;
	}

	if ( backEnd.doneSunFlare)	// leilei - only do sun once
		return;

	if (backEnd.viewParms.isPortal) // leilei - do sun only not in the portal (keeps sun from disappearing when portal is onscreen)
		return;

	fetype = r_flareSun->integer;

	// if we specified a flare type for sunshader in the sky, then we use it.
	if (tr.sunOn && (r_drawSun->integer == 1)) {
		fetype = tr.sunFlare;
	}
	// but, if we don't want the sunflare (type is 0 - either on the r_drawSun 2 override or the shader) then we just forget it
	if (fetype == 0) return;

	qglLoadMatrixf( backEnd.viewParms.world.modelMatrix );
	qglTranslatef (backEnd.viewParms.or.origin[0], backEnd.viewParms.or.origin[1], backEnd.viewParms.or.origin[2]);

	dist = 	backEnd.viewParms.zFar / 1.75;		// div sqrt(3)
	size = dist * 0.4;

	VectorScale( tr.sunDirection, dist, origin );
	PerpendicularVector( vec1, tr.sunDirection );
	CrossProduct( tr.sunDirection, vec1, vec2 );

	VectorScale( vec1, size, vec1 );
	VectorScale( vec2, size, vec2 );

	// farthest depth range
	qglDepthRange( 1.0, 1.0 );


	{
		vec3_t	coll;

		coll[0]= 1.0;
		coll[1]= 1.0;
		coll[2]= 1.0;


		coll[0]=tr.sunLight[0]/64;
		coll[1]=tr.sunLight[1]/64;
		coll[2]=tr.sunLight[2]/64;


		int g;
		for (g=0; g<3; g++)
			if (coll[g] > 1) coll[g] = 1;

		VectorCopy( origin, temp );
		VectorSubtract( temp, vec1, temp );
		VectorAdd( temp, vec2, temp );

		VectorCopy( origin, sunorg );
		VectorSubtract( sunorg, backEnd.viewParms.or.origin, sunorg );
		VectorCopy( backEnd.viewParms.or.origin, sunorg );
		VectorAdd( origin, sunorg, sunorg );

		size = coll[0] + coll[1] + coll[2] * 805;

		RB_AddFlare( (void *)NULL, 0, sunorg, coll, NULL, size, fetype, 1.0f, 2);


	}

	// back to normal depth range
	qglDepthRange( 0.0, 1.0 );

	backEnd.doneSunFlare = qtrue;


}

