#include "WoWCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "../Attributes/WoWAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayAbilitySpec.h"
#include "../States/WoWPlayerState.h"

AWoWCharacterBase::AWoWCharacterBase()
{
    // Set this character to call Tick() every frame
    PrimaryActorTick.bCanEverTick = true;

    // Flag to track ability initialization
    bAbilitiesInitialized = false;
}

void AWoWCharacterBase::BeginPlay()
{
    Super::BeginPlay();
    
    // Initialization moved to PossessedBy or OnRep_PlayerState
}

void AWoWCharacterBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AWoWCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}

UAbilitySystemComponent* AWoWCharacterBase::GetAbilitySystemComponent() const
{
    // ASC now comes from PlayerState for player characters
    const AWoWPlayerState* PS = Cast<AWoWPlayerState>(GetPlayerState());
    if (PS)
    {
        return PS->GetAbilitySystemComponent();
    }
    
    return nullptr;
}

UWoWAttributeSet* AWoWCharacterBase::GetAttributeSet() const
{
    // AttributeSet now comes from PlayerState for player characters
    const AWoWPlayerState* PS = Cast<AWoWPlayerState>(GetPlayerState());
    if (PS)
    {
        return PS->GetAttributeSet();
    }
    
    return nullptr;
}

void AWoWCharacterBase::InitializeAttributes()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    
    if (HasAuthority() && ASC && DefaultAttributeEffect)
    {
        // Create a context for the effect
        FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
        EffectContext.AddSourceObject(this);

        // Create a spec for the effect
        FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(DefaultAttributeEffect, 1, EffectContext);
        
        if (SpecHandle.IsValid())
        {
            // Apply the effect
            FActiveGameplayEffectHandle ActiveGEHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
        }
    }
}

void AWoWCharacterBase::GiveAbilities()
{
    UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
    
    if (HasAuthority() && ASC)
    {
        // Grant each ability in our list
        for (TSubclassOf<UGameplayAbility>& StartupAbility : DefaultAbilities)
        {
            if (StartupAbility)
            {
                // Create the ability spec
                ASC->GiveAbility(
                    FGameplayAbilitySpec(StartupAbility, 1, INDEX_NONE, this));
            }
        }
    }
}

void AWoWCharacterBase::ApplyStartupEffects()
{
    // Apply any startup gameplay effects here
}

float AWoWCharacterBase::GetHealthPercent() const
{
    UWoWAttributeSet* AttribSet = GetAttributeSet();
    if (AttribSet)
    {
        return AttribSet->GetHealth() / AttribSet->GetMaxHealth();
    }
    return 0.0f;
}

float AWoWCharacterBase::GetManaPercent() const
{
    UWoWAttributeSet* AttribSet = GetAttributeSet();
    if (AttribSet)
    {
        return AttribSet->GetMana() / AttribSet->GetMaxMana();
    }
    return 0.0f;
}

float AWoWCharacterBase::GetHealth() const
{
    UWoWAttributeSet* AttribSet = GetAttributeSet();
    if (AttribSet)
    {
        return AttribSet->GetHealth();
    }
    return 0.0f;
}

float AWoWCharacterBase::GetMaxHealth() const
{
    UWoWAttributeSet* AttribSet = GetAttributeSet();
    if (AttribSet)
    {
        return AttribSet->GetMaxHealth();
    }
    return 0.0f;
}

float AWoWCharacterBase::GetMana() const
{
    UWoWAttributeSet* AttribSet = GetAttributeSet();
    if (AttribSet)
    {
        return AttribSet->GetMana();
    }
    return 0.0f;
}

float AWoWCharacterBase::GetMaxMana() const
{
    UWoWAttributeSet* AttribSet = GetAttributeSet();
    if (AttribSet)
    {
        return AttribSet->GetMaxMana();
    }
    return 0.0f;
}