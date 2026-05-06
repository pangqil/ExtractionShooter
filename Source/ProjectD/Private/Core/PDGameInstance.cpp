#include "Core/PDGameInstance.h"

void UPDGameInstance::SavePlayerData(const FPDPlayerData& InData)
{
	PlayerData=InData;
}

FPDPlayerData UPDGameInstance::LoadPlayerData() const
{
	return PlayerData;
}