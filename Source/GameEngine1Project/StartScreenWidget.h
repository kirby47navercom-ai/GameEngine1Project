#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StartScreenWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UTextBlock;
class UVerticalBox;

UCLASS()
class GAMEENGINE1PROJECT_API UStartScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UPROPERTY()
	TObjectPtr<UBorder> SettingsPanel;

	void BuildStartScreenWidgetTree();
	UTextBlock* AddText(UCanvasPanel* Root, const FText& Text, FVector2D Position, FVector2D Size,
		float FontSize, FLinearColor Color, bool bCenterAligned = true);
	UButton* AddMenuButton(UVerticalBox* ButtonList, const FText& Label);

	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleQuitClicked();
};
