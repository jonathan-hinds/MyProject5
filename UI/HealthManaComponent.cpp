#include "HealthManaComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
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
    // Only create the HUD for locally controlled players
    APlayerController* PC = nullptr;
    APawn* Owner = Cast<APawn>(GetOwner());
    
    if (Owner)
    {
        PC = Cast<APlayerController>(Owner->GetController());
        
        // Only create HUD widget for locally controlled pawns
        if (PC && PC->IsLocalController() && HUDWidgetClass)
        {
            // Create the HUD widget
            HUDWidget = CreateWidget<UUserWidget>(PC, HUDWidgetClass);
            
            if (HUDWidget)
            {
                // Add the widget to the viewport
                HUDWidget->AddToViewport();
            }
        }
    }
}

void UHealthManaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}