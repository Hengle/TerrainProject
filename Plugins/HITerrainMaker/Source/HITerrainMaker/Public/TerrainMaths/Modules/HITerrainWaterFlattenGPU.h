﻿#pragma once

#include "CoreMinimal.h"
#include "ShaderParameterStruct.h"
#include "HITerrainModule.h"

class HITERRAINMAKER_API FWaterFlattenShader: public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FWaterFlattenShader);
	SHADER_USE_PARAMETER_STRUCT(FWaterFlattenShader, FGlobalShader);

	public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	
	SHADER_PARAMETER(int, Size)
	SHADER_PARAMETER(float, DeltaTime)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float4>, TerrainData)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float4>, Flux)
	
	END_SHADER_PARAMETER_STRUCT()


	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};

class HITERRAINMAKER_API FWaterFlattenShader2: public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FWaterFlattenShader2);
	SHADER_USE_PARAMETER_STRUCT(FWaterFlattenShader2, FGlobalShader);

	public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	
	SHADER_PARAMETER(int, Size)
	SHADER_PARAMETER(float, DeltaTime)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float4>, TerrainData)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float4>, Flux)
	
	END_SHADER_PARAMETER_STRUCT()


	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
};

class HITERRAINMAKER_API FHITerrainWaterFlattenGPU : public FHITerrainModule
{
public:
	FHITerrainWaterFlattenGPU();
	
	virtual void ApplyModule(UHITerrainData* Data) override;

	void ApplyWaterFlattenShader(UHITerrainData* Data);

	void SetNumIteration(int32 InNumIteration);
	void SetDeltaTime(float InDeltaTime);

private:
	int32 NumIteration;
	float DeltaTime;

	FTerrainRWBufferStructured TerrainData;
	FTerrainRWBufferStructured Flux;
};
