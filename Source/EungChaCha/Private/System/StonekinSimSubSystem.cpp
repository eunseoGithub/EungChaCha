// Fill out your copyright notice in the Description page of Project Settings.


#include "System/StonekinSimSubSystem.h"

#include "Async/ParallelFor.h"
#include "Landscape.h"
#include "StonekinSimManager.h"
#include "Kismet/GameplayStatics.h"
#include "System/MainGameInstance.h"
#include "LandscapeProxy.h"

void UStonekinSimSubSystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (UMainGameInstance* GI = UMainGameInstance::Get(this))
	{
		GI->GetLandscapeHeightMap();
	}
}

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
	Velocities.Add(FVector::ZeroVector);
	BoidsForces.Add(FVector::ZeroVector);
	Rotations.Add(FQuat::Identity);
	IdToIndexMap.Add(NewId,NewIndex);
	return NewId;
}


void UStonekinSimSubSystem::UpdateSimulation(float DeltaTime)
{
	if (Positions.Num()==0) return;
	if (Manager==nullptr) Manager = Cast<AStonekinSimManager>(UGameplayStatics::GetActorOfClass(GetWorld(),AStonekinSimManager::StaticClass()));
	if (Manager==nullptr) return;

	ComputeBoidsForces();
	ApplyMovement(DeltaTime);
}

// -------------------------------------------------------
// FSpatialHashGrid: Build
// 매 프레임 돌멩이 위치를 기반으로 그리드 재구축 (싱글스레드)
// -------------------------------------------------------
void FSpatialHashGrid::Build(const TArray<FVector>& Positions)
{
	const int32 N = Positions.Num();

	AgentCellHash.SetNum(N);
	SortedAgents.SetNum(N);
	CellStart.Init(-1, TableSize);
	CellEnd.Init(-1, TableSize);

	// Step 1: 각 에이전트의 셀 해시 계산
	for (int32 i = 0; i < N; i++)
	{
		SortedAgents[i]  = i;
		AgentCellHash[i] = HashCell(WorldToCell(Positions[i]));
	}

	// Step 2: 셀 해시 기준으로 SortedAgents 정렬
	SortedAgents.Sort([&](int32 A, int32 B)
	{
		return AgentCellHash[A] < AgentCellHash[B];
	});

	// Step 3: CellStart / CellEnd 기록
	for (int32 Idx = 0; Idx < N; Idx++)
	{
		int32 Hash = AgentCellHash[SortedAgents[Idx]];

		if (Idx == 0 || AgentCellHash[SortedAgents[Idx - 1]] != Hash)
			CellStart[Hash] = Idx;

		CellEnd[Hash] = Idx + 1;
	}
}

// -------------------------------------------------------
// FSpatialHashGrid: QueryNeighbors
// 특정 위치 기준 반경 내 에이전트 인덱스 반환 (읽기 전용, ParallelFor 안전)
// -------------------------------------------------------
void FSpatialHashGrid::QueryNeighbors(const FVector& Pos, float Radius,
                                       TArray<int32>& OutNeighbors) const
{
	FIntPoint Center = WorldToCell(Pos);

	// CellSize = NeighborRange 이면 Range = 1 → 주변 9셀만 검사
	int32 Range = FMath::CeilToInt(Radius / CellSize);

	for (int32 dx = -Range; dx <= Range; dx++)
	for (int32 dy = -Range; dy <= Range; dy++)
	{
		int32 Hash = HashCell(FIntPoint(Center.X + dx, Center.Y + dy));

		if (CellStart[Hash] == -1) continue;

		// 연속 메모리 순회 (캐시 히트)
		for (int32 Idx = CellStart[Hash]; Idx < CellEnd[Hash]; Idx++)
			OutNeighbors.Add(SortedAgents[Idx]);
	}
	// 해시 충돌로 인한 false positive는 Boids 루프의 Distance 체크로 자동 필터링됨
}

// -------------------------------------------------------
// 군체 알고리즘: 이웃 탐색 → Separation / Alignment / Cohesion 계산
// 결과를 BoidsForces[i] 에 저장, ApplyMovement 에서 사용
// -------------------------------------------------------
void UStonekinSimSubSystem::ComputeBoidsForces()
{
	const int32 N = Positions.Num();

	const float DesiredSep    = Manager->DesiredSeparation;
	const float SepWeight     = Manager->SepWeight;
	const float AliWeight     = Manager->AliWeight;
	const float CohWeight     = Manager->CohWeight;
	const float NeighborRange = Manager->NeighborRange;

	FVector FlatClickPos = CurrentClickPosition;
	FlatClickPos.Z = 100.f;

	// ─── Step 1: Grid Build (싱글스레드, 순서 보장) ───
	SpatialGrid.CellSize  = NeighborRange;  // 셀 크기 = 탐색 반경 → 항상 9셀만 검사
	SpatialGrid.TableSize = 2003;           // 1000개 기준 소수
	SpatialGrid.Build(Positions);
	// 이 시점부터 SpatialGrid는 읽기 전용

	// ─── Step 2: Boids 계산 (ParallelFor, 병렬) ───
	// BoidsForces[i]는 i번 스레드만 쓰므로 락 불필요
	ParallelFor(N, [&](int32 i)
	{
		FVector MyPos = Positions[i];
		MyPos.Z = 100.f;

		FVector Separation  = FVector::ZeroVector;
		FVector Alignment   = FVector::ZeroVector;
		FVector Cohesion    = FVector::ZeroVector;
		int32   NeighborCount = 0;

		// Grid 읽기 전용 조회 → 스레드 안전
		TArray<int32> Neighbors;
		SpatialGrid.QueryNeighbors(MyPos, NeighborRange, Neighbors);

		for (int32 j : Neighbors)
		{
			if (i == j) continue;

			FVector OtherPos = Positions[j];
			OtherPos.Z = 100.f;

			float Distance = FVector::Dist(MyPos, OtherPos);

			if (Distance < DesiredSep && Distance > 0.01f)
			{
				FVector Diff = (MyPos - OtherPos).GetSafeNormal();
				Separation  += Diff / Distance;
			}

			if (Distance < NeighborRange && Distance > 0.01f)
			{
				Alignment += (FlatClickPos - OtherPos).GetSafeNormal2D();
				Cohesion  += OtherPos;
				NeighborCount++;
			}
		}

		FVector BoidsForce = FVector::ZeroVector;
		if (NeighborCount > 0)
		{
			Alignment /= NeighborCount;
			Cohesion   = (Cohesion / NeighborCount) - MyPos;

			BoidsForce += Separation.GetSafeNormal2D() * SepWeight;
			BoidsForce += Alignment.GetSafeNormal2D()  * AliWeight;
			BoidsForce += Cohesion.GetSafeNormal2D()   * CohWeight;
		}

		BoidsForces[i] = BoidsForce;  // i번 인덱스에만 쓰기 → 스레드 안전
	});
}

// -------------------------------------------------------
// 이동: TargetDir + BoidsForces → Steering → 위치/회전 갱신
// -------------------------------------------------------
void UStonekinSimSubSystem::ApplyMovement(float DeltaTime)
{
	const int32 Size = Positions.Num();

	const float TargetWeight      = Manager->TargetWeight;
	const float MaxSpeed          = Manager->MaxSpeed;
	const float MaxSteeringForce  = Manager->MaxSteeringForce;
	const float ArrivalSlowRadius = Manager->ArrivalSlowRadius;
	const float ArrivalStopRadius = Manager->ArrivalStopRadius;
	const float MinArrivalScale   = Manager->MinArrivalScale;

	FVector FlattenedClickPos = CurrentClickPosition;
	FlattenedClickPos.Z = 100.0f;

	for (int32 i = 0; i < Size; ++i)
	{
		FVector MyPos = Positions[i];
		MyPos.Z = 100.f;

		float DistToTarget = FVector::Dist2D(MyPos, FlattenedClickPos);
		FVector TargetDir  = (FlattenedClickPos - MyPos).GetSafeNormal2D();

		//TargetDir 에만 ArrivalScale 적용 → Separation은 항상 살아있음
		float ArrivalScale;
		if      (DistToTarget <= ArrivalStopRadius) ArrivalScale = MinArrivalScale;
		else if (DistToTarget <  ArrivalSlowRadius) ArrivalScale = FMath::Lerp(MinArrivalScale, 1.f, (DistToTarget - ArrivalStopRadius) / (ArrivalSlowRadius - ArrivalStopRadius));
		else                                        ArrivalScale = 1.f;

		//목표 방향 힘(감속 적용) + 군체 힘 합산
		//Separation은 ArrivalScale에 영향받지 않아서, 정착한 돌도 뒤에서 밀리면 움직임
		FVector FinalForce = TargetDir * TargetWeight * ArrivalScale + BoidsForces[i];

		// 1. 목표 속도 계산
		FVector DesiredVel = FinalForce.GetSafeNormal2D() * MaxSpeed;

		// 2. 조향력 = 목표속도 - 현재속도 (급격한 전환을 MaxSteeringForce로 제한)
		FVector Steering = DesiredVel - Velocities[i];
		Steering = Steering.GetClampedToMaxSize2D(MaxSteeringForce * DeltaTime);

		// 3. 관성 적용: 현재 속도에 조향력을 더함
		FVector NewVel = Velocities[i] + Steering;
		NewVel = NewVel.GetClampedToMaxSize2D(MaxSpeed);

		//감속 처리: 목표 근처에서 속도를 줄이되 최소 20%는 유지 → 밀리면 반응 가능
		if (DistToTarget < ArrivalSlowRadius)
		{
			float DampFactor = DistToTarget / ArrivalSlowRadius;
			NewVel *= FMath::Lerp(0.2f, 1.f, DampFactor);
		}

		//위치 갱신 + 지형 높이
		FVector MoveDelta = NewVel * DeltaTime;
		FVector NewPos    = MyPos + MoveDelta;

		float TerrainHeight = GetStoneHeight(NewPos);
		NewPos.Z = TerrainHeight + 10.f;

		Positions[i]  = NewPos;
		Velocities[i] = NewVel;

		//돌멩이 구르기
		FVector MoveDir = NewVel.GetSafeNormal2D();
		if (!MoveDir.IsNearlyZero())
		{
			FVector RotationAxis = FVector::CrossProduct(FVector::UpVector, MoveDir);
			float AngleDelta = MoveDelta.Size() / 5.f;
			FQuat DeltaRot = FQuat(RotationAxis, AngleDelta);
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

float UStonekinSimSubSystem::GetStoneHeight(FVector CurrentPos)
{
	UMainGameInstance* GI = UMainGameInstance::Get(this);
	if (!GI) return 0.f;
	
	if (const TOptional<float> HeightOpt = GI->LandScapeActor->GetHeightAtLocation(CurrentPos,EHeightfieldSource::Complex))
	{
		float WorldZ = HeightOpt.GetValue();
		return WorldZ;
	}
	return 0.f;
}

void UStonekinSimSubSystem::SetClickPosition(const FVector& ClickPosition)
{
	this->CurrentClickPosition = ClickPosition;
}

FVector UStonekinSimSubSystem::GetClickPosition() const
{
	return CurrentClickPosition;
}

