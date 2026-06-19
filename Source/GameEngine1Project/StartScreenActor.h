#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StartScreenActor.generated.h"

class UStartScreenWidget;

UCLASS()
class GAMEENGINE1PROJECT_API AStartScreenActor : public AActor
{
	GENERATED_BODY()

public:
	AStartScreenActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<UStartScreenWidget> StartWidget;

	void TryCreateStartWidget();
};
