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

#include "tr_local.h"
#include "qpal.h"
#include "../qcommon/cm_local.h"
#include "../qcommon/cm_public.h"

//
// LEILEI'S PARTICLES!
//

/*

	TODO:
	- FIX the particles not spawning correctly
	- Refactor all the spawning functions into emitting structs
	- and then make it possible to get those externally scriptable 
	- Refactor all the four-phase particle color ramps into hex values (and decode them)
	- Preserve volume on spark effects
	- Quake particle effects
	- Atlases
	- Trail system 
	- Get dynamic lights working

*/


// partially ported out of cg_marks.c
// and heavily improved upon

#define BLOODRED	2
#define EMISIVEFADE	3
#define GREY75		4

#define BLOODRED	2
#define EMISIVEFADE	3
#define LFXSTUFF	1
#define LFXSHOCK	2
#define LFXSMOKE	3
#define LFXSPARK	4
#define LFXBURST	5
#define LFXLIQUID	6
#define LFXSTRAND	7
#define LFXTRAIL	8
#define LFXBUBBLE	9
#define	LFXQUAKE	10
#define	LFXDLIGHT	11


// color types

#define	P_LFX		1	// ramp
#define P_LFXSIMPLE	2	// solid color
#define P_INDEXED	3	// indexed color from qpal


#define	MAX_TRAIL_POINTS 8
#define	TRAIL_LENGTH_SPLIT 64

typedef struct particle_s {
	struct particle_s	*next;

	float		time;
	float		endtime;

	vec3_t		org;
	vec3_t		vel;
	vec3_t		accel;
	int			color;
	float		colorvel;
	float		alpha;
	float		alphavel;
	int			type;
	shader_t	*pshader;

	float		height;
	float		width;

	float		endheight;
	float		endwidth;

	float		start;
	float		end;

	float		startfade;
	qboolean	rotate;
	int			snum;

	qboolean	link;

	// Ridah
	int			shaderAnim;
	int			roll;

	int			accumroll;

	// leilei
	float		stretch;
	vec3_t		angle;
	vec3_t		avelocity;
	vec3_t		src;
	vec3_t		dest;
	float		airfriction;
	float 		rollfriction;
	int		qolor;		// quake palette color translation
	int		ramp;		// quake color ramping (rocket trails, explosion)

	float		cols[4][5];	// fading color cycle
	vec3_t		dir;		// angle, direction
	float 		rollvel;	// velocity of roll
	float		bounce;		// how much elasticity does it have
	vec3_t		torg[MAX_TRAIL_POINTS];	// extra origins to use for trails and bursts
	int		active_trail;	// how many trail points are active on this trail
	int		bubbleit;	// turn into a bubble when crossing into water.
	int		material;	// kind of material it is?

	float		die;		// quake translation

	int 		rendertype;	// particle type (spark, oriented, etc)
	int		colortype;	// color type (ramp, palette, singular)
	int			fogNum;
} particle_t;

// Emitters tell what to spawn when an effect is called

typedef struct pemit_s {
	int	type;
	int 	count;
} pemit_t;


// Specified effects will have names and emitters assigned so scripts can replace them
typedef struct peffect_s {

	pemit_t emit[12];		// an effect can have up to 12 emitters
} peffect_t;



void R_AddParticleToScene (particle_t *p, vec3_t org, float alpha);
void R_LetsBounce ( particle_t *p);

// Shaders

static shader_t	*addshock;
static shader_t	*subshock;
static shader_t	*modshock;
static shader_t	*alfshock;

static shader_t	*addball;
static shader_t	*subball;
static shader_t	*modball;
static shader_t	*alfball;

shader_t *CreateInternalLFXShaders( int who, int blend,  const char *shadername );

static shader_t	*blood1;
static shader_t	*blood2;

static shader_t	*watsplash;
static shader_t	*watburst;
static shader_t	*watbubble;

static shader_t	*fireball;

static shader_t	*addsmoke;
static shader_t	*subsmoke;
static shader_t	*modsmoke;
static shader_t	*alfsmoke;



#define MAX_ENTITIES 500


// more quake stuff


static int ramp1[8] = { 0x6f, 0x6d, 0x6b, 0x69, 0x67, 0x65, 0x63, 0x61 };
static int ramp2[8] = { 0x6f, 0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x68, 0x66 };
static int ramp3[8] = { 0x6d, 0x6b, 6, 5, 4, 3 };



typedef enum {
	P_NONE,
	P_WEATHER,
	P_FLAT,
	P_SMOKE,
	P_ROTATE,
	P_WEATHER_TURBULENT,
	P_ANIM,	// Ridah
	P_BAT,
	P_BLEED,
	P_FLAT_SCALEUP,
	P_FLAT_SCALEUP_FADE,
	P_WEATHER_FLURRY,
	P_SMOKE_IMPACT,
	P_BUBBLE,
	P_BUBBLE_TURBULENT,
	P_SPRITE,
	P_BEAM,	// leilei	- angle not calculated
	P_SPARK,	// leilei	- angle and length recalculated from velocity
	P_QUAKESTATIC,
	P_QUAKEGRAV,
	P_QUAKESLOWGRAV,
	P_QUAKEFIRE,
	P_QUAKEEXPLODE,
	P_QUAKEEXPLODE2,
	P_QUAKEBLOB,
	P_QUAKEBLOB2,
	P_QUAKE,
	P_DLIGHT
} particle_type_t;




#define	MAX_SHADER_ANIMS	32
#define	MAX_SHADER_ANIM_FRAMES	64

static int	numShaderAnims;
// done.

#define		PARTICLE_GRAVITY	40
#define		MAX_PARTICLES	2048 // 1000000	// we can do CRAZY amounts of particles. but let's be sensible

particle_t	*active_particles, *free_particles;
particle_t	particles[MAX_PARTICLES];
int		cl_numparticles = MAX_PARTICLES;

qboolean		initparticles = qfalse;
static vec3_t			pvforward, pvright, pvup;
static vec3_t			rforward, rright, rup;

static float			oldTime;


static float		THEtime;

// lazy struct hack to make quake code backporting quicker and painless

typedef struct {
	double time;
} imlazy_t;

static imlazy_t cl;

#define pt_grav P_QUAKEGRAV
#define pt_slowgrav P_QUAKESLOWGRAV
#define pt_static P_QUAKESTATIC
#define pt_explode P_QUAKEEXPLODE
#define pt_explode2 P_QUAKEEXPLODE2
#define pt_blob P_QUAKEBLOB
#define pt_blob2 P_QUAKEBLOB2
#define pt_fire P_QUAKEFIRE

void CM_Trace( trace_t *results, const vec3_t start, const vec3_t end, vec3_t mins, vec3_t maxs,
               clipHandle_t model, const vec3_t origin, int brushmask, int capsule, sphere_t *sphere );

static void	P_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
                     int skipNumber, int mask )
{

	CM_Trace(result, start, end, NULL, NULL, 0, start, mask, 0, NULL);
}

/*
===============
R_ClearParticles
===============
*/
void R_ClearParticles (void)
{
	int		i;

	memset( particles, 0, sizeof(particles) );

	free_particles = &particles[0];
	active_particles = NULL;

	for (i=0 ; i<cl_numparticles ; i++) {
		particles[i].next = &particles[i+1];
		particles[i].type = 0;
	}
	particles[cl_numparticles-1].next = NULL;

	oldTime = THEtime;


	numShaderAnims = i;

	initparticles = qtrue;
}

extern int			r_numdlights;

/*
=====================
R_AddParticleToScene
=====================
*/
void R_AddParticleToScene (particle_t *p, vec3_t org, float alpha)
{

	vec3_t		point;
	float		width;
	float		height;
	float		time, time2;
	float		ratio;
	int		fogNum = tess.fogNum;
	float		invratio;
	vec4_t		color;

	time = 0;
	invratio = 0; // shut up gcc

	VectorSet (color, p->cols[0][0], p->cols[1][0], p->cols[2][0]);
	THEtime = backEnd.refdef.time;

	// qarticle transitions
	cl.time = THEtime / 10; // quarticles



	time = THEtime- p->time;
	time2 = p->endtime - p->time;
	ratio = time / time2;

/*
	{
		byte fogFactors[3] = {255, 255, 255};
		if(tr.world && p->fogNum > 0 && p->fogNum < tr.world->numfogs) {
			tess.numVertexes = 1;
			VectorCopy(org, tess.xyz[0]);
			tess.fogNum = p->fogNum;

			RB_CalcModulateColorsByFog(fogFactors);

		}
	}
*/


	{
		// create a front rotating facing polygon
		// that can change colors
		// and........ something.
		vec3_t	rr, ru;
		vec3_t	rotate_ang;

		if (p->colortype == P_LFX) {
			// Ramp

			// hack to prevent that one long particle from going crazy in the world.
			if (p->startfade > THEtime) {
				p->bounce = 0;
				p->bubbleit = 0; //no cpu expensive checks
				return;
			}

			if (THEtime > p->startfade) {
				float inv1,inv2,inv3,inv4;
				invratio = 1 - ( (THEtime- p->startfade) / (p->endtime - p->startfade) );

				inv1 = invratio * 4.0;
				inv2 = invratio * 4.0 - 1;
				inv3 = invratio * 4.0 - 2;
				inv4 = invratio * 4.0 - 3;

				if (inv1 > 1.0f) inv1 = 1.0f;
				if (inv2 > 1.0f) inv2 = 1.0f;
				if (inv3 > 1.0f) inv3 = 1.0f;
				if (inv4 > 1.0f) inv4 = 1.0f;
				{
					int et;
					vec4_t	fcol;

					for(et=0; et<4; et++) {
						if (invratio < 0.25f) {
							fcol[et] = (p->cols[et][3] * inv1) + (p->cols[et][4] * (1 - inv1));
						}
						else if (invratio < 0.50f) {
							fcol[et] = (p->cols[et][2] * inv2) + (p->cols[et][3] * (1 - inv2));
						}
						else if (invratio < 0.75f) {
							fcol[et] = (p->cols[et][1] * inv3) + (p->cols[et][2] * (1 - inv3));
						}
						else {
							fcol[et] = (p->cols[et][0] * inv4) + (p->cols[et][1] * (1 - inv4));
						}
					}
					for(et=0; et<4; et++) {
						if (fcol[et]>1) {
							fcol[et]=1.0f;
						}
						if (fcol[et]<0) {
							fcol[et]=0.0f;
						}
					}
					color[0] = fcol[0];
					color[1] = fcol[1];
					color[2] = fcol[2];
					color[3] = fcol[3];
				}

			}

			if (invratio > 4) {
				invratio = 4;
			}
		}
		else if (p->colortype == P_INDEXED) {
			VectorSet(color, qpalette[p->qolor][0], qpalette[p->qolor][1], qpalette[p->qolor][2]);
			color[3] = 1.0f;
		}
		else {
			// simple
			invratio = 1;
			color[0] = p->cols[0][0];
			color[1] = p->cols[1][0];
			color[2] = p->cols[2][0];
			color[3] = p->alpha;

		}

		if (p->rendertype == LFXSHOCK) {
			// ORIENTED sprite - used for shockwaves, water waves, waves, etc.
			vec3_t	argles;
			vec3_t		right, up;
			int ind=0;
			vectoangles( p->dir, argles );
			AngleVectors ( argles, NULL, right, up);

			width = p->width + ( ratio * ( p->endwidth - p->width) );
			height = p->height + ( ratio * ( p->endheight - p->height) );

			if (p->roll) {
				vectoangles( p->dir, rotate_ang );
				rotate_ang[ROLL] += p->roll;
				AngleVectors ( rotate_ang, NULL, right, up);
			}




			RB_BeginSurface( p->pshader, fogNum );
			VectorMA (org, -height, right, point);
			VectorMA (point, -width, up, point);



			VectorCopy (point, tess.xyz[tess.numVertexes] );
			tess.texCoords[tess.numVertexes][0][0] = 0;
			tess.texCoords[tess.numVertexes][0][1] = 0;
			tess.vertexColors[tess.numVertexes][0] = 255 * color[0];
			tess.vertexColors[tess.numVertexes][1] = 255 * color[1];
			tess.vertexColors[tess.numVertexes][2] = 255 * color[2];
			tess.vertexColors[tess.numVertexes][3] = 255 * invratio;
			tess.numVertexes++;

			VectorMA( point, 2*height, up, point);
			VectorCopy (point, tess.xyz[tess.numVertexes]);
			tess.texCoords[tess.numVertexes][0][0] = 0;
			tess.texCoords[tess.numVertexes][0][1] = 1;
			tess.vertexColors[tess.numVertexes][0] = 255 * color[0];
			tess.vertexColors[tess.numVertexes][1] = 255 * color[1];
			tess.vertexColors[tess.numVertexes][2] = 255 * color[2];
			tess.vertexColors[tess.numVertexes][3] = 255 * invratio;
			tess.numVertexes++;


			VectorMA( point, 2*width, right, point );
			VectorCopy (point, tess.xyz[tess.numVertexes]);
			tess.texCoords[tess.numVertexes][0][0] = 1;
			tess.texCoords[tess.numVertexes][0][1] = 1;
			tess.vertexColors[tess.numVertexes][0] = 255 * color[0];
			tess.vertexColors[tess.numVertexes][1] = 255 * color[1];
			tess.vertexColors[tess.numVertexes][2] = 255 * color[2];
			tess.vertexColors[tess.numVertexes][3] = 255 * invratio;
			tess.numVertexes++;


			VectorMA( point, -2*height, up, point );
			VectorCopy (point, tess.xyz[tess.numVertexes]);
			tess.texCoords[tess.numVertexes][0][0] = 1;
			tess.texCoords[tess.numVertexes][0][1] = 0;
			tess.vertexColors[tess.numVertexes][0] = 255 * color[0];
			tess.vertexColors[tess.numVertexes][1] = 255 * color[1];
			tess.vertexColors[tess.numVertexes][2] = 255 * color[2];
			tess.vertexColors[tess.numVertexes][3] = 255 * invratio;
			tess.numVertexes++;

			tess.indexes[tess.numIndexes++] = 0;
			tess.indexes[tess.numIndexes++] = 1;
			tess.indexes[tess.numIndexes++] = 2;
			tess.indexes[tess.numIndexes++] = 0;
			tess.indexes[tess.numIndexes++] = 2;
			tess.indexes[tess.numIndexes++] = 3;

			ind+=4;
			RB_EndSurface();

		}

		else if (p->rendertype == LFXSPARK) {
			// STRETCHY SPARK sprite - used for sparks etc
			vec3_t	argles;
			vec3_t		right, up, fwd;
			vec3_t		line;
			float		len, begin, end;
			vec3_t		start, finish;
			int ind=0;
			vec3_t oldorgstretch;
			vectoangles( p->dir, argles );
			AngleVectors ( argles, NULL, right, up);

			// Set up the 'beam'


			oldorgstretch[0] = org[0] + ((p->vel[0] ) * 0.005);
			oldorgstretch[1] = org[1] + ((p->vel[1]) * 0.005);
			oldorgstretch[2] = org[2] + ((p->vel[2] ) * 0.005);


			VectorSubtract( org, oldorgstretch, fwd );

			len = VectorNormalize( fwd );
			//len = DotProduct(p->vel);// * (5 / (THEtime- oldTime)) ;
			len *= p->height + p->width;

			len *= 7;	// doesn't look good in low framerates :/
			begin = -len / 2;
			end = len / 2;



			// Set up the particle

			width = p->width + ( ratio * ( p->endwidth - p->width) );
			height = p->height + ( ratio * ( p->endheight - p->height) );


			VectorMA( org, begin, fwd, start );
			VectorMA( org, end, fwd, finish );

			line[0] = DotProduct( fwd, backEnd.refdef.viewaxis[1] );
			line[1] = DotProduct( fwd, backEnd.refdef.viewaxis[2] );

			VectorScale( backEnd.refdef.viewaxis[1], line[1], right );
			VectorMA( right, -line[0], backEnd.refdef.viewaxis[2], right );
			VectorNormalize( right );

			RB_BeginSurface( p->pshader, fogNum );

			VectorMA( finish, width, right, tess.xyz[tess.numVertexes] );
			tess.texCoords[tess.numVertexes][0][0] = 0;
			tess.texCoords[tess.numVertexes][0][1] = 0;
			tess.vertexColors[tess.numVertexes][0] = 255 * color[0];
			tess.vertexColors[tess.numVertexes][1] = 255 * color[1];
			tess.vertexColors[tess.numVertexes][2] = 255 * color[2];
			tess.vertexColors[tess.numVertexes][3] = 255 * invratio;
			tess.numVertexes++;

			VectorMA( finish, -width, right, tess.xyz[tess.numVertexes] );
			tess.texCoords[tess.numVertexes][0][0] = 0;
			tess.texCoords[tess.numVertexes][0][1] = 1;
			tess.vertexColors[tess.numVertexes][0] = 255 * color[0];
			tess.vertexColors[tess.numVertexes][1] = 255 * color[1];
			tess.vertexColors[tess.numVertexes][2] = 255 * color[2];
			tess.vertexColors[tess.numVertexes][3] = 255 * invratio;
			tess.numVertexes++;


			VectorMA( start, -width, right, tess.xyz[tess.numVertexes] );
			tess.texCoords[tess.numVertexes][0][0] = 1;
			tess.texCoords[tess.numVertexes][0][1] = 1;
			tess.vertexColors[tess.numVertexes][0] = 255 * color[0];
			tess.vertexColors[tess.numVertexes][1] = 255 * color[1];
			tess.vertexColors[tess.numVertexes][2] = 255 * color[2];
			tess.vertexColors[tess.numVertexes][3] = 255 * invratio;
			tess.numVertexes++;


			VectorMA( start, width, right, tess.xyz[tess.numVertexes] );
			tess.texCoords[tess.numVertexes][0][0] = 1;
			tess.texCoords[tess.numVertexes][0][1] = 0;
			tess.vertexColors[tess.numVertexes][0] = 255 * color[0];
			tess.vertexColors[tess.numVertexes][1] = 255 * color[1];
			tess.vertexColors[tess.numVertexes][2] = 255 * color[2];
			tess.vertexColors[tess.numVertexes][3] = 255 * invratio;
			tess.numVertexes++;

			tess.indexes[tess.numIndexes++] = 0;
			tess.indexes[tess.numIndexes++] = 1;
			tess.indexes[tess.numIndexes++] = 2;
			tess.indexes[tess.numIndexes++] = 0;
			tess.indexes[tess.numIndexes++] = 2;
			tess.indexes[tess.numIndexes++] = 3;

			ind+=4;
			RB_EndSurface();


			// center glow of softness?  Enable for 2002 particleset?

		}
		else if (p->rendertype == LFXBURST) {
			// STRETCHY BURST sprite - used for explosions
			// like spark but origin doesnt change. it just keeps stretching
			// from where it was spawned.
			vec3_t	argles;
			vec3_t		right, up, fwd;
			vec3_t		line;
			float		len, begin, end;
			vec3_t		start, finish;
			int ind=0;
			vec3_t oldorgstretch;
			vectoangles( p->dir, argles );
			AngleVectors ( argles, NULL, right, up);

			// Set up the 'beam'



			oldorgstretch[0] = p->torg[0][0];
			oldorgstretch[1] = p->torg[0][1];
			oldorgstretch[2] = p->torg[0][2];


			VectorSubtract( org, oldorgstretch, fwd );

			len = VectorNormalize( fwd );
			//len = DotProduct(p->vel);// * (5 / (THEtime- oldTime)) ;
			len *= p->height + p->width;

			len *= 1;	// doesn't look good in low framerates :/
			begin = 0;
			end = len;



			// Set up the particle

			width = p->width + ( ratio * ( p->endwidth - p->width) );
			height = p->height + ( ratio * ( p->endheight - p->height) );


			VectorMA( org, begin, fwd, start );
			VectorMA( org, end, fwd, finish );

			line[0] = DotProduct( fwd, backEnd.refdef.viewaxis[1] );
			line[1] = DotProduct( fwd, backEnd.refdef.viewaxis[2] );

			VectorScale( backEnd.refdef.viewaxis[1], line[1], right );
			VectorMA( right, -line[0], backEnd.refdef.viewaxis[2], right );
			VectorNormalize( right );

			RB_BeginSurface( p->pshader, fogNum );

			VectorMA( finish, width, right, tess.xyz[tess.numVertexes] );
			tess.texCoords[tess.numVertexes][0][0] = 0;
			tess.texCoords[tess.numVertexes][0][1] = 0;
			tess.vertexColors[tess.numVertexes][0] = 255 * color[0];
			tess.vertexColors[tess.numVertexes][1] = 255 * color[1];
			tess.vertexColors[tess.numVertexes][2] = 255 * color[2];
			tess.vertexColors[tess.numVertexes][3] = 255 * invratio;
			tess.numVertexes++;

			VectorMA( finish, -width, right, tess.xyz[tess.numVertexes] );
			tess.texCoords[tess.numVertexes][0][0] = 0;
			tess.texCoords[tess.numVertexes][0][1] = 1;
			tess.vertexColors[tess.numVertexes][0] = 255 * color[0];
			tess.vertexColors[tess.numVertexes][1] = 255 * color[1];
			tess.vertexColors[tess.numVertexes][2] = 255 * color[2];
			tess.vertexColors[tess.numVertexes][3] = 255 * invratio;
			tess.numVertexes++;


			VectorMA( start, -width, right, tess.xyz[tess.numVertexes] );
			tess.texCoords[tess.numVertexes][0][0] = 1;
			tess.texCoords[tess.numVertexes][0][1] = 1;
			tess.vertexColors[tess.numVertexes][0] = 255 * color[0];
			tess.vertexColors[tess.numVertexes][1] = 255 * color[1];
			tess.vertexColors[tess.numVertexes][2] = 255 * color[2];
			tess.vertexColors[tess.numVertexes][3] = 255 * invratio;
			tess.numVertexes++;


			VectorMA( start, width, right, tess.xyz[tess.numVertexes] );
			tess.texCoords[tess.numVertexes][0][0] = 1;
			tess.texCoords[tess.numVertexes][0][1] = 0;
			tess.vertexColors[tess.numVertexes][0] = 255 * color[0];
			tess.vertexColors[tess.numVertexes][1] = 255 * color[1];
			tess.vertexColors[tess.numVertexes][2] = 255 * color[2];
			tess.vertexColors[tess.numVertexes][3] = 255 * invratio;
			tess.numVertexes++;

			tess.indexes[tess.numIndexes++] = 0;
			tess.indexes[tess.numIndexes++] = 1;
			tess.indexes[tess.numIndexes++] = 2;
			tess.indexes[tess.numIndexes++] = 0;
			tess.indexes[tess.numIndexes++] = 2;
			tess.indexes[tess.numIndexes++] = 3;

			ind+=4;
			RB_EndSurface();
		}
	/*	else if (p->rendertype == LFXDLIGHT) {
			// not a sprite, this just makes a dynamic light
		//	RE_AddLightToScene  ( p->org, 333, 255, 255, 255 );
		}
	*/
		else if (p->rendertype == LFXTRAIL) {
			// STRETCHY TRAIL sprite - used for..... i dunno
			// like burst but splits into more trails when the certain length is achieved
			// TO RETURN
		}
		else {
			// VP PARALLEL sprite
			//	trace_t pt1, pt2, pt3, pt4;
			//	float avgfrac;
			int ind=0;
			width = p->width + ( ratio * ( p->endwidth - p->width) );
			height = p->height + ( ratio * ( p->endheight - p->height) );

			if (p->roll) {
				vectoangles( backEnd.refdef.viewaxis[0], rotate_ang );
				rotate_ang[ROLL] += p->roll;
				AngleVectors ( rotate_ang, NULL, rr, ru);
			}

			if (p->roll) {
				VectorMA (org, -height, ru, point);
				VectorMA (point, -width, rr, point);
			}
			else {
				VectorMA (org, -height, pvup, point);
				VectorMA (point, -width, pvright, point);
			}

			// Faded clipping test. we don't need dx10 after all :)


			RB_BeginSurface( p->pshader, fogNum );

			VectorCopy (point, tess.xyz[tess.numVertexes] );
			tess.texCoords[tess.numVertexes][0][0] = 0;
			tess.texCoords[tess.numVertexes][0][1] = 0;
			tess.vertexColors[tess.numVertexes][0] = 255 * color[0];
			tess.vertexColors[tess.numVertexes][1] = 255 * color[1];
			tess.vertexColors[tess.numVertexes][2] = 255 * color[2];
			tess.vertexColors[tess.numVertexes][3] = 255 * invratio;
			tess.numVertexes++;

			if (p->roll) {
				VectorMA (point, 2*height, ru, point);
			}
			else {
				VectorMA (point, 2*height, pvup, point);
			}
			VectorCopy (point, tess.xyz[tess.numVertexes] );
			tess.texCoords[tess.numVertexes][0][0] = 0;
			tess.texCoords[tess.numVertexes][0][1] = 1;
			tess.vertexColors[tess.numVertexes][0] = 255 * color[0];
			tess.vertexColors[tess.numVertexes][1] = 255 * color[1];
			tess.vertexColors[tess.numVertexes][2] = 255 * color[2];
			tess.vertexColors[tess.numVertexes][3] = 255 * invratio;
			tess.numVertexes++;

			if (p->roll) {
				VectorMA (point, 2*width, rr, point);
			}
			else {
				VectorMA (point, 2*width, pvright, point);
			}
			VectorCopy (point, tess.xyz[tess.numVertexes] );
			tess.texCoords[tess.numVertexes][0][0] = 1;
			tess.texCoords[tess.numVertexes][0][1] = 1;
			tess.vertexColors[tess.numVertexes][0] = 255 * color[0];
			tess.vertexColors[tess.numVertexes][1] = 255 * color[1];
			tess.vertexColors[tess.numVertexes][2] = 255 * color[2];
			tess.vertexColors[tess.numVertexes][3] = 255 * invratio;
			tess.numVertexes++;

			if (p->roll) {
				VectorMA (point, -2*height, ru, point);
			}
			else {
				VectorMA (point, -2*height, pvup, point);
			}
			VectorCopy (point, tess.xyz[tess.numVertexes] );
			tess.texCoords[tess.numVertexes][0][0] = 1;
			tess.texCoords[tess.numVertexes][0][1] = 0;
			tess.vertexColors[tess.numVertexes][0] = 255 * color[0];
			tess.vertexColors[tess.numVertexes][1] = 255 * color[1];
			tess.vertexColors[tess.numVertexes][2] = 255 * color[2];
			tess.vertexColors[tess.numVertexes][3] = 255 * invratio;
			tess.numVertexes++;

			tess.indexes[tess.numIndexes++] = 0;
			tess.indexes[tess.numIndexes++] = 1;
			tess.indexes[tess.numIndexes++] = 2;
			tess.indexes[tess.numIndexes++] = 0;
			tess.indexes[tess.numIndexes++] = 2;
			tess.indexes[tess.numIndexes++] = 3;

			ind+=4;
			RB_EndSurface();

		}
	}


	if (!p->pshader) {
		// force a shader.
		p->pshader = alfball;
		return;
	}



}



static float roll = 0.0;
#define MINe(p,q) ((p <= q) ? p : q)

particle_t* reallyactive;

void R_AddParticles (void)
{
	particle_t		*p, *next;
	float			alpha;
	float			time, time2;
	vec3_t			oldorg;	// leilei
	particle_t		*active, *tail;
	vec3_t			rotate_ang;

	float			frametime;
	float			time3;
	float			time1;
	float			dvel;
	float			grav;

	float f;

	if (!initparticles)
		R_ClearParticles ();

	// This is supposed to prevent excessive calculations, but it's buggy and not working properly and conflicts
	// with the hud, of all things


	// leilei - quake time stuf
	frametime = THEtime- oldTime;
	frametime *= 0.001;
	time3 = frametime * 15;
	time2 = frametime * 10; // 15;
	time1 = frametime * 5;
	//grav = frametime * -800;

	grav = PARTICLE_GRAVITY;
	dvel = 4*frametime;

	time = frametime;


	VectorCopy( backEnd.refdef.viewaxis[0], pvforward );
	VectorCopy( backEnd.refdef.viewaxis[1], pvright );
	VectorCopy( backEnd.refdef.viewaxis[2], pvup );

	vectoangles( backEnd.refdef.viewaxis[0], rotate_ang );
	roll += ((THEtime- oldTime) * 0.1) ;
	rotate_ang[ROLL] += (roll*0.9);
	AngleVectors ( rotate_ang, rforward, rright, rup);

	oldTime = THEtime;


	active = NULL;
	tail = NULL;

	// Intended to keep physics calculations under control, but it doens't work properly. :(
	//if ((!backEnd.doneParticles) && !(tr.refdef.rdflags & RDF_NOWORLDMODEL))
	for (p=active_particles ; p ; p=next) {
		next = p->next;
		//time = (THEtime- p->time)*0.001;
		VectorCopy(p->org, oldorg);
		// if we're told to die, it's most likely a quake particle.
		if (p->die) {
			int scal = 2;
			p->endtime = THEtime + (p->die);
			p->die = 0;
			p->height= scal;
			p->width= scal;
			p->endheight= scal;
			p->endwidth= scal;
			p->bounce = 0;
			p->bubbleit = 0;
			//	ri.Printf( PRINT_ALL, "boung!");
			p->colortype = P_INDEXED;
			p->alpha = 1;
			p->qolor = p->color;
			p->time = THEtime;
			p->startfade = THEtime;
			p->rendertype = LFXQUAKE;
			p->accel[0] = p->accel[1] = p->accel[2] = 0;
			p->pshader = alfball;
			//p->qarticle = 1;
		}



		if (p->rendertype != LFXQUAKE) {
			p->vel[0] += p->accel[0]*frametime;
			p->vel[1] += p->accel[1]*frametime;
			p->vel[2] += p->accel[2]*frametime;
		}
		else {
			p->vel[0] += frametime;
			p->vel[1] += frametime;
			p->vel[2] += frametime;
			int i;
			switch (p->type) {
			case pt_static:
				break;
			case pt_fire:
				p->ramp += time1;
				if (p->ramp >= 6)
					p->die = -1;
				else
					p->color = ramp3[(int)p->ramp];
				p->vel[2] += grav;
				break;

			case pt_explode:
				p->ramp += time2;
				if (p->ramp >=8)
					p->die = -1;
				else
					p->color = ramp1[(int)p->ramp];
				for (i=0 ; i<3 ; i++)
					p->vel[i] += p->vel[i]*dvel;
				p->vel[2] -= grav;
				break;

			case pt_explode2:
				p->ramp += time3;
				if (p->ramp >=8)
					p->die = -1;
				else
					p->color = ramp2[(int)p->ramp];
				for (i=0 ; i<3 ; i++)
					p->vel[i] -= p->vel[i]*frametime;
				p->vel[2] -= grav;
				break;

			case pt_blob:
				for (i=0 ; i<3 ; i++)
					p->vel[i] += p->vel[i]*dvel;
				p->vel[2] -= grav;
				break;

			case pt_blob2:
				for (i=0 ; i<2 ; i++)
					p->vel[i] -= p->vel[i]*dvel;
				p->vel[2] -= grav;
				break;

			case pt_grav:
				p->vel[2] -= grav * 20;
				break;

			case pt_slowgrav:
				p->accel[2] -= grav;
				break;
			}

			if (THEtime> p->endtime) {
				p->next = free_particles;
				free_particles = p;
				p->type = 0;
				p->color = 0;
				p->alpha = 0;
				p->qolor = 0;
				p->accel[0] = 0;
				p->accel[1] = 0;
				p->accel[2] = 0;
				p->rollvel = 0;
				p->rollfriction = 0;
				//p->qarticle = 0;
				p->cols[0][0] = 0;
				p->cols[1][0] = 0;
				p->cols[2][0] = 0;
				p->cols[3][0] = 0;
				p->cols[0][1] = 0;
				p->cols[1][1] = 0;
				p->cols[2][1] = 0;
				p->cols[3][1] = 0;
				p->cols[0][2] = 0;
				p->cols[1][2] = 0;
				p->cols[2][2] = 0;
				p->cols[3][2] = 0;
				p->cols[0][3] = 0;
				p->cols[1][3] = 0;
				p->cols[2][3] = 0;
				p->cols[3][3] = 0;
				p->cols[0][4] = 0;
				p->cols[1][4] = 0;
				p->cols[2][4] = 0;
				p->cols[3][4] = 0;
				p->airfriction = 0;
				p->bounce = 0;
				p->bubbleit = 0;
				p->material = 0;
				p->active_trail = 0;
				continue;
			}
		}
		oldorg[0] = p->org[0];
		oldorg[1] = p->org[1];
		oldorg[2] = p->org[2];
		p->org[0] += p->vel[0]*frametime;
		p->org[1] += p->vel[1]*frametime;
		p->org[2] += p->vel[2]*frametime;

		if (p->rendertype != LFXQUAKE) {
			if (alpha > 1.0)
				alpha = 1;
			if (p->rollfriction) {
				f = 1.0f - MINe(p->rollfriction * frametime, 1);
				p->rollvel *= f;
			}

			if (p->airfriction) {
				f = 1.0f - MINe(p->airfriction * frametime, 1);
				VectorScale(p->vel, f, p->vel);
			}
			p->roll += (p->rollvel*frametime);
			alpha = p->alpha + time*p->alphavel;

			// hack to prevent that one long particle from going crazy in the world.
			if (p->startfade > THEtime) {
				p->alpha = -1; // kill it
			}

			if (alpha <= 0) {
				// faded out
				p->next = free_particles;
				free_particles = p;
				p->type = 0;
				p->airfriction = 0;
				p->color = 0;
				p->alpha = 0;
				p->endtime = THEtime- 0.1;
				p->active_trail = 0;
				continue;
			}

			if (THEtime> p->endtime || p->alpha < 0.0f) {
				p->next = free_particles;
				free_particles = p;
				p->type = 0;
				p->color = 0;
				p->alpha = 0;
				p->qolor = 0;
				p->accel[0] = 0;
				p->accel[1] = 0;
				p->accel[2] = 0;
				p->rollvel = 0;
				p->rollfriction = 0;
				//p->qarticle = 0;
				p->cols[0][0] = 0;
				p->cols[1][0] = 0;
				p->cols[2][0] = 0;
				p->cols[3][0] = 0;
				p->cols[0][1] = 0;
				p->cols[1][1] = 0;
				p->cols[2][1] = 0;
				p->cols[3][1] = 0;
				p->cols[0][2] = 0;
				p->cols[1][2] = 0;
				p->cols[2][2] = 0;
				p->cols[3][2] = 0;
				p->cols[0][3] = 0;
				p->cols[1][3] = 0;
				p->cols[2][3] = 0;
				p->cols[3][3] = 0;
				p->cols[0][4] = 0;
				p->cols[1][4] = 0;
				p->cols[2][4] = 0;
				p->cols[3][4] = 0;
				p->airfriction = 0;
				p->bounce = 0;
				p->bubbleit = 0;
				p->material = 0;
				p->active_trail = 0;
				continue;
			}
		}


		p->next = NULL;
		if (!tail) {
			active = tail = p;
		}
		else {
			tail->next = p;
			tail = p;
		}



		// leilei - trail stuffs

		vectoangles( p->vel, p->angle );
		p->stretch = 34;
		p->src[0] = p->org[0];
		p->src[1] = p->org[1];
		p->src[2] = p->org[2];

		p->dest[0] = p->org[0] + p->vel[0] + p->accel[0];
		p->dest[1] = p->org[1] + p->vel[1] + p->accel[1];
		p->dest[2] = p->org[2] + p->vel[2] + p->accel[2];


		// leilei - bubble conversion
		if (p->bubbleit && p->type != LFXBUBBLE) {
			int contents = CM_PointContents( p->org, 0 );
			if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME ) ) {
				p->type = LFXBUBBLE;
				p->pshader = watbubble;
				p->airfriction = 2.0f;
				p->accel[0] = 0;
				p->accel[1] = 0;
				p->accel[2] = 66;
				p->cols[0][0] = 1;
				p->cols[0][1] = 1;
				p->cols[0][2] = 1;
				p->cols[1][0] = 1;
				p->cols[1][1] = 1;
				p->cols[1][2] = 1;
				p->cols[2][0] = 1;
				p->cols[2][1] = 1;
				p->cols[2][2] = 1;
				p->cols[3][0] = 1;
				p->cols[3][1] = 1;
				p->cols[3][2] = 1;
				p->endtime += 1000; // last it a wile
			}

		}
		if(p->type == LFXBUBBLE) {
			int contents = CM_PointContents( p->org, 0 );
			if ( contents | ( CONTENTS_WATER | CONTENTS_SLIME ) ) {
				p->endtime = THEtime;
			}
			else {
				p->accel[0] = (crandom() * 4 - 2);
				p->accel[1] = (crandom() * 4 - 2);
				p->accel[2] = 66;

			}
		}


		// leilei - bounce physics
		if (p->bounce > 0) {
			trace_t	trace;
			float dist;

			// Do the trace of truth
			P_Trace (&trace, oldorg, NULL, NULL, p->org, -1, CONTENTS_SOLID);
			{
				if (trace.fraction < 1) {
					VectorCopy(trace.endpos, p->org);	// particle where we've hit from
					if (p->bounce < 0) {
						// bounce -1 means remove on impact
						p->endtime = THEtime;
					}
					// anything else - bounce off solid
					dist = DotProduct(p->vel, trace.plane.normal) * -p->bounce;
					VectorMA(p->vel, dist, trace.plane.normal, p->vel);
					// lets roll
					//p->vel[2] = 7;
					if(p->bounce < 0.2f) {
						// kill!
						p->alpha = -5;
					}
				}
			}
		}

		//R_AddParticleToScene (p, p->org, alpha);


	}

	active_particles = active;

	reallyactive = active;

	backEnd.doneParticles = qtrue; // we did it!
}


void R_RenderParticles (void)
{
	particle_t		*p, *next;

	if (!initparticles)
		return;

	if (!backEnd.doneParticles)
		R_AddParticles();

	active_particles = reallyactive;

	for (p=active_particles ; p ; p=next) {
		next = p->next;
		R_AddParticleToScene (p, p->org, p->alpha);
	}


}




























//
// leilei's LFX particle system effects
//


void R_LFX_Blood (const vec3_t org, const vec3_t dir, float pressure)
{
	int i, j;
	int count = pressure * 4;
	particle_t	*p;

	for (i = 0; i < count; i++) {
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		p->time = THEtime;
		p->bubbleit = 2; // dissolve
		p->endtime = THEtime+ (1200 * (crandom() * 22));
		//p->startfade = p->endtime;
		//p->color = GREY75;
		p->rendertype = LFXSMOKE;
		p->alpha = 1.0f;
		p->alphavel = 2.0f;
		p->height = 1.5;
		p->width = 1.5;
		p->endheight = 1.5;
		p->endwidth = 1.5;

		p->cols[0][0] = p->cols[0][1] = p->cols[0][2] = p->cols[0][3] = p->cols[0][4] = 1.0;
		p->cols[1][0] = p->cols[1][1] = p->cols[1][2] = p->cols[1][3] = p->cols[1][4] = 0.0;
		p->cols[2][0] = p->cols[2][1] = p->cols[2][2] = p->cols[2][3] = p->cols[2][4] = 0.0;

		// Manage blending Functions
		p->pshader = addsmoke;

		p->colortype = P_LFX;


		VectorCopy(org, p->org);
		p->bounce = 1.1f;
		for (j = 0; j < 3; j++) {
			//p->org[j] = org[j] + ((rand() & (count/8)) - (count/16));
			p->org[j] = org[j] + ((crandom() * (count/16)) - (count/32));
			p->vel[j] = dir[j] * (i * 2.6);

			p->vel[j] *= 0.01;
		}

		p->accel[0] = p->accel[1] = p->accel[2] = 0;
		p->accel[2] = -(PARTICLE_GRAVITY*8);

		p->airfriction = 0;
		//	p->org[j] = org[j] + (rand()&((count))-(count));
		//	p->vel[j] =  dir[j]*(rand()*(count*0.5))-(count) * 0.7;
		//	p->vel[2] += (i / 5);

		//	}

	}


}


void R_LFX_Smoke (const vec3_t org, const vec3_t dir, float spread, float speed, vec4_t color1, vec4_t color2, vec4_t color3, vec4_t color4, vec4_t color5, int count, int duration, float scaleup, int blendfunc)
{
	int i, j;
	int cont = 50;
	particle_t	*p;

	cont = (44 / (count / 2 + 1)) + 1;

	for (i = 0; i < cont; i++) {
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		p->time = THEtime;

		p->endtime = THEtime+ duration;
		p->startfade = THEtime;

		p->rendertype = LFXSMOKE;
		p->alpha = 0.1f;
		p->alphavel = 0.0f;
		p->height = p->width = 1.0 * (count / 3);
		p->bubbleit = 0;
		p->endheight = p->height + scaleup;
		p->endwidth = p->width + scaleup;


		// Manage random roll and
		p->rotate = qtrue;
		p->roll = crandom()*179;

		p->cols[0][0] = color1[0];
		p->cols[1][0] = color1[1];
		p->cols[2][0] = color1[2];
		p->cols[3][0] = color1[3];

		p->cols[0][1] = color2[0];
		p->cols[1][1] = color2[1];
		p->cols[2][1] = color2[2];
		p->cols[3][1] = color2[3];

		p->cols[0][2] = color3[0];
		p->cols[1][2] = color3[1];
		p->cols[2][2] = color3[2];
		p->cols[3][2] = color3[3];

		p->cols[0][3] = color4[0];
		p->cols[1][3] = color4[1];
		p->cols[2][3] = color4[2];
		p->cols[3][3] = color4[3];

		p->cols[0][4] = color5[0];
		p->cols[1][4] = color5[1];
		p->cols[2][4] = color5[2];
		p->cols[3][4] = color5[3];

		// Manage blending Functions
		if (blendfunc == 1)
			p->pshader = addsmoke;
		else if (blendfunc == 2)
			p->pshader = modsmoke;
		else if (blendfunc == 3)
			p->pshader = subsmoke;
		else if (blendfunc == 8)
			p->pshader = fireball;
		else
			p->pshader = alfsmoke;

		p->colortype = P_LFX;

		VectorCopy(org, p->org);

		// Manage spread of origin and velocity
		for (j = 0; j < 3; j++) {
			//p->org[j] = org[j] + ((crandom() * (spread / 8)) - (spread/16));
			p->org[j] = org[j];

			p->vel[j] = (crandom() * dir[j]) * speed;
			p->vel[j] += ((crandom() * (spread)) - (spread/2));
		}

		p->airfriction = 1.6f;
		p->bounce = 1.7f;
		p->rollvel = crandom() * 40 - 20;
		p->rollfriction = 0.7;
	}
}




void R_LFX_Smoke2 (const vec3_t org, const vec3_t dir, float spread, float speed, vec4_t color1, vec4_t color2, vec4_t color3, vec4_t color4, vec4_t color5, int count, int duration, float scale, float scaleup, int blendfunc)
{
	int i, j;
	int cont = 50;
	particle_t	*p;

	cont = count;

	p = 0; // die warnings

	for (i = 0; i < cont; i++) {
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		p->time = THEtime;

		p->endtime = THEtime+ duration;
		p->startfade = THEtime;
		p->bubbleit = 1;
		p->rendertype = LFXSMOKE;
		p->alpha = 0.1f;
		p->alphavel = 0.0f;
		p->height = p->width = (1.0 * scale);

		p->endheight = p->height + scaleup;
		p->endwidth = p->width + scaleup;


		// Manage random roll and
		p->rotate = qtrue;
		p->roll = crandom()*179;

		p->cols[0][0] = color1[0];
		p->cols[1][0] = color1[1];
		p->cols[2][0] = color1[2];
		p->cols[3][0] = color1[3];

		p->cols[0][1] = color2[0];
		p->cols[1][1] = color2[1];
		p->cols[2][1] = color2[2];
		p->cols[3][1] = color2[3];

		p->cols[0][2] = color3[0];
		p->cols[1][2] = color3[1];
		p->cols[2][2] = color3[2];
		p->cols[3][2] = color3[3];

		p->cols[0][3] = color4[0];
		p->cols[1][3] = color4[1];
		p->cols[2][3] = color4[2];
		p->cols[3][3] = color4[3];

		p->cols[0][4] = color5[0];
		p->cols[1][4] = color5[1];
		p->cols[2][4] = color5[2];
		p->cols[3][4] = color5[3];


		if (blendfunc == 1)
			p->pshader = addsmoke;
		else if (blendfunc == 2)
			p->pshader = modsmoke;
		else if (blendfunc == 3)
			p->pshader = subsmoke;
		else if (blendfunc == 8)
			p->pshader = fireball;
		else
			p->pshader = alfsmoke;

		p->colortype = P_LFX;

		VectorCopy(org, p->org);
		/*
			// Manage spread of origin and velocity
			 for (j = 0; j < 3; j++) {
				//p->org[j] = org[j] + ((crandom() * (spread / 8)) - (spread/16));
				p->org[j] = org[j];

				p->vel[j] = (crandom() * dir[j]) * speed;
				p->vel[j] += ((crandom() * (spread)) - (spread/2));
				}

			}
		*/
		// Manage spread of origin and velocity
		for (j = 0; j < 3; j++) {
			p->org[j] = org[j];
			p->vel[j] = (rand() & (int)spread) - (int)(spread * 0.5f);
			p->vel[j] += dir[j];
			p->vel[j] += (dir[j] * (rand() & (int)speed));

		}

		//p->rollvel = crandom() * (50 - 100)*DotProduct(p->vel);
		p->rollvel = crandom() * ((p->vel[0]+p->vel[1]+p->vel[2])/6) - ((p->vel[0]+p->vel[1]+p->vel[2])/3);
		p->rollfriction = 2;
		p->airfriction = 3.7f;
		p->accel[2] = (PARTICLE_GRAVITY*1.3);
		p->bounce = 3.5f;

		p->bounce = 0.0f; // bounce is slow...
	}
}


void R_LFX_Shock (const vec3_t org, const vec3_t dir, float spread, float speed, vec4_t color1, vec4_t color2, vec4_t color3, vec4_t color4, vec4_t color5, int count, int duration, float scaleup, int blendfunc)
{
	int i, j;
	particle_t	*p;


	for (i = 0; i < count; i++) {
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		p->time = THEtime;

		p->endtime = THEtime+ duration;
		p->startfade = THEtime;
		p->bubbleit = 0;
		p->rendertype = LFXSHOCK;

//	p->rendertype = BLOODRED;	// note - not actually bloodred.
		p->alpha = 0.1f;
		p->alphavel = 0.0f;
		//p->qolor = (color & ~7) + (rand() & 7);
		p->height = p->width = 1.0 * (count / 3);

		p->endheight = p->height + scaleup;
		p->endwidth = p->width + scaleup;


		// Manage random roll and
		p->rotate = qtrue;
		//p->rotate = qtrue;
		p->roll = crandom()*179;
		//p->roll = rand()%179;

		p->dir[0] = dir[0];
		p->dir[1] = dir[1];
		p->dir[2] = dir[2];

		p->cols[0][0] = color1[0];
		p->cols[1][0] = color1[1];
		p->cols[2][0] = color1[2];
		p->cols[3][0] = color1[3];

		p->cols[0][1] = color2[0];
		p->cols[1][1] = color2[1];
		p->cols[2][1] = color2[2];
		p->cols[3][1] = color2[3];

		p->cols[0][2] = color3[0];
		p->cols[1][2] = color3[1];
		p->cols[2][2] = color3[2];
		p->cols[3][2] = color3[3];

		p->cols[0][3] = color4[0];
		p->cols[1][3] = color4[1];
		p->cols[2][3] = color4[2];
		p->cols[3][3] = color4[3];

		p->cols[0][4] = color5[0];
		p->cols[1][4] = color5[1];
		p->cols[2][4] = color5[2];
		p->cols[3][4] = color5[3];


		// Manage blending Functions
		if (blendfunc == 1)
			p->pshader = addshock;
		else if (blendfunc == 2)
			p->pshader = modshock;
		else if (blendfunc == 14)
			p->pshader = watsplash;
		else if (blendfunc == 3)
			p->pshader = subshock;
		else

			p->pshader = alfshock;
		p->colortype = P_LFX;

		VectorCopy(org, p->org);

		// Manage spread of origin and velocity
		for (j = 0; j < 3; j++) {
			//p->org[j] = org[j] + ((crandom() * (spread / 8)) - (spread/16));
			p->org[j] = org[j];

			p->vel[j] = (crandom() * dir[j]) * speed;
			p->vel[j] += ((crandom() * (spread)) - (spread/2));
		}

		// Manage the Air Friction hack.


		// for (j = 0; j < 3; j++) {
		//	p->accel[j] = (p->vel[j] * -1);
		//	}




		//p->accel[0] = p->accel[1] = p->accel[2] = 0;
		//p->accel[2] = -(PARTICLE_GRAVITY / 2);

		p->airfriction = 0;
	}
}


void R_LFX_Spark (const vec3_t org, const vec3_t dir, float spread, float speed, vec4_t color1, vec4_t color2, vec4_t color3, vec4_t color4, vec4_t color5, int count, int duration, float scaleup, int blendfunc)
{
	int i, j;
	int cont = 50;
	particle_t	*p;

	cont = count;

	for (i = 0; i < cont; i++) {
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		p->time = THEtime;

		p->endtime = THEtime+ duration;
		p->startfade = THEtime;

		p->rendertype = LFXSPARK;
		//p->color = EMISIVEFADE;
		p->alpha = 1.0f;
		p->alphavel = 0.0f;
		//p->qolor = (color & ~7) + (rand() & 7);
		p->height = p->width = scaleup;

		p->endheight = p->height;
		p->endwidth = p->width;


		p->rotate = qfalse; // sparks don't rotate
		p->roll = 0;	// sparks don't roll


		p->accel[0] =	p->accel[1] =	p->accel[2] = 0;

		p->bubbleit = 1;

		p->cols[0][0] = color1[0];
		p->cols[1][0] = color1[1];
		p->cols[2][0] = color1[2];
		p->cols[3][0] = color1[3];

		p->cols[0][1] = color2[0];
		p->cols[1][1] = color2[1];
		p->cols[2][1] = color2[2];
		p->cols[3][1] = color2[3];

		p->cols[0][2] = color3[0];
		p->cols[1][2] = color3[1];
		p->cols[2][2] = color3[2];
		p->cols[3][2] = color3[3];

		p->cols[0][3] = color4[0];
		p->cols[1][3] = color4[1];
		p->cols[2][3] = color4[2];
		p->cols[3][3] = color4[3];

		p->cols[0][4] = color5[0];
		p->cols[1][4] = color5[1];
		p->cols[2][4] = color5[2];
		p->cols[3][4] = color5[3];

		// Manage blending Functions
		if (blendfunc == 1)
			p->pshader = addball;
		else if (blendfunc == 2)
			p->pshader = modball;
		else if (blendfunc == 3)
			p->pshader = subball;
		else if (blendfunc == 666)
			p->pshader = blood1;
		else
			p->pshader = alfball;



		//p->pshader = cgs.media.whiteShader;
//

		p->colortype = P_LFX;

		VectorCopy(org, p->org);


		// Manage spread of origin and velocity
		for (j = 0; j < 3; j++) {
			p->org[j] = org[j];
			p->vel[j] = (rand() & (int)spread) - (int)(spread * 0.5f);
			p->vel[j] += dir[j];
			p->vel[j] += (dir[j] * (rand() & (int)speed));

		}

		/*
			 for (j = 0; j < 3; j++) {
				float sped = speed * (1 + crandom()) + (speed * 0.3f);
				//p->org[j] = org[j] + ((crandom() * (spread / 8)) - (spread/16));
				p->org[j] = org[j];

				p->vel[j] = (dir[j]) * sped;
				p->vel[j] += ((crandom() * (spread)) - (spread/2));

			//	p->vel[j] *= crandom();


				}
		*/
		// a little kick up
		p->vel[2] += (speed * 0.4f);



		p->accel[0] = p->accel[1] = p->accel[2] = 0;
		p->accel[2] = -(800 * 0.5f);	// TODO: insert proper gravity.

		// prepare the initial stretch frame

		p->airfriction = 1.6f;
		p->bounce = 1.6f;
		if (blendfunc == 666) {
			p->bounce = 666;	// instantly make a red decal and kill itself. for blood
			p->accel[2] = -(800);	// TODO: insert proper gravity.
		}
	}
//	VectorCopy(p->org,p->torg[0]); // a org to stretch from.

}




void R_LFX_Burst (const vec3_t org, const vec3_t dir, float spread, float speed, vec4_t color1, vec4_t color2, vec4_t color3, vec4_t color4, vec4_t color5, int count, int duration, float scaleup, int blendfunc)
{
	int i, j;
	int cont = 50;
	particle_t	*p;

	cont = count;

	for (i = 0; i < cont; i++) {
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;


		p->time = THEtime;

		p->endtime = THEtime+ duration;
		p->startfade = THEtime;

		p->rendertype = LFXBURST;
		//p->color = EMISIVEFADE;
		p->alpha = 1.0f;
		p->alphavel = 0.0f;
		//p->qolor = (color & ~7) + (rand() & 7);
		p->height = p->width = scaleup;

		p->endheight = p->height;
		p->endwidth = p->width;


		p->rotate = qfalse; // sparks don't rotate
		p->roll = 0;	// sparks don't roll


		p->accel[0] =	p->accel[1] =	p->accel[2] = 0;

		p->cols[0][0] = color1[0];
		p->cols[1][0] = color1[1];
		p->cols[2][0] = color1[2];
		p->cols[3][0] = color1[3];

		p->cols[0][1] = color2[0];
		p->cols[1][1] = color2[1];
		p->cols[2][1] = color2[2];
		p->cols[3][1] = color2[3];

		p->cols[0][2] = color3[0];
		p->cols[1][2] = color3[1];
		p->cols[2][2] = color3[2];
		p->cols[3][2] = color3[3];

		p->cols[0][3] = color4[0];
		p->cols[1][3] = color4[1];
		p->cols[2][3] = color4[2];
		p->cols[3][3] = color4[3];

		p->cols[0][4] = color5[0];
		p->cols[1][4] = color5[1];
		p->cols[2][4] = color5[2];
		p->cols[3][4] = color5[3];

		// Manage blending Functions
		if (blendfunc == 1)
			p->pshader = addball;
		else if (blendfunc == 2)
			p->pshader = modball;
		else if (blendfunc == 3)
			p->pshader = subball;
		else if (blendfunc == 7)
			p->pshader = watburst;
		else if (blendfunc == 666)
			p->pshader = blood2;
		else
			p->pshader = alfball;



		p->colortype = P_LFX;

		VectorCopy(org, p->org);


		// Manage spread of origin and velocity
		for (j = 0; j < 3; j++) {
			float sped = speed * (1 + crandom()) + (speed * 0.3f);
			p->vel[j] =  ((crandom() * (spread)) - (spread/2)) / 360;
			p->vel[j] *= sped;

		}

		p->accel[0] = p->accel[1] = p->accel[2] = 0;

		// prepare the initial stretch frame

		p->airfriction = 1.6f;
		//p->bounce = 2.0f;
		VectorCopy(p->org,p->torg[0]); // a org to stretch from.

		if (blendfunc == 666) {
			p->airfriction = 2.9f;
			//	p->bounce = 1.3f;
			p->accel[2] = -(PARTICLE_GRAVITY/2);

		}
		// for water splashes
		if (blendfunc == 7) {
			p->airfriction = 1.8f;
			p->bounce = 0.0f;
			p->vel[0] = dir[0];
			p->vel[1] = dir[1];
			p->vel[2] = dir[2];
			p->height = 2;
			p->width = 2;
			p->endheight = 8;
			p->endwidth = 8;
			p->accel[2] = -(PARTICLE_GRAVITY);

		}

	}
}



void R_LFX_Dlight (const vec3_t org, const vec3_t dir, float spread, float speed, vec4_t color1, vec4_t color2, vec4_t color3, vec4_t color4, vec4_t color5, int count, int duration, float scaleup, int blendfunc)
{
	particle_t	*p;
	if (!free_particles)
		return;
	p = free_particles;
	free_particles = p->next;
	p->next = active_particles;
	active_particles = p;

	p->time = THEtime;
	p->endtime = THEtime+ duration;
	p->startfade = THEtime;
	p->rendertype = LFXDLIGHT;
	//p->color = EMISIVEFADE;
	p->alpha = 1.0f;
	p->alphavel = 0.0f;
	//p->qolor = (color & ~7) + (rand() & 7);
	p->height = p->width = scaleup;
	p->endheight = p->height;
	p->endwidth = p->width;
	p->rotate = qfalse; 
	p->roll = 0;
	p->accel[0] =	p->accel[1] =	p->accel[2] = 0;
	p->cols[0][0] = color1[0];
	p->cols[1][0] = color1[1];
	p->cols[2][0] = color1[2];
	p->cols[3][0] = color1[3];
	p->cols[0][1] = color2[0];
	p->cols[1][1] = color2[1];
	p->cols[2][1] = color2[2];
	p->cols[3][1] = color2[3];
	p->cols[0][2] = color3[0];
	p->cols[1][2] = color3[1];
	p->cols[2][2] = color3[2];
	p->cols[3][2] = color3[3];
	p->cols[0][3] = color4[0];
	p->cols[1][3] = color4[1];
	p->cols[2][3] = color4[2];
	p->cols[3][3] = color4[3];
	p->cols[0][4] = color5[0];
	p->cols[1][4] = color5[1];
	p->cols[2][4] = color5[2];
	p->cols[3][4] = color5[3];
	p->colortype = P_LFX;
	VectorCopy(org, p->org);
	p->accel[0] = p->accel[1] = p->accel[2] = 0;
	p->vel[0] = p->vel[1] = p->vel[2] = 0;
	// prepare the initial stretch frame
	p->airfriction = 0;
	p->bounce = 0;
	
}



// i'll probably fail, but still. GENERIC FUNCTION
void R_LFX_Generic (int type, vec3_t org, vec3_t dir, float alpha, int spread, int orgoff, float randroll, float speed, int cred, int cgreen, int cblue, int count, int duration, float scale, float scaleup, float bounce, float airfriction, float grav, float rollfriction, shader_t *shader)
{
	int i, j;
	int cont = 50;
	particle_t	*p;

	cont = count;

	p = 0; // die warnings

	for (i = 0; i < cont; i++) {
		if (!free_particles) {
			return;
		}
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		p->time = THEtime;
		p->endtime = THEtime+ duration;
		p->startfade = THEtime;
		p->bubbleit = 0;	// don't do bubble check for any generic particles.
		p->rendertype = type;
		p->alpha = alpha;
		p->alphavel = 0.0f;
		p->height = p->width = (1.0 * scale);

		p->endheight = p->height + scaleup;
		p->endwidth = p->width + scaleup;

		p->rotate = qtrue;
		p->roll = 0;//crandom()*randroll;


		// we only use one color for this fine generic function

		p->cols[0][0] = cred / 255;
		p->cols[1][0] = cgreen / 255;
		p->cols[2][0] = cblue / 255;
		p->cols[3][0] = 1.0f;

		p->cols[0][1] = cred / 255;
		p->cols[1][1] = cgreen / 255;
		p->cols[2][1] = cblue / 255;
		p->cols[3][1] = 1.0f;

		p->cols[0][2] = cred / 255;
		p->cols[1][2] = cgreen / 255;
		p->cols[2][2] = cblue / 255;
		p->cols[3][2] = 1.0f;

		p->cols[0][3] = cred / 255;
		p->cols[1][3] = cgreen / 255;
		p->cols[2][3] = cblue / 255;
		p->cols[3][3] = 1.0f;

		p->cols[0][4] = cred / 255;
		p->cols[1][4] = cgreen / 255;
		p->cols[2][4] = cblue / 255;
		p->cols[3][4] = 1.0f;


		p->pshader = shader;

		//p->rollvel = crandom() * (50 - 100)*DotProduct(p->vel);
		p->rollvel = crandom() * ((p->vel[0]+p->vel[1]+p->vel[2])/6) - ((p->vel[0]+p->vel[1]+p->vel[2])/3);
		p->rollfriction = rollfriction;
		p->airfriction = airfriction;
		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = (PARTICLE_GRAVITY*grav);
		p->bounce = bounce;


		p->colortype = P_LFXSIMPLE; // it's always an lfx for this one.

		VectorCopy(org, p->org);

		// Manage spread of origin and velocity
		for (j = 0; j < 3; j++) {
			p->org[j] = org[j] + ((rand() % orgoff) - (int)(orgoff * 0.5f));
			//p->org[j] = org[j];
			p->vel[j] = (rand() & (int)spread) - (int)(spread * 0.5f);
			p->vel[j] += dir[j];
			p->vel[j] += (dir[j] * (rand() & (int)speed));
		}
	}
}











// macros kind of




// give some turbulence to smokes
void R_LFX_PushSmoke (const vec3_t there, float force)
{
	// will reimplement properly later
}




void R_LetsBounce ( particle_t *p)
{

	// will reimplement properly later

}













// QUAKE EFFECTS


// attempt at generic particles function similar to quake...
void R_RunParticleEffect (const vec3_t org, const vec3_t dir, int color, int count)
{
	for (int i = 0; i < count; i++) {
		if (!free_particles)
			return;
		particle_t	*p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		p->die = cl.time + 0.1*(rand()%5);
		p->color = (color&~7) + (rand()&7);
		p->type = pt_slowgrav;
		for (int j=0 ; j<3 ; j++) {
			p->org[j] = org[j] + ((rand()&15)-8);
			p->vel[j] = dir[j]*15;// + (rand()%300)-150;
		}
	}
}


// also from Quake!

void R_QarticleExplosion(const vec3_t org)
{
	int i, j;
	particle_t *p;

	for (i = 0; i < 512; i++) {
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->rendertype = EMISIVEFADE;
		p->alpha = 1.0f;
		p->alphavel = 1.0f;
		p->time = THEtime;

		p->bubbleit = 0;
		// crap for q3's particle system...
		p->endtime = THEtime+ 500;
		p->startfade = p->endtime;
		p->height = 0.5;
		p->width = 0.5;
		p->endheight = 0.5;
		p->endwidth = 0.5;
		p->pshader = alfsmoke;
		p->qolor = ramp1[0];

//	p->qolor = 44;

		VectorCopy(org, p->org);

		p->ramp = rand() & 3;
		if (i & 1) {
//	    p->type = P_QUAKE;
			for (j = 0; j < 3; j++) {
				p->org[j] = org[j] + ((rand() % 32) - 16);
				p->vel[j] = (rand() % 512) - 256;
				p->accel[j] = p->vel[j] * 4;
			}
		}
		else {
//	    p->type = P_QUAKE;
			for (j = 0; j < 3; j++) {
				p->org[j] = org[j] + ((rand() % 32) - 16);
				p->vel[j] = (rand() % 512) - 256;
				p->accel[j] = p->vel[j] * 4;
			}
		}
	}
}


void Q_ParticleExplosion (const vec3_t org)
{
	int			i, j;
	particle_t	*p;

	for (i=0 ; i<1024 ; i++) {
		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->die = cl.time + 5;
		p->color = ramp1[0];
		p->ramp = rand()&3;
		if (i & 1) {
			p->type = pt_explode;
			for (j=0 ; j<3 ; j++) {
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (rand()%512)-256;
			}
		}
		else {
			p->type = pt_explode2;
			for (j=0 ; j<3 ; j++) {
				p->org[j] = org[j] + ((rand()%32)-16);
				p->vel[j] = (rand()%512)-256;
			}
		}
	}
}

void Q_RocketTrail (vec3_t start, vec3_t end, int type)
{
	vec3_t		vec;
	float		len;
	int			j;
	particle_t	*p;
	int			dec;
	static int	tracercount;

	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);
	if (type < 128)
		dec = 3;
	else {
		dec = 1;
		type -= 128;
	}

	while (len > 0) {
		len -= dec;

		if (!free_particles)
			return;
		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		VectorCopy (vec3_origin, p->vel);
		p->die = cl.time + 2;

		switch (type) {
		case 0:	// rocket trail
			p->ramp = (rand()&3);
			p->qolor = ramp3[(int)p->ramp];
			p->type = pt_fire;
			for (j=0 ; j<3 ; j++)
				p->org[j] = start[j] + ((rand()%6)-3);
			break;

		case 1:	// smoke smoke
			p->ramp = (rand()&3) + 2;
			p->color = ramp3[(int)p->ramp];
			p->type = pt_fire;
			for (j=0 ; j<3 ; j++)
				p->org[j] = start[j] + ((rand()%6)-3);
			break;

		case 2:	// blood
			p->type = pt_grav;
			p->color = 67 + (rand()&3);
			for (j=0 ; j<3 ; j++)
				p->org[j] = start[j] + ((rand()%6)-3);
			break;

		case 3:
		case 5:	// tracer
			p->die = cl.time + 0.5;
			p->type = pt_static;
			if (type == 3)
				p->color = 52 + ((tracercount&4)<<1);
			else
				p->color = 230 + ((tracercount&4)<<1);

			tracercount++;

			VectorCopy (start, p->org);
			if (tracercount & 1) {
				p->vel[0] = 30*vec[1];
				p->vel[1] = 30*-vec[0];
			}
			else {
				p->vel[0] = 30*-vec[1];
				p->vel[1] = 30*vec[0];
			}
			break;

		case 4:	// slight blood
			p->type = pt_grav;
			p->color = 67 + (rand()&3);
			for (j=0 ; j<3 ; j++)
				p->org[j] = start[j] + ((rand()%6)-3);
			len -= 3;
			break;

		case 6:	// voor trail
			p->color = 9*16 + 8 + (rand()&3);
			p->type = pt_static;
			p->die = cl.time + 0.3;
			for (j=0 ; j<3 ; j++)
				p->org[j] = start[j] + ((rand()&15)-8);
			break;
		}


		VectorAdd (start, vec, start);
	}
}


void LFX_ShaderInit ( void )
{

	addsmoke = R_FindShader( "psmoke-add", LIGHTMAP_NONE, qtrue );
	subsmoke = R_FindShader( "psmoke-sub", LIGHTMAP_NONE, qtrue );
	modsmoke = R_FindShader( "psmoke-mod", LIGHTMAP_NONE, qtrue );
	alfsmoke = R_FindShader("psmoke-blend",LIGHTMAP_NONE, qtrue );

	blood1 = R_FindShader( "pblood1", LIGHTMAP_NONE, qtrue );
	blood2 = R_FindShader( "pblood2", LIGHTMAP_NONE, qtrue );

	watsplash = R_FindShader( "pwatersplash", LIGHTMAP_NONE, qtrue );
	watburst  = R_FindShader( "pwaterburst", LIGHTMAP_NONE, qtrue );
	watbubble = R_FindShader( "pwaterbubble", LIGHTMAP_NONE, qtrue );
	fireball  = R_FindShader( "pfireball", LIGHTMAP_NONE, qtrue );

	addball = R_FindShader( "pball-add", LIGHTMAP_NONE, qtrue );
	subball = R_FindShader( "pball-sub", LIGHTMAP_NONE, qtrue );
	modball = R_FindShader( "pball-mod", LIGHTMAP_NONE, qtrue );
	alfball = R_FindShader( "pball-blend", LIGHTMAP_NONE, qtrue );


	addshock = R_FindShader( "pshock-add", LIGHTMAP_NONE, qtrue );
	subshock = R_FindShader( "pshock-sub", LIGHTMAP_NONE, qtrue );
	modshock = R_FindShader( "pshock-mod", LIGHTMAP_NONE, qtrue );
	alfshock = R_FindShader( "pshock-blend", LIGHTMAP_NONE, qtrue );

// fallback to builtin textures if these shaders failed to load.
// or allow simpler 32x32 textures anyway

	if (!addsmoke || !r_particlesTex->value) addsmoke = CreateInternalLFXShaders( 1, 1, "psmoke-add" );
	if (!alfsmoke || !r_particlesTex->value) alfsmoke = CreateInternalLFXShaders( 1, 0, "psmoke-blend" );

	if (!addshock || !r_particlesTex->value) addshock = CreateInternalLFXShaders( 1, 1, "pshock-add" );
	if (!alfshock || !r_particlesTex->value) alfshock = CreateInternalLFXShaders( 1, 0, "pshock-blend" );

	if (!watsplash || !r_particlesTex->value) watsplash = CreateInternalLFXShaders( 1, 1, "pwatsplash" );
	if (!watburst  || !r_particlesTex->value) watburst  = CreateInternalLFXShaders( 1, 1, "pwatburst"  );
	if (!watbubble || !r_particlesTex->value) watbubble = CreateInternalLFXShaders( 1, 1, "pwatbubble" );

	if (!addball || !r_particlesTex->value) addball = CreateInternalLFXShaders( 0, 1, "pball-add" );
	if (!alfball || !r_particlesTex->value) alfball = CreateInternalLFXShaders( 0, 0, "pball-blend" );

	if (!blood1 || !r_particlesTex->value) blood1 = CreateInternalLFXShaders( 1, 0, "pblood1" );	// smoke
	if (!blood2 || !r_particlesTex->value) blood2 = CreateInternalLFXShaders( 1, 0, "pblood2" );	// smoke
	if (!fireball || !r_particlesTex->value) fireball = CreateInternalLFXShaders( 1, 1, "pfireball" );

	R_ClearParticles ();
}

// 1996 - it's like the first game ok
// - just dots

void LFX_ParticleEffect1996 (int effect, const vec3_t org, const vec3_t dir)
{
	vec3_t notatall;
	memset(&notatall, 0, sizeof(notatall));

	// Smoke trails on grenades and rockets
	if (effect == 1) {
		R_RunParticleEffect (org, notatall, 10, 8);
		//Q_RocketTrail(org, dir, r_leidebug->integer);
	}

	// Bullet Hit
	if (effect == 2 || effect == 3) {
		R_RunParticleEffect (org, notatall, 6, 20);

	}

	// Grenade/Rocket/Prox Explosion

	else if (effect == 5 || effect == 4 || effect == 11) {
		Q_ParticleExplosion(org);

	}

	// Blood Sprays for bullets
	if (effect == 14) {
		R_RunParticleEffect (org, dir,21, 20);
	}


	// Water Splash for bullets
	if (effect == 19) {
		R_RunParticleEffect (org, dir,41, 20);
	}


}


// 200X - most games of that decade...
// - usually ltos of particles
// - usually
void LFX_ParticleEffect200X (int effect, const vec3_t org, const vec3_t dir)
{
	vec3_t origin, sprOrg, sprVel; // laziness
	vec4_t colory, colory2, colory3, colory4;

	VectorCopy(org, sprOrg);
	VectorCopy(org, origin);
	VectorCopy(dir, sprVel);

	// Smoke trails on grenades and rockets
	if (effect == 1) {
		colory[0] = 0.7;
		colory[1] = 0.7;
		colory[2] = 0.7;
		colory[3] = 1.0;
		colory2[0] = 0.5;
		colory2[1] = 0.5;
		colory2[2] = 0.5;
		colory2[3] = 0.8;
		colory3[0] = 0.5;
		colory3[1] = 0.4;
		colory3[2] = 0.3;
		colory3[3] = 0.5;
		colory4[0] = 0.5;
		colory4[1] = 0.4;
		colory4[2] = 0.3;
		colory4[3] = 0.0;
		R_LFX_Smoke2 (sprOrg, sprVel, 1 + (random()*3), 0.54+ (random()*8.7), colory, colory2, colory3, colory4, colory4, 1, 2470, 4, 8+ (random()*6), 4);
	}

	// Bullet Hit
	if (effect == 2) {
		colory[0] = 0.7;
		colory[1] = 0.7;
		colory[2] = 0.7;
		colory[3] = 0.0;
		colory2[0] = 0.7;
		colory2[1] = 0.7;
		colory2[2] = 0.7;
		colory2[3] = 0.5;
		colory3[0] = 0.6;
		colory3[1] = 0.6;
		colory3[2] = 0.6;
		colory3[3] = 0.8;
		colory4[0] = 0.6;
		colory4[1] = 0.6;
		colory4[2] = 0.6;
		colory4[3] = 0.0;

		VectorMA( origin, 4, dir, sprOrg );
		VectorScale( dir, 1, sprVel );
		// Sparks
		colory[0] = 1;
		colory[1] = 1;
		colory[2] = 1.0;
		colory[3] = 1.0;
		colory2[0] = 1;
		colory2[1] = 1;
		colory2[2] = 0.8;
		colory2[3] = 0.9;
		colory3[0] = 0.7;
		colory3[1] = 0.5;
		colory3[2] = 0.2;
		colory3[3] = 0.7;
		colory4[0] = 0.1;
		colory4[1] = 0.06;
		colory4[2] = 0.0;
		colory4[3] = 0.0;

		R_LFX_Burst (sprOrg, sprVel, 8366, 2, colory, colory2, colory3, colory4, colory4, 4, 50, 2, 1);


		VectorMA( origin, 1, dir, sprOrg );
		VectorScale( dir, 1, sprVel );
		R_LFX_Spark (sprOrg, sprVel, 95, 325, colory, colory2, colory3, colory4, colory4, 5, 140, 0.5f, 1);
		R_LFX_Spark (sprOrg, sprVel, 95, 85, colory, colory2, colory3, colory4, colory4, 1, 1540, 0.5f, 1);

		//R_LFX_PushSmoke (sprOrg, 44);


	}

	// Shotgun Hit
	if (effect == 3) {
		colory[0] = 0.7;
		colory[1] = 0.7;
		colory[2] = 0.7;
		colory[3] = 0.0;
		colory2[0] = 0.7;
		colory2[1] = 0.7;
		colory2[2] = 0.7;
		colory2[3] = 0.5;
		colory3[0] = 0.6;
		colory3[1] = 0.6;
		colory3[2] = 0.6;
		colory3[3] = 0.8;
		colory4[0] = 0.6;
		colory4[1] = 0.6;
		colory4[2] = 0.6;
		colory4[3] = 0.0;
		VectorMA( origin, 1, dir, sprOrg );
		VectorScale( dir, 7, sprVel );
		R_LFX_Smoke2 (sprOrg, sprVel, 2 + (random()*6), 6.54+ (random()*8.7), colory, colory2, colory3, colory4, colory4, 1, 800 + (random()*2000), 2, 8+ (random()*6), 0);
		R_LFX_Smoke2 (sprOrg, sprVel, 2 + (random()*6), 3.54+ (random()*8.7), colory, colory2, colory3, colory4, colory4, 1, 300 + (random()*2000), 2, 8+ (random()*6), 4);
		colory[0] = 0.9;
		colory[1] = 0.8;
		colory[2] = 1.0;
		colory[3] = 1.0;
		colory2[0] = 0.7;
		colory2[1] = 0.5;
		colory2[2] = 0.2;
		colory2[3] = 0.9;
		colory3[0] = 0.1;
		colory3[1] = 0.1;
		colory3[2] = 0.1;
		colory3[3] = 0.7;
		colory4[0] = 0.0;
		colory4[1] = 0.0;
		colory4[2] = 0.0;
		colory4[3] = 0.0;
		VectorMA( origin, 4, dir, sprOrg );
		VectorScale( dir, 64, sprVel );
		R_LFX_Shock (origin, dir, 0, 32, colory, colory2, colory3, colory4, colory4, 1, 200, 50,1);
		colory[0] = 1;
		colory[1] = 1;
		colory[2] = 1.0;
		colory[3] = 1.0;
		colory2[0] = 1;
		colory2[1] = 1;
		colory2[2] = 0.8;
		colory2[3] = 0.9;
		colory3[0] = 0.7;
		colory3[1] = 0.5;
		colory3[2] = 0.2;
		colory3[3] = 0.7;
		colory4[0] = 0.1;
		colory4[1] = 0.06;
		colory4[2] = 0.0;
		colory4[3] = 0.0;
		VectorMA( origin, 1, dir, sprOrg );
		VectorScale( dir, 1, sprVel );
		R_LFX_Spark (sprOrg, sprVel, 25, 85, colory, colory2, colory3, colory4, colory4, 3, 2540, 0.5f, 1);
	}

	// Plasma Hit
	if (effect == 6) {
		colory[0] = 1.0;
		colory[1] = 0.7;
		colory[2] =  1.0;
		colory[3] = 0.0;
		colory2[0] = 0.3;
		colory2[1] = 0.7;
		colory2[2] = 1.0;
		colory2[3] = 0.5;
		colory3[0] = 0.1;
		colory3[1] = 0.2;
		colory3[2] = 0.6;
		colory3[3] = 0.8;
		colory4[0] = 0.0;
		colory4[1] = 0.0;
		colory4[2] = 0.0;
		colory4[3] = 0.0;
		VectorMA( origin, 1, dir, sprOrg );
		VectorScale( dir, 7, sprVel );
		R_LFX_Smoke2 (sprOrg, sprVel, 2 + (random()*6), 6.54+ (random()*8.7), colory, colory2, colory3, colory4, colory4, 1, 800 + (random()*2000), 2, 8+ (random()*6), 0);
		VectorMA( origin, 2, dir, sprOrg );
		VectorScale( dir, 64, sprVel );
		R_LFX_Shock (sprOrg, dir, 0, 0, colory, colory2, colory3, colory4, colory4, 1,600, 80,1);
		VectorMA( origin, 1, dir, sprOrg );
		VectorScale( dir, 1, sprVel );
		R_LFX_Spark (sprOrg, sprVel, 25, 85, colory, colory2, colory3, colory4, colory4, 5, 5540, 0.5f, 1);
	}

	// Lightning Hit
	if (effect == 8) {
		colory[0] = 0.7;
		colory[1] = 0.7;
		colory[2] = 0.7;
		colory[3] = 0.0;
		colory2[0] = 0.7;
		colory2[1] = 0.7;
		colory2[2] = 0.7;
		colory2[3] = 0.5;
		colory3[0] = 0.6;
		colory3[1] = 0.6;
		colory3[2] = 0.6;
		colory3[3] = 0.8;
		colory4[0] = 0.6;
		colory4[1] = 0.6;
		colory4[2] = 0.6;
		colory4[3] = 0.0;
		VectorMA( origin, 1, dir, sprOrg );
		VectorScale( dir, 7, sprVel );
		R_LFX_Smoke2 (sprOrg, sprVel, 2 + (random()*6), 3.54+ (random()*8.7), colory, colory2, colory3, colory4, colory4, 1, 300 + (random()*2000), 2, 8+ (random()*6), 4);
	}


	// Grenade/Rocket/Prox Explosion

	else if (effect == 5 || effect == 4 || effect == 11) {


		colory[0] = 1.0;
		colory[1] = 1.0;
		colory[2] = 1.0;
		colory[3] = 1.0;
		colory2[0] = 1.0;
		colory2[1] = 1.0;
		colory2[2] = 0.5;
		colory2[3] = 0.9;
		colory3[0] = 0.7;
		colory3[1] = 0.3;
		colory3[2] = 0.1;
		colory3[3] = 0.7;
		colory4[0] = 0.0;
		colory4[1] = 0.0;
		colory4[2] = 0.0;
		colory4[3] = 0.0;
		VectorMA( origin, 4, dir, sprOrg );
		VectorScale( dir, 64, sprVel );

		R_LFX_Shock (sprOrg, dir, 0, 0, colory, colory2, colory3, colory4, colory4, 1, 500, 270, 1);

		// fireball

		colory[0] = 1.0;
		colory[1] = 0.2;
		colory[2] = 0.1;
		colory[3] = 0.0;
		colory2[0] = 0.5;
		colory2[1] = 0.0;
		colory2[2] = 0.0;
		colory2[3] = 0.2;
		colory3[0] = 0.1;
		colory3[1] = 0.0;
		colory3[2] = 0.0;
		colory3[3] = 0.7;
		colory4[0] = 0.0;
		colory4[1] = 0.0;
		colory4[2] = 0.0;
		colory4[3] = 0.0;
		VectorMA( origin, 12, dir, sprOrg );
		VectorScale( dir, 64, sprVel );

		R_LFX_Smoke (sprOrg, sprVel, 32, 3.54, colory, colory2, colory3, colory4, colory4, 16, 1000, 94, 1);

		colory[0] = 1.0;
		colory[1] = 1.0;
		colory[2] = 0.9;
		colory[3] = 1.0;
		colory2[0] = 1.0;
		colory2[1] = 0.7;
		colory2[2] = 0.2;
		colory2[3] = 0.9;
		colory3[0] = 0.3;
		colory3[1] = 0.2;
		colory3[2] = 0.1;
		colory3[3] = 0.7;
		colory4[0] = 0.0;
		colory4[1] = 0.0;
		colory4[2] = 0.0;
		colory4[3] = 0.0;

		R_LFX_Smoke (sprOrg, sprVel, 62, 2, colory, colory2, colory3, colory4, colory4, 12, 200,84, 8);
		R_LFX_Smoke (sprOrg, sprVel, 32, 1.54, colory, colory2, colory3, colory4, colory4, 22, 600, 74, 8);

		R_LFX_Smoke (sprOrg, sprVel, 44, 1.3, colory, colory2, colory3, colory4, colory4, 3, 800,3, 8);

		R_LFX_Smoke (sprOrg, sprVel, 32, 0.54, colory, colory2, colory3, colory4, colory4, 12, 600, 74, 8);

		// Shroom Cloud
		VectorMA( origin, 16, dir, sprOrg );
		VectorScale( dir, 64, sprVel );

		colory[0] = 0.5;
		colory[1] = 0.0;
		colory[2] = 0.0;
		colory[3] = 0.0;
		colory2[0] = 1.0;
		colory2[1] = 1.0;
		colory2[2] = 0.2;
		colory2[3] = 0.2;
		colory3[0] = 0.5;
		colory3[1] = 0.1;
		colory3[2] = 0.0;
		colory3[3] = 0.5;
		colory4[0] = 0.0;
		colory4[1] = 0.0;
		colory4[2] = 0.0;
		colory4[3] = 0.0;
		R_LFX_Smoke (sprOrg, sprVel, 3, 0.7, colory, colory2, colory3, colory4, colory4, 22, 300,135, 8);

		// Sparks!

		colory[0] = 1;
		colory[1] = 1;
		colory[2] = 1.0;
		colory[3] = 1.0;
		colory2[0] = 1;
		colory2[1] = 1;
		colory2[2] = 0.8;
		colory2[3] = 0.9;
		colory3[0] = 0.7;
		colory3[1] = 0.5;
		colory3[2] = 0.2;
		colory3[3] = 0.7;
		colory4[0] = 0.1;
		colory4[1] = 0.06;
		colory4[2] = 0.0;
		colory4[3] = 0.0;
		VectorMA( origin, 12, dir, sprOrg );
		VectorScale( dir, 64, sprVel );

		R_LFX_Burst (sprOrg, sprVel, 175, 15, colory, colory2, colory3, colory4, colory4, 15, 240, 22, 1);
		//R_LFX_Dlight (sprOrg, sprVel, 4000, 1, colory, colory2, colory3, colory4, colory4, 4, 50, 2, 1);
		R_LFX_Spark (sprOrg, sprVel, 175, 5, colory, colory2, colory3, colory4, colory4, 25, 1240, 0.8f, 1);
		RE_AddRippleToScene( origin, 300 );
	}

	// BFG
	else if (effect == 9) {
		colory[0] = 1.0;
		colory[1] = 1.0;
		colory[2] = 1.0;
		colory[3] = 1.0;
		colory2[0] = 0.3;
		colory2[1] = 1.0;
		colory2[2] = 0.2;
		colory2[3] = 0.9;
		colory3[0] = 0.0;
		colory3[1] = 0.3;
		colory3[2] = 0.1;
		colory3[3] = 0.7;
		colory4[0] = 0.0;
		colory4[1] = 0.0;
		colory4[2] = 0.0;
		colory4[3] = 0.0;
		VectorMA( origin, 4, dir, sprOrg );
		VectorScale( dir, 64, sprVel );

		R_LFX_Shock (sprOrg, dir, 0, 0, colory, colory2, colory3, colory4, colory4, 1, 300, 470, 1);

		// fireball


		VectorMA( origin, 12, dir, sprOrg );
		VectorScale( dir, 64, sprVel );

		R_LFX_Smoke (sprOrg, sprVel, 32, 3.54, colory, colory2, colory3, colory4, colory4, 16, 200, 174, 1);


		R_LFX_Smoke (sprOrg, sprVel, 62, 2, colory, colory2, colory3, colory4, colory4, 12, 800,84, 8);
		R_LFX_Smoke (sprOrg, sprVel, 32, 1.54, colory, colory2, colory3, colory4, colory4, 22, 400, 74, 8);

		R_LFX_Smoke (sprOrg, sprVel, 44, 1.3, colory, colory2, colory3, colory4, colory4, 3, 500,3, 8);

		R_LFX_Smoke (sprOrg, sprVel, 32, 0.54, colory, colory2, colory3, colory4, colory4, 12, 400, 74, 8);

		// Shroom Cloud
		VectorMA( origin, 16, dir, sprOrg );
		VectorScale( dir, 64, sprVel );

		R_LFX_Smoke (sprOrg, sprVel, 3, 0.7, colory, colory2, colory3, colory4, colory4, 22, 300,135, 8);

		// Sparks!

		VectorMA( origin, 12, dir, sprOrg );
		VectorScale( dir, 64, sprVel );

		R_LFX_Burst (sprOrg, sprVel, 175, 15, colory, colory2, colory3, colory4, colory4, 15, 140, 32, 1);
		//R_LFX_Dlight (sprOrg, sprVel, 8366, 2, colory, colory2, colory3, colory4, colory4, 4, 50, 2, 1);
		R_LFX_Spark (sprOrg, sprVel, 175, 5, colory, colory2, colory3, colory4, colory4, 15, 1040, 0.8f, 1);
		//R_LFX_Dlight (sprOrg, sprVel, 4000, 1, colory, colory2, colory3, colory4, colory4, 4, 50, 2, 1);
		RE_AddRippleToScene( origin, 300 );
	}

	// Nail Hit
	if (effect == 10) {
		colory[0] = 0.7;
		colory[1] = 0.7;
		colory[2] = 0.7;
		colory[3] = 0.0;
		colory2[0] = 0.7;
		colory2[1] = 0.7;
		colory2[2] = 0.7;
		colory2[3] = 0.5;
		colory3[0] = 0.6;
		colory3[1] = 0.6;
		colory3[2] = 0.6;
		colory3[3] = 0.8;
		colory4[0] = 0.6;
		colory4[1] = 0.6;
		colory4[2] = 0.6;
		colory4[3] = 0.0;
		VectorMA( origin, 1, dir, sprOrg );
		VectorScale( dir, 7, sprVel );
		R_LFX_Smoke2 (sprOrg, sprVel, 2 + (random()*6), 3.54+ (random()*8.7), colory, colory2, colory3, colory4, colory4, 1, 300 + (random()*2000), 2, 8+ (random()*6), 4);

		colory[0] = 0.7;
		colory[1] = 1.0;
		colory[2] = 1.0;
		colory[3] = 0.0;
		colory2[0] = 0.3;
		colory2[1] = 0.7;
		colory2[2] = 1.0;
		colory2[3] = 0.5;
		colory3[0] = 0.0;
		colory3[1] = 0.2;
		colory3[2] = 0.5;
		colory3[3] = 0.8;
		colory4[0] = 0.0;
		colory4[1] = 0.0;
		colory4[2] = 0.0;
		colory4[3] = 0.0;
		RE_AddRippleToScene( origin, 64 );
		VectorMA( origin, 4, dir, sprOrg );
		VectorScale( dir, 1, sprVel );

		R_LFX_Burst (sprOrg, sprVel, 8366, 2, colory, colory2, colory3, colory4, colory4, 4, 150, 3, 1);

		VectorMA( origin, 1, dir, sprOrg );
		VectorScale( dir, 1, sprVel );

		R_LFX_Spark (sprOrg, sprVel, 95, 225, colory, colory2, colory3, colory4, colory4, 5, 140, 0.5f, 1);
		R_LFX_Spark (sprOrg, sprVel, 95, 185, colory, colory2, colory3, colory4, colory4, 1, 2540, 0.5f, 1);

	}

	// Blood Sprays for bullets
	if (effect == 14 && com_blood->integer) {
		sprVel[2] += 54;
		colory[0] = 1.0;
		colory[1] = 0.0;
		colory[2] = 0.0;
		colory[3] = 1.0;
		colory2[0] = 0.8;
		colory2[1] = 0.0;
		colory2[2] = 0.0;
		colory2[3] = 0.5;
		colory3[0] = 0.6;
		colory3[1] = 0.0;
		colory3[2] = 0.0;
		colory3[3] = 0.8;
		colory4[0] = 0.3;
		colory4[1] = 0.0;
		colory4[2] = 0.0;
		colory4[3] = 0.0;
		//R_LFX_Smoke2 (sprOrg, sprVel, 2 + (random()*6), 6.54+ (random()*8.7), colory, colory2, colory3, colory4, colory4, 1, 800 + (random()*2000), 2, 8+ (random()*6), 0);
		//R_LFX_Smoke2 (sprOrg, sprVel, 2 + (random()*6), 3.54+ (random()*8.7), colory, colory2, colory3, colory4, colory4, 1, 300 + (random()*2000), 2, 8+ (random()*6), 4);
		R_LFX_Burst (sprOrg, sprVel, 175, 15, colory, colory2, colory3, colory4, colory4, 2, 1755, 12, 666);
		VectorMA( origin, 1, dir, sprOrg );
		VectorScale( dir, 2, sprVel );
		//R_LFX_Spark (sprOrg, sprVel, 25, 85, colory, colory2, colory3, colory4, colory4, 8, 2386, 0.5f, 666);
	}

	// "Blood" Sprays for bullets
	if (effect == 14 && !com_blood->integer) {

		// nonviolent blue center
		colory[0] = 0.0;
		colory[1] = 0.7;
		colory[2] = 1.0;
		colory[3] = 0.0;
		colory2[0] = 0.0;
		colory2[1] = 0.0;
		colory2[2] = 1.0;
		colory2[3] = 0.5;
		colory3[0] = 0.0;
		colory3[1] = 0.0;
		colory3[2] = 0.6;
		colory3[3] = 0.8;
		colory4[0] = 0.0;
		colory4[1] = 0.0;
		colory4[2] = 0.0;
		colory4[3] = 0.0;
		R_LFX_Burst (sprOrg, sprVel, 175, 15, colory, colory2, colory3, colory4, colory4, 2, 105, 12, 1);
		R_LFX_Smoke (sprOrg, sprVel, 0, 0, colory, colory2, colory3, colory4, colory4, 32, 400, -33, 1);
		VectorMA( origin, 1, dir, sprOrg );
		VectorScale( dir, 1, sprVel );

		colory[0] = 1;
		colory[1] = 1;
		colory[2] = 1.0;
		colory[3] = 1.0;
		colory2[0] = 1;
		colory2[1] = 1;
		colory2[2] = 0.8;
		colory2[3] = 0.9;
		colory3[0] = 0.7;
		colory3[1] = 0.5;
		colory3[2] = 0.2;
		colory3[3] = 0.7;
		colory4[0] = 0.1;
		colory4[1] = 0.06;
		colory4[2] = 0.0;
		colory4[3] = 0.0;

		R_LFX_Spark (sprOrg, sprVel, 25, 85, colory, colory2, colory3, colory4, colory4, 2,586, 0.5f, 1);
		R_LFX_Shock (origin, dir, 0, 0, colory, colory2, colory3, colory4, colory4, 1, 255, 39,1);
	}


	// Blood Spray for a gibbing
	if (effect == 16 && com_blood->integer) {
		sprVel[2] += 124;
		colory[0] = 1.0;
		colory[1] = 0.0;
		colory[2] = 0.0;
		colory[3] = 1.0;
		colory2[0] = 0.8;
		colory2[1] = 0.0;
		colory2[2] = 0.0;
		colory2[3] = 0.5;
		colory3[0] = 0.6;
		colory3[1] = 0.0;
		colory3[2] = 0.0;
		colory3[3] = 0.8;
		colory4[0] = 0.3;
		colory4[1] = 0.0;
		colory4[2] = 0.0;
		colory4[3] = 0.0;
		//	R_LFX_Smoke2 (sprOrg, sprVel, 2 + (random()*6), 6.54+ (random()*8.7), colory, colory2, colory3, colory4, colory4, 1, 800 + (random()*2000), 2, 8+ (random()*6), 0);
		//	R_LFX_Smoke2 (sprOrg, sprVel, 2 + (random()*6), 3.54+ (random()*8.7), colory, colory2, colory3, colory4, colory4, 1, 300 + (random()*2000), 2, 8+ (random()*6), 4);
		R_LFX_Burst (sprOrg, sprVel, 275, 4, colory, colory2, colory3, colory4, colory4, 9, 1455, 42, 666);
	}

	// Water Splash for bullets
	if (effect == 19) {
		colory[0] = 1.0;
		colory[1] = 1.0;
		colory[2] = 1.0;
		colory[3] = 1.0;
		colory2[0] = 0.3;
		colory2[1] = 0.5;
		colory2[2] = 0.6;
		colory2[3] = 0.6;
		colory3[0] = 0.1;
		colory3[1] = 0.2;
		colory3[2] = 0.3;
		colory3[3] = 0.3;
		colory4[0] = 0.0;
		colory4[1] = 0.0;
		colory4[2] = 0.0;
		colory4[3] = 0.0;
		VectorScale( dir, 39, sprVel );
		RE_AddRippleToScene( origin, 64 );

		R_LFX_Shock (origin, dir, 0, 0, colory, colory2, colory3, colory4, colory4, 1, 800, 80,14);
		R_LFX_Burst (sprOrg, sprVel, 22, 266, colory, colory2, colory3, colory4, colory4, 1, 1900, 5, 7);
		R_LFX_Spark (sprOrg, sprVel, 134, 4, colory, colory2, colory3, colory4, colory4, 7, 1286, 0.5f, 1);
	}
	/*
		// these are not properly implemented through cgame yet as they call *every* frame instead of being a truly one shot effect.
		// muzzieflash plasmagun
		if (effect == 66)
			{
				colory[0] = 1.0; colory[1] = 1.0; colory[2] = 1.0; colory[3] = 1.0;
				colory2[0] = 0.3; colory2[1] = 0.5; colory2[2] = 0.6; colory2[3] = 0.5;
				colory3[0] = 0.1; colory3[1] = 0.2; colory3[2] = 0.3; colory3[3] = 0.8;
				colory4[0] = 0.0; colory4[1] = 0.0; colory4[2] = 0.0; colory4[3] = 0.0;
				VectorScale( dir, 39, sprVel );
				R_LFX_Shock (origin, dir, 0, 0, colory, colory2, colory3, colory4, colory4, 1, 255, 8,1);
				//R_LFX_Burst (sprOrg, sprVel, 22, 266, colory, colory2, colory3, colory4, colory4, 1, 144, 2, 7);
				//R_LFX_Spark (sprOrg, sprVel, 134, 4, colory, colory2, colory3, colory4, colory4, 7, 1556, 0.25f, 1);
			}
		// muzzleflash shotgun
		if (effect == 63)
			{
			colory[0] = 0.7; colory[1] = 0.7; colory[2] = 0.7; colory[3] = 0.0;
			colory2[0] = 0.7; colory2[1] = 0.7; colory2[2] = 0.7; colory2[3] = 0.5;
			colory3[0] = 0.6; colory3[1] = 0.6; colory3[2] = 0.6; colory3[3] = 0.8;
			colory4[0] = 0.6; colory4[1] = 0.6; colory4[2] = 0.6; colory4[3] = 0.0;

				//R_LFX_Burst (sprOrg, sprVel, 22, 266, colory, colory2, colory3, colory4, colory4, 1, 144, 2, 7);

				R_LFX_Smoke2 (sprOrg, sprVel, 2 + (random()*6), 3.54+ (random()*55.7), colory, colory2, colory3, colory4, colory4, 4, 300 + (random()*3000), 2, 8+ (random()*6), 4);
				VectorScale( dir, 39, sprVel );
				R_LFX_Spark (sprOrg, sprVel, 14, 8, colory, colory2, colory3, colory4, colory4, 22, 556, 0.25f, 1);
			}

	*/
}

// 1997 -
// - alpha blends ONLY!
// - low amounts of polygons
// - high atlas usage
void LFX_ParticleEffect1997 (int effect, const vec3_t org, const vec3_t dir)
{
#if 0
	vec3_t origin, sprOrg, sprVel; // laziness

	VectorCopy(org, sprOrg);
	VectorCopy(org, origin);
	VectorCopy(dir, sprVel);
#endif

	// Smoke trails on grenades and rockets
	// this should be several smoke atlases along a line.
	if (effect == 1) {

	}

	// Bullet Hit
	// a smoke puff, a badly looping spark model, and an occasional ball spark (no stretch) of a random amount from 1 to 6 balls.
	if (effect == 2) {

	}

	// Shotgun Hit
	// can be just smoke puffs, preferably atlasy.
	if (effect == 3) {

	}

	// Plasma Hit
	// should become a plain sprite atlas animation.
	if (effect == 6) {

	}

	// Lightning Hit
	// can become a plain sprite, but we already have that, so.....
	if (effect == 8) {

	}


	// Grenade/Rocket/Prox Explosion
	// 	should be an explosion atlas, but on later versions there's several more delayed explosion atlases to spice up the variety
	else if (effect == 5 || effect == 4 || effect == 11) {


	}

	// BFG, could be an expanding sphere with an explosion atlas
	else if (effect == 9) {


	}
#if 0
	// Blood Sprays for bullets
	vec4_t colory, colory2, colory3, colory4;
	if (effect == 14 && com_blood->integer) {
		colory[0] = 1.0;
		colory[1] = 0.0;
		colory[2] = 0.0;
		colory[3] = 1.0;
		colory2[0] = 0.8;
		colory2[1] = 0.0;
		colory2[2] = 0.0;
		colory2[3] = 0.5;
		colory3[0] = 0.6;
		colory3[1] = 0.0;
		colory3[2] = 0.0;
		colory3[3] = 0.8;
		colory4[0] = 0.3;
		colory4[1] = 0.0;
		colory4[2] = 0.0;
		colory4[3] = 0.0;
		//R_LFX_Smoke2 (sprOrg, sprVel, 2 + (random()*6), 6.54+ (random()*8.7), colory, colory2, colory3, colory4, colory4, 1, 800 + (random()*2000), 2, 8+ (random()*6), 0);
		//R_LFX_Smoke2 (sprOrg, sprVel, 2 + (random()*6), 3.54+ (random()*8.7), colory, colory2, colory3, colory4, colory4, 1, 300 + (random()*2000), 2, 8+ (random()*6), 4);
	}
#endif

	// "Blood" Sprays for bullets
	if (effect == 14 && !com_blood->integer) {
		// usually there are none.
	}


	// Blood Spray for a gibbing
	if (effect == 16 && com_blood->integer) {
		// usually there are none.
	}

	// Water Splash for bullets - a splash atlas burst animation.
	if (effect == 19) {

	}

}


// 1998 - a certain game from washington state
// - some atlas animation, but not much
// - glquake particles are still present
// - alpha and additives only
// - sparks, but sparsely and no collision
// - some flare testing on some effects

void LFX_ParticleEffect1998 (int effect, const vec3_t org, const vec3_t dir)
{
	vec3_t origin, sprOrg, sprVel; // laziness
	vec4_t colory, colory2, colory3, colory4;

	VectorCopy(org, sprOrg);
	VectorCopy(org, origin);
	VectorCopy(dir, sprVel);

	// Smoke trails on grenades and rockets
	// might be a beam.
	if (effect == 1) {

	}

	// Bullet Hit
	// glquake black particles of debris coming off
	// followed by a few stretchy sparks of orange that fall out of it
	//


	if (effect == 2) {
		// Sparks
		colory[0] = 0.6;
		colory[1] = 0.4;
		colory[2] = 0.0;
		colory[3] = 1.0;
		colory2[0] = 0.7;
		colory2[1] = 0.4;
		colory2[2] = 0.0;
		colory2[3] = 0.9;
		colory3[0] = 0.5;
		colory3[1] = 0.2;
		colory3[2] = 0.0;
		colory3[3] = 0.7;
		colory4[0] = 0.1;
		colory4[1] = 0.06;
		colory4[2] = 0.0;
		colory4[3] = 0.0;

		VectorMA( origin, 1, dir, sprOrg );
		VectorScale( dir, 1, sprVel );
		sprVel[2] += 2;
		R_LFX_Spark (sprOrg, sprVel, 11, 32, colory, colory2, colory3, colory4, colory4, 4, 440, 0.5f, 1);
		sprVel[2] += 64;
		R_LFX_Generic (
		    LFXSMOKE, 	// Particle Type
		    sprOrg, 	// Origin
		    sprVel,		// Velocity
		    1.0f,		// Starting Alpha
		    85,		// Spread Factor
		    3,		// Random Origin Offset
		    0,		// Random Roll Value
		    0,		// Additional Random Speed
		    8, 		// Particle Color Red
		    8, 		// Particle Color Green
		    8, 		// Particle Color Blue
		    8,		// Particle Count
		    400,		// Particle Life
		    0.5f,		// Particle Scale
		    0.5f,		// Particle Scale Towards
		    0.1f,		// Particle Bounce Factor
		    0,		// Air Friction (stopping particle velocity in air)
		    -16.0f,		// Particle Gravity Factor
		    0.0f,		// Particle Rolling Friction
		    alfball		// Particle Shader
		);
	}
	else if (effect == 3) {
		if (rand() < 0.5f) {	// only happens for half the pellets
			// Sparks
			colory[0] = 0.6;
			colory[1] = 0.4;
			colory[2] = 0.0;
			colory[3] = 1.0;
			colory2[0] = 0.7;
			colory2[1] = 0.4;
			colory2[2] = 0.0;
			colory2[3] = 0.9;
			colory3[0] = 0.5;
			colory3[1] = 0.2;
			colory3[2] = 0.0;
			colory3[3] = 0.7;
			colory4[0] = 0.1;
			colory4[1] = 0.06;
			colory4[2] = 0.0;
			colory4[3] = 0.0;

			VectorMA( origin, 1, dir, sprOrg );
			VectorScale( dir, 1, sprVel );

			R_LFX_Spark (sprOrg, sprVel, 95, 32, colory, colory2, colory3, colory4, colory4, 3, 440, 0.5f, 1);
			sprVel[2] += 64;
			R_LFX_Generic (
			    LFXSMOKE, 	// Particle Type
			    sprOrg, 	// Origin
			    sprVel,		// Velocity
			    1.0f,		// Starting Alpha
			    85,		// Spread Factor
			    3,		// Random Origin Offset
			    0,		// Random Roll Value
			    0,		// Additional Random Speed
			    8, 		// Particle Color Red
			    8, 		// Particle Color Green
			    8, 		// Particle Color Blue
			    8,		// Particle Count
			    400,		// Particle Life
			    0.5f,		// Particle Scale
			    0.5f,		// Particle Scale Towards
			    0.1f,		// Particle Bounce Factor
			    0,		// Air Friction (stopping particle velocity in air)
			    -16.0f,		// Particle Gravity Factor
			    0.0f,		// Particle Rolling Friction
			    alfball		// Particle Shader
			);
		}
	}


	// Plasma Hit
	// should become a plain sprite atlas animation.
	if (effect == 6) {

	}
	else if (effect == 5 || effect == 4 || effect == 11) {
		// Grenade/Rocket/Prox Explosion
		// 	should be an explosion atlas
		// 	plus a smoke atlas that comes in later
		//	plus a few glquake-ish embers
		//	plus a couple of sparks that shoots off other sparks


	}
	else if (effect == 9) {
		// BFG, could be just a shockwave.

	}

	// Blood Sprays for bullets
	// should be a nondirectional splat of a center atlas gush
	// and then several randomly directed blood drops that roll. all is GE128
	if (effect == 14 && com_blood->integer) {
		colory[0] = 1.0;
		colory[1] = 0.0;
		colory[2] = 0.0;
		colory[3] = 1.0;
		colory2[0] = 0.8;
		colory2[1] = 0.0;
		colory2[2] = 0.0;
		colory2[3] = 0.5;
		colory3[0] = 0.6;
		colory3[1] = 0.0;
		colory3[2] = 0.0;
		colory3[3] = 0.8;
		colory4[0] = 0.3;
		colory4[1] = 0.0;
		colory4[2] = 0.0;
		colory4[3] = 0.0;
		//R_LFX_Smoke2 (sprOrg, sprVel, 2 + (random()*6), 6.54+ (random()*8.7), colory, colory2, colory3, colory4, colory4, 1, 800 + (random()*2000), 2, 8+ (random()*6), 0);
		//R_LFX_Smoke2 (sprOrg, sprVel, 2 + (random()*6), 3.54+ (random()*8.7), colory, colory2, colory3, colory4, colory4, 1, 300 + (random()*2000), 2, 8+ (random()*6), 4);
	}
}

// 1999 - a certain competing game
// - high emphasis on atlas animations
// - no alpha blending - only alpha blend, subtractive blend (for grenade smoke) and GE128 alpha testing.
void LFX_ParticleEffect1999 (int effect, const vec3_t org, const vec3_t dir)
{
	vec3_t origin, sprOrg, sprVel; // laziness
	vec4_t colory, colory2, colory3, colory4;

	VectorCopy(org, sprOrg);
	VectorCopy(org, origin);
	VectorCopy(dir, sprVel);

	// Smoke trails on grenades and rockets
	// this should be several smoke atlases along a line.
	if (effect == 1) {

	}

	// Bullet Hit
	// a smoke puff, a badly looping spark model, and an occasional ball spark (no stretch) of a random amount from 1 to 6 balls.
	if (effect == 2) {
		R_LFX_Generic (
		    777777, 	// Particle Type
		    sprOrg, 	// Origin
		    sprVel,		// Velocity
		    1.0f,		// Starting Alpha
		    96,		// Spread Factor
		    4,		// Random Origin Offset
		    0,		// Random Roll Value
		    200,		// Additional Random Speed
		    255, 		// Particle Color Red
		    255, 		// Particle Color Green
		    32, 		// Particle Color Blue
		    5,		// Particle Count
		    600,		// Particle Life
		    1,		// Particle Scale
		    1,		// Particle Scale Towards
		    0.1f,		// Particle Bounce Factor
		    0,		// Air Friction (stopping particle velocity in air)
		    -12.0f,		// Particle Gravity Factor
		    0.0f,		// Particle Rolling Friction
		    addball		// Particle Shader
		);
	}

	// Shotgun Hit
	// can be just smoke puffs, preferably atlasy.
	if (effect == 3) {
		VectorMA( origin, 8, dir, sprOrg );
		sprVel[2] += 20;
		R_LFX_Generic (
		    LFXSMOKE, 	// Particle Type
		    sprOrg, 	// Origin
		    sprVel,		// Velocity
		    1.0f,		// Starting Alpha
		    0,		// Spread Factor
		    4,		// Random Origin Offset
		    0,		// Random Roll Value
		    0,		// Additional Random Speed
		    255, 		// Particle Color Red
		    255, 		// Particle Color Green
		    255, 		// Particle Color Blue
		    5,		// Particle Count
		    1200,		// Particle Life
		    12,		// Particle Scale
		    12,		// Particle Scale Towards
		    0,		// Particle Bounce Factor
		    0,		// Air Friction (stopping particle velocity in air)
		    0,		// Particle Gravity Factor
		    10,		// Particle Rolling Friction
		    addsmoke	// Particle Shader
		);
	}

	// Plasma Hit
	// should become a plain sprite atlas animation.
	if (effect == 6) {

	}

	// Lightning Hit
	// can become a plain sprite, but we already have that, so.....
	if (effect == 8) {

	}
	else if (effect == 5 || effect == 4 || effect == 11) {
		// Grenade/Rocket/Prox Explosion
		// 	should be an explosion atlas, but on later versions there's several more delayed explosion atlases to spice up the variety


	}
	else if (effect == 9) {
		// BFG, could be an expanding sphere with an explosion atlas

	}

	// Blood Sprays for bullets
	// should be an additive smoke puff that doesn't move (but go through an atlas animation)
	// as well as a model that has autosprites for ge128 blood sprays, which is kinda cheesy but we can replicate this effect
	// withi particles anyhow because we're awesome for having a real particle system ha ha ha ha ha
	if (effect == 14 && com_blood->integer) {
		VectorMA( origin, 8, dir, sprOrg );
		sprVel[2] = 0;
		sprVel[1] = 0;
		sprVel[0] = 0;
		R_LFX_Generic (
		    LFXSMOKE, 	// Particle Type
		    sprOrg, 	// Origin
		    sprVel,		// Velocity
		    1.0f,		// Starting Alpha
		    0,		// Spread Factor
		    4,		// Random Origin Offset
		    0,		// Random Roll Value
		    0,		// Additional Random Speed
		    255, 		// Particle Color Red
		    0, 		// Particle Color Green
		    0, 		// Particle Color Blue
		    1,		// Particle Count
		    700,		// Particle Life
		    8,		// Particle Scale
		    8,		// Particle Scale Towards
		    0,		// Particle Bounce Factor
		    0,		// Air Friction (stopping particle velocity in air)
		    0,		// Particle Gravity Factor
		    10,		// Particle Rolling Friction
		    addsmoke	// Particle Shader
		);

		R_LFX_Generic (
		    LFXSMOKE, 	// Particle Type
		    sprOrg, 	// Origin
		    sprVel,		// Velocity
		    1.0f,		// Starting Alpha
		    96,		// Spread Factor
		    4,		// Random Origin Offset
		    0,		// Random Roll Value
		    200,		// Additional Random Speed
		    255, 		// Particle Color Red
		    0, 		// Particle Color Green
		    0, 		// Particle Color Blue
		    5,		// Particle Count
		    600,		// Particle Life
		    1,		// Particle Scale
		    1,		// Particle Scale Towards
		    0.1f,		// Particle Bounce Factor
		    0,		// Air Friction (stopping particle velocity in air)
		    -12.0f,		// Particle Gravity Factor
		    0.0f,		// Particle Rolling Friction
		    alfsmoke	// Particle Shader
		);
	}

	// "Blood" Sprays for bullets
	if (effect == 14 && !com_blood->integer) {
		// not yet. this should be an additive green puff
	}


	// Blood Spray for a gibbing
	if (effect == 16 && com_blood->integer) {
		// not yet.
	}

	// Water Splash for bullets - there should only be a polygonal white ring. but we don't do model drawing right now
	if (effect == 19) {
		colory[0] = 1.0;
		colory[1] = 1.0;
		colory[2] = 1.0;
		colory[3] = 1.0;
		colory2[0] = 0.7;
		colory2[1] = 0.7;
		colory2[2] = 0.7;
		colory2[3] = 0.6;
		colory3[0] = 0.4;
		colory3[1] = 0.4;
		colory3[2] = 0.4;
		colory3[3] = 0.3;
		colory4[0] = 0.0;
		colory4[1] = 0.0;
		colory4[2] = 0.0;
		colory4[3] = 0.0;
		VectorScale( dir, 39, sprVel );
		R_LFX_Shock (origin, dir, 0, 0, colory, colory2, colory3, colory4, colory4, 1, 800, 80,14);
	}
}

void LFX_ParticleEffect (int effect, const vec3_t org, const vec3_t dir)
{
	// choosing particle sets

	if (r_particles->value == 1996)					// Mimicing the grand old game
		LFX_ParticleEffect1996(effect, org, dir);
	else if (r_particles->value == 1997)				// Mimicing games for an allegedly 64-bit video game system
		LFX_ParticleEffect1997(effect, org, dir);
	else if (r_particles->value == 1998)				// Mimicing a certain game from Washington state
		LFX_ParticleEffect1998(effect, org, dir);
	else if (r_particles->value == 1999)				// Mimicing a certain competing game
		LFX_ParticleEffect1999(effect, org, dir);
	else
		LFX_ParticleEffect200X(effect, org, dir);
}
