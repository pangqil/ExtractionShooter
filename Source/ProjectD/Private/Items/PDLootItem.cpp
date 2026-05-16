#include "Items/PDLootItem.h"

void APDLootItem::SetItemID(FName NewItemID)
{
	// protected 필드 — 자식이라 직접 접근 가능. BeginPlay 가 이후 LoadItemData 에서 본 값 사용.
	ItemRowName = NewItemID;
}
