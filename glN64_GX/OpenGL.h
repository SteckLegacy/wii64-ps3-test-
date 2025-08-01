/**
 * glN64_GX - OpenGL.h
 * Copyright (C) 2003 Orkin
 * Copyright (C) 2008, 2009, 2010 sepp256 (Port to Wii/Gamecube/PS3)
 *
 * glN64 homepage: http://gln64.emulation64.com
 * Wii64 homepage: http://www.emulatemii.com
 * email address: sepp256@gmail.com
 *
**/

#ifndef OPENGL_H
#define OPENGL_H

#ifndef __LINUX__
# include <windows.h>
# include "wglext.h"
# include <GL/gl.h>
# include "glext.h"
#else
# include "../main/winlnxdefs.h"
#define GL_GLEXT_PROTOTYPES
#define __WIN32__
#include "gl.h" 
#include "glext.h"
#undef __WIN32__
//# include <GL/gl.h>
//# include <GL/glext.h>
# if !(defined(__GX__)||defined(PS3))
# include "SDL.h"
# endif // !_GX__ !PS3
#endif // __LINUX__

#include "glATI.h"
#include "gSP.h"

#define GXprojZScale		 0.5  //0.25 //0.5
#define GXprojZOffset		-0.5 //-0.5
#define GXpolyOffsetFactor	 5.0e-4 //Tweaked for co-planar polygons. Interestingly, Z resolution should be 5.96e-8.

#ifdef PS3
#include <rsx/rsx.h>
#include <sysutil/video_out.h>
#include "../main/rsxutil.h"
#ifdef SHOW_DEBUG
#include "../main/debug/DEBUG.h"
#endif

extern "C" void dbg_printf(const char *fmt,...);

#include <vectormath/cpp/vectormath_aos.h>
using namespace Vectormath::Aos;

#include "combined_shader_vpo.h"
#include "combined_shader_fpo.h"

enum Shaders
{
	SHADER_PASSTEX=1,
	SHADER_PASSCOLOR,
	SHADER_MODULATE
};
#endif // PS3

struct GLVertex
{
	float x, y, z, w;
	struct
	{
		float r, g, b, a;
	} color, secondaryColor;
	float s0, t0, s1, t1;
	float fog;
#ifdef __GX__
	float zPrime;
#endif //__GX__
};

struct GLInfo
{
#if !(defined(__GX__)||defined(PS3))
#ifndef __LINUX__
	HGLRC	hRC, hPbufferRC;
	HDC		hDC, hPbufferDC;
	HWND	hWnd;
	HPBUFFERARB	hPbuffer;
#else
	SDL_Surface *hScreen;
#endif // __LINUX__
#endif // !__GX__ !PS3

	DWORD	fullscreenWidth, fullscreenHeight, fullscreenBits, fullscreenRefresh;
	DWORD	width, height, windowedWidth, windowedHeight, heightOffset;

	BOOL	fullscreen, forceBilinear, fog;

	float	scaleX, scaleY;

	BOOL	ATI_texture_env_combine3;	// Radeon
	BOOL	ATIX_texture_env_route;		// Radeon

	BOOL	ARB_multitexture;			// TNT, GeForce, Rage 128, Radeon
	BOOL	ARB_texture_env_combine;	// GeForce, Rage 128, Radeon
	BOOL	ARB_texture_env_crossbar;	// Radeon (GeForce supports it, but doesn't report it)

	BOOL	EXT_fog_coord;				// TNT, GeForce, Rage 128, Radeon
	BOOL	EXT_texture_env_combine;	// TNT, GeForce, Rage 128, Radeon
	BOOL	EXT_secondary_color;		// GeForce, Radeon

	BOOL	NV_texture_env_combine4;	// TNT, GeForce
	BOOL	NV_register_combiners;		// GeForce
	BOOL	ARB_buffer_region;
	BOOL	ARB_pbuffer;
	BOOL	ARB_render_texture;
	BOOL	ARB_pixel_format;

	int		maxTextureUnits;			// TNT = 2, GeForce = 2-4, Rage 128 = 2, Radeon = 3-6
	int		maxGeneralCombiners;

	BOOL	enable2xSaI;
	BOOL	frameBufferTextures;
	int		textureBitDepth;
	float	originAdjust;

	GLVertex vertices[256];
	BYTE	triangles[80][3];
	BYTE	numTriangles;
	BYTE	numVertices;
#ifndef __LINUX__
	HWND	hFullscreenWnd;
#endif
	BOOL	usePolygonStipple;
	GLubyte	stipplePattern[32][8][128];
	BYTE	lastStipple;

	BYTE	combiner;

#ifdef PS3
	u32 fpsize;
	u32 vpsize;
	u32 fp_offset;
	u32 *fp_buffer;

	rsxProgramConst *projMatrix_id;
	rsxProgramConst *modelViewMatrix_id;
	rsxProgramAttrib *vertexPosition_id;
	rsxProgramAttrib *vertexColor0_id;
	rsxProgramAttrib *vertexTexcoord_id;
	rsxProgramAttrib *textureUnit_id;
	rsxProgramConst *mode_id;
	f32 shader_mode;

	void *vp_ucode;
	rsxVertexProgram *vpo;
	void *fp_ucode;
	rsxFragmentProgram *fpo;

	Matrix4 projMatrix, modelViewMatrix;
	//Temporary
	u32 finish_ref;
	u16* FBtex;
#endif //PS3
#if defined(__GX__)||defined(PS3)	//Variables specific to GX
	int		GXorigX, GXorigY;
	int		GXwidth, GXheight;
#endif //__GX__ PS3
#ifdef __GX__
	float	GXscaleX, GXscaleY;
	float	GXzPrimeScale, GXzPrimeTranslate;
	Mtx44	GXcombW;
	Mtx44	GXprojWnear;
	Mtx44	GXprojIdent;
	Mtx44	GXprojTemp;
	Mtx		GXmodelViewIdent;
	BOOL	GXuseCombW;
	BOOL	GXuseProjWnear;
	BOOL	GXupdateMtx;
	int		GXnumVtxMP;
	bool	GXuseAlphaCompare;
	float	GXfogStartZ;
	float	GXfogEndZ;
	GXColor	GXfogColor;
	GXColor GXclearColor;
	u8		GXfogType;
	u8*		GXclearBufferTex;
	bool	GXupdateFog;
	bool	GXpolyOffset;
	bool	GXrenderTexRect;
	bool	GXforceClampS0;
	bool	GXforceClampT0;
	bool	GXforceClampS1;
	bool	GXforceClampT1;
	bool	GXuseMinMagNearest;
	bool	GXclearColorBuffer;
	bool	GXclearDepthBuffer;
#endif
};

extern GLInfo OGL;

struct GLcolor
{
	float r, g, b, a;
};

#ifndef __LINUX__
extern PFNGLCOMBINERPARAMETERFVNVPROC glCombinerParameterfvNV;
extern PFNGLCOMBINERPARAMETERFNVPROC glCombinerParameterfNV;
extern PFNGLCOMBINERPARAMETERIVNVPROC glCombinerParameterivNV;
extern PFNGLCOMBINERPARAMETERINVPROC glCombinerParameteriNV;
extern PFNGLCOMBINERINPUTNVPROC glCombinerInputNV;
extern PFNGLCOMBINEROUTPUTNVPROC glCombinerOutputNV;
extern PFNGLFINALCOMBINERINPUTNVPROC glFinalCombinerInputNV;
extern PFNGLGETCOMBINERINPUTPARAMETERFVNVPROC glGetCombinerInputParameterfvNV;
extern PFNGLGETCOMBINERINPUTPARAMETERIVNVPROC glGetCombinerInputParameterivNV;
extern PFNGLGETCOMBINEROUTPUTPARAMETERFVNVPROC glGetCombinerOutputParameterfvNV;
extern PFNGLGETCOMBINEROUTPUTPARAMETERIVNVPROC glGetCombinerOutputParameterivNV;
extern PFNGLGETFINALCOMBINERINPUTPARAMETERFVNVPROC glGetFinalCombinerInputParameterfvNV;
extern PFNGLGETFINALCOMBINERINPUTPARAMETERIVNVPROC glGetFinalCombinerInputParameterivNV;

extern PFNGLACTIVETEXTUREARBPROC glActiveTextureARB;
extern PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB;
extern PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB;

extern PFNGLSECONDARYCOLOR3FVEXTPROC glSecondaryColor3fvEXT;

extern PFNGLSECONDARYCOLOR3BEXTPROC glSecondaryColor3bEXT;
extern PFNGLSECONDARYCOLOR3BVEXTPROC glSecondaryColor3bvEXT;
extern PFNGLSECONDARYCOLOR3DEXTPROC glSecondaryColor3dEXT;
extern PFNGLSECONDARYCOLOR3DVEXTPROC glSecondaryColor3dvEXT;
extern PFNGLSECONDARYCOLOR3FEXTPROC glSecondaryColor3fEXT;
extern PFNGLSECONDARYCOLOR3FVEXTPROC glSecondaryColor3fvEXT;
extern PFNGLSECONDARYCOLOR3IEXTPROC glSecondaryColor3iEXT;
extern PFNGLSECONDARYCOLOR3IVEXTPROC glSecondaryColor3ivEXT;
extern PFNGLSECONDARYCOLOR3SEXTPROC	glSecondaryColor3sEXT;
extern PFNGLSECONDARYCOLOR3SVEXTPROC glSecondaryColor3svEXT;
extern PFNGLSECONDARYCOLOR3UBEXTPROC glSecondaryColor3ubEXT;
extern PFNGLSECONDARYCOLOR3UBVEXTPROC glSecondaryColor3ubvEXT;
extern PFNGLSECONDARYCOLOR3UIEXTPROC glSecondaryColor3uiEXT;
extern PFNGLSECONDARYCOLOR3UIVEXTPROC glSecondaryColor3uivEXT;
extern PFNGLSECONDARYCOLOR3USEXTPROC glSecondaryColor3usEXT;
extern PFNGLSECONDARYCOLOR3USVEXTPROC glSecondaryColor3usvEXT;
extern PFNGLSECONDARYCOLORPOINTEREXTPROC glSecondaryColorPointerEXT;

extern PFNWGLCREATEBUFFERREGIONARBPROC wglCreateBufferRegionARB;
extern PFNWGLDELETEBUFFERREGIONARBPROC wglDeleteBufferRegionARB;
extern PFNWGLSAVEBUFFERREGIONARBPROC wglSaveBufferRegionARB;
extern PFNWGLRESTOREBUFFERREGIONARBPROC wglRestoreBufferRegionARB;

extern PFNWGLCREATEPBUFFERARBPROC wglCreatePbufferARB;
extern PFNWGLGETPBUFFERDCARBPROC wglGetPbufferDCARB;
extern PFNWGLRELEASEPBUFFERDCARBPROC wglReleasePbufferDCARB;
extern PFNWGLDESTROYPBUFFERARBPROC wglDestroyPbufferARB;
extern PFNWGLQUERYPBUFFERARBPROC wglQueryPbufferARB;

extern PFNWGLGETPIXELFORMATATTRIBIVARBPROC wglGetPixelFormatAttribivARB;
extern PFNWGLGETPIXELFORMATATTRIBFVARBPROC wglGetPixelFormatAttribfvARB;
extern PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
#endif // !__LINUX__

bool OGL_Start();
void OGL_Stop();
void OGL_AddTriangle( SPVertex *vertices, int v0, int v1, int v2 );
void OGL_DrawTriangles();
void OGL_DrawLine( SPVertex *vertices, int v0, int v1, float width );
void OGL_DrawRect( int ulx, int uly, int lrx, int lry, float *color );
void OGL_DrawTexturedRect( float ulx, float uly, float lrx, float lry, float uls, float ult, float lrs, float lrt, bool flip );
void OGL_UpdateScale();
void OGL_ClearDepthBuffer();
void OGL_ClearColorBuffer( float *color );
void OGL_ResizeWindow();
void OGL_SaveScreenshot();
#ifdef __LINUX__
void OGL_SwapBuffers();
#endif // __LINUX__
void OGL_ReadScreen( void **dest, long *width, long *height );
#ifdef PS3
void OGL_RSXinitDlist();
#endif // PS3
#ifdef __GX__
void OGL_GXinitDlist();
void OGL_GXclearEFB();
#endif // __GX__

#endif
