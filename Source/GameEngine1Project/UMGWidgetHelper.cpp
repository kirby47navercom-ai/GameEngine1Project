#include "UMGWidgetHelper.h"

#include "AudioCaptureCore.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Font.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

namespace
{
	void GetUserWidgetChildren(UUserWidget* RootWidget, TArray<UWidget*>& OutWidgets)
	{
		OutWidgets.Reset();

		if (!RootWidget || !RootWidget->WidgetTree)
		{
			return;
		}

		RootWidget->WidgetTree->GetAllWidgets(OutWidgets);
	}

	bool WidgetNameContains(const UWidget* Widget, const FString& Token)
	{
		return Widget && Widget->GetName().Contains(Token, ESearchCase::IgnoreCase);
	}

	void FindAllMicCombos(UUserWidget* RootWidget, TArray<UComboBoxString*>& OutCombos)
	{
		OutCombos.Reset();

		if (!RootWidget)
		{
			return;
		}

		if (UComboBoxString* NamedCombo = Cast<UComboBoxString>(RootWidget->GetWidgetFromName(TEXT("ComboBoxString_214"))))
		{
			OutCombos.AddUnique(NamedCombo);
		}

		TArray<UWidget*> Widgets;
		GetUserWidgetChildren(RootWidget, Widgets);
		for (UWidget* Widget : Widgets)
		{
			if (UComboBoxString* Combo = Cast<UComboBoxString>(Widget))
			{
				OutCombos.AddUnique(Combo);
			}
		}
	}

	USlider* FindSystemMenuMasterSlider(UUserWidget* RootWidget)
	{
		TArray<UWidget*> Widgets;
		GetUserWidgetChildren(RootWidget, Widgets);

		for (UWidget* Widget : Widgets)
		{
			if (USlider* Slider = Cast<USlider>(Widget))
			{
				if (WidgetNameContains(Slider, TEXT("Master")) || WidgetNameContains(Slider, TEXT("Volume")))
				{
					return Slider;
				}
			}
		}

		for (UWidget* Widget : Widgets)
		{
			if (USlider* Slider = Cast<USlider>(Widget))
			{
				return Slider;
			}
		}

		return nullptr;
	}

	bool IsVolumePlaceholderText(const FString& Text)
	{
		const FString Compact = Text.Replace(TEXT(" "), TEXT(""));
		return Compact.Equals(TEXT("A"), ESearchCase::IgnoreCase) ||
			Compact.Equals(TEXT("AA"), ESearchCase::IgnoreCase) ||
			Compact.Equals(TEXT("AAA"), ESearchCase::IgnoreCase) ||
			Compact.Equals(TEXT("AAAA"), ESearchCase::IgnoreCase);
	}

	bool IsVolumeTextByName(const UTextBlock* TextBlock)
	{
		return WidgetNameContains(TextBlock, TEXT("VolumeValue")) ||
			WidgetNameContains(TextBlock, TEXT("VolumePercent")) ||
			WidgetNameContains(TextBlock, TEXT("MasterVolumeValue")) ||
			WidgetNameContains(TextBlock, TEXT("Percent"));
	}

	void FindSystemMenuVolumeTexts(UUserWidget* RootWidget, TArray<UTextBlock*>& OutTextBlocks)
	{
		OutTextBlocks.Reset();

		TArray<UWidget*> Widgets;
		GetUserWidgetChildren(RootWidget, Widgets);
		for (UWidget* Widget : Widgets)
		{
			if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
			{
				if (IsVolumeTextByName(TextBlock))
				{
					OutTextBlocks.AddUnique(TextBlock);
				}
			}
		}

		for (UWidget* Widget : Widgets)
		{
			if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
			{
				if (IsVolumePlaceholderText(TextBlock->GetText().ToString()))
				{
					OutTextBlocks.AddUnique(TextBlock);
				}
			}
		}
	}

	TArray<FString> GetAvailableMicrophoneNames()
	{
		TArray<FString> DeviceNames;

		Audio::FAudioCapture AudioCapture;
		TArray<Audio::FCaptureDeviceInfo> DeviceInfos;
		if (AudioCapture.GetCaptureDevicesAvailable(DeviceInfos))
		{
			for (int32 Index = 0; Index < DeviceInfos.Num(); ++Index)
			{
				FString DeviceName = DeviceInfos[Index].DeviceName;
				if (DeviceName.IsEmpty())
				{
					DeviceName = FString::Printf(TEXT("Microphone %d"), Index + 1);
				}
				DeviceNames.Add(DeviceName);
			}
		}

		if (DeviceNames.IsEmpty())
		{
			DeviceNames.Add(TEXT("No microphone device"));
		}

		return DeviceNames;
	}

	APawn* GetPlayerPawnFromContext(UObject* WorldContextObject)
	{
		if (!WorldContextObject)
		{
			return nullptr;
		}

		return UGameplayStatics::GetPlayerPawn(WorldContextObject, 0);
	}

	UGameInstance* GetGameInstanceFromContext(UObject* WorldContextObject)
	{
		if (!WorldContextObject)
		{
			return nullptr;
		}

		return UGameplayStatics::GetGameInstance(WorldContextObject);
	}

	FString GetStringPropertyValue(UObject* TargetObject, const FName PropertyName)
	{
		if (!TargetObject)
		{
			return FString();
		}

		if (FStrProperty* StringProperty = FindFProperty<FStrProperty>(TargetObject->GetClass(), PropertyName))
		{
			return StringProperty->GetPropertyValue_InContainer(TargetObject);
		}

		return FString();
	}

	void SetStringPropertyValue(UObject* TargetObject, const FName PropertyName, const FString& Value)
	{
		if (!TargetObject)
		{
			return;
		}

		if (FStrProperty* StringProperty = FindFProperty<FStrProperty>(TargetObject->GetClass(), PropertyName))
		{
			StringProperty->SetPropertyValue_InContainer(TargetObject, Value);
		}
	}

	void SetBoolPropertyValue(UObject* TargetObject, const FName PropertyName, const bool bValue)
	{
		if (!TargetObject)
		{
			return;
		}

		if (FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(TargetObject->GetClass(), PropertyName))
		{
			BoolProperty->SetPropertyValue_InContainer(TargetObject, bValue);
		}
	}

	bool GetBoolPropertyValue(UObject* TargetObject, const FName PropertyName, const bool DefaultValue)
	{
		if (!TargetObject)
		{
			return DefaultValue;
		}

		if (FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(TargetObject->GetClass(), PropertyName))
		{
			return BoolProperty->GetPropertyValue_InContainer(TargetObject);
		}

		return DefaultValue;
	}

	float GetFloatPropertyValue(UObject* TargetObject, const FName PropertyName, const float DefaultValue)
	{
		if (!TargetObject)
		{
			return DefaultValue;
		}

		if (FFloatProperty* FloatProperty = FindFProperty<FFloatProperty>(TargetObject->GetClass(), PropertyName))
		{
			return FloatProperty->GetPropertyValue_InContainer(TargetObject);
		}

		if (FDoubleProperty* DoubleProperty = FindFProperty<FDoubleProperty>(TargetObject->GetClass(), PropertyName))
		{
			return static_cast<float>(DoubleProperty->GetPropertyValue_InContainer(TargetObject));
		}

		return DefaultValue;
	}

	void SetFloatPropertyValue(UObject* TargetObject, const FName PropertyName, const float Value)
	{
		if (!TargetObject)
		{
			return;
		}

		if (FFloatProperty* FloatProperty = FindFProperty<FFloatProperty>(TargetObject->GetClass(), PropertyName))
		{
			FloatProperty->SetPropertyValue_InContainer(TargetObject, Value);
			return;
		}

		if (FDoubleProperty* DoubleProperty = FindFProperty<FDoubleProperty>(TargetObject->GetClass(), PropertyName))
		{
			DoubleProperty->SetPropertyValue_InContainer(TargetObject, static_cast<double>(Value));
		}
	}

	FString GetStoredSelectedMicFromPlayer(UObject* WorldContextObject)
	{
		APawn* PlayerPawn = GetPlayerPawnFromContext(WorldContextObject);
		return GetStringPropertyValue(PlayerPawn, TEXT("SelectedMicDeviceName"));
	}

	FString GetStoredSelectedMicFromGameInstance(UObject* WorldContextObject)
	{
		UGameInstance* GameInstance = GetGameInstanceFromContext(WorldContextObject);
		return GetStringPropertyValue(GameInstance, TEXT("SelectedMicDeviceName"));
	}

	FString GetStoredSelectedMic(UObject* WorldContextObject)
	{
		const FString PlayerSelection = GetStoredSelectedMicFromPlayer(WorldContextObject);
		if (!PlayerSelection.IsEmpty())
		{
			return PlayerSelection;
		}

		return GetStoredSelectedMicFromGameInstance(WorldContextObject);
	}

	void StoreSelectedMicOnPlayer(UObject* WorldContextObject, const FString& DeviceName)
	{
		if (!WorldContextObject || DeviceName.IsEmpty() || DeviceName == TEXT("No microphone device"))
		{
			return;
		}

		APawn* PlayerPawn = GetPlayerPawnFromContext(WorldContextObject);
		if (!PlayerPawn)
		{
			return;
		}

		SetStringPropertyValue(PlayerPawn, TEXT("SelectedMicDeviceName"), DeviceName);
	}

	void StoreSelectedMicOnGameInstance(UObject* WorldContextObject, const FString& DeviceName)
	{
		if (!WorldContextObject || DeviceName.IsEmpty() || DeviceName == TEXT("No microphone device"))
		{
			return;
		}

		UGameInstance* GameInstance = GetGameInstanceFromContext(WorldContextObject);
		SetStringPropertyValue(GameInstance, TEXT("SelectedMicDeviceName"), DeviceName);
		SetBoolPropertyValue(GameInstance, TEXT("Microphone?"), true);
	}

	void StoreSelectedMic(UObject* WorldContextObject, const FString& DeviceName)
	{
		StoreSelectedMicOnPlayer(WorldContextObject, DeviceName);
		StoreSelectedMicOnGameInstance(WorldContextObject, DeviceName);
	}

	float GetStoredMasterVolume(UObject* WorldContextObject)
	{
		UGameInstance* GameInstance = GetGameInstanceFromContext(WorldContextObject);
		if (!GameInstance)
		{
			return 1.0f;
		}

		const bool bHasSavedMasterVolume =
			GetBoolPropertyValue(GameInstance, TEXT("bHasSavedMasterVolume"), false);
		if (!bHasSavedMasterVolume)
		{
			return 1.0f;
		}

		return FMath::Clamp(GetFloatPropertyValue(GameInstance, TEXT("MasterVolume"), 1.0f), 0.0f, 1.0f);
	}

	void StoreMasterVolume(UObject* WorldContextObject, const float Volume)
	{
		UGameInstance* GameInstance = GetGameInstanceFromContext(WorldContextObject);
		if (!GameInstance)
		{
			return;
		}

		SetFloatPropertyValue(GameInstance, TEXT("MasterVolume"), FMath::Clamp(Volume, 0.0f, 1.0f));
		SetBoolPropertyValue(GameInstance, TEXT("bHasSavedMasterVolume"), true);
	}

	void ApplySystemMasterVolume(UObject* WorldContextObject, const float Volume)
	{
		if (!WorldContextObject)
		{
			return;
		}

		USoundMix* MasterMix = LoadObject<USoundMix>(nullptr, TEXT("/Game/Player/UI/SM_SystemMenu_Master.SM_SystemMenu_Master"));
		USoundClass* MasterClass = LoadObject<USoundClass>(nullptr, TEXT("/Engine/EngineSounds/Master.Master"));
		if (!MasterMix || !MasterClass)
		{
			return;
		}

		const float ClampedVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
		UGameplayStatics::SetSoundMixClassOverride(
			WorldContextObject,
			MasterMix,
			MasterClass,
			ClampedVolume,
			1.0f,
			0.05f,
			true);
		UGameplayStatics::PushSoundMixModifier(WorldContextObject, MasterMix);
	}

	FString ChooseInitialMicSelection(const TArray<FString>& DeviceNames, const FString& StoredSelection)
	{
		if (!StoredSelection.IsEmpty() && DeviceNames.Contains(StoredSelection))
		{
			return StoredSelection;
		}

		return DeviceNames.Num() > 0 ? DeviceNames[0] : FString();
	}

	void SetReadableNumberFont(UTextBlock* TextBlock)
	{
		if (!TextBlock)
		{
			return;
		}

		UObject* RobotoFont = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
		if (!RobotoFont)
		{
			return;
		}

		FSlateFontInfo FontInfo = TextBlock->GetFont();
		FontInfo.FontObject = RobotoFont;
		FontInfo.Size = FMath::Max(FontInfo.Size, 22);
		TextBlock->SetFont(FontInfo);
	}
}

bool UUMGWidgetHelper::SetNamedWidgetRenderTranslation(UUserWidget* RootWidget, FName WidgetName, FVector2D Translation)
{
	if (!RootWidget || WidgetName.IsNone())
	{
		return false;
	}

	UWidget* TargetWidget = RootWidget->GetWidgetFromName(WidgetName);
	if (!TargetWidget)
	{
		return false;
	}

	TargetWidget->SetRenderTranslation(Translation);
	return true;
}

bool UUMGWidgetHelper::InitializeSystemMenu(UUserWidget* RootWidget, UObject* WorldContextObject)
{
	if (!RootWidget)
	{
		return false;
	}

	bool bChangedAnyWidget = false;
	const TArray<FString> DeviceNames = GetAvailableMicrophoneNames();
	const FString StoredSelection = GetStoredSelectedMic(WorldContextObject);
	const FString InitialSelection = ChooseInitialMicSelection(DeviceNames, StoredSelection);

	TArray<UComboBoxString*> MicCombos;
	FindAllMicCombos(RootWidget, MicCombos);
	for (UComboBoxString* MicCombo : MicCombos)
	{
		if (!MicCombo)
		{
			continue;
		}

		MicCombo->SetIsEnabled(true);
		MicCombo->SetVisibility(ESlateVisibility::Visible);
		MicCombo->ClearOptions();
		for (const FString& DeviceName : DeviceNames)
		{
			MicCombo->AddOption(DeviceName);
		}
		MicCombo->RefreshOptions();

		if (!InitialSelection.IsEmpty())
		{
			MicCombo->SetSelectedOption(InitialSelection);
			StoreSelectedMic(WorldContextObject, InitialSelection);
			bChangedAnyWidget = true;
		}
	}

	if (USlider* MasterSlider = FindSystemMenuMasterSlider(RootWidget))
	{
		const float StoredVolume = GetStoredMasterVolume(WorldContextObject);
		MasterSlider->SetValue(StoredVolume);
		ApplySystemMasterVolume(WorldContextObject, StoredVolume);
		bChangedAnyWidget = true;
	}

	bChangedAnyWidget |= UpdateSystemMenuVolumeText(RootWidget);
	return bChangedAnyWidget;
}

bool UUMGWidgetHelper::UpdateSystemMenuVolumeText(UUserWidget* RootWidget)
{
	if (!RootWidget)
	{
		return false;
	}

	TArray<UTextBlock*> VolumeTexts;
	FindSystemMenuVolumeTexts(RootWidget, VolumeTexts);

	const USlider* MasterSlider = FindSystemMenuMasterSlider(RootWidget);
	const float SliderValue = MasterSlider ? MasterSlider->GetValue() : 1.0f;
	StoreMasterVolume(RootWidget, SliderValue);
	ApplySystemMasterVolume(RootWidget, SliderValue);

	if (VolumeTexts.IsEmpty())
	{
		return MasterSlider != nullptr;
	}

	const int32 Percent = FMath::Clamp(FMath::RoundToInt(SliderValue * 100.0f), 0, 100);
	const FText PercentText = FText::FromString(FString::Printf(TEXT("%d%%"), Percent));

	bool bPrimarySet = false;
	for (UTextBlock* VolumeText : VolumeTexts)
	{
		if (!VolumeText)
		{
			continue;
		}

		if (!bPrimarySet || IsVolumeTextByName(VolumeText))
		{
			VolumeText->SetText(PercentText);
			SetReadableNumberFont(VolumeText);
			bPrimarySet = true;
		}
		else if (IsVolumePlaceholderText(VolumeText->GetText().ToString()))
		{
			VolumeText->SetText(FText::GetEmpty());
		}
	}

	return true;
}
