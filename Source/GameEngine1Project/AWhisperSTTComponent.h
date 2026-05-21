// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <string>
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AudioCaptureCore.h"
#include "Async/Future.h"

THIRD_PARTY_INCLUDES_START
#include "whisper.h"
THIRD_PARTY_INCLUDES_END

#include "AWhisperSTTComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpeechRecognized, FString, RecognizedText);
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GAMEENGINE1PROJECT_API AAWhisperSTTComponent : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAWhisperSTTComponent();
	// 🚨 [추가 2] 블루프린트에서 노드로 꺼내 쓸 수 있는 이벤트 디스패처
	UPROPERTY(BlueprintAssignable, Category = "Whisper AI")
	FOnSpeechRecognized OnSpeechRecognizedEvent;

	UFUNCTION(BlueprintCallable, Category = "Whisper AI")
	bool InitWhisperModel(const FString& ModelPath);

	UFUNCTION(BlueprintCallable, Category = "Whisper AI")
	FString RecognizeSpeech(const TArray<float>& AudioSamples);

	UFUNCTION(BlueprintCallable, Category = "Whisper AI")
	void StartRecording(FString DeviceName);

	UFUNCTION(BlueprintCallable, Category = "Whisper AI")
	TArray<FString> GetAvailableMicrophones();

	UFUNCTION(BlueprintCallable, Category = "Whisper AI")
	void StopRecordingAndRecognize();

	// 두 문장이 얼마나 비슷한지 0.0 ~ 1.0 사이의 값으로 반환합니다. (1.0이면 완벽 일치)
	UFUNCTION(BlueprintCallable, Category = "Whisper AI | Utility")
	float CalculateStringSimilarity(const FString& Source, const FString& Target);

	// 이 컴포넌트가 파괴될 때 메모리를 정리할 함수입니다.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	struct whisper_context* WhisperCtx = nullptr;
	Audio::FAudioCapture AudioCaptureNative;
	Audio::FOnAudioCaptureFunction CaptureDelegate;
	void OnAudioCaptureNative(const float* InAudio, int32 NumSamples, int32 InNumChannels, int32 InSampleRate, double StreamTime, bool bFault);
	TArray<float> CapturedAudioData;
	FCriticalSection WhisperMutex;
	FCriticalSection AudioMutex;
	TFuture<void> RecognitionTask;//보조 스레드의 작업 상태를 담아둘 박스
	int32 HardwareSampleRate = 16000;
	bool bIsRecording = false;
	std::string language =  "ko";
	TArray<FString> prohibitionWord{ TEXT(" "),TEXT("."),TEXT(","),TEXT("?"),
		TEXT("!"),TEXT("~"),TEXT("("),TEXT(")"),TEXT("\'"),TEXT("\"") };
};
