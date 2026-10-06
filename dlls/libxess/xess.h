/*
 * The XeSS declarations libxess's D3D12 bridge implements, copied from Intel's public headers
 * inc/xess/xess.h and inc/xess/xess_d3d12.h (github.com/intel/xess, branch main), which carry
 * this notice:
 *
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
 * to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __WINE_DLLS_LIBXESS_XESS_H
#define __WINE_DLLS_LIBXESS_XESS_H

#include <stdint.h>

typedef struct _xess_context_handle_t* xess_context_handle_t;

#pragma pack(push, 8)
typedef struct _xess_version_t
{
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
    uint16_t reserved;
} xess_version_t;
#pragma pack(pop)

#pragma pack(push, 8)
typedef struct _xess_2d_t
{
    uint32_t x;
    uint32_t y;
} xess_2d_t;
#pragma pack(pop)

typedef xess_2d_t xess_coord_t;

typedef enum _xess_quality_settings_t
{
    XESS_QUALITY_SETTING_ULTRA_PERFORMANCE = 100,
    XESS_QUALITY_SETTING_PERFORMANCE = 101,
    XESS_QUALITY_SETTING_BALANCED = 102,
    XESS_QUALITY_SETTING_QUALITY = 103,
    XESS_QUALITY_SETTING_ULTRA_QUALITY = 104,
    XESS_QUALITY_SETTING_ULTRA_QUALITY_PLUS = 105,
    XESS_QUALITY_SETTING_AA = 106,
} xess_quality_settings_t;

typedef enum _xess_init_flags_t
{
    XESS_INIT_FLAG_NONE = 0,
    XESS_INIT_FLAG_HIGH_RES_MV = 1 << 0,
    XESS_INIT_FLAG_INVERTED_DEPTH = 1 << 1,
    XESS_INIT_FLAG_EXPOSURE_SCALE_TEXTURE = 1 << 2,
    XESS_INIT_FLAG_RESPONSIVE_PIXEL_MASK = 1 << 3,
    XESS_INIT_FLAG_USE_NDC_VELOCITY = 1 << 4,
    XESS_INIT_FLAG_EXTERNAL_DESCRIPTOR_HEAP = 1 << 5,
    XESS_INIT_FLAG_LDR_INPUT_COLOR = 1 << 6,
    XESS_INIT_FLAG_JITTERED_MV = 1 << 7,
    XESS_INIT_FLAG_ENABLE_AUTOEXPOSURE = 1 << 8
} xess_init_flags_t;

#pragma pack(push, 8)
typedef struct _xess_properties_t
{
    uint32_t requiredDescriptorCount;
    uint64_t tempBufferHeapSize;
    uint64_t tempTextureHeapSize;
} xess_properties_t;
#pragma pack(pop)

typedef enum _xess_result_t
{
    XESS_RESULT_WARNING_NONEXISTING_FOLDER = 1,
    XESS_RESULT_WARNING_OLD_DRIVER = 2,
    XESS_RESULT_SUCCESS = 0,
    XESS_RESULT_ERROR_UNSUPPORTED_DEVICE = -1,
    XESS_RESULT_ERROR_UNSUPPORTED_DRIVER = -2,
    XESS_RESULT_ERROR_UNINITIALIZED = -3,
    XESS_RESULT_ERROR_INVALID_ARGUMENT = -4,
    XESS_RESULT_ERROR_DEVICE_OUT_OF_MEMORY = -5,
    XESS_RESULT_ERROR_DEVICE = -6,
    XESS_RESULT_ERROR_NOT_IMPLEMENTED = -7,
    XESS_RESULT_ERROR_INVALID_CONTEXT = -8,
    XESS_RESULT_ERROR_OPERATION_IN_PROGRESS = -9,
    XESS_RESULT_ERROR_UNSUPPORTED = -10,
    XESS_RESULT_ERROR_CANT_LOAD_LIBRARY = -11,
    XESS_RESULT_ERROR_WRONG_CALL_ORDER = -12,

    XESS_RESULT_ERROR_UNKNOWN = -1000,
} xess_result_t;

typedef enum _xess_logging_level_t
{
    XESS_LOGGING_LEVEL_DEBUG = 0,
    XESS_LOGGING_LEVEL_INFO = 1,
    XESS_LOGGING_LEVEL_WARNING = 2,
    XESS_LOGGING_LEVEL_ERROR = 3
} xess_logging_level_t;

typedef void (*xess_app_log_callback_t)(const char *message, xess_logging_level_t loggingLevel);

#pragma pack(push, 8)
typedef struct _xess_d3d12_execute_params_t
{
    ID3D12Resource *pColorTexture;
    ID3D12Resource *pVelocityTexture;
    ID3D12Resource *pDepthTexture;
    ID3D12Resource *pExposureScaleTexture;
    ID3D12Resource *pResponsivePixelMaskTexture;
    ID3D12Resource *pOutputTexture;
    float jitterOffsetX;
    float jitterOffsetY;
    float exposureScale;
    uint32_t resetHistory;
    uint32_t inputWidth;
    uint32_t inputHeight;
    xess_coord_t inputColorBase;
    xess_coord_t inputMotionVectorBase;
    xess_coord_t inputDepthBase;
    xess_coord_t inputResponsiveMaskBase;
    xess_coord_t reserved0;
    xess_coord_t outputColorBase;
    ID3D12DescriptorHeap* pDescriptorHeap;
    uint32_t descriptorHeapOffset;
} xess_d3d12_execute_params_t;
#pragma pack(pop)

#pragma pack(push, 8)
typedef struct _xess_d3d12_init_params_t
{
    xess_2d_t outputResolution;
    xess_quality_settings_t qualitySetting;
    uint32_t initFlags;
    uint32_t creationNodeMask;
    uint32_t visibleNodeMask;
    ID3D12Heap* pTempBufferHeap;
    uint64_t bufferHeapOffset;
    ID3D12Heap* pTempTextureHeap;
    uint64_t textureHeapOffset;
    ID3D12PipelineLibrary *pPipelineLibrary;
} xess_d3d12_init_params_t;
#pragma pack(pop)

#endif /* __WINE_DLLS_LIBXESS_XESS_H */
