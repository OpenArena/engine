
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


#ifndef TR_LOCAL_H
#define TR_LOCAL_H

#include "../qcommon/q_shared.h"
#include "../qcommon/qfiles.h"
#include "../qcommon/qcommon.h"

#include "../renderercommon/tr_public.h"
#include "../renderercommon/tr_common.h"
#ifdef BROKEN_IQM
#include "../renderercommon/iqm.h"
#endif
#include "../renderercommon/qgl.h"

#define GL_INDEX_TYPE		GL_UNSIGNED_INT
typedef unsigned int glIndex_t;

// Lousy hack
#define GLSL_POSTPROCESSING 1
#define GLSL_TEXTURES 1
#define GLSL_BACKEND 1

// 14 bits
// can't be increased without changing bit packing for drawsurfs
// see QSORT_SHADERNUM_SHIFT
#define SHADERNUM_BITS	14
#define MAX_SHADERS		(1<<SHADERNUM_BITS)

extern qboolean    textureCompressionSupport;	// leilei - compressed texture
extern qboolean    palettedTextureSupport;	// leilei - paletted texture

typedef struct dlight_s {
	vec3_t	origin;
	vec3_t	color;				// range from 0.0 to 1.0, should be color normalized
	float	radius;

	vec3_t	transformed;		// origin in local coordinate system
	int		additive;			// texture detail is lost tho when the lightmap is dark
} dlight_t;


// leilei - crazy MDR physics bones
#define MAX_MDR_PHYSICS 32

#define MDP_SPRING 	1
#define MDP_JIGGLE 	2	
#define MDP_HAIR   	3
#define MDP_SCALESET	4
#define MDP_LINK	5 // points targbone to targbone2
#define MDP_INHERIT	6 // takes origin/angle change from targbone to targbone2
#define MDP_DEBUG  	7

#ifdef BROKEN_MDRPHYS
typedef struct mdrPhysics_s {
	int 		targbone;	// Bone to target
	int 		type;		// type of physic
	float 		spring;
	vec3_t		limitl; 	// length limit for spring
	vec3_t		limitr;		// rotateable limit for spring
	int		scaleset;	// which set of scale to use
	vec3_t		scale1;		// For scaleset
	vec3_t		scale2;		// For scaleset
	int 		suggestive; 	// user disableable
} mdrPhysics_t;
	
// For Model_t's to track their properties of each physic and the bone they modify
typedef struct mdrPhys_s {
	mdrPhysics_t	p[MAX_MDR_PHYSICS];
	int		maxphs;
	int		modelindex;
	//model_t		*mod;
	int		mdln;
	int		inited;
} mdrPhys_t;

// For RefEntity_t's to track their bones that are being moved
typedef struct mdrPhysLocal_s
{
	vec3_t	oldorg[MDR_MAX_BONES];
	vec3_t 	neworg[MDR_MAX_BONES];
	vec3_t	oldmorg[MDR_MAX_BONES];
	vec3_t 	newmorg[MDR_MAX_BONES];
	vec3_t	vel[MDR_MAX_BONES];
	int	time[MDR_MAX_BONES];			// when to iterate physics
	int	modindex;			// hack to verify if it's the same model getting physics?
} mdrPhysLocal_t;

#endif

// a trRefEntity_t has all the information passed in by
// the client game, as well as some locally derived info
typedef struct {
	refEntity_t	e;

	float		axisLength;		// compensate for non-normalized axis

	qboolean	needDlights;	// true for bmodels that touch a dlight
	qboolean	lightingCalculated;
	vec3_t		lightDir;		// normalized direction towards light
	vec3_t		ambientLight;	// color normalized to 0-255
	int			ambientLightInt;	// 32 bit rgba packed
	vec3_t		directedLight;
	vec3_t		dynamicLight;
	float		lightDistance;

#ifdef BROKEN_MDRPHYS
	// leilei - local mdr physics for this ent
	vec3_t		bonfs;
	vec3_t		olorg;
	vec3_t		nuorg;
	mdrPhys_t	*phy;
	mdrPhysLocal_t	pl;
#endif
} trRefEntity_t;


typedef struct {
	vec3_t		origin;			// in world coordinates
	vec3_t		axis[3];		// orientation in world
	vec3_t		viewOrigin;		// viewParms->or.origin in local coordinates
	float		modelMatrix[16];
} orientationr_t;

//===============================================================================

typedef enum {
	SS_BAD,
	SS_PORTAL,			// mirrors, portals, viewscreens
	SS_ENVIRONMENT,		// sky box
	SS_OPAQUE,			// opaque

	SS_DECAL,			// scorch marks, etc.
	SS_SEE_THROUGH,		// ladders, grates, grills that may have small blended edges
						// in addition to alpha test
	SS_BANNER,

	SS_FOG,

	SS_UNDERWATER,		// for items that should be drawn in front of the water plane

	SS_BLEND0,			// regular transparency and filters
	SS_BLEND1,			// generally only used for additive type effects
	SS_BLEND2,
	SS_BLEND3,

	SS_BLEND6,
	SS_STENCIL_SHADOW,
	SS_ALMOST_NEAREST,	// gun smoke puffs

	SS_NEAREST			// blood blobs
} shaderSort_t;


#define MAX_SHADER_STAGES 8

typedef enum {
	GF_NONE,

	GF_SIN,
	GF_SQUARE,
	GF_TRIANGLE,
	GF_SAWTOOTH, 
	GF_INVERSE_SAWTOOTH, 

	GF_NOISE

} genFunc_t;


typedef enum {
	DEFORM_NONE,
	DEFORM_WAVE,
	DEFORM_NORMALS,
	DEFORM_BULGE,
	DEFORM_MOVE,
	DEFORM_PROJECTION_SHADOW,
	DEFORM_AUTOSPRITE,
	DEFORM_AUTOSPRITE2,
	DEFORM_TEXT0,
	DEFORM_TEXT1,
	DEFORM_TEXT2,
	DEFORM_TEXT3,
	DEFORM_TEXT4,
	DEFORM_TEXT5,
	DEFORM_TEXT6,
	DEFORM_TEXT7,
	DEFORM_LFX
} deform_t;

typedef enum {
	AGEN_IDENTITY,
	AGEN_SKIP,
	AGEN_ENTITY,
	AGEN_ONE_MINUS_ENTITY,
	AGEN_VERTEX,
	AGEN_ONE_MINUS_VERTEX,
	AGEN_LIGHTING_SPECULAR,
	AGEN_WAVEFORM,
	AGEN_PORTAL,
	AGEN_CONST
} alphaGen_t;

typedef enum {
	CGEN_BAD,
	CGEN_IDENTITY_LIGHTING,	// tr.identityLight
	CGEN_IDENTITY,			// always (1,1,1,1)
	CGEN_ENTITY,			// grabbed from entity's modulate field
	CGEN_ONE_MINUS_ENTITY,	// grabbed from 1 - entity.modulate
	CGEN_EXACT_VERTEX,		// tess.vertexColors
	CGEN_VERTEX,			// tess.vertexColors * tr.identityLight
	CGEN_ONE_MINUS_VERTEX,
	CGEN_WAVEFORM,			// programmatically generated
	CGEN_LIGHTING_DIFFUSE,
	CGEN_LIGHTING_UNIFORM,
	CGEN_LIGHTING_DYNAMIC,
	CGEN_LIGHTING_FLAT_AMBIENT,		// leilei - cel hack
	CGEN_LIGHTING_FLAT_DIRECT,
	CGEN_FOG,				// standard fog
	CGEN_CONST,				// fixed color
	CGEN_VERTEX_LIT,			// leilei - tess.vertexColors * tr.identityLight * ambientlight*directlight
	CGEN_MATERIAL,				// leilei - material system
	CGEN_LIGHTING_DIFFUSE_SPECULAR		// leilei - LIGHTING_DIFFUSE, capped by specular exponent
} colorGen_t;

typedef enum {
	TCGEN_BAD,
	TCGEN_IDENTITY,			// clear to 0,0
	TCGEN_LIGHTMAP,
	TCGEN_TEXTURE,
	TCGEN_ENVIRONMENT_MAPPED,
	TCGEN_ENVIRONMENT_MAPPED_WATER,	// leilei - fake water reflection
	TCGEN_EYE_LEFT,				// eyes
	TCGEN_EYE_RIGHT,			// eyes
	TCGEN_FOG,
	TCGEN_VECTOR			// S and T from world coordinates
} texCoordGen_t;

typedef enum {
	ACFF_NONE,
	ACFF_MODULATE_RGB,
	ACFF_MODULATE_RGBA,
	ACFF_MODULATE_ALPHA
} acff_t;

typedef struct {
	genFunc_t	func;

	float base;
	float amplitude;
	float phase;
	float frequency;
} waveForm_t;

typedef struct {
	int    lfxtype; // ... later
/*
	int amount;
	int radiusmode;
	float radius

	float minsize
	float maxsize
	float r
	vec3_t vel;
	vec3_t org;
	float spread;
	int blend;
	int shader;
*/
} lfx_t;


// leilei - texture atlases
typedef struct {
	float width;			// columns
	float height;			// rows
	float fps;			// frames per second
	int frame;			// offset frame
	float mode;			// 0 - static/anim  1 - entityalpha
} atlas_t;


#define TR_MAX_TEXMODS 4

typedef enum {
	TMOD_NONE,
	TMOD_TRANSFORM,
	TMOD_TURBULENT,
	TMOD_SCROLL,
	TMOD_SCALE,
	TMOD_STRETCH,
	TMOD_LIGHTSCALE,		// leilei - cel hack
	TMOD_ATLAS,			// leilei - atlases
	TMOD_ROTATE,
	TMOD_ENTITY_TRANSLATE
} texMod_t;

// leilei - rgbMod - color modulations
typedef enum {
	CMOD_BAD,
	CMOD_GLOW,
	CMOD_LIGHTING,
	CMOD_NORMALIZETOALPHA,
	CMOD_NORMALIZETOALPHAFAST,
	CMOD_UVCOL,
	CMOD_OPAQUE
} colorMod_t;

#define	MAX_SHADER_DEFORMS	3
typedef struct {
	deform_t	deformation;			// vertex coordinate modification type

	vec3_t		moveVector;
	waveForm_t	deformationWave;
	float		deformationSpread;

	float		bulgeWidth;
	float		bulgeHeight;
	float		bulgeSpeed;

	lfx_t		deformationLfx;
} deformStage_t;


typedef struct {
	texMod_t		type;

	// used for TMOD_TURBULENT and TMOD_STRETCH
	waveForm_t		wave;

	// used for TMOD_TRANSFORM
	float			matrix[2][2];		// s' = s * m[0][0] + t * m[1][0] + trans[0]
	float			translate[2];		// t' = s * m[0][1] + t * m[0][1] + trans[1]

	// used for TMOD_SCALE
	float			scale[2];			// s *= scale[0]
	                                    // t *= scale[1]

	// used for TMOD_SCROLL
	float			scroll[2];			// s' = s + scroll[0] * time
										// t' = t + scroll[1] * time
	// leilei - used for TMOD_ATLAS
	atlas_t			atlas;

	// + = clockwise
	// - = counterclockwise
	float			rotateSpeed;

} texModInfo_t;


#define	MAX_IMAGE_ANIMATIONS	16 // leilei - was 8

typedef struct {
	image_t			*image[MAX_IMAGE_ANIMATIONS];
	int				numImageAnimations;
	float			imageAnimationSpeed;

	texCoordGen_t	tcGen;
	vec3_t			tcGenVectors[2];

	int				numTexMods;
	texModInfo_t	*texMods;

	int				videoMapHandle;
	qboolean		isLightmap;
	qboolean		isVideoMap;

	// leilei - alphahack
	
	char			*texname;
	int			alphahack;
} textureBundle_t;

#define NUM_TEXTURE_BUNDLES 16 // leilei - was 8, increased for motion blur

typedef struct {
	qboolean		active;
	
	textureBundle_t	bundle[NUM_TEXTURE_BUNDLES];

	waveForm_t		rgbWave;
	colorGen_t		rgbGen;

	waveForm_t		alphaWave;
	alphaGen_t		alphaGen;

	byte			constantColor[4];			// for CGEN_CONST and AGEN_CONST

	unsigned		stateBits;					// GLS_xxxx mask

	acff_t			adjustColorsForFog;

	qboolean		isDetail;
	int			mipBias;

	int			isGLSL;
	int			isBlend;			// leilei - for leifx
	qhandle_t		program;

	int			imgWidth;
	int			imgHeight;		//leilei for glsl shaders

	colorMod_t		rgbMod;			// leilei - rgbMod
	int			rgbModCol;
	int			rgbModMode;

	int			matAmb;			// leilei - material ambience
	int			matDif;			// leilei - material diffuse
	int			matSpec;		// leilei - material specular
	int			matEmis;		// leilei - material emissive
	int			matHard;		// leilei - material specular hardness
	int			matAlpha;		// leilei - material alpha

} shaderStage_t;

struct shaderCommands_s;

typedef enum {
	CT_FRONT_SIDED,
	CT_BACK_SIDED,
	CT_TWO_SIDED
} cullType_t;

typedef enum {
	FP_NONE,		// surface is translucent and will just be adjusted properly
	FP_EQUAL,		// surface is opaque but possibly alpha tested
	FP_LE			// surface is trnaslucent, but still needs a fog pass (fog surface)
} fogPass_t;

typedef struct {
	float		cloudHeight;
	image_t		*outerbox[6], *innerbox[6];
} skyParms_t;

typedef struct {
	vec3_t	color;
	float	depthForOpaque;
} fogParms_t;


typedef struct shader_s {
	char		name[MAX_QPATH];		// game path, including extension
	int			lightmapIndex;			// for a shader to match, both name and lightmapIndex must match

	int			index;					// this shader == tr.shaders[index]
	int			sortedIndex;			// this shader == tr.sortedShaders[sortedIndex]

	float		sort;					// lower numbered shaders draw before higher numbered

	qboolean	defaultShader;			// we want to return index 0 if the shader failed to
										// load for some reason, but R_FindShader should
										// still keep a name allocated for it, so if
										// something calls RE_RegisterShader again with
										// the same name, we don't try looking for it again

	qboolean	explicitlyDefined;		// found in a .shader file

	int			surfaceFlags;			// if explicitlyDefined, this will have SURF_* flags
	int			contentFlags;

	qboolean	entityMergable;			// merge across entites optimizable (smoke, blood)

	qboolean	isSky;
	skyParms_t	sky;
	fogParms_t	fogParms;

	float		portalRange;			// distance to fog out at

	int			multitextureEnv;		// 0, GL_MODULATE, GL_ADD (FIXME: put in stage)

	cullType_t	cullType;				// CT_FRONT_SIDED, CT_BACK_SIDED, or CT_TWO_SIDED
	qboolean	polygonOffset;			// set for decals and other items that must be offset 
	qboolean	noMipMaps;				// for console fonts, 2D elements, etc.
	qboolean	noPicMip;				// for images that must always be full resolution

	fogPass_t	fogPass;				// draw a blended pass, possibly with depth test equals

	qboolean	needsNormal;			// not all shaders will need all data to be gathered
	qboolean	needsST1;
	qboolean	needsST2;
	qboolean	needsColor;

	//	leilei - automatic detail texturing
	int		hasDetail;	// shader has a detail stage
	int		hasDepthWrite;	// shader has a depthwrite stage (detailing around holes)
	int		hasMaterial;	// shader represents this material

	int			numDeforms;
	deformStage_t	deforms[MAX_SHADER_DEFORMS];

	int			numUnfoggedPasses;
	shaderStage_t	*stages[MAX_SHADER_STAGES];		

	void		(*optimalStageIteratorFunc)( void );

  float clampTime;                                  // time this shader is clamped to
  float timeOffset;                                 // current time offset for this shader

  struct shader_s *remappedShader;                  // current shader this one is remapped too

	struct	shader_s	*next;
} shader_t;


// leilei - shader materials for detail texturing

#define		SHADMAT_GENERIC 	0	// none
#define		SHADMAT_METAL	 	1	// from metalsteps
#define		SHADMAT_WOOD	 	2	// ql
#define		SHADMAT_FLESH	 	3	// unused
#define		SHADMAT_SAND	 	4	// from dust?
#define		SHADMAT_SNOW	 	5	// unused
#define		SHADMAT_EARTH	 	6	// unused
#define		SHADMAT_CONCRETE	7 	// unused? redundant?
#define		SHADMAT_ICE		8	// from slick

// trRefdef_t holds everything that comes in refdef_t,
// as well as the locally generated scene information
typedef struct {
	int			x, y, width, height;
	float		fov_x, fov_y;
	vec3_t		vieworg;
	vec3_t		viewaxis[3];		// transformation matrix

	stereoFrame_t	stereoFrame;

	int			time;				// time in milliseconds for shader effects and other time dependent rendering issues
	int			rdflags;			// RDF_NOWORLDMODEL, etc

	// 1 bits will prevent the associated area from rendering at all
	byte		areamask[MAX_MAP_AREA_BYTES];
	qboolean	areamaskModified;	// qtrue if areamask changed since last scene

	float		floatTime;			// tr.refdef.time / 1000.0

	// text messages for deform text shaders
	char		text[MAX_RENDER_STRINGS][MAX_RENDER_STRING_LENGTH];

	int			num_entities;
	trRefEntity_t	*entities;

	int			num_dlights;
	struct dlight_s	*dlights;

	int			numPolys;
	struct srfPoly_s	*polys;

	int			numDrawSurfs;
	struct drawSurf_s	*drawSurfs;

} trRefdef_t;


//=================================================================================

// skins allow models to be retextured without modifying the model file
typedef struct {
	char		name[MAX_QPATH];
	shader_t	*shader;
} skinSurface_t;

typedef struct skin_s {
	char		name[MAX_QPATH];		// game path, including extension
	int			numSurfaces;
	skinSurface_t	*surfaces[MD3_MAX_SURFACES];
} skin_t;


typedef struct {
	int			originalBrushNumber;
	vec3_t		bounds[2];

	unsigned	colorInt;				// in packed byte format
	float		tcScale;				// texture coordinate vector scales
	fogParms_t	parms;

	// for clipping distance in fog when outside
	qboolean	hasSurface;
	float		surface[4];
} fog_t;

typedef struct {
	orientationr_t	or;
	orientationr_t	world;
	vec3_t		pvsOrigin;			// may be different than or.origin for portals
	qboolean	isPortal;			// true if this view is through a portal
	qboolean	isMirror;			// the portal is a mirror, invert the face culling
	int			frameSceneNum;		// copied from tr.frameSceneNum
	int			frameCount;			// copied from tr.frameCount
	cplane_t	portalPlane;		// clip anything behind this if mirroring
	int			viewportX, viewportY, viewportWidth, viewportHeight;
	float		fovX, fovY;
	float		projectionMatrix[16];
	cplane_t	frustum[4];
	vec3_t		visBounds[2];
	float		zFar;
	stereoFrame_t	stereoFrame;
} viewParms_t;


/*
==============================================================================

SURFACES

==============================================================================
*/

// any changes in surfaceType must be mirrored in rb_surfaceTable[]
typedef enum {
	SF_BAD,
	SF_SKIP,				// ignore
	SF_FACE,
	SF_GRID,
	SF_TRIANGLES,
	SF_POLY,
	SF_MD3,
	SF_MDR,
#ifdef BROKEN_MDRPHYS
	SF_MDP,
#endif
#ifdef BROKEN_IQM
	SF_IQM,
#endif
	SF_FLARE,
	SF_ENTITY,				// beams, rails, lightning, etc that can be determined by entity
	SF_DISPLAY_LIST,

	SF_NUM_SURFACE_TYPES,
	SF_MAX = 0x7fffffff			// ensures that sizeof( surfaceType_t ) == sizeof( int )
} surfaceType_t;

typedef struct drawSurf_s {
	unsigned			sort;			// bit combination for fast compares
	surfaceType_t		*surface;		// any of surface*_t
} drawSurf_t;

#define	MAX_FACE_POINTS		64

#define	MAX_PATCH_SIZE		32			// max dimensions of a patch mesh in map file
#define	MAX_GRID_SIZE		65			// max dimensions of a grid mesh in memory

// when cgame directly specifies a polygon, it becomes a srfPoly_t
// as soon as it is called
typedef struct srfPoly_s {
	surfaceType_t	surfaceType;
	qhandle_t		hShader;
	int				fogIndex;
	int				numVerts;
	polyVert_t		*verts;
} srfPoly_t;

typedef struct srfDisplayList_s {
	surfaceType_t	surfaceType;
	int				listNum;
} srfDisplayList_t;


typedef struct srfFlare_s {
	surfaceType_t	surfaceType;
	vec3_t			origin;
	vec3_t			normal;
	vec3_t			color;
	shader_t		*shadder;	// leilei - for custom flares
} srfFlare_t;

typedef struct srfGridMesh_s {
	surfaceType_t	surfaceType;

	// dynamic lighting information
	int				dlightBits;

	// culling information
	vec3_t			meshBounds[2];
	vec3_t			localOrigin;
	float			meshRadius;

	// lod information, which may be different
	// than the culling information to allow for
	// groups of curves that LOD as a unit
	vec3_t			lodOrigin;
	float			lodRadius;
	int				lodFixed;
	int				lodStitched;

	// vertexes
	int				width, height;
	float			*widthLodError;
	float			*heightLodError;
	drawVert_t		verts[1];		// variable sized
} srfGridMesh_t;



#define	VERTEXSIZE	8
typedef struct {
	surfaceType_t	surfaceType;
	cplane_t	plane;

	// dynamic lighting information
	int			dlightBits;

	// triangle definitions (no normals at points)
	int			numPoints;
	int			numIndices;
	int			ofsIndices;
	float		points[1][VERTEXSIZE];	// variable sized
										// there is a variable length list of indices here also
} srfSurfaceFace_t;


// misc_models in maps are turned into direct geometry by q3map
typedef struct {
	surfaceType_t	surfaceType;

	// dynamic lighting information
	int				dlightBits;

	// culling information (FIXME: use this!)
	vec3_t			bounds[2];
	vec3_t			localOrigin;
	float			radius;

	// triangle definitions
	int				numIndexes;
	int				*indexes;

	int				numVerts;
	drawVert_t		*verts;
} srfTriangles_t;

#ifdef BROKEN_IQM
// inter-quake-model
typedef struct {
	int		num_vertexes;
	int		num_triangles;
	int		num_frames;
	int		num_surfaces;
	int		num_joints;
	int		num_poses;
	struct srfIQModel_s	*surfaces;

	float		*positions;
	float		*texcoords;
	float		*normals;
	float		*tangents;
	byte		*blendIndexes;
	union {
		float	*f;
		byte	*b;
	} blendWeights;
	byte		*colors;
	int		*triangles;

	// depending upon the exporter, blend indices and weights might be int/float
	// as opposed to the recommended byte/byte, for example Noesis exports
	// int/float whereas the official IQM tool exports byte/byte
	byte blendWeightsType; // IQM_UBYTE or IQM_FLOAT

	int		*jointParents;
	float		*jointMats;
	float		*poseMats;
	float		*bounds;
	char		*names;
} iqmData_t;

// inter-quake-model surface
typedef struct srfIQModel_s {
	surfaceType_t	surfaceType;
	char		name[MAX_QPATH];
	shader_t	*shader;
	iqmData_t	*data;
	int		first_vertex, num_vertexes;
	int		first_triangle, num_triangles;
} srfIQModel_t;

#endif

extern	void (*rb_surfaceTable[SF_NUM_SURFACE_TYPES])(void *);

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


//
// in memory representation
//

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2

typedef struct msurface_s {
	int					viewCount;		// if == tr.viewCount, already added
	struct shader_s		*shader;
	int					fogIndex;

	surfaceType_t		*data;			// any of srf*_t
} msurface_t;



#define	CONTENTS_NODE		-1
typedef struct mnode_s {
	// common with leaf and node
	int			contents;		// -1 for nodes, to differentiate from leafs
	int			visframe;		// node needs to be traversed if current
	vec3_t		mins, maxs;		// for bounding box culling
	struct mnode_s	*parent;

	// node specific
	cplane_t	*plane;
	struct mnode_s	*children[2];	

	// leaf specific
	int			cluster;
	int			area;

	msurface_t	**firstmarksurface;
	int			nummarksurfaces;
} mnode_t;

typedef struct {
	vec3_t		bounds[2];		// for culling
	msurface_t	*firstSurface;
	int			numSurfaces;
} bmodel_t;

typedef struct {
	char		name[MAX_QPATH];		// ie: maps/tim_dm2.bsp
	char		baseName[MAX_QPATH];	// ie: tim_dm2

	int			dataSize;

	int			numShaders;
	dshader_t	*shaders;

	bmodel_t	*bmodels;

	int			numplanes;
	cplane_t	*planes;

	int			numnodes;		// includes leafs
	int			numDecisionNodes;
	mnode_t		*nodes;

	int			numsurfaces;
	msurface_t	*surfaces;

	int			nummarksurfaces;
	msurface_t	**marksurfaces;

	int			numfogs;
	fog_t		*fogs;

	vec3_t		lightGridOrigin;
	vec3_t		lightGridSize;
	vec3_t		lightGridInverseSize;
	int			lightGridBounds[3];


	byte		*lightGridData;



	int			numClusters;
	int			clusterBytes;
	const byte	*vis;			// may be passed in by CM_LoadMap to save space

	byte		*novis;			// clusterBytes of 0xff

	char		*entityString;
	char		*entityParsePoint;
} world_t;

#define MAX_PROGRAMS 256
#define MAX_PROGRAM_OBJECTS 8

typedef struct {
	int				index;
	char			name[MAX_QPATH];
	qboolean		valid;
	GLhandleARB		program;

	GLint			u_AlphaGen;
	alphaGen_t		v_AlphaGen;

	GLint			u_AmbientLight;
	vec3_t			v_AmbientLight;

	GLint			u_DynamicLight;
	vec3_t			v_DynamicLight;

	GLint			u_LightDistance;
	float			v_LightDistance;

	GLint			u_ColorGen;
	colorGen_t		v_ColorGen;

	GLint			u_ConstantColor;
	byte			v_ConstantColor[4];

	GLint			u_DirectedLight;
	vec3_t			v_DirectedLight;

	GLint			u_EntityColor;
	byte			v_EntityColor[4];

	GLint			u_FogColor;
	unsigned		v_FogColor;

	GLint			u_Greyscale;
	int			v_Greyscale;

	GLint			u_IdentityLight;
	float			v_IdentityLight;

	GLint			u_LightDirection;
	vec3_t			v_LightDirection;

	GLint			u_ModelViewMatrix;
	float			v_ModelViewMatrix[16];

	GLint			u_ModelViewProjectionMatrix;
	float			v_ModelViewProjectionMatrix[16];

	GLint			u_ProjectionMatrix;
	float			v_ProjectionMatrix[16];

	GLint			u_TCGen0;
	texCoordGen_t	v_TCGen0;

	GLint			u_TCGen1;
	texCoordGen_t	v_TCGen1;

	GLint			u_TexEnv;
	int				v_TexEnv;

	GLint			u_Texture0;
	GLint			u_Texture1;
	GLint			u_Texture2;
	GLint			u_Texture3;
	GLint			u_Texture4;
	GLint			u_Texture5;
	GLint			u_Texture6;
	GLint			u_Texture7;

	GLint			u_Time;
	float			v_Time;

	GLint			u_ViewOrigin;
	vec3_t			v_ViewOrigin;
	
//Postprocessing usefull vars
	GLint			u_ScreenSizeX;
	GLint			u_ScreenSizeY;
	GLfloat			u_ScreenToNextPixelX;
	GLfloat			u_ScreenToNextPixelY;
	GLfloat			u_zFar;
	GLint			u_ActualScreenSizeX;
	GLint			u_ActualScreenSizeY;

// leilei - 'compatibility' with ruby shader vars (HACK HACK HACK)

	GLint			rubyInputSize;
	vec3_t			v_rubyInputSize;

	GLint			rubyOutputSize;
	vec3_t			v_rubyOutputSize;

	GLint			rubyTextureSize;
	vec3_t			v_rubyTextureSize;

	GLfloat			rubyFrameCount;


// leilei - Color control

	GLfloat			u_CC_Brightness;
	GLfloat			u_CC_Contrast;
	GLfloat			u_CC_Saturation;
	GLfloat			u_CC_Overbright;
	GLfloat			u_CC_Gamma;

	
//End Postprocess Vars	


//	leilei - addition (HACK!)	

	GLint			u_Normal;
	vec3_t			v_Normal;


	
} glslProgram_t;

//======================================================================

typedef enum {
	MOD_BAD,
	MOD_BRUSH,
	MOD_MESH,
	MOD_MDR
#ifdef BROKEN_IQM
	,MOD_IQM
#endif
} modtype_t;

typedef struct model_s {
	char		name[MAX_QPATH];
	modtype_t	type;
	int			index;		// model = tr.models[model->index]

	int			dataSize;	// just for listing purposes
	bmodel_t	*bmodel;		// only if type == MOD_BRUSH
	md3Header_t	*md3[MD3_MAX_LODS];	// only if type == MOD_MESH
	void	*modelData;			// only if type == (MOD_MDR | MOD_IQM)
#ifdef BROKEN_MDRPHYS
	mdrPhys_t		phys;		// leilei - MDR model physics
	int			mdln;
	int			hasphysics;	// leilei - mdr physics
#endif

	int			 numLods;
} model_t;


#define	MAX_MOD_KNOWN	1024

void		R_ModelInit (void);
model_t		*R_GetModelByHandle( qhandle_t hModel );
int			R_LerpTag( orientation_t *tag, qhandle_t handle, int startFrame, int endFrame, 
					 float frac, const char *tagName );
void		R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs );

void		R_Modellist_f (void);

//====================================================

#define	MAX_DRAWIMAGES			2048
#define	MAX_LIGHTMAPS			256
#define	MAX_SKINS				1024


#define	MAX_DRAWSURFS			0x10000
#define	DRAWSURF_MASK			(MAX_DRAWSURFS-1)

/*

the drawsurf sort data is packed into a single 32 bit value so it can be
compared quickly during the qsorting process

the bits are allocated as follows:

0 - 1	: dlightmap index
//2		: used to be clipped flag REMOVED - 03.21.00 rad
2 - 6	: fog index
11 - 20	: entity index
21 - 31	: sorted shader index

	TTimo - 1.32
0-1   : dlightmap index
2-6   : fog index
7-16  : entity index
17-30 : sorted shader index
*/
#define	QSORT_FOGNUM_SHIFT	2
#define	QSORT_REFENTITYNUM_SHIFT	7
#define	QSORT_SHADERNUM_SHIFT	(QSORT_REFENTITYNUM_SHIFT+REFENTITYNUM_BITS)
#if (QSORT_SHADERNUM_SHIFT+SHADERNUM_BITS) > 32
	#error "Need to update sorting, too many bits."
#endif

extern	int			gl_filter_min, gl_filter_max;

/*
** performanceCounters_t
*/
typedef struct {
	int		c_sphere_cull_patch_in, c_sphere_cull_patch_clip, c_sphere_cull_patch_out;
	int		c_box_cull_patch_in, c_box_cull_patch_clip, c_box_cull_patch_out;
	int		c_sphere_cull_md3_in, c_sphere_cull_md3_clip, c_sphere_cull_md3_out;
	int		c_box_cull_md3_in, c_box_cull_md3_clip, c_box_cull_md3_out;

	int		c_leafs;
	int		c_dlightSurfaces;
	int		c_dlightSurfacesCulled;
} frontEndCounters_t;

#define	FOG_TABLE_SIZE		256
#define FUNCTABLE_SIZE		1024
#define FUNCTABLE_SIZE2		10
#define FUNCTABLE_MASK		(FUNCTABLE_SIZE-1)


// the renderer front end should never modify glstate_t
typedef struct {
	int			currenttextures[2];
	int			currenttmu;
	float		currentModelViewMatrix[16];
	float		currentModelViewProjectionMatrix[16];
	float		currentProjectionMatrix[16];
	qhandle_t	currentProgram;
	qhandle_t	postprocessingProgram;
	qboolean	finishCalled;
	int			texEnv[2];
	int			faceCulling;
	unsigned long	glStateBits;
} glstate_t;

typedef struct {
	int		c_surfaces, c_shaders, c_vertexes, c_indexes, c_totalIndexes;
	float	c_overDraw;
	
	int		c_dlightVertexes;
	int		c_dlightIndexes;

	int		c_flareAdds;
	int		c_flareTests;
	int		c_flareRenders;

	int		msec;			// total msec for backend run
} backEndCounters_t;

// all state modified by the back end is seperated
// from the front end state
typedef struct {
	trRefdef_t	refdef;
	viewParms_t	viewParms;
	orientationr_t	or;
	backEndCounters_t	pc;
	qboolean	isHyperspace;
	trRefEntity_t	*currentEntity;
	qboolean	skyRenderedThisView;	// flag for drawing sun

	qboolean	projection2D;	// if qtrue, drawstretchpic doesn't need to change modes
	byte		color2D[4];
	qboolean	vertexes2D;		// shader needs to be finished
	qboolean	doneBloom;		// done bloom this frame
	qboolean	donepostproc;		// done postprocess this frame
	qboolean	doneleifx;		// leilei - done leifxing this frame
	qboolean	doneAltBrightness;	// leilei - done alternate brightness this frame
	qboolean	doneSun;		// leilei - done drawing a sun
	qboolean	doneSunFlare;		// leilei - done drawing a sun flare
	qboolean	donewater;		// leilei - done water this frame
	qboolean	donepalette;		// leilei - done paletting this frame
	qboolean	doneSurfaces;   // done any 3d surfaces already
	qboolean	doneParticles;   // done any particle movement
	qboolean	doneFlareTests;		// leilei - done testing flares
	float		flareTestTime;
	trRefEntity_t	entity2D;	// currentEntity will point at this when doing 2D rendering
} backEndState_t;

/*
** trGlobals_t 
**
** Most renderer globals are defined here.
** backend functions should never modify any of these fields,
** but may read fields that aren't dynamically modified
** by the frontend.
*/
typedef struct {
	qboolean				registered;		// cleared at shutdown, set at beginRegistration

	int						visCount;		// incremented every time a new vis cluster is entered
	int						frameCount;		// incremented every frame
	int						sceneCount;		// incremented every scene
	int						viewCount;		// incremented every view (twice a scene if portaled)
											// and every R_MarkFragments call

	int						frameSceneNum;	// zeroed at RE_BeginFrame

	qboolean				worldMapLoaded;
	world_t					*world;

	const byte				*externalVisData;	// from RE_SetWorldVisData, shared with CM_Load

	image_t					*defaultImage;
	image_t					*scratchImage[32];
	image_t					*fogImage;
	image_t					*dlightImage;	// inverse-quare highlight for projective adding
	image_t					*waterImage;
	image_t					*flareImage;
	image_t					*whiteImage;			// full of 0xff
	image_t					*identityLightImage;	// full of tr.identityLightByte

	shader_t				*defaultShader;
	shader_t				*shadowShader;
	shader_t				*projectionShadowShader;

	shader_t				*flareShader;
	shader_t				*flareShaderAtlas;	// leilei - lens reflections
	shader_t				*sunShader;
	char					*sunShaderCustom; 	// leilei - sunShader override

	shader_t				*coneShader;		// leilei - flashblends

	qhandle_t				skipProgram;
	qhandle_t				defaultProgram;
	qhandle_t				vertexLitProgram;
	qhandle_t				lightmappedMultitextureProgram;
	qhandle_t				skyProgram;
	qhandle_t				postprocessingProgram;

	qhandle_t				leiFXDitherProgram;	// leilei
	qhandle_t				leiFXGammaProgram;	// leilei
	qhandle_t				leiFXFilterProgram;	// leilei
	qhandle_t				BrightnessProgram;	// leilei

	int						numPrograms;
	glslProgram_t			*programs[MAX_PROGRAMS];

	int						numLightmaps;
	image_t					**lightmaps;

	trRefEntity_t			*currentEntity;
	trRefEntity_t			worldEntity;		// point currentEntity at this when rendering world
	int						currentEntityNum;
	int						shiftedEntityNum;	// currentEntityNum << QSORT_REFENTITYNUM_SHIFT
	model_t					*currentModel;

	viewParms_t				viewParms;

	float					identityLight;		// 1.0 / ( 1 << overbrightBits )
	int						identityLightByte;	// identityLight * 255
	int						overbrightBits;		// r_overbrightBits->integer, but set to 0 if no hw gamma

	orientationr_t			or;					// for current entity

	trRefdef_t				refdef;

	int						viewCluster;

	vec3_t					sunLight;			// from the sky shader for this level
	vec3_t					sunDirection;
	int					sunFlare;			// leilei - sun flare type
	float					sunOn;				// leilei - allow the sun to be drawn, if a sunShader is specified by a sky shader.  This is a float so we can multiply the size of the sun in the shader

	frontEndCounters_t		pc;
	int						frontEndMsec;		// not in pc due to clearing issue

	//
	// put large tables at the end, so most elements will be
	// within the +/32K indexed range on risc processors
	//
	model_t					*models[MAX_MOD_KNOWN];
	int						numModels;

	int						numAnimations;

	int						numImages;
	image_t					*images[MAX_DRAWIMAGES];

	// shader indexes from other modules will be looked up in tr.shaders[]
	// shader indexes from drawsurfs will be looked up in sortedShaders[]
	// lower indexed sortedShaders must be rendered first (opaque surfaces before translucent)
	int						numShaders;
	shader_t				*shaders[MAX_SHADERS];
	shader_t				*sortedShaders[MAX_SHADERS];

	int						numSkins;
	skin_t					*skins[MAX_SKINS];

	float					sinTable[FUNCTABLE_SIZE];
	float					squareTable[FUNCTABLE_SIZE];
	float					triangleTable[FUNCTABLE_SIZE];
	float					sawToothTable[FUNCTABLE_SIZE];
	float					inverseSawToothTable[FUNCTABLE_SIZE];
	float					fogTable[FOG_TABLE_SIZE];

	shader_t				*placeholderTextureShader;	// leilei - for map textures
	shader_t				*placeholderModelShader;	// leilei - for models
	shader_t				*placeholderSkyShader;		// leilei - for skies
	shader_t				*placeholderWaterShader;	// leilei - for liquids
	shader_t				*placeholderLavaShader;		// leilei - for lavas
	shader_t				*placeholderSlimeShader;	// leilei - for slimes
	shader_t				*placeholderFogShader;		// leilei - for fogs
	shader_t				*placeholderShader;		// leilei - for anything else
	
	qboolean				placeholderTextureAvail;
	qboolean				placeholderModelAvail;
	qboolean				placeholderSkyAvail;
	qboolean				placeholderWaterAvail;
	qboolean				placeholderLavaAvail;
	qboolean				placeholderSlimeAvail;
	qboolean				placeholderFogAvail;
	qboolean				placeholderAvail;

	image_t					*transRights;	// leilei

} trGlobals_t;

extern backEndState_t	backEnd;
extern trGlobals_t	tr;
extern glstate_t	glState;		// outside of TR since it shouldn't be cleared during ref re-init

extern qboolean  vertexShaders;
extern qboolean  postprocess;
extern int	 leifxmode;	// leilei - leifx
extern char	 depthimage;

//
// cvars
//

extern cvar_t	*r_flareSize;
extern cvar_t	*r_flareFade;
extern cvar_t	*r_flareQuality;
extern cvar_t	*r_flareSun;
extern cvar_t	*r_flareMethod;
extern cvar_t	*r_flareDelay;

// coefficient for the flare intensity falloff function.
#define FLARE_STDCOEFF "150"
extern cvar_t	*r_flareCoeff;

extern cvar_t	*r_railWidth;
extern cvar_t	*r_railCoreWidth;
extern cvar_t	*r_railSegmentLength;

extern cvar_t	*r_ignore;				// used for debugging anything
extern cvar_t	*r_verbose;				// used for verbose debug spew
extern cvar_t	*r_ignoreFastPath;		// allows us to ignore our Tess fast paths

extern cvar_t	*r_znear;				// near Z clip plane
extern cvar_t	*r_zproj;				// z distance of projection plane
extern cvar_t	*r_stereoSeparation;			// separation of cameras for stereo rendering

extern cvar_t	*r_measureOverdraw;		// enables stencil buffer overdraw measurement

extern cvar_t	*r_lodbias;				// push/pull LOD transitions
extern cvar_t	*r_lodscale;

extern cvar_t	*r_primitives;			// "0" = based on compiled vertex array existance
										// "1" = glDrawElemet tristrips
										// "2" = glDrawElements triangles
										// "-1" = no drawing

extern cvar_t	*r_inGameVideo;				// controls whether in game video should be draw
extern cvar_t	*r_fastsky;				// controls whether sky should be cleared or drawn
extern cvar_t	*r_drawSun;				// controls drawing of sun quad
extern cvar_t	*r_dynamiclight;		// dynamic lights enabled/disabled
extern cvar_t	*r_dlightBacks;			// dlight non-facing surfaces for continuity

extern	cvar_t	*r_norefresh;			// bypasses the ref rendering
extern	cvar_t	*r_drawentities;		// disable/enable entity rendering
extern	cvar_t	*r_drawworld;			// disable/enable world rendering
extern	cvar_t	*r_speeds;				// various levels of information display
extern  cvar_t	*r_detailTextures;		// enables/disables detail texturing stages
extern	cvar_t	*r_novis;				// disable/enable usage of PVS
extern	cvar_t	*r_nocull;
extern	cvar_t	*r_facePlaneCull;		// enables culling of planar surfaces with back side test
extern	cvar_t	*r_nocurves;
extern	cvar_t	*r_showcluster;

extern cvar_t	*r_gamma;

extern cvar_t	*r_ext_vertex_shader;
extern cvar_t	*r_postprocess;
extern	cvar_t	*r_nobind;						// turns off binding to appropriate textures
extern	cvar_t	*r_singleShader;				// make most world faces use default shader
extern	cvar_t	*r_roundImagesDown;
extern	cvar_t	*r_colorMipLevels;				// development aid to see texture mip usage
extern	cvar_t	*r_picmip;						// controls picmip values
extern	cvar_t	*r_finish;
extern	cvar_t	*r_textureMode;
extern	cvar_t	*r_offsetFactor;
extern	cvar_t	*r_offsetUnits;

extern	cvar_t	*r_fullbright;					// avoid lightmap pass
extern	cvar_t	*r_lightmap;					// render lightmaps only
extern	cvar_t	*r_vertexLight;					// vertex lighting mode for better performance
extern	cvar_t	*r_uiFullScreen;				// ui is running fullscreen

extern	cvar_t	*r_logFile;						// number of frames to emit GL logs
extern	cvar_t	*r_showtris;					// enables wireframe rendering of the world
extern	cvar_t	*r_showsky;						// forces sky in front of all surfaces
extern	cvar_t	*r_shownormals;					// draws wireframe normals
extern	cvar_t	*r_clear;						// force screen clear every frame

extern	cvar_t	*r_shadows;						// controls shadows: 0 = none, 1 = blur, 2 = stencil, 3 = black planar projection
extern	cvar_t	*r_flares;						// light flares

extern	cvar_t	*r_intensity;

extern	cvar_t	*r_lockpvs;
extern	cvar_t	*r_noportals;
extern	cvar_t	*r_portalOnly;

extern	cvar_t	*r_subdivisions;
extern	cvar_t	*r_lodCurveError;
extern	cvar_t	*r_skipBackEnd;

extern	cvar_t	*r_anaglyphMode;

extern	cvar_t	*r_monolightmaps;
extern	cvar_t	*r_ignoreGLErrors;

extern	cvar_t	*r_overBrightBits;
extern	cvar_t	*r_mapOverBrightBits;

extern	cvar_t	*r_debugSurface;
extern	cvar_t	*r_simpleMipMaps;

extern	cvar_t	*r_showImages;
extern	cvar_t	*r_debugSort;

extern	cvar_t	*r_printShaders;

extern cvar_t	*r_marksOnTriangleMeshes;

extern	cvar_t	*r_lensReflection1;
extern	cvar_t	*r_lensReflection2;
extern	cvar_t	*r_lensReflectionBrightness;

extern cvar_t	*r_loadDDS;				// leilei - Compressed Texture
extern cvar_t	*r_ext_paletted_texture;		// leilei - Paletted Texture
extern cvar_t	*r_ext_gamma_control;			// leilei - 3dfx gamma control
extern	cvar_t	*r_specMode;		
//extern	cvar_t	*r_waveMode;	

extern	cvar_t	*r_flaresDlight;
extern	cvar_t	*r_flaresDlightShrink;
extern	cvar_t	*r_flaresDlightFade;
extern	cvar_t	*r_flaresDlightOpacity;
extern	cvar_t	*r_flaresDlightScale;

extern cvar_t	*r_alternateBrightness;		// leilei - alternate brightness
extern cvar_t	*r_leifx;	// Leilei - leifx nostalgia filter
extern cvar_t	*r_shadeMethod;	// Leilei - new model shading methods


extern cvar_t	*r_suggestiveThemes;	// Leilei - mature content

extern cvar_t	*r_leidebug;	// Leilei - debug only!
extern cvar_t	*r_leidebugeye;	// Leilei - debug only!
extern cvar_t	*r_particles;	// Leilei - particles!
#ifdef BROKEN_MDRPHYS
extern cvar_t	*r_mdrPhysics;	// Leilei - MDR Physics
#endif

extern	cvar_t	*r_iconmip;	// leilei - icon mip - picmip for 2d icons
extern	cvar_t	*r_iconBits;	// leilei - icon color depth for 2d icons

extern cvar_t	*r_lerpbias;	// Leilei - lerping bias

extern	cvar_t	*r_lightmapBits;	// leilei - lightmap color depth
extern	cvar_t	*r_lightmapColorNorm;	// leilei - lightmap color normalize

extern  cvar_t	*r_detailTextureScale;		// leilei - scale tweak the detail textures, 0 doesn't tweak at all.
extern  cvar_t	*r_detailTextureLayers;		// leilei - add in more smaller detail texture layers, expensive!

extern  cvar_t	*r_textureDither;		// leilei - apply dithering for lower texture bits

extern cvar_t	*r_skytess;		// leilei - adjusts the subdivisions of the sky (max 8, min 1)

//====================================================================

void R_SwapBuffers( int );

void R_RenderView( viewParms_t *parms );

void R_AddMD3Surfaces( trRefEntity_t *e );
void R_AddNullModelSurfaces( trRefEntity_t *e );
void R_AddBeamSurfaces( trRefEntity_t *e );
void R_AddRailSurfaces( trRefEntity_t *e, qboolean isUnderwater );
void R_AddLightningBoltSurfaces( trRefEntity_t *e );

void R_AddPolygonSurfaces( void );

void R_DecomposeSort( unsigned sort, int *entityNum, shader_t **shader, 
					 int *fogNum, int *dlightMap );

void R_AddDrawSurf( surfaceType_t *surface, shader_t *shader, int fogIndex, int dlightMap );


#define	CULL_IN		0		// completely unclipped
#define	CULL_CLIP	1		// clipped by one or more planes
#define	CULL_OUT	2		// completely outside the clipping planes
void R_LocalNormalToWorld (vec3_t local, vec3_t world);
void R_LocalPointToWorld (vec3_t local, vec3_t world);
int R_CullLocalBox (vec3_t bounds[2]);
int R_CullPointAndRadius( vec3_t origin, float radius );
int R_CullLocalPointAndRadius( vec3_t origin, float radius );

void R_SetupProjection(viewParms_t *dest, float zProj, qboolean computeFrustum);
void R_RotateForEntity( const trRefEntity_t *ent, const viewParms_t *viewParms, orientationr_t *or );

/*
** GL wrapper/helper functions
*/
void	GL_Bind( image_t *image );
void	GL_SetDefaultState (void);
void	GL_SelectTexture( int unit );
void	GL_TextureMode( const char *string );
void	GL_CheckErrors( void );
void	GL_State( unsigned long stateVector );
void	GL_TexEnv( int env );
void	GL_Cull( int cullType );

#define GLS_SRCBLEND_ZERO						0x00000001
#define GLS_SRCBLEND_ONE						0x00000002
#define GLS_SRCBLEND_DST_COLOR					0x00000003
#define GLS_SRCBLEND_ONE_MINUS_DST_COLOR		0x00000004
#define GLS_SRCBLEND_SRC_ALPHA					0x00000005
#define GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA		0x00000006
#define GLS_SRCBLEND_DST_ALPHA					0x00000007
#define GLS_SRCBLEND_ONE_MINUS_DST_ALPHA		0x00000008
#define GLS_SRCBLEND_ALPHA_SATURATE				0x00000009
#define		GLS_SRCBLEND_BITS					0x0000000f

#define GLS_DSTBLEND_ZERO						0x00000010
#define GLS_DSTBLEND_ONE						0x00000020
#define GLS_DSTBLEND_SRC_COLOR					0x00000030
#define GLS_DSTBLEND_ONE_MINUS_SRC_COLOR		0x00000040
#define GLS_DSTBLEND_SRC_ALPHA					0x00000050
#define GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA		0x00000060
#define GLS_DSTBLEND_DST_ALPHA					0x00000070
#define GLS_DSTBLEND_ONE_MINUS_DST_ALPHA		0x00000080
#define		GLS_DSTBLEND_BITS					0x000000f0

#define GLS_DEPTHMASK_TRUE						0x00000100

#define GLS_POLYMODE_LINE						0x00001000

#define GLS_DEPTHTEST_DISABLE					0x00010000
#define GLS_DEPTHFUNC_EQUAL						0x00020000

#define GLS_ATEST_GT_0							0x10000000
#define GLS_ATEST_LT_80							0x20000000
#define GLS_ATEST_GE_80							0x40000000
#define		GLS_ATEST_BITS						0x70000000

#define GLS_DEFAULT			GLS_DEPTHMASK_TRUE

void	RE_StretchRaw (int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);
void	RE_UploadCinematic (int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty);

void		RE_BeginFrame( stereoFrame_t stereoFrame );
void		RE_BeginRegistration( glconfig_t *glconfig );
void		RE_LoadWorldMap( const char *mapname );
void		RE_SetWorldVisData( const byte *vis );
qhandle_t	RE_RegisterModel( const char *name );
qhandle_t	RE_RegisterSkin( const char *name );
void		RE_Shutdown( qboolean destroyWindow );

qboolean	R_GetEntityToken( char *buffer, int size );

model_t		*R_AllocModel( void );

void    	R_Init( void );

void		R_SetColorMappings( void );
void		R_GammaCorrect( byte *buffer, int bufSize );

void	R_ImageListMapOnly_f( void ); // leilei - stuff hack
void	R_ImageList_f( void );
void	R_SkinList_f( void );
// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=516
const void *RB_TakeScreenshotCmd( const void *data );
void	R_ScreenShot_f( void );
void	R_GLSLPalette_f( void );

void	R_InitFogTable( void );
float	R_FogFactor( float s, float t );
void	R_InitImages( void );
void	R_DeleteTextures( void );
int		R_SumOfUsedImages( void );
void	R_InitSkins( void );
skin_t	*R_GetSkinByHandle( qhandle_t hSkin );

int R_ComputeLOD( trRefEntity_t *ent );

const void *RB_TakeVideoFrameCmd( const void *data );

//
// tr_shader.c
//
shader_t	*R_FindShader( const char *name, int lightmapIndex, qboolean mipRawImage );
shader_t	*R_GetShaderByHandle( qhandle_t hShader );
shader_t	*R_GetShaderByState( int index, long *cycleTime );
shader_t *R_FindShaderByName( const char *name );
void		R_InitShaders( void );
void		R_ShaderList_f( void );
void    R_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset);

/*
====================================================================

TESSELATOR/SHADER DECLARATIONS

====================================================================
*/
typedef byte color4ub_t[4];

typedef struct stageVars
{
	color4ub_t	colors[SHADER_MAX_VERTEXES];
	vec2_t		texcoords[NUM_TEXTURE_BUNDLES][SHADER_MAX_VERTEXES];
} stageVars_t;


typedef struct shaderCommands_s 
{
	glIndex_t	indexes[SHADER_MAX_INDEXES] QALIGN(16);
	vec4_t		xyz[SHADER_MAX_VERTEXES] QALIGN(16);
	vec4_t		normal[SHADER_MAX_VERTEXES] QALIGN(16);
	vec2_t		texCoords[SHADER_MAX_VERTEXES][2] QALIGN(16);
	color4ub_t	vertexColors[SHADER_MAX_VERTEXES] QALIGN(16);
	int			vertexDlightBits[SHADER_MAX_VERTEXES] QALIGN(16);

	stageVars_t	svars QALIGN(16);

	color4ub_t	constantColor255[SHADER_MAX_VERTEXES] QALIGN(16);

	shader_t	*shader;
	float		shaderTime;
	int		fogNum;

	int		dlightBits;	// or together of all vertexDlightBits

	int		numIndexes;
	int		numVertexes;

	// info extracted from current shader
	int			numPasses;
	void		(*currentStageIteratorFunc)( void );
	shaderStage_t	**xstages;

	// lfx stuf
	float		lfxTime;
	float		lfxTimeNext;
} shaderCommands_t;

extern	shaderCommands_t	tess;

void RB_SetGL2D (void);
void RB_BeginSurface(shader_t *shader, int fogNum );
void RB_EndSurface(void);
void RB_CheckOverflow( int verts, int indexes );
#define RB_CHECKOVERFLOW(v,i) if (tess.numVertexes + (v) >= SHADER_MAX_VERTEXES || tess.numIndexes + (i) >= SHADER_MAX_INDEXES ) {RB_CheckOverflow(v,i);}

void RB_StageIteratorGeneric( void );
void RB_StageIteratorSky( void );
void RB_StageIteratorVertexLitTexture( void );
void RB_StageIteratorLightmappedMultitexture( void );

void RB_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, byte *color );
void RB_AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, byte *color, float s1, float t1, float s2, float t2 );

void RB_ShowImages( void );

void		GLimp_InitExtraExtensions(void);

/*
============================================================

WORLD MAP

============================================================
*/

void R_AddBrushModelSurfaces( trRefEntity_t *e );
void R_AddWorldSurfaces( void );
qboolean R_inPVS( const vec3_t p1, const vec3_t p2 );


/*
============================================================

FLARES

============================================================
*/

void R_ClearFlares( void );

void RB_AddFlare(srfFlare_t *surface, int fogNum, vec3_t point, vec3_t color, vec3_t normal, int radii,  int efftype, float scaled, int type);
void RB_AddDlightFlares( void );
void RB_RenderFlares (void);

/*
============================================================

GLSL

============================================================
*/

/*
 * R_GLSL_SetUniform
 * Only upload uniform data when value changes
 */
static ID_INLINE void R_GLSL_SetUniform_AlphaGen(glslProgram_t *program, alphaGen_t value) {
	if (program->v_AlphaGen == value)
		return;

	program->v_AlphaGen = value;
	qglUniform1iARB(program->u_AlphaGen, value);
}

static ID_INLINE void R_GLSL_SetUniform_AmbientLight(glslProgram_t *program, const vec3_t value) {
	if (VectorCompare(program->v_AmbientLight, value))
		return;

	VectorCopy(value, program->v_AmbientLight);
	qglUniform3fARB(program->u_AmbientLight, value[0] / 255.0f, value[1] / 255.0f, value[2] / 255.0f);
}

static ID_INLINE void R_GLSL_SetUniform_DynamicLight(glslProgram_t *program, const vec3_t value) {
	if (VectorCompare(program->v_DynamicLight, value))
		return;

	VectorCopy(value, program->v_DynamicLight);
	qglUniform3fARB(program->u_DynamicLight, value[0] / 255.0f, value[1] / 255.0f, value[2] / 255.0f);
}

static ID_INLINE void R_GLSL_SetUniform_LightDistance(glslProgram_t *program, float value) {
	if (program->v_LightDistance == value)
		return;

	program->v_LightDistance = value;
	qglUniform1fARB(program->u_LightDistance, value);
}

static ID_INLINE void R_GLSL_SetUniform_ColorGen(glslProgram_t *program, colorGen_t value) {
	if (program->v_ColorGen == value)
		return;

	program->v_ColorGen = value;
	qglUniform1iARB(program->u_ColorGen, value);
}

static ID_INLINE void R_GLSL_SetUniform_ConstantColor(glslProgram_t *program, byte value[4]) {
	if (program->v_ConstantColor[0] == value[0] &&
		program->v_ConstantColor[1] == value[1] &&
		program->v_ConstantColor[2] == value[2] &&
		program->v_ConstantColor[3] == value[3])
		return;

	program->v_ConstantColor[0] = value[0];
	program->v_ConstantColor[1] = value[1];
	program->v_ConstantColor[2] = value[2];
	program->v_ConstantColor[3] = value[3];
	qglUniform4fARB(program->u_ConstantColor, value[0] / 255.0f, value[1] / 255.0f, value[2] / 255.0f, value[3] / 255.0f);
}

static ID_INLINE void R_GLSL_SetUniform_DirectedLight(glslProgram_t *program, const vec3_t value) {
	if (VectorCompare(program->v_DirectedLight, value))
		return;

	VectorCopy(value, program->v_DirectedLight);
	qglUniform3fARB(program->u_DirectedLight, value[0] / 255.0f, value[1] / 255.0f, value[2] / 255.0f);
}

static ID_INLINE void R_GLSL_SetUniform_EntityColor(glslProgram_t *program, byte value[4]) {
	if (program->v_EntityColor[0] == value[0] &&
		program->v_EntityColor[1] == value[1] &&
		program->v_EntityColor[2] == value[2] &&
		program->v_EntityColor[3] == value[3])
		return;

	program->v_EntityColor[0] = value[0];
	program->v_EntityColor[1] = value[1];
	program->v_EntityColor[2] = value[2];
	program->v_EntityColor[3] = value[3];
	qglUniform4fARB(program->u_EntityColor, value[0] / 255.0f, value[1] / 255.0f, value[2] / 255.0f, value[3] / 255.0f);
}

static ID_INLINE void R_GLSL_SetUniform_FogColor(glslProgram_t *program, unsigned value) {
	byte	temp[4];

	if (program->v_FogColor == value)
		return;

	*(unsigned *)temp = value;

	program->v_FogColor = value;
	qglUniform4fARB(program->u_FogColor, temp[0] / 255.0f, temp[1] / 255.0f, temp[2] / 255.0f, temp[3] / 255.0f);
}

static ID_INLINE void R_GLSL_SetUniform_Greyscale(glslProgram_t *program, int value) {
	if (program->v_Greyscale == value)
		return;

	program->v_Greyscale = value;
	qglUniform1iARB(program->u_Greyscale, value);
}

static ID_INLINE void R_GLSL_SetUniform_IdentityLight(glslProgram_t *program, float value) {
	if (program->v_IdentityLight == value)
		return;

	program->v_IdentityLight = value;
	qglUniform1fARB(program->u_IdentityLight, value);
}

static ID_INLINE void R_GLSL_SetUniform_LightDirection(glslProgram_t *program, const vec3_t value) {
	if (VectorCompare(program->v_LightDirection, value))
		return;

	VectorCopy(value, program->v_LightDirection);
	qglUniform3fARB(program->u_LightDirection, value[0], value[1], value[2]);
}

static ID_INLINE void R_GLSL_SetUniform_ModelViewMatrix(glslProgram_t *program, const float value[16]) {
	if (Matrix4Compare(program->v_ModelViewMatrix, value))
		return;

	Matrix4Copy(value, program->v_ModelViewMatrix);
	qglUniformMatrix4fvARB(program->u_ModelViewMatrix, 1, GL_FALSE, value);
}

static ID_INLINE void R_GLSL_SetUniform_ModelViewProjectionMatrix(glslProgram_t *program, const float value[16]) {
	if (Matrix4Compare(program->v_ModelViewProjectionMatrix, value))
		return;

	Matrix4Copy(value, program->v_ModelViewProjectionMatrix);
	qglUniformMatrix4fvARB(program->u_ModelViewProjectionMatrix, 1, GL_FALSE, value);
}

static ID_INLINE void R_GLSL_SetUniform_ProjectionMatrix(glslProgram_t *program, const float value[16]) {
	if (Matrix4Compare(program->v_ProjectionMatrix, value))
		return;

	Matrix4Copy(value, program->v_ProjectionMatrix);
	qglUniformMatrix4fvARB(program->u_ProjectionMatrix, 1, GL_FALSE, value);
}

static ID_INLINE void R_GLSL_SetUniform_TCGen0(glslProgram_t *program, texCoordGen_t value) {
	if (program->v_TCGen0 == value)
		return;

	program->v_TCGen0 = value;
	qglUniform1iARB(program->u_TCGen0, value);
}

static ID_INLINE void R_GLSL_SetUniform_TCGen1(glslProgram_t *program, texCoordGen_t value) {
	if (program->v_TCGen1 == value)
		return;

	program->v_TCGen1 = value;
	qglUniform1iARB(program->u_TCGen1, value);
}

static ID_INLINE void R_GLSL_SetUniform_TexEnv(glslProgram_t *program, int value) {
	switch (value) {
		case GL_REPLACE:
			value = 0;
			break;
		case GL_MODULATE:
			value = 1;
			break;
		case GL_DECAL:
			value = 2;
			break;
		case GL_ADD:
			value = 4;
			break;
		default:
			value = -1;
	}

	if (program->v_TexEnv == value)
		return;

	program->v_TexEnv = value;

	qglUniform1iARB(program->u_TexEnv, value);
}

static ID_INLINE void R_GLSL_SetUniform_Texture0(glslProgram_t *program, GLint value) {
	qglUniform1iARB(program->u_Texture0, value);
}

static ID_INLINE void R_GLSL_SetUniform_Texture1(glslProgram_t *program, GLint value) {
	qglUniform1iARB(program->u_Texture1, value);
}

static ID_INLINE void R_GLSL_SetUniform_Texture2(glslProgram_t *program, GLint value) {
	qglUniform1iARB(program->u_Texture2, value);
}

static ID_INLINE void R_GLSL_SetUniform_Texture3(glslProgram_t *program, GLint value) {
	qglUniform1iARB(program->u_Texture3, value);
}

static ID_INLINE void R_GLSL_SetUniform_Texture4(glslProgram_t *program, GLint value) {
	qglUniform1iARB(program->u_Texture4, value);
}

static ID_INLINE void R_GLSL_SetUniform_Texture5(glslProgram_t *program, GLint value) {
	qglUniform1iARB(program->u_Texture5, value);
}

static ID_INLINE void R_GLSL_SetUniform_Texture6(glslProgram_t *program, GLint value) {
	qglUniform1iARB(program->u_Texture6, value);
}

static ID_INLINE void R_GLSL_SetUniform_Texture7(glslProgram_t *program, GLint value) {
	qglUniform1iARB(program->u_Texture7, value);
}

static ID_INLINE void R_GLSL_SetUniform_Time(glslProgram_t *program, float value) {
	if (program->v_Time == value)
		return;

	program->v_Time = value;
	qglUniform1fARB(program->u_Time, value);
}

static ID_INLINE void R_GLSL_SetUniform_ViewOrigin(glslProgram_t *program, const vec3_t value) {
	if (VectorCompare(program->v_ViewOrigin, value))
		return;

	VectorCopy(value, program->v_ViewOrigin);
	qglUniform3fARB(program->u_ViewOrigin, value[0], value[1], value[2]);
}

// Postprocess Vars
static ID_INLINE void R_GLSL_SetUniform_u_ScreenSizeX(glslProgram_t *program, GLint value) {
	qglUniform1iARB(program->u_ScreenSizeX, value);
}
static ID_INLINE void R_GLSL_SetUniform_u_ScreenSizeY(glslProgram_t *program, GLint value) {
	qglUniform1iARB(program->u_ScreenSizeY, value);
}

static ID_INLINE void R_GLSL_SetUniform_u_ScreenToNextPixelX(glslProgram_t *program, GLfloat value) {
	qglUniform1fARB(program->u_ScreenToNextPixelX, value);
}

static ID_INLINE void R_GLSL_SetUniform_u_ScreenToNextPixelY(glslProgram_t *program, GLfloat value) {
	qglUniform1fARB(program->u_ScreenToNextPixelY, value);
}

static ID_INLINE void R_GLSL_SetUniform_u_zFar(glslProgram_t *program, GLfloat value) {
	qglUniform1fARB(program->u_zFar, value);
}


static ID_INLINE void R_GLSL_SetUniform_u_CC_Brightness(glslProgram_t *program, GLfloat value) {
	qglUniform1fARB(program->u_CC_Brightness, value);
}


static ID_INLINE void R_GLSL_SetUniform_u_CC_Gamma(glslProgram_t *program, GLfloat value) {
	qglUniform1fARB(program->u_CC_Gamma, value);
}


static ID_INLINE void R_GLSL_SetUniform_u_CC_Contrast(glslProgram_t *program, GLfloat value) {
	qglUniform1fARB(program->u_CC_Contrast, value);
}


static ID_INLINE void R_GLSL_SetUniform_u_CC_Saturation(glslProgram_t *program, GLfloat value) {
	qglUniform1fARB(program->u_CC_Saturation, value);
}


static ID_INLINE void R_GLSL_SetUniform_u_CC_Overbright(glslProgram_t *program, GLfloat value) {
	qglUniform1fARB(program->u_CC_Overbright, value);
}


static ID_INLINE void R_GLSL_SetUniform_u_ActualScreenSizeX(glslProgram_t *program, GLint value) {
	qglUniform1iARB(program->u_ActualScreenSizeX, value);
}
static ID_INLINE void R_GLSL_SetUniform_u_ActualScreenSizeY(glslProgram_t *program, GLint value) {
	qglUniform1iARB(program->u_ActualScreenSizeY, value);
}


static ID_INLINE void R_GLSL_SetUniform_rubyTextureSize(glslProgram_t *program, const GLint value, const GLint valub) {
//	if (VectorCompare(program->v_rubyTextureSize, value))
//		return;
//	VectorCopy(value, program->v_rubyTextureSize);
//	program->v_rubyTextureSize[0] = value;
//	program->v_rubyTextureSize[1] = valub;
//	qglUniform3fARB(program->rubyTextureSize, value, valub, 1.0);
}

static ID_INLINE void R_GLSL_SetUniform_rubyInputSize(glslProgram_t *program, const GLint value, const GLint valub) {
//	if (VectorCompare(program->v_rubyInputSize, value))
//		return;
//	program->v_rubyInputSize[0] = value;
//	program->v_rubyInputSize[1] = valub;
	//VectorCopy(vec2(value, valub), program->v_rubyInputSize);
//	qglUniform3fARB(program->rubyInputSize, value, valub, 1.0);
}

static ID_INLINE void R_GLSL_SetUniform_rubyOutputSize(glslProgram_t *program, const GLint value, const GLint valub) {
//	if (VectorCompare(program->v_rubyOutputSize, value))
//		return;
//	VectorCopy(value, program->v_rubyOutputSize);
//	program->v_rubyOutputSize[0] = value;
//	program->v_rubyOutputSize[1] = valub;
//	qglUniform3fARB(program->rubyOutputSize, value, valub, 1.0);
}

void R_GLSL_Init(void);
qhandle_t RE_GLSL_RegisterProgram(const char *name, const char *programVertexObjects, int numVertexObjects, const char *programFragmentObjects, int numFragmentObjects);
qhandle_t RE_GLSL_RegisterProgramRaw(const char *name, const char *programVertexObjects, int numVertexObjects, const char *programFragmentObjects, int numFragmentObjects);
void R_GLSL_UseProgram(qhandle_t index);
void RB_GLSL_StageIteratorGeneric(void);
void RB_GLSL_StageIteratorVertexLitTexture(void);
void RB_GLSL_StageIteratorLightmappedMultitexture(void);
void RB_GLSL_StageIteratorSky(void);

/*
============================================================

LIGHTS

============================================================
*/

void R_DlightBmodel( bmodel_t *bmodel );
void R_SetupEntityLighting( const trRefdef_t *refdef, trRefEntity_t *ent );
void R_TransformDlights( int count, dlight_t *dl, orientationr_t *or );
int R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );


/*
============================================================

SHADOWS

============================================================
*/

void RB_ShadowTessEnd( void );
void RB_ShadowFinish( void );
void RB_ProjectionShadowDeform( void );

/*
============================================================

SKIES

============================================================
*/

void R_BuildCloudData( shaderCommands_t *shader );
void R_InitSkyTexCoords( float cloudLayerHeight );
void R_DrawSkyBox( shaderCommands_t *shader );
void RB_DrawSun( void );
void RB_DrawSunFlare( void );
void RB_ClipSkyPolygons( shaderCommands_t *shader );

/*
============================================================

CURVE TESSELATION

============================================================
*/

#define PATCH_STITCHING

srfGridMesh_t *R_SubdividePatchToGrid( int width, int height,
								drawVert_t points[MAX_PATCH_SIZE*MAX_PATCH_SIZE] );
srfGridMesh_t *R_GridInsertColumn( srfGridMesh_t *grid, int column, int row, vec3_t point, float loderror );
srfGridMesh_t *R_GridInsertRow( srfGridMesh_t *grid, int row, int column, vec3_t point, float loderror );
void R_FreeSurfaceGridMesh( srfGridMesh_t *grid );

/*
============================================================

MARKERS, POLYGON PROJECTION ON WORLD POLYGONS

============================================================
*/

int R_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection,
				   int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );


/*
============================================================

SCENE GENERATION

============================================================
*/

void R_InitNextFrame( void );

void RE_ClearScene( void );
void RE_AddRefEntityToScene( const refEntity_t *ent );
void RE_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts, int num );
void RE_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void RE_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void RE_RenderScene( const refdef_t *fd );

/*
=============================================================

UNCOMPRESSING BONES

=============================================================
*/

#define MC_BITS_X (16)
#define MC_BITS_Y (16)
#define MC_BITS_Z (16)
#define MC_BITS_VECT (16)

#define MC_SCALE_X (1.0f/64)
#define MC_SCALE_Y (1.0f/64)
#define MC_SCALE_Z (1.0f/64)

void MC_UnCompress(float mat[3][4],const unsigned char * comp);

/*
=============================================================

ANIMATED MODELS

=============================================================
*/

void R_MDRAddAnimSurfaces( trRefEntity_t *ent );
void RB_MDRSurfaceAnim( mdrSurface_t *surface );
#ifdef BROKEN_IQM
qboolean R_LoadIQM (model_t *mod, void *buffer, int filesize, const char *name );
void R_AddIQMSurfaces( trRefEntity_t *ent );
void RB_IQMSurfaceAnim( surfaceType_t *surface );
int R_IQMLerpTag( orientation_t *tag, iqmData_t *data,
                  int startFrame, int endFrame,
                  float frac, const char *tagName );
#endif
/*
=============================================================

IMAGE LOADERS

=============================================================
*/

void R_LoadBMP( const char *name, byte **pic, int *width, int *height );
void R_LoadJPG( const char *name, byte **pic, int *width, int *height );
void R_LoadPCX( const char *name, byte **pic, int *width, int *height );
void R_LoadPNG( const char *name, byte **pic, int *width, int *height );
void R_LoadTGA( const char *name, byte **pic, int *width, int *height );
//void R_LoadDDS( const char *name, byte **pic, int *width, int *height );
/*
=============================================================
=============================================================
*/
void	R_TransformModelToClip( const vec3_t src, const float *modelMatrix, const float *projectionMatrix,
							vec4_t eye, vec4_t dst );
void	R_TransformClipToWindow( const vec4_t clip, const viewParms_t *view, vec4_t normalized, vec4_t window );

void	RB_DeformTessGeometry( void );

void	RB_CalcEnvironmentTexCoords( float *dstTexCoords );
void    RB_CalcEnvironmentTexCoordsEx( float *st, int xx, int yy, int mode ); // leilei - extra envmapping

void    RB_CalcEyes( float *st, qboolean theothereye); // leilei - eyes

void	RB_CalcFogTexCoords( float *dstTexCoords );
void	RB_CalcScrollTexCoords( const float scroll[2], float *dstTexCoords );
void	RB_CalcRotateTexCoords( float rotSpeed, float *dstTexCoords );
void	RB_CalcScaleTexCoords( const float scale[2], float *dstTexCoords );
void	RB_CalcTurbulentTexCoords( const waveForm_t *wf, float *dstTexCoords );
void	RB_CalcTransformTexCoords( const texModInfo_t *tmi, float *dstTexCoords );
void	RB_CalcModulateColorsByFog( unsigned char *dstColors );
void	RB_CalcModulateAlphasByFog( unsigned char *dstColors );
void	RB_CalcModulateRGBAsByFog( unsigned char *dstColors );
void	RB_CalcWaveAlpha( const waveForm_t *wf, unsigned char *dstColors );
void	RB_CalcWaveColor( const waveForm_t *wf, unsigned char *dstColors );
void	RB_CalcAlphaFromEntity( unsigned char *dstColors );
void	RB_CalcAlphaFromOneMinusEntity( unsigned char *dstColors );
void	RB_CalcStretchTexCoords( const waveForm_t *wf, float *texCoords );
void	RB_CalcAtlasTexCoords( const atlas_t *at, float *st );
void	RB_CalcColorFromEntity( unsigned char *dstColors );
void	RB_CalcColorFromOneMinusEntity( unsigned char *dstColors );
void	RB_CalcSpecularAlpha( unsigned char *alphas );
void	RB_CalcSpecularAlphaNew( unsigned char *alphas );
void 	RB_CalcGlowBlend( unsigned char *colors, int glowcol, int fx ); // leilei - rgbMod
void 	RB_CalcUVColor( unsigned char *colors, int glowcol, int fx ); 	// leilei - rgbMod
void 	RB_CalcNormalizeToAlpha( unsigned char *colors); 		// leilei - rgbMod 
void	RB_CalcDiffuseColor( unsigned char *colors );
void    RB_CalcMaterials( unsigned char *colors, int ambient, int diffuse, int specular, int emissive, int spechard, int alpha ); // leilei - materials

void	RB_CalcVertLights( unsigned char *colors );	// leilei - dynamic vertex lights

/*
=============================================================

RENDERER BACK END FUNCTIONS

=============================================================
*/

void RB_ExecuteRenderCommands( const void *data );

/*
=============================================================

RENDERER BACK END COMMAND QUEUE

=============================================================
*/

#define	MAX_RENDER_COMMANDS	0x40000

typedef struct {
	byte	cmds[MAX_RENDER_COMMANDS];
	int		used;
} renderCommandList_t;

typedef struct {
	int		commandId;
	float	color[4];
} setColorCommand_t;

typedef struct {
	int		commandId;
	int		buffer;
} drawBufferCommand_t;

typedef struct {
	int		commandId;
	image_t	*image;
	int		width;
	int		height;
	void	*data;
} subImageCommand_t;

typedef struct {
	int		commandId;
} swapBuffersCommand_t;

typedef struct {
	int		commandId;
	int		buffer;
} endFrameCommand_t;

typedef struct {
	int		commandId;
	shader_t	*shader;
	float	x, y;
	float	w, h;
	float	s1, t1;
	float	s2, t2;
} stretchPicCommand_t;

typedef struct {
	int		commandId;
	trRefdef_t	refdef;
	viewParms_t	viewParms;
	drawSurf_t *drawSurfs;
	int		numDrawSurfs;
} drawSurfsCommand_t;

typedef struct {
	int commandId;
	int x;
	int y;
	int width;
	int height;
	char *fileName;
	qboolean jpeg;
} screenshotCommand_t;

typedef struct {
	int						commandId;
	int						width;
	int						height;
	byte					*captureBuffer;
	byte					*encodeBuffer;
	qboolean			motionJpeg;
} videoFrameCommand_t;

typedef struct
{
	int commandId;

	GLboolean rgba[4];
} colorMaskCommand_t;

typedef struct
{
	int commandId;
} clearDepthCommand_t;

typedef enum {
	RC_END_OF_LIST,
	RC_SET_COLOR,
	RC_STRETCH_PIC,
	RC_DRAW_SURFS,
	RC_DRAW_BUFFER,
	RC_SWAP_BUFFERS,
	RC_SCREENSHOT,
	RC_VIDEOFRAME,
	RC_COLORMASK,
	RC_CLEARDEPTH
} renderCommand_t;


// these are sort of arbitrary limits.
// the limits apply to the sum of all scenes in a frame --
// the main view, all the 3D icons, etc
#define	MAX_POLYS		600
#define	MAX_POLYVERTS	3000

// all of the information needed by the back end must be
// contained in a backEndData_t
typedef struct {
	drawSurf_t	drawSurfs[MAX_DRAWSURFS];
	dlight_t	dlights[MAX_DLIGHTS];
	trRefEntity_t	entities[MAX_REFENTITIES];
	srfPoly_t	*polys;//[MAX_POLYS];
	polyVert_t	*polyVerts;//[MAX_POLYVERTS];
	renderCommandList_t	commands;
} backEndData_t;

extern	int		max_polys;
extern	int		max_polyverts;

extern	backEndData_t	*backEndData;	// the second one may not be allocated


void *R_GetCommandBuffer( int bytes );
void RB_ExecuteRenderCommands( const void *data );

void R_IssuePendingRenderCommands( void );

void R_AddDrawSurfCmd( drawSurf_t *drawSurfs, int numDrawSurfs );

void RE_SetColor( const float *rgba );
void RE_StretchPic ( float x, float y, float w, float h, 
					  float s1, float t1, float s2, float t2, qhandle_t hShader );
void RE_BeginFrame( stereoFrame_t stereoFrame );
void RE_EndFrame( int *frontEndMsec, int *backEndMsec );
void RE_SaveJPG(char * filename, int quality, int image_width, int image_height,
                unsigned char *image_buffer, int padding);
size_t RE_SaveJPGToBuffer(byte *buffer, size_t bufSize, int quality,
		          int image_width, int image_height, byte *image_buffer, int padding);
void RE_TakeVideoFrame( int width, int height,
		byte *captureBuffer, byte *encodeBuffer, qboolean motionJpeg );

//Bloom Stuff
void R_BloomInit( void );
void R_BloomScreen( void );
// Postprocessing
void R_PostprocessScreen( void );
void R_PostprocessingInit(void);

// leilei
void R_BrightScreen( void );
void R_AltBrightnessInit( void );
extern int softwaremode;
extern int leifxmode;
extern int voodootype; // 0 - none 1 - Voodoo Graphics 2 - Voodoo2, 3 - Voodoo Banshee/3, 4 - Voodoo4/5

void R_AddParticles (void);
void R_RenderParticles (void);
void R_ClearParticles (void);
void R_LFX_Spark (const vec3_t org, const vec3_t dir, float spread, float speed, vec4_t color1, vec4_t color2, vec4_t color3, vec4_t color4, vec4_t color5, int count, int duration, float scaleup, int blendfunc);
void R_LFX_Smoke (const vec3_t org, const vec3_t dir, float spread, float speed, vec4_t color1, vec4_t color2, vec4_t color3, vec4_t color4, vec4_t color5, int count, int duration, float scaleup, int blendfunc);
void R_LFX_Smoke2 (const vec3_t org, const vec3_t dir, float spread, float speed, vec4_t color1, vec4_t color2, vec4_t color3, vec4_t color4, vec4_t color5, int count, int duration, float scale, float scaleup, int blendfunc);
void R_LFX_Shock (const vec3_t org, const vec3_t dir, float spread, float speed, vec4_t color1, vec4_t color2, vec4_t color3, vec4_t color4, vec4_t color5, int count, int duration, float scaleup, int blendfunc);
void R_LFX_Burst (const vec3_t org, const vec3_t dir, float spread, float speed, vec4_t color1, vec4_t color2, vec4_t color3, vec4_t color4, vec4_t color5, int count, int duration, float scaleup, int blendfunc);
void R_LFX_PushSmoke (const vec3_t there, float force);

void R_RunParticleEffect (const vec3_t org, const vec3_t dir, int color, int count);
void R_QarticleExplosion(const vec3_t org);
void R_LFX_Blood (const vec3_t org, const vec3_t dir, float pressure) ;
void LFX_ShaderInit(void);
void LFX_ParticleEffect (int effect, const vec3_t org, const vec3_t dir);
void RE_GetViewPosition(vec3_t point);

#endif //TR_LOCAL_H

