#include "StartScreenActor.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "StartScreenWidget.h"

AStartScreenActor::AStartScreenActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AStartScreenActor::BeginPlay()
{
	Super::BeginPlay();
	TryCreateStartWidget();
}

void AStartScreenActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TryCreateStartWidget();
}

void AStartScreenActor::TryCreateStartWidget()
{
	if (StartWidget)
	{
		SetActorTickEnabled(false);
		return;
	}

	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		return;
	}

	PlayerController->bShowMouseCursor = true;
	PlayerController->bEnableClickEvents = true;
	PlayerController->bEnableMouseOverEvents = true;

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);

	StartWidget = CreateWidget<UStartScreenWidget>(PlayerController, UStartScreenWidget::StaticClass());
	if (StartWidget)
	{
		StartWidget->AddToViewport(100);
		SetActorTickEnabled(false);
	}
}
