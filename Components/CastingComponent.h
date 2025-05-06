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
    
    UFUNCTION(BlueprintPure, Category = "Casting")
    bool WasInterrupted() const;
    
    UFUNCTION(BlueprintPure, Category = "Casting")
    FString GetSpellName() const;
    
    UFUNCTION(BlueprintPure, Category = "Casting")
    float GetCastProgress() const;

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
    void ResetAfterInterrupt();

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
    
    FTimerHandle InterruptResetTimer;
    
    float GetWorldTime() const;
    void UpdateClientCastProgress(float DeltaTime);
};