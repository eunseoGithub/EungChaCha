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
	UPROPERTY(EditAnywhere, Category="Stonekin|Weights")
	float MaxSpeed = 150.f;
	UPROPERTY(EditAnywhere, Category="Stonekin|Weights")
	float MaxSteeringForce = 300.f;
	UPROPERTY(EditAnywhere, Category="Stonekin|Weights")
	float ArrivalSlowRadius = 300.f;// 이 거리부터 감속 시작
	UPROPERTY(EditAnywhere, Category="Stonekin|Weights")
	float ArrivalStopRadius = 80.f;//이 거리 안에서 완전 정지
};
