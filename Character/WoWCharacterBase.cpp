#include "WoWCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "../Attributes/WoWAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayAbilitySpec.h"
#include "../Components/EffectApplicationComponent.h"
#include "../States/WoWPlayerState.h"

AWoWCharacterBase::AWoWCharacterBase()
{
    // Set this character to call Tick() every frame
    PrimaryActorTick.bCanEverTick = true;

    // Flag to track ability initialization
    bAbilitiesInitialized = false;
    EffectApplicationComponent = CreateDefaultSubobject<UEffectApplicationComponent>(TEXT("EffectApplicationComponent"));

}

void AWoWCharacterBase::BeginPlay()
{
    Super::BeginPlay();
    
    {
        EffectApplicationComponent->SetEffectDataAsset(EffectDataAsset);
    }

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
            // Apply the effect that sets base primary attributes
            FActiveGameplayEffectHandle ActiveGEHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
            
            // Now that primary attributes are set, calculate derived attributes
            UWoWAttributeSet* WoWAS = Cast<UWoWAttributeSet>(GetAttributeSet());
            if (WoWAS)
            {
                WoWAS->UpdateDerivedAttributes(ASC);
            }
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

float AWoWCharacterBase::GetMovementDirection() const
{
    const FVector ForwardVector = GetActorForwardVector();
    const FVector Velocity = GetVelocity();
    
    if (Velocity.SizeSquared2D() < 10.0f)
    {
        return 0.0f;
    }
    
    // Normalize the velocity vector
    const FVector VelocityNormalized = Velocity.GetSafeNormal2D();
    
    // Calculate the angle between the forward vector and velocity
    float ForwardDot = FVector::DotProduct(ForwardVector, VelocityNormalized);
    
    // Calculate the right vector dot product
    const FVector RightVector = FVector::CrossProduct(FVector::UpVector, ForwardVector);
    float RightDot = FVector::DotProduct(RightVector, VelocityNormalized);
    
    // Calculate the angle in degrees
    float AngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(RightDot, ForwardDot));
    
    return AngleDegrees;
}

EMovementStance AWoWCharacterBase::GetMovementStance() const
{
    // If not moving, return idle
    if (!IsMoving())
    {
        return EMovementStance::Idle;
    }
    
    // Get the movement direction angle (-180 to 180 degrees)
    float Direction = GetMovementDirection();
    
    // Normalize angle to -180 to 180 range
    while (Direction > 180.0f) Direction -= 360.0f;
    while (Direction < -180.0f) Direction += 360.0f;
    
    // Simple direction check for the 4 cardinal directions
    if (Direction >= -45.0f && Direction <= 45.0f)
    {
        return EMovementStance::Forward;
    }
    else if (Direction >= 45.0f && Direction <= 135.0f)
    {
        return EMovementStance::StrafeRight;
    }
    else if ((Direction >= 135.0f) || (Direction <= -135.0f))
    {
        return EMovementStance::Backward;
    }
    else // Between -45 and -135
    {
        return EMovementStance::StrafeLeft;
    }
}

EMovementStance AWoWCharacterBase::DirectionToStance(float Direction) const
{
    // Normalize angle to -180 to 180 range
    while (Direction > 180.0f) Direction -= 360.0f;
    while (Direction < -180.0f) Direction += 360.0f;
    
    // Forward: -45 to 45 degrees
    if (Direction >= -45.0f && Direction <= 45.0f)
    {
        return EMovementStance::Forward;
    }
    // Backward: 135 to 180 or -135 to -180 degrees
    else if (Direction >= 135.0f || Direction <= -135.0f)
    {
        return EMovementStance::Backward;
    }
    // Strafe Right: 45 to 135 degrees
    else if (Direction > 45.0f && Direction < 135.0f)
    {
        return EMovementStance::StrafeRight;
    }
    // Strafe Left: -45 to -135 degrees
    else
    {
        return EMovementStance::StrafeLeft;
    }
}

// Add to WoWCharacterBase.cpp
UAnimSequence* AWoWCharacterBase::GetCurrentAnimation() const
{
    // Get the current movement stance
    EMovementStance CurrentStance = GetMovementStance();
    
    // Return the appropriate animation
    switch (CurrentStance)
    {
        case EMovementStance::Forward:
            return ForwardAnimation;
            
        case EMovementStance::Backward:
            return BackwardAnimation;
            
        case EMovementStance::StrafeLeft:
            return StrafeLeftAnimation;
            
        case EMovementStance::StrafeRight:
            return StrafeRightAnimation;
            
        case EMovementStance::Idle:
        default:
            return IdleAnimation;
    }
}

float AWoWCharacterBase::GetMovementSpeed() const
{
    return GetVelocity().Size2D();
}

bool AWoWCharacterBase::IsMoving() const
{
    return GetMovementSpeed() > 10.0f;
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