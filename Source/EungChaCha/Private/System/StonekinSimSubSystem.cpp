// Fill out your copyright notice in the Description page of Project Settings.


#include "System/StonekinSimSubSystem.h"

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
// 군체 알고리즘: 이웃 탐색 → Separation / Alignment / Cohesion 계산
// 결과를 BoidsForces[i] 에 저장, ApplyMovement 에서 사용
// -------------------------------------------------------
void UStonekinSimSubSystem::ComputeBoidsForces()
{
	const int32 Size = Positions.Num();

	const float DesiredSeparation = Manager->DesiredSeparation;
	const float SepWeight         = Manager->SepWeight;
	const float AliWeight         = Manager->AliWeight;
	const float CohWeight         = Manager->CohWeight;
	const float NeighborRange     = Manager->NeighborRange;

	FVector FlattenedClickPos = CurrentClickPosition;
	FlattenedClickPos.Z = 100.0f;

	for (int32 i = 0; i < Size; ++i)
	{
		FVector Separation  = FVector::ZeroVector;
		FVector Alignment   = FVector::ZeroVector;
		FVector Cohesion    = FVector::ZeroVector;
		int32 NeighborCount = 0;

		FVector MyPos = Positions[i];
		MyPos.Z = 100.f;

		//나(i)와 다른(j) 비교
		for (int32 j = 0; j < Size; j++)
		{
			if (i == j) continue;//자기 자신은 건너뜀

			FVector OtherPos = Positions[j];
			OtherPos.Z = 100.f;

			float Distance = FVector::Dist(MyPos, OtherPos);

			if (Distance < DesiredSeparation && Distance > 0.01f)
			{
				FVector Diff = MyPos - OtherPos;//other 벡터가 MyPos을 바라보는 벡터
				Diff.Normalize();//Diff의 방향 벡터
				Separation += Diff / Distance;//가까울수록 강하게 밀어냄
			}

			if (Distance < NeighborRange && Distance > 0.01f)
			{
				Alignment += (FlattenedClickPos - OtherPos).GetSafeNormal2D();//이웃이 목표를 바라보는 방향
				Cohesion  += OtherPos;//이웃들의 위치를 다 더해줌
				NeighborCount++;
			}
		}

		FVector BoidsForce = FVector::ZeroVector;
		if (NeighborCount > 0)
		{
			Alignment /= NeighborCount;//이웃 방향 평균
			Cohesion   = (Cohesion / NeighborCount) - MyPos;//무리 중심을 향한 벡터

			BoidsForce += Separation.GetSafeNormal2D() * SepWeight;//밀어내기
			BoidsForce += Alignment.GetSafeNormal2D()  * AliWeight;//방향 맞추기
			BoidsForce += Cohesion.GetSafeNormal2D()   * CohWeight;//무리 따라가기
		}

		BoidsForces[i] = BoidsForce;
	}
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
			float AngleDelta = MoveDelta.Size() / 50.f;
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

