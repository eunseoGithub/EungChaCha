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
	LandScapeActor = nullptr;
	for (AActor* actor : GWorld->PersistentLevel->Actors)
	{
		if (actor->IsA<ALandscape>())
		{
			LandScapeActor = Cast<ALandscape>(actor);
			break;
		}
	}
	
	if (LandScapeActor== nullptr) return;
	
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
