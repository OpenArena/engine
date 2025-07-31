/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2007 HermitWorks Entertainment Corporation
Copyright (C) 2006-2009 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of XreaL source code.

XreaL source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

XreaL source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with XreaL source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

//
// leilei - this is altered a bit from the usual XreaL/Space Trader image_dds
//
//	- Removed depth and cubemap stuff as that is not relevant to our spec (we just want faster textures in less bits)
//	- Removed software decoding (patent concerns even if it's expired by now)
//
//	TODO:
//	- Non-square textures don't get uploaded properly!!!
//	- DXT3/5 are not handled properly for alpha support.
//	- backport LadyHavoc's halfsize-decoding from Darkplaces
//	- figure out why the x.org turks driver fails with this.
//
//
//	Because it's super incomplete, i'm shoving it in a define.
//
#ifdef BROKEN_DDS
#include "qgl.h"
#include "../renderer_oa/tr_local.h"

//extern void (APIENTRY * qglColorTableEXT)( GLint, GLint, GLint, GLint, GLint, const GLvoid);
//extern void (APIENTRY * qglColorTableSGI)( GLint, GLint, GLint, GLint, GLint, const GLvoid);
//extern void (APIENTRY * qglCompressedTexImage2DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);

typedef struct
{
	unsigned int   dwColorSpaceLowValue;	// low boundary of color space that is to
	// be treated as Color Key, inclusive

	unsigned int   dwColorSpaceHighValue;	// high boundary of color space that is
	// to be treated as Color Key, inclusive
} DDCOLORKEY_t;

typedef struct
{
	unsigned int   dwCaps;		// capabilities of surface wanted
	unsigned int   dwCaps2;
	unsigned int   dwCaps3;
	union
	{
		unsigned int   dwCaps4;
		unsigned int   dwVolumeDepth;
	} u0;
} DDSCAPS2_t;

typedef struct
{
	unsigned int   dwSize;		// size of structure
	unsigned int   dwFlags;	// pixel format flags
	unsigned int   dwFourCC;	// (FOURCC code)

	union
	{
		unsigned int   dwRGBBitCount;	// how many bits per pixel
		unsigned int   dwYUVBitCount;	// how many bits per pixel
		unsigned int   dwZBufferBitDepth;	// how many total bits/pixel in z buffer (including any stencil bits)
		unsigned int   dwAlphaBitDepth;	// how many bits for alpha channels
		unsigned int   dwLuminanceBitCount;	// how many bits per pixel
		unsigned int   dwBumpBitCount;	// how many bits per "buxel", total
		unsigned int   dwPrivateFormatBitCount;	// Bits per pixel of private driver formats. Only valid in texture
		// format list and if DDPF_D3DFORMAT is set
	} u0;

	union
	{
		unsigned int   dwRBitMask;	// mask for red bit
		unsigned int   dwYBitMask;	// mask for Y bits
		unsigned int   dwStencilBitDepth;	// how many stencil bits (note: dwZBufferBitDepth-dwStencilBitDepth is total Z-only bits)
		unsigned int   dwLuminanceBitMask;	// mask for luminance bits
		unsigned int   dwBumpDuBitMask;	// mask for bump map U delta bits
		unsigned int   dwOperations;	// DDPF_D3DFORMAT Operations
	} u1;

	union
	{
		unsigned int   dwGBitMask;	// mask for green bits
		unsigned int   dwUBitMask;	// mask for U bits
		unsigned int   dwZBitMask;	// mask for Z bits
		unsigned int   dwBumpDvBitMask;	// mask for bump map V delta bits
		struct
		{
			unsigned short  wFlipMSTypes;	// Multisample methods supported via flip for this D3DFORMAT
			unsigned short  wBltMSTypes;	// Multisample methods supported via blt for this D3DFORMAT
		} MultiSampleCaps;

	} u2;

	union
	{
		unsigned int   dwBBitMask;	// mask for blue bits
		unsigned int   dwVBitMask;	// mask for V bits
		unsigned int   dwStencilBitMask;	// mask for stencil bits
		unsigned int   dwBumpLuminanceBitMask;	// mask for luminance in bump map
	} u3;

	union
	{
		unsigned int   dwRGBAlphaBitMask;	// mask for alpha channel
		unsigned int   dwYUVAlphaBitMask;	// mask for alpha channel
		unsigned int   dwLuminanceAlphaBitMask;	// mask for alpha channel
		unsigned int   dwRGBZBitMask;	// mask for Z channel
		unsigned int   dwYUVZBitMask;	// mask for Z channel
	} u4;
} DDPIXELFORMAT_t;

typedef struct
{
	unsigned int   dwSize;		// size of the DDSURFACEDESC structure
	unsigned int   dwFlags;	// determines what fields are valid
	unsigned int   dwHeight;	// height of surface to be created
	unsigned int   dwWidth;	// width of input surface

	union
	{
		int            lPitch;	// distance to start of next line (return value only)
		unsigned int   dwLinearSize;	// Formless late-allocated optimized surface size
	} u0;

	union
	{
		unsigned int   dwBackBufferCount;	// number of back buffers requested
		unsigned int   dwDepth;	// the depth if this is a volume texture
	} u1;

	union
	{
		unsigned int   dwMipMapCount;	// number of mip-map levels requestde
		// dwZBufferBitDepth removed, use ddpfPixelFormat one instead
		unsigned int   dwRefreshRate;	// refresh rate (used when display mode is described)
		unsigned int   dwSrcVBHandle;	// The source used in VB::Optimize
	} u2;

	unsigned int   dwAlphaBitDepth;	// depth of alpha buffer requested
	unsigned int   dwReserved;	// reserved
	void           *lpSurface;	// pointer to the associated surface memory

	union
	{
		DDCOLORKEY_t    ddckCKDestOverlay;	// color key for destination overlay use
		unsigned int   dwEmptyFaceColor;	// Physical color for empty cubemap faces
	} u3;
	DDCOLORKEY_t    ddckCKDestBlt;	// color key for destination blt use
	DDCOLORKEY_t    ddckCKSrcOverlay;	// color key for source overlay use
	DDCOLORKEY_t    ddckCKSrcBlt;	// color key for source blt use
	union
	{
		DDPIXELFORMAT_t ddpfPixelFormat;	// pixel format description of the surface
		unsigned int   dwFVF;	// vertex format description of vertex buffers
	} u4;
	DDSCAPS2_t      ddsCaps;	// direct draw surface capabilities
	unsigned int   dwTextureStage;	// stage in multitexture cascade
} DDSURFACEDESC2_t;

//
// DDSURFACEDESC2 flags that mark the validity of the struct data
//
#define DDSD_CAPS								0x00000001l	// default
#define DDSD_HEIGHT								0x00000002l	// default
#define DDSD_WIDTH								0x00000004l	// default
#define DDSD_PIXELFORMAT						0x00001000l	// default
#define DDSD_PITCH								0x00000008l	// For uncompressed formats
#define DDSD_MIPMAPCOUNT						0x00020000l
#define DDSD_LINEARSIZE							0x00080000l	// For compressed formats
#define DDSD_DEPTH								0x00800000l	// Volume Textures

//
// DDPIXELFORMAT flags
//
#define DDPF_ALPHAPIXELS						0x00000001l
#define DDPF_FOURCC								0x00000004l	// Compressed formats
#define DDPF_RGB								0x00000040l	// Uncompressed formats
#define DDPF_ALPHA								0x00000002l
#define DDPF_COMPRESSED							0x00000080l
#define DDPF_LUMINANCE							0x00020000l
#define DDPF_PALETTEINDEXED4					0x00000008l
#define DDPF_PALETTEINDEXEDTO8					0x00000010l
#define DDPF_PALETTEINDEXED8					0x00000020l

//
// DDSCAPS flags
//
#define DDSCAPS_COMPLEX							0x00000008l
#define DDSCAPS_TEXTURE							0x00001000l	// default
#define DDSCAPS_MIPMAP							0x00400000l

#define DDSCAPS2_VOLUME							0x00200000l
#define DDSCAPS2_CUBEMAP						0x00000200L
#define DDSCAPS2_CUBEMAP_POSITIVEX				0x00000400L
#define DDSCAPS2_CUBEMAP_NEGATIVEX				0x00000800L
#define DDSCAPS2_CUBEMAP_POSITIVEY				0x00001000L
#define DDSCAPS2_CUBEMAP_NEGATIVEY				0x00002000L
#define DDSCAPS2_CUBEMAP_POSITIVEZ				0x00004000L
#define DDSCAPS2_CUBEMAP_NEGATIVEZ				0x00008000L

#ifndef MAKEFOURCC

#define MAKEFOURCC(ch0, ch1, ch2, ch3)											\
    ((unsigned int)(char)(ch0) | ((unsigned int)(char)(ch1) << 8) |			\
    ((unsigned int)(char)(ch2) << 16) | ((unsigned int)(char)(ch3) << 24 ))

#endif

#define FOURCC_DDS		MAKEFOURCC( 'D', 'D', 'S', ' ' )

//FOURCC codes for DXTn compressed-texture pixel formats
#define FOURCC_DXT1		MAKEFOURCC( 'D', 'X', 'T', '1' )
#define FOURCC_DXT2		MAKEFOURCC( 'D', 'X', 'T', '2' )
#define FOURCC_DXT3		MAKEFOURCC( 'D', 'X', 'T', '3' )
#define FOURCC_DXT4		MAKEFOURCC( 'D', 'X', 'T', '4' )
#define FOURCC_DXT5		MAKEFOURCC( 'D', 'X', 'T', '5' )

#define R_LoadDDSImage_MAX_MIPS 16


int		swdecode = 0;	// not really working

static void R_UploadCompressedImage2D(image_t * img, GLenum target, int level, GLenum format, int width, int height, int size,
									  const void *data)
{
	GLenum          int_fmat;

	//if(glConfig.textureCompression == TC_S3TC)

	if (!swdecode)
	{
		qglCompressedTexImage2DARB(target, level, format, width, height, 0, size, data);
		
		return;
	}
	else
	{


		// nothing atol

	}

	// leilei - software decoding is ALL STRIPPED!!! Not only for simplicity,
	//          but there is a patent issue regarding the decoding.
	//			So, we only support DDS textures though directly
	//		using the driver itself to upload and don't even try to decode
	//		any of it. Not even for a lousy PowerVR alpha hack.
	//			Besides. the intent is to get those old dmm maps working
	//		as intended. I don't really have big plans for DDS.
}

static void R_UploadImage2D(image_t * img, GLenum target, int level, GLenum int_fmat,
							int width, int height, GLenum format, GLenum type, const void *data)
{


	qglTexImage2D(target, level, int_fmat, width, height, 0, format, type, data);
}
extern image_t *R_AllocImage(const char *name, qboolean linkIntoHashTable);

image_t        *R_LoadDDSImageData(void *pImageData, const char *name, int bits,  filterType_t filterType, wrapType_t wrapType)
{
	image_t        *ret = NULL;

	byte           *buff;

	DDSURFACEDESC2_t *ddsd;		//used to get at the dds header in the read buffer

	//based on texture type:
	//  cube        width != 0, height == 0, depth == 0
	//  2D          width != 0, height != 0, depth == 0
	//  volume      width != 0, height != 0, depth != 0
	int             width, height, depth;

	//mip count and pointers to image data for each mip
	//level, idx 0 = top level last pointer does not start
	//a mip level, it's just there to mark off the end of
	//the final mip data segment (thus the odd + 1)
	//
	//for cube textures we only find the offsets into the
	//first face of the cube, subsequent faces will use the
	//same offsets, just shifted over
	int             mipLevels;
	byte           *mipOffsets[R_LoadDDSImage_MAX_MIPS + 1];

	qboolean        usingAlpha;

	qboolean        compressed;
	GLuint          format = 0;
	GLuint          internal_format = 0;
	GLenum          type = GL_UNSIGNED_BYTE;

	vec4_t          zeroClampBorder = { 0, 0, 0, 1 };
	vec4_t          alphaZeroClampBorder = { 0, 0, 0, 0 };

	//comes from R_CreateImage


	buff = (byte *) pImageData;

	if(strncmp((const char *)buff, "DDS ", 4) != 0)
	{
		ri.Printf(PRINT_WARNING, "R_LoadDDSImage: invalid dds header \"%s\"\n", name);
		goto ret_error;
	}

	ddsd = (DDSURFACEDESC2_t *) (buff + 4);

	if(ddsd->dwSize != sizeof(DDSURFACEDESC2_t) || ddsd->u4.ddpfPixelFormat.dwSize != sizeof(DDPIXELFORMAT_t))
	{
		ri.Printf(PRINT_WARNING, "R_LoadDDSImage: invalid dds header \"%s\"\n", name);
		goto ret_error;
	}

	usingAlpha = (ddsd->u4.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS) ? qtrue : qfalse;
	mipLevels = ((ddsd->dwFlags & DDSD_MIPMAPCOUNT) && (ddsd->u2.dwMipMapCount > 1)) ? ddsd->u2.dwMipMapCount : 1;

	if(mipLevels > R_LoadDDSImage_MAX_MIPS)
	{
		ri.Printf(PRINT_WARNING, "R_LoadDDSImage: dds image has too many mip levels \"%s\"\n", name);
		goto ret_error;
	}

	compressed = (ddsd->u4.ddpfPixelFormat.dwFlags & DDPF_FOURCC) ? qtrue : qfalse;

	// either a cube or a volume
	if(ddsd->ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP)
	{
		// cube texture

		if(ddsd->dwWidth != ddsd->dwHeight)
		{
			ri.Printf(PRINT_WARNING, "R_LoadDDSImage: invalid dds image \"%s\"\n", name);
			goto ret_error;
		}

		width = ddsd->dwWidth;
		height = 0;
		depth = 0;

		if(width & (width - 1))
		{
			//cubes must be a power of two
			ri.Printf(PRINT_WARNING, "R_LoadDDSImage: cube images must be power of two \"%s\"\n", name);
			goto ret_error;
		}
	}
	else if((ddsd->ddsCaps.dwCaps2 & DDSCAPS2_VOLUME) && (ddsd->dwFlags & DDSD_DEPTH))
	{
		// 3D texture

		width = ddsd->dwWidth;
		height = ddsd->dwHeight;
		depth = ddsd->u1.dwDepth;

		if(width & (width - 1) || height & (height - 1) || depth & (depth - 1))
		{
			ri.Printf(PRINT_WARNING, "R_LoadDDSImage: volume images must be power of two \"%s\"\n", name);
			goto ret_error;
		}
	}
	else
	{
		// 2D texture

		width = ddsd->dwWidth;
		height = ddsd->dwHeight;
		depth = 0;

		//these are allowed to be non power of two, will be dealt with later on
		//except for compressed images!
		if(compressed && (width & (width - 1) || height & (height - 1)))
		{
			ri.Printf(PRINT_WARNING, "R_LoadDDSImage: compressed texture images must be power of two \"%s\"\n", name);
			goto ret_error;
		}
	}

	if(compressed)
	{
		int             blockSize;

		if(depth != 0)
		{
			ri.Printf(PRINT_WARNING, "R_LoadDDSImage: compressed volume textures are not supported \"%s\"\n", name);
			goto ret_error;
		}

		//compressed FOURCC formats
		switch (ddsd->u4.ddpfPixelFormat.dwFourCC)
		{
			case FOURCC_DXT1:
				blockSize = 8;
				usingAlpha = qtrue;
				format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
				break;

			case FOURCC_DXT3:
				blockSize = 16;
				usingAlpha = qtrue;
				format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
				break;

			case FOURCC_DXT5:
				blockSize = 16;
				usingAlpha = qtrue;
				format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
				break;

			default:
				ri.Printf(PRINT_WARNING, "R_LoadDDSImage: unsupported FOURCC \"%s\", \"%s\"\n",
						  &ddsd->u4.ddpfPixelFormat.dwFourCC, name);
				goto ret_error;
		}

		//get mip offsets
		if(format)
		{
			int             w = width;
			int             h = height;
			int             offset = 0;
			int             i;

			if(h == 0)
				h = w;			//cube map

			for(i = 0; (i < mipLevels) && (w || h); i++)
			{
				int             qw, qh;

				mipOffsets[i] = buff + 4 + sizeof(DDSURFACEDESC2_t) + offset;

				if(w == 0)
					w = 1;
				if(h == 0)
					h = 1;

				//size formula comes from DX docs (August 2005 SDK reference)
				qw = w >> 2;
				if(qw == 0)
					qw = 1;
				qh = h >> 2;
				if(qh == 0)
					qh = 1;

				offset += qw * qh * blockSize;

				w >>= 1;
				h >>= 1;
			}

			//put in the trailing offset
			mipOffsets[i] = buff + 4 + sizeof(DDSURFACEDESC2_t) + offset;
		}
		else
		{
			ri.Printf(PRINT_WARNING, "R_LoadDDSImage: error reading DDS image \"%s\"\n", name);
			goto ret_error;
		}

		internal_format = format;
	}
	else
	{
		// uncompressed format
		if(ddsd->u4.ddpfPixelFormat.dwFlags & DDPF_RGB)
		{
			switch (ddsd->u4.ddpfPixelFormat.u0.dwRGBBitCount)
			{
				case 32:
					internal_format = usingAlpha ? GL_RGBA8 : GL_RGB8;
					format = GL_BGRA_EXT;
					break;

				case 24:
					internal_format = GL_RGB8;
					format = GL_BGR_EXT;
					break;

				case 16:
					if(usingAlpha)
					{
						//must be A1R5G5B5
						ri.Printf(PRINT_WARNING, "R_LoadDDSImage: unsupported format, 5551 \"%s\"\n", name);
						goto ret_error;
					}
					else
					{
						//find out if it's X1R5G5B5 or R5G6B5

						internal_format = GL_RGB5;
						format = GL_RGB;

						if(ddsd->u4.ddpfPixelFormat.u2.dwGBitMask == 0x7E0)
							type = GL_UNSIGNED_SHORT_5_6_5;
						else
						{
							ri.Printf(PRINT_WARNING, "R_LoadDDSImage: unsupported format, 5551 \"%s\"\n", name);
							goto ret_error;
						}
					}
					break;

				default:
					ri.Printf(PRINT_WARNING, "R_LoadDDSImage: unsupported RGB bit depth \"%s\"\n", name);
					goto ret_error;
			}
		}
		else if(ddsd->u4.ddpfPixelFormat.dwFlags & DDPF_LUMINANCE)
		{
			internal_format = usingAlpha ? GL_LUMINANCE8_ALPHA8 : GL_LUMINANCE8;
			format = usingAlpha ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;
		}
		else if(ddsd->u4.ddpfPixelFormat.dwFlags & DDPF_ALPHA)
		{
			internal_format = GL_ALPHA8;
			format = GL_ALPHA;
		}
		else if(ddsd->u4.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED4)
		{
			internal_format = usingAlpha ? GL_RGB5_A1 : GL_RGB5;
			format = GL_COLOR_INDEX4_EXT;
		}
		else if(ddsd->u4.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8)
		{
			internal_format = usingAlpha ? GL_RGBA : GL_RGB;
			format = GL_COLOR_INDEX8_EXT;
		}
		else
		{
			ri.Printf(PRINT_WARNING, "R_LoadDDSImage: unsupported DDS image type \"%s\"\n", name);
			goto ret_error;
		}

		if(format)
		{
			int             offset = 0;
			int             w = width;
			int             h = height;
			int             d = depth;
			int             i;

			if(h == 0)
				h = w;			//cube map

			for(i = 0; (i < mipLevels) && (w || h || d); i++)
			{
				mipOffsets[i] = buff + 4 + sizeof(DDSURFACEDESC2_t) + offset;

				if(w == 0)
					w = 1;
				if(h == 0)
					h = 1;
				if(d == 0)
					d = 1;

				offset += (w * h * d * (ddsd->u4.ddpfPixelFormat.u0.dwRGBBitCount / 8));

				w >>= 1;
				h >>= 1;
				d >>= 1;
			}

			//put in the trailing offset
			mipOffsets[i] = buff + 4 + sizeof(DDSURFACEDESC2_t) + offset;
		}
		else
		{
			ri.Printf(PRINT_WARNING, "R_LoadDDSImage: Unexpected error reading DDS image \"%s\"\n", name);
			goto ret_error;
		}
	}

	//we now have a full description of our image set up
	//if we fail after this point we still want our image_t
	//record in the hash so that we don't go trying to read
	//the file again - been printing an error and returning
	//NULL up to this point...
	ret = R_AllocImage(name, qtrue);

	ret->uploadWidth = ret->width = width;
	ret->uploadHeight = ret->height = height;

	ret->internalFormat = internal_format;

	ret->filterType = ((filterType == FT_DEFAULT) && (mipLevels > 1)) ? FT_DEFAULT : FT_LINEAR;

	if(ret->filterType == FT_LINEAR)
		mipLevels = 1;

	// leilei - we do not support depth or cubemaps.

	{
		// planar texture

		int             w = width;
		int             h = height;

		int             i;

		GLuint          texnum;

		if(w & (w - 1) || h & (h - 1))
		{
			// non-pow2: check extensions

		//	if(glConfig.textureNPOTAvailable)
		//	{
		//		ret->type = GL_TEXTURE_2D;
		//	}
		//	else
			{
				ret->type = GL_TEXTURE_2D; // FIXME R_StateGetRectTarget();
			}
		}
		else
		{
			ret->type = GL_TEXTURE_2D;
		}

		texnum = ret->texnum;

		if (w != h){ // leilei - rectangle texture hack
			//ret->type = GL_TEXTURE_RECTANGLE_ARB;	
			h = w;
			ri.Printf(PRINT_WARNING, "R_LoadDDSImage: uploading %s, %i x %i\n", ret, w, h);
		}

		GL_Bind(ret);

		if(mipLevels == 1)
			qglTexParameteri(ret->type, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);

		for(i = 0; i < mipLevels; i++)
		{
			if(compressed)
			{
				GLsizei         size = mipOffsets[i + 1] - mipOffsets[i];
				R_UploadCompressedImage2D(ret, ret->type, i, internal_format, w, h, size, mipOffsets[i]);
			}
			else
			{
				R_UploadImage2D(ret, ret->type, i, internal_format, w, h, format, type, mipOffsets[i]);
			}

			w >>= 1;
			if(w == 0)
				w = 1;
			h >>= 1;
			if(h == 0)
				h = 1;
		}
	}


	GL_CheckErrors();

	// set filter type
	ret->filterType = filterType;

	// who cares about filtertype?
		if(textureFilterAnisotropic)
			qglTexParameterf(ret->type, GL_TEXTURE_MAX_ANISOTROPY_EXT, r_ext_texture_filter_anisotropic->value);

			qglTexParameterf(ret->type, GL_TEXTURE_MIN_FILTER, gl_filter_min);
			qglTexParameterf(ret->type, GL_TEXTURE_MAG_FILTER, gl_filter_max);

	GL_CheckErrors();

	// set wrap type
	// we do care about this though. bouncepads etc
	ret->wrapType = wrapType;

	switch (wrapType)
	{
		case WT_REPEAT:
			qglTexParameterf(ret->type, GL_TEXTURE_WRAP_S, GL_REPEAT);
			qglTexParameterf(ret->type, GL_TEXTURE_WRAP_T, GL_REPEAT);
			break;

		case WT_CLAMP:
			qglTexParameterf(ret->type, GL_TEXTURE_WRAP_S, GL_CLAMP);
			qglTexParameterf(ret->type, GL_TEXTURE_WRAP_T, GL_CLAMP);
			break;

		case WT_EDGE_CLAMP:
			qglTexParameterf(ret->type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			qglTexParameterf(ret->type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			break;

		case WT_ZERO_CLAMP:
			qglTexParameterf(ret->type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			qglTexParameterf(ret->type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			qglTexParameterfv(ret->type, GL_TEXTURE_BORDER_COLOR, zeroClampBorder);
			break;

		case WT_ALPHA_ZERO_CLAMP:
			qglTexParameterf(ret->type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			qglTexParameterf(ret->type, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			qglTexParameterfv(ret->type, GL_TEXTURE_BORDER_COLOR, alphaZeroClampBorder);
			break;

		default:
			ri.Printf(PRINT_WARNING, "WARNING: unknown wrap type for image '%s'\n", ret->imgName);
			qglTexParameterf(ret->type, GL_TEXTURE_WRAP_S, GL_REPEAT);
			qglTexParameterf(ret->type, GL_TEXTURE_WRAP_T, GL_REPEAT);
			break;
	}


	GL_CheckErrors();

	//GL_Unbind();
	qglBindTexture(ret->type, 0);

  ret_error:
	return ret;
}


image_t        *R_LoadDDSImage(const char *name, int bits,  filterType_t filterType, wrapType_t wrapType)
{
	image_t        *ret;
	byte           *buff;
	int             len;

	// comes from R_CreateImage
	/*
	if(tr.numImages == MAX_DRAWIMAGES)
	{
		ri.Printf(PRINT_WARNING, "R_LoadDDSImage: MAX_DRAWIMAGES hit\n");
		return NULL;
	}
	*/

	len = ri.FS_ReadFile(name, (void **)&buff);
	if(!buff)
		return 0;

	ret = R_LoadDDSImageData(buff, name, bits, filterType, wrapType);

	ri.FS_FreeFile(buff);

	return ret;
}

#endif // BROKEN_DDS