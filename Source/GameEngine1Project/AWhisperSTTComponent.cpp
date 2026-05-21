// Fill out your copyright notice in the Description page of Project Settings.


#include "AWhisperSTTComponent.h"
#include <string>
#include "VoiceModule.h"
#include "Interfaces/VoiceCapture.h"
#include "Async/Async.h"


// Sets default values
AAWhisperSTTComponent::AAWhisperSTTComponent()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	WhisperCtx = nullptr;

}

// 2. 모델 파일 불러오기
bool AAWhisperSTTComponent::InitWhisperModel(const FString& ModelPath)
{
	// 이미 불러온 모델이 있다면 메모리에서 먼저 지웁니다.
	FScopeLock Lock(&WhisperMutex);

	if (WhisperCtx != nullptr)
	{
		whisper_free(WhisperCtx);
		WhisperCtx = nullptr;
	}

	// 언리얼의 FString 경로를 일반 C++ 문자열(UTF8)로 변환합니다.
	std::string PathStr = TCHAR_TO_UTF8(*ModelPath);

	// whisper.cpp의 핵심 함수! 모델 파일을 읽어서 메모리에 올립니다.
	WhisperCtx = whisper_init_from_file(PathStr.c_str());

	if (WhisperCtx == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Whisper 모델을 불러오는데 실패했습니다: %s"), *ModelPath);
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("Whisper 모델 로드 성공!"));
	return true;
}

FString AAWhisperSTTComponent::RecognizeSpeech(const TArray<float>& AudioSamples)
{

	FScopeLock Lock(&WhisperMutex);
	// 모델이 안 불러와졌거나, 오디오 데이터가 없으면 빈 글자를 반환합니다.
	if (WhisperCtx == nullptr || AudioSamples.Num() == 0)
	{
		return FString("");
	}

	// Whisper AI를 돌리기 위한 기본 설정값을 가져옵니다.
	whisper_full_params Params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
	Params.print_progress = false; // 콘솔창에 진행률 도배되는 것 방지
	Params.language = language.data();        // 한국어 인식 설정 (영어면 "en")
	Params.no_context = true;
	// ⭐️ 여기서 AI 연산이 시작됩니다! (오디오 배열을 넘겨줌)
	if (whisper_full(WhisperCtx, Params, AudioSamples.GetData(), AudioSamples.Num()) != 0)
	{
		//UE_LOG(LogTemp, Error, TEXT("음성 인식 처리에 실패했습니다."));
		return FString("");
	}

	// 인식된 텍스트 조각(세그먼트)들을 하나로 합칩니다.
	FString ResultText = "";
	const int n_segments = whisper_full_n_segments(WhisperCtx);

	for (int i = 0; i < n_segments; ++i)
	{
		// AI가 뱉어낸 텍스트 조각을 가져와서 언리얼 FString으로 변환 후 이어 붙입니다.
		const char* text = whisper_full_get_segment_text(WhisperCtx, i);
		ResultText += FString(UTF8_TO_TCHAR(text));
	}

	return ResultText;
}
void AAWhisperSTTComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	bIsRecording = false;

	if (AudioCaptureNative.IsStreamOpen())
	{
		AudioCaptureNative.StopStream();
		AudioCaptureNative.CloseStream();
	}

	// 실행 중인 음성인식 스레드 종료 대기
	if (RecognitionTask.IsValid() && !RecognitionTask.IsReady())
	{
		RecognitionTask.Wait();
	}

	FScopeLock Lock(&WhisperMutex);
	if (WhisperCtx)
	{
		whisper_free(WhisperCtx);
		WhisperCtx = nullptr;
	}
}
// Called when the game starts or when spawned
void AAWhisperSTTComponent::BeginPlay()
{
	Super::BeginPlay();
	
}
void AAWhisperSTTComponent::OnAudioCaptureNative(const float* InAudio, int32 NumSamples, int32 InNumChannels, int32 InSampleRate, double StreamTime, bool bFault)
{
	if (!bIsRecording) return;

	if (!bFault && InAudio)
	{
		FScopeLock Lock(&AudioMutex);
		HardwareSampleRate = InSampleRate;

		for (int32 i = 0; i < NumSamples; ++i)
		{
			float MixedSample = 0.0f;
			for (int32 ch = 0; ch < InNumChannels; ++ch)
			{
				MixedSample += InAudio[i * InNumChannels + ch];
			}
			CapturedAudioData.Add(MixedSample / InNumChannels);
		}
	}
}
void AAWhisperSTTComponent::StartRecording(FString DeviceName)
{
	TArray<Audio::FCaptureDeviceInfo> DeviceInfos;
	if (!AudioCaptureNative.GetCaptureDevicesAvailable(DeviceInfos)) return;

	int32 TargetIndex = INDEX_NONE;

	// 🚨 [핵심 해결 코드] 블루프린트에서 빈칸이 들어오면 무조건 0번(기본) 마이크를 잡도록 강제합니다!
	if (DeviceName.IsEmpty() && DeviceInfos.Num() > 0)
	{
		TargetIndex = 0;
		DeviceName = DeviceInfos[0].DeviceName;
	}
	else
	{
		for (int32 i = 0; i < DeviceInfos.Num(); ++i)
		{
			if (DeviceInfos[i].DeviceName == DeviceName)
			{
				TargetIndex = i;
				break;
			}
		}
	}

	// 마이크를 진짜 못 찾았다면 빨간 글씨로 경고를 띄웁니다.
	if (TargetIndex == INDEX_NONE)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("에러: 일치하는 마이크를 찾을 수 없습니다!"));
		return;
	}

	Audio::FAudioCaptureDeviceParams Params;
	Params.DeviceIndex = TargetIndex;

	{
		FScopeLock Lock(&AudioMutex);
		CapturedAudioData.Empty();
	}

	if (!AudioCaptureNative.IsStreamOpen())
	{
		Audio::FOnAudioCaptureFunction Callback = [this](const void* InAudio, int32 InNumSamples, int32 InNumChannels, int32 InSampleRate, double InStreamTime, bool bInFault)
			{
				const float* AudioFloatData = static_cast<const float*>(InAudio);
				this->OnAudioCaptureNative(AudioFloatData, InNumSamples, InNumChannels, InSampleRate, InStreamTime, bInFault);
			};

		if (AudioCaptureNative.OpenAudioCaptureStream(Params, MoveTemp(Callback), 1024))
		{
			AudioCaptureNative.StartStream();
		}
	}

	// 🚨 스위치를 켭니다! 
	bIsRecording = true;
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, FString::Printf(TEXT("마이크 녹음 시작! (%s)"), *DeviceName));
}
TArray<FString> AAWhisperSTTComponent::GetAvailableMicrophones()
{
	TArray<FString> DeviceNames;

	Audio::FAudioCapture AudioCapture;
	TArray<Audio::FCaptureDeviceInfo> DeviceInfos;

	// 엔진에게 "지금 연결된 마이크 장치 정보 다 가져와!" 라고 명령합니다.
	if (AudioCapture.GetCaptureDevicesAvailable(DeviceInfos))
	{
		for (const Audio::FCaptureDeviceInfo& Info : DeviceInfos)
		{
			// 가져온 장치 정보들 중에서 '이름'만 쏙쏙 뽑아서 배열에 담습니다.
			DeviceNames.Add(Info.DeviceName);
		}
	}

	// 혹시 마이크가 하나도 없으면 알려줍니다.
	if (DeviceNames.IsEmpty())
	{
		DeviceNames.Add(TEXT("연결된 마이크가 없습니다."));
	}

	return DeviceNames;
}
void AAWhisperSTTComponent::StopRecordingAndRecognize()
{
	bIsRecording = false;
	FScopeLock Lock(&AudioMutex);

	if (CapturedAudioData.Num() == 0) return; // return ""; 에서 수정

	// ... (MaxVolume 계산, 16000Hz 리샘플링 코드는 그대로 유지) ...

	TWeakObjectPtr<AAWhisperSTTComponent> WeakThis(this);

	// 🚨 [삭제] static 변수는 과거의 기억을 계속 붙잡고 있으므로 지워야 합니다.
	// static FString result; 
		// 16000Hz 리샘플링
	TArray<float> ResampledData;
	if (HardwareSampleRate == 16000)
	{
		ResampledData = CapturedAudioData;
	}
	else
	{
		float Ratio = (float)HardwareSampleRate / 16000.0f;
		int32 NewSize = FMath::FloorToInt(CapturedAudioData.Num() / Ratio);
		ResampledData.Reserve(NewSize);

		for (int32 i = 0; i < NewSize; ++i)
		{
			int32 Index = FMath::Clamp(FMath::RoundToInt(i * Ratio), 0, CapturedAudioData.Num() - 1);
			ResampledData.Add(CapturedAudioData[Index]);
		}
	}
	if (CapturedAudioData.Num() < HardwareSampleRate * 0.3f)
	{
		return;
	}
	RecognitionTask = Async(EAsyncExecution::Thread, [WeakThis, ResampledData]()
		{
			if (!WeakThis.IsValid()) return;

			FString FinalResult = WeakThis->RecognizeSpeech(ResampledData);

			AsyncTask(ENamedThreads::GameThread, [WeakThis, FinalResult]()
				{
					if (WeakThis.IsValid())
					{
						FString s = FinalResult;
						for (const FString& str : WeakThis->prohibitionWord)
						{
							s = s.Replace(*str, TEXT(""));
						}

						// 🚨 [핵심] result 변수에 담지 말고, 예전처럼 이벤트를 쏴서 블루프린트에 바로 넘겨줍니다!
						WeakThis->OnSpeechRecognizedEvent.Broadcast(s);
					}
				});
		});

	CapturedAudioData.Empty();

	// 🚨 [삭제] return result; 는 지워줍니다.
}
float AAWhisperSTTComponent::CalculateStringSimilarity(const FString& Source, const FString& Target)
{
	// 1. 특수문자 및 공백 제거 (정규화)
	FString CleanSource = Source.Replace(TEXT(" "), TEXT("")).Replace(TEXT("."), TEXT("")).Replace(TEXT("!"), TEXT("")).Replace(TEXT("?"), TEXT(""));
	FString CleanTarget = Target.Replace(TEXT(" "), TEXT("")).Replace(TEXT("."), TEXT("")).Replace(TEXT("!"), TEXT("")).Replace(TEXT("?"), TEXT(""));

	int32 Len1 = CleanSource.Len();
	int32 Len2 = CleanTarget.Len();

	if (Len1 == 0 && Len2 == 0) return 1.0f;
	if (Len1 == 0 || Len2 == 0) return 0.0f;

	// 레벤슈타인 거리 (Levenshtein Distance) 알고리즘 테이블
	TArray<TArray<int32>> Matrix;
	Matrix.SetNumZeroed(Len1 + 1);
	for (int32 i = 0; i <= Len1; ++i)
	{
		Matrix[i].SetNumZeroed(Len2 + 1);
		Matrix[i][0] = i;
	}
	for (int32 j = 0; j <= Len2; ++j)
	{
		Matrix[0][j] = j;
	}

	for (int32 i = 1; i <= Len1; ++i)
	{
		for (int32 j = 1; j <= Len2; ++j)
		{
			int32 Cost = (CleanSource[i - 1] == CleanTarget[j - 1]) ? 0 : 1;

			int32 Deletion = Matrix[i - 1][j] + 1;
			int32 Insertion = Matrix[i][j - 1] + 1;
			int32 Substitution = Matrix[i - 1][j - 1] + Cost;

			Matrix[i][j] = FMath::Min3(Deletion, Insertion, Substitution);
		}
	}

	// 0.0 ~ 1.0 사이의 퍼센티지로 변환
	int32 MaxLen = FMath::Max(Len1, Len2);
	int32 Distance = Matrix[Len1][Len2];

	return 1.0f - ((float)Distance / (float)MaxLen);
}
// Called every frame
void AAWhisperSTTComponent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

