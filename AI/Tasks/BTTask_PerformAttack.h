#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
// These are the crucial includes you need
#include "GameplayAbilities/Public/Abilities/GameplayAbility.h"
#include "GameplayAbilities/Public/AbilitySystemComponent.h"
#include "BTTask_PerformAttack.generated.h"

UCLASS()
class MYPROJECT5_API UBTTask_PerformAttack : public UBTTaskNode
{
    GENERATED_BODY()
    
public:
    UBTTask_PerformAttack();
    
    // Override ExecuteTask to implement the task logic
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    
protected:
    // Blackboard key for target actor
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FName TargetKey;
    
    // The ability to activate
    UPROPERTY(EditAnywhere, Category = "Attack")
    TSubclassOf<class UGameplayAbility> AttackAbility;
};