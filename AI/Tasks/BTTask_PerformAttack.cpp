#include "BTTask_PerformAttack.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "../../Character/WoWEnemyCharacter.h"
#include "../../Character/WoWPlayerCharacter.h"
#include "../../States/WoWPlayerState.h"
#include "AbilitySystemGlobals.h"

UBTTask_PerformAttack::UBTTask_PerformAttack()
{
    NodeName = "Perform Attack";
    TargetKey = "TargetActor";
}

EBTNodeResult::Type UBTTask_PerformAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    UE_LOG(LogTemp, Warning, TEXT("==== PERFORM ATTACK TASK STARTED ===="));
    
    // Get AI controller and enemy character
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get AI Controller"));
        return EBTNodeResult::Failed;
    }
    
    AWoWEnemyCharacter* EnemyCharacter = Cast<AWoWEnemyCharacter>(AIController->GetPawn());
    if (!EnemyCharacter)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get Enemy Character"));
        return EBTNodeResult::Failed;
    }
    
    // Get target from blackboard
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get BlackboardComponent"));
        return EBTNodeResult::Failed;
    }
    
    AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey));
    if (!TargetActor)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get Target Actor from Blackboard"));
        return EBTNodeResult::Failed;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Target found: %s"), *TargetActor->GetName());
    
    // Get ability system component and try to activate the ability
    UAbilitySystemComponent* AbilitySystemComp = EnemyCharacter->GetAbilitySystemComponent();
    if (!AbilitySystemComp)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get AbilitySystemComponent from enemy"));
        return EBTNodeResult::Failed;
    }
    
    // Check for cooldown tag to avoid attacking too quickly
    FGameplayTag CooldownTag = FGameplayTag::RequestGameplayTag(FName("Enemy.Attack.Cooldown"));
    if (AbilitySystemComp->HasMatchingGameplayTag(CooldownTag))
    {
        // Enemy is on cooldown, can't attack yet
        UE_LOG(LogTemp, Warning, TEXT("Enemy attack on cooldown"));
        return EBTNodeResult::Failed;
    }
    
    // Get target ASC - this handles both enemy and player targets
    UAbilitySystemComponent* TargetASC = nullptr;
    if (TargetActor->IsA(AWoWPlayerCharacter::StaticClass()))
    {
        // For player characters, try to get ASC from PlayerState
        UE_LOG(LogTemp, Warning, TEXT("Target is a player character, getting ASC from PlayerState"));
        AWoWPlayerCharacter* PlayerTarget = Cast<AWoWPlayerCharacter>(TargetActor);
        if (PlayerTarget)
        {
            AWoWPlayerState* PS = PlayerTarget->GetPlayerState<AWoWPlayerState>();
            if (PS)
            {
                TargetASC = PS->GetAbilitySystemComponent();
                UE_LOG(LogTemp, Warning, TEXT("Got ASC from PlayerState: %s"), 
                    TargetASC ? TEXT("Success") : TEXT("Failed"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to get PlayerState"));
            }
        }
    }
    else
    {
        // For non-player targets, get ASC directly
        TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
        UE_LOG(LogTemp, Warning, TEXT("Target is not a player, getting ASC directly: %s"), 
            TargetASC ? TEXT("Success") : TEXT("Failed"));
    }
    
    if (!AttackAbility.Get())
    {
        UE_LOG(LogTemp, Error, TEXT("Attack Ability is not set in BTTask_PerformAttack!"));
        return EBTNodeResult::Failed;
    }
    
    // Find the ability spec
    FGameplayAbilitySpec* AbilitySpec = nullptr;
    UE_LOG(LogTemp, Warning, TEXT("Searching for ability of class: %s"), *AttackAbility->GetName());
    UE_LOG(LogTemp, Warning, TEXT("Number of activatable abilities: %d"), AbilitySystemComp->GetActivatableAbilities().Num());
    
    for (FGameplayAbilitySpec& Spec : AbilitySystemComp->GetActivatableAbilities())
    {
        if (Spec.Ability)
        {
            UE_LOG(LogTemp, Warning, TEXT("Found ability: %s"), *Spec.Ability->GetName());
            if (Spec.Ability->GetClass() == AttackAbility)
            {
                AbilitySpec = &Spec;
                UE_LOG(LogTemp, Warning, TEXT("Found matching ability"));
                break;
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Found null ability in spec"));
        }
    }
    
    if (AbilitySpec)
    {
        UE_LOG(LogTemp, Warning, TEXT("AbilitySpec found"));
        
        // Try to activate the ability - UE 5.4 version
        if (AbilitySystemComp->TryActivateAbility(AbilitySpec->Handle))
        {
            UE_LOG(LogTemp, Warning, TEXT("Successfully activated ability"));
            
            // Calculate attack delay based on haste
            float HasteMultiplier = EnemyCharacter->GetHasteMultiplier();
            float BaseAttackSpeed = 10.0f;  // Base 10 second attack speed
            float MinAttackSpeed = 1.5f;    // Minimum attack speed
            
            // Apply the haste multiplier to the weapon base speed
            float AdjustedAttackDelay = BaseAttackSpeed * HasteMultiplier;
            
            // Ensure we don't go below minimum attack speed
            AdjustedAttackDelay = FMath::Max(AdjustedAttackDelay, MinAttackSpeed);
            
            // Add cooldown tag
            FGameplayTagContainer CooldownTags;
            CooldownTags.AddTag(CooldownTag);
            AbilitySystemComp->AddLooseGameplayTags(CooldownTags);
            
            // Set up a timer to remove the cooldown tag
            FTimerHandle CooldownTimerHandle;
            FTimerDelegate TimerDelegate;
            
            // Use weak pointer to avoid memory issues if enemy is destroyed
            TWeakObjectPtr<UAbilitySystemComponent> WeakASC = AbilitySystemComp;
            
            TimerDelegate.BindLambda([WeakASC, CooldownTag]()
            {
                if (WeakASC.IsValid())
                {
                    // Remove the tag when cooldown expires
                    FGameplayTagContainer TagsToRemove;
                    TagsToRemove.AddTag(CooldownTag);
                    WeakASC->RemoveLooseGameplayTags(TagsToRemove);
                }
            });
            
            // Start the timer with the adjusted attack delay
            AIController->GetWorld()->GetTimerManager().SetTimer(
                CooldownTimerHandle, 
                TimerDelegate, 
                AdjustedAttackDelay, 
                false
            );
            
            // Log the attack delay
            UE_LOG(LogTemp, Warning, TEXT("Enemy attack cooldown: %.2f seconds"), AdjustedAttackDelay);
            
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, 
                    FString::Printf(TEXT("Enemy attack speed: %.2fs"), AdjustedAttackDelay));
            }
            
            UE_LOG(LogTemp, Warning, TEXT("==== PERFORM ATTACK TASK SUCCEEDED ===="));
            return EBTNodeResult::Succeeded;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to activate ability"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("AbilitySpec not found for attack ability"));
    }
    
    UE_LOG(LogTemp, Warning, TEXT("==== PERFORM ATTACK TASK FAILED ===="));
    return EBTNodeResult::Failed;
}