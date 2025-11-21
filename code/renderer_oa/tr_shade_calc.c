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
// tr_shade_calc.c

#include "tr_local.h"
#if idppc_altivec && !defined(MACOS_X)
#include <altivec.h>
#endif






#define	WAVEVALUE( table, base, amplitude, phase, freq )  ((base) + table[ ri.ftol( ( ( (phase) + tess.shaderTime * (freq) ) * FUNCTABLE_SIZE ) ) & FUNCTABLE_MASK ] * (amplitude))

static float *TableForFunc( genFunc_t func ) 
{
	switch ( func )
	{
	case GF_SIN:
		return tr.sinTable;
	case GF_TRIANGLE:
		return tr.triangleTable;
	case GF_SQUARE:
		return tr.squareTable;
	case GF_SAWTOOTH:
		return tr.sawToothTable;
	case GF_INVERSE_SAWTOOTH:
		return tr.inverseSawToothTable;
	case GF_NONE:
	default:
		break;
	}

	ri.Error( ERR_DROP, "TableForFunc called with invalid function '%d' in shader '%s'", func, tess.shader->name );
	return NULL;
}

/*
** EvalWaveForm
**
** Evaluates a given waveForm_t, referencing backEnd.refdef.time directly
*/
static float EvalWaveForm( const waveForm_t *wf ) 
{
	float	*table;

	table = TableForFunc( wf->func );

	return WAVEVALUE( table, wf->base, wf->amplitude, wf->phase, wf->frequency );
}

static float EvalWaveFormClamped( const waveForm_t *wf )
{
	float glow  = EvalWaveForm( wf );

	if ( glow < 0 )
	{
		return 0;
	}

	if ( glow > 1 )
	{
		return 1;
	}

	return glow;
}

/*
** RB_CalcStretchTexCoords
*/
void RB_CalcStretchTexCoords( const waveForm_t *wf, float *st )
{
	float p;
	texModInfo_t tmi;

	p = 1.0f / EvalWaveForm( wf );

	tmi.matrix[0][0] = p;
	tmi.matrix[1][0] = 0;
	tmi.translate[0] = 0.5f - 0.5f * p;

	tmi.matrix[0][1] = 0;
	tmi.matrix[1][1] = p;
	tmi.translate[1] = 0.5f - 0.5f * p;

	RB_CalcTransformTexCoords( &tmi, st );
}

/*
====================================================================

DEFORMATIONS

====================================================================
*/

/*
========================
RB_CalcDeformVertexes

========================
*/
void RB_CalcDeformVertexes( deformStage_t *ds )
{
	int i;
	vec3_t	offset;
	float	scale;
	float	*xyz = ( float * ) tess.xyz;
	float	*normal = ( float * ) tess.normal;
	float	*table;

	if ( ds->deformationWave.frequency == 0 )
	{
		scale = EvalWaveForm( &ds->deformationWave );

		for ( i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4 )
		{
			VectorScale( normal, scale, offset );
			
			xyz[0] += offset[0];
			xyz[1] += offset[1];
			xyz[2] += offset[2];
		}
	}
	else
	{
		table = TableForFunc( ds->deformationWave.func );

		for ( i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4 )
		{
			float off = ( xyz[0] + xyz[1] + xyz[2] ) * ds->deformationSpread;

			scale = WAVEVALUE( table, ds->deformationWave.base, 
				ds->deformationWave.amplitude,
				ds->deformationWave.phase + off,
				ds->deformationWave.frequency );

			VectorScale( normal, scale, offset );
			
			xyz[0] += offset[0];
			xyz[1] += offset[1];
			xyz[2] += offset[2];
		}
	}
}

/*
=========================
RB_CalcDeformNormals

Wiggle the normals for wavy environment mapping
=========================
*/
void RB_CalcDeformNormals( deformStage_t *ds ) {
	int i;
	float	scale;
	float	*xyz = ( float * ) tess.xyz;
	float	*normal = ( float * ) tess.normal;

	for ( i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4 ) {
		scale = 0.98f;
		scale = R_NoiseGet4f( xyz[0] * scale, xyz[1] * scale, xyz[2] * scale,
			tess.shaderTime * ds->deformationWave.frequency );
		normal[ 0 ] += ds->deformationWave.amplitude * scale;

		scale = 0.98f;
		scale = R_NoiseGet4f( 100 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale,
			tess.shaderTime * ds->deformationWave.frequency );
		normal[ 1 ] += ds->deformationWave.amplitude * scale;

		scale = 0.98f;
		scale = R_NoiseGet4f( 200 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale,
			tess.shaderTime * ds->deformationWave.frequency );
		normal[ 2 ] += ds->deformationWave.amplitude * scale;

		VectorNormalizeFast( normal );
	}
}


void RB_CalcDeformNormalsEvenMore( deformStage_t *ds ) {
	int i;
	float	scale;
	float	*xyz = ( float * ) tess.xyz;
	float	*normal = ( float * ) tess.normal;

	for ( i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4 ) {
		scale = 5.98f;
		scale = R_NoiseGet4f( xyz[0] * scale, xyz[1] * scale, xyz[2] * scale,
			tess.shaderTime * ds->deformationWave.frequency );
		normal[ 0 ] += ds->deformationWave.amplitude * scale;

		scale = 5.98f;
		scale = R_NoiseGet4f( 100 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale,
			tess.shaderTime * ds->deformationWave.frequency );
		normal[ 1 ] += ds->deformationWave.amplitude * scale;

		scale = 5.98f;
		scale = R_NoiseGet4f( 200 + xyz[0] * scale, xyz[1] * scale, xyz[2] * scale,
			tess.shaderTime * ds->deformationWave.frequency );
		normal[ 2 ] += ds->deformationWave.amplitude * scale;

		VectorNormalizeFast( normal );
	}
}

/*
========================
RB_CalcBulgeVertexes

========================
*/
void OldRB_CalcBulgeVertexes( deformStage_t *ds ) {
	int i;
	const float *st = ( const float * ) tess.texCoords[0];
	float		*xyz = ( float * ) tess.xyz;
	float		*normal = ( float * ) tess.normal;
	float		now;

	now = backEnd.refdef.time * ds->bulgeSpeed * 0.001f;

	for ( i = 0; i < tess.numVertexes; i++, xyz += 4, st += 4, normal += 4 ) {
		int		off;
		float scale;

		off = (float)( FUNCTABLE_SIZE / (M_PI*2) ) * ( st[0] * ds->bulgeWidth + now );

		scale = tr.sinTable[ off & FUNCTABLE_MASK ] * ds->bulgeHeight;
			
		xyz[0] += normal[0] * scale;
		xyz[1] += normal[1] * scale;
		xyz[2] += normal[2] * scale;
	}
}


// leilei - adapted a bit from the jk2 source, for performance

void RB_CalcBulgeVertexes( deformStage_t *ds ) {
	int i;
	float		*xyz = ( float * ) tess.xyz;
	float		*normal = ( float * ) tess.normal;

	if ( ds->bulgeSpeed == 0.0f && ds->bulgeWidth == 0.0f )
	{
		// We don't have a speed and width, so just use height to expand uniformly
		for ( i = 0; i < tess.numVertexes; i++, xyz += 4, normal += 4 ) 
		{
			xyz[0] += normal[0] * ds->bulgeHeight;
			xyz[1] += normal[1] * ds->bulgeHeight;
			xyz[2] += normal[2] * ds->bulgeHeight;
		}	
	}
	else
	{
		const float *st = ( const float * ) tess.texCoords[0];
		float		now;
	
		now = backEnd.refdef.time * ds->bulgeSpeed * 0.001f;
	for ( i = 0; i < tess.numVertexes; i++, xyz += 4, st += 4, normal += 4 ) {
		int		off;
		float scale;

		off = (float)( FUNCTABLE_SIZE / (M_PI*2) ) * ( st[0] * ds->bulgeWidth + now );

		scale = tr.sinTable[ off & FUNCTABLE_MASK ] * ds->bulgeHeight;
			
		xyz[0] += normal[0] * scale;
		xyz[1] += normal[1] * scale;
		xyz[2] += normal[2] * scale;
	}
	}
}



/*
======================
RB_CalcMoveVertexes

A deformation that can move an entire surface along a wave path
======================
*/
void RB_CalcMoveVertexes( deformStage_t *ds ) {
	int			i;
	float		*xyz;
	float		*table;
	float		scale;
	vec3_t		offset;

	table = TableForFunc( ds->deformationWave.func );

	scale = WAVEVALUE( table, ds->deformationWave.base, 
		ds->deformationWave.amplitude,
		ds->deformationWave.phase,
		ds->deformationWave.frequency );

	VectorScale( ds->moveVector, scale, offset );

	xyz = ( float * ) tess.xyz;
	for ( i = 0; i < tess.numVertexes; i++, xyz += 4 ) {
		VectorAdd( xyz, offset, xyz );
	}
}


/*
=============
DeformText

Change a polygon into a bunch of text polygons
=============
*/
void DeformText( const char *text ) {
	int		i;
	vec3_t	origin, width, height;
	int		len;
	int		ch;
	byte	color[4];
	float	bottom, top;
	vec3_t	mid;

	height[0] = 0;
	height[1] = 0;
	height[2] = -1;
	CrossProduct( tess.normal[0], height, width );

	// find the midpoint of the box
	VectorClear( mid );
	bottom = 999999;
	top = -999999;
	for ( i = 0 ; i < 4 ; i++ ) {
		VectorAdd( tess.xyz[i], mid, mid );
		if ( tess.xyz[i][2] < bottom ) {
			bottom = tess.xyz[i][2];
		}
		if ( tess.xyz[i][2] > top ) {
			top = tess.xyz[i][2];
		}
	}
	VectorScale( mid, 0.25f, origin );

	// determine the individual character size
	height[0] = 0;
	height[1] = 0;
	height[2] = ( top - bottom ) * 0.5f;

	VectorScale( width, height[2] * -0.75f, width );

	// determine the starting position
	len = strlen( text );
	VectorMA( origin, (len-1), width, origin );

	// clear the shader indexes
	tess.numIndexes = 0;
	tess.numVertexes = 0;

	color[0] = color[1] = color[2] = color[3] = 255;

	// draw each character
	for ( i = 0 ; i < len ; i++ ) {
		ch = text[i];
		ch &= 255;

		if ( ch != ' ' ) {
			int		row, col;
			float	frow, fcol, size;

			row = ch>>4;
			col = ch&15;

			frow = row*0.0625f;
			fcol = col*0.0625f;
			size = 0.0625f;

			RB_AddQuadStampExt( origin, width, height, color, fcol, frow, fcol + size, frow + size );
		}
		VectorMA( origin, -2, width, origin );
	}
}

/*
==================
GlobalVectorToLocal
==================
*/
static void GlobalVectorToLocal( const vec3_t in, vec3_t out ) {
	out[0] = DotProduct( in, backEnd.or.axis[0] );
	out[1] = DotProduct( in, backEnd.or.axis[1] );
	out[2] = DotProduct( in, backEnd.or.axis[2] );
}

/*
=====================
AutospriteDeform

Assuming all the triangles for this shader are independant
quads, rebuild them as forward facing sprites
=====================
*/
static void AutospriteDeform( void ) {
	int		i;
	int		oldVerts;
	float	*xyz;
	vec3_t	mid, delta;
	float	radius;
	vec3_t	left, up;
	vec3_t	leftDir, upDir;

	if ( tess.numVertexes & 3 ) {
		ri.Printf( PRINT_WARNING, "Autosprite shader %s had odd vertex count\n", tess.shader->name );
	}
	if ( tess.numIndexes != ( tess.numVertexes >> 2 ) * 6 ) {
		ri.Printf( PRINT_WARNING, "Autosprite shader %s had odd index count\n", tess.shader->name );
	}

	oldVerts = tess.numVertexes;
	tess.numVertexes = 0;
	tess.numIndexes = 0;

	if ( backEnd.currentEntity != &tr.worldEntity ) {
		GlobalVectorToLocal( backEnd.viewParms.or.axis[1], leftDir );
		GlobalVectorToLocal( backEnd.viewParms.or.axis[2], upDir );
	} else {
		VectorCopy( backEnd.viewParms.or.axis[1], leftDir );
		VectorCopy( backEnd.viewParms.or.axis[2], upDir );
	}

	for ( i = 0 ; i < oldVerts ; i+=4 ) {
		// find the midpoint
		xyz = tess.xyz[i];

		mid[0] = 0.25f * (xyz[0] + xyz[4] + xyz[8] + xyz[12]);
		mid[1] = 0.25f * (xyz[1] + xyz[5] + xyz[9] + xyz[13]);
		mid[2] = 0.25f * (xyz[2] + xyz[6] + xyz[10] + xyz[14]);

		VectorSubtract( xyz, mid, delta );
		radius = VectorLength( delta ) * 0.707f;		// / sqrt(2)

		VectorScale( leftDir, radius, left );
		VectorScale( upDir, radius, up );

		if ( backEnd.viewParms.isMirror ) {
			VectorSubtract( vec3_origin, left, left );
		}

	  // compensate for scale in the axes if necessary
  	if ( backEnd.currentEntity->e.nonNormalizedAxes ) {
      float axisLength;
		  axisLength = VectorLength( backEnd.currentEntity->e.axis[0] );
  		if ( !axisLength ) {
	  		axisLength = 0;
  		} else {
	  		axisLength = 1.0f / axisLength;
  		}
      VectorScale(left, axisLength, left);
      VectorScale(up, axisLength, up);
    }

		RB_AddQuadStamp( mid, left, up, tess.vertexColors[i] );
	}
}


/*
=====================
Autosprite2Deform

Autosprite2 will pivot a rectangular quad along the center of its long axis
=====================
*/
int edgeVerts[6][2] = {
	{ 0, 1 },
	{ 0, 2 },
	{ 0, 3 },
	{ 1, 2 },
	{ 1, 3 },
	{ 2, 3 }
};

static void Autosprite2Deform( void ) {
	int		i, j, k;
	int		indexes;
	float	*xyz;
	vec3_t	forward;

	if ( tess.numVertexes & 3 ) {
		ri.Printf( PRINT_WARNING, "Autosprite2 shader %s had odd vertex count", tess.shader->name );
	}
	if ( tess.numIndexes != ( tess.numVertexes >> 2 ) * 6 ) {
		ri.Printf( PRINT_WARNING, "Autosprite2 shader %s had odd index count", tess.shader->name );
	}

	if ( backEnd.currentEntity != &tr.worldEntity ) {
		GlobalVectorToLocal( backEnd.viewParms.or.axis[0], forward );
	} else {
		VectorCopy( backEnd.viewParms.or.axis[0], forward );
	}

	// this is a lot of work for two triangles...
	// we could precalculate a lot of it is an issue, but it would mess up
	// the shader abstraction
	for ( i = 0, indexes = 0 ; i < tess.numVertexes ; i+=4, indexes+=6 ) {
		float	lengths[2];
		int		nums[2];
		vec3_t	mid[2];
		vec3_t	major, minor;
		float	*v1, *v2;

		// find the midpoint
		xyz = tess.xyz[i];

		// identify the two shortest edges
		nums[0] = nums[1] = 0;
		lengths[0] = lengths[1] = 999999;

		for ( j = 0 ; j < 6 ; j++ ) {
			float	l;
			vec3_t	temp;

			v1 = xyz + 4 * edgeVerts[j][0];
			v2 = xyz + 4 * edgeVerts[j][1];

			VectorSubtract( v1, v2, temp );
			
			l = DotProduct( temp, temp );
			if ( l < lengths[0] ) {
				nums[1] = nums[0];
				lengths[1] = lengths[0];
				nums[0] = j;
				lengths[0] = l;
			} else if ( l < lengths[1] ) {
				nums[1] = j;
				lengths[1] = l;
			}
		}

		for ( j = 0 ; j < 2 ; j++ ) {
			v1 = xyz + 4 * edgeVerts[nums[j]][0];
			v2 = xyz + 4 * edgeVerts[nums[j]][1];

			mid[j][0] = 0.5f * (v1[0] + v2[0]);
			mid[j][1] = 0.5f * (v1[1] + v2[1]);
			mid[j][2] = 0.5f * (v1[2] + v2[2]);
		}

		// find the vector of the major axis
		VectorSubtract( mid[1], mid[0], major );

		// cross this with the view direction to get minor axis
		CrossProduct( major, forward, minor );
		VectorNormalize( minor );
		
		// re-project the points
		for ( j = 0 ; j < 2 ; j++ ) {
			float	l;

			v1 = xyz + 4 * edgeVerts[nums[j]][0];
			v2 = xyz + 4 * edgeVerts[nums[j]][1];

			l = 0.5 * sqrt( lengths[j] );
			
			// we need to see which direction this edge
			// is used to determine direction of projection
			for ( k = 0 ; k < 5 ; k++ ) {
				if ( tess.indexes[ indexes + k ] == i + edgeVerts[nums[j]][0]
					&& tess.indexes[ indexes + k + 1 ] == i + edgeVerts[nums[j]][1] ) {
					break;
				}
			}

			if ( k == 5 ) {
				VectorMA( mid[j], l, minor, v1 );
				VectorMA( mid[j], -l, minor, v2 );
			} else {
				VectorMA( mid[j], -l, minor, v1 );
				VectorMA( mid[j], l, minor, v2 );
			}
		}
	}
}


/*
=====================
RB_DeformTessGeometry

=====================
*/
void RB_DeformTessGeometry( void ) {
	int		i;
	deformStage_t	*ds;

	for ( i = 0 ; i < tess.shader->numDeforms ; i++ ) {
		ds = &tess.shader->deforms[ i ];

		switch ( ds->deformation ) {
			case DEFORM_NONE:
				break;
			case DEFORM_NORMALS:
				RB_CalcDeformNormals( ds );
				break;
			case DEFORM_WAVE:
				RB_CalcDeformVertexes( ds );
				break;
			case DEFORM_BULGE:
				RB_CalcBulgeVertexes( ds );
				break;
			case DEFORM_MOVE:
				RB_CalcMoveVertexes( ds );
				break;
			case DEFORM_PROJECTION_SHADOW:
				RB_ProjectionShadowDeform();
				break;
			case DEFORM_AUTOSPRITE:
				AutospriteDeform();
				break;
			case DEFORM_AUTOSPRITE2:
				Autosprite2Deform();
				break;
			case DEFORM_TEXT0:
			case DEFORM_TEXT1:
			case DEFORM_TEXT2:
			case DEFORM_TEXT3:
			case DEFORM_TEXT4:
			case DEFORM_TEXT5:
			case DEFORM_TEXT6:
			case DEFORM_TEXT7:
				DeformText( backEnd.refdef.text[ds->deformation - DEFORM_TEXT0] );
				break;
			default:
				break;
		}
	}
}

/*
====================================================================

COLORS

====================================================================
*/


/*
** RB_CalcColorFromEntity
*/
void RB_CalcColorFromEntity( unsigned char *dstColors )
{
	int	i;
	int *pColors = ( int * ) dstColors;
	int c;

	if ( !backEnd.currentEntity )
		return;

	c = * ( int * ) backEnd.currentEntity->e.shaderRGBA;

	for ( i = 0; i < tess.numVertexes; i++, pColors++ )
	{
		*pColors = c;
	}
}

/*
** RB_CalcColorFromOneMinusEntity
*/
void RB_CalcColorFromOneMinusEntity( unsigned char *dstColors )
{
	int	i;
	int *pColors = ( int * ) dstColors;
	unsigned char invModulate[4];
	int c;

	if ( !backEnd.currentEntity )
		return;

	invModulate[0] = 255 - backEnd.currentEntity->e.shaderRGBA[0];
	invModulate[1] = 255 - backEnd.currentEntity->e.shaderRGBA[1];
	invModulate[2] = 255 - backEnd.currentEntity->e.shaderRGBA[2];
	invModulate[3] = 255 - backEnd.currentEntity->e.shaderRGBA[3];	// this trashes alpha, but the AGEN block fixes it

	c = * ( int * ) invModulate;

	for ( i = 0; i < tess.numVertexes; i++, pColors++ )
	{
		*pColors = c;
	}
}

/*
** RB_CalcAlphaFromEntity
*/
void RB_CalcAlphaFromEntity( unsigned char *dstColors )
{
	int	i;

	if ( !backEnd.currentEntity )
		return;

	dstColors += 3;

	for ( i = 0; i < tess.numVertexes; i++, dstColors += 4 )
	{
		*dstColors = backEnd.currentEntity->e.shaderRGBA[3];
	}
}

/*
** RB_CalcAlphaFromOneMinusEntity
*/
void RB_CalcAlphaFromOneMinusEntity( unsigned char *dstColors )
{
	int	i;

	if ( !backEnd.currentEntity )
		return;

	dstColors += 3;

	for ( i = 0; i < tess.numVertexes; i++, dstColors += 4 )
	{
		*dstColors = 0xff - backEnd.currentEntity->e.shaderRGBA[3];
	}
}

/*
** RB_CalcWaveColor
*/
void RB_CalcWaveColor( const waveForm_t *wf, unsigned char *dstColors )
{
	int i;
	int v;
	float glow;
	int *colors = ( int * ) dstColors;
	byte	color[4];


  if ( wf->func == GF_NOISE ) {
		glow = wf->base + R_NoiseGet4f( 0, 0, 0, ( tess.shaderTime + wf->phase ) * wf->frequency ) * wf->amplitude;
	} else {
		glow = EvalWaveForm( wf ) * tr.identityLight;
	}
	
	if ( glow < 0 ) {
		glow = 0;
	}
	else if ( glow > 1 ) {
		glow = 1;
	}

	v = ri.ftol(255 * glow);
	color[0] = color[1] = color[2] = v;
	color[3] = 255;
	v = *(int *)color;
	
	for ( i = 0; i < tess.numVertexes; i++, colors++ ) {
		*colors = v;
	}
}

/*
** RB_CalcWaveAlpha
*/
void RB_CalcWaveAlpha( const waveForm_t *wf, unsigned char *dstColors )
{
	int i;
	int v;
	float glow;

	glow = EvalWaveFormClamped( wf );

	v = 255 * glow;

	for ( i = 0; i < tess.numVertexes; i++, dstColors += 4 )
	{
		dstColors[3] = v;
	}
}

/*
** RB_CalcModulateColorsByFog
*/
void RB_CalcModulateColorsByFog( unsigned char *colors ) {
	int		i;
	float	texCoords[SHADER_MAX_VERTEXES][2];

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	RB_CalcFogTexCoords( texCoords[0] );

	for ( i = 0; i < tess.numVertexes; i++, colors += 4 ) {
		float f = 1.0 - R_FogFactor( texCoords[i][0], texCoords[i][1] );
		colors[0] *= f;
		colors[1] *= f;
		colors[2] *= f;
	}
}

/*
** RB_CalcModulateAlphasByFog
*/
void RB_CalcModulateAlphasByFog( unsigned char *colors ) {
	int		i;
	float	texCoords[SHADER_MAX_VERTEXES][2];

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	RB_CalcFogTexCoords( texCoords[0] );

	for ( i = 0; i < tess.numVertexes; i++, colors += 4 ) {
		float f = 1.0 - R_FogFactor( texCoords[i][0], texCoords[i][1] );
		colors[3] *= f;
	}
}

/*
** RB_CalcModulateRGBAsByFog
*/
void RB_CalcModulateRGBAsByFog( unsigned char *colors ) {
	int		i;
	float	texCoords[SHADER_MAX_VERTEXES][2];

	// calculate texcoords so we can derive density
	// this is not wasted, because it would only have
	// been previously called if the surface was opaque
	RB_CalcFogTexCoords( texCoords[0] );

	for ( i = 0; i < tess.numVertexes; i++, colors += 4 ) {
		float f = 1.0 - R_FogFactor( texCoords[i][0], texCoords[i][1] );
		colors[0] *= f;
		colors[1] *= f;
		colors[2] *= f;
		colors[3] *= f;
	}
}


/*
====================================================================

TEX COORDS

====================================================================
*/

/*
========================
RB_CalcFogTexCoords

To do the clipped fog plane really correctly, we should use
projected textures, but I don't trust the drivers and it
doesn't fit our shader data.
========================
*/
void RB_CalcFogTexCoords( float *st ) {
	int			i;
	float		*v;
	float		s, t;
	float		eyeT;
	qboolean	eyeOutside;
	fog_t		*fog;
	vec3_t		local;
	vec4_t		fogDistanceVector, fogDepthVector = {0, 0, 0, 0};

	fog = tr.world->fogs + tess.fogNum;

	// all fogging distance is based on world Z units
	VectorSubtract( backEnd.or.origin, backEnd.viewParms.or.origin, local );
	fogDistanceVector[0] = -backEnd.or.modelMatrix[2];
	fogDistanceVector[1] = -backEnd.or.modelMatrix[6];
	fogDistanceVector[2] = -backEnd.or.modelMatrix[10];
	fogDistanceVector[3] = DotProduct( local, backEnd.viewParms.or.axis[0] );

	// scale the fog vectors based on the fog's thickness
	fogDistanceVector[0] *= fog->tcScale;
	fogDistanceVector[1] *= fog->tcScale;
	fogDistanceVector[2] *= fog->tcScale;
	fogDistanceVector[3] *= fog->tcScale;

	// rotate the gradient vector for this orientation
	if ( fog->hasSurface ) {
		fogDepthVector[0] = fog->surface[0] * backEnd.or.axis[0][0] + 
			fog->surface[1] * backEnd.or.axis[0][1] + fog->surface[2] * backEnd.or.axis[0][2];
		fogDepthVector[1] = fog->surface[0] * backEnd.or.axis[1][0] + 
			fog->surface[1] * backEnd.or.axis[1][1] + fog->surface[2] * backEnd.or.axis[1][2];
		fogDepthVector[2] = fog->surface[0] * backEnd.or.axis[2][0] + 
			fog->surface[1] * backEnd.or.axis[2][1] + fog->surface[2] * backEnd.or.axis[2][2];
		fogDepthVector[3] = -fog->surface[3] + DotProduct( backEnd.or.origin, fog->surface );

		eyeT = DotProduct( backEnd.or.viewOrigin, fogDepthVector ) + fogDepthVector[3];
	} else {
		eyeT = 1;	// non-surface fog always has eye inside
	}

	// see if the viewpoint is outside
	// this is needed for clipping distance even for constant fog

	if ( eyeT < 0 ) {
		eyeOutside = qtrue;
	} else {
		eyeOutside = qfalse;
	}

	fogDistanceVector[3] += 1.0/512;

	// calculate density for each point
	for (i = 0, v = tess.xyz[0] ; i < tess.numVertexes ; i++, v += 4) {
		// calculate the length in fog
		s = DotProduct( v, fogDistanceVector ) + fogDistanceVector[3];
		t = DotProduct( v, fogDepthVector ) + fogDepthVector[3];

		// partially clipped fogs use the T axis		
		if ( eyeOutside ) {
			if ( t < 1.0 ) {
				t = 1.0/32;	// point is outside, so no fogging
			} else {
				t = 1.0/32 + 30.0/32 * t / ( t - eyeT );	// cut the distance at the fog plane
			}
		} else {
			if ( t < 0 ) {
				t = 1.0/32;	// point is outside, so no fogging
			} else {
				t = 31.0/32;
			}
		}

		st[0] = s;
		st[1] = t;
		st += 2;
	}
}



/*
** RB_CalcEnvironmentTexCoords
*/
void RB_CalcEnvironmentTexCoords( float *st ) 
{
	int			i;
	float		*v, *normal;
	vec3_t		viewer, reflected;
	float		d;

	v = tess.xyz[0];
	normal = tess.normal[0];

	for (i = 0 ; i < tess.numVertexes ; i++, v += 4, normal += 4, st += 2 ) 
	{
		VectorSubtract (backEnd.or.viewOrigin, v, viewer);
		VectorNormalizeFast (viewer);

		d = DotProduct (normal, viewer);

		reflected[0] = normal[0]*2*d - viewer[0];
		reflected[1] = normal[1]*2*d - viewer[1];
		reflected[2] = normal[2]*2*d - viewer[2];

		st[0] = 0.5 + reflected[1] * 0.5;
		st[1] = 0.5 - reflected[2] * 0.5;
	}
}

/*
** RB_CalcEnvironmentTexCoordsNew

	This one also is offset by origin and axis which makes it look better on moving
	objects and weapons. May be slow.

*/
void RB_CalcEnvironmentTexCoordsNew( float *st ) 
{

	int			i;
	float		*v, *normal;
	vec3_t		viewer, reflected, where, what, why, who;
	float		d = 0.0f;

	v = tess.xyz[0];
	normal = tess.normal[0];

	for (i = 0 ; i < tess.numVertexes ; i++, v += 4, normal += 4, st += 2 ) 
	{

		VectorSubtract (backEnd.or.axis[0], v, what);
		VectorSubtract (backEnd.or.axis[1], v, why);
		VectorSubtract (backEnd.or.axis[2], v, who);

		VectorSubtract (backEnd.or.origin, v, where);
		VectorSubtract (backEnd.or.viewOrigin, v, viewer);

		VectorNormalizeFast (viewer);
		VectorNormalizeFast (where);
		VectorNormalizeFast (what);
		VectorNormalizeFast (why);
		VectorNormalizeFast (who);

		d = DotProduct (normal, viewer);

		if ( backEnd.currentEntity == &tr.worldEntity ){

		reflected[0] = normal[0]*2*d - viewer[0];
		reflected[1] = normal[1]*2*d - viewer[1];
		reflected[2] = normal[2]*2*d - viewer[2];

		}
	else
		{
		reflected[0] = normal[0]*2*d - viewer[0] - (where[0] * 5) + (what[0] * 4);
		reflected[1] = normal[1]*2*d - viewer[1] - (where[1] * 5) + (why[1] * 4);
		reflected[2] = normal[2]*2*d - viewer[2] - (where[2] * 5) + (who[2] * 4);


		}
		st[0] = 0.33 + reflected[1] * 0.33;
		st[1] = 0.33 - reflected[2] * 0.33;
	}
}


/*
** RB_CalcEnvironmentTexCoordsEx
  
   leilei - extended environment texcoords (to replace redundancy), to:

	- specify which axis are used
	- imitate raven behavior
	- allow reflection from local lights (i.e. for celshading, or weapon glimmer)

	mode 1 = raven mode (including weapon shine). 
		note: this ISN'T from JediOutcast but rather clean-room experiments for VitaVoyager
	mode 2 = weapon shine
	mode 3 = all models shine
	mode 4? = sun shine (water)?
	mode 5 - view space (i.e. buffer reflections/refractions)
*/
void RB_CalcEnvironmentTexCoordsEx( float *st, int xx, int yy, int mode ) 
{
	int			i;
	float		*v, *normal;
	vec3_t		viewer, reflected;
	float		d;

	v = tess.xyz[0];
	normal = tess.normal[0];

	// clamp these
	if (xx > 2) xx = 2;	if (yy > 2) yy = 2;
	if (xx < 0) xx = 0;	if (yy < 0) yy = 0;

	if (mode == 1) // raven
	{
		if (backEnd.currentEntity->e.renderfx & RF_FIRST_PERSON) // Weapons
		{
			for (i = 0 ; i < tess.numVertexes ; i++, v += 4, normal += 4, st += 2 ) 
			{
				VectorCopy (backEnd.currentEntity->lightDir, viewer);
				VectorNormalizeFast (viewer);
	
				d = DotProduct (normal, viewer);
		
				reflected[xx] = normal[xx]*2*d - viewer[xx];
				reflected[yy] = normal[yy]*2*d - viewer[yy];
		
				st[0] = reflected[xx] * 0.5;
				st[1] = reflected[yy] * 0.5;
			}
		}
		else	// World
		{
			for (i = 0 ; i < tess.numVertexes ; i++, v += 4, normal += 4, st += 2 ) 
			{
				VectorSubtract (backEnd.or.viewOrigin, v, viewer);
				VectorNormalizeFast (viewer);
		
				d = DotProduct (normal, viewer);
		
				reflected[xx] = normal[xx]*2*d - viewer[xx];
				reflected[yy] = normal[yy]*2*d - viewer[yy];
		
				st[0] = reflected[xx] * 0.5;
				st[1] = reflected[yy] * 0.5;
			}
		}
	}
	else	if (mode == 5) // view space
	{
		{
			for (i = 0 ; i < tess.numVertexes ; i++, v += 4, normal += 4, st += 2 ) 
			{
				vec3_t deflected;
				vec4_t deflecteder;
				VectorSubtract (backEnd.or.viewOrigin, v, viewer);
				VectorNormalizeFast (viewer);
		

				R_TransformModelToClip(v, backEnd.or.modelMatrix, backEnd.viewParms.projectionMatrix, deflected, deflecteder );

				VectorNormalizeFast(deflecteder);

				st[0] = 0.5 + deflecteder[0] * 0.5;
				st[1] = 0.5 + deflecteder[1] * 0.5;
			}
		}
	}

	else
	{
		if ((backEnd.currentEntity->e.renderfx & RF_FIRST_PERSON && mode == 2) || ( ( backEnd.currentEntity != &tr.worldEntity ) && mode == 3)) // local light sine
		{
			for (i = 0 ; i < tess.numVertexes ; i++, v += 4, normal += 4, st += 2 ) 
			{
				VectorCopy (backEnd.currentEntity->lightDir, viewer);
				VectorNormalizeFast (viewer);
	
				d = DotProduct (normal, viewer);
		
				reflected[xx] = normal[xx]*2*d - viewer[xx];
				reflected[yy] = normal[yy]*2*d - viewer[yy];
		
				st[0] = 0.5 + reflected[xx] * 0.5;
				st[1] = 0.5 - reflected[yy] * 0.5;
			}
		}
		for (i = 0 ; i < tess.numVertexes ; i++, v += 4, normal += 4, st += 2 ) 
		{
			VectorSubtract (backEnd.or.viewOrigin, v, viewer);
			VectorNormalizeFast (viewer);
	
			d = DotProduct (normal, viewer);
	
			reflected[xx] = normal[xx]*2*d - viewer[xx];
			reflected[yy] = normal[yy]*2*d - viewer[yy];
	
			st[0] = 0.5 + reflected[xx] * 0.5;
			st[1] = 0.5 - reflected[yy] * 0.5;
		}
	}
}

/*
** RB_CalcTurbulentTexCoords
*/
void RB_CalcTurbulentTexCoords( const waveForm_t *wf, float *st )
{
	int i;
	float now;

	now = ( wf->phase + tess.shaderTime * wf->frequency );

	for ( i = 0; i < tess.numVertexes; i++, st += 2 )
	{
		float s = st[0];
		float t = st[1];

		st[0] = s + tr.sinTable[ ( ( int ) ( ( ( tess.xyz[i][0] + tess.xyz[i][2] )* 1.0/128 * 0.125 + now ) * FUNCTABLE_SIZE ) ) & ( FUNCTABLE_MASK ) ] * wf->amplitude;
		st[1] = t + tr.sinTable[ ( ( int ) ( ( tess.xyz[i][1] * 1.0/128 * 0.125 + now ) * FUNCTABLE_SIZE ) ) & ( FUNCTABLE_MASK ) ] * wf->amplitude;
	}
}

/*
** RB_CalcScaleTexCoords
*/
void RB_CalcScaleTexCoords( const float scale[2], float *st )
{
	int i;

	for ( i = 0; i < tess.numVertexes; i++, st += 2 )
	{
		st[0] *= scale[0];
		st[1] *= scale[1];
	}
}

/*
** RB_CalcScrollTexCoords
*/
void RB_CalcScrollTexCoords( const float scrollSpeed[2], float *st )
{
	int i;
	float timeScale = tess.shaderTime;
	float adjustedScrollS, adjustedScrollT;

	adjustedScrollS = scrollSpeed[0] * timeScale;
	adjustedScrollT = scrollSpeed[1] * timeScale;

	// clamp so coordinates don't continuously get larger, causing problems
	// with hardware limits
	adjustedScrollS = adjustedScrollS - floor( adjustedScrollS );
	adjustedScrollT = adjustedScrollT - floor( adjustedScrollT );

	for ( i = 0; i < tess.numVertexes; i++, st += 2 )
	{
		st[0] += adjustedScrollS;
		st[1] += adjustedScrollT;
	}
}

/*
** RB_CalcTransformTexCoords
*/
void RB_CalcTransformTexCoords( const texModInfo_t *tmi, float *st  )
{
	int i;

	for ( i = 0; i < tess.numVertexes; i++, st += 2 )
	{
		float s = st[0];
		float t = st[1];

		st[0] = s * tmi->matrix[0][0] + t * tmi->matrix[1][0] + tmi->translate[0];
		st[1] = s * tmi->matrix[0][1] + t * tmi->matrix[1][1] + tmi->translate[1];
	}
}

/*
** RB_CalcRotateTexCoords
*/
void RB_CalcRotateTexCoords( float degsPerSecond, float *st )
{
	float timeScale = tess.shaderTime;
	float degs;
	int index;
	float sinValue, cosValue;
	texModInfo_t tmi;

	degs = -degsPerSecond * timeScale;
	index = degs * ( FUNCTABLE_SIZE / 360.0f );

	sinValue = tr.sinTable[ index & FUNCTABLE_MASK ];
	cosValue = tr.sinTable[ ( index + FUNCTABLE_SIZE / 4 ) & FUNCTABLE_MASK ];

	tmi.matrix[0][0] = cosValue;
	tmi.matrix[1][0] = -sinValue;
	tmi.translate[0] = 0.5 - 0.5 * cosValue + 0.5 * sinValue;

	tmi.matrix[0][1] = sinValue;
	tmi.matrix[1][1] = cosValue;
	tmi.translate[1] = 0.5 - 0.5 * sinValue - 0.5 * cosValue;

	RB_CalcTransformTexCoords( &tmi, st );
}




/*
** RB_CalcAtlasTexCoords
*/


// TODO: refactor. There is a loop in there for now

void RB_CalcAtlasTexCoords( const atlas_t *at, float *st )
{
	texModInfo_t tmi;
	int w = (int)at->width;	
	int h = (int)at->height;

	int framex = 0;
	int framey = 0;

	// modes:
	// 0 - static / animated
	// 1 - entity alpha (i.e. cgame rocket smoke)

	if (at->mode == 1)	// follow alpha modulation
	{
		int frametotal = w * h;
		float alha = ((0.25+backEnd.currentEntity->e.shaderRGBA[3]) / (tr.identityLight * 256.0f));
		int framethere = frametotal - ((frametotal * alha));
		framex = 0;
		for(int f=0; f<framethere; f++)
		{
			framex +=1;

			if (framex >= w){
				framey +=1;	// next row!
				framex = 0; // reset column
			}
		}

	}
	else			// static/animated
	{
		//
		// Process frame sequence for animation
		//
		
		{
			int framethere = (tess.shaderTime * at->fps) + at->frame;			

				int f;
				framex = 0;
				for(f=0; f<framethere; f++)
				{
					framex +=1;
	
						if (framex >= w){
							framey +=1;	// next row!
							framex = 0; // reset column
						}
						if (framey >= h){
							framey = 0; // reset row
							framex = 0; // reset column
						}
				}
				

		}
	}

		

	
	//
	// now use that information to alter our coordinates
	//

	tmi.matrix[0][0] = 1.0f / w;
	tmi.matrix[1][0] = 0;
	tmi.matrix[0][1] = 0;
	tmi.translate[0] = ((1.0f / w) * framex);

	tmi.matrix[0][1] = 0;
	tmi.matrix[1][1] = 1.0f / h;
	tmi.translate[1] = ((1.0f / h) * framey);

	RB_CalcTransformTexCoords( &tmi, st );
}


/*
** RB_CalcSpecularAlpha
**
** Calculates specular coefficient and places it in the alpha channel
*/
vec3_t lightOrigin = { -960, 1980, 96 };		// FIXME: track dynamically

void RB_CalcSpecularAlpha( unsigned char *alphas ) {
	int			i;
	float		*v, *normal;
	vec3_t		viewer,  reflected;
	float		l, d;
	int			b;
	vec3_t		lightDir;
	int			numVertexes;

	v = tess.xyz[0];
	normal = tess.normal[0];

	alphas += 3;

	numVertexes = tess.numVertexes;
	for (i = 0 ; i < numVertexes ; i++, v += 4, normal += 4, alphas += 4) {
		float ilength;

		VectorSubtract( lightOrigin, v, lightDir );
//		ilength = Q_rsqrt( DotProduct( lightDir, lightDir ) );
		VectorNormalizeFast( lightDir );

		// calculate the specular color
		d = DotProduct (normal, lightDir);
//		d *= ilength;

		// we don't optimize for the d < 0 case since this tends to
		// cause visual artifacts such as faceted "snapping"
		reflected[0] = normal[0]*2*d - lightDir[0];
		reflected[1] = normal[1]*2*d - lightDir[1];
		reflected[2] = normal[2]*2*d - lightDir[2];

		VectorSubtract (backEnd.or.viewOrigin, v, viewer);
		ilength = Q_rsqrt( DotProduct( viewer, viewer ) );
		l = DotProduct (reflected, viewer);
		l *= ilength;

		if (l < 0) {
			b = 0;
		} else {
			l = l*l;
			l = l*l;
			b = l * 255;
			if (b > 255) {
				b = 255;
			}
		}

		*alphas = b;
	}
}

// This fixed version comes from ZEQ2Lite
void RB_CalcSpecularAlphaNew( unsigned char *alphas ) {
int			i;
	float		*v, *normal;
	vec3_t		viewer,  reflected;
	float		l, d;
	int			b;
	vec3_t		lightDir;
	int			numVertexes;

	v = tess.xyz[0];
	normal = tess.normal[0];

	alphas += 3;

	numVertexes = tess.numVertexes;
	for (i = 0 ; i < numVertexes ; i++, v += 4, normal += 4, alphas += 4) {
		float ilength;

		if ( backEnd.currentEntity == &tr.worldEntity )
			VectorSubtract( lightOrigin, v, lightDir );	// old compatibility with maps that use it on some models
		else
			VectorCopy( backEnd.currentEntity->lightDir, lightDir );

		VectorNormalizeFast( lightDir );

		// calculate the specular color
		d = DotProduct (normal, lightDir);

		// we don't optimize for the d < 0 case since this tends to
		// cause visual artifacts such as faceted "snapping"
		reflected[0] = normal[0]*2*d - lightDir[0];
		reflected[1] = normal[1]*2*d - lightDir[1];
		reflected[2] = normal[2]*2*d - lightDir[2];

		VectorSubtract (backEnd.or.viewOrigin, v, viewer);
		ilength = Q_rsqrt( DotProduct( viewer, viewer ) );
		l = DotProduct (reflected, viewer);
		l *= ilength;

		if (l < 0) {
			b = 0;
		} else {
			l = l*l;
			l = l*l;
			b = l * 255;
			if (b > 255) {
				b = 255;
			}
		}

		*alphas = b;
	}

}



/*
** RB_GlowBlend
**
** leilei - blends a specific color (specified in hex)
*/

static void RB_GlowBlend( unsigned char *colors, int glowcol, int fx )
{
	int				i;
	float			*v, *normal;
	float			incoming;
	vec3_t			lightDir;
	vec3_t			directedLight;
	int				numVertexes;

	directedLight[0] = (glowcol >> 16) & 0xFF;
	directedLight[1] = (glowcol >> 8 ) & 0xFF;
	directedLight[2] = glowcol & 0xFF;
	
	// if we don't have overbrights, we need to beef up the colors
	if (tr.identityLight == 1){
	int f;
		for (f=0;f<3;f++)
		{
			directedLight[f] *=2;
			if (directedLight[f]>255)directedLight[f]=255;
		}
	}	
	
	VectorCopy( backEnd.or.viewOrigin, lightDir );

	v = tess.xyz[0];
	normal = tess.normal[0];
		VectorNormalizeFast( lightDir );

	numVertexes = tess.numVertexes;
	for (i = 0 ; i < numVertexes ; i++, v += 4, normal += 4) {

		if (fx == 1)
			incoming = sin(i+(tess.shaderTime * 12)) * DotProduct (normal, lightDir); // fluctuate!
		else if (fx == 2)
			incoming = sin((tess.shaderTime * 12)) *  DotProduct (normal, lightDir); // pulsate!
		else if (fx == 3){
			incoming = DotProduct (normal, lightDir) * 4.0;			
			incoming = sin(incoming+tess.shaderTime * 16); // wave
			}
		else if (fx == 6)
		{
			float eh, off;	
			vec3_t the;
	
			eh = (v[0] + v[1] + v[2]) * 3;
			off = sin(eh + tess.shaderTime * 5);

			VectorScale( normal, off, the);
			
			incoming = VectorNormalize(the);
			if (colors[i*4+3] < 128) incoming = 0;

			// clamp
			if (incoming > 1) incoming = 1;
			if (incoming < 0) incoming = 0;
	
			// blend the new color over the current color
			colors[i*4+0] = (colors[i*4+0] * (1-incoming)) + ((incoming) * directedLight[0]);
			colors[i*4+1] = (colors[i*4+1] * (1-incoming)) + ((incoming) * directedLight[1]);
			colors[i*4+2] = (colors[i*4+2] * (1-incoming)) + ((incoming) * directedLight[2]);
			return;
		}
		else	// generic glow
			{ 
			incoming = DotProduct (normal, lightDir) * 0.8f;
			}
	
		// clamp
		if (incoming > 1) incoming = 1;
		if (incoming < 0) incoming = 0;

		// blend the new color over the current color
		colors[i*4+0] = (colors[i*4+0] * (incoming)) + ((1-incoming) * directedLight[0]);
		colors[i*4+1] = (colors[i*4+1] * (incoming)) + ((1-incoming) * directedLight[1]);
		colors[i*4+2] = (colors[i*4+2] * (incoming)) + ((1-incoming) * directedLight[2]);
	}
}



/*
** RB_UVColor
**
** leilei - blends a specific color (specified in hex)
*/

static void RB_UVColor( unsigned char *colors, int glowcol, int fx )
{
	int				i;
	float			*v, *normal;
	float			incoming;
	vec3_t			lightDir;
	vec3_t			directedLight;
	float			*texc;
	vec3_t			col;

	int				numVertexes;
	vec3_t		newcol;

	directedLight[0] = (glowcol >> 16) & 0xFF;
	directedLight[1] = (glowcol >> 8 ) & 0xFF;
	directedLight[2] = glowcol & 0xFF;
	

	VectorCopy( backEnd.or.viewOrigin, lightDir );

	v = tess.xyz[0];
	normal = tess.normal[0];
	texc = (float*) tess.texCoords[0];

	numVertexes = tess.numVertexes;
	for (i = 0 ; i < numVertexes ; i++, v += 4, texc+=2 ) {
		int tf;
		// For each mode, deal with fading for towards the edge of a uvmap

		incoming = 0;
		if (fx == 0) // Off the top edge, set it to the color
		{
			if (texc[1] < 0.0f) incoming = 1;
		}
		if (fx == 1) // Fade it 
		{
			if (v[4] < 0.0f) incoming = 255;
		}

		for(tf=0 ; tf<3 ; tf++ )
		{
			newcol[tf] = incoming;

			if (newcol[tf] > 255) newcol[tf] = 255;
			if (newcol[tf] < 0) newcol[tf] = 0;
		}

		// blend the new color over the current color
		colors[i*4+0] = newcol[0];
		colors[i*4+1] = newcol[1];
		colors[i*4+2] = newcol[2];
	}
}




/*
** RB_CalcDiffuseColor
**
** The basic vertex lighting calc
*/

// leilei - with some extra modes (flat, high, rim)

// fastest possible path - but shouldn't be very bright.
static void RB_CalcDiffuseColor_flat( unsigned char *colors )
{
	int				i;
	float			*v, *normal;
	trRefEntity_t	*ent;
	vec3_t			ambientLight;
	vec3_t			directedLight;
	int				numVertexes;
	ent = backEnd.currentEntity;
	VectorCopy( ent->ambientLight, ambientLight );
	VectorCopy( ent->directedLight, directedLight );

	v = tess.xyz[0];
	normal = tess.normal[0];
	numVertexes = tess.numVertexes;

	int r = ri.ftol(ambientLight[0] + directedLight[0]);
	int g = ri.ftol(ambientLight[1] + directedLight[1]);
	int b = ri.ftol(ambientLight[2] + directedLight[2]);

	r = (ambientLight[0] + r) / 2;
	g = (ambientLight[1] + g) / 2;
	b = (ambientLight[2] + b) / 2;

	if (r < ambientLight[0]) r = ambientLight[0];
	if (g < ambientLight[1]) g = ambientLight[1];
	if (b < ambientLight[2]) b = ambientLight[2];

	if ( r > 255 ) 	r = 255;
	if ( g > 255 ) 	g = 255;
	if ( b > 255 ) 	b = 255;

	for (i = 0 ; i < numVertexes ; i++, v += 4, normal += 4) {
		colors[i*4+0] = r;
		colors[i*4+1] = g;
		colors[i*4+2] = b;
		colors[i*4+3] = 255;
	}
}



#if idppc_altivec
static void RB_CalcDiffuseColor_altivec( unsigned char *colors )
{
	int				i;
	float			*v, *normal;
	trRefEntity_t	*ent;
	int				ambientLightInt;
	vec3_t			lightDir;
	int				numVertexes;
	vector unsigned char vSel = VECCONST_UINT8(0x00, 0x00, 0x00, 0xff,
                                               0x00, 0x00, 0x00, 0xff,
                                               0x00, 0x00, 0x00, 0xff,
                                               0x00, 0x00, 0x00, 0xff);
	vector float ambientLightVec;
	vector float directedLightVec;
	vector float lightDirVec;
	vector float normalVec0, normalVec1;
	vector float incomingVec0, incomingVec1, incomingVec2;
	vector float zero, jVec;
	vector signed int jVecInt;
	vector signed short jVecShort;
	vector unsigned char jVecChar, normalPerm;
	ent = backEnd.currentEntity;
	ambientLightInt = ent->ambientLightInt;
	// A lot of this could be simplified if we made sure
	// entities light info was 16-byte aligned.
	jVecChar = vec_lvsl(0, ent->ambientLight);
	ambientLightVec = vec_ld(0, (vector float *)ent->ambientLight);
	jVec = vec_ld(11, (vector float *)ent->ambientLight);
	ambientLightVec = vec_perm(ambientLightVec,jVec,jVecChar);

	jVecChar = vec_lvsl(0, ent->directedLight);
	directedLightVec = vec_ld(0,(vector float *)ent->directedLight);
	jVec = vec_ld(11,(vector float *)ent->directedLight);
	directedLightVec = vec_perm(directedLightVec,jVec,jVecChar);	 

	jVecChar = vec_lvsl(0, ent->lightDir);
	lightDirVec = vec_ld(0,(vector float *)ent->lightDir);
	jVec = vec_ld(11,(vector float *)ent->lightDir);
	lightDirVec = vec_perm(lightDirVec,jVec,jVecChar);	 

	zero = (vector float)vec_splat_s8(0);
	VectorCopy( ent->lightDir, lightDir );

	v = tess.xyz[0];
	normal = tess.normal[0];

	normalPerm = vec_lvsl(0,normal);
	numVertexes = tess.numVertexes;
	for (i = 0 ; i < numVertexes ; i++, v += 4, normal += 4) {
		normalVec0 = vec_ld(0,(vector float *)normal);
		normalVec1 = vec_ld(11,(vector float *)normal);
		normalVec0 = vec_perm(normalVec0,normalVec1,normalPerm);
		incomingVec0 = vec_madd(normalVec0, lightDirVec, zero);
		incomingVec1 = vec_sld(incomingVec0,incomingVec0,4);
		incomingVec2 = vec_add(incomingVec0,incomingVec1);
		incomingVec1 = vec_sld(incomingVec1,incomingVec1,4);
		incomingVec2 = vec_add(incomingVec2,incomingVec1);
		incomingVec0 = vec_splat(incomingVec2,0);
		incomingVec0 = vec_max(incomingVec0,zero);
		normalPerm = vec_lvsl(12,normal);
		jVec = vec_madd(incomingVec0, directedLightVec, ambientLightVec);
		jVecInt = vec_cts(jVec,0);	// RGBx
		jVecShort = vec_pack(jVecInt,jVecInt);		// RGBxRGBx
		jVecChar = vec_packsu(jVecShort,jVecShort);	// RGBxRGBxRGBxRGBx
		jVecChar = vec_sel(jVecChar,vSel,vSel);		// RGBARGBARGBARGBA replace alpha with 255
		vec_ste((vector unsigned int)jVecChar,0,(unsigned int *)&colors[i*4]);	// store color
	}
}
#endif

static void RB_CalcDiffuseColor_scalar( unsigned char *colors )
{
	int				i, j;
	float			*v, *normal;
	float			incoming;
	trRefEntity_t	*ent;
	int				ambientLightInt;
	vec3_t			ambientLight;
	vec3_t			lightDir;
	vec3_t			directedLight;
	int				numVertexes;
	ent = backEnd.currentEntity;
	ambientLightInt = ent->ambientLightInt;
	VectorCopy( ent->ambientLight, ambientLight );
	VectorCopy( ent->directedLight, directedLight );
	VectorCopy( ent->lightDir, lightDir );

	v = tess.xyz[0];
	normal = tess.normal[0];

	numVertexes = tess.numVertexes;
	for (i = 0 ; i < numVertexes ; i++, v += 4, normal += 4) {
		incoming = DotProduct (normal, lightDir);
		if ( incoming <= 0 ) {
			*(int *)&colors[i*4] = ambientLightInt;
			continue;
		} 
		j = ri.ftol(ambientLight[0] + incoming * directedLight[0]);
		if ( j > 255 ) {
			j = 255;
		}
		colors[i*4+0] = j;

		j = ri.ftol(ambientLight[1] + incoming * directedLight[1]);
		if ( j > 255 ) {
			j = 255;
		}
		colors[i*4+1] = j;

		j = ri.ftol(ambientLight[2] + incoming * directedLight[2]);
		if ( j > 255 ) {
			j = 255;
		}
		colors[i*4+2] = j;

		colors[i*4+3] = 255;
	}
}

extern float ProjectRadius( float r, vec3_t location );
void RB_CalcDiffuseColor( unsigned char *colors )
{
#if idppc_altivec
	if (com_altivec->integer) {
		// must be in a seperate function or G3 systems will crash.
		RB_CalcDiffuseColor_altivec( colors );
		return;
	}
#endif


	if (r_shadeMethod->integer == -1)			// Fastest
	{
		RB_CalcDiffuseColor_flat( colors );
	}
	else if (r_shadeMethod->integer == 2)			// mimic UE1 style
	{
		RB_CalcMaterials( colors, 0xFFFFFF, 0x000000, 0xFFFFFF, 0x000000, 128, 255 );
	}
	else if (r_shadeMethod->integer == 3)			// have a little both diffuse and specular for a balanced look
	{
		RB_CalcMaterials( colors, 0xFFFFFF, 0x808080, 0x808080, 0x000000, 128, 255 );
	}
#ifdef BROKEN_SHADELOD // leilei: FIXME this does not work as intended 
	else if ((r_shadeMethod->integer > 150) && (r_shadeMethod->integer < 667)) 			// values in a certain range is an adaptive LOD selection of -1, 0 and 3
	{
		float projectedRadius, fsh, shadescale, radius;
		trRefEntity_t	*ent = backEnd.currentEntity;
		fsh = 1;
		radius = 1;
		if ( ( projectedRadius = ProjectRadius( radius, ent->e.origin ) ) != 0 )
		{
			shadescale = r_shadeMethod->value*-0.2;
			if (shadescale > 666) shadescale = 666;
			fsh = 1.0f - projectedRadius * shadescale;
		}
		if (fsh > 5)
				RB_CalcMaterials( colors, 0xFFFFFF, 0x808080, 0x808080, 0x000000, 128, 255 );
		else if (fsh > 2.5)
				RB_CalcDiffuseColor_scalar( colors );
		else
				RB_CalcDiffuseColor_flat( colors );

	}
#endif	// BROKEN
	else							// standard idtech3 shading
	{
		RB_CalcDiffuseColor_scalar( colors );
	}

	// leilei - glow property
	if ((backEnd.currentEntity->e.glow >= 1337 && backEnd.currentEntity->e.glow <= 1340 )) // look for 1337-1340 as compatibility so we don't get malformed glows from old cgames
	{
		RB_GlowBlend( colors, backEnd.currentEntity->e.glowcol, (backEnd.currentEntity->e.glow - 1337) ); 
	}


}



void RB_CalcGlowBlend( unsigned char *colors, int glowcol, int fx )
{
	RB_GlowBlend( colors, glowcol, fx ); 
}

void RB_CalcUVColor( unsigned char *colors, int glowcol, int fx )
{
	RB_UVColor( colors, glowcol, fx ); 
}


// normalize colors but alpha has the intensity of the colors.
void RB_CalcNormalizeToAlpha( unsigned char *colors)
{
	int				i;
	float			*v, *normal;
	int				numVertexes;

	v = tess.xyz[0];
	normal = tess.normal[0]; // do we even need this

	numVertexes = tess.numVertexes;
	for (i = 0 ; i < numVertexes ; i++, v += 4, normal += 4) {

		colors[i*4+3] = LUMA(colors[i*4+0], colors[i*4+1], colors[i*4+2]); 
		// leilei - boost it up a little more to fight pvr1's subtractive modulation
		//colors[i*4+3] = pow(colors[i*4+3]/255,0.7f)*255;
		{
			int		max;
	
			max = colors[i*4+0] > colors[i*4+1] ? colors[i*4+0] : colors[i*4+1];
			max = max > colors[i*4+2] ? max : colors[i*4+2];
			max += 1;
			colors[i*4+0] = colors[i*4+0] * 255 / max;
			colors[i*4+1] = colors[i*4+1] * 255 / max;
			colors[i*4+2] = colors[i*4+2] * 255 / max;

		}
	}
}



/*
** RB_VertLightsPerVert
**
** leilei - for maps, try to light up vertex lighting with dynamic lights
*/


static void RB_VertLightsPerVert( unsigned char *colors )
{
	int				i, f;
	float			*v, *normal;
	int				numVertexes;

	vec3_t cl;
	vec3_t cv;

	v = tess.xyz[0];
	normal = tess.normal[0];

	numVertexes = tess.numVertexes;
	for (i = 0 ; i < numVertexes ; i++, v += 4, normal += 4) {
		// get the world colors
		cl[0] = colors[i*4+0];
		cl[1] = colors[i*4+1];
		cl[2] = colors[i*4+2];


	if(r_dynamiclight->integer)
		{
			for (f=0 ; f<backEnd.refdef.num_dlights ; f++) 
			{
				dlight_t		*l;
				float r, g, b;
				l = &backEnd.refdef.dlights[f];
				r = l->color[0]; g = l->color[1]; b = l->color[2];
				
					if (l->radius >= 1) // if light is good, mark the map
					{
							// calc the light against the world verts
							VectorSubtract( l->origin, v, cv );
							float power = 2048 * ( l->radius );
							float d = VectorNormalize( cv );
							power = power / ( d * d );
							if (power < 0) power = 0;
						
							// add the color of the light
							cl[0] += (r * power);
							cl[1] += (g * power);
							cl[2] += (b * power);
						
							// clamp stuff
							// normalize by color instead of saturating to white
							if ( ( (int)cl[0] | (int)cl[1] | (int)cl[2] ) > 255 ) {
								int		max;
						
								max = cl[0] > cl[1] ? cl[0] : cl[1];
								max = max > cl[2] ? max : cl[2];
								cl[0] = cl[0] * 255 / max;
								cl[1] = cl[1] * 255 / max;
								cl[2] = cl[2] * 255 / max;
							}
							if (cl[0] < 0) cl[0] = 0;
							if (cl[1] < 0) cl[1] = 0;
							if (cl[2] < 0) cl[2] = 0;
					}
			}
		}
		// give the world our new modulated color
		colors[i*4+0] = cl[0];
		colors[i*4+1] = cl[1];
		colors[i*4+2] = cl[2];

	}
}


void RB_CalcVertLights( unsigned char *colors )
{
	RB_VertLightsPerVert( colors ); 
}

#define 	MAXFRESLIGHTS 	512

/*
** RB_CalcMaterialColor
**
** leilei - for more control on diffuse/ambient/emiss/specular colors rather than diffuse only. Possibly could use some SIMD magic here
*/

static void RB_CalcMaterialColor( unsigned char *colors, int maxl, int ambient, int diffuse, int specular, int emissive, int spechard, int alpha )
{
	int				i, j, l;
	float			*v, *normal;
	trRefEntity_t	*ent;
	vec3_t			ambientLight;
	int			numVertexes;
	vec3_t			lorigin;

	vec3_t			matAmb, matDif, matSpec, matEmis;
	float			matHard = spechard / 128.0f;
	int		specenabled = 0;


	if (spechard == 128)
		matHard = 1;

	if (specular)
		specenabled = 1;

	// Parse material colors

	matAmb[0] = ((ambient >> 16) & 0xFF);
	matAmb[1] = ((ambient >> 8 ) & 0xFF);
	matAmb[2] = (ambient & 0xFF);

	matDif[0] = ((diffuse >> 16) & 0xFF);
	matDif[1] = ((diffuse >> 8 ) & 0xFF);
	matDif[2] = (diffuse & 0xFF);

	matSpec[0] = ((specular >> 16) & 0xFF);
	matSpec[1] = ((specular >> 8 ) & 0xFF);
	matSpec[2] = (specular & 0xFF);

	matEmis[0] = (emissive >> 16) & 0xFF;
	matEmis[1] = (emissive >> 8 ) & 0xFF;
	matEmis[2] = emissive & 0xFF;

	VectorNormalize( matAmb );
	VectorNormalize( matDif );
	VectorNormalize( matSpec );
	VectorNormalize( matEmis );

	// Overbright clamp

	matEmis[0] *= tr.identityLight;
	matEmis[1] *= tr.identityLight;
	matEmis[2] *= tr.identityLight;

	// setup ent

	ent = backEnd.currentEntity;

	if ( ent->e.renderfx & RF_LIGHTING_ORIGIN ) 
		VectorCopy( ent->e.lightingOrigin, lorigin );
	else
		VectorCopy( ent->e.origin, lorigin );


	VectorCopy( ent->ambientLight, ambientLight );

	int		liteOn[maxl];
	vec3_t		liteOrg[maxl];
	vec3_t		liteCol[maxl];
	int		active = 1;

	if (maxl > MAXFRESLIGHTS) maxl = MAXFRESLIGHTS;

	// first light is always the world
	liteOn[0] = 1;
	VectorCopy( ent->directedLight, liteCol[0] );
	VectorCopy( ent->lightDir, liteOrg[0] );

	ambientLight[0] *= matAmb[0]*2;
	ambientLight[1] *= matAmb[1]*2;
	ambientLight[2] *= matAmb[2]*2;

	

	if (maxl>1)
	{
		// add dynamic lights
		{
			dlight_t		*l;

			for (i=0 ; i<backEnd.refdef.num_dlights ; i++, l++) {

				if (active>maxl)
				continue;

				l = &backEnd.refdef.dlights[i];

				VectorCopy( l->color, liteCol[active] );

				VectorSubtract( l->origin, lorigin, liteOrg[active] );

				// attenuation
				float d = VectorNormalize( liteOrg[active] );
				float power = 16 * ( l->radius );
				power = power / ( d * d );
				if (power < 0) power = 0;
				if (power > 1) power = 1;	// needed to prevent powerup light overflow, as quad/flagcarriers players should not glow themselves

				// transform
				liteOrg[active][0] = DotProduct( liteOrg[active], ent->e.axis[0] );
				liteOrg[active][1] = DotProduct( liteOrg[active], ent->e.axis[1] );
				liteOrg[active][2] = DotProduct( liteOrg[active], ent->e.axis[2] );

				liteCol[active][0] *= (255 * power);
				liteCol[active][1] *= (255 * power);
				liteCol[active][2] *= (255 * power);

				liteOn[active] = 1;
				active++;
	
			}
		}
	}


	// Set up our model
	v = tess.xyz[0];
	normal = tess.normal[0];

	numVertexes = tess.numVertexes;


	for (i = 0 ; i < numVertexes ; i++, v += 4, normal += 4) {
		vec3_t difs;
		vec3_t specs;

		// clear
		difs[0] 	= difs[1] 	= difs[2] = 0;
		specs[0] 	= specs[1] 	= specs[2] = 0;

		// HACK - Fix the world light
		VectorCopy( ent->lightDir, liteOrg[0] );

		// go through lights and add them
		for (l=0;l<active;l++)
		{
			vec3_t reflected, viewer;
			float al;		
			float b, c;
			float ilength = Q_rsqrt( DotProduct( liteOrg[l], liteOrg[l] ) );
			if (!liteOn[l])
				continue;
			// calculate the specular color
			float d = DotProduct (normal, liteOrg[l]);
			c = d; // get the diffuse

			if (specenabled)
			{
				d *= ilength;
				reflected[0] = normal[0]*2*d - liteOrg[l][0];
				reflected[1] = normal[1]*2*d - liteOrg[l][1];
				reflected[2] = normal[2]*2*d - liteOrg[l][2];
	
				VectorSubtract (backEnd.or.viewOrigin, v, viewer);
				ilength = Q_rsqrt( DotProduct( viewer, viewer ) );
				al = DotProduct (reflected, viewer);
				al *= ilength;
		
				// TODO: Metallic surface property option?
				//al *= (al);
				
				if (al < 0) {
					b = 0;
				} else {
					if (matHard != 1){
					al *= (al*matHard);
					al *= (al*matHard);
					}
					b = al;
					if (b > 1) {
						b = 1;
					}
				}
	
				specs[0] += b * liteCol[l][0];
				specs[1] += b * liteCol[l][1];
				specs[2] += b * liteCol[l][2];
			}
	
			if (c<0) c=0;
			if (c>255) c=255;
			if (liteOn[l] != 2){
			difs[0] += c * liteCol[l][0];
			difs[1] += c * liteCol[l][1];
			difs[2] += c * liteCol[l][2];
			}
			
		}

		// Add Ambient and CLAMP
		j = ri.ftol(ambientLight[0] + (difs[0] * matDif[0]) + (specs[0] * matSpec[0]));
		if ( j > 255 ) {
			j = 255;
		}
		colors[i*4+0] = j;

		j = ri.ftol(ambientLight[1] + (difs[1] * matDif[1]) + (specs[1] * matSpec[1]));
		if ( j > 255 ) {
			j = 255;
		}
		colors[i*4+1] = j;

		j = ri.ftol(ambientLight[2] + (difs[2] * matDif[2]) + (specs[2] * matSpec[2]));
		if ( j > 255 ) {
			j = 255;
		}
		colors[i*4+2] = j;

		colors[i*4+3] = alpha;
	}
}

void RB_CalcMaterials( unsigned char *colors, int ambient, int diffuse, int specular, int emissive, int spechard, int alpha )
{
	// TODO: Low detail materials
	RB_CalcMaterialColor( colors, 1, ambient, diffuse, specular, emissive, spechard, alpha );

	// leilei - glow property
	if ((backEnd.currentEntity->e.glow >= 1337 && backEnd.currentEntity->e.glow <= 1340 )) // look for 1337-1340 as compatibility so we don't get malformed glows from old cgames
	{
		RB_GlowBlend( colors, backEnd.currentEntity->e.glowcol, (backEnd.currentEntity->e.glow - 1337) ); 
	}
}






//
// EYES
//

vec3_t eyemin = { -12, -12, -8 };		// clamps
vec3_t eyemax = { 12, 12, 8 };		// clamps

/*
** RB_CalcEyes
*/



void RB_CalcEyes( float *st, qboolean theothereye) 
{
	int			i;
	float		*v, *normal;
	vec3_t		viewer, reflected, eyepos, stare;
	float		d;
	int	idk;

	vec3_t		stareat;

	VectorCopy(backEnd.or.viewOrigin, stareat);




	// transform the direction to local space
//	VectorNormalize( staree );
//	stareat[0] = DotProduct( staree, backEnd.currentEntity->e.axis[0] );
//	stareat[1] = DotProduct( staree, backEnd.currentEntity->e.axis[1] );
//	stareat[2] = DotProduct( staree, backEnd.currentEntity->e.axis[2] );
//	VectorNormalize( stareat );
	v = tess.xyz[0];
	normal = tess.normal[0];



	//VectorCopy(lightOrigin, eyepos);
	//normal = backEnd.currentEntity.eyepos[0];
	VectorCopy(backEnd.currentEntity->e.eyepos[0], eyepos);

	if (!theothereye){
		eyepos[1] *= -1;
		}

	VectorCopy(backEnd.currentEntity->e.eyelook, stareat);


	// We need to adjust the stareat vectors to local coordinates


	vec3_t temp;
	VectorSubtract( stareat, backEnd.currentEntity->e.origin, temp );
	stareat[0] = DotProduct( temp, backEnd.currentEntity->e.axis[0] );
	stareat[1] = DotProduct( temp, backEnd.currentEntity->e.axis[1] );
	stareat[2] = DotProduct( temp, backEnd.currentEntity->e.axis[2] );




// debug light positions
if (r_leidebugeye->integer == 2)
	{
	vec3_t	temp;
	vec3_t	temp2;


	VectorCopy(eyepos, temp);
	VectorCopy(backEnd.currentEntity->e.eyelook, temp2);
//	VectorCopy(backEnd.currentEntity->e.eyelook, stareat);

	ri.Printf( PRINT_WARNING, "EYES %f %f %f--\nVieworigin:%f %f %f \nEye look desired:%f %f %f\n", temp[0], temp[1], temp[2], backEnd.or.viewOrigin[0], backEnd.or.viewOrigin[1], backEnd.or.viewOrigin[2], stareat[0], stareat[1], stareat[2] );


	VectorNormalize(temp2);

	GL_Bind( tr.whiteImage );
	qglColor3f (1,1,1);
	qglDepthRange( 0, 0 );	// never occluded
	GL_State( GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE );
	qglBegin (GL_LINES);
	qglVertex3fv (eyepos);
	qglVertex3fv (stareat);
	qglEnd ();
	qglDepthRange( 0, 1 );
	}


	for (i = 0 ; i < tess.numVertexes ; i++, v += 4, normal += 4, st += 2 ) 
	{
		// Base eye position
		VectorSubtract (backEnd.or.viewOrigin, v, viewer);
		//VectorSubtract (backEnd.currentEntity->e.eyepos[0], v, viewer);

		VectorSubtract (eyepos, v, viewer);

		VectorNormalizeFast (viewer);

		

		d = DotProduct (normal, viewer);
		d = d * 0.01f;	// only have a slight normal
		//d = r_leidebug->value;

		//d = 1;
		// Stuff to look at


		//VectorSubtract (backEnd.currentEntity->e.eyelook, v, stare);

		if (r_leidebugeye->integer==1)
		VectorSubtract (backEnd.or.viewOrigin, v, stare);
		else if (r_leidebugeye->integer==3)
		VectorSubtract (stareat, v, stare);




		VectorSubtract (stareat, v, stare);
		VectorNormalizeFast (stare);

		// Limit the eye's turning so it doesn't have dead eyes
		for (idk=0;idk<3;idk++){
			stare[idk] *=  22;
			if (stare[idk] > eyemax[idk]) stare[idk] = eyemax[idk];
			if (stare[idk] < eyemin[idk]) stare[idk] = eyemin[idk];
			stare[idk] /=  22;
		}		
		VectorAdd(viewer, stare, viewer);
		
	
		reflected[0] = normal[0]*2*d - viewer[0];
		reflected[1] = normal[1]*2*d - viewer[1];
		reflected[2] = normal[2]*2*d - viewer[2];

		st[0] = 0.5 + reflected[1] * 0.5;
		st[1] = 0.5 - reflected[2] * 0.5;

	}

}

