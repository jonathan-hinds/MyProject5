#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthManaComponent.generated.h"

class UUserWidget;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MYPROJECT5_API UHealthManaComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UHealthManaComponent();

protected:
    virtual void BeginPlay() override;

    // Create the HUD widget after a delay
    UFUNCTION()
    void CreateHUDWidget();

    // The widget class to spawn
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UUserWidget> HUDWidgetClass;
    
    // Reference to the created widget
    UPROPERTY()
    UUserWidget* HUDWidget;

public:    
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};