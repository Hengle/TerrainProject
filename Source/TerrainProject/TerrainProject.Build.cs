// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TerrainProject : ModuleRules
{
	public TerrainProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "ProceduralMeshComponent" });
		PublicDependencyModuleNames.AddRange(new string[] { "HITerrainMaker"});
	}
}
