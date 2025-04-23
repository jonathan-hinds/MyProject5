#include "BTTask_PerformAttack.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
// Use the correct relative path based on your project structure
#include "../../Character/WoWEnemyCharacter.h"

UBTTask_PerformAttack::UBTTask_PerformAttack()
{
    NodeName = "Perform Attack";
    TargetKey = "TargetActor";
}

EBTNodeResult::Type UBTTask_PerformAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // Get AI controller and enemy character
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return EBTNodeResult::Failed;
    }
    
    AWoWEnemyCharacter* EnemyCharacter = Cast<AWoWEnemyCharacter>(AIController->GetPawn());
    if (!EnemyCharacter)
    {
        return EBTNodeResult::Failed;
    }
    
    // Get target from blackboard
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return EBTNodeResult::Failed;
    }
    
    AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey));
    if (!TargetActor)
    {
        return EBTNodeResult::Failed;
    }
    
    // Get ability system component and try to activate the ability
    UAbilitySystemComponent* AbilitySystemComp = EnemyCharacter->GetAbilitySystemComponent();
    if (!AbilitySystemComp || !AttackAbility.Get())
    {
        return EBTNodeResult::Failed;
    }
    
    // Find the ability spec
    FGameplayAbilitySpec* AbilitySpec = nullptr;
    for (FGameplayAbilitySpec& Spec : AbilitySystemComp->GetActivatableAbilities())
    {
        if (Spec.Ability && Spec.Ability->GetClass() == AttackAbility)
        {
            AbilitySpec = &Spec;
            break;
        }
    }
    
    if (AbilitySpec)
    {
        // Create event data with target
        FGameplayEventData EventData;
        EventData.Target = TargetActor;
        
        // Try to activate the ability - UE5.4 version may have different signature
        if (AbilitySystemComp->TryActivateAbility(AbilitySpec->Handle))
        {
            return EBTNodeResult::Succeeded;
        }
    }
    
    return EBTNodeResult::Failed;
}