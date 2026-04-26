#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StonekinControlWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS()
class EUNGCHACHA_API UStonekinControlWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual bool Initialize() override;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StoneCountText;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> AddStoneButton;
	
	UFUNCTION()
	void OnAddClicked();
};
