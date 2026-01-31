// Fill out your copyright notice in the Description page of Project Settings.


#include "System/MainGameModeBase.h"

void AMainGameModeBase::StartPhase()
{
}

void AMainGameModeBase::EndPhase()
{
}

void AMainGameModeBase::SpawnInitialSwarm()
{
}

void AMainGameModeBase::ResolveEncounter()
{
}

void AMainGameModeBase::ComputeStabilityDamage()
{
}

void AMainGameModeBase::ApplyTerrainModifiers()
{
}

void AMainGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
}

void AMainGameModeBase::StartPlay()
{
	Super::StartPlay();
}

void AMainGameModeBase::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}
