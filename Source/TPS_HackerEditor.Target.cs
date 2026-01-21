// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TPS_HackerEditorTarget : TargetRules
{
	public TPS_HackerEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
		ExtraModuleNames.Add("TPS_Hacker");
	}
}