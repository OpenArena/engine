/** Idea and structure from James Canete's opengl2 renderer but the code is
    based on OpenArena 0.8.8's sdl_glimp.c changes. */

// tr_extensions.c - extensions needed by the renderer not in sdl_glimp.c
#ifdef USE_LOCAL_HEADERS
#  include "SDL.h"
#else
#  include <SDL.h>
#endif

#include "tr_local.h"

#ifdef _WIN32
BOOL  ( WINAPI * qwglGetDeviceGammaRamp3DFX)( HDC, LPVOID );		// leilei - 3dfx gamma fix
BOOL  ( WINAPI * qwglSetDeviceGammaRamp3DFX)( HDC, LPVOID );
#endif

// GL_ARB_shader_objects
GLvoid (APIENTRYP qglDeleteObjectARB) (GLhandleARB obj);
GLhandleARB (APIENTRYP qglGetHandleARB) (GLenum pname);
GLvoid (APIENTRYP qglDetachObjectARB) (GLhandleARB containerObj, GLhandleARB attachedObj);
GLhandleARB (APIENTRYP qglCreateShaderObjectARB) (GLenum shaderType);
GLvoid (APIENTRYP qglShaderSourceARB) (GLhandleARB shaderObj, GLsizei count, const GLcharARB **string,
				       const GLint *length);
GLvoid (APIENTRYP qglCompileShaderARB) (GLhandleARB shaderObj);
GLhandleARB (APIENTRYP qglCreateProgramObjectARB) (void);
GLvoid (APIENTRYP qglAttachObjectARB) (GLhandleARB containerObj, GLhandleARB obj);
GLvoid (APIENTRYP qglLinkProgramARB) (GLhandleARB programObj);
GLvoid (APIENTRYP qglUseProgramObjectARB) (GLhandleARB programObj);
GLvoid (APIENTRYP qglValidateProgramARB) (GLhandleARB programObj);
GLvoid (APIENTRYP qglUniform1fARB) (GLint location, GLfloat v0);
GLvoid (APIENTRYP qglUniform2fARB) (GLint location, GLfloat v0, GLfloat v1);
GLvoid (APIENTRYP qglUniform3fARB) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
GLvoid (APIENTRYP qglUniform4fARB) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
GLvoid (APIENTRYP qglUniform1iARB) (GLint location, GLint v0);
GLvoid (APIENTRYP qglUniform2iARB) (GLint location, GLint v0, GLint v1);
GLvoid (APIENTRYP qglUniform3iARB) (GLint location, GLint v0, GLint v1, GLint v2);
GLvoid (APIENTRYP qglUniform4iARB) (GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
GLvoid (APIENTRYP qglUniform1fvARB) (GLint location, GLsizei count, const GLfloat *value);
GLvoid (APIENTRYP qglUniform2fvARB) (GLint location, GLsizei count, const GLfloat *value);
GLvoid (APIENTRYP qglUniform3fvARB) (GLint location, GLsizei count, const GLfloat *value);
GLvoid (APIENTRYP qglUniform4fvARB) (GLint location, GLsizei count, const GLfloat *value);
GLvoid (APIENTRYP qglUniform1ivARB) (GLint location, GLsizei count, const GLint *value);
GLvoid (APIENTRYP qglUniform2ivARB) (GLint location, GLsizei count, const GLint *value);
GLvoid (APIENTRYP qglUniform3ivARB) (GLint location, GLsizei count, const GLint *value);
GLvoid (APIENTRYP qglUniform4ivARB) (GLint location, GLsizei count, const GLint *value);
GLvoid (APIENTRYP qglUniformMatrix2fvARB) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GLvoid (APIENTRYP qglUniformMatrix3fvARB) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GLvoid (APIENTRYP qglUniformMatrix4fvARB) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
GLvoid (APIENTRYP qglGetObjectParameterfvARB) (GLhandleARB obj, GLenum pname, GLfloat *params);
GLvoid (APIENTRYP qglGetObjectParameterivARB) (GLhandleARB obj, GLenum pname, GLint *params);
GLvoid (APIENTRYP qglGetInfoLogARB) (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
GLvoid (APIENTRYP qglGetAttachedObjectsARB) (GLhandleARB containerObj, GLsizei maxCount, GLsizei *count,
					     GLhandleARB *obj);
GLint(APIENTRYP qglGetUniformLocationARB) (GLhandleARB programObj, const GLcharARB * name);
GLvoid (APIENTRYP qglGetActiveUniformARB) (GLhandleARB programObj, GLuint index, GLsizei maxLength,
					   GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
GLvoid (APIENTRYP qglGetUniformfvARB) (GLhandleARB programObj, GLint location, GLfloat *params);
GLvoid (APIENTRYP qglGetUniformivARB) (GLhandleARB programObj, GLint location, GLint *params);
GLvoid (APIENTRYP qglGetShaderSourceARB) (GLhandleARB obj, GLsizei maxLength, GLsizei *length,
					  GLcharARB *source);
GLvoid (APIENTRYP qglVertexAttrib1fARB) (GLuint index, GLfloat v0);
GLvoid (APIENTRYP qglVertexAttrib1sARB) (GLuint index, GLshort v0);
GLvoid (APIENTRYP qglVertexAttrib1dARB) (GLuint index, GLdouble v0);
GLvoid (APIENTRYP qglVertexAttrib2fARB) (GLuint index, GLfloat v0, GLfloat v1);
GLvoid (APIENTRYP qglVertexAttrib2sARB) (GLuint index, GLshort v0, GLshort v1);
GLvoid (APIENTRYP qglVertexAttrib2dARB) (GLuint index, GLdouble v0, GLdouble v1);
GLvoid (APIENTRYP qglVertexAttrib3fARB) (GLuint index, GLfloat v0, GLfloat v1, GLfloat v2);
GLvoid (APIENTRYP qglVertexAttrib3sARB) (GLuint index, GLshort v0, GLshort v1, GLshort v2);
GLvoid (APIENTRYP qglVertexAttrib3dARB) (GLuint index, GLdouble v0, GLdouble v1, GLdouble v2);
GLvoid (APIENTRYP qglVertexAttrib4fARB) (GLuint index, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
GLvoid (APIENTRYP qglVertexAttrib4sARB) (GLuint index, GLshort v0, GLshort v1, GLshort v2, GLshort v3);
GLvoid (APIENTRYP qglVertexAttrib4dARB) (GLuint index, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3);
GLvoid (APIENTRYP qglVertexAttrib4NubARB) (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
GLvoid (APIENTRYP qglVertexAttrib1fvARB) (GLuint index, GLfloat *v);
GLvoid (APIENTRYP qglVertexAttrib1svARB) (GLuint index, GLshort *v);
GLvoid (APIENTRYP qglVertexAttrib1dvARB) (GLuint index, GLdouble *v);
GLvoid (APIENTRYP qglVertexAttrib2fvARB) (GLuint index, GLfloat *v);
GLvoid (APIENTRYP qglVertexAttrib2svARB) (GLuint index, GLshort *v);
GLvoid (APIENTRYP qglVertexAttrib2dvARB) (GLuint index, GLdouble *v);
GLvoid (APIENTRYP qglVertexAttrib3fvARB) (GLuint index, GLfloat *v);
GLvoid (APIENTRYP qglVertexAttrib3svARB) (GLuint index, GLshort *v);
GLvoid (APIENTRYP qglVertexAttrib3dvARB) (GLuint index, GLdouble *v);
GLvoid (APIENTRYP qglVertexAttrib4fvARB) (GLuint index, const GLfloat *v);
GLvoid (APIENTRYP qglVertexAttrib4svARB) (GLuint index, GLshort *v);
GLvoid (APIENTRYP qglVertexAttrib4dvARB) (GLuint index, GLdouble *v);
GLvoid (APIENTRYP qglVertexAttrib4ivARB) (GLuint index, GLint *v);
GLvoid (APIENTRYP qglVertexAttrib4bvARB) (GLuint index, GLbyte *v);
GLvoid (APIENTRYP qglVertexAttrib4ubvARB) (GLuint index, GLubyte *v);
GLvoid (APIENTRYP qglVertexAttrib4usvARB) (GLuint index, GLushort *v);
GLvoid (APIENTRYP qglVertexAttrib4uivARB) (GLuint index, GLuint *v);
GLvoid (APIENTRYP qglVertexAttrib4NbvARB) (GLuint index, const GLbyte *v);
GLvoid (APIENTRYP qglVertexAttrib4NsvARB) (GLuint index, const GLshort *v);
GLvoid (APIENTRYP qglVertexAttrib4NivARB) (GLuint index, const GLint *v);
GLvoid (APIENTRYP qglVertexAttrib4NubvARB) (GLuint index, const GLubyte *v);
GLvoid (APIENTRYP qglVertexAttrib4NusvARB) (GLuint index, const GLushort *v);
GLvoid (APIENTRYP qglVertexAttrib4NuivARB) (GLuint index, const GLuint *v);
GLvoid (APIENTRYP qglVertexAttribPointerARB) (GLuint index, GLint size, GLenum type, GLboolean normalized,
					      GLsizei stride, const GLvoid *pointer);
GLvoid (APIENTRYP qglEnableVertexAttribArrayARB) (GLuint index);
GLvoid (APIENTRYP qglDisableVertexAttribArrayARB) (GLuint index);
GLvoid (APIENTRYP qglBindAttribLocationARB) (GLhandleARB programObj, GLuint index, const GLcharARB *name);
GLvoid (APIENTRYP qglGetActiveAttribARB) (GLhandleARB programObj, GLuint index, GLsizei maxLength,
					  GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
GLint(APIENTRYP qglGetAttribLocationARB) (GLhandleARB programObj, const GLcharARB * name);
GLvoid (APIENTRYP qglGetVertexAttribdvARB) (GLuint index, GLenum pname, GLdouble *params);
GLvoid (APIENTRYP qglGetVertexAttribfvARB) (GLuint index, GLenum pname, GLfloat *params);
GLvoid (APIENTRYP qglGetVertexAttribivARB) (GLuint index, GLenum pname, GLint *params);
GLvoid (APIENTRYP qglGetVertexAttribPointervARB) (GLuint index, GLenum pname, GLvoid **pointer);

// leilei - paletted texture
GLvoid (APIENTRYP qglColorTableEXT)( GLint, GLint, GLint, GLint, GLint, const GLvoid *);
GLvoid (APIENTRYP qglColorTableSGI)( GLint, GLint, GLint, GLint, GLint, const GLvoid *);

// leilei - DDS/DXT

GLvoid (APIENTRYP qglCompressedTexImage2DARB) (GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *);
GLvoid (APIENTRYP qglColorTableEXT)( GLint, GLint, GLint, GLint, GLint, const GLvoid *);
GLvoid (APIENTRYP qglColorTableSGI)( GLint, GLint, GLint, GLint, GLint, const GLvoid *);

/*
typedef enum {
	GLHW_GENERIC,			// where everthing works the way it should
	GLHW_3DFX_2D3D,			// Voodoo Banshee or Voodoo3, relevant since if this is
							// the hardware type then there can NOT exist a secondary
							// display adapter
	GLHW_RIVA128,			// where you can't interpolate alpha
	GLHW_RAGEPRO,			// where you can't modulate alpha on alpha textures
	GLHW_PERMEDIA2			// where you don't have src*dst
} glHardwareType2_t;
*/

int	softwaremode;	// leilei - detect for software mode
int	voodootype;	// leilei - detect for voodoo revision
char	extensions_string_full [BIG_INFO_STRING]; // leilei - gl extensions crash workaround

/** From renderer_opengl2 (v28) */
static qboolean GLimp_HaveExtension(const char *ext)
{
	const char *ptr = Q_stristr( glConfig.extensions_string, ext );
	if (ptr == NULL)
		return qfalse;
	ptr += strlen(ext);
	return ((*ptr == ' ') || (*ptr == '\0'));  // verify it's complete string.
}

void GLimp_InitExtraExtensions()
{
	vertexShaders = qfalse;
#ifdef GLSL_BACKEND
	if ( GLimp_HaveExtension( "GL_ARB_shader_objects" )
	     && GLimp_HaveExtension( "GL_ARB_fragment_shader" )
	     && GLimp_HaveExtension( "GL_ARB_vertex_shader" )
	     && GLimp_HaveExtension( "GL_ARB_shading_language_100" ) )
	{
		if ( r_ext_vertex_shader->integer ) {
		  ri.Printf( PRINT_ALL, "...using GL_ARB_vertex_shader\n" );

		  qglDeleteObjectARB = (GLvoid (APIENTRYP)(GLhandleARB)) SDL_GL_GetProcAddress("glDeleteObjectARB");
		  qglGetHandleARB = (GLhandleARB (APIENTRYP)(GLenum)) SDL_GL_GetProcAddress("glGetHandleARB");
		  qglDetachObjectARB = (GLvoid (APIENTRYP)(GLhandleARB, GLhandleARB)) SDL_GL_GetProcAddress("glDetachObjectARB");
		  qglCreateShaderObjectARB = (GLhandleARB (APIENTRYP)(GLenum)) SDL_GL_GetProcAddress("glCreateShaderObjectARB");
		  qglShaderSourceARB = (GLvoid (APIENTRYP)(GLhandleARB, GLsizei, const GLcharARB **,
							   const GLint *)) SDL_GL_GetProcAddress("glShaderSourceARB");
		  qglCompileShaderARB = (GLvoid (APIENTRYP)(GLhandleARB)) SDL_GL_GetProcAddress("glCompileShaderARB");
		  qglCreateProgramObjectARB = (GLhandleARB (APIENTRYP)(void)) SDL_GL_GetProcAddress("glCreateProgramObjectARB");
		  qglAttachObjectARB = (GLvoid (APIENTRYP)(GLhandleARB, GLhandleARB)) SDL_GL_GetProcAddress("glAttachObjectARB");
		  qglLinkProgramARB = (GLvoid (APIENTRYP)(GLhandleARB)) SDL_GL_GetProcAddress("glLinkProgramARB");
		  qglUseProgramObjectARB = (GLvoid (APIENTRYP)(GLhandleARB)) SDL_GL_GetProcAddress("glUseProgramObjectARB");
		  qglValidateProgramARB = (GLvoid (APIENTRYP)(GLhandleARB)) SDL_GL_GetProcAddress("glValidateProgramARB");
		  qglUniform1fARB = (GLvoid (APIENTRYP)(GLint, GLfloat)) SDL_GL_GetProcAddress("glUniform1fARB");
		  qglUniform2fARB = (GLvoid (APIENTRYP)(GLint, GLfloat, GLfloat)) SDL_GL_GetProcAddress("glUniform2fARB");
		  qglUniform3fARB = (GLvoid (APIENTRYP)(GLint, GLfloat, GLfloat, GLfloat)) SDL_GL_GetProcAddress("glUniform3fARB");
		  qglUniform4fARB = (GLvoid (APIENTRYP)(GLint, GLfloat, GLfloat, GLfloat, GLfloat)) SDL_GL_GetProcAddress("glUniform4fARB");
		  qglUniform1iARB = (GLvoid (APIENTRYP)(GLint, GLint)) SDL_GL_GetProcAddress("glUniform1iARB");
		  qglUniform2iARB = (GLvoid (APIENTRYP)(GLint, GLint, GLint)) SDL_GL_GetProcAddress("glUniform2iARB");
		  qglUniform3iARB = (GLvoid (APIENTRYP)(GLint, GLint, GLint, GLint)) SDL_GL_GetProcAddress("glUniform3iARB");
		  qglUniform4iARB = (GLvoid (APIENTRYP)(GLint, GLint, GLint, GLint, GLint)) SDL_GL_GetProcAddress("glUniform4iARB");
		  qglUniform1fvARB = (GLvoid (APIENTRYP)(GLint, GLsizei, const GLfloat *)) SDL_GL_GetProcAddress("glUniform1fvARB");
		  qglUniform2fvARB = (GLvoid (APIENTRYP)(GLint, GLsizei, const GLfloat *)) SDL_GL_GetProcAddress("glUniform2fvARB");
		  qglUniform3fvARB = (GLvoid (APIENTRYP)(GLint, GLsizei, const GLfloat *)) SDL_GL_GetProcAddress("glUniform3fvARB");
		  qglUniform4fvARB = (GLvoid (APIENTRYP)(GLint, GLsizei, const GLfloat *)) SDL_GL_GetProcAddress("glUniform4fvARB");
		  qglUniform1ivARB = (GLvoid (APIENTRYP)(GLint, GLsizei, const GLint *)) SDL_GL_GetProcAddress("glUniform1ivARB");
		  qglUniform2ivARB = (GLvoid (APIENTRYP)(GLint, GLsizei, const GLint *)) SDL_GL_GetProcAddress("glUniform2ivARB");
		  qglUniform3ivARB = (GLvoid (APIENTRYP)(GLint, GLsizei, const GLint *)) SDL_GL_GetProcAddress("glUniform3ivARB");
		  qglUniform4ivARB = (GLvoid (APIENTRYP)(GLint, GLsizei, const GLint *)) SDL_GL_GetProcAddress("glUniform4ivARB");
		  qglUniformMatrix2fvARB = (GLvoid (APIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)) SDL_GL_GetProcAddress("glUniformMatrix2fvARB");
		  qglUniformMatrix3fvARB = (GLvoid (APIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)) SDL_GL_GetProcAddress("glUniformMatrix3fvARB");
		  qglUniformMatrix4fvARB = (GLvoid (APIENTRYP)(GLint, GLsizei, GLboolean, const GLfloat *)) SDL_GL_GetProcAddress("glUniformMatrix4fvARB");
		  qglGetObjectParameterfvARB = (GLvoid (APIENTRYP)(GLhandleARB, GLenum, GLfloat *)) SDL_GL_GetProcAddress("glGetObjectParameterfvARB");
		  qglGetObjectParameterivARB = (GLvoid (APIENTRYP)(GLhandleARB, GLenum, GLint *)) SDL_GL_GetProcAddress("glGetObjectParameterivARB");
		  qglGetInfoLogARB = (GLvoid (APIENTRYP)(GLhandleARB, GLsizei, GLsizei *, GLcharARB *)) SDL_GL_GetProcAddress("glGetInfoLogARB");
		  qglGetAttachedObjectsARB = (GLvoid (APIENTRYP)(GLhandleARB, GLsizei, GLsizei *, GLhandleARB *)) SDL_GL_GetProcAddress("glGetAttachedObjectsARB");
		  qglGetUniformLocationARB = (GLint (APIENTRYP)(GLhandleARB, const GLcharARB *)) SDL_GL_GetProcAddress("glGetUniformLocationARB");
		  qglGetActiveUniformARB = (GLvoid (APIENTRYP)(GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *)) SDL_GL_GetProcAddress("glGetActiveUniformARB");
		  qglGetUniformfvARB = (GLvoid (APIENTRYP)(GLhandleARB, GLint, GLfloat *)) SDL_GL_GetProcAddress("glGetUniformfvARB");
		  qglGetUniformivARB = (GLvoid (APIENTRYP)(GLhandleARB, GLint, GLint *)) SDL_GL_GetProcAddress("glGetUniformivARB");
		  qglGetShaderSourceARB = (GLvoid (APIENTRYP)(GLhandleARB, GLsizei, GLsizei *, GLcharARB *)) SDL_GL_GetProcAddress("glGetShaderSourceARB");

		  qglVertexAttrib1fARB = (GLvoid (APIENTRYP)(GLuint, GLfloat)) SDL_GL_GetProcAddress("glVertexAttrib1fARB");
		  qglVertexAttrib1sARB = (GLvoid (APIENTRYP)(GLuint, GLshort)) SDL_GL_GetProcAddress("glVertexAttrib1sARB");
		  qglVertexAttrib1dARB = (GLvoid (APIENTRYP)(GLuint, GLdouble)) SDL_GL_GetProcAddress("glVertexAttrib1dARB");
		  qglVertexAttrib2fARB = (GLvoid (APIENTRYP)(GLuint, GLfloat, GLfloat)) SDL_GL_GetProcAddress("glVertexAttrib2fARB");
		  qglVertexAttrib2sARB = (GLvoid (APIENTRYP)(GLuint, GLshort, GLshort)) SDL_GL_GetProcAddress("glVertexAttrib2sARB");
		  qglVertexAttrib2dARB = (GLvoid (APIENTRYP)(GLuint, GLdouble, GLdouble)) SDL_GL_GetProcAddress("glVertexAttrib2dARB");
		  qglVertexAttrib3fARB = (GLvoid (APIENTRYP)(GLuint, GLfloat, GLfloat, GLfloat)) SDL_GL_GetProcAddress("glVertexAttrib3fARB");
		  qglVertexAttrib3sARB = (GLvoid (APIENTRYP)(GLuint, GLshort, GLshort, GLshort)) SDL_GL_GetProcAddress("glVertexAttrib3sARB");
		  qglVertexAttrib3dARB = (GLvoid (APIENTRYP)(GLuint, GLdouble, GLdouble, GLdouble)) SDL_GL_GetProcAddress("glVertexAttrib3dARB");
		  qglVertexAttrib4fARB = (GLvoid (APIENTRYP)(GLuint, GLfloat, GLfloat, GLfloat, GLfloat)) SDL_GL_GetProcAddress("glVertexAttrib4fARB");
		  qglVertexAttrib4sARB = (GLvoid (APIENTRYP)(GLuint, GLshort, GLshort, GLshort, GLshort)) SDL_GL_GetProcAddress("glVertexAttrib4sARB");
		  qglVertexAttrib4dARB = (GLvoid (APIENTRYP)(GLuint, GLdouble, GLdouble, GLdouble, GLdouble)) SDL_GL_GetProcAddress("glVertexAttrib4dARB");
		  qglVertexAttrib4NubARB = (GLvoid (APIENTRYP)(GLuint, GLubyte, GLubyte, GLubyte, GLubyte)) SDL_GL_GetProcAddress("glVertexAttrib4NubARB");
		  qglVertexAttrib1fvARB = (GLvoid (APIENTRYP)(GLuint, GLfloat *)) SDL_GL_GetProcAddress("glVertexAttrib1fvARB");
		  qglVertexAttrib1svARB = (GLvoid (APIENTRYP)(GLuint, GLshort *)) SDL_GL_GetProcAddress("glVertexAttrib1svARB");
		  qglVertexAttrib1dvARB = (GLvoid (APIENTRYP)(GLuint, GLdouble *)) SDL_GL_GetProcAddress("glVertexAttrib1dvARB");
		  qglVertexAttrib2fvARB = (GLvoid (APIENTRYP)(GLuint, GLfloat *)) SDL_GL_GetProcAddress("glVertexAttrib2fvARB");
		  qglVertexAttrib2svARB = (GLvoid (APIENTRYP)(GLuint, GLshort *)) SDL_GL_GetProcAddress("glVertexAttrib2svARB");
		  qglVertexAttrib2dvARB = (GLvoid (APIENTRYP)(GLuint, GLdouble *)) SDL_GL_GetProcAddress("glVertexAttrib2dvARB");
		  qglVertexAttrib3fvARB = (GLvoid (APIENTRYP)(GLuint, GLfloat *)) SDL_GL_GetProcAddress("glVertexAttrib3fvARB");
		  qglVertexAttrib3svARB = (GLvoid (APIENTRYP)(GLuint, GLshort *)) SDL_GL_GetProcAddress("glVertexAttrib3svARB");
		  qglVertexAttrib3dvARB = (GLvoid (APIENTRYP)(GLuint, GLdouble *)) SDL_GL_GetProcAddress("glVertexAttrib3dvARB");
		  qglVertexAttrib4fvARB = (GLvoid (APIENTRYP)(GLuint, const GLfloat *)) SDL_GL_GetProcAddress("glVertexAttrib4fvARB");
		  qglVertexAttrib4svARB = (GLvoid (APIENTRYP)(GLuint, GLshort *)) SDL_GL_GetProcAddress("glVertexAttrib4svARB");
		  qglVertexAttrib4dvARB = (GLvoid (APIENTRYP)(GLuint, GLdouble *)) SDL_GL_GetProcAddress("glVertexAttrib4dvARB");
		  qglVertexAttrib4ivARB = (GLvoid (APIENTRYP)(GLuint, GLint *)) SDL_GL_GetProcAddress("glVertexAttrib4ivARB");
		  qglVertexAttrib4bvARB = (GLvoid (APIENTRYP)(GLuint, GLbyte *)) SDL_GL_GetProcAddress("glVertexAttrib4bvARB");
		  qglVertexAttrib4ubvARB = (GLvoid (APIENTRYP)(GLuint, GLubyte *)) SDL_GL_GetProcAddress("glVertexAttrib4ubvARB");
		  qglVertexAttrib4usvARB = (GLvoid (APIENTRYP)(GLuint, GLushort *)) SDL_GL_GetProcAddress("glVertexAttrib4usvARB");
		  qglVertexAttrib4uivARB = (GLvoid (APIENTRYP)(GLuint, GLuint *)) SDL_GL_GetProcAddress("glVertexAttrib4uivARB");
		  qglVertexAttrib4NbvARB = (GLvoid (APIENTRYP)(GLuint, const GLbyte *)) SDL_GL_GetProcAddress("glVertexAttrib4NbvARB");
		  qglVertexAttrib4NsvARB = (GLvoid (APIENTRYP)(GLuint, const GLshort *)) SDL_GL_GetProcAddress("glVertexAttrib4NsvARB");
		  qglVertexAttrib4NivARB = (GLvoid (APIENTRYP)(GLuint, const GLint *)) SDL_GL_GetProcAddress("glVertexAttrib4NivARB");
		  qglVertexAttrib4NubvARB = (GLvoid (APIENTRYP)(GLuint, const GLubyte *)) SDL_GL_GetProcAddress("glVertexAttrib4NubvARB");
		  qglVertexAttrib4NusvARB = (GLvoid (APIENTRYP)(GLuint, const GLushort *)) SDL_GL_GetProcAddress("glVertexAttrib4NusvARB");
		  qglVertexAttrib4NuivARB = (GLvoid (APIENTRYP)(GLuint, const GLuint *)) SDL_GL_GetProcAddress("glVertexAttrib4NuivARB");
		  qglVertexAttribPointerARB = (GLvoid (APIENTRYP)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid *)) SDL_GL_GetProcAddress("glVertexAttribPointerARB");
		  qglEnableVertexAttribArrayARB = (GLvoid (APIENTRYP)(GLuint)) SDL_GL_GetProcAddress("glEnableVertexAttribArrayARB");
		  qglDisableVertexAttribArrayARB = (GLvoid (APIENTRYP)(GLuint)) SDL_GL_GetProcAddress("glDisableVertexAttribArrayARB");
		  qglBindAttribLocationARB = (GLvoid (APIENTRYP)(GLhandleARB, GLuint, const GLcharARB *)) SDL_GL_GetProcAddress("glBindAttribLocationARB");
		  qglGetActiveAttribARB = (GLvoid (APIENTRYP)(GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *, GLenum *, GLcharARB *)) SDL_GL_GetProcAddress("glGetActiveAttribARB");
		  qglGetAttribLocationARB = (GLint (APIENTRYP)(GLhandleARB, const GLcharARB *)) SDL_GL_GetProcAddress("glGetAttribLocationARB");
		  qglGetVertexAttribdvARB = (GLvoid (APIENTRYP)(GLuint, GLenum, GLdouble *)) SDL_GL_GetProcAddress("glGetVertexAttribdvARB");
		  qglGetVertexAttribfvARB = (GLvoid (APIENTRYP)(GLuint, GLenum, GLfloat *)) SDL_GL_GetProcAddress("glGetVertexAttribfvARB");
		  qglGetVertexAttribivARB = (GLvoid (APIENTRYP)(GLuint, GLenum, GLint *)) SDL_GL_GetProcAddress("glGetVertexAAttribivARB");
		  qglGetVertexAttribPointervARB = (GLvoid (APIENTRYP)(GLuint, GLenum, GLvoid **)) SDL_GL_GetProcAddress("glGetVertexAttribPointervARB");
		  vertexShaders = qtrue;
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_ARB_vertex_shader\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_ARB_vertex_shader not found\n" );
	}
#endif
	// leilei - paletted texturing
	palettedTextureSupport = qfalse;
	if ( GLimp_HaveExtension( "GL_EXT_paletted_texture" ) )
	{
		if ( r_ext_paletted_texture->integer ) {
			qglColorTableEXT = (GLvoid (APIENTRYP)(GLint, GLint, GLint, GLint, GLint, const GLvoid *)) SDL_GL_GetProcAddress("glColorTableEXT");
			// SGI

			qglColorTableSGI = (GLvoid (APIENTRYP)(GLint, GLint, GLint, GLint, GLint, const GLvoid *)) SDL_GL_GetProcAddress("glColorTableSGI");
			{
				ri.Printf( PRINT_ALL, "...using GL_EXT_paletted_texture\n");
				palettedTextureSupport = qtrue;
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_EXT_paletted_texture\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_EXT_paletted_texture not found\n" );
	}
	// leilei  - DDS/DXT
	textureCompressionSupport = qfalse;
	if ( GLimp_HaveExtension( "GL_ARB_texture_compression" ) )
	{
		if ( r_loadDDS->integer ) {
			qglCompressedTexImage2DARB = (GLvoid (APIENTRYP)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid *)) SDL_GL_GetProcAddress("glCompressedTexImage2DARB");
			{
				ri.Printf( PRINT_ALL, "...using GL_ARB_texture_compression\n");
				textureCompressionSupport = qtrue;
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring GL_ARB_texture_compression\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...GL_ARB_texture_compression not found\n" );
	}

#ifdef _WIN32
	// leilei - 3dfx gamma
	// WGL_3DFX_gamma_control
	qwglGetDeviceGammaRamp3DFX = NULL;
	qwglSetDeviceGammaRamp3DFX = NULL;

	if ( strstr( glConfig.extensions_string, "WGL_3DFX_gamma_control" ) )
	{
		if ( !r_ignorehwgamma->integer && r_ext_gamma_control->integer )
		{
			qwglGetDeviceGammaRamp3DFX = ( BOOL ( WINAPI * )( HDC, LPVOID ) ) SDL_GL_GetProcAddress( "wglGetDeviceGammaRamp3DFX" );
			qwglSetDeviceGammaRamp3DFX = ( BOOL ( WINAPI * )( HDC, LPVOID ) ) SDL_GL_GetProcAddress( "wglSetDeviceGammaRamp3DFX" );

			if ( qwglGetDeviceGammaRamp3DFX && qwglSetDeviceGammaRamp3DFX )
			{
				ri.Printf( PRINT_ALL, "...using WGL_3DFX_gamma_control\n" );
			}
			else
			{
				qwglGetDeviceGammaRamp3DFX = NULL;
				qwglSetDeviceGammaRamp3DFX = NULL;
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "...ignoring WGL_3DFX_gamma_control\n" );
		}
	}
	else
	{
		ri.Printf( PRINT_ALL, "...WGL_3DFX_gamma_control not found\n" );
	}
#endif

    // XXX This is likely too late to check the com_abnormalExit cvar
    if( ri.Cvar_VariableIntegerValue( "com_abnormalExit" ) )
    {
        ri.Cvar_Set( "r_postprocess", "none");
        ri.Cvar_Set( "r_ext_vertex_shader", "0");
        ri.Cvar_Set( "r_leifx", "0");	// leilei FIXME - q3 cinematics crash leifx modes
    }


	// Vendor-specific graphics fixes because we need it for these cards
	char	buf[1024];

// get our config strings
	Q_strncpyz( glConfig.vendor_string, (char *) qglGetString (GL_VENDOR), sizeof( glConfig.vendor_string ) );
	Q_strncpyz( glConfig.renderer_string, (char *) qglGetString (GL_RENDERER), sizeof( glConfig.renderer_string ) );
	if (*glConfig.renderer_string && glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] == '\n')
		glConfig.renderer_string[strlen(glConfig.renderer_string) - 1] = 0;
	Q_strncpyz( glConfig.version_string, (char *) qglGetString (GL_VERSION), sizeof( glConfig.version_string ) );
	Q_strncpyz( glConfig.extensions_string, (char *) qglGetString (GL_EXTENSIONS), sizeof( glConfig.extensions_string ) );



	Q_strncpyz( buf, glConfig.renderer_string, sizeof(buf) );
	Q_strlwr( buf );

	// leilei - we are  done with our extensions string, if it's too long we can just trim it so we won't hardlock old qvms with an overflow.
	Q_strncpyz( extensions_string_full, (char *) qglGetString (GL_EXTENSIONS), sizeof( glConfig.extensions_string ) ); // copy to a global for gfxinfo
	char	butt[640]; 
	Q_strncpyz( glConfig.extensions_string, (char *) qglGetString (GL_EXTENSIONS), sizeof( butt ) );


	//
	// this is where hardware specific workarounds that should be
	// detected/initialized every startup should go.
	//
	if ( strstr( buf, "banshee" ) || strstr( buf, "voodoo3" ) )
	{
		glConfig.hardwareType = GLHW_3DFX_2D3D;
	}
	// VOODOO GRAPHICS w/ 2MB
	else if ( strstr( buf, "voodoo graphics/1 tmu/2 mb" ) )
	{
		voodootype = 1;
		ri.Printf( PRINT_ALL, "3dfx Voodoo Graphics assumed\n" );
	}
	// any Voodoo Graphics
	else if ( strstr( buf, "voodoo graphics" ) )
	{
		voodootype = 1;
		ri.Printf( PRINT_ALL, "3dfx Voodoo Graphics assumed\n" );
	}
	else if ( strstr( buf, "voodoo2" ) )
	{
		voodootype = 2;
		ri.Printf( PRINT_ALL, "3dfx Voodoo2 assumed\n" );
	}
	else if ( strstr( buf, "voodoo3" ) )
	{
		voodootype = 3;
		ri.Printf( PRINT_ALL, "3dfx Voodoo3 assumed\n" );
	}
	else if ( strstr( buf, "voodoo4" ) )
	{
		voodootype = 4;
		ri.Printf( PRINT_ALL, "3dfx Voodoo4 assumed\n" );
	}
	else if ( strstr( buf, "voodoo5" ) )
	{
		voodootype = 4;
		ri.Printf( PRINT_ALL, "3dfx Voodoo5 assumed\n" );
	}
	// Just any voodoo at this point.
	else if ( strstr( buf, "voodoo" ) )
	{
		voodootype = 1;
		ri.Printf( PRINT_ALL, "3dfx Voodoo of some sort assumed (V1 mode)\n" );
	}
	else if ( strstr( buf, "glzicd" ) )
	{
	}
	else if ( strstr( buf, "rage pro" ) || strstr( buf, "Rage Pro" ) || strstr( buf, "ragepro" ) )
	{
		glConfig.hardwareType = GLHW_RAGEPRO;
	}
	else if ( strstr( buf, "rage 128" ) )
	{
	}
	else if ( strstr( buf, "permedia2" ) )
	{
		glConfig.hardwareType = GLHW_PERMEDIA2;
	}
	else if ( strstr( buf, "sis" ) )		// unfortunately onboard SiS is a bit craptastic so give it permedia fallback as well
	{
		glConfig.hardwareType = GLHW_PERMEDIA2;
	}
	else if ( strstr( buf, "powervr" ) )
	{
	//	glConfig.hardwareType = GLHW_PCX2;
	}
	else if ( strstr( buf, "brian" ) ||  strstr( buf, "paul" ) ||  strstr( buf, "GDI" ) )	 // software mode opengl needs speedup tricks
	{
		softwaremode = 1;
	}
	else if ( strstr( buf, "riva 128" ) )
	{
		glConfig.hardwareType = GLHW_RIVA128;
	}
	else if ( strstr( buf, "riva tnt " ) )
	{
	}
}
