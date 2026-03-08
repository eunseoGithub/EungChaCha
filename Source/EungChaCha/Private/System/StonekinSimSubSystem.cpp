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
	Rotations.Add(FQuat::Identity);
	IdToIndexMap.Add(NewId,NewIndex);
	return NewId;
}


// 매 프레임
// 	↓
// 각 엔티티 i에 대해:
// 	↓
// [이웃 탐색] 주변 j들로부터 Separation / Alignment / Cohesion 누적
// 	↓
// [힘 합산]   FinalForce = Target + Sep + Ali + Coh
// 	↓
// [감속 비율] 목표까지 거리로 ArrivalScale 계산
// 	↓
// [조향]      DesiredVel 목표로 조금씩 방향/속도 전환 (관성)
// 	↓
// [정지/감속] StopRadius → ZeroVector / SlowRadius → Damping
// 	↓
// [이동]      위치 갱신 + 지형 높이 적용
// 	↓
// [회전]      이동 방향으로 구르기
void UStonekinSimSubSystem::UpdateSimulation(float DeltaTime)
{
	//엔티티가 없으면 즉시 종료
	//Manager에서 파라미터를 매프레임 읽어옴
	//클릭 위치의 Z를 100으로 고정->모든 계산을 같은 높이에서 한다
	if (Positions.Num()==0) return;
	const int32 Size = Positions.Num();
	
	if (Manager==nullptr) Manager = Cast<AStonekinSimManager>(UGameplayStatics::GetActorOfClass(GetWorld(),AStonekinSimManager::StaticClass()));
	if (Manager==nullptr) return;
	
	const float DesiredSeparation = Manager->DesiredSeparation;
	const float SepWeight = Manager->SepWeight;
	const float AliWeight = Manager->AliWeight;
	const float CohWeight = Manager->CohWeight;
	const float TargetWeight = Manager->TargetWeight;
	const float NeighborRange = Manager->NeighborRange;
	const float MaxSpeed = Manager->MaxSpeed;
	const float MaxSteeringForce = Manager->MaxSteeringForce;
	const float ArrivalSlowRadius = Manager->ArrivalSlowRadius;
	const float ArrivalStopRadius = Manager->ArrivalStopRadius;
	
	FVector FlattenedClickPos = CurrentClickPosition;
	FlattenedClickPos.Z = 100.0f;
	
	//나(i)를 순서대로 처리
	for (int32 i = 0 ; i < Size;++i)
	{
		FVector Separation = FVector::ZeroVector;
		FVector Alignment = FVector::ZeroVector;
		FVector Cohesion = FVector::ZeroVector;
		int32 NeighborCount = 0;
		
		FVector MyPos = Positions[i];
		MyPos.Z = 100.f;
		
		//나(i)와 다른(j) 비교
		for (int32 j = 0 ; j <Size;j++)
		{
			if (i == j) continue;//자기 자신은 건너뜀
			
			FVector OtherPos = Positions[j];
			OtherPos.Z = 100.f;
			
			float Distance = FVector::Dist(MyPos, OtherPos);
			if (Distance < DesiredSeparation && Distance > 0.01f)
			{
				FVector Diff = MyPos-OtherPos;//other 벡터가 MyPos을 바라보는 벡터
				Diff.Normalize();//Diff의 방향 벡터
				Separation += Diff/Distance;//MyPos이 저 바라보는 벡터, 가까울수록 강하게 밀어냄
			}
			
			if (Distance < NeighborRange && Distance > 0.01f)
			{
				Alignment += (FlattenedClickPos - OtherPos).GetSafeNormal2D();//otherPos 벡터가 ClickPos 벡터를 바라보는 방향 벡터(Z는 무시)
				Cohesion += OtherPos;//이웃들의 위치를 다 더해줌
				NeighborCount++;
			}
		}
		float DistToTarget = FVector::Dist2D(MyPos, FlattenedClickPos);
		FVector TargetDir  = (FlattenedClickPos - MyPos).GetSafeNormal2D();

		FVector FinalForce = TargetDir * TargetWeight;// 목표 방향 힘
		
		if (NeighborCount > 0)
		{
			Alignment /= NeighborCount;//이웃 방향 평균
			Cohesion = (Cohesion/NeighborCount) - MyPos;// 무리 중심 - 내 위치를 뺀 값 = 중심을 향한 벡터
			
			FinalForce += (Separation.GetSafeNormal2D()* SepWeight);//밀어내기
			FinalForce += (Alignment.GetSafeNormal2D()*AliWeight);//방향 맞추기
			FinalForce += (Cohesion.GetSafeNormal2D()*CohWeight);//무리 따라가기
		}
		float ArrivalScale;
		if (DistToTarget <= ArrivalStopRadius) // 정지 구간
			ArrivalScale = 0.f;
		else if (DistToTarget <  ArrivalSlowRadius) // 감속 구간
			ArrivalScale = (DistToTarget - ArrivalStopRadius) / (ArrivalSlowRadius - ArrivalStopRadius);
		else// 전속력 구간
			ArrivalScale = 1.f;
		
		// 1. 목표 속도 계산
		FVector DesiredVel = FinalForce.GetSafeNormal2D() * MaxSpeed * ArrivalScale;

		// 2. 조향력 = 목표속도 - 현재속도 (급격한 전환을 MaxSteeringForce로 제한)
		FVector Steering = DesiredVel - Velocities[i];
		Steering = Steering.GetClampedToMaxSize2D(MaxSteeringForce * DeltaTime);

		// 3. 관성 적용: 현재 속도에 조향력을 더함
		FVector NewVel = Velocities[i] + Steering;
		NewVel = NewVel.GetClampedToMaxSize2D(MaxSpeed);
	
		if (DistToTarget <= ArrivalStopRadius)
		{
			NewVel = FVector::ZeroVector; // StopRadius 안 → 즉시 정지
		}
		else if (DistToTarget < ArrivalSlowRadius)
		{
			float DampFactor = DistToTarget / ArrivalSlowRadius;
			NewVel *= FMath::Lerp(0.f, 1.f, DampFactor); // 0.85 → 0 으로 강화
		}
		
		//위치 갱신 + 지형 높이
		FVector MoveDelta = NewVel * DeltaTime;
		FVector NewPos = MyPos + MoveDelta;

		float TerrainHeight = GetStoneHeight(NewPos);
		NewPos.Z = TerrainHeight + 10.f;

		Positions[i] = NewPos;
		Velocities[i] = NewVel;
		
		//돌멩이 구르기
		FVector MoveDir = NewVel.GetSafeNormal2D();  // FinalDir 대신 실제 이동 방향 사용
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

