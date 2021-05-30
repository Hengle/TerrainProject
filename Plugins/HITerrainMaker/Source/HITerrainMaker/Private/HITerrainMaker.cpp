#include "HITerrainMaker.h"

#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FHITerrainMakerModule"

void FHITerrainMakerModule::StartupModule()
{
	FString ShaderDictionary = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("HITerrainMaker"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping("/TerrainShaders", ShaderDictionary);
}

void FHITerrainMakerModule::ShutdownModule()
{

}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FHITerrainMakerModule, HITerrainMaker)