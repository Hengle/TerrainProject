#include "HITerrainProviderBase.h"

void UHITerrainProviderBase::Initialize()
{	
	FRuntimeMeshLODProperties LODProperties;
	LODProperties.ScreenSize = 0.0f;

	ConfigureLODs({ LODProperties });

	SetupMaterialSlot(0, FName("Material"), Material);

	FRuntimeMeshSectionProperties Properties;
	Properties.bCastsShadow = true;
	Properties.bIsVisible = true;
	Properties.MaterialSlot = 0;
	Properties.UpdateFrequency = ERuntimeMeshUpdateFrequency::Infrequent;
	CreateSection(0, 0, Properties);

	MarkCollisionDirty();
}





void UHITerrainProviderBase::SetData(UHITerrainData* InData)
{
	Data = InData;
	MarkAllLODsDirty();
}

void UHITerrainProviderBase::SetSize(int32 InSize) 
{
	Size = InSize;
	MarkAllLODsDirty();
}

int32 UHITerrainProviderBase::GetSize() 
{
	return Size;
}

void UHITerrainProviderBase::SetStep(float InStep)
{
	Step = InStep;
	MarkAllLODsDirty();
}

float UHITerrainProviderBase::GetStep()
{
	return Step;
}
