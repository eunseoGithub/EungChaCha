#include "UI/StonekinControlWidget.h"

#include "Components/Button.h"

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
}
