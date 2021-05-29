// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "HITerrainModule.h"
#include "ShaderParameterStruct.h"


class HITERRAINMAKER_API FHITerrainErosionGPU: public FHITerrainModule
{
	public:
	FHITerrainErosionGPU();
	virtual ~FHITerrainErosionGPU() override;
	
	void SetNumIteration(int32 InNumIteration);
	void SetDeltaTime(float InDeltaTime);
	
	void SetEnableHydroErosion(bool InBool);
	void SetEnableThermalErosion(bool InBool);
	
	void SetRainAmount(float InRainAmount);
	void SetEvaporationAmount(float InEvaporationAmount);
	void SetErosionScale(float InErosionScale);
	void SetDepositionScale(float InDepositionScale);
	
	void SetThermalErosionScale(float InThermalErosionScale);
	
	virtual void ApplyModule(UHITerrainData* Data) override;

	void ApplyErosionShader(UHITerrainData* Data);

	private:
	int32 NumIteration;
	float DeltaTime;
	bool bEnableHydroErosion;
	bool bEnableThermalErosion;
	
	float RainAmount;
	float EvaporationAmount;
	float ErosionScale;
	float DepositionScale;

	float ThermalErosionScale;

	FTerrainRWBufferStructured TerrainData;
	FTerrainRWBufferStructured Flux;
	FTerrainRWBufferStructured TerrainFlux;
	FTerrainRWBufferStructured Velocity;
	FTerrainRWBufferStructured TempTerrainData;
};


class HITERRAINMAKER_API FErosionShaderRain: public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FErosionShaderRain);
	SHADER_USE_PARAMETER_STRUCT(FErosionShaderRain, FGlobalShader);

	public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	
	SHADER_PARAMETER(int, Size)
	SHADER_PARAMETER(float, DeltaTime)
	SHADER_PARAMETER(float, RainAmount)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float4>, TerrainData)
	
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

class HITERRAINMAKER_API FErosionShaderCalcFlow: public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FErosionShaderCalcFlow);
	SHADER_USE_PARAMETER_STRUCT(FErosionShaderCalcFlow, FGlobalShader);

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

class HITERRAINMAKER_API FErosionShaderApplyFlow: public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FErosionShaderApplyFlow);
	SHADER_USE_PARAMETER_STRUCT(FErosionShaderApplyFlow, FGlobalShader);

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

class HITERRAINMAKER_API FErosionShaderCalcErosionDeposition: public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FErosionShaderCalcErosionDeposition);
	SHADER_USE_PARAMETER_STRUCT(FErosionShaderCalcErosionDeposition, FGlobalShader);

	public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	
	SHADER_PARAMETER(int, Size)
	SHADER_PARAMETER(float, DeltaTime)
	SHADER_PARAMETER(float, ErosionScale)
	SHADER_PARAMETER(float, DepositionScale)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float4>, TerrainData)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float4>, Flux)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float2>, Velocity)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float4>, TempTerrainData)
	
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

class HITERRAINMAKER_API FErosionShaderApplyErosionDeposition: public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FErosionShaderApplyErosionDeposition);
	SHADER_USE_PARAMETER_STRUCT(FErosionShaderApplyErosionDeposition, FGlobalShader);

	public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	
	SHADER_PARAMETER(int, Size)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float4>, TerrainData)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float4>, TempTerrainData)
	
	END_SHADER_PARAMETER_STRUCT()
	
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}
	static bool ValidateCompiledResult(EShaderPlatform InPlatform, const FShaderParameterMap& InParameterMap, TArray<FString>& OutError)
	{
		return true;
	}
};

class HITERRAINMAKER_API FErosionShaderSedimentFlow: public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FErosionShaderSedimentFlow);
	SHADER_USE_PARAMETER_STRUCT(FErosionShaderSedimentFlow, FGlobalShader);

	public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	
	SHADER_PARAMETER(int, Size)
	SHADER_PARAMETER(float, DeltaTime)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float4>, TerrainData)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float2>, Velocity)
	
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

class HITERRAINMAKER_API FErosionShaderEvaporation: public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FErosionShaderEvaporation);
	SHADER_USE_PARAMETER_STRUCT(FErosionShaderEvaporation, FGlobalShader);

	public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	
	SHADER_PARAMETER(int, Size)
	SHADER_PARAMETER(float, DeltaTime)
	SHADER_PARAMETER(float, EvaporationAmount)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float4>, TerrainData)
	
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

class HITERRAINMAKER_API FErosionShaderCalcThermal: public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FErosionShaderCalcThermal);
	SHADER_USE_PARAMETER_STRUCT(FErosionShaderCalcThermal, FGlobalShader);

	public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	
	SHADER_PARAMETER(int, Size)
	SHADER_PARAMETER(float, DeltaTime)
	SHADER_PARAMETER(float, ThermalErosionScale)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float4>, TerrainData)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float4>, TerrainFlux)
	
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

class HITERRAINMAKER_API FErosionShaderApplyThermal: public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FErosionShaderApplyThermal);
	SHADER_USE_PARAMETER_STRUCT(FErosionShaderApplyThermal, FGlobalShader);

	public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	
	SHADER_PARAMETER(int, Size)
	SHADER_PARAMETER(float, DeltaTime)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float4>, TerrainData)
	SHADER_PARAMETER_UAV(RWStructuredBuffer<float4>, TerrainFlux)
	
	END_SHADER_PARAMETER_STRUCT()
	
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static inline void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

	static bool ValidateCompiledResult(EShaderPlatform InPlatform, const FShaderParameterMap& InParameterMap, TArray<FString>& OutError)
	{
		return true;
	}
};
