#include "HITerrainProviderBase.h"

void UHITerrainProviderBase::Initialize()
{	
	FRuntimeMeshLODProperties LODProperties;
	LODProperties.ScreenSize = 0.0f;

	ConfigureLODs({ LODProperties });

	SetupMaterialSlot(0, FName("Cube Base"), Material);

	FRuntimeMeshSectionProperties Properties;
	Properties.bCastsShadow = true;
	Properties.bIsVisible = true;
	Properties.MaterialSlot = 0;
	Properties.UpdateFrequency = ERuntimeMeshUpdateFrequency::Infrequent;
	CreateSection(0, 0, Properties);

	MarkCollisionDirty();
}





void UHITerrainProviderBase::SetData(UHITerrainDataBase* InData)
{
	Data = InData;
}

void UHITerrainProviderBase::SetSize(int32 InSize) 
{
	Size = InSize;
}

int32 UHITerrainProviderBase::GetSize() 
{
	return Size;
}

void UHITerrainProviderBase::SetStep(float InStep)
{
	Step = InStep;
}

float UHITerrainProviderBase::GetStep()
{
	return Step;
}
