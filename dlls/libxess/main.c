/*
 * Stand-ins for Intel's XeSS libraries (libxess.dll, libxess_dx11.dll)
 *
 * Copyright 2026 Chad Cormier Roussel
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* XeSS's path for GPUs other than Intel's is built on DP4a, which Apple GPUs emulate
 * at ~4 FPS. These stand-ins report every device as unsupported, so games fall back to
 * another upscaler; libxess.dll's D3D12 calls are answered by MetalFX instead (d3d12.c).
 * Each export ignores the arguments it doesn't name below. */

#include <stdarg.h>
#include <stdint.h>

#include "windef.h"
#include "winbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xess);

/* xess_result_t values and xess_version_t, from Intel's public xess.h */
#define XESS_RESULT_SUCCESS                  0
#define XESS_RESULT_ERROR_UNSUPPORTED_DEVICE (-1)
#define XESS_RESULT_ERROR_INVALID_ARGUMENT   (-4)
#define XESS_RESULT_ERROR_INVALID_CONTEXT    (-8)

typedef struct
{
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
    uint16_t reserved;
} xess_version_t;

/* the SDK version of the libraries these stand in for (SMITE 2 ships 2.0.1.41) */
int CDECL xessGetVersion( xess_version_t *version )
{
    TRACE( "%p\n", version );
    if (!version) return XESS_RESULT_ERROR_INVALID_ARGUMENT;
    version->major = 2;
    version->minor = 0;
    version->patch = 1;
    version->reserved = 0;
    return XESS_RESULT_SUCCESS;
}

/* xessD3D12CreateContext, xessD3D11CreateContext */
int CDECL xess_create_context( void *device, void **context )
{
    WARN( "XeSS isn't available on this GPU; MACNEUTRON_XESS=1 uses the game's own\n" );
    if (context) *context = NULL;
    return XESS_RESULT_ERROR_UNSUPPORTED_DEVICE;
}

/* xessVKCreateContext */
int CDECL xess_vk_create_context( void *instance, void *physical_device, void *device, void **context )
{
    return xess_create_context( device, context );
}

/* capability queries and calls of unknown signature */
int CDECL xess_unsupported(void)
{
    TRACE( "\n" );
    return XESS_RESULT_ERROR_UNSUPPORTED_DEVICE;
}

/* every call that takes a context: none was ever created */
int CDECL xess_invalid_context(void)
{
    TRACE( "\n" );
    return XESS_RESULT_ERROR_INVALID_CONTEXT;
}

int CDECL xessDestroyContext( void *context )
{
    TRACE( "%p\n", context );
    return context ? XESS_RESULT_ERROR_INVALID_CONTEXT : XESS_RESULT_SUCCESS;
}
