// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Inno_Racing : ModuleRules
{
	public Inno_Racing(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput",
			"CommonUI",
			"UMG",
			"Slate",
			"SlateCore",
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"Niagara",
			"PhysicsCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { 
			"OnlineSubsystemNull"
		});
	}
}
