#include "StonekinRenderManager.h"

#include "System/StonekinSimSubSystem.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "System/MainGameInstance.h"

AStonekinRenderManager::AStonekinRenderManager()
{
	PrimaryActorTick.bCanEverTick = true;
	
	HismComponent = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISM"));
	SetRootComponent(HismComponent);
	
}

void AStonekinRenderManager::BeginPlay()
{
	Super::BeginPlay();
	
	SimSubSystem = GetWorld()->GetSubsystem<UStonekinSimSubSystem>();
	
	const TArray<FVector>& Positions = SimSubSystem->GetPositions();
	const TArray<FQuat>& Rotations = SimSubSystem->GetRotations();
	int32 NumInstances = Positions.Num();
	if (Positions.Num()!=Rotations.Num()) return;
	int32 CurrentISMCount = HismComponent->GetInstanceCount();
	if (NumInstances > CurrentISMCount)
	{
		for (int32 i = CurrentISMCount; i < NumInstances;++i)
		{
			HismComponent->AddInstance(FTransform(Positions[i]),true);
		}
	}
	
}

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
		HismComponent->UpdateInstanceTransform(i,NewTransform,true,false,false);
	}
	HismComponent->MarkRenderStateDirty();
}

