#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MainHUD.generated.h"
class UStonekinControlWidget;

UCLASS()
class EUNGCHACHA_API AMainHUD : public AHUD
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "UI")
	UStonekinControlWidget* GetControlWidget() const { return ControlWidget; }

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UStonekinControlWidget> ControlWidgetClass;
	
private:
	UPROPERTY()
	TObjectPtr<UStonekinControlWidget> ControlWidget;
};
