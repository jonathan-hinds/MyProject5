#include "HealthManaComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Misc/OutputDeviceNull.h"
#include "../Character/WoWPlayerCharacter.h"
#include "TimerManager.h"

UHealthManaComponent::UHealthManaComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UHealthManaComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Delay UI creation slightly to ensure attributes are initialized
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UHealthManaComponent::CreateHUDWidget, 0.1f, false);
}

void UHealthManaComponent::CreateHUDWidget()
{
    APlayerController* PC = nullptr;
    APawn* Owner = Cast<APawn>(GetOwner());
    
    if (Owner)
    {
        PC = Cast<APlayerController>(Owner->GetController());
        
        if (PC && PC->IsLocalController() && HUDWidgetClass)
        {
            HUDWidget = CreateWidget<UUserWidget>(PC, HUDWidgetClass);
            
            if (HUDWidget)
            {
                HUDWidget->AddToViewport();
            }
        }
    }
}

void UHealthManaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}