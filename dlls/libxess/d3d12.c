/*
 * XeSS on D3D12, answered by MetalFX's temporal upscaler through DXMT's private interface
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

/* XeSS's network runs on Apple GPUs at ~4 FPS (DP4a, emulated). These exports keep XeSS's API and
 * hand the frame to MetalFX instead: one upscaler per context, recorded into the game's command
 * list (DXMT's IMTLD3D12CommandListExt), which keeps it alive until the list's allocator is reset.
 * Without DXMT (Wine's own D3D12) a context can't be made and games pick another upscaler.
 * A bad call returns XeSS's error code, records nothing and is logged once; the first Init and the
 * first Execute log their parameters once, as WARN: the launcher's logging mode never shows TRACE. */

#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "d3d12.h"
#include "initguid.h"
#include "dxmt_d3d12_ext.h"
#include "xess.h"
#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(xess);

C_ASSERT(sizeof(xess_d3d12_execute_params_t) == 136);
C_ASSERT(offsetof(xess_d3d12_execute_params_t, pDescriptorHeap) == 120);
C_ASSERT(sizeof(xess_d3d12_init_params_t) == 64);
C_ASSERT(sizeof(xess_properties_t) == 24);
C_ASSERT(sizeof(MTL_TEMPORAL_SCALER_D3D12_DESC) == 56);
C_ASSERT(sizeof(MTL_TEMPORAL_UPSCALE_D3D12_DESC) == 128);

/* bits 0-8, and xess_debug.h's XESS_DEBUG_ENABLE_PROFILING; EXTERNAL_DESCRIPTOR_HEAP (5) and
 * LDR_INPUT_COLOR (6) need nothing from MetalFX */
#define KNOWN_FLAGS (0x1ffu | 1u << 30)

struct xess_d3d12
{
    struct list entry;
    SRWLOCK lock;
    IMTLD3D12DeviceExt *ext;
    float min_scale, max_scale;  /* the device's range of output / input */
    xess_d3d12_init_params_t init;
    BOOL initialized, built;
    float jitter_x, jitter_y, velocity_x, velocity_y;
    IMTLD3D12TemporalScaler *scaler;
    MTL_TEMPORAL_SCALER_D3D12_DESC scaler_desc;
    xess_app_log_callback_t log;
    xess_logging_level_t log_level;
};

/* Every live context: a handle not in the list is refused, never dereferenced. */
static SRWLOCK contexts_lock = SRWLOCK_INIT;
static struct list contexts = LIST_INIT(contexts);

/* The context behind a handle, locked; NULL for a handle that isn't one.
 * ponytail: the list lock is held (shared) while waiting for the context's lock, so a destroy (it takes the list
 * lock exclusively) and the lookups queued behind it wait for the one call inside that context: recording an
 * upscale, or at most one ~13 ms upscaler creation; never a GPU wait, and never a deadlock (always list, then
 * context). A reference count taken under the list lock would let go of it first; worth it only if a game's
 * threads show that wait. */
static struct xess_d3d12 *lock_context(xess_context_handle_t handle)
{
    struct xess_d3d12 *ctx;

    AcquireSRWLockShared(&contexts_lock);
    LIST_FOR_EACH_ENTRY(ctx, &contexts, struct xess_d3d12, entry)
    {
        if (ctx != (struct xess_d3d12 *)handle) continue;
        AcquireSRWLockExclusive(&ctx->lock);
        ReleaseSRWLockShared(&contexts_lock);
        return ctx;
    }
    ReleaseSRWLockShared(&contexts_lock);
    return NULL;
}

/* A bad call's message, made the first time a call site sees one (`once`), logged by
 * unlock_context once the context is unlocked: the game's callback may call back in. */
static void once(LONG *flag, char *msg, const char *format, ...)
{
    va_list args;

    if (InterlockedExchange(flag, 1)) return;
    va_start(args, format);
    vsnprintf(msg, 512, format, args);
    va_end(args);
}

static xess_result_t unlock_context(struct xess_d3d12 *ctx, xess_result_t ret, const char *msg)
{
    xess_app_log_callback_t log = ctx->log_level <= XESS_LOGGING_LEVEL_WARNING ? ctx->log : NULL;

    ReleaseSRWLockExclusive(&ctx->lock);
    if (!*msg) return ret;
    WARN("%s\n", msg);
    if (log) log(msg, XESS_LOGGING_LEVEL_WARNING);
    return ret;
}

static void release_scaler(struct xess_d3d12 *ctx)
{
    if (ctx->scaler) IMTLD3D12TemporalScaler_Release(ctx->scaler);
    ctx->scaler = NULL;
}

static xess_result_t result_of(HRESULT hr)
{
    if (SUCCEEDED(hr)) return XESS_RESULT_SUCCESS;
    if (hr == E_INVALIDARG) return XESS_RESULT_ERROR_INVALID_ARGUMENT;
    if (hr == E_OUTOFMEMORY) return XESS_RESULT_ERROR_DEVICE_OUT_OF_MEMORY;
    return XESS_RESULT_ERROR_DEVICE;
}

/* Output size / input size per quality mode, as XeSS 1.3 and later (Intel's XeSS-SR Developer Guide 2.0). */
static float ratio_of(xess_quality_settings_t quality)
{
    switch (quality)
    {
    case XESS_QUALITY_SETTING_AA: return 1.0f;
    case XESS_QUALITY_SETTING_ULTRA_QUALITY_PLUS: return 1.3f;
    case XESS_QUALITY_SETTING_ULTRA_QUALITY: return 1.5f;
    case XESS_QUALITY_SETTING_QUALITY: return 1.7f;
    case XESS_QUALITY_SETTING_BALANCED: return 2.0f;
    case XESS_QUALITY_SETTING_PERFORMANCE: return 2.3f;
    case XESS_QUALITY_SETTING_ULTRA_PERFORMANCE: return 3.0f;
    }
    return 0;
}

/* `output` / `ratio` rounded down to even, within the device's range of output / input. */
static uint32_t input_size(const struct xess_d3d12 *ctx, uint32_t output, float ratio)
{
    uint32_t in = (uint32_t)(output / ratio) & ~1u, lo = (uint32_t)ceilf(output / ctx->max_scale),
             hi = (uint32_t)(output / ctx->min_scale);

    lo += lo & 1;
    return max(min(in, hi), lo);
}

/* The optimal input of a mode; FALSE for an unknown mode or an empty output. */
static BOOL optimal_input(const struct xess_d3d12 *ctx, const xess_2d_t *output, xess_quality_settings_t quality,
                          xess_2d_t *optimal)
{
    float ratio = ratio_of(quality);

    if (!ratio || !output->x || !output->y) return FALSE;
    optimal->x = input_size(ctx, output->x, ratio);
    optimal->y = input_size(ctx, output->y, ratio);
    return TRUE;
}

xess_result_t CDECL d3d12_create_context(ID3D12Device *device, xess_context_handle_t *handle)
{
    IMTLD3D12DeviceExt *ext;
    struct xess_d3d12 *ctx;
    float lo, hi;

    TRACE("%p, %p\n", device, handle);
    if (!handle) return XESS_RESULT_ERROR_INVALID_ARGUMENT;
    *handle = NULL;
    if (!device) return XESS_RESULT_ERROR_INVALID_ARGUMENT;
    if (FAILED(ID3D12Device_QueryInterface(device, &IID_IMTLD3D12DeviceExt, (void **)&ext)))
    {
        static LONG warned;
        if (!InterlockedExchange(&warned, 1))
            WARN("XeSS needs DXMT's MetalFX upscaler, not on this device; MACNEUTRON_XESS=1 uses the game's own\n");
        return XESS_RESULT_ERROR_UNSUPPORTED_DEVICE;
    }
    if (FAILED(IMTLD3D12DeviceExt_GetTemporalScalerScaleRange(ext, &lo, &hi)) || !(ctx = calloc(1, sizeof(*ctx))))
    {
        IMTLD3D12DeviceExt_Release(ext);
        return XESS_RESULT_ERROR_DEVICE;
    }
    InitializeSRWLock(&ctx->lock);
    ctx->ext = ext;
    ctx->min_scale = lo;
    ctx->max_scale = hi;
    ctx->jitter_x = ctx->jitter_y = ctx->velocity_x = ctx->velocity_y = 1.0f;
    ctx->log_level = XESS_LOGGING_LEVEL_ERROR;
    AcquireSRWLockExclusive(&contexts_lock);
    list_add_tail(&contexts, &ctx->entry);
    ReleaseSRWLockExclusive(&contexts_lock);
    *handle = (xess_context_handle_t)ctx;
    TRACE("-> %p, scale %.3f-%.3f\n", ctx, lo, hi);
    return XESS_RESULT_SUCCESS;
}

/* Like xessDestroyContext in main.c (libxess_dx11's): a NULL handle is fine. The upscaler stays
 * alive in any command list that recorded it until that list's allocator is reset. */
xess_result_t CDECL d3d12_destroy_context(xess_context_handle_t handle)
{
    struct xess_d3d12 *ctx, *found = NULL;

    TRACE("%p\n", handle);
    if (!handle) return XESS_RESULT_SUCCESS;
    AcquireSRWLockExclusive(&contexts_lock);
    LIST_FOR_EACH_ENTRY(ctx, &contexts, struct xess_d3d12, entry)
    {
        if (ctx != (struct xess_d3d12 *)handle) continue;
        list_remove(&ctx->entry);
        found = ctx;
        break;
    }
    ReleaseSRWLockExclusive(&contexts_lock);
    if (!found) return XESS_RESULT_ERROR_INVALID_CONTEXT;
    /* a call still inside the context finishes first */
    AcquireSRWLockExclusive(&found->lock);
    ReleaseSRWLockExclusive(&found->lock);
    release_scaler(found);
    IMTLD3D12DeviceExt_Release(found->ext);
    free(found);
    return XESS_RESULT_SUCCESS;
}

/* Pipelines are MetalFX's: built when the upscaler is made. `blocking` (a C bool) isn't read. */
xess_result_t CDECL d3d12_build_pipelines(xess_context_handle_t handle, ID3D12PipelineLibrary *library,
                                          bool blocking, uint32_t flags)
{
    struct xess_d3d12 *ctx;

    TRACE("%p, %p, %#x\n", handle, library, flags);
    if (!(ctx = lock_context(handle))) return XESS_RESULT_ERROR_INVALID_CONTEXT;
    ctx->built = TRUE;
    return unlock_context(ctx, XESS_RESULT_SUCCESS, "");
}

xess_result_t CDECL d3d12_get_pipeline_build_status(xess_context_handle_t handle)
{
    struct xess_d3d12 *ctx;

    if (!(ctx = lock_context(handle))) return XESS_RESULT_ERROR_INVALID_CONTEXT;
    return unlock_context(ctx, ctx->built ? XESS_RESULT_SUCCESS : XESS_RESULT_ERROR_WRONG_CALL_ORDER, "");
}

/* XeSS's Init has no texture formats: the upscaler is made for the usual ones (the mode's optimal
 * input, RGBA16F colour and output, D32 depth, RG16F motion vectors), and Execute makes another
 * if the game's textures differ. One that can't be made fails Init; Unreal then uses its own AA. */
static xess_result_t init(struct xess_d3d12 *ctx, const xess_d3d12_init_params_t *params, char *msg)
{
    static LONG logged, bad_flags, bad_size, failed;
    MTL_TEMPORAL_SCALER_D3D12_DESC desc = {0};
    uint32_t flags;
    xess_2d_t in;
    HRESULT hr;

    if (!params) return XESS_RESULT_ERROR_INVALID_ARGUMENT;
    flags = params->initFlags;
    if (!InterlockedExchange(&logged, 1))
        WARN("output %ux%u, quality %d, flags %#x, temp heaps %p %p, pipeline library %p\n",
             params->outputResolution.x, params->outputResolution.y, params->qualitySetting, flags,
             params->pTempBufferHeap, params->pTempTextureHeap, params->pPipelineLibrary);
    if (flags & ~KNOWN_FLAGS)
    {
        once(&bad_flags, msg, "xessD3D12Init: unknown flags %#x", flags & ~KNOWN_FLAGS);
        return XESS_RESULT_ERROR_INVALID_ARGUMENT;
    }
    if (!optimal_input(ctx, &params->outputResolution, params->qualitySetting, &in))
    {
        once(&bad_size, msg, "xessD3D12Init: output %ux%u, quality %d", params->outputResolution.x,
             params->outputResolution.y, params->qualitySetting);
        return XESS_RESULT_ERROR_INVALID_ARGUMENT;
    }
    /* re-initialised (a resolution or quality change): the old upscaler goes now */
    release_scaler(ctx);
    ctx->initialized = FALSE;
    ctx->init = *params;
    desc.InputWidth = in.x;
    desc.InputHeight = in.y;
    desc.OutputWidth = params->outputResolution.x;
    desc.OutputHeight = params->outputResolution.y;
    desc.ColorFormat = desc.OutputFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
    desc.DepthFormat = DXGI_FORMAT_D32_FLOAT;
    desc.MotionFormat = DXGI_FORMAT_R16G16_FLOAT;
    desc.ReactiveMaskFormat = DXGI_FORMAT_UNKNOWN;
    desc.AutoExposure = !!(flags & XESS_INIT_FLAG_ENABLE_AUTOEXPOSURE);
    desc.OutputResolutionMotionVectors = !!(flags & XESS_INIT_FLAG_HIGH_RES_MV);
    desc.JitteredMotionVectors = !!(flags & XESS_INIT_FLAG_JITTERED_MV);
    if (FAILED(hr = IMTLD3D12DeviceExt_CreateTemporalScaler(ctx->ext, &desc, &ctx->scaler)))
    {
        ctx->scaler = NULL;
        once(&failed, msg, "xessD3D12Init: no MetalFX upscaler for %ux%u to %ux%u: %#lx", in.x, in.y,
             desc.OutputWidth, desc.OutputHeight, hr);
        return XESS_RESULT_ERROR_DEVICE_OUT_OF_MEMORY;
    }
    ctx->scaler_desc = desc;
    ctx->initialized = TRUE;
    return XESS_RESULT_SUCCESS;
}

xess_result_t CDECL d3d12_init(xess_context_handle_t handle, const xess_d3d12_init_params_t *params)
{
    struct xess_d3d12 *ctx;
    char msg[512] = "";

    TRACE("%p, %p\n", handle, params);
    if (!(ctx = lock_context(handle))) return XESS_RESULT_ERROR_INVALID_CONTEXT;
    return unlock_context(ctx, init(ctx, params, msg), msg);
}

xess_result_t CDECL d3d12_get_init_params(xess_context_handle_t handle, xess_d3d12_init_params_t *params)
{
    struct xess_d3d12 *ctx;
    xess_result_t ret = XESS_RESULT_SUCCESS;

    if (!(ctx = lock_context(handle))) return XESS_RESULT_ERROR_INVALID_CONTEXT;
    if (!params) ret = XESS_RESULT_ERROR_INVALID_ARGUMENT;
    else if (!ctx->initialized) ret = XESS_RESULT_ERROR_UNINITIALIZED;
    else *params = ctx->init;
    return unlock_context(ctx, ret, "");
}

/* How much of a texture lies past (x, y): 0 if none, or if it isn't a 2D texture. */
static void extent(ID3D12Resource *resource, D3D12_RESOURCE_DESC *desc, uint32_t x, uint32_t y, UINT *w, UINT *h)
{
    *desc = ID3D12Resource_GetDesc(resource);
    *w = desc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D && desc->Width > x ? desc->Width - x : 0;
    *h = desc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D && desc->Height > y ? desc->Height - y : 0;
}

static xess_result_t execute(struct xess_d3d12 *ctx, ID3D12GraphicsCommandList *list,
                             const xess_d3d12_execute_params_t *p, char *msg)
{
    static LONG logged, bad, too_small, failed, foreign, refused;
    D3D12_RESOURCE_DESC color, depth, motion, output, mask = {0};
    MTL_TEMPORAL_SCALER_D3D12_DESC want = {0};
    MTL_TEMPORAL_UPSCALE_D3D12_DESC d = {0};
    IMTLD3D12CommandListExt *ext;
    UINT cw, ch, dw, dh, mw, mh, kw = ~0u, kh = ~0u, iw, ih;
    uint32_t flags = ctx->init.initFlags;
    ID3D12Resource *reactive;
    xess_result_t ret;
    HRESULT hr;

    if (!ctx->initialized) return XESS_RESULT_ERROR_UNINITIALIZED;
    if (!list || !p || !p->pColorTexture || !p->pVelocityTexture || !p->pDepthTexture || !p->pOutputTexture ||
        !p->inputWidth || !p->inputHeight ||
        ctx->init.outputResolution.x > p->inputWidth * ctx->max_scale ||
        ctx->init.outputResolution.y > p->inputHeight * ctx->max_scale ||
        ctx->init.outputResolution.x < p->inputWidth * ctx->min_scale ||
        ctx->init.outputResolution.y < p->inputHeight * ctx->min_scale)
    {
        once(&bad, msg, "xessD3D12Execute: list %p, params %p, colour %p, velocity %p, depth %p, output %p, input %ux%u",
             list, p, p ? p->pColorTexture : NULL, p ? p->pVelocityTexture : NULL, p ? p->pDepthTexture : NULL,
             p ? p->pOutputTexture : NULL, p ? p->inputWidth : 0, p ? p->inputHeight : 0);
        return XESS_RESULT_ERROR_INVALID_ARGUMENT;
    }
    reactive = flags & XESS_INIT_FLAG_RESPONSIVE_PIXEL_MASK ? p->pResponsivePixelMaskTexture : NULL;
    extent(p->pColorTexture, &color, p->inputColorBase.x, p->inputColorBase.y, &cw, &ch);
    extent(p->pDepthTexture, &depth, p->inputDepthBase.x, p->inputDepthBase.y, &dw, &dh);
    extent(p->pVelocityTexture, &motion, p->inputMotionVectorBase.x, p->inputMotionVectorBase.y, &mw, &mh);
    output = ID3D12Resource_GetDesc(p->pOutputTexture);
    if (reactive)
        extent(reactive, &mask, p->inputResponsiveMaskBase.x, p->inputResponsiveMaskBase.y, &kw, &kh);
    if (!InterlockedExchange(&logged, 1))
        WARN("flags %#x, input %ux%u, output %ux%u; formats colour %u depth %u motion %u output %u mask %u; "
             "sizes colour %ux%u depth %ux%u motion %ux%u output %ux%u; bases colour %u,%u depth %u,%u "
             "motion %u,%u mask %u,%u output %u,%u; exposure texture %p, mask %p; jitter %f,%f scale %f,%f; "
             "velocity scale %f,%f; exposure %f; reset %u\n", flags, p->inputWidth, p->inputHeight,
             ctx->init.outputResolution.x, ctx->init.outputResolution.y, color.Format, depth.Format,
             motion.Format, output.Format, mask.Format, (UINT)color.Width, color.Height, (UINT)depth.Width, depth.Height,
             (UINT)motion.Width, motion.Height, (UINT)output.Width, output.Height, p->inputColorBase.x, p->inputColorBase.y,
             p->inputDepthBase.x, p->inputDepthBase.y, p->inputMotionVectorBase.x, p->inputMotionVectorBase.y,
             p->inputResponsiveMaskBase.x, p->inputResponsiveMaskBase.y, p->outputColorBase.x,
             p->outputColorBase.y, p->pExposureScaleTexture, p->pResponsivePixelMaskTexture, p->jitterOffsetX,
             p->jitterOffsetY, ctx->jitter_x, ctx->jitter_y, ctx->velocity_x, ctx->velocity_y, p->exposureScale,
             p->resetHistory);

    /* The upscaler for these textures: the one there is if they hold the region it reads, else one
     * reading as much of them as they share (no more than the output), so that a game's dynamic
     * resolution moves only the content size. */
    want.OutputWidth = ctx->init.outputResolution.x;
    want.OutputHeight = ctx->init.outputResolution.y;
    want.ColorFormat = color.Format;
    want.DepthFormat = depth.Format;
    want.MotionFormat = motion.Format;
    want.OutputFormat = output.Format;
    want.ReactiveMaskFormat = reactive ? mask.Format : DXGI_FORMAT_UNKNOWN;
    want.AutoExposure = !!(flags & XESS_INIT_FLAG_ENABLE_AUTOEXPOSURE);
    want.OutputResolutionMotionVectors = !!(flags & XESS_INIT_FLAG_HIGH_RES_MV);
    want.JitteredMotionVectors = !!(flags & XESS_INIT_FLAG_JITTERED_MV);
    if (want.OutputResolutionMotionVectors && (mw < want.OutputWidth || mh < want.OutputHeight))
        mw = mh = 0;
    else if (want.OutputResolutionMotionVectors)
        mw = mh = ~0u;  /* not an input-size texture */
    iw = min(min(min(cw, dw), min(mw, kw)), want.OutputWidth);
    ih = min(min(min(ch, dh), min(mh, kh)), want.OutputHeight);
    if (iw < p->inputWidth || ih < p->inputHeight)
    {
        once(&too_small, msg, "xessD3D12Execute: input %ux%u at bases (%u,%u) (%u,%u) (%u,%u) isn't in its textures",
             p->inputWidth, p->inputHeight, p->inputColorBase.x, p->inputColorBase.y, p->inputDepthBase.x,
             p->inputDepthBase.y, p->inputMotionVectorBase.x, p->inputMotionVectorBase.y);
        return XESS_RESULT_ERROR_INVALID_ARGUMENT;
    }
    want.InputWidth = ctx->scaler_desc.InputWidth;
    want.InputHeight = ctx->scaler_desc.InputHeight;
    if (!ctx->scaler || memcmp(&want, &ctx->scaler_desc, sizeof(want)) || iw < want.InputWidth ||
        ih < want.InputHeight || p->inputWidth > want.InputWidth || p->inputHeight > want.InputHeight)
    {
        release_scaler(ctx);
        want.InputWidth = iw;
        want.InputHeight = ih;
        if (FAILED(hr = IMTLD3D12DeviceExt_CreateTemporalScaler(ctx->ext, &want, &ctx->scaler)))
        {
            ctx->scaler = NULL;
            once(&failed, msg, "xessD3D12Execute: no MetalFX upscaler for %ux%u to %ux%u, formats colour %u "
                 "depth %u motion %u output %u mask %u: %#lx", iw, ih, want.OutputWidth, want.OutputHeight,
                 want.ColorFormat, want.DepthFormat, want.MotionFormat, want.OutputFormat,
                 want.ReactiveMaskFormat, hr);
            return result_of(hr);
        }
        ctx->scaler_desc = want;
    }

    d.Color = p->pColorTexture;
    d.Depth = p->pDepthTexture;
    d.MotionVector = p->pVelocityTexture;
    d.Exposure = flags & XESS_INIT_FLAG_EXPOSURE_SCALE_TEXTURE ? p->pExposureScaleTexture : NULL;
    d.ReactiveMask = reactive;
    d.Output = p->pOutputTexture;
    d.InputContentWidth = p->inputWidth;
    d.InputContentHeight = p->inputHeight;
    d.ColorOffsetX = p->inputColorBase.x;
    d.ColorOffsetY = p->inputColorBase.y;
    d.DepthOffsetX = p->inputDepthBase.x;
    d.DepthOffsetY = p->inputDepthBase.y;
    d.MotionOffsetX = p->inputMotionVectorBase.x;
    d.MotionOffsetY = p->inputMotionVectorBase.y;
    d.ReactiveOffsetX = p->inputResponsiveMaskBase.x;
    d.ReactiveOffsetY = p->inputResponsiveMaskBase.y;
    d.OutputOffsetX = p->outputColorBase.x;
    d.OutputOffsetY = p->outputColorBase.y;
    /* Intel's XeSS-SR Developer Guide 2.0: the jitter is the projection's, which moves the samples by
     * -jitter (MetalFX takes -(the sample offset)), and motion vectors point from the current frame to
     * the previous one, as MetalFX's do: both pass through unnegated (Ruling 2; Task 3 checks them
     * against Unreal's real values). */
    d.JitterOffsetX = p->jitterOffsetX * ctx->jitter_x;
    d.JitterOffsetY = p->jitterOffsetY * ctx->jitter_y;
    d.MotionVectorScaleX = ctx->velocity_x;
    d.MotionVectorScaleY = ctx->velocity_y;
    if (flags & XESS_INIT_FLAG_USE_NDC_VELOCITY)
    {
        /* NDC spans the motion vectors' region: the content, or the output with high-res ones */
        d.MotionVectorScaleX *= 0.5f * (want.OutputResolutionMotionVectors ? want.OutputWidth : p->inputWidth);
        d.MotionVectorScaleY *= -0.5f * (want.OutputResolutionMotionVectors ? want.OutputHeight : p->inputHeight);
    }
    /* Literal (0 with an exposure texture, as DXMT's DLSS bridge) until Task 3's log shows real
     * exposureScale values: XeSS may mean the inverse of MetalFX's preExposure (Ruling 4). */
    d.PreExposure = flags & XESS_INIT_FLAG_EXPOSURE_SCALE_TEXTURE ? 0.0f : p->exposureScale;
    d.Reset = !!p->resetHistory;
    d.DepthReversed = !!(flags & XESS_INIT_FLAG_INVERTED_DEPTH);

    if (FAILED(hr = ID3D12GraphicsCommandList_QueryInterface(list, &IID_IMTLD3D12CommandListExt, (void **)&ext)))
    {
        once(&foreign, msg, "xessD3D12Execute: command list %p isn't DXMT's: %#lx", list, hr);
        return result_of(hr);
    }
    hr = IMTLD3D12CommandListExt_TemporalUpscale(ext, ctx->scaler, &d);
    IMTLD3D12CommandListExt_Release(ext);
    if ((ret = result_of(hr)))
        once(&refused, msg, "xessD3D12Execute: DXMT refused the upscale (%ux%u to %ux%u): %#lx", p->inputWidth,
             p->inputHeight, want.OutputWidth, want.OutputHeight, hr);
    return ret;
}

xess_result_t CDECL d3d12_execute(xess_context_handle_t handle, ID3D12GraphicsCommandList *list,
                                  const xess_d3d12_execute_params_t *params)
{
    struct xess_d3d12 *ctx;
    char msg[512] = "";

    TRACE("%p, %p, %p\n", handle, list, params);
    if (!(ctx = lock_context(handle))) return XESS_RESULT_ERROR_INVALID_CONTEXT;
    return unlock_context(ctx, execute(ctx, list, params, msg), msg);
}

xess_result_t CDECL d3d12_get_optimal_input_resolution(xess_context_handle_t handle, const xess_2d_t *output,
        xess_quality_settings_t quality, xess_2d_t *optimal, xess_2d_t *min, xess_2d_t *max)
{
    struct xess_d3d12 *ctx;
    BOOL ok;

    if (!(ctx = lock_context(handle))) return XESS_RESULT_ERROR_INVALID_CONTEXT;
    ok = output && optimal && min && max && optimal_input(ctx, output, quality, optimal);
    if (ok)
    {
        /* the guide's dynamic range: from the mode's input to the output's size (Ruling 3: min is the
         * mode's optimal size, not output / 3) */
        *min = *optimal;
        *max = *output;
    }
    return unlock_context(ctx, ok ? XESS_RESULT_SUCCESS : XESS_RESULT_ERROR_INVALID_ARGUMENT, "");
}

xess_result_t CDECL d3d12_get_input_resolution(xess_context_handle_t handle, const xess_2d_t *output,
                                               xess_quality_settings_t quality, xess_2d_t *input)
{
    struct xess_d3d12 *ctx;
    BOOL ok;

    if (!(ctx = lock_context(handle))) return XESS_RESULT_ERROR_INVALID_CONTEXT;
    ok = output && input && optimal_input(ctx, output, quality, input);
    return unlock_context(ctx, ok ? XESS_RESULT_SUCCESS : XESS_RESULT_ERROR_INVALID_ARGUMENT, "");
}

/* Intel's XeFX library: 0.0.0 off Intel GPUs. */
xess_result_t CDECL d3d12_get_xefx_version(xess_context_handle_t handle, xess_version_t *version)
{
    struct xess_d3d12 *ctx;

    if (!(ctx = lock_context(handle))) return XESS_RESULT_ERROR_INVALID_CONTEXT;
    if (version) memset(version, 0, sizeof(*version));
    return unlock_context(ctx, version ? XESS_RESULT_SUCCESS : XESS_RESULT_ERROR_INVALID_ARGUMENT, "");
}

/* No descriptors or heaps from the game: MetalFX allocates its own. */
xess_result_t CDECL d3d12_get_properties(xess_context_handle_t handle, const xess_2d_t *output,
                                         xess_properties_t *properties)
{
    struct xess_d3d12 *ctx;

    if (!(ctx = lock_context(handle))) return XESS_RESULT_ERROR_INVALID_CONTEXT;
    if (properties) memset(properties, 0, sizeof(*properties));
    return unlock_context(ctx, output && properties ? XESS_RESULT_SUCCESS : XESS_RESULT_ERROR_INVALID_ARGUMENT, "");
}

xess_result_t CDECL d3d12_is_optimal_driver(xess_context_handle_t handle)
{
    struct xess_d3d12 *ctx;

    if (!(ctx = lock_context(handle))) return XESS_RESULT_ERROR_INVALID_CONTEXT;
    return unlock_context(ctx, XESS_RESULT_SUCCESS, "");
}

/* Called with the bridge's WARN-level messages when `level` is WARNING or lower. */
xess_result_t CDECL d3d12_set_logging_callback(xess_context_handle_t handle, xess_logging_level_t level,
                                               xess_app_log_callback_t callback)
{
    struct xess_d3d12 *ctx;

    if (!(ctx = lock_context(handle))) return XESS_RESULT_ERROR_INVALID_CONTEXT;
    ctx->log = callback;
    ctx->log_level = level;
    return unlock_context(ctx, XESS_RESULT_SUCCESS, "");
}

xess_result_t CDECL d3d12_set_jitter_scale(xess_context_handle_t handle, float x, float y)
{
    struct xess_d3d12 *ctx;

    if (!(ctx = lock_context(handle))) return XESS_RESULT_ERROR_INVALID_CONTEXT;
    ctx->jitter_x = x;
    ctx->jitter_y = y;
    return unlock_context(ctx, XESS_RESULT_SUCCESS, "");
}

xess_result_t CDECL d3d12_set_velocity_scale(xess_context_handle_t handle, float x, float y)
{
    struct xess_d3d12 *ctx;

    if (!(ctx = lock_context(handle))) return XESS_RESULT_ERROR_INVALID_CONTEXT;
    ctx->velocity_x = x;
    ctx->velocity_y = y;
    return unlock_context(ctx, XESS_RESULT_SUCCESS, "");
}

xess_result_t CDECL d3d12_get_jitter_scale(xess_context_handle_t handle, float *x, float *y)
{
    struct xess_d3d12 *ctx;

    if (!(ctx = lock_context(handle))) return XESS_RESULT_ERROR_INVALID_CONTEXT;
    if (!x || !y) return unlock_context(ctx, XESS_RESULT_ERROR_INVALID_ARGUMENT, "");
    *x = ctx->jitter_x;
    *y = ctx->jitter_y;
    return unlock_context(ctx, XESS_RESULT_SUCCESS, "");
}

xess_result_t CDECL d3d12_get_velocity_scale(xess_context_handle_t handle, float *x, float *y)
{
    struct xess_d3d12 *ctx;

    if (!(ctx = lock_context(handle))) return XESS_RESULT_ERROR_INVALID_CONTEXT;
    if (!x || !y) return unlock_context(ctx, XESS_RESULT_ERROR_INVALID_ARGUMENT, "");
    *x = ctx->velocity_x;
    *y = ctx->velocity_y;
    return unlock_context(ctx, XESS_RESULT_SUCCESS, "");
}
