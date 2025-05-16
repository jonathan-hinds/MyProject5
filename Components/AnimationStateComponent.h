// AnimationStateComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h" // For replication
#include "Animation/AnimMontage.h" // For UAnimMontage
#include "AnimationStateComponent.generated.h"

class UAnimMontage;

// Simple enum for animation states
UENUM(BlueprintType)
enum class EAbilityAnimState : uint8
{
    Idle        UMETA(DisplayName = "Idle"),
    Casting     UMETA(DisplayName = "Casting"),
    SpellCast   UMETA(DisplayName = "SpellCast"),
    MeleeAttack UMETA(DisplayName = "MeleeAttack")
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYPROJECT5_API UAnimationStateComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UAnimationStateComponent();
    
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    
    // The current animation state
    UPROPERTY(ReplicatedUsing = OnRep_AnimState, BlueprintReadOnly, Category = "Animation")
    EAbilityAnimState CurrentAnimState;
    
    // Reference to montages
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    UAnimMontage* CastingMontage;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    UAnimMontage* SpellCastMontage;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
    UAnimMontage* MeleeAttackMontage;
    
    // Cast timing data for syncing animations
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Animation")
    float CastingPlayRate;
    
    // Handle state replication
    UFUNCTION()
    void OnRep_AnimState();

public:
    // Called by ability system
    UFUNCTION(BlueprintCallable, Category = "Animation")
    void NotifyCastingStarted(float CastTime, bool IsMelee = false);
    
    UFUNCTION(BlueprintCallable, Category = "Animation")
    void NotifyAbilityExecuted(bool IsMelee = false);
    
    UFUNCTION(BlueprintCallable, Category = "Animation")
    void NotifyCastingInterrupted();
    
    UFUNCTION(BlueprintCallable, Category = "Animation")
    void NotifyReturnToIdle();
    
    // Server RPCs
    UFUNCTION(Server, Reliable)
    void Server_NotifyCastingStarted(float CastTime, bool IsMelee);
    
    UFUNCTION(Server, Reliable)
    void Server_NotifyAbilityExecuted(bool IsMelee);
    
    UFUNCTION(Server, Reliable)
    void Server_NotifyCastingInterrupted();
    
    UFUNCTION(Server, Reliable)
    void Server_NotifyReturnToIdle();
    
    // Blueprint-exposed helpers
    UFUNCTION(BlueprintPure, Category = "Animation")
    bool IsCasting() const { return CurrentAnimState == EAbilityAnimState::Casting; }
    
    UFUNCTION(BlueprintPure, Category = "Animation")
    bool IsExecutingAbility() const { return CurrentAnimState == EAbilityAnimState::SpellCast || CurrentAnimState == EAbilityAnimState::MeleeAttack; }
    
    UFUNCTION(BlueprintPure, Category = "Animation")
    EAbilityAnimState GetCurrentAnimState() const { return CurrentAnimState; }
    
    UFUNCTION(BlueprintPure, Category = "Animation")
    UAnimMontage* GetCurrentMontage() const;
    
    UFUNCTION(BlueprintPure, Category = "Animation")
    float GetCastingPlayRate() const { return CastingPlayRate; }
};