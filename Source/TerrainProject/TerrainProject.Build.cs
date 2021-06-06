/*
* This is an independent project of an individual developer. Dear PVS-Studio, please check it.
* PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
*/
// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TerrainProject : ModuleRules
{
	public TerrainProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "UMG"});
		PublicDependencyModuleNames.AddRange(new string[] { "HITerrainMaker", "Slate", "SlateCore"});
	}
}
