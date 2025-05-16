// AnimationStateComponent.cpp
#include "AnimationStateComponent.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h" // For UAnimInstance
#include "Components/SkeletalMeshComponent.h" // For playing montages

UAnimationStateComponent::UAnimationStateComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
    
    CurrentAnimState = EAbilityAnimState::Idle;
    CastingPlayRate = 1.0f;
}

void UAnimationStateComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UAnimationStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UAnimationStateComponent, CurrentAnimState);
    DOREPLIFETIME(UAnimationStateComponent, CastingPlayRate);
}

void UAnimationStateComponent::OnRep_AnimState()
{
    // Animation blueprint will handle playing the appropriate animation
    // We don't need to play montages directly here
}

UAnimMontage* UAnimationStateComponent::GetCurrentMontage() const
{
    switch (CurrentAnimState)
    {
        case EAbilityAnimState::Casting:
            return CastingMontage;
        case EAbilityAnimState::SpellCast:
            return SpellCastMontage;
        case EAbilityAnimState::MeleeAttack:
            return MeleeAttackMontage;
        default:
            return nullptr;
    }
}

void UAnimationStateComponent::NotifyCastingStarted(float CastTime, bool IsMelee)
{
    // Calculate play rate based on cast time
    if (CastingMontage && CastTime > 0.0f)
    {
        float MontageLength = CastingMontage->GetPlayLength();
        if (MontageLength > 0.0f)
        {
            CastingPlayRate = MontageLength / CastTime;
        }
        else
        {
            CastingPlayRate = 1.0f;
        }
    }
    else
    {
        CastingPlayRate = 1.0f;
    }
    
    // Update animation state
    CurrentAnimState = EAbilityAnimState::Casting;
    
    // Call server RPC if this is a client
    if (GetOwnerRole() < ROLE_Authority)
    {
        Server_NotifyCastingStarted(CastTime, IsMelee);
    }
}

void UAnimationStateComponent::NotifyAbilityExecuted(bool IsMelee)
{
    // Set appropriate animation state
    CurrentAnimState = IsMelee ? EAbilityAnimState::MeleeAttack : EAbilityAnimState::SpellCast;
    
    // Call server RPC if this is a client
    if (GetOwnerRole() < ROLE_Authority)
    {
        Server_NotifyAbilityExecuted(IsMelee);
    }
}

void UAnimationStateComponent::NotifyCastingInterrupted()
{
    // Just return to idle state
    CurrentAnimState = EAbilityAnimState::Idle;
    
    // Call server RPC if this is a client
    if (GetOwnerRole() < ROLE_Authority)
    {
        Server_NotifyCastingInterrupted();
    }
}

void UAnimationStateComponent::NotifyReturnToIdle()
{
    // Update animation state
    CurrentAnimState = EAbilityAnimState::Idle;
    
    // Call server RPC if this is a client
    if (GetOwnerRole() < ROLE_Authority)
    {
        Server_NotifyReturnToIdle();
    }
}

void UAnimationStateComponent::Server_NotifyCastingStarted_Implementation(float CastTime, bool IsMelee)
{
    NotifyCastingStarted(CastTime, IsMelee);
}

void UAnimationStateComponent::Server_NotifyAbilityExecuted_Implementation(bool IsMelee)
{
    NotifyAbilityExecuted(IsMelee);
}

void UAnimationStateComponent::Server_NotifyCastingInterrupted_Implementation()
{
    NotifyCastingInterrupted();
}

void UAnimationStateComponent::Server_NotifyReturnToIdle_Implementation()
{
    NotifyReturnToIdle();
}