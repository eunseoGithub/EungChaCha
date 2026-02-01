// Fill out your copyright notice in the Description page of Project Settings.


#include "StonekinRenderManager.h"

#include "System/StonekinSimSubSystem.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

// Sets default values
AStonekinRenderManager::AStonekinRenderManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	HISMComponent = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISM"));
	SetRootComponent(HISMComponent);
	
}

// Called when the game starts or when spawned
void AStonekinRenderManager::BeginPlay()
{
	Super::BeginPlay();
	
	SimSubSystem = GetWorld()->GetSubsystem<UStonekinSimSubSystem>();
	
	const TArray<FVector>& Positions = SimSubSystem->GetPositions();
	const TArray<FQuat>& Rotations = SimSubSystem->GetRotations();
	int32 NumInstances = Positions.Num();
	if (Positions.Num()!=Rotations.Num()) return;
	int32 CurrentISMCount = HISMComponent->GetInstanceCount();
	if (NumInstances > CurrentISMCount)
	{
		for (int32 i = CurrentISMCount; i < NumInstances;++i)
		{
			HISMComponent->AddInstance(FTransform(Positions[i]));
		}
	}
}

// Called every frame
void AStonekinRenderManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (!SimSubSystem) return;
	
	const TArray<FVector>& Positions = SimSubSystem->GetPositions();
	const TArray<FQuat>& Rotations = SimSubSystem->GetRotations();
	int32 NumInstances = Positions.Num();
	if (Positions.Num()!=Rotations.Num()) return;
	
	for (int32 i = 0 ; i <NumInstances;++i)
	{
		FTransform NewTransform(Rotations[i],Positions[i],FVector(0.1f));
		HISMComponent->UpdateInstanceTransform(i,NewTransform,true,false,false);
	}
	HISMComponent->MarkRenderStateDirty();
}

