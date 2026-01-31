// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class EUNGCHACHA_API AMainGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
public:
	void StartPhase();
	void EndPhase();
	void SpawnInitialSwarm();
	void ResolveEncounter();
	void ComputeStabilityDamage();
	void ApplyTerrainModifiers();
protected:
	void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	void StartPlay() override;
	void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	
};
