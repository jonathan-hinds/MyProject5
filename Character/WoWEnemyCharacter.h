#pragma once

#include "CoreMinimal.h"
#include "WoWCharacterBase.h"
#include "WoWEnemyCharacter.generated.h"

class UBehaviorTree;
class UPawnSensingComponent;
class UAbilitySystemComponent;
class UWoWAttributeSet;

// Enemy type enum (melee, ranged, caster)
UENUM(BlueprintType)
enum class EEnemyType : uint8
{
    Melee       UMETA(DisplayName = "Melee"),
    Ranged      UMETA(DisplayName = "Ranged"),
    Caster      UMETA(DisplayName = "Caster")
};

UCLASS()
class MYPROJECT5_API AWoWEnemyCharacter : public AWoWCharacterBase
{
    GENERATED_BODY()
    
public:
    AWoWEnemyCharacter();

    // Override from base class to handle ability system setup for enemy
    virtual void PossessedBy(AController* NewController) override;
    
    // Called to initialize attributes and abilities
    virtual void InitializeEnemy();
    
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;
    
    // Returns the behavior tree asset to use for this enemy
    UFUNCTION(BlueprintCallable, Category = "AI")
    UBehaviorTree* GetBehaviorTree() const;
    
    // Calculate the enemy's attack power based on stats
    UFUNCTION(BlueprintCallable, Category = "Stats")
    float CalculateAttackPower() const;
    
    // Calculate base damage for an attack
    UFUNCTION(BlueprintCallable, Category = "Combat")
    float CalculateBaseDamage() const;
    
    // Override the base class functions to return the ASC on this enemy
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    virtual UWoWAttributeSet* GetAttributeSet() const override;

protected:
    // Component for detecting players
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UPawnSensingComponent* PawnSensingComponent;
    
    // Behavior tree asset for AI
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
    UBehaviorTree* BehaviorTreeAsset;
    
    // Enemy level (affects stats)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "1"))
    int32 Level;
    
    // Enemy type (affects stat priorities and abilities)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
    EEnemyType EnemyType;
    
    // Base weapon damage range (min-max)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
    FVector2D WeaponDamageRange;
    
    // Detection range
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
    float SightRadius;
    
    // Ability System Component for the enemy
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
    UAbilitySystemComponent* AbilitySystemComponent;

    // Attribute Set
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
    UWoWAttributeSet* AttributeSet;
    
    // Callback for when a pawn is seen
    UFUNCTION()
    void OnPawnSeen(APawn* SeenPawn);
};