// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "HITerrainModule.h"
#include "ShaderParameterStruct.h"

class HITERRAINMAKER_API FErosionShader: public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FErosionShader);
	SHADER_USE_PARAMETER_STRUCT(FErosionShader, FGlobalShader);

public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	
	SHADER_PARAMETER(int, Size)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float>, TerrainHeight)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float>, TerrainWater)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float>, TerrainSediment)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float>, TerrainHardness)
	
	END_SHADER_PARAMETER_STRUCT()

	

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

private:
};

class HITERRAINMAKER_API FHITerrainErosionGPU: public FHITerrainModule
{
public:
	virtual void ApplyModule(UHITerrainData* Data) override;

	void ApplyErosionShader(UHITerrainData* Data);
};
