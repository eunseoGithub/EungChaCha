// Fill out your copyright notice in the Description page of Project Settings.


#include "System/StonekinSimSubSystem.h"

#include "StonekinSimManager.h"
#include "Kismet/GameplayStatics.h"
#include "System/MainGameInstance.h"

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
	Velocities.Add(100.f);
	Rotations.Add(FQuat::Identity);
	IdToIndexMap.Add(NewId,NewIndex);
	return NewId;
}

void UStonekinSimSubSystem::UpdateSimulation(float DeltaTime)
{
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
	
	FVector FlattenedClickPos = CurrentClickPosition;
	FlattenedClickPos.Z = 100.0f;
	
	for (int32 i = 0 ; i < Size;++i)
	{
		FVector Separation = FVector::ZeroVector;
		FVector Alignment = FVector::ZeroVector;
		FVector Cohesion = FVector::ZeroVector;
		int32 NeighborCount = 0;
		
		FVector MyPos = Positions[i];
		MyPos.Z = 100.f;
		
		for (int32 j = 0 ; j <Size;j++)
		{
			if (i == j) continue;
			
			FVector OtherPos = Positions[j];
			OtherPos.Z = 100.f;
			
			float Distance = FVector::Dist(MyPos, OtherPos);
			if (Distance < DesiredSeparation && Distance > 0.01f)
			{
				FVector Diff = MyPos-OtherPos;//other 벡터가 MyPos을 바라보는 벡터
				Diff.Normalize();//Diff의 방향 벡터
				Separation += Diff/Distance;//MyPos이 저 바라보는 벡터
			}
			
			if (Distance < NeighborRange && Distance > 0.01f)
			{
				Alignment += (FlattenedClickPos - OtherPos).GetSafeNormal2D();//otherPos 벡터가 ClickPos 벡터를 바라보는 방향 벡터(Z는 무시)
				Cohesion += OtherPos;//이웃들의 위치를 다 더해줌
				NeighborCount++;
			}
		}
		FVector TargetDir = (FlattenedClickPos - MyPos).GetSafeNormal2D();
		FVector FinalForce = TargetDir * TargetWeight;
		
		if (NeighborCount > 0)
		{
			Alignment /= NeighborCount;
			Cohesion = (Cohesion/NeighborCount) - MyPos;
			
			FinalForce += (Separation.GetSafeNormal2D()* SepWeight);
			FinalForce += (Alignment.GetSafeNormal2D()*AliWeight);
			FinalForce += (Cohesion.GetSafeNormal2D()*CohWeight);
		}
		
		FVector FinalDir = FinalForce.GetSafeNormal2D();
		
		float Speed = Velocities[i];
		FVector MoveDelta = FinalDir * Speed * DeltaTime;
		
		FVector NewPos = MyPos + MoveDelta;
		
		float TerrainHeight = GetStoneHeight(NewPos);
		NewPos.Z = TerrainHeight + 10.f;
		
		Positions[i] = NewPos;
		if (!FinalDir.IsNearlyZero())
		{
			FVector RotationAxis = FVector::CrossProduct(FVector::UpVector,FinalDir);
			float AngleDelta = MoveDelta.Size()/50.f;
			FQuat DeltaRot = FQuat(RotationAxis,AngleDelta);
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
	if (GI->HeightData.Num()==0) return 0.f;
	
	int32 MinX = GI->LandscapeExtent.Min.X;
	int32 MinY = GI->LandscapeExtent.Min.Y;
	int32 MaxX = GI->LandscapeExtent.Max.X;
	int32 MaxY = GI->LandscapeExtent.Max.Y;
	
	int32 Width = MaxX - MinX +1;
	int32 Height = MaxY - MinY +1;
	
	if (GI->HeightData.Num() !=Width * Height)
	{
		UE_LOG(LogTemp,Error,TEXT("HeightData is not allowed"));
	}
	
	//World -> Landscape local(cm)
	const FVector Local = GI->LandscapeTransform.InverseTransformPosition(CurrentPos);
	
	//local = landscape 기준 로컬 공간에서 몇 cm 떨어져있는지
	//local(cm) -> Grid
	//if landscapeScale3D.X/Y is 100, 100 cm -> Grid 1
	const float ScaleX = FMath::Max(1e-6f, GI->LandscapeScale3D.X); // 1grid = 100cm
	const float ScaleY = FMath::Max(1e-6f, GI->LandscapeScale3D.Y); // 1grid = 100cm
	
	const int32 GridX = FMath::RoundToInt(Local.X / ScaleX); // local의 좌표가 몇번째 그리드인지 체크
	const int32 GridY = FMath::RoundToInt(Local.Y/ScaleY); // local의 좌표가 몇번째 그리드인지 체크
	
	int32 MapX = GridX - MinX;
	int32 MapY = GridY - MinY;
	
	MapX = FMath::Clamp(MapX,0,Width -1);//clamp로 index값 설정
	MapY = FMath::Clamp(MapY, 0, Height-1);//clamp로 index값 설정
	
	const uint16 RawData = GI->HeightData[MapY*Width + MapX];//행우선 배열에서 알맞는 그리드의 z값을 가져옴
	//여기서 RawData는 32768이 Landscape
	
	//utint16 -> height(cm)
	constexpr float MidValue = 32768.f;
	const float HeightLocalZ = (static_cast<float>(RawData) - MidValue) / 128.f* GI->LandscapeZScale;
	
	//offset world z
	const float WorldZ = GI->LandscapeTransform.GetLocation().Z + HeightLocalZ;
	
	return WorldZ;
	
}

void UStonekinSimSubSystem::SetClickPosition(const FVector& ClickPosition)
{
	this->CurrentClickPosition = ClickPosition;
}

FVector UStonekinSimSubSystem::GetClickPosition() const
{
	return CurrentClickPosition;
}

