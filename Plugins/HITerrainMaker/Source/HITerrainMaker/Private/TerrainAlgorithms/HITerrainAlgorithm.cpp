#include "TerrainAlgorithms/HITerrainAlgorithm.h"
#include "TerrainDatas/HITerrainData.h"

void UHITerrainAlgorithm::Init(FTerrainInformationPtr InInformation)
{
	for(UHITerrainAlgorithm* SubAlgorithm: SubAlgorithms)
	{
		SubAlgorithm->Init(InInformation);
	}
	Information = InInformation;
	bIsInited = true;
}

void UHITerrainAlgorithm::Apply(UHITerrainData* Data)
{
	if(!bIsInited)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainAlgorithm::Apply Not Inited!"))
	}
	if(Information == nullptr)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainAlgorithm::Apply No TerrainInformation!"))
	}
}

void UHITerrainAlgorithm::DebugApply(UHITerrainData* Data)
{
	if(!bIsInited)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainAlgorithm::DebugApply Not Inited!"))
	}
	if(Information == nullptr)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainAlgorithm::DebugApply No TerrainInformation!"))
	}
}
