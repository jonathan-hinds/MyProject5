// TargetingComponent.cpp
#include "TargetingComponent.h"
#include "../Character/WoWCharacterBase.h"
#include "../Character/WoWEnemyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "../Abilities/WoWAutoAttackAbility.h"

UTargetingComponent::UTargetingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
    
    WeaponBaseSpeed = 10.0f;  // Base 10 second attack speed (at 1 agility)
    MinAttackSpeed = 1.5f;    // Minimum 1.5 second attack speed
    AutoAttackAbilityTag = "Ability.Attack.Melee";
    bIsAutoAttacking = false;
    MeleeAttackRange = 200.0f;
}

void UTargetingComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UTargetingComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
    DOREPLIFETIME(UTargetingComponent, CurrentTarget);
    DOREPLIFETIME(UTargetingComponent, bIsAutoAttacking);
}

void UTargetingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Check if the owner is locally controlled
    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (OwnerPawn && OwnerPawn->IsLocallyControlled())
    {
        // Handle target selection via mouse click
        HandleTargetSelection();
        
        // Handle auto-attack
        HandleAutoAttack();
    }
}

// In Components/TargetingComponent.cpp

void UTargetingComponent::HandleTargetSelection()
{
    // Get player controller
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC)
    {
        return;
    }
    
    // Check for right mouse button click to select target and start auto-attack
    if (PC->WasInputKeyJustPressed(EKeys::RightMouseButton))
    {
        // Perform trace from mouse position
        FHitResult HitResult;
        if (PC->GetHitResultUnderCursor(ECC_Visibility, true, HitResult))
        {
            // Check if we hit a targetable character (enemy or player)
            AWoWCharacterBase* TargetCharacter = Cast<AWoWCharacterBase>(HitResult.GetActor());
            if (TargetCharacter)
            {
                // Don't allow targeting self
                if (TargetCharacter != GetOwner())
                {
                    // Set as target
                    SetTarget(TargetCharacter);
                    
                    // Only start auto-attack if it's an enemy
                    AWoWEnemyCharacter* EnemyCharacter = Cast<AWoWEnemyCharacter>(TargetCharacter);
                    if (EnemyCharacter)
                    {
                        StartAutoAttack();
                    }
                    else
                    {
                        // It's a player, so don't auto-attack
                        StopAutoAttack();
                    }
                }
            }
            else
            {
                // Clicked on something that's not a character - clear target
                ClearTarget();
                StopAutoAttack();
            }
        }
        else
        {
            // No hit result - clicked on empty space, clear target
            ClearTarget();
            StopAutoAttack();
        }
    }
    
    // Check for left mouse button click to just target without attacking
    if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
    {
        // Perform trace from mouse position
        FHitResult HitResult;
        if (PC->GetHitResultUnderCursor(ECC_Visibility, true, HitResult))
        {
            // Check if we hit a targetable character (enemy or player)
            AWoWCharacterBase* TargetCharacter = Cast<AWoWCharacterBase>(HitResult.GetActor());
            if (TargetCharacter)
            {
                // Don't allow targeting self
                if (TargetCharacter != GetOwner())
                {
                    // Set as target (without auto-attack)
                    SetTarget(TargetCharacter);
                }
            }
            else
            {
                // Clicked on something that's not a character - clear target
                ClearTarget();
            }
        }
        else
        {
            // No hit result - clicked on empty space, clear target
            ClearTarget();
        }
    }
}

// In Components/TargetingComponent.cpp

void UTargetingComponent::HandleAutoAttack()
{
    // Check if we should be auto-attacking
    if (!bIsAutoAttacking || !HasValidTarget())
    {
        return;
    }
    
    // Only auto-attack enemies, not players
    AWoWEnemyCharacter* EnemyTarget = Cast<AWoWEnemyCharacter>(CurrentTarget);
    if (!EnemyTarget)
    {
        // Target is not an enemy, don't auto-attack
        StopAutoAttack();
        return;
    }
    
    // Check if we have a timer running
    if (GetWorld()->GetTimerManager().IsTimerActive(AutoAttackTimerHandle))
    {
        return;
    }
    
    // Calculate distance to target
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }
    
    // Calculate distance to target
    float DistanceToTarget = FVector::Dist(Owner->GetActorLocation(), CurrentTarget->GetActorLocation());
    
    // If we're too far away, don't perform the attack but keep auto-attack enabled
    // Just set a shorter timer to check again soon
    if (DistanceToTarget > MeleeAttackRange)
    {
        // Set a short timer to check again
        GetWorld()->GetTimerManager().SetTimer(AutoAttackTimerHandle, 0.5f, false);
        
        // Optional: Display a "Too far away" message if you want visual feedback
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 0.5f, FColor::Yellow, TEXT("Auto-attack active - target out of range"));
        }
        return;
    }
    
    // Get the ability system component
    IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Owner);
    if (!ASCInterface)
    {
        return;
    }
    
    UAbilitySystemComponent* AbilitySystemComponent = ASCInterface->GetAbilitySystemComponent();
    if (!AbilitySystemComponent)
    {
        return;
    }
    
    // Get the owner's haste multiplier
    float HasteMultiplier = 1.0f;
    AWoWCharacterBase* OwnerChar = Cast<AWoWCharacterBase>(Owner);
    if (OwnerChar)
    {
        HasteMultiplier = OwnerChar->GetHasteMultiplier();
    }
    
    // Apply the haste multiplier to the weapon base speed
    float AdjustedAttackDelay = WeaponBaseSpeed * HasteMultiplier;
    
    // Ensure we don't go below minimum attack speed
    AdjustedAttackDelay = FMath::Max(AdjustedAttackDelay, MinAttackSpeed);
    
    // Activate the auto attack ability
    FGameplayTagContainer AbilityTagContainer;
    AbilityTagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName(AutoAttackAbilityTag)));
    
    // Activate an ability with the matching tags
    AbilitySystemComponent->TryActivateAbilitiesByTag(AbilityTagContainer);
    
    // Set timer for next attack using adjusted delay
    GetWorld()->GetTimerManager().SetTimer(AutoAttackTimerHandle, AdjustedAttackDelay, false);
}

void UTargetingComponent::SetTarget(AActor* NewTarget)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, 
        FString::Printf(TEXT("SetTarget: %s"), NewTarget ? *NewTarget->GetName() : TEXT("None")));
    // If we're not the server, send RPC to server
    if (GetOwnerRole() < ROLE_Authority)
    {
        Server_SetTarget(NewTarget);
    }
    else
    {
        // We are the server, set directly
        CurrentTarget = NewTarget;
    }
}

bool UTargetingComponent::Server_SetTarget_Validate(AActor* NewTarget)
{
    return true;
}

void UTargetingComponent::Server_SetTarget_Implementation(AActor* NewTarget)
{
    CurrentTarget = NewTarget;
}

void UTargetingComponent::ClearTarget()
{
    // If we're not the server, send RPC to server
    if (GetOwnerRole() < ROLE_Authority)
    {
        Server_ClearTarget();
    }
    else
    {
        // We are the server, set directly
        CurrentTarget = nullptr;
    }
}

bool UTargetingComponent::Server_ClearTarget_Validate()
{
    return true;
}

void UTargetingComponent::Server_ClearTarget_Implementation()
{
    CurrentTarget = nullptr;
}

bool UTargetingComponent::HasValidTarget() const
{
    // Use IsValid instead of IsPendingKill
    return CurrentTarget != nullptr && IsValid(CurrentTarget);
}

float UTargetingComponent::GetTargetHealthPercent() const
{
    if (!HasValidTarget())
    {
        return 0.0f;
    }
    
    AWoWCharacterBase* TargetCharacter = Cast<AWoWCharacterBase>(CurrentTarget);
    if (TargetCharacter)
    {
        return TargetCharacter->GetHealthPercent();
    }
    
    return 0.0f;
}

float UTargetingComponent::GetTargetManaPercent() const
{
    if (!HasValidTarget())
    {
        return 0.0f;
    }
    
    AWoWCharacterBase* TargetCharacter = Cast<AWoWCharacterBase>(CurrentTarget);
    if (TargetCharacter)
    {
        return TargetCharacter->GetManaPercent();
    }
    
    return 0.0f;
}

void UTargetingComponent::GetTargetHealth(float& Health, float& MaxHealth) const
{
    if (!HasValidTarget())
    {
        Health = 0.0f;
        MaxHealth = 0.0f;
        return;
    }
    
    AWoWCharacterBase* TargetCharacter = Cast<AWoWCharacterBase>(CurrentTarget);
    if (TargetCharacter)
    {
        Health = TargetCharacter->GetHealth();
        MaxHealth = TargetCharacter->GetMaxHealth();
    }
    else
    {
        Health = 0.0f;
        MaxHealth = 0.0f;
    }
}

void UTargetingComponent::GetTargetMana(float& Mana, float& MaxMana) const
{
    if (!HasValidTarget())
    {
        Mana = 0.0f;
        MaxMana = 0.0f;
        return;
    }
    
    AWoWCharacterBase* TargetCharacter = Cast<AWoWCharacterBase>(CurrentTarget);
    if (TargetCharacter)
    {
        Mana = TargetCharacter->GetMana();
        MaxMana = TargetCharacter->GetMaxMana();
    }
    else
    {
        Mana = 0.0f;
        MaxMana = 0.0f;
    }
}

// In Components/TargetingComponent.cpp

FString UTargetingComponent::GetTargetName() const
{
    if (!HasValidTarget())
    {
        return FString();
    }
    
    // For players, try to get a more friendly name
    AController* TargetController = nullptr;
    APawn* TargetPawn = Cast<APawn>(CurrentTarget);
    if (TargetPawn)
    {
        TargetController = TargetPawn->GetController();
    }
    
    if (TargetController && TargetController->IsPlayerController())
    {
        // It's a player character, try to get the player name
        APlayerState* PS = TargetController->PlayerState;
        if (PS)
        {
            return PS->GetPlayerName();
        }
    }
    
    // Fallback to actor name
    return CurrentTarget->GetName();
}

void UTargetingComponent::StartAutoAttack()
{
    if (!HasValidTarget())
    {
        return;
    }
    
    // If we're not the server, send RPC to server
    if (GetOwnerRole() < ROLE_Authority)
    {
        Server_StartAutoAttack();
    }
    else
    {
        // We are the server, set directly
        bIsAutoAttacking = true;
    }
    
    // Reset timer to start checking immediately - this is fine to do locally
    GetWorld()->GetTimerManager().ClearTimer(AutoAttackTimerHandle);
    
    // Optional: Display a message that auto-attack is active
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("Auto-attack started"));
    }
}

bool UTargetingComponent::Server_StartAutoAttack_Validate()
{
    return true;
}

void UTargetingComponent::Server_StartAutoAttack_Implementation()
{
    bIsAutoAttacking = true;
}

void UTargetingComponent::StopAutoAttack()
{
    // If we're not the server, send RPC to server
    if (GetOwnerRole() < ROLE_Authority)
    {
        Server_StopAutoAttack();
    }
    else
    {
        // We are the server, set directly
        bIsAutoAttacking = false;
    }
    
    // Clear timer locally regardless
    GetWorld()->GetTimerManager().ClearTimer(AutoAttackTimerHandle);
}

bool UTargetingComponent::Server_StopAutoAttack_Validate()
{
    return true;
}

void UTargetingComponent::Server_StopAutoAttack_Implementation()
{
    bIsAutoAttacking = false;
}