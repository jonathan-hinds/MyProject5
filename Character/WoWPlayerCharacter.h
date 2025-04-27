#pragma once

#include "CoreMinimal.h"
#include "WoWCharacterBase.h"
#include "../Components/TargetingComponent.h"
#include "../Components/HotbarComponent.h"
#include "WoWPlayerCharacter.generated.h"


class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;

UCLASS()
class MYPROJECT5_API AWoWPlayerCharacter : public AWoWCharacterBase
{
    GENERATED_BODY()
    
public:
    AWoWPlayerCharacter();

    // Called to bind functionality to input
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    // Camera boom positioning the camera behind the character
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    USpringArmComponent* CameraBoom;

    // Default ability data asset for hotbar
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
    UAbilityDataAsset* DefaultAbilityDataAsset;

    // Follow camera
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
    UCameraComponent* FollowCamera;

    // Character mesh (temporary for testing)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh)
    UStaticMeshComponent* CharacterMesh;

    // Targeting component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Targeting")
    UTargetingComponent* TargetingComponent;

    // Hotbar component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
    UHotbarComponent* HotbarComponent;
    // Movement functions
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);

    // Targeting input handlers
    void OnTargetEnemy();
    void OnTargetAndAttackEnemy();

        // Hotbar input handlers
    void OnHotbarSlot1();
    void OnHotbarSlot2();
    void OnHotbarSlot3();
    void OnHotbarSlot4();
    void OnHotbarSlot5();
    void OnHotbarSlot6();
    void OnHotbarSlot7();
    void OnHotbarSlot8();
    void OnHotbarSlot9();
    void OnHotbarSlot10();
    void OnHotbarSlot11();
    void OnHotbarSlot12();

    // Generic hotbar slot handler
    void HandleHotbarSlot(int32 SlotIndex);

public:
    // Override from base class to handle ability system related setup for player
    virtual void PossessedBy(AController* NewController) override;
    
    // Called when this Pawn is replicated
    virtual void OnRep_PlayerState() override;

    // Get hotbar component
    UFUNCTION(BlueprintPure, Category = "Abilities")
    UHotbarComponent* GetHotbarComponent() const { return HotbarComponent; }
    
};