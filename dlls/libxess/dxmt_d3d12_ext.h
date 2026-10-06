/*
 * DXMT's private D3D12 interface to MetalFX's temporal upscaler (a C copy of DXMT's
 * src/d3d12/d3d12_interfaces.hpp, MacNeutron's DXMT patch 0004): the IIDs, the method order
 * after IUnknown's and the structs field for field never change.
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

#ifndef __WINE_DLLS_LIBXESS_DXMT_D3D12_EXT_H
#define __WINE_DLLS_LIBXESS_DXMT_D3D12_EXT_H

DEFINE_GUID(IID_IMTLD3D12TemporalScaler, 0x5b27acde, 0xa0d8, 0x4677, 0x93, 0xc4, 0xee, 0xdd, 0x17, 0xa0, 0xab, 0x14);
DEFINE_GUID(IID_IMTLD3D12DeviceExt, 0xa8c64ba7, 0x6d54, 0x4833, 0x8c, 0xc6, 0x26, 0x05, 0xc5, 0xb4, 0x54, 0xbe);
DEFINE_GUID(IID_IMTLD3D12CommandListExt, 0x5690e8a7, 0x51a1, 0x4b37, 0x8b, 0xbd, 0x73, 0x0d, 0x0f, 0xdb, 0x2c, 0xec);

typedef struct MTL_TEMPORAL_SCALER_D3D12_DESC
{
    UINT InputWidth, InputHeight;
    UINT OutputWidth, OutputHeight;
    DXGI_FORMAT ColorFormat, DepthFormat, MotionFormat, OutputFormat;
    DXGI_FORMAT ReactiveMaskFormat;  /* DXGI_FORMAT_UNKNOWN: no reactive mask */
    BOOL AutoExposure, OutputResolutionMotionVectors, JitteredMotionVectors;
    FLOAT InputContentMinScale, InputContentMaxScale;  /* 0: the device's bound */
} MTL_TEMPORAL_SCALER_D3D12_DESC;

typedef struct MTL_TEMPORAL_UPSCALE_D3D12_DESC
{
    ID3D12Resource *Color, *Depth, *MotionVector, *Exposure /* may be null */, *ReactiveMask /* may be null */, *Output;
    UINT InputContentWidth, InputContentHeight;
    UINT ColorOffsetX, ColorOffsetY, DepthOffsetX, DepthOffsetY, MotionOffsetX, MotionOffsetY,
         ReactiveOffsetX, ReactiveOffsetY, OutputOffsetX, OutputOffsetY;
    FLOAT JitterOffsetX, JitterOffsetY, MotionVectorScaleX, MotionVectorScaleY, PreExposure;
    BOOL Reset, DepthReversed;
} MTL_TEMPORAL_UPSCALE_D3D12_DESC;

#define INTERFACE IMTLD3D12TemporalScaler
DECLARE_INTERFACE_(IMTLD3D12TemporalScaler, IUnknown)
{
    STDMETHOD_(HRESULT, QueryInterface)(THIS_ REFIID riid, void **object) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
};
#undef INTERFACE

#define INTERFACE IMTLD3D12DeviceExt
DECLARE_INTERFACE_(IMTLD3D12DeviceExt, IUnknown)
{
    STDMETHOD_(HRESULT, QueryInterface)(THIS_ REFIID riid, void **object) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(GetTemporalScalerScaleRange)(THIS_ FLOAT *min, FLOAT *max) PURE;
    STDMETHOD(CreateTemporalScaler)(THIS_ const MTL_TEMPORAL_SCALER_D3D12_DESC *desc,
            IMTLD3D12TemporalScaler **scaler) PURE;
};
#undef INTERFACE

#define INTERFACE IMTLD3D12CommandListExt
DECLARE_INTERFACE_(IMTLD3D12CommandListExt, IUnknown)
{
    STDMETHOD_(HRESULT, QueryInterface)(THIS_ REFIID riid, void **object) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;
    STDMETHOD(TemporalUpscale)(THIS_ IMTLD3D12TemporalScaler *scaler, const MTL_TEMPORAL_UPSCALE_D3D12_DESC *desc) PURE;
};
#undef INTERFACE

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IMTLD3D12TemporalScaler_Release(p) (p)->lpVtbl->Release(p)
#define IMTLD3D12DeviceExt_Release(p) (p)->lpVtbl->Release(p)
#define IMTLD3D12DeviceExt_GetTemporalScalerScaleRange(p,a,b) (p)->lpVtbl->GetTemporalScalerScaleRange(p,a,b)
#define IMTLD3D12DeviceExt_CreateTemporalScaler(p,a,b) (p)->lpVtbl->CreateTemporalScaler(p,a,b)
#define IMTLD3D12CommandListExt_Release(p) (p)->lpVtbl->Release(p)
#define IMTLD3D12CommandListExt_TemporalUpscale(p,a,b) (p)->lpVtbl->TemporalUpscale(p,a,b)
#endif

#endif /* __WINE_DLLS_LIBXESS_DXMT_D3D12_EXT_H */
