#include "TerrainAlgorithms/HITerrainAlgorithm.h"
#include "TerrainDatas/HITerrainData.h"

void UHITerrainAlgorithm::Apply(UHITerrainData* Data)
{
	if(!bIsInited)
	{
		UE_LOG(LogHITerrain, Error, TEXT("UHITerrainAlgorithm::Apply Not Inited!"))
	}
}
