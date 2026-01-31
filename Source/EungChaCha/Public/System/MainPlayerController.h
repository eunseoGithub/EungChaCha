// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainPlayerController.generated.h"

class UStonekinSimSubSystem;
class UInputMappingContext;
class UInputAction;
class UEnhancedInputComponent;

UCLASS()
class EUNGCHACHA_API AMainPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	void Move();
	void Attack();
	void InteractCommand();
	void MakeDrag();
	void SetSelect();
	void ClearSelect();
protected:
	void BeginPlay() override;
	void SetupInputComponent() override;
	void PlayerTick(float DeltaTime) override;
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input)
	TObjectPtr<UInputAction> ClickAction;
	
private:
	UPROPERTY()
	TObjectPtr<UStonekinSimSubSystem> StoneSub;
};
