#pragma once

#include "CoreMinimal.h"
#include "Items/PDItemBase.h"
#include "PDLootItem.generated.h"

/**
 * 적 사망 드랍 전용 얇은 자식 — APDItemBase 의 protected ItemRowName 을
 * 자식 권한으로 외부에서 주입 가능하게 SetItemID 만 추가.
 *
 * 사용:
 *  - 스폰 직후(BeginPlay 전) SetItemID 호출 → APDItemBase::BeginPlay 가 새 row 를 로드.
 *  - BeginPlay 이후 호출은 다음 LoadItemData 까지 반영되지 않음(LoadItemData 가 private).
 *  - 따라서 SpawnActorDeferred → SetItemID → FinishSpawning 패턴으로 사용.
 *
 * 디자이너:
 *  - 이 클래스 기반 BP 한 개(예: BP_PDLootItem_Generic) 만들고 ItemDataTable=DT_ItemData 지정.
 *  - LootTable 의 ItemClass 슬롯은 본 클래스 자손만 노출.
 */
UCLASS(Blueprintable)
class PROJECTD_API APDLootItem : public APDItemBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PD|Item")
	void SetItemID(FName NewItemID);
};
