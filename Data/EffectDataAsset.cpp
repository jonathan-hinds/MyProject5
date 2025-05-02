// File: EffectDataAsset.cpp
#include "EffectDataAsset.h"

bool UEffectDataAsset::GetEffectDataByID(int32 EffectID, FEffectTableRow& OutEffectData) const
{
    if (!EffectsDataTable)
    {
        return false;
    }
    
    // Search for the effect with matching ID
    TArray<FEffectTableRow*> AllEffects;
    EffectsDataTable->GetAllRows<FEffectTableRow>(TEXT("GetEffectDataByID"), AllEffects);
    
    for (FEffectTableRow* EffectRow : AllEffects)
    {
        if (EffectRow && EffectRow->EffectID == EffectID)
        {
            OutEffectData = *EffectRow;
            return true;
        }
    }
    
    return false;
}

bool UEffectDataAsset::GetEffectsDataByIDs(const TArray<int32>& EffectIDs, TArray<FEffectTableRow>& OutEffectsData) const
{
    if (!EffectsDataTable || EffectIDs.Num() == 0)
    {
        return false;
    }
    
    // Clear the output array
    OutEffectsData.Empty();
    
    // Get all effects in the table
    TArray<FEffectTableRow*> AllEffects;
    EffectsDataTable->GetAllRows<FEffectTableRow>(TEXT("GetEffectsDataByIDs"), AllEffects);
    
    // Create a map for faster lookups
    TMap<int32, FEffectTableRow*> EffectMap;
    for (FEffectTableRow* EffectRow : AllEffects)
    {
        if (EffectRow)
        {
            EffectMap.Add(EffectRow->EffectID, EffectRow);
        }
    }
    
    // Find each requested effect
    bool FoundAll = true;
    for (int32 ID : EffectIDs)
    {
        FEffectTableRow* const* FoundEffect = EffectMap.Find(ID);
        if (FoundEffect && *FoundEffect)
        {
            OutEffectsData.Add(**FoundEffect);
        }
        else
        {
            FoundAll = false;
        }
    }
    
    return FoundAll && OutEffectsData.Num() > 0;
}