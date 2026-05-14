// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/HUD/PDHUDWidget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet/PDAttributeSet.h"
#include "GameFramework/Pawn.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Type/Types.h"
#include "Widgets/HUD/PDAttributeBarWidget.h"
#include "Widgets/HUD/PDBodyPartHealthGroupWidget.h"
#include "Widgets/HUD/PDNewQuickSlotBarWidget.h"
#include "Widgets/HUD/PDDebuffIconBarWidget.h"
#include "Widgets/Crosshair/PDCrosshairWidget.h"
#include "Items/PDQuickSlotComponent.h"

UPDHUDWidget::UPDHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// HUD는 입력 모드를 절대 건드리면 안 됨. 베이스 기본값 Menu가 FInputModeUIOnly를 강제하는 사고 방지.
	InputMode = EWidgetInputMode::Passive;
}

void UPDHUDWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	APawn* Pawn = GetOwningPlayerPawn();
	UAbilitySystemComponent* ASC = Pawn
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn)
		: nullptr;
	RebindToASC(ASC);

	RefreshNewQuickSlots();
}

void UPDHUDWidget::NativeOnDeactivated()
{
	RebindToASC(nullptr);
	Super::NativeOnDeactivated();
}

void UPDHUDWidget::RebindToASC(UAbilitySystemComponent* NewASC)
{
	if (CachedASC.Get() == NewASC) return;

	UnbindAllAttributes();
	UnbindAllTags();

	CachedASC = NewASC;
	if (!NewASC) return;

	BindAllAttributes();
	BindAllDebuffTags();
}

void UPDHUDWidget::BindAllAttributes()
{
	BindAttributeToBar(Bar_Stamina,
		UPDAttributeSet::GetStaminaAttribute(),
		UPDAttributeSet::GetMaxStaminaAttribute());

	BindAttributeToBar(Bar_Hunger,
		UPDAttributeSet::GetHungerAttribute(),
		UPDAttributeSet::GetMaxHungerAttribute());

	BindAttributeToBar(Bar_Thirst,
		UPDAttributeSet::GetThirstAttribute(),
		UPDAttributeSet::GetMaxThirstAttribute());

	if (Bar_BodyParts)
	{
		BindAttributeToBar(Bar_BodyParts->GetBar(EBodyPart::Head),
			UPDAttributeSet::GetHeadHPAttribute(),
			UPDAttributeSet::GetMaxHeadHPAttribute());

		BindAttributeToBar(Bar_BodyParts->GetBar(EBodyPart::Torso),
			UPDAttributeSet::GetTorsoHPAttribute(),
			UPDAttributeSet::GetMaxTorsoHPAttribute());

		BindAttributeToBar(Bar_BodyParts->GetBar(EBodyPart::Arm_L),
			UPDAttributeSet::GetArmLHPAttribute(),
			UPDAttributeSet::GetMaxArmLHPAttribute());

		BindAttributeToBar(Bar_BodyParts->GetBar(EBodyPart::Arm_R),
			UPDAttributeSet::GetArmRHPAttribute(),
			UPDAttributeSet::GetMaxArmRHPAttribute());

		BindAttributeToBar(Bar_BodyParts->GetBar(EBodyPart::Leg_L),
			UPDAttributeSet::GetLegLHPAttribute(),
			UPDAttributeSet::GetMaxLegLHPAttribute());

		BindAttributeToBar(Bar_BodyParts->GetBar(EBodyPart::Leg_R),
			UPDAttributeSet::GetLegRHPAttribute(),
			UPDAttributeSet::GetMaxLegRHPAttribute());
	}
}

void UPDHUDWidget::BindAttributeToBar(UPDAttributeBarWidget* Bar,
	const FGameplayAttribute& CurrentAttr,
	const FGameplayAttribute& MaxAttr)
{
	if (!Bar || !CachedASC.IsValid()) return;

	RefreshBar(Bar, CurrentAttr, MaxAttr);

	UAbilitySystemComponent* ASC = CachedASC.Get();
	TWeakObjectPtr<UPDAttributeBarWidget> WeakBar = Bar;
	TWeakObjectPtr<UPDHUDWidget> WeakSelf = this;

	auto OnChanged =
		[WeakBar, WeakSelf, CurrentAttr, MaxAttr](const FOnAttributeChangeData& /*Data*/)
		{
			if (!WeakBar.IsValid() || !WeakSelf.IsValid()) return;
			WeakSelf->RefreshBar(WeakBar.Get(), CurrentAttr, MaxAttr);
		};

	BoundAttributeHandles.Add({ CurrentAttr,
		ASC->GetGameplayAttributeValueChangeDelegate(CurrentAttr).AddLambda(OnChanged) });

	BoundAttributeHandles.Add({ MaxAttr,
		ASC->GetGameplayAttributeValueChangeDelegate(MaxAttr).AddLambda(OnChanged) });
}

void UPDHUDWidget::RefreshBar(UPDAttributeBarWidget* Bar,
	const FGameplayAttribute& CurrentAttr,
	const FGameplayAttribute& MaxAttr) const
{
	if (!Bar || !CachedASC.IsValid()) return;

	UAbilitySystemComponent* ASC = CachedASC.Get();
	Bar->SetValues(
		ASC->GetNumericAttribute(CurrentAttr),
		ASC->GetNumericAttribute(MaxAttr));
}

void UPDHUDWidget::UnbindAllAttributes()
{
	if (CachedASC.IsValid())
	{
		UAbilitySystemComponent* ASC = CachedASC.Get();
		for (const FBoundAttributeHandle& BH : BoundAttributeHandles)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(BH.Attribute).Remove(BH.Handle);
		}
	}
	BoundAttributeHandles.Reset();
}

void UPDHUDWidget::BindAllDebuffTags()
{
	BindTagEvent(PDGameplayTags::State_Debuff_Bleeding,
		[this](const FGameplayTag& Tag, int32 NewCount) { HandleBleeding(Tag, NewCount); });

	BindTagEvent(PDGameplayTags::State_Debuff_LegDamaged,
		[this](const FGameplayTag& Tag, int32 NewCount) { HandleLegDamaged(Tag, NewCount); });

	BindTagEvent(PDGameplayTags::State_Debuff_ArmDamaged,
		[this](const FGameplayTag& Tag, int32 NewCount) { HandleArmDamaged(Tag, NewCount); });

	BindTagEvent(PDGameplayTags::State_Debuff_Starving,
		[this](const FGameplayTag& Tag, int32 NewCount) { HandleStarving(Tag, NewCount); });

	BindTagEvent(PDGameplayTags::State_Debuff_Dehydrated,
		[this](const FGameplayTag& Tag, int32 NewCount) { HandleDehydrated(Tag, NewCount); });
}

void UPDHUDWidget::BindTagEvent(const FGameplayTag& Tag,
	TFunction<void(const FGameplayTag&, int32)> OnChanged)
{
	if (!CachedASC.IsValid()) return;
	UAbilitySystemComponent* ASC = CachedASC.Get();

	// 초기 상태 동기화 — 바인딩 시점의 현재 태그 카운트로 핸들러 1회 호출.
	// RegisterGameplayTagEvent(NewOrRemoved)는 전이만 trigger하므로, 이미 1+ 상태인 태그를 놓치지 않기 위해 필요.
	OnChanged(Tag, ASC->GetGameplayTagCount(Tag));

	TWeakObjectPtr<UPDHUDWidget> WeakSelf = this;
	FDelegateHandle Handle = ASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved)
		.AddLambda([WeakSelf, OnChanged](const FGameplayTag CallbackTag, int32 NewCount)
		{
			if (!WeakSelf.IsValid()) return;
			OnChanged(CallbackTag, NewCount);
		});

	BoundTagHandles.Add({ Tag, Handle });
}

void UPDHUDWidget::UnbindAllTags()
{
	if (CachedASC.IsValid())
	{
		UAbilitySystemComponent* ASC = CachedASC.Get();
		for (const FBoundTagHandle& BH : BoundTagHandles)
		{
			ASC->RegisterGameplayTagEvent(BH.Tag, EGameplayTagEventType::NewOrRemoved).Remove(BH.Handle);
		}
	}
	BoundTagHandles.Reset();
}

void UPDHUDWidget::HandleBleeding(const FGameplayTag& Tag, int32 NewCount)
{
	if (Bar_Debuffs)
	{
		Bar_Debuffs->SetDebuffActive(Tag, NewCount > 0);
	}
}

void UPDHUDWidget::HandleLegDamaged(const FGameplayTag& Tag, int32 NewCount)
{
	if (Bar_Debuffs)
	{
		Bar_Debuffs->SetDebuffActive(Tag, NewCount > 0);
	}
}

void UPDHUDWidget::HandleArmDamaged(const FGameplayTag& Tag, int32 NewCount)
{
	if (Bar_Debuffs)
	{
		Bar_Debuffs->SetDebuffActive(Tag, NewCount > 0);
	}
}

void UPDHUDWidget::HandleStarving(const FGameplayTag& Tag, int32 NewCount)
{
	if (Bar_Debuffs)
	{
		Bar_Debuffs->SetDebuffActive(Tag, NewCount > 0);
	}
}

void UPDHUDWidget::HandleDehydrated(const FGameplayTag& Tag, int32 NewCount)
{
	if (Bar_Debuffs)
	{
		Bar_Debuffs->SetDebuffActive(Tag, NewCount > 0);
	}
}

void UPDHUDWidget::UpdateCrosshair(FVector2D MousePos, float Spread)
{
	if (WBP_Crosshair) WBP_Crosshair->UpdateCrosshair(MousePos, Spread);
}

void UPDHUDWidget::SetCrosshairType(EWeaponType NewType)
{
	if (WBP_Crosshair) WBP_Crosshair->SetCrosshairType(NewType);
}

void UPDHUDWidget::SetCrosshairVisible(bool bVisible)
{
	if (!WBP_Crosshair) return;
	WBP_Crosshair->SetVisibility(
		bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UPDHUDWidget::RefreshNewQuickSlots()
{
	if (!Bar_NewQuickSlots)
	{
		return;
	}

	UPDQuickSlotComponent* QuickSlotComponent = nullptr;
	if (APawn* Pawn = GetOwningPlayerPawn())
	{
		QuickSlotComponent = Pawn->FindComponentByClass<UPDQuickSlotComponent>();
	}

	Bar_NewQuickSlots->BindQuickSlotComponent(QuickSlotComponent);
}
