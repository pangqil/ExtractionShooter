// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/HUD/PDHUDWidget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet/PDAttributeSet.h"
#include "GameFramework/Pawn.h"
#include "Widgets/HUD/PDAttributeBarWidget.h"

void UPDHUDWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	TryBindToOwningPawnASC();
}

void UPDHUDWidget::NativeOnDeactivated()
{
	UnbindAll();
	Super::NativeOnDeactivated();
}

void UPDHUDWidget::TryBindToOwningPawnASC()
{
	APawn* Pawn = GetOwningPlayerPawn();
	if (!Pawn) return;

	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!ASC) return;

	CachedASC = ASC;

	BindAttributeToBar(Bar_Health,
		UPDAttributeSet::GetTorsoHPAttribute(),
		UPDAttributeSet::GetMaxTorsoHPAttribute());

	BindAttributeToBar(Bar_Stamina,
		UPDAttributeSet::GetStaminaAttribute(),
		UPDAttributeSet::GetMaxStaminaAttribute());

	BindAttributeToBar(Bar_Hunger,
		UPDAttributeSet::GetHungerAttribute(),
		UPDAttributeSet::GetMaxHungerAttribute());

	BindAttributeToBar(Bar_Thirst,
		UPDAttributeSet::GetThirstAttribute(),
		UPDAttributeSet::GetMaxThirstAttribute());
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

	BoundHandles.Add({ CurrentAttr,
		ASC->GetGameplayAttributeValueChangeDelegate(CurrentAttr).AddLambda(OnChanged) });

	BoundHandles.Add({ MaxAttr,
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

void UPDHUDWidget::UnbindAll()
{
	if (CachedASC.IsValid())
	{
		UAbilitySystemComponent* ASC = CachedASC.Get();
		for (const FBoundHandle& BH : BoundHandles)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(BH.Attribute).Remove(BH.Handle);
		}
	}
	BoundHandles.Reset();
	CachedASC.Reset();
}
