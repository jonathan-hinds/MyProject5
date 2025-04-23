#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "BTTask_MoveToTarget.generated.h"

UCLASS()
class MYPROJECT5_API UBTTask_MoveToTarget : public UBTTask_MoveTo
{
    GENERATED_BODY()
    
public:
    UBTTask_MoveToTarget();
    
    // Override ExecuteTask to implement the task logic
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    
protected:
    // Blackboard key for target actor
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FName TargetKey;
};