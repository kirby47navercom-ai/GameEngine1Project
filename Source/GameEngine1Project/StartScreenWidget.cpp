#include "StartScreenWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SWidget.h"

namespace
{
	void ConfigureCanvasSlot(UWidget* Widget, FVector2D Position, FVector2D Size, FVector2D Alignment = FVector2D(0.5f, 0.5f))
	{
		if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Widget->Slot))
		{
			Slot->SetAnchors(FAnchors(0.5f, 0.5f));
			Slot->SetAlignment(Alignment);
			Slot->SetPosition(Position);
			Slot->SetSize(Size);
		}
	}

	FSlateFontInfo MakeStartScreenFont(float FontSize, bool bTitleFont = false)
	{
		const TCHAR* FontPath = bTitleFont
			? TEXT("/Game/Font/\uACBD\uAE30\uCC9C\uB144\uC81C\uBAA9_Bold_Font.\uACBD\uAE30\uCC9C\uB144\uC81C\uBAA9_Bold_Font")
			: TEXT("/Game/Font/\uACBD\uAE30\uCC9C\uB144\uBC14\uD0D5_Bold_Font.\uACBD\uAE30\uCC9C\uB144\uBC14\uD0D5_Bold_Font");

		if (UFont* Font = LoadObject<UFont>(nullptr, FontPath))
		{
			return FSlateFontInfo(Font, static_cast<int32>(FontSize));
		}

		return FSlateFontInfo(FCoreStyle::GetDefaultFont(), static_cast<int32>(FontSize));
	}
}

TSharedRef<SWidget> UStartScreenWidget::RebuildWidget()
{
	BuildStartScreenWidgetTree();
	return Super::RebuildWidget();
}

void UStartScreenWidget::BuildStartScreenWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("StartRoot"));
	WidgetTree->RootWidget = Root;

	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.015f, 0.018f, 0.028f, 0.88f));
	Root->AddChild(Backdrop);
	if (UCanvasPanelSlot* BackdropSlot = Cast<UCanvasPanelSlot>(Backdrop->Slot))
	{
		BackdropSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BackdropSlot->SetOffsets(FMargin(0.f));
	}

	AddText(Root, FText::FromString(TEXT("\uD0C8\uCD9C\u0020\u3131\u3131\uD613")), FVector2D(0.f, -255.f), FVector2D(900.f, 92.f),
		68.f, FLinearColor(1.f, 0.82f, 0.48f, 1.f));
	AddText(Root, FText::FromString(TEXT("\u0032\u0030\u0032\u0032\u0031\u0038\u0030\u0030\u0032\u0031\u0020\uC591\uD604\uBE48")), FVector2D(0.f, -190.f), FVector2D(720.f, 42.f),
		24.f, FLinearColor(0.56f, 0.82f, 1.f, 0.92f));

	UVerticalBox* ButtonList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainButtonList"));
	Root->AddChild(ButtonList);
	ConfigureCanvasSlot(ButtonList, FVector2D(0.f, 35.f), FVector2D(380.f, 260.f));

	UButton* StartButton = AddMenuButton(ButtonList, FText::FromString(TEXT("\uAC8C\uC784 \uC2DC\uC791")));
	UButton* SettingsButton = AddMenuButton(ButtonList, FText::FromString(TEXT("\uC124\uC815")));
	UButton* QuitButton = AddMenuButton(ButtonList, FText::FromString(TEXT("\uAC8C\uC784 \uC885\uB8CC")));

	StartButton->OnClicked.AddDynamic(this, &UStartScreenWidget::HandleStartClicked);
	SettingsButton->OnClicked.AddDynamic(this, &UStartScreenWidget::HandleSettingsClicked);
	QuitButton->OnClicked.AddDynamic(this, &UStartScreenWidget::HandleQuitClicked);

	SettingsPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsPanel"));
	SettingsPanel->SetBrushColor(FLinearColor(0.04f, 0.032f, 0.025f, 0.88f));
	SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	Root->AddChild(SettingsPanel);
	ConfigureCanvasSlot(SettingsPanel, FVector2D(0.f, 235.f), FVector2D(520.f, 76.f));

	UTextBlock* SettingsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsText"));
	SettingsText->SetText(FText::FromString(TEXT("\uC124\uC815\uC740 \uAC8C\uC784 \uC548 Tab \uBA54\uB274\uC5D0\uC11C \uB9C8\uC774\uD06C\uC640 \uBCFC\uB968\uC744 \uC870\uC808\uD560 \uC218 \uC788\uC5B4")));
	SettingsText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.92f, 0.78f, 1.f)));
	SettingsText->SetJustification(ETextJustify::Center);
	SettingsText->SetAutoWrapText(true);
	SettingsText->SetFont(MakeStartScreenFont(18.f));
	SettingsPanel->SetContent(SettingsText);
}

UTextBlock* UStartScreenWidget::AddText(UCanvasPanel* Root, const FText& Text, FVector2D Position,
	FVector2D Size, float FontSize, FLinearColor Color, bool bCenterAligned)
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	TextBlock->SetText(Text);
	TextBlock->SetColorAndOpacity(FSlateColor(Color));
	TextBlock->SetJustification(bCenterAligned ? ETextJustify::Center : ETextJustify::Left);
	TextBlock->SetFont(MakeStartScreenFont(FontSize, FontSize > 40.f));
	Root->AddChild(TextBlock);
	ConfigureCanvasSlot(TextBlock, Position, Size);
	return TextBlock;
}

UButton* UStartScreenWidget::AddMenuButton(UVerticalBox* ButtonList, const FText& Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());

	FButtonStyle Style = Button->GetStyle();
	Style.Normal.TintColor = FSlateColor(FLinearColor(0.08f, 0.07f, 0.08f, 0.88f));
	Style.Hovered.TintColor = FSlateColor(FLinearColor(0.38f, 0.17f, 0.08f, 0.96f));
	Style.Pressed.TintColor = FSlateColor(FLinearColor(0.78f, 0.34f, 0.1f, 1.f));
	Button->SetStyle(Style);

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Text->SetText(Label);
	Text->SetJustification(ETextJustify::Center);
	Text->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.9f, 0.72f, 1.f)));
	Text->SetFont(MakeStartScreenFont(30.f, true));
	Button->SetContent(Text);

	UVerticalBoxSlot* ButtonSlot = ButtonList->AddChildToVerticalBox(Button);
	ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	ButtonSlot->SetPadding(FMargin(0.f, 8.f));
	ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
	return Button;
}

void UStartScreenWidget::HandleStartClicked()
{
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		PlayerController->bShowMouseCursor = false;
		PlayerController->bEnableClickEvents = false;
		PlayerController->bEnableMouseOverEvents = false;

		FInputModeGameOnly InputMode;
		PlayerController->SetInputMode(InputMode);
	}

	UGameplayStatics::SetGamePaused(this, false);
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/map2/GameMap1")));
}

void UStartScreenWidget::HandleSettingsClicked()
{
	if (!SettingsPanel)
	{
		return;
	}

	const bool bShouldShow = SettingsPanel->GetVisibility() == ESlateVisibility::Collapsed;
	SettingsPanel->SetVisibility(bShouldShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UStartScreenWidget::HandleQuitClicked()
{
	APlayerController* PlayerController = GetOwningPlayer();
	UKismetSystemLibrary::QuitGame(this, PlayerController, EQuitPreference::Quit, false);
}
