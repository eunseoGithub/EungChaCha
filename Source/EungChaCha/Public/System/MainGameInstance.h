#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MainGameInstance.generated.h"

class ALandscape;

UCLASS()
class EUNGCHACHA_API UMainGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	static UMainGameInstance* Get(const UObject* WorldContext);
	void UserSetting();
	void StartNewGame();
	void ContinueGame();
	void GetLandscapeHeightMap();
	TArray<uint16> HeightData;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	TObjectPtr<ALandscape> LandScapeActor;
protected:
	void Init() override;
	void OnStart() override;
	void Shutdown() override;
	
};
