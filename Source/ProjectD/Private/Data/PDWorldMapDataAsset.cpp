#include "Data/PDWorldMapDataAsset.h"
#include "Engine/World.h"

FPDWorldMapEntry UPDWorldMapDataAsset::GetEntryForWorld(const UWorld* InWorld) const
{
	if (!InWorld) return DefaultEntry;

	//현재 월드 에셋 path로 매핑 찾기
	const FSoftObjectPath CurrentWorldPath(InWorld);

	for (const TPair<TSoftObjectPtr<UWorld>, FPDWorldMapEntry>& Pair : LevelMaps)
	{
		if (Pair.Key.ToSoftObjectPath() == CurrentWorldPath)
		{
			return Pair.Value;
		}
	}

	return DefaultEntry;
}