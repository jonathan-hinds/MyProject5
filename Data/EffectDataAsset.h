// File: EffectDataAsset.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AbilityEffectTypes.h"
#include "EffectDataAsset.generated.h"

UCLASS()
class MYPROJECT5_API UEffectDataAsset : public UDataAsset
{
    GENERATED_BODY()
    
public:
    // The data table containing effect definitions
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
    UDataTable* EffectsDataTable;
    
    // Get effect data by ID
    UFUNCTION(BlueprintCallable, Category = "Effects")
    bool GetEffectDataByID(int32 EffectID, FEffectTableRow& OutEffectData) const;
    
    // Get multiple effect data entries by ID array
    UFUNCTION(BlueprintCallable, Category = "Effects")
    bool GetEffectsDataByIDs(const TArray<int32>& EffectIDs, TArray<FEffectTableRow>& OutEffectsData) const;
};