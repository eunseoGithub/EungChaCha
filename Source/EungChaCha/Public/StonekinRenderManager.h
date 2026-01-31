// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StonekinRenderManager.generated.h"

class UHierarchicalInstancedStaticMeshComponent;

UCLASS()
class EUNGCHACHA_API AStonekinRenderManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AStonekinRenderManager();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadWrite)
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> HISMComponent;
	
	UPROPERTY()
	class UStonekinSimSubSystem* SimSubSystem;
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
