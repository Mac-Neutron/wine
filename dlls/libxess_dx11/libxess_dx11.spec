# The exports of Intel's libxess_dx11.dll (XeSS SDK 2.0.1), in its order. Arguments are listed only
# where the stand-in reads them (main.c).
@ cdecl INTC_D3D11_XessCreateContext() xess_unsupported
@ cdecl INTC_D3D11_XessDestroyContext() xess_unsupported
@ cdecl INTC_D3D11_XessExecute() xess_unsupported
@ cdecl INTC_D3D11_XessForceLegacyScaleFactors() xess_unsupported
@ cdecl INTC_D3D11_XessGetExposureMultiplier() xess_unsupported
@ cdecl INTC_D3D11_XessGetInitParams() xess_unsupported
@ cdecl INTC_D3D11_XessGetInputResolution() xess_unsupported
@ cdecl INTC_D3D11_XessGetIntelXeFXVersion() xess_unsupported
@ cdecl INTC_D3D11_XessGetJitterScale() xess_unsupported
@ cdecl INTC_D3D11_XessGetOptimalInputResolution() xess_unsupported
@ cdecl INTC_D3D11_XessGetProfilingData() xess_unsupported
@ cdecl INTC_D3D11_XessGetVelocityScale() xess_unsupported
@ cdecl INTC_D3D11_XessGetVersion() xess_unsupported
@ cdecl INTC_D3D11_XessInit() xess_unsupported
@ cdecl INTC_D3D11_XessIsOptimalDriver() xess_unsupported
@ cdecl INTC_D3D11_XessSelectNetworkModel() xess_unsupported
@ cdecl INTC_D3D11_XessSetExposureMultiplier() xess_unsupported
@ cdecl INTC_D3D11_XessSetJitterScale() xess_unsupported
@ cdecl INTC_D3D11_XessSetLoggingCallback() xess_unsupported
@ cdecl INTC_D3D11_XessSetVelocityScale() xess_unsupported
@ cdecl INTC_D3D12_GetApplicationInfo() xess_unsupported
@ cdecl INTC_D3D12_GetGpuCrashDumpBufferInfo() xess_unsupported
@ cdecl INTC_D3D12_GpuCrashDumpOnCommandListCreate() xess_unsupported
@ cdecl INTC_D3D12_GpuCrashDumpOnCommandListDestroy() xess_unsupported
@ cdecl INTC_D3D12_GpuCrashDumpOnDeviceDestroy() xess_unsupported
@ cdecl INTC_D3D12_RegisterDriverCallbacks() xess_unsupported
@ cdecl xessD3D11CreateContext(ptr ptr) xess_create_context
@ cdecl xessD3D11Execute() xess_invalid_context
@ cdecl xessD3D11GetInitParams() xess_invalid_context
@ cdecl xessD3D11Init() xess_invalid_context
@ cdecl xessDestroyContext(ptr)
@ cdecl xessForceLegacyScaleFactors() xess_invalid_context
@ cdecl xessGetContextParameterP() xess_invalid_context
@ cdecl xessGetExposureMultiplier() xess_invalid_context
@ cdecl xessGetInputResolution() xess_invalid_context
@ cdecl xessGetIntelXeFXVersion() xess_invalid_context
@ cdecl xessGetJitterScale() xess_invalid_context
@ cdecl xessGetOptimalInputResolution() xess_invalid_context
@ cdecl xessGetPipelineBuildStatus() xess_invalid_context
@ cdecl xessGetProfilingData() xess_invalid_context
@ cdecl xessGetProperties() xess_invalid_context
@ cdecl xessGetVelocityScale() xess_invalid_context
@ cdecl xessGetVersion(ptr)
@ cdecl xessIsOptimalDriver() xess_unsupported
@ cdecl xessSelectNetworkModel() xess_invalid_context
@ cdecl xessSetContextParameterF() xess_invalid_context
@ cdecl xessSetExposureMultiplier() xess_invalid_context
@ cdecl xessSetJitterScale() xess_invalid_context
@ cdecl xessSetLoggingCallback() xess_invalid_context
@ cdecl xessSetVelocityScale() xess_invalid_context
@ cdecl xessStartDump() xess_invalid_context
