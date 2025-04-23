#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "WoWEnemyController.generated.h"

class UBehaviorTreeComponent;
class UBlackboardComponent;

UCLASS()
class MYPROJECT5_API AWoWEnemyController : public AAIController
{
    GENERATED_BODY()
    
public:
    AWoWEnemyController();
    
    // Called when this controller possesses a pawn
    virtual void OnPossess(APawn* InPawn) override;
    
    // Set the target pawn in the blackboard
    void SetTargetPawn(APawn* TargetPawn);

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;
    
    // Behavior tree component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UBehaviorTreeComponent* BehaviorTreeComponent;
    
    // Blackboard component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
    UBlackboardComponent* BlackboardComponent;
    
    // Blackboard key for target actor
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
    FName TargetKey;
    
    // Blackboard key for enemy home location
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
    FName HomeLocationKey;
};