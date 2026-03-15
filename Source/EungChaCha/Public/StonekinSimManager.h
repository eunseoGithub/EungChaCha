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
	UPROPERTY(EditAnywhere, Category="Stonekin|Weights")
	float ArrivalSlowRadius = 600.f;// 이 거리부터 감속 시작
	UPROPERTY(EditAnywhere, Category="Stonekin|Weights")
	float ArrivalStopRadius = 80.f;//이 거리 안에서 감속 최대
	UPROPERTY(EditAnywhere, Category="Stonekin|Weights")
	float MinArrivalScale = 1.0f;//정착 후 남길 목표 힘 비율 (0=완전 포기, 높을수록 중앙으로 밀고 들어옴)
};
