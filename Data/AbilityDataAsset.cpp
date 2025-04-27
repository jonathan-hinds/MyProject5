#include "AbilityDataAsset.h"

bool UAbilityDataAsset::GetAbilityDataByID(int32 AbilityID, FAbilityTableRow& OutAbilityData) const
{
    if (!AbilityDataTable)
    {
        return false;
    }
    
    TArray<FAbilityTableRow*> AllAbilities;
    AbilityDataTable->GetAllRows(TEXT("GetAbilityDataByID"), AllAbilities);
    
    for (FAbilityTableRow* AbilityRow : AllAbilities)
    {
        if (AbilityRow && AbilityRow->AbilityID == AbilityID)
        {
            OutAbilityData = *AbilityRow;
            return true;
        }
    }
    
    return false;
}

bool UAbilityDataAsset::GetAbilityDataBySlot(int32 SlotIndex, FAbilityTableRow& OutAbilityData) const
{
    if (!AbilityDataTable)
    {
        return false;
    }
    
    TArray<FAbilityTableRow*> AllAbilities;
    AbilityDataTable->GetAllRows(TEXT("GetAbilityDataBySlot"), AllAbilities);
    
    for (FAbilityTableRow* AbilityRow : AllAbilities)
    {
        if (AbilityRow && AbilityRow->DefaultHotbarSlot == SlotIndex)
        {
            OutAbilityData = *AbilityRow;
            return true;
        }
    }
    
    return false;
}