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
	
	InitObstacleTree();
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
	
	AssignedSlots.Add(FVector::ZeroVector);
	bArrived.Add(true);
	StillFrameCount.Add(0);
	
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

	//Step 1: Grid Build (싱글스레드, 순서 보장)
	SpatialGrid.CellSize  = NeighborRange;  // 셀 크기 = 탐색 반경 → 항상 9셀만 검사
	SpatialGrid.TableSize = 2003;           // 1000개 기준 소수
	SpatialGrid.Build(Positions);
	// 이 시점부터 SpatialGrid는 읽기 전용

	//Step 2: Boids 계산 (ParallelFor, 병렬)
	ParallelFor(N, [&](int32 i)
	{
		FVector Pos = Positions[i];
		Pos.Z = 100.f;
		
		FVector Slot = AssignedSlots[i];
		Slot.Z = 100.f;
		
		FVector Separation  = FVector::ZeroVector;
		FVector Alignment   = FVector::ZeroVector;
		FVector Cohesion    = FVector::ZeroVector;
		int32   NeighborCount = 0;

		// Grid 읽기 전용 조회 → 스레드 안전
		TArray<int32> Neighbors;
		SpatialGrid.QueryNeighbors(Pos, NeighborRange, Neighbors);

		for (int32 j : Neighbors)
		{
			if (i == j) continue;

			FVector OtherPos = Positions[j];
			OtherPos.Z = 100.f;

			float Distance = FVector::Dist(Pos, OtherPos);

			if (Distance < DesiredSep && Distance > 0.01f)
			{
				FVector Diff = (Pos - OtherPos).GetSafeNormal();
				Separation  += Diff / Distance;
			}

			if (Distance < NeighborRange && Distance > 0.01f)
			{
				Alignment += (Slot - OtherPos).GetSafeNormal2D();
				Cohesion  += OtherPos;
				NeighborCount++;
			}
		}

		FVector BoidsForce = Separation.GetSafeNormal2D() * SepWeight;
		
		if (!bArrived[i] && NeighborCount > 0)
		{
			Alignment /= NeighborCount;
			Cohesion   = (Cohesion / NeighborCount) - Pos;
			BoidsForce += Alignment.GetSafeNormal2D()  * AliWeight;
			BoidsForce += Cohesion.GetSafeNormal2D()   * CohWeight;
		}
		
		const float WallDetect = Manager->WallDetectRadius;
		const float WallRepulse = Manager->WallRepulsionForce;
		
		FAABB SearchRange = FAABB::FromCenterExtent(
			FVector2D(Pos.X,Pos.Y),
			FVector2D(WallDetect, WallDetect)
		);
		TArray<FAABB> NearByWalls;
		ObstacleTree.Query(0, SearchRange, NearByWalls);
		
		for (const FAABB& Wall : NearByWalls)
		{
			float CloseX = FMath::Clamp(Pos.X, Wall.Min.X, Wall.Max.X);
			float CloseY = FMath::Clamp(Pos.Y, Wall.Min.Y, Wall.Max.Y);
			
			FVector2D Closest(CloseX, CloseY);
			FVector2D ToStone = FVector2D(Pos.X, Pos.Y) - Closest;
			float Distance = ToStone.Size();

			if (Distance >= WallDetect || Distance < 0.1f) continue;
			
			float Strength = 1.f - (Distance / WallDetect);
			FVector2D Repulsion = ToStone.GetSafeNormal() * Strength * WallRepulse;
			
			BoidsForce += FVector(Repulsion.X, Repulsion.Y, 0.f);
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

	const float TargetWeight = Manager->TargetWeight;
	const float MaxSpeed = Manager->MaxSpeed;
	const float MaxSteeringForce = Manager->MaxSteeringForce;
	const float ProximityRadius = Manager->ProximityRadius;
	const float StillThreshold = Manager->StillThreshold;
	const float StillFrameRequired = Manager->StillFrameRequired;
	
	for (int32 i = 0; i < Size; ++i)
	{
		FVector Pos = Positions[i];
		Pos.Z = 100.f;
		
		FVector Slot = AssignedSlots[i];
		Slot.Z = 100.f;

		if (bArrived[i])
		{
			Velocities[i] = FVector::ZeroVector;
			
			FVector PushDelta = BoidsForces[i].GetClampedToMaxSize2D(MaxSpeed * 0.3f) * DeltaTime;
			FVector NewPos = Pos + PushDelta;
			float TerrainHeight = GetStoneHeight(NewPos);
			NewPos.Z = TerrainHeight + 10.f;
			Positions[i] = NewPos;
			continue;
		}
		
		float DistToSlot = FVector::Dist2D(Pos, Slot);
		FVector TargetDir = (Slot - Pos).GetSafeNormal2D();
		FVector FinalForce = TargetDir * TargetWeight + BoidsForces[i];
		
		FVector DesireVel = FinalForce.GetSafeNormal2D() * MaxSpeed;
		FVector Steering = (DesireVel - Velocities[i]).GetClampedToMaxSize2D(MaxSteeringForce * DeltaTime);
		FVector NewVel = (Velocities[i] + Steering).GetClampedToMaxSize2D(MaxSpeed);
		
		if (DistToSlot < ProximityRadius)
		{
			float DampFactor = FMath::Clamp(DistToSlot / ProximityRadius, 0.1f, 1.f);
			NewVel *= DampFactor;
		}

		if (DistToSlot <= ProximityRadius)
		{
			if (NewVel.Size2D() < StillThreshold)
				StillFrameCount[i]++;
			else
				StillFrameCount[i] = 0;
			
			if (StillFrameCount[i] >= StillFrameRequired)
				bArrived[i] = true;
		}
		else
		{
			StillFrameCount[i] = 0;
		}
		
		//위치 갱신 + 지형 높이
		FVector MoveDelta = NewVel * DeltaTime;
		FVector NewPos    = Pos + MoveDelta;
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

void UStonekinSimSubSystem::InitObstacleTree()
{
	UWorld* World = GetWorld();
	if (!World) return;
	
	FAABB WorldBounds = FAABB::FromCenterExtent(
		FVector2D(0.f, 0.f),
		FVector2D(80000.f, 80000.f)
	);
	ObstacleTree.Init(WorldBounds);
	
	TArray<AActor*> ObstacleActors;
	UGameplayStatics::GetAllActorsWithTag(World, FName("Obstacle"), ObstacleActors);

	for (AActor* Actor : ObstacleActors)
	{
		if (!Actor) return;
		
		FVector Origin;
		FVector BoxExtent;
		Actor->GetActorBounds(false, Origin, BoxExtent);
		
		FAABB ObstacleBox = FAABB::FromCenterExtent(
			FVector2D(Origin.X, Origin.Y),
			FVector2D(BoxExtent.X, BoxExtent.Y)
		);
		
		ObstacleTree.Insert(0, ObstacleBox);
	}
	
	UE_LOG(LogTemp, Log, TEXT("ObstacleTree 초기화 완료 : 벽 %d개"), ObstacleActors.Num());
	
}

TArray<FVector> UStonekinSimSubSystem::GenerateFibonacciSlots(const FVector& ClickPos, int32 Count, float Spacing)
{
	TArray<FVector> Slots;
	Slots.Reserve(Count);
	
	const float GoldenAngle = 137.5077f * (PI / 180.f);//라디안
	
	for (int32 i = 0; i < Count; i++)
	{
		float Radius = Spacing * FMath::Sqrt((float)i);
		float Angle = i * GoldenAngle;
		
		float X = ClickPos.X + Radius * FMath::Cos(Angle);
		float Y = ClickPos.Y + Radius * FMath::Sin(Angle);
		float Z = GetStoneHeight(FVector(X, Y, 0.f)) + 10.f;
		
		Slots.Add(FVector(X, Y, Z));
	}
	
	return Slots;
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
	CurrentClickPosition = ClickPosition;
	
	const int32 N = Positions.Num();
	if (N == 0 || !Manager) return;
	
	TArray<FVector> Slots = GenerateFibonacciSlots(ClickPosition, N, Manager->FibonacciSpacing);

	Slots.Sort([&](const FVector& A, const FVector& B)
	{
		float AngleA = FMath::Atan2(A.Y - ClickPosition.Y, A.X - ClickPosition.X);
		float AngleB = FMath::Atan2(B.Y - ClickPosition.Y, B.X - ClickPosition.X);
		return AngleA < AngleB;
	});
	
	TArray<int32> SortedIndices;
	SortedIndices.SetNum(N);
	for (int32 i = 0; i < N; i++) SortedIndices[i] = i;

	SortedIndices.Sort([&](int32 A, int32 B)
	{
		float AngleA = FMath::Atan2(Positions[A].Y - ClickPosition.Y, Positions[A].X - ClickPosition.X);
		float AngleB = FMath::Atan2(Positions[B].Y - ClickPosition.Y, Positions[B].X - ClickPosition.X);
		return AngleA < AngleB;
	});

	for (int32 i = 0; i < N; i++)
	{
		int32 StoneIdx = SortedIndices[i];
		AssignedSlots[StoneIdx] = Slots[i];
		bArrived[StoneIdx] = false;
		StillFrameCount[StoneIdx] = 0;
	}
	
}

FVector UStonekinSimSubSystem::GetClickPosition() const
{
	return CurrentClickPosition;
}

