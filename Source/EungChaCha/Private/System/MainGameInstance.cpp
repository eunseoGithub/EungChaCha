// Fill out your copyright notice in the Description page of Project Settings.


#include "System/MainGameInstance.h"

#include "Landscape.h"
#if WITH_EDITOR
#include "LandscapeEdit.h"
#endif
#include "Kismet/GameplayStatics.h"

UMainGameInstance* UMainGameInstance::Get(const UObject* WorldContext)
{
	return Cast<UMainGameInstance>(UGameplayStatics::GetGameInstance(WorldContext));
}

void UMainGameInstance::UserSetting()
{
}

void UMainGameInstance::StartNewGame()
{
}

void UMainGameInstance::ContinueGame()
{
}

void UMainGameInstance::GetLandscapeHeightMap()
{
	ALandscape* LandScapeActor = nullptr;
	for (AActor* actor : GWorld->PersistentLevel->Actors)
	{
		if (actor->IsA<ALandscape>())
		{
			LandScapeActor = Cast<ALandscape>(actor);
			break;
		}
	}
	
	if (LandScapeActor== nullptr) return;// 여기까지 landscape 가져오기
	
	ULandscapeInfo* LandscapeInfo = LandScapeActor->GetLandscapeInfo();//landscape 컴포넌트 구성 얻어오기
	if (!LandscapeInfo) return;
	
	
	
	LandscapeInfo->ExportHeightmap(TEXT("D:\\UnrealProject\\EungChaCha\\Content\\Heightmap.png"));//디버그용 height map 뽑기
	
	//그리드 구해오기
	LandscapeInfo->GetLandscapeExtent(LandscapeExtent.Min.X, LandscapeExtent.Min.Y,LandscapeExtent.Max.X,LandscapeExtent.Max.Y);
	
	
	FLandscapeEditDataInterface LandScapeEdit = FLandscapeEditDataInterface(LandscapeInfo);
	
	//HeightData의 저장 공간 확보
	HeightData.Reset();
	int32 Width = LandscapeExtent.Max.X-LandscapeExtent.Min.X+1;
	int32 Height = LandscapeExtent.Max.Y-LandscapeExtent.Min.Y+1;
	HeightData.AddZeroed(Width * Height);
	
	//지정한 그리드 범위의 raw height를 행 우선 형태로 쭉 채워 넣음
	LandScapeEdit.GetHeightDataFast(LandscapeExtent.Min.X, LandscapeExtent.Min.Y, LandscapeExtent.Max.X,LandscapeExtent.Max.Y,HeightData.GetData(),0);
	
	LandscapeTransform = LandScapeActor->GetActorTransform();
	LandscapeZScale = LandScapeActor->GetTransform().GetScale3D().Z;
	LandscapeScale3D = LandscapeTransform.GetScale3D();
	
	// for (int32 i = 0; i < HeightData.Num();++i)
	// {
	// 	uint16 Data = HeightData[i];
	// 	
	// 	float FloatValue = ((float)Data - MidValue) * 256.f / MidValue;
	// 	float WorldHeight = FloatValue * LandscapeZScale;
	// 	float Offset = 1.0f;
	// 	float MoveHeight = WorldHeight + Offset;
	// 	uint16 MovedData = (uint16)((MoveHeight) / (LandscapeZScale * 256.f)*MidValue - MidValue);
	// 	HeightData[i] = MovedData;
	// }
	
}

void UMainGameInstance::Init()
{
	Super::Init();
}

void UMainGameInstance::OnStart()
{
	Super::OnStart();
}

void UMainGameInstance::Shutdown()
{
	Super::Shutdown();
}
