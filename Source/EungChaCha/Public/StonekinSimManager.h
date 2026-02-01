// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StonekinSimManager.generated.h"

UCLASS()
class EUNGCHACHA_API AStonekinSimManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AStonekinSimManager();

	UPROPERTY(EditAnywhere, Category="Stonekin|Weights")
	float SepWeight = 3.0f;
	UPROPERTY(EditAnywhere, Category="Stonekin|Weights")
	float AliWeight = 1.0f;
	UPROPERTY(EditAnywhere, Category="Stonekin|Weights")
	float CohWeight = 0.5f;
	UPROPERTY(EditAnywhere, Category="Stonekin|Weights")
	float TargetWeight = 2.0f;
	UPROPERTY(EditAnywhere, Category="Stonekin|Weights")
	float NeighborRange = 250.f;
	UPROPERTY(EditAnywhere, Category="Stonekin|Weights")
	float DesiredSeparation =120.f;
};
