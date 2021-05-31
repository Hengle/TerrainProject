#include "TerrainAlgorithms/MidtermAlgorithms/RidgedMultiAlgorithm.h"

#include "TerrainDatas/HITerrainData.h"

void URidgedMultiAlgorithm::Init(FTerrainInformationPtr InInformation)
{
    Super::Init(InInformation);
    RiverPositions_RidgedMulti.SetSeed (Information->Seed + 100);
    RiverPositions_RidgedMulti.SetFrequency (0.5);
    RiverPositions_RidgedMulti.SetLacunarity (2.0f);
    RiverPositions_RidgedMulti.SetOctaveCount (1);
    RiverPositions_RidgedMulti.SetNoiseQuality (noise::QUALITY_BEST);

    
    RiverPositions_Curve.SetSourceModule (0, RiverPositions_RidgedMulti);
    RiverPositions_Curve.AddControlPoint (-2.000,  2.000);
    RiverPositions_Curve.AddControlPoint (-1.000,  1.000);
    RiverPositions_Curve.AddControlPoint (-0.125,  0.875);
    RiverPositions_Curve.AddControlPoint ( 0.000, -1.000);
    RiverPositions_Curve.AddControlPoint ( 1.000, -1.500);
    RiverPositions_Curve.AddControlPoint ( 2.000, -2.000);
    
    RiverPositions.SetSourceModule (0, RiverPositions_Curve);
}

void URidgedMultiAlgorithm::ApplyAlgorithm(UHITerrainData* Data)
{
    Super::ApplyAlgorithm(Data);
    int32 Size = Data->Size();
    for(int32 i = 0; i < Size; i++)
    {
        for(int32 j = 0; j < Size; j++)
        {
            float Value = 0.0f;
            Value = RiverPositions.GetValue(i * 0.01f, j * 0.01f, 0.0f) * 2000;
            Data->SetHeightValue(i, j, Value);
        }
    }
}

void URidgedMultiAlgorithm::DebugAlgorithm(UHITerrainData* Data)
{
    Super::DebugAlgorithm(Data);
    int32 Size = Data->Size();
    for(int32 i = 0; i < Size; i++)
    {
        for(int32 j = 0; j < Size; j++)
        {
            float Value = 0.0f;
            Value = RiverPositions.GetValue(i * 0.01f, j * 0.01f, 0.0f) * 2000;
            Data->SetHeightValue(i, j, Value);
        }
    }
}
