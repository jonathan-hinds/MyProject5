#include "WoWEnemyController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "../Character/WoWEnemyCharacter.h"

AWoWEnemyController::AWoWEnemyController()
{
    // Create blackboard and behavior tree components
    BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
    BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
    
    // Default blackboard keys
    TargetKey = "TargetActor";
    HomeLocationKey = "HomeLocation";
}

void AWoWEnemyController::BeginPlay()
{
    Super::BeginPlay();
}

void AWoWEnemyController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    
    // Get the enemy character
    AWoWEnemyCharacter* EnemyCharacter = Cast<AWoWEnemyCharacter>(InPawn);
    if (EnemyCharacter)
    {
        // Initialize blackboard and start behavior tree
        UBehaviorTree* BehaviorTree = EnemyCharacter->GetBehaviorTree();
        if (BlackboardComponent && BehaviorTree && BehaviorTree->BlackboardAsset)
        {
            BlackboardComponent->InitializeBlackboard(*BehaviorTree->BlackboardAsset);
            
            // Set home location as the current location
            BlackboardComponent->SetValueAsVector(HomeLocationKey, EnemyCharacter->GetActorLocation());
            
            // Start behavior tree
            BehaviorTreeComponent->StartTree(*BehaviorTree);
        }
    }
}

void AWoWEnemyController::SetTargetPawn(APawn* TargetPawn)
{
    if (BlackboardComponent && TargetPawn)
    {
        BlackboardComponent->SetValueAsObject(TargetKey, TargetPawn);
    }
}