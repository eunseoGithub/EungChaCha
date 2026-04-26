#include "UI/MainHUD.h"

#include "Blueprint/UserWidget.h"
#include "UI/StonekinControlWidget.h"

void AMainHUD::BeginPlay()
{
	Super::BeginPlay();
	if (ControlWidgetClass)
	{
		ControlWidget = CreateWidget<UStonekinControlWidget>(GetWorld(), ControlWidgetClass);
		
		if (ControlWidget)
		{
			ControlWidget->AddToViewport();
		}
	}
}
