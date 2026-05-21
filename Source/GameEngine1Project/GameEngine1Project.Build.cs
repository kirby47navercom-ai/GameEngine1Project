// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.IO;
public class GameEngine1Project : ModuleRules
{
	public GameEngine1Project(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore","Voice", "AudioCapture" ,"AudioCaptureCore" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

        // Whisper 서드파티 라이브러리 경로 설정
        string WhisperPath = Path.Combine(ModuleDirectory, "ThirdParty", "Whisper");

        // 1. 헤더 파일(.h)이 있는 Include 폴더 연결
        PublicIncludePaths.Add(Path.Combine(WhisperPath, "Include"));

        // 2. 라이브러리 파일(.lib)이 있는 Lib 폴더 연결 (파일 이름이 whisper.lib가 맞는지 확인!)
        PublicAdditionalLibraries.Add(Path.Combine(WhisperPath, "Lib", "whisper.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(WhisperPath, "Lib", "ggml.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(WhisperPath, "Lib", "ggml-cpu.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(WhisperPath, "Lib", "ggml-base.lib"));
        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
