#include "WoWPlayerState.h"
#include "AbilitySystemComponent.h"
#include "../Attributes/WoWAttributeSet.h"
#include "Net/UnrealNetwork.h"

AWoWPlayerState::AWoWPlayerState()
{
    // Create ability system component
    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

    // Create the attribute set
    AttributeSet = CreateDefaultSubobject<UWoWAttributeSet>(TEXT("AttributeSet"));

    // PlayerStates are always relevant, so the AbilitySystemComponent will also be
    NetUpdateFrequency = 100.0f;
}

void AWoWPlayerState::BeginPlay()
{
    Super::BeginPlay();
    
    // Only the server should initialize health
    if (GetLocalRole() == ROLE_Authority)
    {
        // Ensure health is set to max health initially
        UWoWAttributeSet* WoWAS = Cast<UWoWAttributeSet>(AttributeSet);
        if (WoWAS)
        {
            // This directly sets the attribute value, which will then replicate to clients
            WoWAS->SetHealth(WoWAS->GetMaxHealth());
        }
    }
}

UAbilitySystemComponent* AWoWPlayerState::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

UWoWAttributeSet* AWoWPlayerState::GetAttributeSet() const
{
    return AttributeSet;
}