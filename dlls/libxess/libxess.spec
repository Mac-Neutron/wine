# The exports of Intel's libxess.dll (XeSS SDK 2.0.1), in its order. Arguments are listed only
# where the stand-ins read them (main.c) or the D3D12 bridge implements the call (d3d12.c).
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
@ cdecl xessAILGetDecision() xess_unsupported
@ cdecl xessAILGetVersion() xess_unsupported
@ cdecl xessD3D12BuildPipelines(ptr ptr long long) d3d12_build_pipelines
@ cdecl xessD3D12CreateContext(ptr ptr) d3d12_create_context
@ cdecl xessD3D12Execute(ptr ptr ptr) d3d12_execute
@ cdecl xessD3D12GetInitParams(ptr ptr) d3d12_get_init_params
@ cdecl xessD3D12GetProfilingData(ptr) d3d12_not_implemented
@ cdecl xessD3D12GetResourcesToDump(ptr) d3d12_not_implemented
@ cdecl xessD3D12Init(ptr ptr) d3d12_init
@ cdecl xessDestroyContext(ptr) d3d12_destroy_context
@ cdecl xessForceLegacyScaleFactors(ptr) d3d12_not_implemented
@ cdecl xessGetContextParameterP() xess_invalid_context
@ cdecl xessGetExposureMultiplier(ptr) d3d12_not_implemented
@ cdecl xessGetInputResolution(ptr ptr long ptr) d3d12_get_input_resolution
@ cdecl xessGetIntelXeFXVersion(ptr ptr) d3d12_get_xefx_version
@ cdecl xessGetJitterScale(ptr ptr ptr) d3d12_get_jitter_scale
@ cdecl xessGetMaxResponsiveMaskValue(ptr) d3d12_not_implemented
@ cdecl xessGetOptimalInputResolution(ptr ptr long ptr ptr ptr) d3d12_get_optimal_input_resolution
@ cdecl xessGetPipelineBuildStatus(ptr) d3d12_get_pipeline_build_status
@ cdecl xessGetProfilingData(ptr) d3d12_not_implemented
@ cdecl xessGetProperties(ptr ptr ptr) d3d12_get_properties
@ cdecl xessGetVelocityScale(ptr ptr ptr) d3d12_get_velocity_scale
@ cdecl xessGetVersion(ptr)
@ cdecl xessIsOptimalDriver(ptr) d3d12_is_optimal_driver
@ cdecl xessSelectNetworkModel(ptr) d3d12_not_implemented
@ cdecl xessSetContextParameterF() xess_invalid_context
@ cdecl xessSetExposureMultiplier(ptr) d3d12_not_implemented
@ cdecl xessSetJitterScale(ptr float float) d3d12_set_jitter_scale
@ cdecl xessSetLoggingCallback(ptr long ptr) d3d12_set_logging_callback
@ cdecl xessSetMaxResponsiveMaskValue(ptr) d3d12_not_implemented
@ cdecl xessSetVelocityScale(ptr float float) d3d12_set_velocity_scale
@ cdecl xessStartDump(ptr) d3d12_not_implemented
@ cdecl xessTrinityCreateContext() xess_unsupported
@ cdecl xessTrinityGetRequiredDeviceExtensions() xess_unsupported
@ cdecl xessTrinityGetRequiredDeviceFeatures() xess_unsupported
@ cdecl xessTrinityGetRequiredInstanceExtensions() xess_unsupported
@ cdecl xessVKAILGetDecision() xess_unsupported
@ cdecl xessVKBuildPipelines() xess_invalid_context
@ cdecl xessVKCreateContext(ptr ptr ptr ptr) xess_vk_create_context
@ cdecl xessVKExecute() xess_invalid_context
@ cdecl xessVKGetInitParams() xess_invalid_context
@ cdecl xessVKGetRequiredDeviceExtensions() xess_unsupported
@ cdecl xessVKGetRequiredDeviceFeatures() xess_unsupported
@ cdecl xessVKGetRequiredInstanceExtensions() xess_unsupported
@ cdecl xessVKGetResourcesToDump() xess_invalid_context
@ cdecl xessVKInit() xess_invalid_context
