#include "WoWEnemyCharacter.h"
#include "AbilitySystemComponent.h"
#include "../Attributes/WoWAttributeSet.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Perception/PawnSensingComponent.h"
#include "../AI/WoWEnemyController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Math/UnrealMathUtility.h"

AWoWEnemyCharacter::AWoWEnemyCharacter()
{
    // Create pawn sensing component
    PawnSensingComponent = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("PawnSensingComponent"));
    
    // Create ability system component
    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

    // Create the attribute set
    AttributeSet = CreateDefaultSubobject<UWoWAttributeSet>(TEXT("AttributeSet"));
    
    // Create a static mesh component for the enemy's visual representation
    UStaticMeshComponent* MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    MeshComponent->SetupAttachment(RootComponent);
    MeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
    
    // Default values
    Level = 1;
    EnemyType = EEnemyType::Melee;
    WeaponDamageRange = FVector2D(5.0f, 7.0f);
    SightRadius = 800.0f;
    
    // Configure character movement
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 600.0f, 0.0f);
    
    // Configure AI controller class
    AIControllerClass = AWoWEnemyController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AWoWEnemyCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    // Initialize the ability system component
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
        UE_LOG(LogTemp, Warning, TEXT("Enemy %s initialized ASC"), *GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Enemy %s has no ASC!"), *GetName());
    }
    
    // Setup pawn sensing
    if (PawnSensingComponent)
    {
        PawnSensingComponent->OnSeePawn.AddDynamic(this, &AWoWEnemyCharacter::OnPawnSeen);
        PawnSensingComponent->SetPeripheralVisionAngle(60.0f);
        PawnSensingComponent->SightRadius = SightRadius;
    }
    
    // Initialize the enemy's stats and abilities
    InitializeEnemy();
}

UAbilitySystemComponent* AWoWEnemyCharacter::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

UWoWAttributeSet* AWoWEnemyCharacter::GetAttributeSet() const
{
    return AttributeSet;
}

void AWoWEnemyCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    
    // Initialize the ability system for this enemy
    if (AbilitySystemComponent)
    {
        // Make sure the ability system knows its owner/avatar
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
        
        // Refresh any gameplay cues
        AbilitySystemComponent->ForceReplication();
        
        UE_LOG(LogTemp, Warning, TEXT("Enemy %s PossessedBy - ASC initialized"), *GetName());
    }
}

void AWoWEnemyCharacter::InitializeEnemy()
{
    // Apply the default attribute effect, which will set primary stats
    if (DefaultAttributeEffect && AbilitySystemComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT("Enemy %s initializing attributes"), *GetName());
        
        // Apply level scaling (roughly 10% increase per level)
        float LevelScaling = 1.0f + ((Level - 1) * 0.1f);
        
        // Apply the effect to set initial attribute values
        FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
        EffectContext.AddSourceObject(this);
        
        FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributeEffect, Level, EffectContext);
        
        if (SpecHandle.IsValid())
        {
            // Apply the initial attributes
            FActiveGameplayEffectHandle ActiveGEHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
            
            if (ActiveGEHandle.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("Applied attribute effect to enemy %s"), *GetName());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to apply attribute effect to enemy %s"), *GetName());
            }
            
            // Now fine-tune attributes based on enemy type
            UWoWAttributeSet* WoWAS = Cast<UWoWAttributeSet>(AttributeSet);
            if (WoWAS)
            {
                float StrengthMod = 1.0f;
                float AgilityMod = 1.0f;
                float IntellectMod = 1.0f;
                float StaminaMod = 1.0f;
                float SpiritMod = 1.0f;
                
                // Adjust stat priorities based on enemy type
                switch (EnemyType)
                {
                    case EEnemyType::Melee:
                        StrengthMod = 1.5f;  // Melee enemies have more strength
                        StaminaMod = 1.2f;   // And more stamina for health
                        break;
                        
                    case EEnemyType::Ranged:
                        AgilityMod = 1.5f;   // Ranged enemies have more agility
                        break;
                        
                    case EEnemyType::Caster:
                        IntellectMod = 1.5f; // Casters have more intellect
                        SpiritMod = 1.2f;    // And more spirit for mana regen
                        break;
                }
                
                // Apply the type-specific and level-scaled modifiers
                float CurrentStrength = WoWAS->GetStrength();
                float CurrentAgility = WoWAS->GetAgility();
                float CurrentIntellect = WoWAS->GetIntellect();
                float CurrentStamina = WoWAS->GetStamina();
                float CurrentSpirit = WoWAS->GetSpirit();
                
                // Apply modifiers
                WoWAS->SetStrength(CurrentStrength * StrengthMod * LevelScaling);
                WoWAS->SetAgility(CurrentAgility * AgilityMod * LevelScaling);
                WoWAS->SetIntellect(CurrentIntellect * IntellectMod * LevelScaling);
                WoWAS->SetStamina(CurrentStamina * StaminaMod * LevelScaling);
                WoWAS->SetSpirit(CurrentSpirit * SpiritMod * LevelScaling);
                
                // Update derived attributes (health, mana, etc.)
                WoWAS->UpdateDerivedAttributes(AbilitySystemComponent);
                
                UE_LOG(LogTemp, Warning, TEXT("Enemy %s stats: STR=%.1f, AGI=%.1f, INT=%.1f, STA=%.1f, SPI=%.1f"), 
                       *GetName(), WoWAS->GetStrength(), WoWAS->GetAgility(), WoWAS->GetIntellect(), 
                       WoWAS->GetStamina(), WoWAS->GetSpirit());
                
                UE_LOG(LogTemp, Warning, TEXT("Enemy %s vitals: Health=%.1f/%.1f, Mana=%.1f/%.1f"), 
                       *GetName(), WoWAS->GetHealth(), WoWAS->GetMaxHealth(), 
                       WoWAS->GetMana(), WoWAS->GetMaxMana());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Enemy %s has no WoWAttributeSet!"), *GetName());
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create effect spec for enemy %s attributes"), *GetName());
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Enemy %s missing DefaultAttributeEffect or AbilitySystemComponent"), *GetName());
    }
    
    // Give the enemy its abilities
    if (AbilitySystemComponent)
    {
        GiveAbilities();
        UE_LOG(LogTemp, Warning, TEXT("Gave abilities to enemy %s"), *GetName());
    }
}

UBehaviorTree* AWoWEnemyCharacter::GetBehaviorTree() const
{
    return BehaviorTreeAsset;
}

float AWoWEnemyCharacter::CalculateAttackPower() const
{
    // Get the attribute set
    UWoWAttributeSet* WoWAS = Cast<UWoWAttributeSet>(AttributeSet);
    if (!WoWAS)
    {
        return 0.0f;
    }
    
    // Calculate attack power based on enemy type and primary stats
    float AttackPower = 0.0f;
    
    switch (EnemyType)
    {
        case EEnemyType::Melee:
            // Melee AP formula: Strength * 2 + Level
            AttackPower = WoWAS->GetStrength() * 2.0f + Level;
            break;
            
        case EEnemyType::Ranged:
            // Ranged AP formula: Agility * 2 + Level
            AttackPower = WoWAS->GetAgility() * 2.0f + Level;
            break;
            
        case EEnemyType::Caster:
            // Caster AP formula: Intellect * 2 + Level
            AttackPower = WoWAS->GetIntellect() * 2.0f + Level;
            break;
    }
    
    return AttackPower;
}

float AWoWEnemyCharacter::CalculateBaseDamage() const
{
    // Calculate base damage using weapon damage and attack power
    float AttackPower = CalculateAttackPower();
    
    // Random value between min and max weapon damage
    float WeaponDamage = FMath::RandRange(WeaponDamageRange.X, WeaponDamageRange.Y);
    
    // WoW-like formula: Weapon damage + (Attack Power / 14)
    float BaseDamage = WeaponDamage + (AttackPower / 14.0f);
    
    // Scale with level for balance
    return BaseDamage * (1.0f + (Level * 0.05f));
}

void AWoWEnemyCharacter::OnPawnSeen(APawn* SeenPawn)
{
    // If the seen pawn is a player, alert the AI controller
    if (SeenPawn && SeenPawn->IsPlayerControlled())
    {
        UE_LOG(LogTemp, Warning, TEXT("Enemy %s saw player %s"), *GetName(), *SeenPawn->GetName());
        
        AWoWEnemyController* EnemyController = Cast<AWoWEnemyController>(GetController());
        if (EnemyController)
        {
            // Set target in blackboard
            EnemyController->SetTargetPawn(SeenPawn);
        }
    }
}