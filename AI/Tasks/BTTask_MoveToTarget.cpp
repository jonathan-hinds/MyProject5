#include "BTTask_MoveToTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"

UBTTask_MoveToTarget::UBTTask_MoveToTarget()
{
    NodeName = "Move To Target";
    TargetKey = "TargetActor";
    AcceptableRadius = 100.0f;  // Using parent class's AcceptableRadius
}

EBTNodeResult::Type UBTTask_MoveToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // Get AI controller
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController)
    {
        return EBTNodeResult::Failed;
    }
    
    // Get blackboard component
    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return EBTNodeResult::Failed;
    }
    
    // Get target from blackboard
    AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey));
    if (!TargetActor)
    {
        return EBTNodeResult::Failed;
    }
    
    // This is the simplest approach - let the parent class handle the move
    // and return InProgress to let the behavior tree continue checking
    AIController->MoveToActor(TargetActor, AcceptableRadius);
    return EBTNodeResult::InProgress;
}