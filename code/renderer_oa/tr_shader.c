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

// tr_shader.c -- this file deals with the parsing and definition of shaders

static char *s_shaderText;

// the shader is parsed into these global variables, then copied into
// dynamically allocated memory if it is valid.
static	shaderStage_t	stages[MAX_SHADER_STAGES];		
static	shader_t		shader;
static	texModInfo_t	texMods[MAX_SHADER_STAGES][TR_MAX_TEXMODS];

#define FILE_HASH_SIZE		1024
static	shader_t*		hashTable[FILE_HASH_SIZE];

#define MAX_SHADERTEXT_HASH		2048
static char **shaderTextHashTable[MAX_SHADERTEXT_HASH];

extern cvar_t 	*r_mockvr;

extern int detailhack;

/*
================
return a hash value for the filename
================
*/
#ifdef __GNUCC__
  #warning TODO: check if long is ok here 
#endif
static long generateHashValue( const char *fname, const int size ) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (fname[i] != '\0') {
		letter = tolower(fname[i]);
		if (letter =='.') break;				// don't include extension
		if (letter =='\\') letter = '/';		// damn path names
		if (letter == PATH_SEP) letter = '/';		// damn path names
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (size-1);
	return hash;
}

/*
 * R_GLSL_GetProgramByHandle
 * Return adress of program for supplied handle
 
static glslProgram_t *R_GLSL_GetProgramByHandle(qhandle_t index) {
	glslProgram_t *program;

    out of range gets the default program 
	if (index < 1 || index >= tr.numPrograms)
		return tr.programs[0];

	program = tr.programs[index];

	return program;
}*/


/*
 * R_GLSL_AllocProgram
 * Reserve memory for program
 */
static glslProgram_t *R_GLSL_AllocProgram(void) {
#ifdef GLSL_BACKEND
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
	program->u_ScreenSizeX					= -1;
	program->u_ScreenSizeY					= -1;
	program->u_ScreenToNextPixelX			= -1;
	program->u_ScreenToNextPixelY			= -1;
	program->u_zFar							= -1;
	program->u_CC_Brightness		= -1;	
	program->u_CC_Gamma			= -1;
	program->u_CC_Overbright		= -1;
	program->u_CC_Contrast			= -1;
	program->u_CC_Saturation		= -1;
	program->u_ActualScreenSizeX					= -1;
	program->u_ActualScreenSizeY					= -1;

	program->rubyInputSize					= -1;
	program->rubyOutputSize					= -1;
	program->rubyTextureSize					= -1;
	program->rubyFrameCount					= -1;

	
	tr.programs[tr.numPrograms] = program;
	tr.numPrograms++;

	return program;
#endif
}


/*
 * R_GLSL_ParseProgram
 * Parse program for uniform locations
 */
static void R_GLSL_ParseProgram(glslProgram_t *program, char *_text) {
#ifdef GLSL_BACKEND
	char	**text = &_text;
	char	*token;

	token = COM_ParseExt(text, qtrue);
	while (token[0]) {
		if (!Q_stricmp(token, "uniform")) {
			token = COM_ParseExt(text, qfalse);
			if (!Q_stricmp(token, "int")) {
				token = COM_ParseExt(text, qfalse);
				if (!Q_stricmp(token, "u_AlphaGen;")) {
					program->u_AlphaGen = qglGetUniformLocationARB(program->program, "u_AlphaGen");
				} else if (!Q_stricmp(token, "u_ColorGen;")) {
					program->u_ColorGen = qglGetUniformLocationARB(program->program, "u_ColorGen");
				} else if (!Q_stricmp(token, "u_Greyscale;")) {
					program->u_Greyscale = qglGetUniformLocationARB(program->program, "u_Greyscale");
				} else if (!Q_stricmp(token, "u_TCGen0;")) {
					program->u_TCGen0 = qglGetUniformLocationARB(program->program, "u_TCGen0");
				} else if (!Q_stricmp(token, "u_TCGen1;")) {
					program->u_TCGen1 = qglGetUniformLocationARB(program->program, "u_TCGen1");
				} else if (!Q_stricmp(token, "u_TexEnv;")) {
					program->u_TexEnv = qglGetUniformLocationARB(program->program, "u_TexEnv");
				}  else if (!Q_stricmp(token, "u_ScreenSizeX;")) {
					program->u_ScreenSizeX = qglGetUniformLocationARB(program->program, "u_ScreenSizeX");
				}  else if (!Q_stricmp(token, "u_ScreenSizeY;")) {
					program->u_ScreenSizeY = qglGetUniformLocationARB(program->program, "u_ScreenSizeY");
				}  else if (!Q_stricmp(token, "u_ActualScreenSizeX;")) {
					program->u_ActualScreenSizeX = qglGetUniformLocationARB(program->program, "u_ActualScreenSizeX");
				}  else if (!Q_stricmp(token, "u_ActualScreenSizeY;")) {
					program->u_ActualScreenSizeY = qglGetUniformLocationARB(program->program, "u_ActualScreenSizeY");
				} else {
					ri.Printf(PRINT_WARNING, "WARNING: uniform int %s unrecognized in program %s\n", token, program->name);
				}
			} else if (!Q_stricmp(token, "float")) {
				token = COM_ParseExt(text, qfalse);
				if (!Q_stricmp(token, "u_IdentityLight;")) {
					program->u_IdentityLight = qglGetUniformLocationARB(program->program, "u_IdentityLight");
				} else if (!Q_stricmp(token, "u_LightDistance;")) {
					program->u_LightDistance = qglGetUniformLocationARB(program->program, "u_LightDistance");
				} else if (!Q_stricmp(token, "u_Time;")) {
					program->u_Time = qglGetUniformLocationARB(program->program, "u_Time");
				}  else if (!Q_stricmp(token, "u_ScreenToNextPixelX;")) {
					program->u_ScreenToNextPixelX = qglGetUniformLocationARB(program->program, "u_ScreenToNextPixelX");
				}  else if (!Q_stricmp(token, "u_ScreenToNextPixelY;")) {
					program->u_ScreenToNextPixelY = qglGetUniformLocationARB(program->program, "u_ScreenToNextPixelY");
				}  else if (!Q_stricmp(token, "u_zFar;")) {
					program->u_zFar = qglGetUniformLocationARB(program->program, "u_zFar");
				}  else if (!Q_stricmp(token, "u_CC_Brightness;")) {
					program->u_CC_Brightness = qglGetUniformLocationARB(program->program, "u_CC_Brightness");
				}  else if (!Q_stricmp(token, "u_CC_Overbright;")) {
					program->u_CC_Overbright = qglGetUniformLocationARB(program->program, "u_CC_Overbright");
				}  else if (!Q_stricmp(token, "u_CC_Gamma;")) {
					program->u_CC_Gamma = qglGetUniformLocationARB(program->program, "u_CC_Gamma");
				}  else if (!Q_stricmp(token, "u_CC_Contrast;")) {
					program->u_CC_Contrast = qglGetUniformLocationARB(program->program, "u_CC_Contrast");
				}  else if (!Q_stricmp(token, "u_CC_Saturation;")) {
					program->u_CC_Saturation = qglGetUniformLocationARB(program->program, "u_CC_Saturation");
				} else {
					ri.Printf(PRINT_WARNING, "WARNING: uniform float %s unrecognized in program %s\n", token, program->name);
				}
			} else if (!Q_stricmp(token, "mat4")) {
				token = COM_ParseExt(text, qfalse);
				if (!Q_stricmp(token, "u_ModelViewMatrix;")) {
					program->u_ModelViewMatrix = qglGetUniformLocationARB(program->program, "u_ModelViewMatrix");
				} else if (!Q_stricmp(token, "u_ModelViewProjectionMatrix;")) {
					program->u_ModelViewProjectionMatrix = qglGetUniformLocationARB(program->program, "u_ModelViewProjectionMatrix");
				} else if (!Q_stricmp(token, "u_ProjectionMatrix;")) {
					program->u_ProjectionMatrix = qglGetUniformLocationARB(program->program, "u_ProjectionMatrix");
				} else {
					ri.Printf(PRINT_WARNING, "WARNING: uniform mat4 %s unrecognized in program %s\n", token, program->name);
				}
			} else if (!Q_stricmp(token, "sampler2D")) {
				token = COM_ParseExt(text, qfalse);
				if (!Q_stricmp(token, "u_Texture0;")) {
					program->u_Texture0 = qglGetUniformLocationARB(program->program, "u_Texture0");
					R_GLSL_SetUniform_Texture0(program, 0);
				} else if (!Q_stricmp(token, "u_Texture1;")) {
					program->u_Texture1 = qglGetUniformLocationARB(program->program, "u_Texture1");
					R_GLSL_SetUniform_Texture1(program, 1);
				} else if (!Q_stricmp(token, "u_Texture2;")) {
					program->u_Texture2 = qglGetUniformLocationARB(program->program, "u_Texture2");
					R_GLSL_SetUniform_Texture2(program, 2);
				} else if (!Q_stricmp(token, "u_Texture3;")) {
					program->u_Texture3 = qglGetUniformLocationARB(program->program, "u_Texture3");
					R_GLSL_SetUniform_Texture3(program, 3);
				} else if (!Q_stricmp(token, "u_Texture4;")) {
					program->u_Texture4 = qglGetUniformLocationARB(program->program, "u_Texture4");
					R_GLSL_SetUniform_Texture4(program, 4);
				} else if (!Q_stricmp(token, "u_Texture5;")) {
					program->u_Texture5 = qglGetUniformLocationARB(program->program, "u_Texture5");
					R_GLSL_SetUniform_Texture5(program, 5);
				} else if (!Q_stricmp(token, "u_Texture6;")) {
					program->u_Texture6 = qglGetUniformLocationARB(program->program, "u_Texture6");
					R_GLSL_SetUniform_Texture6(program, 6);
				} else if (!Q_stricmp(token, "u_Texture7;")) {
					program->u_Texture7 = qglGetUniformLocationARB(program->program, "u_Texture7");
					R_GLSL_SetUniform_Texture7(program, 7);
				} else {
					ri.Printf(PRINT_WARNING, "WARNING: uniform sampler2D %s unrecognized in program %s\n", token, program->name);
				}
			} else if (!Q_stricmp(token, "vec2")) {
				token = COM_ParseExt(text, qfalse);
				if (!Q_stricmp(token, "rubyTextureSize;")) {
					program->rubyTextureSize = qglGetUniformLocationARB(program->program, "rubyTextureSize");
				} else if (!Q_stricmp(token, "rubyInputSize;")) {
					program->rubyInputSize = qglGetUniformLocationARB(program->program, "rubyInputSize");
				} else if (!Q_stricmp(token, "rubyOutputSize;")) {
					program->rubyOutputSize = qglGetUniformLocationARB(program->program, "rubyOutputSize");
				} else {
					ri.Printf(PRINT_WARNING, "WARNING: uniform vec2 %s unrecognized in program %s\n", token, program->name);
				}
			} else if (!Q_stricmp(token, "vec3")) {
				token = COM_ParseExt(text, qfalse);
				if (!Q_stricmp(token, "u_AmbientLight;")) {
					program->u_AmbientLight = qglGetUniformLocationARB(program->program, "u_AmbientLight");
				} else if (!Q_stricmp(token, "u_DynamicLight;")) {
					program->u_DynamicLight = qglGetUniformLocationARB(program->program, "u_DynamicLight");
				} else if (!Q_stricmp(token, "u_DirectedLight;")) {
					program->u_DirectedLight = qglGetUniformLocationARB(program->program, "u_DirectedLight");
				} else if (!Q_stricmp(token, "u_LightDirection;")) {
					program->u_LightDirection = qglGetUniformLocationARB(program->program, "u_LightDirection");
				} else if (!Q_stricmp(token, "u_ViewOrigin;")) {
					program->u_ViewOrigin = qglGetUniformLocationARB(program->program, "u_ViewOrigin");
				} else {
					ri.Printf(PRINT_WARNING, "WARNING: uniform vec3 %s unrecognized in program %s\n", token, program->name);
				}
			} else if (!Q_stricmp(token, "vec4")) {
				token = COM_ParseExt(text, qfalse);
				if (!Q_stricmp(token, "u_ConstantColor;")) {
					program->u_ConstantColor = qglGetUniformLocationARB(program->program, "u_ConstantColor");
				} else if (!Q_stricmp(token, "u_EntityColor;")) {
					program->u_EntityColor = qglGetUniformLocationARB(program->program, "u_EntityColor");
				} else if (!Q_stricmp(token, "u_FogColor;")) {
					program->u_FogColor = qglGetUniformLocationARB(program->program, "u_FogColor");
				} else {
					ri.Printf(PRINT_WARNING, "WARNING: uniform vec4 %s unrecognized in program %s\n", token, program->name);
				}
			} else {
				ri.Printf(PRINT_WARNING, "WARNING: uniform %s unrecognized in program %s\n", token, program->name);
			}
		}

		token = COM_ParseExt(text, qtrue);
	}
#endif
}

/*
 * R_GLSL_LoadProgram
 * Load, compile and link program
 */
static qboolean R_GLSL_LoadProgram(glslProgram_t *program, const char *name, const char *programVertexObjects, int numVertexObjects, const char *programFragmentObjects, int numFragmentObjects) {
#ifdef GLSL_BACKEND
	GLcharARB		*buffer_vp[MAX_PROGRAM_OBJECTS];
	GLcharARB		*buffer_fp[MAX_PROGRAM_OBJECTS];
	GLcharARB		*buffer;
	GLhandleARB		shader_vp;
	GLhandleARB		shader_fp;
	GLint			status;
	char			*str;
	int				size = 0;
	int				i;


	/* create program */
	program->program = qglCreateProgramObjectARB();

	/* vertex program */
	for (i = 0, str = (char *)programVertexObjects; i < numVertexObjects; i++, str += MAX_QPATH) {
		ri.Printf(PRINT_DEVELOPER, "... loading '%s'\n", str);

		size += ri.FS_ReadFile(str, (void **)&buffer_vp[i]);
		if (!buffer_vp[i]) {
			ri.Printf( PRINT_WARNING,  "Couldn't load %s", str);
			return qfalse;
		}

		/* compile vertex shader */
		shader_vp = qglCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
		qglShaderSourceARB(shader_vp, 1, (const GLcharARB **)&buffer_vp, NULL);
		qglCompileShaderARB(shader_vp);

		/* check for errors in vertex shader */
		qglGetObjectParameterivARB(shader_vp, GL_OBJECT_COMPILE_STATUS_ARB, &status);
		if (!status) {
			int		length;
			char	*msg;

			/* print glsl error message */
			qglGetObjectParameterivARB(shader_vp, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
			msg = ri.Hunk_AllocateTempMemory(length);
			qglGetInfoLogARB(shader_vp, length, &length, msg);
			ri.Printf(PRINT_ALL, "Error:\n%s\n", msg);
			ri.Hunk_FreeTempMemory(msg);

			/* exit */
			ri.Printf( PRINT_WARNING,  "Couldn't compile vertex shader for program %s", name);
			return qfalse;
		}

		/* attach vertex shader to program */
		qglAttachObjectARB(program->program, shader_vp);
		qglDeleteObjectARB(shader_vp);
	}

	/* fragment program */
	for (i = 0, str = (char *)programFragmentObjects; i < numFragmentObjects; i++, str += MAX_QPATH) {
		ri.Printf(PRINT_DEVELOPER, "... loading '%s'\n", str);

		size += ri.FS_ReadFile(str, (void **)&buffer_fp[i]);
		if (!buffer_fp[i]) {
			ri.Printf( PRINT_WARNING,  "Couldn't load %s", str);
			return qfalse;
		}

		/* compile fragment shader */
		shader_fp = qglCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
		qglShaderSourceARB(shader_fp, 1, (const GLcharARB **)&buffer_fp[i], NULL);
		qglCompileShaderARB(shader_fp);

		/* check for errors in fragment shader */
		qglGetObjectParameterivARB(shader_fp, GL_OBJECT_COMPILE_STATUS_ARB, &status);
		if (!status) {
			int		length;
			char	*msg;

			/* print glsl error message */
			qglGetObjectParameterivARB(shader_fp, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
			msg = ri.Hunk_AllocateTempMemory(length);
			qglGetInfoLogARB(shader_fp, length, &length, msg);
			ri.Printf(PRINT_ALL, "Error:\n%s\n", msg);
			ri.Hunk_FreeTempMemory(msg);

			/* exit */
			ri.Printf( PRINT_WARNING,  "Couldn't compile fragment shader for program %s", name);
			return qfalse;
		}

		/* attach fragment shader to program */
		//qglBindRenderbufferEXT(shader_fp, 0);
		//qglBindFramebufferEXT(shader_fp, 0);
		qglAttachObjectARB(program->program, shader_fp);
		qglDeleteObjectARB(shader_fp);
	}

	/* link complete program */
	qglLinkProgramARB(program->program);

	/* check for linking errors */
	qglGetObjectParameterivARB(program->program, GL_OBJECT_LINK_STATUS_ARB, &status);
	if (!status) {
		int		length;
		char	*msg;

		/* print glsl error message */
		qglGetObjectParameterivARB(program->program, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
		msg = ri.Hunk_AllocateTempMemory(length);
		qglGetInfoLogARB(program->program, length, &length, msg);
		ri.Printf(PRINT_ALL, "Error:\n%s\n", msg);
		ri.Hunk_FreeTempMemory(msg);

		/* exit */
		ri.Printf( PRINT_WARNING,  "Couldn't link shaders for program %s", name);
		return qfalse;
	}

	/* build single large program file for parsing */
	buffer = ri.Hunk_AllocateTempMemory(++size);

	Q_strncpyz(buffer, buffer_vp[0], size);

	for (i = 1; i < numVertexObjects; i++)
		strncat(buffer, buffer_vp[i], size);

	for (i = 0; i < numFragmentObjects; i++)
		strncat(buffer, buffer_fp[i], size);

	/* get uniform locations */
	qglUseProgramObjectARB(program->program);
	R_GLSL_ParseProgram(program, buffer);
	qglUseProgramObjectARB(0);

	/* clean up */
	ri.Hunk_FreeTempMemory(buffer);

	for (i = numFragmentObjects - 1; i >= 0; i--)
		ri.FS_FreeFile(buffer_fp[i]);

	for (i = numVertexObjects - 1; i >= 0; i--)
		ri.FS_FreeFile(buffer_vp[i]);

	return qtrue;
#endif
}

static qboolean R_GLSL_LoadProgramRaw(glslProgram_t *program, const char *name, const char *programVertexObjects, int numVertexObjects, const char *programFragmentObjects, int numFragmentObjects) {
#ifdef GLSL_BACKEND
	GLcharARB		*buffer_vp[MAX_PROGRAM_OBJECTS];
	GLcharARB		*buffer_fp[MAX_PROGRAM_OBJECTS];
	GLcharARB		*buffer;
	GLhandleARB		shader_vp;
	GLhandleARB		shader_fp;
	GLint			status;
	char			*str;
	int				size = 0;
	int				i;


	/* create program */
	program->program = qglCreateProgramObjectARB();

	/* vertex program */
	for (i = 0, str = (const char *)programVertexObjects; i < numVertexObjects; i++, str += MAX_QPATH) {
		buffer_vp[i] = str;
		size += sizeof(str)*32;
		if (!buffer_vp[i]) {
			ri.Printf( PRINT_WARNING,  "Couldn't load %s", str);
			return qfalse;
		}

		/* compile vertex shader */
		shader_vp = qglCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
		qglShaderSourceARB(shader_vp, 1, (const GLcharARB **)&buffer_vp, NULL);
		qglCompileShaderARB(shader_vp);

		/* check for errors in vertex shader */
		qglGetObjectParameterivARB(shader_vp, GL_OBJECT_COMPILE_STATUS_ARB, &status);
		if (!status) {
			int		length;
			char	*msg;

			/* print glsl error message */
			qglGetObjectParameterivARB(shader_vp, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
			msg = ri.Hunk_AllocateTempMemory(length);
			qglGetInfoLogARB(shader_vp, length, &length, msg);
			ri.Printf(PRINT_ALL, "Error:\n%s\n", msg);
			ri.Hunk_FreeTempMemory(msg);

			/* exit */
			ri.Printf( PRINT_WARNING,  "Couldn't compile vertex shader for program %s", name);
			return qfalse;
		}

		/* attach vertex shader to program */
		qglAttachObjectARB(program->program, shader_vp);
		qglDeleteObjectARB(shader_vp);
	}

	/* fragment program */
	for (i = 0, str = (const char *)programFragmentObjects; i < numFragmentObjects; i++, str += MAX_QPATH) {
		buffer_fp[i] = str;
		size += sizeof(str)*32;
		if (!buffer_fp[i]) {
			ri.Printf( PRINT_WARNING,  "Couldn't load %s", str);
			return qfalse;
		}

		/* compile fragment shader */
		shader_fp = qglCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
		qglShaderSourceARB(shader_fp, 1, (const GLcharARB **)&buffer_fp[i], NULL);
		qglCompileShaderARB(shader_fp);

		/* check for errors in fragment shader */
		qglGetObjectParameterivARB(shader_fp, GL_OBJECT_COMPILE_STATUS_ARB, &status);
		if (!status) {
			int		length;
			char	*msg;

			/* print glsl error message */

			qglGetObjectParameterivARB(shader_fp, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
			msg = ri.Hunk_AllocateTempMemory(length);
			qglGetInfoLogARB(shader_fp, length, &length, msg);
			ri.Printf(PRINT_ALL, "Error:\n%s\n", msg);
			ri.Hunk_FreeTempMemory(msg);
			ri.Printf(PRINT_DEVELOPER, "oops\n");
			/* exit */
			ri.Printf( PRINT_WARNING,  "Couldn't compile fragment shader for program %s", name);
			return qfalse;
		}

		/* attach fragment shader to program */
		qglAttachObjectARB(program->program, shader_fp);
		qglDeleteObjectARB(shader_fp);
	}

	/* link complete program */
	qglLinkProgramARB(program->program);
	/* check for linking errors */
	qglGetObjectParameterivARB(program->program, GL_OBJECT_LINK_STATUS_ARB, &status);
	if (!status) {
		int		length;
		char	*msg;
		/* print glsl error message */
		qglGetObjectParameterivARB(program->program, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
		msg = ri.Hunk_AllocateTempMemory(length);
		qglGetInfoLogARB(program->program, length, &length, msg);
		ri.Printf(PRINT_ALL, "Error:\n%s\n", msg);
		ri.Hunk_FreeTempMemory(msg);

		/* exit */
		ri.Printf( PRINT_WARNING,  "Couldn't link shaders for program %s", name);
		return qfalse;
	}
	/* build single large program file for parsing */
	buffer = ri.Hunk_AllocateTempMemory(++size);

	Q_strncpyz(buffer, buffer_vp[0], size);

	for (i = 1; i < numVertexObjects; i++)
		strncat(buffer, buffer_vp[i], size);
	for (i = 0; i < numFragmentObjects; i++)
		strncat(buffer, buffer_fp[i], size);
	/* get uniform locations */
	qglUseProgramObjectARB(program->program);
	R_GLSL_ParseProgram(program, buffer);
	qglUseProgramObjectARB(0);
	/* clean up */
	ri.Hunk_FreeTempMemory(buffer);

	return qtrue;
#endif
}



/*
 * RE_GLSL_RegisterProgram
 * Loads in a program of given name
 */
qhandle_t RE_GLSL_RegisterProgram(const char *name, const char *programVertexObjects, int numVertexObjects, const char *programFragmentObjects, int numFragmentObjects) {
#ifdef GLSL_BACKEND
	glslProgram_t	*program;
	qhandle_t		hProgram;

	if (!vertexShaders)
			return 0;

	if (!name || !name[0]) {
		ri.Printf(PRINT_ALL, "RE_GLSL_RegisterProgram: NULL name\n");
		return 0;
	}

	if (strlen(name) >= MAX_QPATH) {
		Com_Printf("Program name exceeds MAX_QPATH\n");
		return 0;
	}

	/* search the currently loaded programs */
	for (hProgram = 0; hProgram < tr.numPrograms; hProgram++) {
		program = tr.programs[hProgram];
		if (!strcmp(program->name, name)) {
			if (!program->valid)
				return 0;

			return hProgram;
		}
	}

	/* allocate a new glslProgram_t */
	if ((program = R_GLSL_AllocProgram()) == NULL) {
		ri.Printf(PRINT_WARNING, "RE_GLSL_RegisterProgram: R_GLSL_AllocProgram() failed for '%s'\n", name);
		return 0;
	}

	/* only set the name after the program has successfully loaded */
	Q_strncpyz(program->name, name, sizeof(program->name));

	R_IssuePendingRenderCommands();

	/* load the files */
	if (!R_GLSL_LoadProgram(program, name, programVertexObjects, numVertexObjects, programFragmentObjects, numFragmentObjects)) {
		qglDeleteObjectARB(program->program);
		program->valid = qfalse; 
		//vertexShaders=0;     //If program in error disable the glsl feature altogether
		return 0;
	}

	program->valid = qtrue;
	return program->index;
#endif
}


/*
 * RE_GLSL_RegisterProgram
 * Loads in a program of given name
 */
qhandle_t RE_GLSL_RegisterProgramRaw(const char *name, const char *programVertexObjects, int numVertexObjects, const char *programFragmentObjects, int numFragmentObjects) {
#ifdef GLSL_BACKEND
	glslProgram_t	*program;
	qhandle_t		hProgram;

	if (!vertexShaders)
			return 0;

	if (!name || !name[0]) {
		ri.Printf(PRINT_ALL, "RE_GLSL_RegisterProgram: NULL name\n");
		return 0;
	}

	if (strlen(name) >= MAX_QPATH) {
		Com_Printf("Program name exceeds MAX_QPATH\n");
		return 0;
	}

	/* search the currently loaded programs */
	for (hProgram = 0; hProgram < tr.numPrograms; hProgram++) {
		program = tr.programs[hProgram];
		if (!strcmp(program->name, name)) {
			if (!program->valid)
				return 0;

			return hProgram;
		}
	}

	/* allocate a new glslProgram_t */
	if ((program = R_GLSL_AllocProgram()) == NULL) {
		ri.Printf(PRINT_WARNING, "RE_GLSL_RegisterProgram: R_GLSL_AllocProgram() failed for '%s'\n", name);
		return 0;
	}

	/* only set the name after the program has successfully loaded */
	Q_strncpyz(program->name, name, sizeof(program->name));

	R_IssuePendingRenderCommands();

	/* load the files */
	if (!R_GLSL_LoadProgramRaw(program, name, (const char*)programVertexObjects, numVertexObjects, (const char*)programFragmentObjects, numFragmentObjects)) {
		qglDeleteObjectARB(program->program);
		program->valid = qfalse; 
		//vertexShaders=0;     //If program in error disable the glsl feature altogether
		return 0;
	}

	program->valid = qtrue;
	return program->index;
#endif
}



void R_RemapShader(const char *shaderName, const char *newShaderName, const char *timeOffset) {
	char		strippedName[MAX_QPATH];
	int			hash;
	shader_t	*sh, *sh2;
	qhandle_t	h;

	sh = R_FindShaderByName( shaderName );
	if (sh == NULL || sh == tr.defaultShader) {
		h = RE_RegisterShaderLightMap(shaderName, 0);
		
		sh = R_GetShaderByHandle(h);
	}
	if (sh == NULL || sh == tr.defaultShader) {
		ri.Printf( PRINT_WARNING, "WARNING: R_RemapShader: shader %s not found\n", shaderName );
		return;
	}

	sh2 = R_FindShaderByName( newShaderName );
	if (sh2 == NULL || sh2 == tr.defaultShader) {
		h = RE_RegisterShaderLightMap(newShaderName, 0);
		sh2 = R_GetShaderByHandle(h);
	}

	if (sh2 == NULL || sh2 == tr.defaultShader) {
		ri.Printf( PRINT_WARNING, "WARNING: R_RemapShader: new shader %s not found\n", newShaderName );
		return;
	}

	// remap all the shaders with the given name
	// even tho they might have different lightmaps
	COM_StripExtension(shaderName, strippedName, sizeof(strippedName));
	hash = generateHashValue(strippedName, FILE_HASH_SIZE);
	for (sh = hashTable[hash]; sh; sh = sh->next) {
		if (Q_stricmp(sh->name, strippedName) == 0) {
			if (sh != sh2) {
				sh->remappedShader = sh2;
			} else {
				sh->remappedShader = NULL;
			}
		}
	}
	if (timeOffset) {
		sh2->timeOffset = atof(timeOffset);
	}
}

/*
===============
ParseVector
===============
*/
static qboolean ParseVector( char **text, int count, float *v ) {
	char	*token;
	int		i;

	// FIXME: spaces are currently required after parens, should change parseext...
	token = COM_ParseExt( text, qfalse );
	if ( strcmp( token, "(" ) ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing parenthesis in shader '%s'\n", shader.name );
		return qfalse;
	}

	for ( i = 0 ; i < count ; i++ ) {
		token = COM_ParseExt( text, qfalse );
		if ( !token[0] ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing vector element in shader '%s'\n", shader.name );
			return qfalse;
		}
		v[i] = atof( token );
	}

	token = COM_ParseExt( text, qfalse );
	if ( strcmp( token, ")" ) ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing parenthesis in shader '%s'\n", shader.name );
		return qfalse;
	}

	return qtrue;
}


/*
===============
NameToAFunc
===============
*/
static unsigned NameToAFunc( const char *funcname )
{	
	if ( !Q_stricmp( funcname, "GT0" ) )
	{
		return GLS_ATEST_GT_0;
	}
	else if ( !Q_stricmp( funcname, "LT128" ) )
	{
		return GLS_ATEST_LT_80;
	}
	else if ( !Q_stricmp( funcname, "GE128" ) )
	{
		return GLS_ATEST_GE_80;
	}

	ri.Printf( PRINT_WARNING, "WARNING: invalid alphaFunc name '%s' in shader '%s'\n", funcname, shader.name );
	return 0;
}


/*
===============
NameToSrcBlendMode
===============
*/
static int NameToSrcBlendMode( const char *name )
{
	if ( !Q_stricmp( name, "GL_ONE" ) )
	{
		return GLS_SRCBLEND_ONE;
	}
	else if ( !Q_stricmp( name, "GL_ZERO" ) )
	{
		return GLS_SRCBLEND_ZERO;
	}
	else if ( !Q_stricmp( name, "GL_DST_COLOR" ) )
	{
		return GLS_SRCBLEND_DST_COLOR;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_COLOR" ) )
	{
		return GLS_SRCBLEND_ONE_MINUS_DST_COLOR;
	}
	else if ( !Q_stricmp( name, "GL_SRC_ALPHA" ) )
	{
		return GLS_SRCBLEND_SRC_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_ALPHA" ) )
	{
		return GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_DST_ALPHA" ) )
	{
		return GLS_SRCBLEND_DST_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_ALPHA" ) )
	{
		return GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_SRC_ALPHA_SATURATE" ) )
	{
		return GLS_SRCBLEND_ALPHA_SATURATE;
	}

	ri.Printf( PRINT_WARNING, "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name );
	return GLS_SRCBLEND_ONE;
}

/*
===============
NameToDstBlendMode
===============
*/
static int NameToDstBlendMode( const char *name )
{
	if ( !Q_stricmp( name, "GL_ONE" ) )
	{
		return GLS_DSTBLEND_ONE;
	}
	else if ( !Q_stricmp( name, "GL_ZERO" ) )
	{
		return GLS_DSTBLEND_ZERO;
	}
	else if ( !Q_stricmp( name, "GL_SRC_ALPHA" ) )
	{
		return GLS_DSTBLEND_SRC_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_ALPHA" ) )
	{
		return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_DST_ALPHA" ) )
	{
		return GLS_DSTBLEND_DST_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_ALPHA" ) )
	{
		return GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;
	}
	else if ( !Q_stricmp( name, "GL_SRC_COLOR" ) )
	{
		return GLS_DSTBLEND_SRC_COLOR;
	}
	else if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_COLOR" ) )
	{
		return GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
	}

	ri.Printf( PRINT_WARNING, "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name );
	return GLS_DSTBLEND_ONE;
}

/*
===============
NameToGenFunc
===============
*/
static genFunc_t NameToGenFunc( const char *funcname )
{
	if ( !Q_stricmp( funcname, "sin" ) )
	{
		return GF_SIN;
	}
	else if ( !Q_stricmp( funcname, "square" ) )
	{
		return GF_SQUARE;
	}
	else if ( !Q_stricmp( funcname, "triangle" ) )
	{
		return GF_TRIANGLE;
	}
	else if ( !Q_stricmp( funcname, "sawtooth" ) )
	{
		return GF_SAWTOOTH;
	}
	else if ( !Q_stricmp( funcname, "inversesawtooth" ) )
	{
		return GF_INVERSE_SAWTOOTH;
	}
	else if ( !Q_stricmp( funcname, "noise" ) )
	{
		return GF_NOISE;
	}

	ri.Printf( PRINT_WARNING, "WARNING: invalid genfunc name '%s' in shader '%s'\n", funcname, shader.name );
	return GF_SIN;
}


/*
===================
ParseWaveForm
===================
*/
static void ParseWaveForm( char **text, waveForm_t *wave )
{
	char *token;

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->func = NameToGenFunc( token );

	// BASE, AMP, PHASE, FREQ
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->base = atof( token );

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->amplitude = atof( token );

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->phase = atof( token );

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->frequency = atof( token );
}


/*
===================
ParseTexMod
===================
*/
static void ParseTexMod( char *_text, shaderStage_t *stage )
{
	const char *token;
	char **text = &_text;
	texModInfo_t *tmi;

	if ( stage->bundle[0].numTexMods == TR_MAX_TEXMODS ) {
		ri.Error( ERR_DROP, "ERROR: too many tcMod stages in shader '%s'", shader.name );
		return;
	}

	tmi = &stage->bundle[0].texMods[stage->bundle[0].numTexMods];
	stage->bundle[0].numTexMods++;

	token = COM_ParseExt( text, qfalse );

	//
	// turb
	//
	if ( !Q_stricmp( token, "turb" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod turb parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.base = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.amplitude = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.phase = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.frequency = atof( token );

		tmi->type = TMOD_TURBULENT;
	}
	//
	// scale
	//
	else if ( !Q_stricmp( token, "scale" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing scale parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scale[0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing scale parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scale[1] = atof( token );
		tmi->type = TMOD_SCALE;
	}
	//
	// scroll
	//
	else if ( !Q_stricmp( token, "scroll" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing scale scroll parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scroll[0] = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing scale scroll parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scroll[1] = atof( token );
		tmi->type = TMOD_SCROLL;
	}
	//
	// stretch
	//
	else if ( !Q_stricmp( token, "stretch" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.func = NameToGenFunc( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.base = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.amplitude = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.phase = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.frequency = atof( token );
		
		tmi->type = TMOD_STRETCH;
	}
//
	// leilei - atlas
	//
	else if ( !Q_stricmp( token, "atlas" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing atlas parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->atlas.mode = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing atlas parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->atlas.frame = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing atlas parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->atlas.fps = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing atlas parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->atlas.width = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing atlas parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->atlas.height = atof( token );
		
		tmi->type = TMOD_ATLAS;
	}

	//
	// transform
	//
	else if ( !Q_stricmp( token, "transform" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[0][0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[0][1] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[1][0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[1][1] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->translate[0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->translate[1] = atof( token );

		tmi->type = TMOD_TRANSFORM;
	}
	//
	// rotate
	//
	else if ( !Q_stricmp( token, "rotate" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod rotate parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->rotateSpeed = atof( token );
		tmi->type = TMOD_ROTATE;
	}
	//
	// entityTranslate
	//
	else if ( !Q_stricmp( token, "entityTranslate" ) )
	{
		tmi->type = TMOD_ENTITY_TRANSLATE;
	}
	else
	{
		ri.Printf( PRINT_WARNING, "WARNING: unknown tcMod '%s' in shader '%s'\n", token, shader.name );
	}
}



/*
==============
ParseNum / ParseHex
==============
*/
int ShParseHex (const char *hex)
{
	const char    *str;
	int    num;

	num = 0;
	str = hex;

	while (*str)
	{
		num <<= 4;
		if (*str >= '0' && *str <= '9')
			num += *str-'0';
		else if (*str >= 'a' && *str <= 'f')
			num += 10 + *str-'a';
		else if (*str >= 'A' && *str <= 'F')
			num += 10 + *str-'A';
		else
			return 0;
		str++;
	}

	return num;
}


static int ShParseNum (const char *str)
{
	if (str[0] == '$')
		return ShParseHex (str+1);
	if (str[0] == '0' && str[1] == 'x')
		return ShParseHex (str+2);
	return atol (str);
}



/*
===================
ParseStage
===================
*/



static qboolean ParseStage( shaderStage_t *stage, char **text )
{
	char *token;
	char programName[MAX_QPATH];
	char programVertexObjects[MAX_PROGRAM_OBJECTS][MAX_QPATH];
	char programFragmentObjects[MAX_PROGRAM_OBJECTS][MAX_QPATH];
	int numVertexObjects = 0;
	int numFragmentObjects = 0;
	int depthMaskBits = GLS_DEPTHMASK_TRUE, blendSrcBits = 0, blendDstBits = 0, atestBits = 0, depthFuncBits = 0;
	qboolean depthMaskExplicit = qfalse;

	qboolean stageMipmaps = !shader.noMipMaps;

	stage->active = qtrue;
	Com_Memset(programName, 0, sizeof(programName));

	while ( 1 )
	{
	
	

		//stage->isBlend = 0;
		token = COM_ParseExt( text, qtrue );
		if ( !token[0] )
		{
			ri.Printf( PRINT_WARNING, "WARNING: no matching '}' found\n" );
			return qfalse;
		}

		if ( token[0] == '}' )
		{
			if (programName[0]) {
				if (!Q_stricmp(programName, "skip")) {
					stage->program = tr.skipProgram;
				} else {
					if (!numVertexObjects) {
						ri.Printf(PRINT_WARNING, "WARNING: no 'vertexProgram' specified for 'program %s' in shader '%s'\n", programName, shader.name);
						return qfalse;
					}

					if (!numFragmentObjects) {
						ri.Printf(PRINT_WARNING, "WARNING: no 'fragmentProgram' specified for 'program %s' in shader '%s'\n", programName, shader.name);
						return qfalse;
					}
#ifdef GLSL_TEXTURES
					stage->isGLSL=0;
					stage->program = RE_GLSL_RegisterProgram(programName, (const char *)programVertexObjects, numVertexObjects, (const char *)programFragmentObjects, numFragmentObjects);
					if (!stage->program) {
						ri.Printf(PRINT_WARNING, "WARNING: RE_GLSL_RegisterProgram failed for 'program %s' in shader '%s'\n", programName, shader.name);
						//return qfalse;
					}
					else
						stage->isGLSL=1;
#endif
				}
			} else if (numVertexObjects) {
				ri.Printf(PRINT_WARNING, "WARNING: no 'program' specified for 'vertexProgram' in shader '%s'\n", shader.name);
				return qfalse;
			} else if (numFragmentObjects) {
				ri.Printf(PRINT_WARNING, "WARNING: no 'program' specified for 'fragmentProgram' in shader '%s'\n", shader.name);
				return qfalse;
			}
		
	break;
	}
		//
		// program <name>
		//
		else if (!Q_stricmp(token, "program")) {
			token = COM_ParseExt(text, qfalse);

			if (!vertexShaders)
				continue;

			if (!token[0]) {
				ri.Printf(PRINT_WARNING, "WARNING: missing parameter for 'program' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			Q_strncpyz(programName, token, sizeof(programName));
		}
		//
		// vertexProgram <path1> .... <pathN>
		//
		else if (!Q_stricmp(token, "vertexProgram")) {
			token = COM_ParseExt(text, qfalse);

			if (!vertexShaders) {
				while (token[0])
					token = COM_ParseExt(text, qfalse);

				continue;
			}

			if (!token[0]) {
				ri.Printf(PRINT_WARNING, "WARNING: missing parameter(s) for 'vertexProgram' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}
#ifdef GLSL_TEXTURES
			// parse up to MAX_PROGRAM_OBJECTS files
			for(;;) {
				if (numVertexObjects < MAX_PROGRAM_OBJECTS) {
					Q_strncpyz(programVertexObjects[numVertexObjects], token, sizeof(programVertexObjects[numVertexObjects]));
					numVertexObjects++;
				} else {
					ri.Printf(PRINT_WARNING, "WARNING: Too many parameters for 'vertexProgram' keyword in shader '%s'\n", shader.name);
					return qfalse;
				}

				token = COM_ParseExt(text, qfalse);
				if (!token[0])
					break;
			}
#else
		// NO!
#endif
		}
		//
		// fragmentProgram <path1> .... <pathN>
		//
		else if (!Q_stricmp(token, "fragmentProgram")) {
			token = COM_ParseExt(text, qfalse);

			if (!vertexShaders) {
				while (token[0])
					token = COM_ParseExt(text, qfalse);

				continue;
			}

			if (!token[0]) {
				ri.Printf(PRINT_WARNING, "WARNING: missing parameter(s) for 'fragmentProgram' keyword in shader '%s'\n", shader.name);
				return qfalse;
			}

			// parse up to MAX_PROGRAM_OBJECTS files
			for(;;) {
				if (numFragmentObjects < MAX_PROGRAM_OBJECTS) {
					Q_strncpyz(programFragmentObjects[numFragmentObjects], token, sizeof(programFragmentObjects[numFragmentObjects]));
					numFragmentObjects++;
				} else {
					ri.Printf(PRINT_WARNING, "WARNING: Too many parameters for 'fragmentProgram' keyword in shader '%s'\n", shader.name);
					return qfalse;
				}

				token = COM_ParseExt(text, qfalse);
				if (!token[0])
					break;
			}
		}
		else if ( !Q_stricmp( token, "mipOffset" ) ){
			token = COM_ParseExt(text,qfalse);
			stage->mipBias = atoi(token);
		}
		else if ( !Q_stricmp( token, "nomipmaps" ) ){
			stageMipmaps = qfalse;
		}
		//
		// map <name>
		//
		else if ( !Q_stricmp( token, "map" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'map' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if ( !Q_stricmp( token, "$whiteimage" ) )
			{
				stage->bundle[0].image[0] = tr.whiteImage;
				continue;
			}
			else if ( !Q_stricmp( token, "$lightmap" ) )
			{
				stage->bundle[0].isLightmap = qtrue;
				if ( shader.lightmapIndex < 0 || !tr.lightmaps ) {
					stage->bundle[0].image[0] = tr.whiteImage;
				} else {
					stage->bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
				}
				continue;
			}
			else
			{
				imgType_t type = IMGTYPE_COLORALPHA;
				imgFlags_t flags = IMGFLAG_NONE;

				if (!shader.noMipMaps)
					flags |= IMGFLAG_MIPMAP;

				if (!shader.noPicMip)
					flags |= IMGFLAG_PICMIP;

				stage->bundle[0].image[0] = R_FindImageFile( token, type, flags );

				if ( !stage->bundle[0].image[0] )
				{
					ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
					return qfalse;
				}
			}
		}
		//
		// map2 <name>
		//
		else if ( !Q_stricmp( token, "map2" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'map2' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if ( !Q_stricmp( token, "$whiteimage" ) )
			{
				stage->bundle[2].image[0] = tr.whiteImage;
				continue;
			}
			
			else if ( !Q_stricmp( token, "$lightmap" ) )
			{
				stage->bundle[2].isLightmap = qtrue;
				if ( shader.lightmapIndex < 0 ) {
					stage->bundle[2].image[0] = tr.whiteImage;
				} else {
					stage->bundle[2].image[0] = tr.lightmaps[shader.lightmapIndex];
				}
				continue;
			}
			else
			{
				imgType_t type = IMGTYPE_COLORALPHA;
				imgFlags_t flags = IMGFLAG_NONE;

				if (stageMipmaps)
					flags |= IMGFLAG_MIPMAP;

				if (!shader.noPicMip)
					flags |= IMGFLAG_PICMIP;

				stage->bundle[2].image[0] = R_FindImageFile( token, type, flags );

				if ( !stage->bundle[2].image[0] )
				{
					ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
					return qfalse;
				}
			}
		}
		//
		// map3 <name>
		//
		else if ( !Q_stricmp( token, "map3" ) || (!Q_stricmp( token, "normalmap" )))
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'map3' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if ( !Q_stricmp( token, "$whiteimage" ) )
			{
				stage->bundle[3].image[0] = tr.whiteImage;
				continue;
			}

			else if ( !Q_stricmp( token, "$lightmap" ) )
			{
				stage->bundle[3].isLightmap = qtrue;
				if ( shader.lightmapIndex < 0 ) {
					stage->bundle[3].image[0] = tr.whiteImage;
				} else {
					stage->bundle[3].image[0] = tr.lightmaps[shader.lightmapIndex];
				}
				continue;
			}
			else
			{
				imgType_t type = IMGTYPE_COLORALPHA;
				imgFlags_t flags = IMGFLAG_NONE;

				if (stageMipmaps)
					flags |= IMGFLAG_MIPMAP;

				if (!shader.noPicMip)
					flags |= IMGFLAG_PICMIP;

				stage->bundle[3].image[0] = R_FindImageFile( token, type, flags );

				if ( !stage->bundle[3].image[0] )
				{
					ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
					return qfalse;
				}
			}
		}
		//
		// map4 <name>
		//
		else if ( !Q_stricmp( token, "map4" )  || (!Q_stricmp( token, "specmap" )))
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'map4' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if ( !Q_stricmp( token, "$whiteimage" ) )
			{
				stage->bundle[4].image[0] = tr.whiteImage;
				continue;
			}
			else if ( !Q_stricmp( token, "$lightmap" ) )
			{
				stage->bundle[4].isLightmap = qtrue;
				if ( shader.lightmapIndex < 0 ) {
					stage->bundle[4].image[0] = tr.whiteImage;
				} else {
					stage->bundle[4].image[0] = tr.lightmaps[shader.lightmapIndex];
				}
				continue;
			}
			else
			{
				imgType_t type = IMGTYPE_COLORALPHA;
				imgFlags_t flags = IMGFLAG_NONE;

				if (stageMipmaps)
					flags |= IMGFLAG_MIPMAP;

				if (!shader.noPicMip)
					flags |= IMGFLAG_PICMIP;

				stage->bundle[4].image[0] = R_FindImageFile( token, type, flags );

				if ( !stage->bundle[4].image[0] )
				{
					ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
					return qfalse;
				}
			}
		}
		//
		// map5 <name>
		//
		else if ( !Q_stricmp( token, "map5" )  || (!Q_stricmp( token, "shadeballmap" )))
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'map5' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if ( !Q_stricmp( token, "$whiteimage" ) )
			{
				stage->bundle[5].image[0] = tr.whiteImage;
				continue;
			}

			else if ( !Q_stricmp( token, "$lightmap" ) )
			{
				stage->bundle[5].isLightmap = qtrue;
				if ( shader.lightmapIndex < 0 ) {
					stage->bundle[5].image[0] = tr.whiteImage;
				} else {
					stage->bundle[5].image[0] = tr.lightmaps[shader.lightmapIndex];
				}
				continue;
			}
			else
			{
				imgType_t type = IMGTYPE_COLORALPHA;
				imgFlags_t flags = IMGFLAG_NONE;

				if (stageMipmaps)
					flags |= IMGFLAG_MIPMAP;

				if (!shader.noPicMip)
					flags |= IMGFLAG_PICMIP;

				stage->bundle[5].image[0] = R_FindImageFile( token, type, flags );

				if ( !stage->bundle[5].image[0] )
				{
					ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
					return qfalse;
				}
			}
		}
		//
		// map6 <name>
		//
		else if ( !Q_stricmp( token, "map6" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'map6' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if ( !Q_stricmp( token, "$whiteimage" ) )
			{
				stage->bundle[6].image[0] = tr.whiteImage;
				continue;
			}
			
			else if ( !Q_stricmp( token, "$lightmap" ) )
			{
				stage->bundle[6].isLightmap = qtrue;
				if ( shader.lightmapIndex < 0 ) {
					stage->bundle[6].image[0] = tr.whiteImage;
				} else {
					stage->bundle[6].image[0] = tr.lightmaps[shader.lightmapIndex];
				}
				continue;
			}
			else
			{
				imgType_t type = IMGTYPE_COLORALPHA;
				imgFlags_t flags = IMGFLAG_NONE;

				if (stageMipmaps)
					flags |= IMGFLAG_MIPMAP;

				if (!shader.noPicMip)
					flags |= IMGFLAG_PICMIP;

				stage->bundle[6].image[0] = R_FindImageFile( token, type, flags );

				if ( !stage->bundle[6].image[0] )
				{
					ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
					return qfalse;
				}
			}
		}
		//
		// map7 <name>
		//
		else if ( !Q_stricmp( token, "map7" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'map7' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if ( !Q_stricmp( token, "$whiteimage" ) )
			{
				stage->bundle[7].image[0] = tr.whiteImage;
				continue;
			}
			
			else if ( !Q_stricmp( token, "$lightmap" ) )
			{
				stage->bundle[7].isLightmap = qtrue;
				if ( shader.lightmapIndex < 0 ) {
					stage->bundle[7].image[0] = tr.whiteImage;
				} else {
					stage->bundle[7].image[0] = tr.lightmaps[shader.lightmapIndex];
				}
				continue;
			}
			else
			{
				imgType_t type = IMGTYPE_COLORALPHA;
				imgFlags_t flags = IMGFLAG_NONE;

				if (stageMipmaps)
					flags |= IMGFLAG_MIPMAP;

				if (!shader.noPicMip)
					flags |= IMGFLAG_PICMIP;

				stage->bundle[7].image[0] = R_FindImageFile( token, type, flags );

				if ( !stage->bundle[7].image[0] )
				{
					ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
					return qfalse;
				}
			}
		}
		//
		// clampmap <name>
		//
		else if ( !Q_stricmp( token, "clampmap" ) )
		{
			imgType_t type = IMGTYPE_COLORALPHA;
			imgFlags_t flags = IMGFLAG_CLAMPTOEDGE;

			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'clampmap' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if (!shader.noMipMaps)
				flags |= IMGFLAG_MIPMAP;

			if (!shader.noPicMip)
				flags |= IMGFLAG_PICMIP;

			stage->bundle[0].image[0] = R_FindImageFile( token, type, flags );
			if ( !stage->bundle[0].image[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
				return qfalse;
			}
		}
		//
		// clampmap2 <name>
		//
		else if ( !Q_stricmp( token, "clampmap2" ) )
		{
			imgType_t type = IMGTYPE_COLORALPHA;
			imgFlags_t flags = IMGFLAG_CLAMPTOEDGE;

			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'clampmap2' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if (stageMipmaps)
				flags |= IMGFLAG_MIPMAP;

			if (!shader.noPicMip)
				flags |= IMGFLAG_PICMIP;

			stage->bundle[2].image[0] = R_FindImageFile( token, type, flags );

			if ( !stage->bundle[2].image[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
				return qfalse;
			}
		}
		//
		// clampmap3 <name>
		//
		else if ( !Q_stricmp( token, "clampmap3" ) )
		{
			imgType_t type = IMGTYPE_COLORALPHA;
			imgFlags_t flags = IMGFLAG_CLAMPTOEDGE;

			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'clampmap3' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if (stageMipmaps)
				flags |= IMGFLAG_MIPMAP;

			if (!shader.noPicMip)
				flags |= IMGFLAG_PICMIP;

			stage->bundle[3].image[0] = R_FindImageFile( token, type, flags );

			if ( !stage->bundle[3].image[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
				return qfalse;
			}
		}
		//
		// clampmap4 <name>
		//
		else if ( !Q_stricmp( token, "clampmap4" ) )
		{
			imgType_t type = IMGTYPE_COLORALPHA;
			imgFlags_t flags = IMGFLAG_CLAMPTOEDGE;

			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'clampmap4' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if (stageMipmaps)
				flags |= IMGFLAG_MIPMAP;

			if (!shader.noPicMip)
				flags |= IMGFLAG_PICMIP;

			stage->bundle[4].image[0] = R_FindImageFile( token, type, flags );

			if ( !stage->bundle[4].image[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
				return qfalse;
			}
		}
		//
		// clampmap5 <name>
		//
		else if ( !Q_stricmp( token, "clampmap5" ) )
		{
			imgType_t type = IMGTYPE_COLORALPHA;
			imgFlags_t flags = IMGFLAG_CLAMPTOEDGE;

			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'clampmap5' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if (stageMipmaps)
				flags |= IMGFLAG_MIPMAP;

			if (!shader.noPicMip)
				flags |= IMGFLAG_PICMIP;

			stage->bundle[5].image[0] = R_FindImageFile( token, type, flags );

			if ( !stage->bundle[5].image[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
				return qfalse;
			}
		}
		//
		// clampmap6 <name>
		//
		else if ( !Q_stricmp( token, "clampmap6" ) )
		{
			imgType_t type = IMGTYPE_COLORALPHA;
			imgFlags_t flags = IMGFLAG_CLAMPTOEDGE;

			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'clampmap6' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if (stageMipmaps)
				flags |= IMGFLAG_MIPMAP;

			if (!shader.noPicMip)
				flags |= IMGFLAG_PICMIP;

			stage->bundle[6].image[0] = R_FindImageFile( token, type, flags );

			if ( !stage->bundle[6].image[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
				return qfalse;
			}
		}
		//
		// clampmap7 <name>
		//
		else if ( !Q_stricmp( token, "clampmap7" ) )
		{
			imgType_t type = IMGTYPE_COLORALPHA;
			imgFlags_t flags = IMGFLAG_CLAMPTOEDGE;

			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'clampmap7' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if (stageMipmaps)
				flags |= IMGFLAG_MIPMAP;

			if (!shader.noPicMip)
				flags |= IMGFLAG_PICMIP;

			stage->bundle[7].image[0] = R_FindImageFile( token, type, flags );

			if ( !stage->bundle[7].image[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
				return qfalse;
			}
		}
		//
		// animMap <frequency> <image1> .... <imageN>
		//
		else if ( !Q_stricmp( token, "animMap" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'animMmap' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[0].imageAnimationSpeed = atof( token );

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 ) {
				int		num;

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					break;
				}
				num = stage->bundle[0].numImageAnimations;
				if ( num < MAX_IMAGE_ANIMATIONS ) {
					imgFlags_t flags = IMGFLAG_NONE;

					if (!shader.noMipMaps)
						flags |= IMGFLAG_MIPMAP;

					if (!shader.noPicMip)
						flags |= IMGFLAG_PICMIP;

					stage->bundle[0].image[num] = R_FindImageFile( token, IMGTYPE_COLORALPHA, flags );
					if ( !stage->bundle[0].image[num] )
					{
						ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
						return qfalse;
					}
					stage->bundle[0].numImageAnimations++;
				}
			}
		}
		//
		// animMap2 <frequency> <image1> .... <imageN>
		//
		else if ( !Q_stricmp( token, "animMap2" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'animMap2' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[2].imageAnimationSpeed = atof( token );

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 ) {
				int		num;

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					break;
				}
				num = stage->bundle[2].numImageAnimations;
				if ( num < MAX_IMAGE_ANIMATIONS ) {
					imgFlags_t flags = IMGFLAG_NONE;

					if (stageMipmaps)
						flags |= IMGFLAG_MIPMAP;

					if (!shader.noPicMip)
						flags |= IMGFLAG_PICMIP;

					stage->bundle[2].image[num] = R_FindImageFile( token, IMGTYPE_COLORALPHA, flags );

					if ( !stage->bundle[2].image[num] )
					{
						ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
						return qfalse;
					}
					stage->bundle[2].numImageAnimations++;
				}
			}
		}
		//
		// animMap3 <frequency> <image1> .... <imageN>
		//
		else if ( !Q_stricmp( token, "animMap3" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'animMap3' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[3].imageAnimationSpeed = atof( token );

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 ) {
				int		num;

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					break;
				}
				num = stage->bundle[3].numImageAnimations;
				if ( num < MAX_IMAGE_ANIMATIONS ) {
					imgFlags_t flags = IMGFLAG_NONE;

					if (stageMipmaps)
						flags |= IMGFLAG_MIPMAP;

					if (!shader.noPicMip)
						flags |= IMGFLAG_PICMIP;

					stage->bundle[3].image[num] = R_FindImageFile( token, IMGTYPE_COLORALPHA, flags );

					if ( !stage->bundle[3].image[num] )
					{
						ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
						return qfalse;
					}
					stage->bundle[3].numImageAnimations++;
				}
			}
		}
		//
		// animMap4 <frequency> <image1> .... <imageN>
		//
		else if ( !Q_stricmp( token, "animMap4" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'animMap4' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[4].imageAnimationSpeed = atof( token );

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 ) {
				int		num;

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					break;
				}
				num = stage->bundle[4].numImageAnimations;
				if ( num < MAX_IMAGE_ANIMATIONS ) {
					imgFlags_t flags = IMGFLAG_NONE;

					if (stageMipmaps)
						flags |= IMGFLAG_MIPMAP;

					if (!shader.noPicMip)
						flags |= IMGFLAG_PICMIP;

					stage->bundle[4].image[num] = R_FindImageFile( token, IMGTYPE_COLORALPHA, flags );

					if ( !stage->bundle[4].image[num] )
					{
						ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
						return qfalse;
					}
					stage->bundle[4].numImageAnimations++;
				}
			}
		}
		//
		// animMap5 <frequency> <image1> .... <imageN>
		//
		else if ( !Q_stricmp( token, "animMap5" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'animMap5' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[5].imageAnimationSpeed = atof( token );

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 ) {
				int		num;

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					break;
				}
				num = stage->bundle[5].numImageAnimations;
				if ( num < MAX_IMAGE_ANIMATIONS ) {
					imgFlags_t flags = IMGFLAG_NONE;

					if (stageMipmaps)
						flags |= IMGFLAG_MIPMAP;

					if (!shader.noPicMip)
						flags |= IMGFLAG_PICMIP;

					stage->bundle[5].image[num] = R_FindImageFile( token, IMGTYPE_COLORALPHA, flags );

					if ( !stage->bundle[5].image[num] )
					{
						ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
						return qfalse;
					}
					stage->bundle[5].numImageAnimations++;
				}
			}
		}
		//
		// animMap6 <frequency> <image1> .... <imageN>
		//
		else if ( !Q_stricmp( token, "animMap6" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'animMap6' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[6].imageAnimationSpeed = atof( token );

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 ) {
				int		num;

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					break;
				}
				num = stage->bundle[6].numImageAnimations;
				if ( num < MAX_IMAGE_ANIMATIONS ) {
					imgFlags_t flags = IMGFLAG_NONE;

					if (stageMipmaps)
						flags |= IMGFLAG_MIPMAP;

					if (!shader.noPicMip)
						flags |= IMGFLAG_PICMIP;

					stage->bundle[6].image[num] = R_FindImageFile( token, IMGTYPE_COLORALPHA, flags );

					if ( !stage->bundle[6].image[num] )
					{
						ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
						return qfalse;
					}
					stage->bundle[6].numImageAnimations++;
				}
			}
		}
		//
		// animMap7 <frequency> <image1> .... <imageN>
		//
		else if ( !Q_stricmp( token, "animMap7" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'animMap7' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[7].imageAnimationSpeed = atof( token );

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 ) {
				int		num;

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					break;
				}
				num = stage->bundle[7].numImageAnimations;
				if ( num < MAX_IMAGE_ANIMATIONS ) {
					imgFlags_t flags = IMGFLAG_NONE;

					if (stageMipmaps)
						flags |= IMGFLAG_MIPMAP;

					if (!shader.noPicMip)
						flags |= IMGFLAG_PICMIP;

					stage->bundle[7].image[num] = R_FindImageFile( token, IMGTYPE_COLORALPHA, flags );

					if ( !stage->bundle[7].image[num] )
					{
						ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
						return qfalse;
					}
					stage->bundle[7].numImageAnimations++;
				}
			}
		}
		else if ( !Q_stricmp( token, "clampAnimMap" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'clampAnimMmap' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[0].imageAnimationSpeed = atof( token );

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 ) {
				int		num;

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					break;
				}
				num = stage->bundle[0].numImageAnimations;
				if ( num < MAX_IMAGE_ANIMATIONS ) {
					imgFlags_t flags = IMGFLAG_SRGB;

					if (stageMipmaps)
						flags |= IMGFLAG_MIPMAP;

					if (!shader.noPicMip)
						flags |= IMGFLAG_PICMIP;

					stage->bundle[0].image[num] = R_FindImageFile( token, IMGTYPE_COLORALPHA, flags );

					if ( !stage->bundle[0].image[num] )
					{
						ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
						return qfalse;
					}
					stage->bundle[0].numImageAnimations++;
				}
			}
		}
		else if ( !Q_stricmp( token, "clampAnimMap2" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'clampAnimMmap2' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[2].imageAnimationSpeed = atof( token );

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 ) {
				int		num;

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					break;
				}
				num = stage->bundle[2].numImageAnimations;
				if ( num < MAX_IMAGE_ANIMATIONS ) {
					imgFlags_t flags = IMGFLAG_SRGB;

					if (stageMipmaps)
						flags |= IMGFLAG_MIPMAP;

					if (!shader.noPicMip)
						flags |= IMGFLAG_PICMIP;

					stage->bundle[2].image[num] = R_FindImageFile( token, IMGTYPE_COLORALPHA, flags );

					if ( !stage->bundle[2].image[num] )
					{
						ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
						return qfalse;
					}
					stage->bundle[2].numImageAnimations++;
				}
			}
		}
		else if ( !Q_stricmp( token, "clampAnimMap3" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'clampAnimMmap3' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[3].imageAnimationSpeed = atof( token );

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 ) {
				int		num;

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					break;
				}
				num = stage->bundle[3].numImageAnimations;
				if ( num < MAX_IMAGE_ANIMATIONS ) {
					imgFlags_t flags = IMGFLAG_SRGB;

					if (stageMipmaps)
						flags |= IMGFLAG_MIPMAP;

					if (!shader.noPicMip)
						flags |= IMGFLAG_PICMIP;

					stage->bundle[3].image[num] = R_FindImageFile( token, IMGTYPE_COLORALPHA, flags );

					if ( !stage->bundle[3].image[num] )
					{
						ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
						return qfalse;
					}
					stage->bundle[3].numImageAnimations++;
				}
			}
		}
				else if ( !Q_stricmp( token, "clampAnimMap4" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'clampAnimMmap4' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[4].imageAnimationSpeed = atof( token );

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 ) {
				int		num;

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					break;
				}
				num = stage->bundle[4].numImageAnimations;
				if ( num < MAX_IMAGE_ANIMATIONS ) {
					imgFlags_t flags = IMGFLAG_SRGB;

					if (stageMipmaps)
						flags |= IMGFLAG_MIPMAP;

					if (!shader.noPicMip)
						flags |= IMGFLAG_PICMIP;

					stage->bundle[4].image[num] = R_FindImageFile( token, IMGTYPE_COLORALPHA, flags );

					if ( !stage->bundle[4].image[num] )
					{
						ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
						return qfalse;
					}
					stage->bundle[4].numImageAnimations++;
				}
			}
		}
				else if ( !Q_stricmp( token, "clampAnimMap5" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'clampAnimMmap5' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[5].imageAnimationSpeed = atof( token );

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 ) {
				int		num;

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					break;
				}
				num = stage->bundle[5].numImageAnimations;
				if ( num < MAX_IMAGE_ANIMATIONS ) {
					imgFlags_t flags = IMGFLAG_SRGB;

					if (stageMipmaps)
						flags |= IMGFLAG_MIPMAP;

					if (!shader.noPicMip)
						flags |= IMGFLAG_PICMIP;

					stage->bundle[5].image[num] = R_FindImageFile( token, IMGTYPE_COLORALPHA, flags );

					if ( !stage->bundle[5].image[num] )
					{
						ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
						return qfalse;
					}
					stage->bundle[5].numImageAnimations++;
				}
			}
		}
				else if ( !Q_stricmp( token, "clampAnimMap6" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'clampAnimMmap6' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[6].imageAnimationSpeed = atof( token );

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 ) {
				int		num;

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					break;
				}
				num = stage->bundle[6].numImageAnimations;
				if ( num < MAX_IMAGE_ANIMATIONS ) {
					imgFlags_t flags = IMGFLAG_SRGB;

					if (stageMipmaps)
						flags |= IMGFLAG_MIPMAP;

					if (!shader.noPicMip)
						flags |= IMGFLAG_PICMIP;

					stage->bundle[6].image[num] = R_FindImageFile( token, IMGTYPE_COLORALPHA, flags );

					if ( !stage->bundle[6].image[num] )
					{
						ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
						return qfalse;
					}
					stage->bundle[6].numImageAnimations++;
				}
			}
		}
				else if ( !Q_stricmp( token, "clampAnimMap7" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'clampAnimMmap7' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[7].imageAnimationSpeed = atof( token );

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 ) {
				int		num;

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					break;
				}
				num = stage->bundle[7].numImageAnimations;
				if ( num < MAX_IMAGE_ANIMATIONS ) {
					imgFlags_t flags = IMGFLAG_SRGB;

					if (stageMipmaps)
						flags |= IMGFLAG_MIPMAP;

					if (!shader.noPicMip)
						flags |= IMGFLAG_PICMIP;

					stage->bundle[7].image[num] = R_FindImageFile( token, IMGTYPE_COLORALPHA, flags );

					if ( !stage->bundle[7].image[num] )
					{
						ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
						return qfalse;
					}
					stage->bundle[7].numImageAnimations++;
				}
			}
		}
		else if ( !Q_stricmp( token, "videoMap" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'videoMmap' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[0].videoMapHandle = ri.CIN_PlayCinematic( token, 0, 0, 256, 256, (CIN_loop | CIN_silent | CIN_shader));
			if (stage->bundle[0].videoMapHandle != -1) {
				stage->bundle[0].isVideoMap = qtrue;
				stage->bundle[0].image[0] = tr.scratchImage[stage->bundle[0].videoMapHandle];
			}
		}
		//
		// videoMap2 <name>
		//
		else if ( !Q_stricmp( token, "videoMap2" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'videoMap2' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[2].videoMapHandle = ri.CIN_PlayCinematic( token, 0, 0, 256, 256, (CIN_loop | CIN_silent | CIN_shader));
			if (stage->bundle[2].videoMapHandle != -1) {
				stage->bundle[2].isVideoMap = qtrue;
				stage->bundle[2].image[0] = tr.scratchImage[stage->bundle[2].videoMapHandle];
			}
		}
		//
		// videoMap3 <name>
		//
		else if ( !Q_stricmp( token, "videoMap3" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'videoMap3' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[3].videoMapHandle = ri.CIN_PlayCinematic( token, 0, 0, 256, 256, (CIN_loop | CIN_silent | CIN_shader));
			if (stage->bundle[3].videoMapHandle != -1) {
				stage->bundle[3].isVideoMap = qtrue;
				stage->bundle[3].image[0] = tr.scratchImage[stage->bundle[3].videoMapHandle];
			}
		}
		//
		// videoMap4 <name>
		//
		else if ( !Q_stricmp( token, "videoMap4" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'videoMap4' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[4].videoMapHandle = ri.CIN_PlayCinematic( token, 0, 0, 256, 256, (CIN_loop | CIN_silent | CIN_shader));
			if (stage->bundle[4].videoMapHandle != -1) {
				stage->bundle[4].isVideoMap = qtrue;
				stage->bundle[4].image[0] = tr.scratchImage[stage->bundle[4].videoMapHandle];
			}
		}
		//
		// videoMap5 <name>
		//
		else if ( !Q_stricmp( token, "videoMap5" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'videoMap5' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[5].videoMapHandle = ri.CIN_PlayCinematic( token, 0, 0, 256, 256, (CIN_loop | CIN_silent | CIN_shader));
			if (stage->bundle[5].videoMapHandle != -1) {
				stage->bundle[5].isVideoMap = qtrue;
				stage->bundle[5].image[0] = tr.scratchImage[stage->bundle[5].videoMapHandle];
			}
		}
		//
		// videoMap6 <name>
		//
		else if ( !Q_stricmp( token, "videoMap6" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'videoMap6' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[6].videoMapHandle = ri.CIN_PlayCinematic( token, 0, 0, 256, 256, (CIN_loop | CIN_silent | CIN_shader));
			if (stage->bundle[6].videoMapHandle != -1) {
				stage->bundle[6].isVideoMap = qtrue;
				stage->bundle[6].image[0] = tr.scratchImage[stage->bundle[6].videoMapHandle];
			}
		}
		//
		// videoMap7 <name>
		//
		else if ( !Q_stricmp( token, "videoMap7" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'videoMap7' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[7].videoMapHandle = ri.CIN_PlayCinematic( token, 0, 0, 256, 256, (CIN_loop | CIN_silent | CIN_shader));
			if (stage->bundle[7].videoMapHandle != -1) {
				stage->bundle[7].isVideoMap = qtrue;
				stage->bundle[7].image[0] = tr.scratchImage[stage->bundle[7].videoMapHandle];
			}
		}
		//
		// alphafunc <func>
		//
		else if ( !Q_stricmp( token, "alphaFunc" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'alphaFunc' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			atestBits = NameToAFunc( token );
		}
		//
		// depthFunc <func>
		//
		else if ( !Q_stricmp( token, "depthfunc" ) )
		{
			token = COM_ParseExt( text, qfalse );

			if ( !token[0] )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'depthfunc' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if ( !Q_stricmp( token, "lequal" ) )
			{
				depthFuncBits = 0;
			}
			else if ( !Q_stricmp( token, "equal" ) )
			{
				depthFuncBits = GLS_DEPTHFUNC_EQUAL;
			}
			else
			{
				ri.Printf( PRINT_WARNING, "WARNING: unknown depthfunc '%s' in shader '%s'\n", token, shader.name );
				continue;
			}
		}
		//
		// detail
		//
		else if ( !Q_stricmp( token, "detail" ) )
		{
			stage->isDetail = qtrue;
		}
		//
		// blendfunc <srcFactor> <dstFactor>
		// or blendfunc <add|filter|blend>
		//
		else if ( !Q_stricmp( token, "blendfunc" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name );
				continue;
			}
			// check for "simple" blends first
			if ( !Q_stricmp( token, "add" ) ) {
				blendSrcBits = GLS_SRCBLEND_ONE;
				blendDstBits = GLS_DSTBLEND_ONE;

			} else if ( !Q_stricmp( token, "filter" ) ) {
				blendSrcBits = GLS_SRCBLEND_DST_COLOR;
				blendDstBits = GLS_DSTBLEND_ZERO;
			} else if ( !Q_stricmp( token, "blend" ) ) {
				blendSrcBits = GLS_SRCBLEND_SRC_ALPHA;
				blendDstBits = GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			} else {
				// complex double blends
				blendSrcBits = NameToSrcBlendMode( token );

				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 )
				{
					ri.Printf( PRINT_WARNING, "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name );
					continue;
				}
				blendDstBits = NameToDstBlendMode( token );
			}
			
			stage->isBlend = 1;	// 2x2

			// clear depth mask for blended surfaces
			if ( !depthMaskExplicit )
			{
				depthMaskBits = 0;
			}


		}
		//
		// rgbGen
		//
		else if ( !Q_stricmp( token, "rgbGen" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameters for rgbGen in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "wave" ) )
			{
				ParseWaveForm( text, &stage->rgbWave );
				stage->rgbGen = CGEN_WAVEFORM;
			}
			else if ( !Q_stricmp( token, "const" ) )
			{
				vec3_t	color;

				ParseVector( text, 3, color );
				stage->constantColor[0] = 255 * color[0];
				stage->constantColor[1] = 255 * color[1];
				stage->constantColor[2] = 255 * color[2];

				stage->rgbGen = CGEN_CONST;
			}
			else if ( !Q_stricmp( token, "identity" ) )
			{
				stage->rgbGen = CGEN_IDENTITY;
			}
			else if ( !Q_stricmp( token, "identityLighting" ) )
			{
				stage->rgbGen = CGEN_IDENTITY_LIGHTING;
			}
			else if ( !Q_stricmp( token, "entity" ) )
			{
				stage->rgbGen = CGEN_ENTITY;
			}
			else if ( !Q_stricmp( token, "oneMinusEntity" ) )
			{
				stage->rgbGen = CGEN_ONE_MINUS_ENTITY;
			}
			else if ( !Q_stricmp( token, "vertex" ) )
			{
				stage->rgbGen = CGEN_VERTEX;
				if ( stage->alphaGen == 0 ) {
					stage->alphaGen = AGEN_VERTEX;
				}
			}
			else if ( !Q_stricmp( token, "exactVertex" ) )
			{
				stage->rgbGen = CGEN_EXACT_VERTEX;
			}
			else if ( !Q_stricmp( token, "lightingDiffuse" ) )
			{
				stage->rgbGen = CGEN_LIGHTING_DIFFUSE;
			}
			else if ( !Q_stricmp( token, "oneMinusVertex" ) )
			{
				stage->rgbGen = CGEN_ONE_MINUS_VERTEX;
			}
			else if ( !Q_stricmp( token, "material" ) )	// leilei - material system
			{
				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					ri.Printf( PRINT_WARNING, "WARNING: missing ambient color in shader '%s', using white\n", shader.name );
					stage->matAmb = 0xFFFFFF;
				}
				else
				stage->matAmb = ShParseNum( token );

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					ri.Printf( PRINT_WARNING, "WARNING: missing diffuse color in shader '%s', using gray\n", shader.name );
					stage->matSpec = 0xEEEEEE;
				}
				else
				stage->matDif = ShParseNum( token );

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					ri.Printf( PRINT_WARNING, "WARNING: missing specular color for material in shader '%s', using none\n", shader.name );
					stage->matSpec = 0x000000;
				}
				else
				stage->matSpec = ShParseNum( token );

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					ri.Printf( PRINT_WARNING, "WARNING: missing emissive color for material in shader '%s', using none\n", shader.name );
					stage->matEmis = 0x000000;
				}
				else
				stage->matEmis = ShParseNum( token );

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					ri.Printf( PRINT_WARNING, "WARNING: missing specular hardness for material in shader '%s', using none\n", shader.name );
					stage->matHard = 0;
				}
				else
				stage->matHard = atoi( token );

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					ri.Printf( PRINT_WARNING, "WARNING: missing alpha for material in shader '%s', using opaque\n", shader.name );
					stage->matAlpha = 255;
				}
				else
				stage->matAlpha = atoi( token );

				stage->rgbGen = CGEN_MATERIAL;
			}
			else
			{
				ri.Printf( PRINT_WARNING, "WARNING: unknown rgbGen parameter '%s' in shader '%s'\n", token, shader.name );
				continue;
			}
		}
		//
		// alphaGen 
		//
		else if ( !Q_stricmp( token, "alphaGen" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameters for alphaGen in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "wave" ) )
			{
				ParseWaveForm( text, &stage->alphaWave );
				stage->alphaGen = AGEN_WAVEFORM;
			}
			else if ( !Q_stricmp( token, "const" ) )
			{
				token = COM_ParseExt( text, qfalse );
				stage->constantColor[3] = 255 * atof( token );
				stage->alphaGen = AGEN_CONST;
			}
			else if ( !Q_stricmp( token, "identity" ) )
			{
				stage->alphaGen = AGEN_IDENTITY;
			}
			else if ( !Q_stricmp( token, "entity" ) )
			{
				stage->alphaGen = AGEN_ENTITY;
			}
			else if ( !Q_stricmp( token, "oneMinusEntity" ) )
			{
				stage->alphaGen = AGEN_ONE_MINUS_ENTITY;
			}
			else if ( !Q_stricmp( token, "vertex" ) )
			{
				stage->alphaGen = AGEN_VERTEX;
			}
			else if ( !Q_stricmp( token, "lightingSpecular" ) )
			{
				stage->alphaGen = AGEN_LIGHTING_SPECULAR;
			}
			else if ( !Q_stricmp( token, "oneMinusVertex" ) )
			{
				stage->alphaGen = AGEN_ONE_MINUS_VERTEX;
			}
			else if ( !Q_stricmp( token, "portal" ) )
			{
				stage->alphaGen = AGEN_PORTAL;
				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 )
				{
					shader.portalRange = 256;
					ri.Printf( PRINT_WARNING, "WARNING: missing range parameter for alphaGen portal in shader '%s', defaulting to 256\n", shader.name );
				}
				else
				{
					shader.portalRange = atof( token );
				}
			}
			else
			{
				ri.Printf( PRINT_WARNING, "WARNING: unknown alphaGen parameter '%s' in shader '%s'\n", token, shader.name );
				continue;
			}
		}
		//
		// rgbMod
		//
		else if ( !Q_stricmp( token, "rgbMod" ) )
		{
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parameters for rgbMod in shader '%s'\n", shader.name );
				continue;
			}
			else if ( !Q_stricmp( token, "glow" ) )
			{
				// TODO: Parse "entity" and "vertex" for their colors, for railguns and maps
				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					ri.Printf( PRINT_WARNING, "WARNING: missing hex color in shader '%s'\n", shader.name );
					return qfalse;
				}
				stage->rgbModCol = ShParseNum( token );

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					stage->rgbModMode = 0;
					stage->rgbMod = CMOD_GLOW;
					return qtrue;
				}
				stage->rgbModMode = atoi( token );

				stage->rgbMod = CMOD_GLOW;
			}
			else if ( !Q_stricmp( token, "uvcol" ) )
			{
				// Parse color
				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					ri.Printf( PRINT_WARNING, "WARNING: missing hex color in shader '%s'\n", shader.name );
					return qfalse;
				}
				stage->rgbModCol = ShParseNum( token );

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					stage->rgbModMode = 0;
					stage->rgbMod = CMOD_UVCOL;
					return qtrue;
				}
				stage->rgbModMode = atoi( token );

				stage->rgbMod = CMOD_UVCOL;
			}
			else if ( !Q_stricmp( token, "normtoalpha" ) )
			{
				// normalize colors, but average color prior is alpha.
				stage->rgbMod = CMOD_NORMALIZETOALPHA;
			}
			else if ( !Q_stricmp( token, "normtoalphafast" ) )
			{
				// normalize and use the first vert's color on whole surface, but average color prior is alpha.
				stage->rgbMod = CMOD_NORMALIZETOALPHAFAST;
			}
			else if ( !Q_stricmp( token, "lighting" ) )
			{
				// modulate a standard diffuse light on top of existing vertex colors
				stage->rgbMod = CMOD_LIGHTING;
			}
			else if ( !Q_stricmp( token, "opaque" ) )
			{
				// modulate a standard diffuse light on top of existing vertex colors
				stage->rgbMod = CMOD_OPAQUE;
			}
			else
			{
				ri.Printf( PRINT_WARNING, "WARNING: unknown rgbMod parameter '%s' in shader '%s'\n", token, shader.name );
				continue;
			}
		}
		//
		// tcGen <function>
		//
		else if ( !Q_stricmp(token, "texgen") || !Q_stricmp( token, "tcGen" ) ) 
		{
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing texgen parm in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "environment" ) )
			{
				stage->bundle[0].tcGen = TCGEN_ENVIRONMENT_MAPPED;
			}
			else if ( !Q_stricmp( token, "cel" ) )
			{
				stage->bundle[0].tcGen = TCGEN_ENVIRONMENT_MAPPED; 
			}
			else if ( !Q_stricmp( token, "celshading" ) )		// leilei - my technique is different
			{
				stage->bundle[0].tcGen = TCGEN_ENVIRONMENT_MAPPED;
			}
			else if ( !Q_stricmp( token, "eyeleft" ) )		// leilei - eye tracking
			{
				stage->bundle[0].tcGen = TCGEN_EYE_LEFT;
			}
			else if ( !Q_stricmp( token, "eyeright" ) )		// leilei - eye tracking
			{
				stage->bundle[0].tcGen = TCGEN_EYE_RIGHT;
			}
			else if ( !Q_stricmp( token, "environmentWater" ) )
			{
				stage->bundle[0].tcGen = TCGEN_ENVIRONMENT_MAPPED_WATER;	// leilei - water's envmaps
			}
			else if ( !Q_stricmp( token, "lightmap" ) )
			{
				stage->bundle[0].tcGen = TCGEN_LIGHTMAP;
			}
			else if ( !Q_stricmp( token, "texture" ) || !Q_stricmp( token, "base" ) )
			{
				stage->bundle[0].tcGen = TCGEN_TEXTURE;
			}
			else if ( !Q_stricmp( token, "vector" ) )
			{
				ParseVector( text, 3, stage->bundle[0].tcGenVectors[0] );
				ParseVector( text, 3, stage->bundle[0].tcGenVectors[1] );

				stage->bundle[0].tcGen = TCGEN_VECTOR;
			}
			else 
			{
				ri.Printf( PRINT_WARNING, "WARNING: unknown texgen parm in shader '%s'\n", shader.name );
			}
		}
		//
		// tcMod <type> <...>
		//
		else if ( !Q_stricmp( token, "tcMod" ) )
		{
			char buffer[1024] = "";

			while ( 1 )
			{
				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 )
					break;
				strcat( buffer, token );
				strcat( buffer, " " );
			}

			ParseTexMod( buffer, stage );

			continue;
		}
		//
		// depthmask
		//
		else if ( !Q_stricmp( token, "depthwrite" ) )
		{
			depthMaskBits = GLS_DEPTHMASK_TRUE;
			depthMaskExplicit = qtrue;

			continue;
		}

		else
		{
			ri.Printf( PRINT_WARNING, "WARNING: unknown parameter '%s' in shader '%s'\n", token, shader.name );
			return qfalse;
		}




	// END DITHER TEST
	}

	//
	// if cgen isn't explicitly specified, use either identity or identitylighting
	//
	if ( stage->rgbGen == CGEN_BAD ) {
		if ( blendSrcBits == 0 ||
			blendSrcBits == GLS_SRCBLEND_ONE || 
			blendSrcBits == GLS_SRCBLEND_SRC_ALPHA ) {
			stage->rgbGen = CGEN_IDENTITY_LIGHTING;
		} else {
			stage->rgbGen = CGEN_IDENTITY;
		}
	}

	
	//
	// implicitly assume that a GL_ONE GL_ZERO blend mask disables blending
	//
	if ( ( blendSrcBits == GLS_SRCBLEND_ONE ) && 
		 ( blendDstBits == GLS_DSTBLEND_ZERO ) )
	{
		blendDstBits = blendSrcBits = 0;
		depthMaskBits = GLS_DEPTHMASK_TRUE;
	}

	// decide which agens we can skip
	if ( stage->alphaGen == AGEN_IDENTITY ) {
		if ( stage->rgbGen == CGEN_IDENTITY
			|| stage->rgbGen == CGEN_LIGHTING_DIFFUSE 
			|| stage->rgbGen == CGEN_LIGHTING_UNIFORM 
			|| stage->rgbGen == CGEN_LIGHTING_DYNAMIC) {
			stage->alphaGen = AGEN_SKIP;
		}
	}

	//
	// compute state bits
	//
	stage->stateBits = depthMaskBits | 
		               blendSrcBits | blendDstBits | 
					   atestBits | 
					   depthFuncBits;


	return qtrue;
}


extern	int		hackoperation;
extern int ismaptexture;
int	surfaceflagsy;

/*
===============
ParseDeform

deformVertexes wave <spread> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes normal <frequency> <amplitude>
deformVertexes move <vector> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes bulge <bulgeWidth> <bulgeHeight> <bulgeSpeed>
deformVertexes projectionShadow
deformVertexes autoSprite
deformVertexes autoSprite2
deformVertexes text[0-7]
deformVertexes tessie	// leilei 
===============
*/
static void ParseDeform( char **text ) {
	char	*token;
	deformStage_t	*ds;

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 )
	{
		ri.Printf( PRINT_WARNING, "WARNING: missing deform parm in shader '%s'\n", shader.name );
		return;
	}

	if ( shader.numDeforms == MAX_SHADER_DEFORMS ) {
		ri.Printf( PRINT_WARNING, "WARNING: MAX_SHADER_DEFORMS in '%s'\n", shader.name );
		return;
	}

	ds = &shader.deforms[ shader.numDeforms ];
	shader.numDeforms++;

	if ( !Q_stricmp( token, "projectionShadow" ) ) {
		ds->deformation = DEFORM_PROJECTION_SHADOW;
		return;
	}


	if ( !Q_stricmp( token, "autosprite" ) ) {
		ds->deformation = DEFORM_AUTOSPRITE;
		return;
	}

	if ( !Q_stricmp( token, "autosprite2" ) ) {
		ds->deformation = DEFORM_AUTOSPRITE2;
		return;
	}

	if ( !Q_stricmpn( token, "text", 4 ) ) {
		int		n;
		
		n = token[4] - '0';
		if ( n < 0 || n > 7 ) {
			n = 0;
		}
		ds->deformation = DEFORM_TEXT0 + n;
		return;
	}

	if ( !Q_stricmp( token, "bulge" ) )	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeWidth = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeHeight = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeSpeed = atof( token );

		ds->deformation = DEFORM_BULGE;
		return;
	}

	if ( !Q_stricmp( token, "wave" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}

		if ( atof( token ) != 0 )
		{
			ds->deformationSpread = 1.0f / atof( token );
		}
		else
		{
			ds->deformationSpread = 100.0f;
			ri.Printf( PRINT_WARNING, "WARNING: illegal div value of 0 in deformVertexes command for shader '%s'\n", shader.name );
		}

		ParseWaveForm( text, &ds->deformationWave );
		ds->deformation = DEFORM_WAVE;
		return;
	}

	if ( !Q_stricmp( token, "normal" ) )
	{
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}
		ds->deformationWave.amplitude = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 )
		{
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}
		ds->deformationWave.frequency = atof( token );

		ds->deformation = DEFORM_NORMALS;
		return;
	}

	if ( !Q_stricmp( token, "move" ) ) {
		int		i;

		for ( i = 0 ; i < 3 ; i++ ) {
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
				return;
			}
			ds->moveVector[i] = atof( token );
		}

		ParseWaveForm( text, &ds->deformationWave );
		ds->deformation = DEFORM_MOVE;
		return;
	}

	ri.Printf( PRINT_WARNING, "WARNING: unknown deformVertexes subtype '%s' found in shader '%s'\n", token, shader.name );
}


/*
===============
ParseSkyParms

skyParms <outerbox> <cloudheight> <innerbox>
===============
*/
static void ParseSkyParms( char **text ) {
	char		*token;
	static char	*suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};
	char		pathname[MAX_QPATH];
	int			i;
	imgFlags_t imgFlags = IMGFLAG_MIPMAP | IMGFLAG_PICMIP;

	// outerbox
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
		return;
	}
	if ( strcmp( token, "-" ) ) {
		for (i=0 ; i<6 ; i++) {
			Com_sprintf( pathname, sizeof(pathname), "%s_%s.tga"
				, token, suf[i] );
			shader.sky.outerbox[i] = R_FindImageFile( ( char * ) pathname, IMGTYPE_COLORALPHA, imgFlags | IMGFLAG_CLAMPTOEDGE );

			if ( !shader.sky.outerbox[i] ) {
				shader.sky.outerbox[i] = tr.defaultImage;
			}
		}
	}

	// cloudheight
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
		return;
	}
	shader.sky.cloudHeight = atof( token );
	if ( !shader.sky.cloudHeight ) {
		shader.sky.cloudHeight = 512;
	}
	R_InitSkyTexCoords( shader.sky.cloudHeight );


	// innerbox
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
		return;
	}
	if ( strcmp( token, "-" ) ) {
		for (i=0 ; i<6 ; i++) {
			Com_sprintf( pathname, sizeof(pathname), "%s_%s.tga"
				, token, suf[i] );
			shader.sky.innerbox[i] = R_FindImageFile( ( char * ) pathname, IMGTYPE_COLORALPHA, imgFlags );
			if ( !shader.sky.innerbox[i] ) {
				shader.sky.innerbox[i] = tr.defaultImage;
			}
		}
	}

	shader.isSky = qtrue;
}


/*
=================
ParseSort
=================
*/
void ParseSort( char **text ) {
	char	*token;

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing sort parameter in shader '%s'\n", shader.name );
		return;
	}

	if ( !Q_stricmp( token, "portal" ) ) {
		shader.sort = SS_PORTAL;
	} else if ( !Q_stricmp( token, "sky" ) ) {
		shader.sort = SS_ENVIRONMENT;
	} else if ( !Q_stricmp( token, "opaque" ) ) {
		shader.sort = SS_OPAQUE;
	}else if ( !Q_stricmp( token, "decal" ) ) {
		shader.sort = SS_DECAL;
	} else if ( !Q_stricmp( token, "seeThrough" ) ) {
		shader.sort = SS_SEE_THROUGH;
	} else if ( !Q_stricmp( token, "banner" ) ) {
		shader.sort = SS_BANNER;
	} else if ( !Q_stricmp( token, "additive" ) ) {
		shader.sort = SS_BLEND1;
	} else if ( !Q_stricmp( token, "nearest" ) ) {
		shader.sort = SS_NEAREST;
	} else if ( !Q_stricmp( token, "underwater" ) ) {
		shader.sort = SS_UNDERWATER;
	} else {
		shader.sort = atof( token );
	}
}



// this table is also present in q3map

typedef struct {
	char	*name;
	int		clearSolid, surfaceFlags, contents;
} infoParm_t;

infoParm_t	infoParms[] = {
	// server relevant contents
	{"water",		1,	0,	CONTENTS_WATER },
	{"slime",		1,	0,	CONTENTS_SLIME },		// mildly damaging
	{"lava",		1,	0,	CONTENTS_LAVA },		// very damaging
	{"playerclip",	1,	0,	CONTENTS_PLAYERCLIP },
	{"monsterclip",	1,	0,	CONTENTS_MONSTERCLIP },
	{"nodrop",		1,	0,	CONTENTS_NODROP },		// don't drop items or leave bodies (death fog, lava, etc)
	{"nonsolid",	1,	SURF_NONSOLID,	0},						// clears the solid flag

	// utility relevant attributes
	{"origin",		1,	0,	CONTENTS_ORIGIN },		// center of rotating brushes
	{"trans",		0,	0,	CONTENTS_TRANSLUCENT },	// don't eat contained surfaces
	{"detail",		0,	0,	CONTENTS_DETAIL },		// don't include in structural bsp
	{"structural",	0,	0,	CONTENTS_STRUCTURAL },	// force into structural bsp even if trnas
	{"areaportal",	1,	0,	CONTENTS_AREAPORTAL },	// divides areas
	{"clusterportal", 1,0,  CONTENTS_CLUSTERPORTAL },	// for bots
	{"donotenter",  1,  0,  CONTENTS_DONOTENTER },		// for bots

	{"fog",			1,	0,	CONTENTS_FOG},			// carves surfaces entering
	{"sky",			0,	SURF_SKY,		0 },		// emit light from an environment map
	{"lightfilter",	0,	SURF_LIGHTFILTER, 0 },		// filter light going through it
	{"alphashadow",	0,	SURF_ALPHASHADOW, 0 },		// test light on a per-pixel basis
	{"hint",		0,	SURF_HINT,		0 },		// use as a primary splitter

	// server attributes
	{"slick",		0,	SURF_SLICK,		0 },
	{"noimpact",	0,	SURF_NOIMPACT,	0 },		// don't make impact explosions or marks
	{"nomarks",		0,	SURF_NOMARKS,	0 },		// don't make impact marks, but still explode
	{"ladder",		0,	SURF_LADDER,	0 },
	{"nodamage",	0,	SURF_NODAMAGE,	0 },
	{"metalsteps",	0,	SURF_METALSTEPS,0 },
	{"flesh",		0,	SURF_FLESH,		0 },
	{"nosteps",		0,	SURF_NOSTEPS,	0 },

	// drawsurf attributes
	{"nodraw",		0,	SURF_NODRAW,	0 },	// don't generate a drawsurface (or a lightmap)
	{"pointlight",	0,	SURF_POINTLIGHT, 0 },	// sample lighting at vertexes
	{"nolightmap",	0,	SURF_NOLIGHTMAP,0 },	// don't generate a lightmap
	{"nodlight",	0,	SURF_NODLIGHT, 0 },		// don't ever add dynamic lights
	{"dust",		0,	SURF_DUST, 0}			// leave a dust trail when walking on this surface
};


/*
===============
ParseSurfaceParm

surfaceparm <name>
===============
*/
static void ParseSurfaceParm( char **text ) {
	char	*token;
	int		numInfoParms = ARRAY_LEN( infoParms );
	int		i;

	token = COM_ParseExt( text, qfalse );
	for ( i = 0 ; i < numInfoParms ; i++ ) {
		if ( !Q_stricmp( token, infoParms[i].name ) ) {
			shader.surfaceFlags |= infoParms[i].surfaceFlags;
			shader.contentFlags |= infoParms[i].contents;
#if 0
			if ( infoParms[i].clearSolid ) {
				si->contents &= ~CONTENTS_SOLID;
			}
#endif
			break;
		}
	}
}

/*
=================
ParseShader

The current text pointer is at the explicit text definition of the
shader.  Parse it into the global shader variable.  Later functions
will optimize it.
=================
*/
static qboolean ParseShader( char **text )
{
	char *token;
	int s;

	s = 0;

	token = COM_ParseExt( text, qtrue );
	if ( token[0] != '{' )
	{
		ri.Printf( PRINT_WARNING, "WARNING: expecting '{', found '%s' instead in shader '%s'\n", token, shader.name );
		return qfalse;
	}

	while ( 1 )
	{
		token = COM_ParseExt( text, qtrue );
		if ( !token[0] )
		{
			ri.Printf( PRINT_WARNING, "WARNING: no concluding '}' in shader %s\n", shader.name );
			return qfalse;
		}

		// end of shader definition
		if ( token[0] == '}' )
		{
			break;
		}

		// stage definition
		else if ( token[0] == '{' )
		{
			if ( s >= MAX_SHADER_STAGES ) {
				ri.Printf( PRINT_WARNING, "WARNING: too many stages in shader %s\n", shader.name );
				return qfalse;
			}

			if ( !ParseStage( &stages[s], text ) )
			{
				return qfalse;
			}
			stages[s].active = qtrue;
			s++;

			continue;
		}
		// skip stuff that only the QuakeEdRadient needs
		else if ( !Q_stricmpn( token, "qer", 3 ) ) {
			SkipRestOfLine( text );
			continue;
		}

		// leilei - Parse a shader for the sun, and a type of flare for the number after it.
		// Inspired by ET's sunShader (my own implementation)
		else if ( !Q_stricmp( token, "sunShader" ) ) {
			// load a sun shader to override our default shader
			token = COM_ParseExt( text, qfalse );

			tr.sunShaderCustom = CopyString(token); // leilei - copy the shader's name so we can load it properly elsewhere (doing it here would make a broken sun)
			tr.sunOn = 1;	// activate sun to be rendered for r_drawSun 1.		

			// the next one is the flare type.
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) 
			{
				tr.sunFlare = 0;	// dont flare if we dont specify
				continue;
			}
			tr.sunFlare = atoi( token );

			// the next one is a size multiplier.
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) 
			{
				tr.sunOn = 1;		// sun is always normal size if we don't specify
				continue;
			}
			if (atof( token )>1)
			tr.sunOn = atof( token );

			continue;
			}

		// sun parms
		else if ( !Q_stricmp( token, "q3map_sun" ) ) {
			float	a, b;

			token = COM_ParseExt( text, qfalse );
			tr.sunLight[0] = atof( token );
			token = COM_ParseExt( text, qfalse );
			tr.sunLight[1] = atof( token );
			token = COM_ParseExt( text, qfalse );
			tr.sunLight[2] = atof( token );
			
			VectorNormalize( tr.sunLight );

			token = COM_ParseExt( text, qfalse );
			a = atof( token );
			VectorScale( tr.sunLight, a, tr.sunLight);

			token = COM_ParseExt( text, qfalse );
			a = atof( token );
			a = a / 180 * M_PI;

			token = COM_ParseExt( text, qfalse );
			b = atof( token );
			b = b / 180 * M_PI;

			tr.sunDirection[0] = cos( a ) * cos( b );
			tr.sunDirection[1] = sin( a ) * cos( b );
			tr.sunDirection[2] = sin( b );
		}
		else if ( !Q_stricmp( token, "deformVertexes" ) ) {
			ParseDeform( text );
			continue;
		}
		else if ( !Q_stricmp( token, "tesssize" ) ) {
			SkipRestOfLine( text );
			continue;
		}
		else if ( !Q_stricmp( token, "clampTime" ) ) {
			token = COM_ParseExt( text, qfalse );
      if (token[0]) {
        shader.clampTime = atof(token);
      }
    }
		// skip stuff that only the q3map needs
		else if ( !Q_stricmpn( token, "q3map", 5 ) ) {
			SkipRestOfLine( text );
			continue;
		}
		// skip stuff that only q3map or the server needs
		else if ( !Q_stricmp( token, "surfaceParm" ) ) {
			ParseSurfaceParm( text );
			continue;
		}
		// no mip maps
		else if ( !Q_stricmp( token, "nomipmaps" ) )
		{
			shader.noMipMaps = qtrue;
			shader.noPicMip = qtrue;
			continue;
		}
		// no picmip adjustment
		else if ( !Q_stricmp( token, "nopicmip" ) )
		{
			shader.noPicMip = qtrue;
			continue;
		}
		// polygonOffset
		else if ( !Q_stricmp( token, "polygonOffset" ) )
		{
			shader.polygonOffset = qtrue;
			continue;
		}
		// entityMergable, allowing sprite surfaces from multiple entities
		// to be merged into one batch.  This is a savings for smoke
		// puffs and blood, but can't be used for anything where the
		// shader calcs (not the surface function) reference the entity color or scroll
		else if ( !Q_stricmp( token, "entityMergable" ) )
		{
			shader.entityMergable = qtrue;
			continue;
		}
		// fogParms
		else if ( !Q_stricmp( token, "fogParms" ) ) 
		{
			if ( !ParseVector( text, 3, shader.fogParms.color ) ) {
				return qfalse;
			}

			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) 
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing parm for 'fogParms' keyword in shader '%s'\n", shader.name );
				continue;
			}
			shader.fogParms.depthForOpaque = atof( token );

			// skip any old gradient directions
			SkipRestOfLine( text );
			continue;
		}
		// portal
		else if ( !Q_stricmp(token, "portal") )
		{
			shader.sort = SS_PORTAL;
			continue;
		}
		// skyparms <cloudheight> <outerbox> <innerbox>
		else if ( !Q_stricmp( token, "skyparms" ) )
		{
			ParseSkyParms( text );
			continue;
		}
		// light <value> determines flaring in q3map, not needed here
		else if ( !Q_stricmp(token, "light") ) 
		{
			(void)COM_ParseExt( text, qfalse );
			continue;
		}
		// cull <face>
		else if ( !Q_stricmp( token, "cull") ) 
		{
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 )
			{
				ri.Printf( PRINT_WARNING, "WARNING: missing cull parms in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "none" ) || !Q_stricmp( token, "twosided" ) || !Q_stricmp( token, "disable" ) )
			{
				shader.cullType = CT_TWO_SIDED;
			}
			else if ( !Q_stricmp( token, "back" ) || !Q_stricmp( token, "backside" ) || !Q_stricmp( token, "backsided" ) )
			{
				shader.cullType = CT_BACK_SIDED;
			}
			else
			{
				ri.Printf( PRINT_WARNING, "WARNING: invalid cull parm '%s' in shader '%s'\n", token, shader.name );
			}
			continue;
		}
		// sort
		else if ( !Q_stricmp( token, "sort" ) )
		{
			ParseSort( text );
			continue;
		}
		else
		{
			ri.Printf( PRINT_WARNING, "WARNING: unknown general shader parameter '%s' in '%s'\n", token, shader.name );
			return qfalse;
		}
	}

	//
	// ignore shaders that don't have any stages, unless it is a sky or fog
	//
	if ( s == 0 && !shader.isSky && !(shader.contentFlags & CONTENTS_FOG ) ) {
		return qfalse;
	}

	shader.explicitlyDefined = qtrue;
			surfaceflagsy = shader.surfaceFlags;


	return qtrue;
}

/*
========================================================================================

SHADER OPTIMIZATION AND FOGGING

========================================================================================
*/

/*
===================
ComputeStageIteratorFunc

See if we can use on of the simple fastpath stage functions,
otherwise set to the generic stage function
===================
*/
static void ComputeStageIteratorFunc( void )
{
#ifdef GLSL_TEXTURES
	if (vertexShaders)
		shader.optimalStageIteratorFunc = RB_GLSL_StageIteratorGeneric;
	else
#endif
		shader.optimalStageIteratorFunc = RB_StageIteratorGeneric;

	//
	// see if this should go into the sky path
	//
	if ( shader.isSky )
	{
		shader.optimalStageIteratorFunc = RB_StageIteratorSky;
		return;
	}



	if ( r_ignoreFastPath->integer )
	{
		return;
	}

	if (r_leifx->integer)	// try not to use the multitexture shaders so we can blend
		return; 	// lightmaps with a different dithering table



	//
	// see if this can go into the vertex lit fast path
	//
	if ( shader.numUnfoggedPasses == 1 )
	{
		if ( stages[0].rgbGen == CGEN_LIGHTING_DIFFUSE || stages[0].rgbGen == CGEN_LIGHTING_UNIFORM || stages[0].rgbGen == CGEN_LIGHTING_DYNAMIC)
		{
			if ( stages[0].alphaGen == AGEN_IDENTITY )
			{
				if ( stages[0].bundle[0].tcGen == TCGEN_TEXTURE )
				{
					if ( !shader.polygonOffset )
					{
						if ( !shader.multitextureEnv )
						{
							if ( !shader.numDeforms )
							{
#ifdef GLSL_TEXTURES
								if (vertexShaders)
									shader.optimalStageIteratorFunc = RB_GLSL_StageIteratorVertexLitTexture;
								else
#endif
									shader.optimalStageIteratorFunc = RB_StageIteratorVertexLitTexture;
								return;
							}
						}
					}
				}
			}
		}
	}

	//
	// see if this can go into an optimized LM, multitextured path
	//

	if ( shader.numUnfoggedPasses == 1 )
	{
		if ( ( stages[0].rgbGen == CGEN_IDENTITY ) && ( stages[0].alphaGen == AGEN_IDENTITY ) )
		{
			if ( stages[0].bundle[0].tcGen == TCGEN_TEXTURE && 
				stages[0].bundle[1].tcGen == TCGEN_LIGHTMAP )
			{
				if ( !shader.polygonOffset )
				{
					if ( !shader.numDeforms )
					{
						if ( shader.multitextureEnv )
						{
#ifdef GLSL_TEXTURES
							if (vertexShaders)
								shader.optimalStageIteratorFunc = RB_GLSL_StageIteratorLightmappedMultitexture;
							else
#endif
								shader.optimalStageIteratorFunc = RB_StageIteratorLightmappedMultitexture;
						}
					}
				}
			}
		}
	}
}

typedef struct {
	int		blendA;
	int		blendB;

	int		multitextureEnv;
	int		multitextureBlend;
} collapse_t;

static collapse_t	collapse[] = {
	{ 0, GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,	
		GL_MODULATE, 0 },

	{ 0, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
		GL_MODULATE, 0 },

	{ GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
		GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
		GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR, GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,
		GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO, GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,
		GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ 0, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE,
		GL_ADD, 0 },

	{ GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE,
		GL_ADD, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE },
#if 0
	{ 0, GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_SRCBLEND_SRC_ALPHA,
		GL_DECAL, 0 },
#endif
	{ -1 }
};

/*
================
CollapseMultitexture

Attempt to combine two stages into a single multitexture stage
FIXME: I think modulated add + modulated add collapses incorrectly
=================
*/
static qboolean CollapseMultitexture( void ) {
	int abits, bbits;
	int i;
	textureBundle_t tmpBundle;

	if ( !qglActiveTextureARB ) {
		return qfalse;
	}

	if (r_leifx->integer > 1)
		return qfalse; // don't do this for leifx mode

	// make sure both stages are active
	if ( !stages[0].active || !stages[1].active ) {
		return qfalse;
	}

	// on voodoo2, don't combine different tmus
	if ( glConfig.driverType == GLDRV_VOODOO || r_leifx->integer ) {
		if ( stages[0].bundle[0].image[0]->TMU ==
			 stages[1].bundle[0].image[0]->TMU ) {
			return qfalse;
		}
	}

	abits = stages[0].stateBits;
	bbits = stages[1].stateBits;

	// make sure that both stages have identical state other than blend modes
	if ( ( abits & ~( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS | GLS_DEPTHMASK_TRUE ) ) !=
		( bbits & ~( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS | GLS_DEPTHMASK_TRUE ) ) ) {
		return qfalse;
	}

	abits &= ( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );
	bbits &= ( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );

	// search for a valid multitexture blend function
	for ( i = 0; collapse[i].blendA != -1 ; i++ ) {
		if ( abits == collapse[i].blendA
			&& bbits == collapse[i].blendB ) {
			break;
		}
	}

	// nothing found
	if ( collapse[i].blendA == -1 ) {
		return qfalse;
	}

	// GL_ADD is a separate extension
	if ( collapse[i].multitextureEnv == GL_ADD && !glConfig.textureEnvAddAvailable ) {
		return qfalse;
	}

	// make sure waveforms have identical parameters
	if ( ( stages[0].rgbGen != stages[1].rgbGen ) ||
		( stages[0].alphaGen != stages[1].alphaGen ) )  {
		return qfalse;
	}

	// an add collapse can only have identity colors
	if ( collapse[i].multitextureEnv == GL_ADD && stages[0].rgbGen != CGEN_IDENTITY ) {
		return qfalse;
	}

	if ( stages[0].rgbGen == CGEN_WAVEFORM )
	{
		if ( memcmp( &stages[0].rgbWave,
					 &stages[1].rgbWave,
					 sizeof( stages[0].rgbWave ) ) )
		{
			return qfalse;
		}
	}
	if ( stages[0].alphaGen == AGEN_WAVEFORM )
	{
		if ( memcmp( &stages[0].alphaWave,
					 &stages[1].alphaWave,
					 sizeof( stages[0].alphaWave ) ) )
		{
			return qfalse;
		}
	}


	// make sure that lightmaps are in bundle 1 for 3dfx
	if ( stages[0].bundle[0].isLightmap )
	{
		tmpBundle = stages[0].bundle[0];
		stages[0].bundle[0] = stages[1].bundle[0];
		stages[0].bundle[1] = tmpBundle;
	}
	else
	{
		stages[0].bundle[1] = stages[1].bundle[0];
	}

	// set the new blend state bits
	shader.multitextureEnv = collapse[i].multitextureEnv;
	stages[0].stateBits &= ~( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );
	stages[0].stateBits |= collapse[i].multitextureBlend;

	//
	// move down subsequent shaders
	//
	memmove( &stages[1], &stages[2], sizeof( stages[0] ) * ( MAX_SHADER_STAGES - 2 ) );
	Com_Memset( &stages[MAX_SHADER_STAGES-1], 0, sizeof( stages[0] ) );

	return qtrue;
}

/*
=============

FixRenderCommandList
https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
Arnout: this is a nasty issue. Shaders can be registered after drawsurfaces are generated
but before the frame is rendered. This will, for the duration of one frame, cause drawsurfaces
to be rendered with bad shaders. To fix this, need to go through all render commands and fix
sortedIndex.
==============
*/
static void FixRenderCommandList( int newShader ) {
	renderCommandList_t	*cmdList = &backEndData->commands;

	if( cmdList ) {
		const void *curCmd = cmdList->cmds;

		while ( 1 ) {
			curCmd = PADP(curCmd, sizeof(void *));

			switch ( *(const int *)curCmd ) {
			case RC_SET_COLOR:
				{
				const setColorCommand_t *sc_cmd = (const setColorCommand_t *)curCmd;
				curCmd = (const void *)(sc_cmd + 1);
				break;
				}
			case RC_STRETCH_PIC:
				{
				const stretchPicCommand_t *sp_cmd = (const stretchPicCommand_t *)curCmd;
				curCmd = (const void *)(sp_cmd + 1);
				break;
				}
			case RC_DRAW_SURFS:
				{
				int i;
				drawSurf_t	*drawSurf;
				shader_t	*shader;
				int			fogNum;
				int			entityNum;
				int			dlightMap;
				int			sortedIndex;
				const drawSurfsCommand_t *ds_cmd =  (const drawSurfsCommand_t *)curCmd;

				for( i = 0, drawSurf = ds_cmd->drawSurfs; i < ds_cmd->numDrawSurfs; i++, drawSurf++ ) {
					R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlightMap );
                    sortedIndex = (( drawSurf->sort >> QSORT_SHADERNUM_SHIFT ) & (MAX_SHADERS-1));
					if( sortedIndex >= newShader ) {
						sortedIndex++;
						drawSurf->sort = (sortedIndex << QSORT_SHADERNUM_SHIFT) | entityNum | ( fogNum << QSORT_FOGNUM_SHIFT ) | (int)dlightMap;
					}
				}
				curCmd = (const void *)(ds_cmd + 1);
				break;
				}
			case RC_DRAW_BUFFER:
				{
				const drawBufferCommand_t *db_cmd = (const drawBufferCommand_t *)curCmd;
				curCmd = (const void *)(db_cmd + 1);
				break;
				}
			case RC_SWAP_BUFFERS:
				{
				const swapBuffersCommand_t *sb_cmd = (const swapBuffersCommand_t *)curCmd;
				curCmd = (const void *)(sb_cmd + 1);
				break;
				}
			case RC_END_OF_LIST:
			default:
				return;
			}
		}
	}
}

/*
==============
SortNewShader

Positions the most recently created shader in the tr.sortedShaders[]
array so that the shader->sort key is sorted relative to the other
shaders.

Sets shader->sortedIndex
==============
*/
static void SortNewShader( void ) {
	int		i;
	float	sort;
	shader_t	*newShader;

	newShader = tr.shaders[ tr.numShaders - 1 ];
	sort = newShader->sort;

	for ( i = tr.numShaders - 2 ; i >= 0 ; i-- ) {
		if ( tr.sortedShaders[ i ]->sort <= sort ) {
			break;
		}
		tr.sortedShaders[i+1] = tr.sortedShaders[i];
		tr.sortedShaders[i+1]->sortedIndex++;
	}

	// Arnout: fix rendercommandlist
	// https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=493
	FixRenderCommandList( i+1 );

	newShader->sortedIndex = i+1;
	tr.sortedShaders[i+1] = newShader;
}


/*
====================
GeneratePermanentShader
====================
*/
static shader_t *GeneratePermanentShader( void ) {
	shader_t	*newShader;
	int			i, b;
	int			size, hash;

	if ( tr.numShaders == MAX_SHADERS ) {
		ri.Printf( PRINT_WARNING, "WARNING: GeneratePermanentShader - MAX_SHADERS hit\n");
		return tr.defaultShader;
	}

	newShader = ri.Hunk_Alloc( sizeof( shader_t ), h_low );

	*newShader = shader;

	if ( shader.sort <= SS_OPAQUE ) {
		newShader->fogPass = FP_EQUAL;
	} else if ( shader.contentFlags & CONTENTS_FOG ) {
		newShader->fogPass = FP_LE;
	}

	tr.shaders[ tr.numShaders ] = newShader;
	newShader->index = tr.numShaders;
	
	tr.sortedShaders[ tr.numShaders ] = newShader;
	newShader->sortedIndex = tr.numShaders;

	tr.numShaders++;

	for ( i = 0 ; i < newShader->numUnfoggedPasses ; i++ ) {
		if ( !stages[i].active ) {
			break;
		}
		newShader->stages[i] = ri.Hunk_Alloc( sizeof( stages[i] ), h_low );
		*newShader->stages[i] = stages[i];

		for ( b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++ ) {
			size = newShader->stages[i]->bundle[b].numTexMods * sizeof( texModInfo_t );
			newShader->stages[i]->bundle[b].texMods = ri.Hunk_Alloc( size, h_low );
			Com_Memcpy( newShader->stages[i]->bundle[b].texMods, stages[i].bundle[b].texMods, size );
		}
	}

	SortNewShader();

	hash = generateHashValue(newShader->name, FILE_HASH_SIZE);
	newShader->next = hashTable[hash];
	hashTable[hash] = newShader;

	return newShader;
}

/*
=================
VertexLightingCollapse

If vertex lighting is enabled, only render a single
pass, trying to guess which is the correct one to best aproximate
what it is supposed to look like.
=================
*/
static void VertexLightingCollapse( void ) {
	int		stage;
	shaderStage_t	*bestStage;
	int		bestImageRank;
	int		rank;

	// if we aren't opaque, just use the first pass
	if ( shader.sort == SS_OPAQUE ) {

		// pick the best texture for the single pass
		bestStage = &stages[0];
		bestImageRank = -999999;

		for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ ) {
			shaderStage_t *pStage = &stages[stage];

			if ( !pStage->active ) {
				break;
			}
			rank = 0;

			if ( pStage->bundle[0].isLightmap ) {
				rank -= 100;
			}
			if ( pStage->bundle[0].tcGen != TCGEN_TEXTURE ) {
				rank -= 5;
			}
			if ( pStage->bundle[0].numTexMods ) {
				rank -= 5;
			}
			if ( pStage->rgbGen != CGEN_IDENTITY && pStage->rgbGen != CGEN_IDENTITY_LIGHTING ) {
				rank -= 3;
			}

			if ( rank > bestImageRank  ) {
				bestImageRank = rank;
				bestStage = pStage;
			}
		}

		stages[0].bundle[0] = bestStage->bundle[0];
		stages[0].stateBits &= ~( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );
		stages[0].stateBits |= GLS_DEPTHMASK_TRUE;
		if ( shader.lightmapIndex == LIGHTMAP_NONE ) {
			stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		} else {
		stages[0].rgbGen = CGEN_VERTEX_LIT;// leilei - dynamic lights in vertex lighting
		//stages[0].rgbGen = CGEN_EXACT_VERTEX;
		}
		stages[0].alphaGen = AGEN_SKIP;		
	} else {
		// don't use a lightmap (tesla coils)
		if ( stages[0].bundle[0].isLightmap ) {
			stages[0] = stages[1];
		}

		// if we were in a cross-fade cgen, hack it to normal
		if ( stages[0].rgbGen == CGEN_ONE_MINUS_ENTITY || stages[1].rgbGen == CGEN_ONE_MINUS_ENTITY ) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if ( ( stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_SAWTOOTH )
			&& ( stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_INVERSE_SAWTOOTH ) ) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if ( ( stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_INVERSE_SAWTOOTH )
			&& ( stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_SAWTOOTH ) ) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
	}

	for ( stage = 1; stage < MAX_SHADER_STAGES; stage++ ) {
		shaderStage_t *pStage = &stages[stage];

		if ( !pStage->active ) {
			break;
		}

		Com_Memset( pStage, 0, sizeof( *pStage ) );
	}
}

/*
=========================
FinishShader

Returns a freshly allocated shader with all the needed info
from the current global working shader
=========================
*/
static shader_t *FinishShader( void ) {
	int stage;
	qboolean		hasLightmapStage;
	qboolean		vertexLightmap;
	

	hasLightmapStage = qfalse;
	vertexLightmap = qfalse;

	

	//
	// set sky stuff appropriate
	//
	if ( shader.isSky ) {
		shader.sort = SS_ENVIRONMENT;
	}

	//
	// set polygon offset
	//
	if ( shader.polygonOffset && !shader.sort ) {
		shader.sort = SS_DECAL;
	}



	//
	// set appropriate stage information
	//
	for ( stage = 0; stage < MAX_SHADER_STAGES; ) {
		shaderStage_t *pStage = &stages[stage];

		if ( !pStage->active ) {
			break;
		}

#ifdef GLSL_TEXTURES
		// Try to use leifx dither here instead of postprocess for more authentic overdraw artifacts
		if (r_leifx->integer > 1)
		{		
			
			if (pStage->isBlend == 1){
			pStage->program = RE_GLSL_RegisterProgram("leifxify2", "glsl/leifxify2_vp.glsl", 1, "glsl/leifxify2_fp.glsl", 1);	// 2x2 dither blend for vertex colors, 4x4 for texture
			ri.Printf( PRINT_DEVELOPER, "picking blended\n");
			}
			else
			pStage->program = RE_GLSL_RegisterProgram("leifxify", "glsl/leifxify2_vp.glsl", 1, "glsl/leifxify_fp.glsl", 1);		// 4x4 dither blend for both color and texture
			pStage->isGLSL=1;
		}

		// Mockvr faking of the filtering and alpha precision of PCX2 cards
		if ((r_mockvr->integer > 1))
		{		
			pStage->program = RE_GLSL_RegisterProgram("leivrify", "glsl/leivrify_vp.glsl", 1, "glsl/leivrify_fp.glsl", 1);
			
			pStage->isGLSL=1;

			// pass texture width/height (filtering etc)
			if(!pStage->imgWidth) pStage->imgWidth = 128;
			if(!pStage->imgHeight) pStage->imgHeight = 128;
		//	 R_GLSL_SetUniform_u_ScreenSizeX(pStage->program, pStage->imgWidth);
		//	 R_GLSL_SetUniform_u_ScreenSizeY(pStage->program, pStage->imgHeight);
		//	 R_GLSL_SetUniform_u_ScreenToNextPixelX(pStage->program, (float)1.0/(float)pStage->imgWidth);
		//	 R_GLSL_SetUniform_u_ScreenToNextPixelY(pStage->program, (float)1.0/(float)pStage->imgHeight);
	
		}

#endif	// GLSL_TEXTURES

    // check for a missing texture
		if ( !pStage->bundle[0].image[0] ) {
			ri.Printf( PRINT_WARNING, "Shader %s has a stage with no image\n", shader.name );
			pStage->active = qfalse;
			stage++;
			continue;
		}


		//
		// ditch this stage if it's detail and detail textures are disabled
		//
		if ( pStage->isDetail && !r_detailTextures->integer )
		{
			int index;
			
			for(index = stage + 1; index < MAX_SHADER_STAGES; index++)
			{
				if(!stages[index].active)
					break;
			}
			
			if(index < MAX_SHADER_STAGES)
				memmove(pStage, pStage + 1, sizeof(*pStage) * (index - stage));
			else
			{
				if(stage + 1 < MAX_SHADER_STAGES)
					memmove(pStage, pStage + 1, sizeof(*pStage) * (index - stage - 1));
				
				Com_Memset(&stages[index - 1], 0, sizeof(*stages));
			}
			
			continue;
		}

		//
		// default texture coordinate generation
		//
		if ( pStage->bundle[0].isLightmap ) {
			if ( pStage->bundle[0].tcGen == TCGEN_BAD ) {
				pStage->bundle[0].tcGen = TCGEN_LIGHTMAP;
			}
			hasLightmapStage = qtrue;
		} else {
			if ( pStage->bundle[0].tcGen == TCGEN_BAD ) {
				pStage->bundle[0].tcGen = TCGEN_TEXTURE;
			}
		}



    // not a true lightmap but we want to leave existing 
    // behaviour in place and not print out a warning
    //if (pStage->rgbGen == CGEN_VERTEX) {
    //  vertexLightmap = qtrue;
    //}

		//
		// determine sort order and fog color adjustment
		//
		if ( ( pStage->stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) &&
			 ( stages[0].stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) ) {
			int blendSrcBits = pStage->stateBits & GLS_SRCBLEND_BITS;
			int blendDstBits = pStage->stateBits & GLS_DSTBLEND_BITS;

			// fog color adjustment only works for blend modes that have a contribution
			// that aproaches 0 as the modulate values aproach 0 --
			// GL_ONE, GL_ONE
			// GL_ZERO, GL_ONE_MINUS_SRC_COLOR
			// GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA

			// modulate, additive
			if ( ( ( blendSrcBits == GLS_SRCBLEND_ONE ) && ( blendDstBits == GLS_DSTBLEND_ONE ) ) ||
				( ( blendSrcBits == GLS_SRCBLEND_ZERO ) && ( blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR ) ) ) {
				pStage->adjustColorsForFog = ACFF_MODULATE_RGB;
			}
			// strict blend
			else if ( ( blendSrcBits == GLS_SRCBLEND_SRC_ALPHA ) && ( blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA ) )
			{
				pStage->adjustColorsForFog = ACFF_MODULATE_ALPHA;
			}
			// premultiplied alpha
			else if ( ( blendSrcBits == GLS_SRCBLEND_ONE ) && ( blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA ) )
			{
				pStage->adjustColorsForFog = ACFF_MODULATE_RGBA;
			} else {
				// we can't adjust this one correctly, so it won't be exactly correct in fog
			}

			// don't screw with sort order if this is a portal or environment
			if ( !shader.sort ) {
				// see through item, like a grill or grate
				if ( pStage->stateBits & GLS_DEPTHMASK_TRUE ) {
					shader.sort = SS_SEE_THROUGH;
				} else {
					shader.sort = SS_BLEND0;
				}
			}
		}
		
		stage++;
	}

	// there are times when you will need to manually apply a sort to
	// opaque alpha tested shaders that have later blend passes
	if ( !shader.sort ) {
		shader.sort = SS_OPAQUE;
	}

	//
	// if we are in r_vertexLight mode, never use a lightmap texture
	//
	if ( stage > 1 && ( (r_vertexLight->integer && !r_uiFullScreen->integer) || glConfig.hardwareType == GLHW_PERMEDIA2 ) ) {
		VertexLightingCollapse();
		stage = 1;
		hasLightmapStage = qfalse;
	}

	//
	// look for multitexture potential
	//
	if (!r_leifx->integer)
	if ( stage > 1 && CollapseMultitexture() ) {
		stage--;
	}


	if ( shader.lightmapIndex >= 0 && !hasLightmapStage ) {
		if (vertexLightmap) {
			ri.Printf( PRINT_DEVELOPER, "WARNING: shader '%s' has VERTEX forced lightmap!\n", shader.name );
		} else {
			ri.Printf( PRINT_DEVELOPER, "WARNING: shader '%s' has lightmap but no lightmap stage!\n", shader.name );
  			shader.lightmapIndex = LIGHTMAP_NONE;
		}
	}



	//
	// compute number of passes
	//
	shader.numUnfoggedPasses = stage;

	// fogonly shaders don't have any normal passes
	if (stage == 0 && !shader.isSky)
		shader.sort = SS_FOG;

	// determine which stage iterator function is appropriate
	ComputeStageIteratorFunc();

	return GeneratePermanentShader();
}

//========================================================================================

/*
====================
FindShaderInShaderText

Scans the combined text description of all the shader files for
the given shader name.

return NULL if not found

If found, it will return a valid shader
=====================
*/
static char *FindShaderInShaderText( const char *shadername ) {

	char *token, *p;

	int i, hash;

	hash = generateHashValue(shadername, MAX_SHADERTEXT_HASH);

	if(shaderTextHashTable[hash])
	{
		for (i = 0; shaderTextHashTable[hash][i]; i++)
		{
			p = shaderTextHashTable[hash][i];
			token = COM_ParseExt(&p, qtrue);
		
			if(!Q_stricmp(token, shadername))
				return p;
		}
	}

	p = s_shaderText;

	if ( !p ) {
		return NULL;
	}

	// look for label
	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		if ( !Q_stricmp( token, shadername ) ) {
			return p;
		}
		else {
			// skip the definition
			SkipBracedSection( &p, 0 );
		}
	}

	return NULL;
}


/*
==================
R_FindShaderByName

Will always return a valid shader, but it might be the
default shader if the real one can't be found.
==================
*/
shader_t *R_FindShaderByName( const char *name ) {
	char		strippedName[MAX_QPATH];
	int			hash;
	shader_t	*sh;

	if ( (name==NULL) || (name[0] == 0) ) {

		return tr.defaultShader;
	}

	COM_StripExtension(name, strippedName, sizeof(strippedName));

	hash = generateHashValue(strippedName, FILE_HASH_SIZE);

	//
	// see if the shader is already loaded
	//
	for (sh=hashTable[hash]; sh; sh=sh->next) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if (Q_stricmp(sh->name, strippedName) == 0) {
			// match found
			return sh;
		}
	}

	
	return tr.defaultShader;
}


/*
===============
R_FindShader

Will always return a valid shader, but it might be the
default shader if the real one can't be found.

In the interest of not requiring an explicit shader text entry to
be defined for every single image used in the game, three default
shader behaviors can be auto-created for any image:

If lightmapIndex == LIGHTMAP_NONE, then the image will have
dynamic diffuse lighting applied to it, as apropriate for most
entity skin surfaces.

If lightmapIndex == LIGHTMAP_2D, then the image will be used
for 2D rendering unless an explicit shader is found

If lightmapIndex == LIGHTMAP_BY_VERTEX, then the image will use
the vertex rgba modulate values, as apropriate for misc_model
pre-lit surfaces.

Other lightmapIndex values will have a lightmap stage created
and src*dest blending applied with the texture, as apropriate for
most world construction surfaces.


Leilei TODO - and if it has a lightmapindex and no detail texture please
try to generate a generic detail stage for the sake of consistency with
the other textures with details so there's one big grainy world
 (only if r_detailTextures 3)

TODO:	if r_detailTextures 2, try to move the detail texture before the lightmap
		stage and use filterblend , so we can use the multitexturing 
		pass performance advantage

===============
*/

static int sugthem;

shader_t *R_FindShaderReal( const char *name, int lightmapIndex, qboolean mipRawImage ) {
	char		strippedName[MAX_QPATH];
	int			i, hash;
	char		*shaderText;
	image_t		*image;
	shader_t	*sh;

	//
	// LEILEI's DETAIL TEXTURE STUFFS
	//
	int material;	// leilei - for picking detail texture
	qboolean shouldIDetail = qfalse; // leilei - checking if I should detail.
	int wi, hi; // leilei - for determining detail texture size by uploaded texture
	int detailScale; // leilei - detail scale hack
	int detailLayer; // leilei - detail layer hack
	// leilei - for adjusting detail textures
		if ( r_detailTextureScale->integer > 0)
			detailScale = r_detailTextureScale->integer;
		else
			detailScale = 8;

		if ( r_detailTextureLayers->integer > 0)
			detailLayer = r_detailTextureLayers->integer;
		else
			detailLayer = 1; // one usual layer

		if (detailLayer > 6) detailLayer = 6; // limit 6 for now
	//
	//		
	//


	if ( name[0] == 0 ) {
		return tr.defaultShader;
	}

	// use (fullbright) vertex lighting if the bsp file doesn't have
	// lightmaps
	if ( lightmapIndex >= 0 && lightmapIndex >= tr.numLightmaps ) {
		lightmapIndex = LIGHTMAP_BY_VERTEX;
	} else if ( lightmapIndex < LIGHTMAP_2D ) {
		// negative lightmap indexes cause stray pointers (think tr.lightmaps[lightmapIndex])
		ri.Printf( PRINT_WARNING, "WARNING: shader '%s' has invalid lightmap index of %d\n", name, lightmapIndex  );
		lightmapIndex = LIGHTMAP_BY_VERTEX;
	}

	COM_StripExtension(name, strippedName, sizeof(strippedName));

	hash = generateHashValue(strippedName, FILE_HASH_SIZE);

	//
	// see if the shader is already loaded
	//
	for (sh = hashTable[hash]; sh; sh = sh->next) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ( (sh->lightmapIndex == lightmapIndex || sh->defaultShader) &&
		     !Q_stricmp(sh->name, strippedName)) {
			// match found
			return sh;
		}
	}

	// clear the global shader
	Com_Memset( &shader, 0, sizeof( shader ) );
	Com_Memset( &stages, 0, sizeof( stages ) );
	Q_strncpyz(shader.name, strippedName, sizeof(shader.name));
	shader.lightmapIndex = lightmapIndex;
	for ( i = 0 ; i < MAX_SHADER_STAGES ; i++ ) {
		stages[i].bundle[0].texMods = texMods[i];
	}

	// FIXME: set these "need" values apropriately
	shader.needsNormal = qtrue;
	shader.needsST1 = qtrue;
	shader.needsST2 = qtrue;
	shader.needsColor = qtrue;


	//
	// attempt to define shader from an explicit parameter file
	//
	shaderText = FindShaderInShaderText( strippedName );
	if ( shaderText ) {
		// enable this when building a pak file to get a global list
		// of all explicit shaders
		if ( r_printShaders->integer ) {
			ri.Printf( PRINT_ALL, "*SHADER* %s\n", name );
		}

		if ( !ParseShader( &shaderText ) ) {
			// had errors, so use default shader
			shader.defaultShader = qtrue;
		}

		if (shader.surfaceFlags | SURF_METALSTEPS) {
			material = 1;
		}


		// leilei -  SUPER detail hack to existing shaders,very aggressive and won't look good on 100% of shaders
		if((shader.lightmapIndex != LIGHTMAP_WHITEIMAGE && shader.lightmapIndex != LIGHTMAP_BY_VERTEX && shader.lightmapIndex != LIGHTMAP_2D && shader.lightmapIndex != LIGHTMAP_NONE) ){
			if (r_detailTextures->integer) {
				image_t *imageDetail = NULL;  // for snagging it into more layers if we find a defined one
				int e = 0;
				int f = 0;
				qboolean gotdetailalready = qfalse;
				qboolean hasaDetailImage = qfalse;
				int thisstage = 0;
				material = 0;
				wi = hi = 32; // reset to none....

				shouldIDetail = qfalse; //yeah
				for (f=0;f<detailLayer;f++) {
					for (e=0;e<(MAX_SHADER_STAGES-1);e++) {
						if (shader.defaultShader)
							break; // DON'T! This fixes a crash, trying to stage up placeholder/default textures

						// Pick the first free stage to do, hopefully the last
						if (stages[e].active == qfalse){
							thisstage = e;
							shouldIDetail = qtrue;
							break;
						}

						// find detail texture scale by determining which of the stages have the largest image
						if (stages[e].bundle[0].image[0]->uploadHeight > wi) wi = stages[e].bundle[0].image[0]->uploadWidth;
						if (stages[e].bundle[0].image[0]->uploadHeight > hi) hi = stages[e].bundle[0].image[0]->uploadHeight;

						// for adjusting the detail textures and skipping some redundancy
						if (stages[e].isDetail){ 

							if (f < 1) { 
								gotdetailalready = qtrue; 
								shouldIDetail = qfalse;
								imageDetail = stages[e].bundle[0].image[0]; // this is it
								hasaDetailImage = qtrue;
							}
							if (r_detailTextureScale->integer) {
								if (stages[e].bundle[0].texMods[0].type == TMOD_SCALE) {
									wi = 0.25 * wi / (detailScale / (f + 1));
									hi = 0.25 * hi / (detailScale / (f + 1));
									stages[e].bundle[0].texMods[0].scale[0] = wi;
									stages[e].bundle[0].texMods[0].scale[1] = hi;
								}
							}
						}						

					}
					if (r_detailTextures->integer < 3) {
						// don't add detail for low settings
						shouldIDetail = qfalse;
					}

					if ((!gotdetailalready || thisstage) && (shouldIDetail)) {
						// detail it up because i don't care.
						wi = 0.25  * (f + 1)  * wi / detailScale;
						hi = 0.25  * (f + 1)  * hi / detailScale;

						detailhack = 1;
						if (material == 1) {
							// metalsteps
							stages[thisstage].bundle[0].image[0] = R_FindImageFile( "gfx/fx/detail/d_genericmetal.tga", IMGTYPE_NORMAL , IMGFLAG_MIPMAP);
						}
						else if (hasaDetailImage && imageDetail) {
							stages[thisstage].bundle[0].image[0] = imageDetail;
						}
						else {
							stages[thisstage].bundle[0].image[0] = R_FindImageFile( "gfx/fx/detail/d_generic.tga", IMGTYPE_NORMAL , IMGFLAG_MIPMAP);
						}
						stages[thisstage].active = qtrue;
						stages[thisstage].rgbGen = CGEN_IDENTITY;
						stages[thisstage].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_SRC_COLOR | GLS_DEPTHFUNC_EQUAL;
						stages[thisstage].bundle[0].texMods[0].scale[0] = wi;
						stages[thisstage].bundle[0].texMods[0].scale[1] = hi;
						stages[thisstage].bundle[0].texMods[0].type = TMOD_SCALE;
						stages[thisstage].isDetail = qtrue;
						stages[thisstage].bundle[0].numTexMods = 1;
						detailhack = 0;
					}
				}

			}
		}
		sh = FinishShader();
		return sh;
	}


	//
	// if not defined in the in-memory shader descriptions,
	// look for a single supported image file
	//
	{
		imgFlags_t flags;

		flags = IMGFLAG_NONE;

		if (mipRawImage)
		{
			flags |= IMGFLAG_MIPMAP | IMGFLAG_PICMIP;
		}
		else
		{
			flags |= IMGFLAG_CLAMPTOEDGE;
		}

		image = R_FindImageFile( name, IMGTYPE_COLORALPHA, flags );
		if ( !image ) {
			ri.Printf( PRINT_DEVELOPER, "Couldn't find image file for shader %s\n", name );
			shader.defaultShader = qtrue;
			return FinishShader();
		}
	}



	//
	// create the default shading commands
	//

	if ( shader.lightmapIndex == LIGHTMAP_NONE ) {
		// dynamic colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex == LIGHTMAP_BY_VERTEX ) {
		// explicit colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_VERTEX_LIT;// leilei - dynamic lights in vertex lighting
		//stages[0].rgbGen = CGEN_EXACT_VERTEX;
		stages[0].alphaGen = AGEN_SKIP;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex == LIGHTMAP_2D ) {
		// GUI elements
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_VERTEX;
		stages[0].alphaGen = AGEN_VERTEX;
		stages[0].stateBits = GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( shader.lightmapIndex == LIGHTMAP_WHITEIMAGE ) {
		// fullbright level
		stages[0].bundle[0].image[0] = tr.whiteImage;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	} else {
		// leilei - automatic detail adding hack
		
		if (r_detailTextures->integer > 2){
		// three pass lightmap with a detail after the texture. slow but is how U did it
		stages[0].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
		stages[0].bundle[0].isLightmap = qtrue;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY;	// lightmaps are scaled on creation
													// for identitylight
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
		// detail
			{
			int f;
				for (f=0;f<detailLayer;f++){
				
				if (f+2 > MAX_SHADER_STAGES) break;// don't exceed limit!
				
			 	stages[2+f].bundle[0].image[0] = R_FindImageFile( "gfx/fx/detail/d_generic.tga", IMGTYPE_NORMAL , IMGFLAG_MIPMAP); // TODO: use metal detail for metal surfaces

			// determine detail size first, our detail textures are typically 128x128
				wi = 0.25 * (f + 1)  * stages[1].bundle[0].image[0]->uploadWidth / detailScale;
				hi = 0.25 * (f + 1) * stages[1].bundle[0].image[0]->uploadHeight / detailScale;

				stages[2+f].active = qtrue;
				stages[2+f].rgbGen = CGEN_IDENTITY;
				stages[2+f].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_SRC_COLOR;

				stages[2+f].bundle[0].texMods[0].scale[0] = wi;
				stages[2+f].bundle[0].texMods[0].scale[1] = hi;

				stages[2+f].bundle[0].texMods[0].type = TMOD_SCALE;
				
				stages[2+f].bundle[0].numTexMods = 1;

				stages[2+f].isDetail = qtrue;
				
				}
			}
		}
		else
		
		{
		// two pass lightmap
		stages[0].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
		stages[0].bundle[0].isLightmap = qtrue;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY;	// lightmaps are scaled on creation
													// for identitylight
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
		}


	// leilei - handle image height

		int y;
		for(y=0;y<MAX_SHADER_STAGES;y++){
	
	
			stages[y].imgWidth = 128;
			stages[y].imgHeight = 128;
	
		}

	}



	return FinishShader();
}

shader_t *R_FindShader( const char *name, int lightmapIndex, qboolean mipRawImage ) {
		shader_t	*sh;		
		shader_t	*ahsh;	

	// load real shader first?
	sh = R_FindShaderReal(name, lightmapIndex, mipRawImage);
	if (!Q_strncmp( name, "models/player", 13) ){	// restrict to players; speedup
		if (r_suggestiveThemes->integer < 1)	// find safe textures/shaders if available
		{
			sugthem = 1;
			ahsh = R_FindShaderReal(va("%s_safe",name), lightmapIndex, mipRawImage);
			if ( ahsh->defaultShader ){
			return sh;
			}
			else
			return ahsh;
	
		}
		else if (r_suggestiveThemes->integer > 1)	// find lewd textures/shaders if available
		{
			sugthem = 2;
			ahsh = R_FindShaderReal(va("%s_lewd",name), lightmapIndex, mipRawImage);
			if ( ahsh->defaultShader ){
			return sh;
			}
			else
			return ahsh;
		}
		else	// if just normally suggestive or an otherwise normal shader, default
		return sh;
	}
	else
	return sh;

}


qhandle_t RE_RegisterShaderFromImage(const char *name, int lightmapIndex, image_t *image, qboolean mipRawImage) {
	int			i, hash;
	shader_t	*sh;

	hash = generateHashValue(name, FILE_HASH_SIZE);

	// probably not necessary since this function
	// only gets called from tr_font.c with lightmapIndex == LIGHTMAP_2D
	// but better safe than sorry.
	if ( lightmapIndex >= tr.numLightmaps ) {
		lightmapIndex = LIGHTMAP_WHITEIMAGE;
	}

	//
	// see if the shader is already loaded
	//
	for (sh=hashTable[hash]; sh; sh=sh->next) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ( (sh->lightmapIndex == lightmapIndex || sh->defaultShader) &&
			// index by name
			!Q_stricmp(sh->name, name)) {
			// match found
			return sh->index;
		}
	}

	// clear the global shader
	Com_Memset( &shader, 0, sizeof( shader ) );
	Com_Memset( &stages, 0, sizeof( stages ) );
	Q_strncpyz(shader.name, name, sizeof(shader.name));
	shader.lightmapIndex = lightmapIndex;
	for ( i = 0 ; i < MAX_SHADER_STAGES ; i++ ) {
		stages[i].bundle[0].texMods = texMods[i];
	}

	// FIXME: set these "need" values apropriately
	shader.needsNormal = qtrue;
	shader.needsST1 = qtrue;
	shader.needsST2 = qtrue;
	shader.needsColor = qtrue;

	//
	// create the default shading commands
	//
	if ( shader.lightmapIndex == LIGHTMAP_NONE ) {
		// dynamic colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex == LIGHTMAP_BY_VERTEX ) {
		// explicit colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_VERTEX_LIT;// leilei - dynamic lights in vertex lighting
		//stages[0].rgbGen = CGEN_EXACT_VERTEX;
		stages[0].alphaGen = AGEN_SKIP;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex == LIGHTMAP_2D ) {
		// GUI elements
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_VERTEX;
		stages[0].alphaGen = AGEN_VERTEX;
		stages[0].stateBits = GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( shader.lightmapIndex == LIGHTMAP_WHITEIMAGE ) {
		// fullbright level
		stages[0].bundle[0].image[0] = tr.whiteImage;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	} else {
		// two pass lightmap
		stages[0].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
		stages[0].bundle[0].isLightmap = qtrue;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY;	// lightmaps are scaled on creation
													// for identitylight
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}

	sh = FinishShader();
  return sh->index; 
}


/* 
====================
RE_RegisterShader

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShaderLightMap( const char *name, int lightmapIndex ) {
	shader_t	*sh;

	if ( strlen( name ) >= MAX_QPATH ) {
		ri.Printf( PRINT_ALL, "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}

	sh = R_FindShader( name, lightmapIndex, qtrue );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}




/* 
====================
RE_RegisterShader

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShader( const char *name ) {
	shader_t	*sh;


	if ( strlen( name ) >= MAX_QPATH ) {
		ri.Printf( PRINT_ALL, "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}


	sh = R_FindShader( name, LIGHTMAP_2D, qtrue );


	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}


/*
====================
RE_RegisterShaderNoMip

For menu graphics that should never be picmiped
====================
*/
qhandle_t RE_RegisterShaderNoMip( const char *name ) {
	shader_t	*sh;

	if ( strlen( name ) >= MAX_QPATH ) {
		ri.Printf( PRINT_ALL, "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}

	sh = R_FindShader( name, LIGHTMAP_2D, qfalse );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}

/*
====================
R_GetShaderByHandle

When a handle is passed in by another module, this range checks
it and returns a valid (possibly default) shader_t to be used internally.
====================
*/
shader_t *R_GetShaderByHandle( qhandle_t hShader ) {
	if ( hShader < 0 ) {
	  ri.Printf( PRINT_WARNING, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader );
		return tr.defaultShader;
	}
	if ( hShader >= tr.numShaders ) {
		ri.Printf( PRINT_WARNING, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader );
		return tr.defaultShader;
	}
	return tr.shaders[hShader];
}

/*
===============
R_ShaderList_f

Dump information on all valid shaders to the console
A second parameter will cause it to print in sorted order
===============
*/
void	R_ShaderList_f (void) {
	int			i;
	int			count;
	shader_t	*shader;

	ri.Printf (PRINT_ALL, "-----------------------\n");

	count = 0;
	for ( i = 0 ; i < tr.numShaders ; i++ ) {
		if ( ri.Cmd_Argc() > 1 ) {
			shader = tr.sortedShaders[i];
		} else {
			shader = tr.shaders[i];
		}

		ri.Printf( PRINT_ALL, "%i ", shader->numUnfoggedPasses );

		if (shader->lightmapIndex >= 0 ) {
			ri.Printf (PRINT_ALL, "L ");
		} else {
			ri.Printf (PRINT_ALL, "  ");
		}
		if ( shader->multitextureEnv == GL_ADD ) {
			ri.Printf( PRINT_ALL, "MT(a) " );
		} else if ( shader->multitextureEnv == GL_MODULATE ) {
			ri.Printf( PRINT_ALL, "MT(m) " );
		} else if ( shader->multitextureEnv == GL_DECAL ) {
			ri.Printf( PRINT_ALL, "MT(d) " );
		} else {
			ri.Printf( PRINT_ALL, "      " );
		}
		if ( shader->explicitlyDefined ) {
			ri.Printf( PRINT_ALL, "E " );
		} else {
			ri.Printf( PRINT_ALL, "  " );
		}

		if ( shader->optimalStageIteratorFunc == RB_StageIteratorGeneric ) {
			ri.Printf( PRINT_ALL, "gen " );
		} else if ( shader->optimalStageIteratorFunc == RB_StageIteratorSky ) {
			ri.Printf( PRINT_ALL, "sky " );
		} else if ( shader->optimalStageIteratorFunc == RB_StageIteratorLightmappedMultitexture ) {
			ri.Printf( PRINT_ALL, "lmmt" );
		} else if ( shader->optimalStageIteratorFunc == RB_StageIteratorVertexLitTexture ) {
			ri.Printf( PRINT_ALL, "vlt " );
		} else {
			ri.Printf( PRINT_ALL, "    " );
		}

		if ( shader->defaultShader ) {
			ri.Printf (PRINT_ALL,  ": %s (DEFAULTED)\n", shader->name);
		} else {
			ri.Printf (PRINT_ALL,  ": %s\n", shader->name);
		}
		count++;
	}
	ri.Printf (PRINT_ALL, "%i total shaders\n", count);
	ri.Printf (PRINT_ALL, "------------------\n");
}

/*
====================
ScanAndLoadShaderFiles

Finds and loads all .shader files, combining them into
a single large text block that can be scanned for shader names
=====================
*/
#define	MAX_SHADER_FILES	4096

static void ScanAndLoadShaderFiles( void )
{
	char **shaderFiles;

	char *buffers[MAX_SHADER_FILES];
	char *p;
	int numShaderFiles;
	int i;
	char *oldp, *token, *hashMem, *textEnd;
	int shaderTextHashTableSizes[MAX_SHADERTEXT_HASH], hash, size;
	char shaderName[MAX_QPATH];
	int shaderLine;

	long sum = 0, summand;
	// scan for shader files
	shaderFiles = ri.FS_ListFiles( "scripts", ".shader", &numShaderFiles );

	if ( !shaderFiles || !numShaderFiles )
	{
		ri.Printf( PRINT_WARNING, "WARNING: no shader files found\n" );
		return;
	}

	// load and parse shader files
	for ( i = 0; i < numShaderFiles; i++ )
	{
		char filename[MAX_QPATH];

		Com_sprintf( filename, sizeof( filename ), "scripts/%s", shaderFiles[i] );
		ri.Printf( PRINT_DEVELOPER, "...loading '%s'\n", filename );
		summand = ri.FS_ReadFile( filename, (void **)&buffers[i] );
		
		if ( !buffers[i] )
			ri.Error( ERR_DROP, "Couldn't load %s", filename );

		// Do a simple check on the shader structure in that file to make sure one bad shader file cannot fuck up all other shaders.
		p = buffers[i];
		COM_BeginParseSession(filename);
		while(1)
		{
			token = COM_ParseExt(&p, qtrue);
			
			if(!*token)
				break;

			Q_strncpyz(shaderName, token, sizeof(shaderName));
			shaderLine = COM_GetCurrentParseLine();

			token = COM_ParseExt(&p, qtrue);
			if(token[0] != '{' || token[1] != '\0')
			{
				ri.Printf(PRINT_WARNING, "WARNING: Ignoring shader file %s. Shader \"%s\" on line %d missing opening brace",
							filename, shaderName, shaderLine);
				if (token[0])
				{
					ri.Printf(PRINT_WARNING, " (found \"%s\" on line %d)", token, COM_GetCurrentParseLine());
				}
				ri.Printf(PRINT_WARNING, ".\n");
				ri.FS_FreeFile(buffers[i]);
				buffers[i] = NULL;
				break;
			}

			if(!SkipBracedSection(&p, 1))
			{
				ri.Printf(PRINT_WARNING, "WARNING: Ignoring shader file %s. Shader \"%s\" on line %d missing closing brace.\n",
							filename, shaderName, shaderLine);
				ri.FS_FreeFile(buffers[i]);
				buffers[i] = NULL;
				break;
			}
		}
			
		
		if (buffers[i])
			sum += summand;		
	}
	if ( numShaderFiles > MAX_SHADER_FILES ) {
		numShaderFiles = MAX_SHADER_FILES;
	}

	// build single large buffer
	s_shaderText = ri.Hunk_Alloc( sum + (numShaderFiles*2), h_low );
	s_shaderText[ 0 ] = '\0';
	textEnd = s_shaderText;
 
	// free in reverse order, so the temp files are all dumped
	for ( i = numShaderFiles - 1; i >= 0 ; i-- )
	{
		if ( !buffers[i] )
			continue;

		strcat( textEnd, buffers[i] );
		strcat( textEnd, "\n" );
		textEnd += strlen( textEnd );
		ri.FS_FreeFile( buffers[i] );
	}

	COM_Compress( s_shaderText );

	// free up memory
	ri.FS_FreeFileList( shaderFiles );

	Com_Memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));
	size = 0;

	p = s_shaderText;
	// look for shader names
	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		hash = generateHashValue(token, MAX_SHADERTEXT_HASH);
		shaderTextHashTableSizes[hash]++;
		size++;
		SkipBracedSection(&p, 0);
	}

	size += MAX_SHADERTEXT_HASH;

	hashMem = ri.Hunk_Alloc( size * sizeof(char *), h_low );

	for (i = 0; i < MAX_SHADERTEXT_HASH; i++) {
		shaderTextHashTable[i] = (char **) hashMem;
		hashMem = ((char *) hashMem) + ((shaderTextHashTableSizes[i] + 1) * sizeof(char *));
	}

	Com_Memset(shaderTextHashTableSizes, 0, sizeof(shaderTextHashTableSizes));

	p = s_shaderText;
	// look for shader names
	while ( 1 ) {
		oldp = p;
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		hash = generateHashValue(token, MAX_SHADERTEXT_HASH);
		shaderTextHashTable[hash][shaderTextHashTableSizes[hash]++] = oldp;

		SkipBracedSection(&p, 0);
	}

	return;

}


/*
====================
CreateInternalShaders
====================
*/
static void CreateInternalShaders( void ) {
	tr.numShaders = 0;

	// init the default shader
	Com_Memset( &shader, 0, sizeof( shader ) );
	Com_Memset( &stages, 0, sizeof( stages ) );

	Q_strncpyz( shader.name, "<default>", sizeof( shader.name ) );

	shader.lightmapIndex = LIGHTMAP_NONE;
	stages[0].bundle[0].image[0] = tr.defaultImage;
	stages[0].active = qtrue;
	stages[0].stateBits = GLS_DEFAULT;
	tr.defaultShader = FinishShader();

	// shadow shader is just a marker
	Q_strncpyz( shader.name, "<stencil shadow>", sizeof( shader.name ) );
	shader.sort = SS_STENCIL_SHADOW;
	tr.shadowShader = FinishShader();

	// leilei - cone shader for flashblend
	Q_strncpyz( shader.name, "<cone add>", sizeof( shader.name ) );
	stages[0].bundle[0].image[0] = tr.whiteImage;
	stages[0].active = qtrue;
	stages[0].rgbGen = CGEN_VERTEX;
	stages[0].alphaGen = AGEN_VERTEX;
 	stages[0].stateBits = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
	tr.coneShader = FinishShader();
}

static void CreateExternalShaders( void ) {

	// leilei - placeholder shaders
	tr.placeholderTextureAvail = 0;
	tr.placeholderModelAvail = 0;
	tr.placeholderAvail = 0;
	tr.placeholderSkyAvail = 0;
	tr.placeholderWaterAvail = 0;
	tr.placeholderLavaAvail = 0;
	tr.placeholderSlimeAvail = 0;
	tr.placeholderFogAvail = 0;




	tr.projectionShadowShader = R_FindShader( "projectionShadow", LIGHTMAP_NONE, qtrue );
	tr.flareShader = R_FindShader( "flareShader", LIGHTMAP_NONE, qtrue );
	tr.flareShaderAtlas = R_FindShader( "flareShaderAtlas", LIGHTMAP_NONE, qtrue );	// leilei - lens reflection

	// Hack to make fogging work correctly on flares. Fog colors are calculated
	// in tr_flare.c already.
	if(!tr.flareShader->defaultShader)
	{
		int index;
		
		for(index = 0; index < tr.flareShader->numUnfoggedPasses; index++)
		{
			tr.flareShader->stages[index]->adjustColorsForFog = ACFF_NONE;
			tr.flareShader->stages[index]->stateBits |= GLS_DEPTHTEST_DISABLE;
		}
	}


	if(!tr.flareShaderAtlas->defaultShader)
	{
		int index;
		
		for(index = 0; index < tr.flareShader->numUnfoggedPasses; index++)
		{
			tr.flareShaderAtlas->stages[index]->adjustColorsForFog = ACFF_NONE;
			tr.flareShaderAtlas->stages[index]->stateBits |= GLS_DEPTHTEST_DISABLE;
		}
	}

//	leilei - placeholder shaders
	tr.placeholderTextureShader = R_FindShader( "placeholder_texture", LIGHTMAP_NONE, qtrue );
	tr.placeholderModelShader = R_FindShader( "placeholder_model", LIGHTMAP_NONE, qtrue );
	tr.placeholderSkyShader = R_FindShader( "placeholder_sky_retro", LIGHTMAP_NONE, qtrue );
	tr.placeholderWaterShader = R_FindShader( "placeholder_water", LIGHTMAP_NONE, qtrue );
	tr.placeholderSlimeShader = R_FindShader( "placeholder_slime", LIGHTMAP_NONE, qtrue );
	tr.placeholderLavaShader = R_FindShader( "placeholder_lava", LIGHTMAP_NONE, qtrue );
	tr.placeholderFogShader = R_FindShader( "placeholder_fog", LIGHTMAP_NONE, qtrue );
	tr.placeholderShader = R_FindShader( "placeholder_model", LIGHTMAP_NONE, qtrue );

	if(!tr.placeholderTextureShader->defaultShader) 	tr.placeholderTextureAvail = 1;
	if(!tr.placeholderModelShader->defaultShader) 		tr.placeholderModelAvail = 1;
	if(!tr.placeholderShader->defaultShader) 		tr.placeholderAvail = 1;
	if(!tr.placeholderSkyShader->defaultShader) 		tr.placeholderSkyAvail = 1;
	if(!tr.placeholderWaterShader->defaultShader) 		tr.placeholderWaterAvail = 1;
	if(!tr.placeholderLavaShader->defaultShader) 		tr.placeholderLavaAvail = 1;
	if(!tr.placeholderSlimeShader->defaultShader) 		tr.placeholderSlimeAvail = 1;
	if(!tr.placeholderFogShader->defaultShader) 		tr.placeholderFogAvail = 1;


// leilei - lfx shaders

	LFX_ShaderInit();
}

/*
==================
R_InitShaders
==================
*/
void R_InitShaders( void ) {
	ri.Printf( PRINT_ALL, "Initializing Shaders\n" );

	Com_Memset(hashTable, 0, sizeof(hashTable));

	CreateInternalShaders();

	ScanAndLoadShaderFiles();

	CreateExternalShaders();
}
