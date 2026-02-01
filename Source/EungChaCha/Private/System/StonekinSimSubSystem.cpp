// Fill out your copyright notice in the Description page of Project Settings.


#include "System/StonekinSimSubSystem.h"

#include "StonekinSimManager.h"
#include "Kismet/GameplayStatics.h"

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
	if (Positions.Num()==0) return;
	const int32 Size = Positions.Num();
	if (Manager==nullptr) Manager = Cast<AStonekinSimManager>(UGameplayStatics::GetActorOfClass(GetWorld(),AStonekinSimManager::StaticClass()));
	if (Manager==nullptr) return;
	
	const float DesiredSeparation = Manager->DesiredSeparation;
	const float SepWeight = Manager->SepWeight;
	const float AliWeight = Manager->AliWeight;
	const float CohWeight = Manager->CohWeight;
	const float TargetWeight = Manager->TargetWeight;
	const float NeighborRange = Manager->NeighborRange;
	
	FVector FlattenedClickPos = CurrentClickPosition;
	FlattenedClickPos.Z = 100.0f;
	
	for (int32 i = 0 ; i < Size;++i)
	{
		FVector Separation = FVector::ZeroVector;
		FVector Alignment = FVector::ZeroVector;
		FVector Cohesion = FVector::ZeroVector;
		int32 NeighborCount = 0;
		
		FVector MyPos = Positions[i];
		MyPos.Z = 100.f;
		
		for (int32 j = 0 ; j <Size;j++)
		{
			if (i == j) continue;
			
			FVector OtherPos = Positions[j];
			OtherPos.Z = 100.f;
			
			float Distance = FVector::Dist(MyPos, OtherPos);
			if (Distance < DesiredSeparation && Distance > 0.01f)
			{
				FVector Diff = MyPos-OtherPos;
				Diff.Normalize();
				Separation += Diff/Distance;
			}
			
			if (Distance < NeighborRange && Distance > 0.01f)
			{
				Alignment += (FlattenedClickPos - OtherPos).GetSafeNormal2D();
				Cohesion += OtherPos;
				NeighborCount++;
			}
		}
		FVector TargetDir = (FlattenedClickPos - MyPos).GetSafeNormal2D();
		FVector FinalForce = TargetDir * TargetWeight;
		
		if (NeighborCount > 0)
		{
			Alignment /= NeighborCount;
			Cohesion = (Cohesion/NeighborCount) - MyPos;
			
			FinalForce += (Separation.GetSafeNormal2D()* SepWeight);
			FinalForce += (Alignment.GetSafeNormal2D()*AliWeight);
			FinalForce += (Cohesion.GetSafeNormal2D()*CohWeight);
		}
		
		FVector FinalDir = FinalForce.GetSafeNormal2D();
		
		float Speed = Velocities[i];
		FVector MoveDelta = FinalDir * Speed * DeltaTime;
		
		FVector NewPos = MyPos + MoveDelta;
		NewPos.Z = 100.0f;
		Positions[i] = NewPos;
		if (!FinalDir.IsNearlyZero())
		{
			FVector RotationAxis = FVector::CrossProduct(FVector::UpVector,FinalDir);
			float AngleDelta = MoveDelta.Size()/50.f;
			FQuat DeltaRot = FQuat(RotationAxis,AngleDelta);
			Rotations[i] = (DeltaRot * Rotations[i].GetNormalized());
		}
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

