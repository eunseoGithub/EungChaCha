// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MainGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class EUNGCHACHA_API UMainGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	void UserSetting();
	void StartNewGame();
	void ContinueGame();
protected:
	void Init() override;
	void OnStart() override;
	void Shutdown() override;
	
};
