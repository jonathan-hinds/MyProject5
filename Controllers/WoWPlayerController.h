#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WoWPlayerController.generated.h"

UCLASS()
class MYPROJECT5_API AWoWPlayerController : public APlayerController
{
    GENERATED_BODY()
    
public:
    AWoWPlayerController();

protected:
    virtual void BeginPlay() override;
};