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

void AWoWEnemyCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
}

void AWoWEnemyCharacter::InitializeEnemy()
{
    // Apply the default attribute effect, which will set primary stats
    if (DefaultAttributeEffect && GetAbilitySystemComponent())
    {
        // Apply enemy-type-based scaling to the primary attributes
        UWoWAttributeSet* AttributeSet = GetAttributeSet();
        if (AttributeSet)
        {
            // Apply level scaling (roughly 10% increase per level)
            float LevelScaling = 1.0f + ((Level - 1) * 0.1f);
            
            // Apply the effect to set initial attribute values
            FGameplayEffectContextHandle EffectContext = GetAbilitySystemComponent()->MakeEffectContext();
            EffectContext.AddSourceObject(this);
            
            FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(DefaultAttributeEffect, Level, EffectContext);
            
            if (SpecHandle.IsValid())
            {
                // Apply the initial attributes
                GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
                
                // Now fine-tune attributes based on enemy type
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
                float CurrentStrength = AttributeSet->GetStrength();
                float CurrentAgility = AttributeSet->GetAgility();
                float CurrentIntellect = AttributeSet->GetIntellect();
                float CurrentStamina = AttributeSet->GetStamina();
                float CurrentSpirit = AttributeSet->GetSpirit();
                
                // Apply modifiers
                AttributeSet->SetStrength(CurrentStrength * StrengthMod * LevelScaling);
                AttributeSet->SetAgility(CurrentAgility * AgilityMod * LevelScaling);
                AttributeSet->SetIntellect(CurrentIntellect * IntellectMod * LevelScaling);
                AttributeSet->SetStamina(CurrentStamina * StaminaMod * LevelScaling);
                AttributeSet->SetSpirit(CurrentSpirit * SpiritMod * LevelScaling);
                
                // Update derived attributes (health, mana, etc.)
                AttributeSet->UpdateDerivedAttributes(GetAbilitySystemComponent());
            }
        }
    }
    
    // Give the enemy its abilities
    GiveAbilities();
}

UBehaviorTree* AWoWEnemyCharacter::GetBehaviorTree() const
{
    return BehaviorTreeAsset;
}

float AWoWEnemyCharacter::CalculateAttackPower() const
{
    // Get the attribute set
    UWoWAttributeSet* AttributeSet = GetAttributeSet();
    if (!AttributeSet)
    {
        return 0.0f;
    }
    
    // Calculate attack power based on enemy type and primary stats
    float AttackPower = 0.0f;
    
    switch (EnemyType)
    {
        case EEnemyType::Melee:
            // Melee AP formula: Strength * 2 + Level
            AttackPower = AttributeSet->GetStrength() * 2.0f + Level;
            break;
            
        case EEnemyType::Ranged:
            // Ranged AP formula: Agility * 2 + Level
            AttackPower = AttributeSet->GetAgility() * 2.0f + Level;
            break;
            
        case EEnemyType::Caster:
            // Caster AP formula: Intellect * 2 + Level
            AttackPower = AttributeSet->GetIntellect() * 2.0f + Level;
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
        AWoWEnemyController* EnemyController = Cast<AWoWEnemyController>(GetController());
        if (EnemyController)
        {
            // Set target in blackboard
            EnemyController->SetTargetPawn(SeenPawn);
        }
    }
}