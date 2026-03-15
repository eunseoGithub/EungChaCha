#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StonekinRenderManager.generated.h"

class UHierarchicalInstancedStaticMeshComponent;

UCLASS()
class EUNGCHACHA_API AStonekinRenderManager : public AActor
{
	GENERATED_BODY()
	
public:	
	AStonekinRenderManager();

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere,BlueprintReadWrite)
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> HismComponent;
	
	UPROPERTY()
	class UStonekinSimSubSystem* SimSubSystem;
	
public:	
	virtual void Tick(float DeltaTime) override;
};
