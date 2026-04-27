#include "UI/StonekinControlWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "System/StonekinSimSubSystem.h"

bool UStonekinControlWidget::Initialize()
{
	if (!Super::Initialize()) return false;
	
	if (AddStoneButton)
	{
		AddStoneButton->OnClicked.AddDynamic(this, &UStonekinControlWidget::OnAddClicked);
	}
	return true;
}

void UStonekinControlWidget::OnAddClicked()
{
	UStonekinSimSubSystem* Sim = GetWorld()->GetSubsystem<UStonekinSimSubSystem>();
	if (!Sim) return;
	for (int i = 0 ; i < 10; i++)
	{
		FVector SpawnLoc = FVector(
		FMath::RandRange(-500, 500),
		FMath::RandRange(-500, 500),
		100.f);
		Sim->AddEntity(SpawnLoc);
	}
	
	
	if (StoneCountText)
	{
		int32 Count = Sim->GetPositions().Num();
		StoneCountText->SetText(FText::AsNumber(Count));
	}
}
