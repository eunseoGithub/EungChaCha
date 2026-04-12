#pragma once

#include "CoreMinimal.h"
#include "FQuadTree.h"
#include "Subsystems/WorldSubsystem.h"
#include "StonekinSimSubSystem.generated.h"

class AStonekinSimManager;

// -------------------------------------------------------
// Grid Hash 구조체 (SoA와 독립적으로 동작)
// -------------------------------------------------------
struct FSpatialHashGrid
{
	float CellSize  = 250.f;
	int32 TableSize = 2003;   // 에이전트 수 * 2 이상 소수

	TArray<int32> AgentCellHash;  // [AgentIdx] = 해당 에이전트의 셀 해시값
	TArray<int32> SortedAgents;   // 셀 해시 기준으로 정렬된 에이전트 인덱스
	TArray<int32> CellStart;      // [HashIdx] = 버킷 시작 위치 (-1: 비어있음)
	TArray<int32> CellEnd;        // [HashIdx] = 버킷 끝 위치

	// 월드 좌표 → 셀 좌표 (2D, Z 무시)
	FORCEINLINE FIntPoint WorldToCell(const FVector& Pos) const
	{
		return FIntPoint(
			FMath::FloorToInt(Pos.X / CellSize),
			FMath::FloorToInt(Pos.Y / CellSize)
		);
	}

	// 셀 좌표 → 해시값 (Matthias Müller 방식)
	FORCEINLINE int32 HashCell(const FIntPoint& Cell) const
	{
		uint32 H = ((uint32)(Cell.X * 92837111)
				  ^ (uint32)(Cell.Y * 689287499));
		return (int32)(H % (uint32)TableSize);
	}

	// 매 프레임 호출: 돌멩이 위치 기반으로 그리드 재구축
	void Build(const TArray<FVector>& Positions);

	// 특정 위치 기준 반경 내 에이전트 인덱스 반환
	void QueryNeighbors(const FVector& Pos, float Radius,
						TArray<int32>& OutNeighbors) const;
};

UCLASS()
class EUNGCHACHA_API UStonekinSimSubSystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
protected:
	void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual TStatId GetStatId() const override;
	void Tick(float DeltaTime) override;
	void Initialize(FSubsystemCollectionBase& Collection) override;
public:
	int32 AddEntity(FVector SpawnLocation);
	void UpdateSimulation(float DeltaTime);
	
	TArray<FVector> GetPositions() const;
	TArray<FQuat> GetRotations() const;
	
	UPROPERTY()
	TObjectPtr<AStonekinSimManager> Manager;
	
	void SetClickPosition(const FVector& ClickPosition);
	FVector GetClickPosition() const;
private:
	float GetStoneHeight(FVector CurrentPos);
	void ComputeBoidsForces();
	void ApplyMovement(float DeltaTime);
	void InitObstacleTree();
private:
	TArray<FVector> Velocities;
	TArray<FVector> Positions;
	TArray<FQuat> Rotations;
	TArray<FVector> BoidsForces;
	TArray<float> Stability;
	TArray<int32> EntityIds;
	
	TMap<int32, int32> IdToIndexMap;
	FVector CurrentClickPosition = {0.f, 0.f, 0.f};
	int32   NextId = 0;

	FSpatialHashGrid SpatialGrid;  // Grid Hash
	FQuadTree ObstacleTree;
public:
	
};
