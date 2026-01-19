// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TPS_Hacker : ModuleRules
{
	public TPS_Hacker(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"TPS_Hacker",
			"TPS_Hacker/Variant_Platforming",
			"TPS_Hacker/Variant_Platforming/Animation",
			"TPS_Hacker/Variant_Combat",
			"TPS_Hacker/Variant_Combat/AI",
			"TPS_Hacker/Variant_Combat/Animation",
			"TPS_Hacker/Variant_Combat/Gameplay",
			"TPS_Hacker/Variant_Combat/Interfaces",
			"TPS_Hacker/Variant_Combat/UI",
			"TPS_Hacker/Variant_SideScrolling",
			"TPS_Hacker/Variant_SideScrolling/AI",
			"TPS_Hacker/Variant_SideScrolling/Gameplay",
			"TPS_Hacker/Variant_SideScrolling/Interfaces",
			"TPS_Hacker/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
