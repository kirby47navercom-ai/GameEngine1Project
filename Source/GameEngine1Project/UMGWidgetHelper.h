#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UMGWidgetHelper.generated.h"

class UUserWidget;

UCLASS()
class GAMEENGINE1PROJECT_API UUMGWidgetHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UI", meta = (DefaultToSelf = "RootWidget"))
	static bool SetNamedWidgetRenderTranslation(UUserWidget* RootWidget, FName WidgetName, FVector2D Translation);

	UFUNCTION(BlueprintCallable, Category = "UI|System Menu", meta = (DefaultToSelf = "RootWidget", WorldContext = "WorldContextObject"))
	static bool InitializeSystemMenu(UUserWidget* RootWidget, UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "UI|System Menu", meta = (DefaultToSelf = "RootWidget"))
	static bool UpdateSystemMenuVolumeText(UUserWidget* RootWidget);
};
