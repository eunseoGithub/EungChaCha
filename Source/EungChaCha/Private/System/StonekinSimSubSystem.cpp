// Fill out your copyright notice in the Description page of Project Settings.


#include "System/StonekinSimSubSystem.h"

#include "Layers/LayersSubsystem.h"

TStatId UStonekinSimSubSystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UStonekinSimSubSystem, STATGROUP_Tickables);
}

void UStonekinSimSubSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateSimulation(DeltaTime);
}

void UStonekinSimSubSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	for (int32 i = 0 ; i < 1000; i++)
	{
		FVector RandomLoc = FVector(FMath::RandRange(-500,500),FMath::RandRange(-500,500),100.f);
		AddEntity(RandomLoc);
	}
}

int32 UStonekinSimSubSystem::AddEntity(FVector SpawnLocation)
{
	int32 NewId = NextId++;
	int32 NewIndex = Positions.Add(SpawnLocation);
	Stability.Add(100.f);
	EntityIds.Add(NewId);
	Velocities.Add(100.f);
	Rotations.Add(FQuat::Identity);
	IdToIndexMap.Add(NewId,NewIndex);
	return NewId;
}

void UStonekinSimSubSystem::UpdateSimulation(float DeltaTime)
{
	if (Positions.Num() == 0) return;
	int size = Positions.Num();
	const float StoneRadius = 1.f;
	for (int32 i = 0 ; i < size;++i)
	{
		FVector Direction = CurrentClickPosition - Positions[i];
		Direction.Normalize();
		
		float Speed = Velocities[i];
		FVector MoveDelta = Direction * Speed * DeltaTime;
		
		Positions[i] += MoveDelta;
		
		FVector RotationAxis = FVector::CrossProduct(FVector::UpVector,Direction);
		
		float DistanceMoved = MoveDelta.Size();// Distance
		float AngleDelta = DistanceMoved / StoneRadius;//Rotation Angle
		
		FQuat DeltaRotation = FQuat(RotationAxis,AngleDelta); // Rotate AngleDelta by RotationAxis
		FQuat FinalRotation = DeltaRotation * Rotations[i];// newRotation * thisRotation is important
		
		FinalRotation.Normalize();
		Rotations[i] = FinalRotation;
	}
}

TArray<FVector> UStonekinSimSubSystem::GetPositions() const
{
	return Positions;
}

TArray<FQuat> UStonekinSimSubSystem::GetRotations() const
{
	return Rotations;
}

void UStonekinSimSubSystem::SetClickPosition(const FVector& ClickPosition)
{
	this->CurrentClickPosition = ClickPosition;
}

FVector UStonekinSimSubSystem::GetClickPosition() const
{
	return CurrentClickPosition;
}

