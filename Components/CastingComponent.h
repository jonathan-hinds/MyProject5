#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CastingComponent.generated.h"

class UWoWGameplayAbilityBase;

UENUM(BlueprintType)
enum class ECastingState : uint8
{
    Idle UMETA(DisplayName = "Idle"),
    Casting UMETA(DisplayName = "Casting"),
    Interrupted UMETA(DisplayName = "Interrupted")
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYPROJECT5_API UCastingComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UCastingComponent();
    
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    void NotifyCastStarted(const FString& InSpellName, float InCastTime, bool InCanCastWhileMoving);
    void NotifyCastCompleted();
    void NotifyCastInterrupted();
    
    UFUNCTION(BlueprintPure, Category = "Casting")
    bool IsCasting() const;
    
    UFUNCTION(BlueprintCallable, Category = "Casting")
    bool WasCastInterrupted() const { return CastingState == ECastingState::Interrupted; }

    // Add this function for the interrupted overlay
    UFUNCTION(BlueprintPure, Category = "Casting")
    bool IsInterrupted() const;
    
    UFUNCTION(BlueprintPure, Category = "Casting")
    FString GetSpellName() const;
    
    UFUNCTION(BlueprintPure, Category = "Casting")
    float GetCastProgress() const;
    
    UFUNCTION(BlueprintPure, Category = "Casting")
    bool CanCastWhileMoving() const { return bCanCastWhileMoving; }

protected:
    virtual void BeginPlay() override;
    
    UFUNCTION(Server, Reliable)
    void Server_NotifyCastStarted(const FString& InSpellName, float InCastTime, bool InCanCastWhileMoving);
    
    UFUNCTION(Server, Reliable)
    void Server_NotifyCastCompleted();
    
    UFUNCTION(Server, Reliable)
    void Server_NotifyCastInterrupted();
    
    UFUNCTION()
    void OnRep_CastingState();
    
    UFUNCTION()
    void ClearInterruptState();
    
    // Check if the owning character is moving
    bool IsOwnerMoving() const;
    
    // Get the owning character's velocity magnitude
    float GetOwnerVelocityMagnitude() const;

private:
    UPROPERTY(ReplicatedUsing = OnRep_CastingState)
    ECastingState CastingState;
    
    UPROPERTY(Replicated)
    FString SpellName;
    
    UPROPERTY(Replicated)
    float CastDuration;
    
    float CastStartTime;
    float CastEndTime;
    float ClientCastProgress;
    
    UPROPERTY(Replicated)
    bool bCanCastWhileMoving;
    
    // Flag to track if we're showing the interrupted state
    UPROPERTY(Replicated)
    bool bShowingInterrupted;
    
    // Movement threshold for interrupting casts (in unreal units per second)
    UPROPERTY(EditDefaultsOnly, Category = "Casting")
    float MovementInterruptThreshold;
    
    // Previous position for movement detection
    FVector PreviousLocation;
    
    // Timer handle for clearing the interrupted state
    FTimerHandle InterruptTimerHandle;
    
    float GetWorldTime() const;
    void UpdateClientCastProgress(float DeltaTime);
    
    // Check for movement interrupt
    void CheckMovementInterrupt();
};