// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "StonekinSimSubSystem.generated.h"



UCLASS()
class EUNGCHACHA_API UStonekinSimSubSystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
protected:
	virtual TStatId GetStatId() const override;
	void Tick(float DeltaTime) override;
	void Initialize(FSubsystemCollectionBase& Collection) override;
public:
	int32 AddEntity(FVector SpawnLocation);
	void UpdateSimulation(float DeltaTime);
	
	TArray<FVector> GetPositions() const;
	TArray<FQuat> GetRotations() const;

private:
	TArray<float> Velocities;
	TArray<FVector> Positions;
	TArray<FQuat> Rotations;
	TArray<float> Stability;
	TArray<int32> EntityIds;
	
	TMap<int32, int32> IdToIndexMap;
	FVector CurrentClickPosition = {0.f,0.f,0.f};
	int32 NextId = 0;
	
public:
	void SetClickPosition(const FVector& ClickPosition);
	FVector GetClickPosition() const;

	
	
};
