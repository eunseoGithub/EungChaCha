#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StonekinSimManager.generated.h"

UCLASS()
class EUNGCHACHA_API AStonekinSimManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AStonekinSimManager();

	UPROPERTY(EditAnywhere, Category="Stonekin|Weights")
	float SepWeight = 4.0f;
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
	UPROPERTY(EditAnywhere, Category="Stonekin|Obstacle")
	float WallDetectRadius = 300.f;//벽 감지 시작 거리
	UPROPERTY(EditAnywhere, Category="Stonekin|Obstacle")
	float WallRepulsionForce = 500.f;//벽 반발력 세기
	UPROPERTY(EditAnywhere, Category="Stonekin|Arrival")
	float FibonacciSpacing = 30.f;
	UPROPERTY(EditAnywhere, Category="Stonekin|Arrival")
	float ProximityRadius = 200.f;
	UPROPERTY(EditAnywhere, Category="Stonekin|Arrival")
	float StillThreshold = 1.f;
	UPROPERTY(EditAnywhere, Category="Stonekin|Arrival")
	int32 StillFrameRequired = 10;
};
