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
	static UMainGameInstance* Get(const UObject* WorldContext);
	void UserSetting();
	void StartNewGame();
	void ContinueGame();
	void GetLandscapeHeightMap();
	TArray<uint16> HeightData;
	FTransform LandscapeTransform;
	FIntRect LandscapeExtent;
	FVector LandscapeScale3D;
	float LandscapeZScale =1.f;
protected:
	void Init() override;
	void OnStart() override;
	void Shutdown() override;
	
};
