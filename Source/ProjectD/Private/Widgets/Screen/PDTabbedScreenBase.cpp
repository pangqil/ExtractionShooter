#include "Widgets/Screen/PDTabbedScreenBase.h"

#include "Core/PDPlayerController.h"
#include "Core/PDPlayerUIManagerComponent.h"
#include "GameplayTag/PDGameplayTags.h"
#include "Widgets/Screen/PDTabButtonWidget.h"
#include "Widgets/Screen/PDTabbedContent.h"
#include "Data/PDTabbedScreenDataAsset.h"
#include "Subsystems/PDFrontendUISubsystem.h"
#include "Components/HorizontalBox.h"
#include "Components/NamedSlot.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Texture2D.h"
#include "Blueprint/UserWidget.h"

UPDTabbedScreenBase* UPDTabbedScreenBase::OpenAtTab(
	UObject* WorldContextObject,
	TSubclassOf<UPDTabbedScreenBase> ScreenClass,
	FGameplayTag TabId,
	EUILayer Layer)
{
	if (!ScreenClass)
	{
		return nullptr;
	}

	UPDFrontendUISubsystem* Subsystem = UPDFrontendUISubsystem::Get(WorldContextObject);
	if (!Subsystem)
	{
		return nullptr;
	}

	UPDActivatableBase* Pushed = Subsystem->PushToLayer(Layer, ScreenClass);
	UPDTabbedScreenBase* Tabbed = Cast<UPDTabbedScreenBase>(Pushed);
	if (Tabbed && TabId.IsValid())
	{
		Tabbed->SetInitialTab(TabId);
	}
	return Tabbed;
}

void UPDTabbedScreenBase::SetInitialTab(FGameplayTag TabId)
{
	PendingInitialTab = TabId;
	if (IsActivated() && TabId.IsValid())
	{
		SwitchToTab(TabId);
	}
}

void UPDTabbedScreenBase::SwitchToTab(FGameplayTag TabId)
{
	if (!TabId.IsValid() || !Switcher_Content)
	{
		return;
	}

	const int32 TargetIndex = FindTabIndexInDataAsset(TabId);
	if (TargetIndex == INDEX_NONE)
	{
		return;
	}

	if (TabSet && TabSet->Tabs.IsValidIndex(TargetIndex) && !TabSet->Tabs[TargetIndex].bEnabled)
	{
		return;
	}

	if (ActiveTabId.IsValid() && ActiveTabId != TabId)
	{
		if (TObjectPtr<UUserWidget>* PrevContent = SpawnedContents.Find(ActiveTabId))
		{
			if (*PrevContent && (*PrevContent)->GetClass()->ImplementsInterface(UPDTabbedContent::StaticClass()))
			{
				if (IPDTabbedContent* PrevIface = Cast<IPDTabbedContent>(*PrevContent))
				{
					PrevIface->OnTabHidden();
				}
			}
		}
	}

	ActiveTabId = TabId;
	LastTabId = TabId;

	TObjectPtr<UUserWidget>* NewContentPtr = SpawnedContents.Find(TabId);
	UUserWidget* NewContent = (NewContentPtr && *NewContentPtr) ? NewContentPtr->Get() : nullptr;
	if (NewContent)
	{
		Switcher_Content->SetActiveWidget(NewContent);

		if (NewContent->GetClass()->ImplementsInterface(UPDTabbedContent::StaticClass()))
		{
			if (IPDTabbedContent* NewIface = Cast<IPDTabbedContent>(NewContent))
			{
				NewIface->OnTabShown();
				if (UWidget* Focus = NewIface->GetTabDesiredFocusTarget())
				{
					Focus->SetUserFocus(GetOwningPlayer());
				}
			}
		}
	}
	else
	{
		// Fail-safe: ContentClass 미등록 탭으로 전환 시 직전 탭 잔상 방지.
		Switcher_Content->SetActiveWidgetIndex(-1);
	}

	ApplyActiveTabVisualState();

	// 풋터 설명 텍스트를 active entry의 Description으로 갱신. TXT_Description 미바인딩 시 무시.
	if (TXT_Description && TabSet && TabSet->Tabs.IsValidIndex(TargetIndex))
	{
		TXT_Description->SetText(TabSet->Tabs[TargetIndex].Description);
	}

	// 좌/우 사이드 패널 swap. DA 주도: panel class가 있는 탭은 해당 위젯으로, 없는 탭은 명시적으로 비움(잔상 방지).
	if (NamedSlot_MainLeft)
	{
		TObjectPtr<UUserWidget>* LeftPanelPtr = SpawnedLeftPanels.Find(TabId);
		UUserWidget* LeftPanel = (LeftPanelPtr && *LeftPanelPtr) ? LeftPanelPtr->Get() : nullptr;
		NamedSlot_MainLeft->SetContent(LeftPanel);
	}
	if (NamedSlot_MainRight)
	{
		TObjectPtr<UUserWidget>* RightPanelPtr = SpawnedRightPanels.Find(TabId);
		UUserWidget* RightPanel = (RightPanelPtr && *RightPanelPtr) ? RightPanelPtr->Get() : nullptr;
		NamedSlot_MainRight->SetContent(RightPanel);
	}
}

void UPDTabbedScreenBase::CycleTab(int32 Direction)
{
	if (!TabSet || TabSet->Tabs.Num() == 0 || Direction == 0)
	{
		return;
	}

	const int32 N = TabSet->Tabs.Num();
	int32 CurrentIdx = FindTabIndexInDataAsset(ActiveTabId);
	if (CurrentIdx == INDEX_NONE)
	{
		CurrentIdx = 0;
	}

	const int32 NormalizedDir = Direction > 0 ? 1 : -1;
	for (int32 Step = 1; Step <= N; ++Step)
	{
		const int32 NextIdx = ((CurrentIdx + NormalizedDir * Step) % N + N) % N;
		const FPDTabbedScreenEntry& Entry = TabSet->Tabs[NextIdx];
		if (Entry.bEnabled && Entry.TabId.IsValid() && Entry.TabId != ActiveTabId)
		{
			SwitchToTab(Entry.TabId);
			return;
		}
	}
}

void UPDTabbedScreenBase::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 키 입력을 직접 받아 Hub 토글을 처리하려면 위젯이 focusable이어야 함. WBP의 Class Defaults가 false여도 강제.
	SetIsFocusable(true);
}

void UPDTabbedScreenBase::NativeOnActivated()
{
	Super::NativeOnActivated();

	RebuildTabsAndContent();

	const FGameplayTag InitialId = ResolveInitialTabId();
	if (InitialId.IsValid())
	{
		SwitchToTab(InitialId);
	}

	PendingInitialTab = FGameplayTag();

	// UIOnly 모드에서 PC InputComponent가 차단되므로, Hub 자신이 키보드 포커스를 받아야 토글 키를 직접 처리할 수 있다.
	if (APlayerController* PC = GetOwningPlayer())
	{
		SetUserFocus(PC);
	}

	UE_LOG(LogTemp, Verbose, TEXT("[Hub] NativeOnActivated: IsFocusable=%d HasKeyboardFocus=%d"),
		IsFocusable() ? 1 : 0, HasKeyboardFocus() ? 1 : 0);
}

bool UPDTabbedScreenBase::TryHandleHubToggleKey(const FKeyEvent& InKeyEvent)
{
	APDPlayerController* PDPC = Cast<APDPlayerController>(GetOwningPlayer());
	if (!PDPC)
	{
		return false;
	}

	const TArray<FKey> ToggleKeys = PDPC->GetMappedKeysForInputTag(PDGameplayTags::Input_Inventory);
	const FKey PressedKey = InKeyEvent.GetKey();
	const bool bMatched = ToggleKeys.Num() > 0
		? ToggleKeys.Contains(PressedKey)
		: (PressedKey == EKeys::Tab);

	if (!bMatched)
	{
		return false;
	}

	if (UPDPlayerUIManagerComponent* UI = PDPC->GetUIManagerComponent())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Hub] Toggle key handled: %s"), *PressedKey.ToString());
		UI->ToggleHub();
		return true;
	}
	return false;
}

FReply UPDTabbedScreenBase::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (TryHandleHubToggleKey(InKeyEvent))
	{
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UPDTabbedScreenBase::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// PreviewKeyDown은 capture phase — 자식 위젯(TabButton 등)이 Tab을 Navigation으로 가로채기 전에 먼저 받음.
	if (TryHandleHubToggleKey(InKeyEvent))
	{
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

void UPDTabbedScreenBase::NativeOnDeactivated()
{
	if (ActiveTabId.IsValid())
	{
		if (TObjectPtr<UUserWidget>* ActiveContent = SpawnedContents.Find(ActiveTabId))
		{
			if (*ActiveContent && (*ActiveContent)->GetClass()->ImplementsInterface(UPDTabbedContent::StaticClass()))
			{
				if (IPDTabbedContent* Iface = Cast<IPDTabbedContent>(*ActiveContent))
				{
					Iface->OnTabHidden();
				}
			}
		}
	}

	ClearTabsAndContent();
	ActiveTabId = FGameplayTag();
	Super::NativeOnDeactivated();
}

void UPDTabbedScreenBase::HandleTabButtonClicked(FGameplayTag ClickedTabId)
{
	SwitchToTab(ClickedTabId);
}

void UPDTabbedScreenBase::RebuildTabsAndContent()
{
	ClearTabsAndContent();

	if (!TabSet || !HBox_TabBar || !Switcher_Content)
	{
		return;
	}

	APlayerController* OwnerPC = GetOwningPlayer();

	for (const FPDTabbedScreenEntry& Entry : TabSet->Tabs)
	{
		if (!Entry.TabId.IsValid())
		{
			continue;
		}

		if (TabButtonClass)
		{
			UPDTabButtonWidget* TabButton = CreateWidget<UPDTabButtonWidget>(OwnerPC, TabButtonClass);
			if (TabButton)
			{
				UTexture2D* IconTex = Entry.Icon.IsNull() ? nullptr : Entry.Icon.LoadSynchronous();
				TabButton->InitializeTabButton(Entry.TabId, Entry.Label, IconTex, Entry.bEnabled);
				TabButton->OnTabButtonClicked.AddDynamic(this, &UPDTabbedScreenBase::HandleTabButtonClicked);
				HBox_TabBar->AddChild(TabButton);
				SpawnedButtons.Add(Entry.TabId, TabButton);
			}
		}

		UClass* ContentClassLoaded = Entry.ContentClass.IsNull() ? nullptr : Entry.ContentClass.LoadSynchronous();
		if (!ContentClassLoaded)
		{
			continue;
		}

		UUserWidget* ContentWidget = CreateWidget<UUserWidget>(OwnerPC, ContentClassLoaded);
		if (!ContentWidget)
		{
			continue;
		}

		Switcher_Content->AddChild(ContentWidget);
		SpawnedContents.Add(Entry.TabId, ContentWidget);

		if (ContentWidget->GetClass()->ImplementsInterface(UPDTabbedContent::StaticClass()))
		{
			if (IPDTabbedContent* Iface = Cast<IPDTabbedContent>(ContentWidget))
			{
				Iface->InitializeForOwner(OwnerPC);
				Iface->OnEmbeddedInHub();
			}
		}

		// 좌/우 사이드 패널 spawn + 캐시. SwitchToTab에서 NamedSlot에 SetContent.
		// IPDTabbedContent 구현 시 InitializeForOwner / OnEmbeddedInHub만 호출 (라이프사이클은 메인 컨텐츠 전용).
		auto SpawnSidePanel = [this, OwnerPC](const TSoftClassPtr<UUserWidget>& PanelClassPtr,
			const FGameplayTag& EntryTabId,
			TMap<FGameplayTag, TObjectPtr<UUserWidget>>& OutCache)
		{
			if (PanelClassPtr.IsNull())
			{
				return;
			}
			UClass* PanelClass = PanelClassPtr.LoadSynchronous();
			if (!PanelClass)
			{
				return;
			}
			UUserWidget* PanelWidget = CreateWidget<UUserWidget>(OwnerPC, PanelClass);
			if (!PanelWidget)
			{
				return;
			}
			OutCache.Add(EntryTabId, PanelWidget);

			if (PanelWidget->GetClass()->ImplementsInterface(UPDTabbedContent::StaticClass()))
			{
				if (IPDTabbedContent* PanelIface = Cast<IPDTabbedContent>(PanelWidget))
				{
					PanelIface->InitializeForOwner(OwnerPC);
					PanelIface->OnEmbeddedInHub();
				}
			}
		};

		SpawnSidePanel(Entry.LeftPanelClass, Entry.TabId, SpawnedLeftPanels);
		SpawnSidePanel(Entry.RightPanelClass, Entry.TabId, SpawnedRightPanels);
	}
}

void UPDTabbedScreenBase::ClearTabsAndContent()
{
	SpawnedContents.Reset();
	SpawnedLeftPanels.Reset();
	SpawnedRightPanels.Reset();
	SpawnedButtons.Reset();
	if (HBox_TabBar)
	{
		HBox_TabBar->ClearChildren();
	}
	if (Switcher_Content)
	{
		Switcher_Content->ClearChildren();
	}
	if (NamedSlot_MainLeft)
	{
		NamedSlot_MainLeft->SetContent(nullptr);
	}
	if (NamedSlot_MainRight)
	{
		NamedSlot_MainRight->SetContent(nullptr);
	}
}

int32 UPDTabbedScreenBase::FindTabIndexInDataAsset(FGameplayTag TabId) const
{
	if (!TabSet)
	{
		return INDEX_NONE;
	}
	for (int32 Index = 0; Index < TabSet->Tabs.Num(); ++Index)
	{
		if (TabSet->Tabs[Index].TabId == TabId)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

void UPDTabbedScreenBase::ApplyActiveTabVisualState()
{
	for (const TPair<FGameplayTag, TObjectPtr<UPDTabButtonWidget>>& Pair : SpawnedButtons)
	{
		if (Pair.Value)
		{
			Pair.Value->SetSelected(Pair.Key == ActiveTabId);
		}
	}
}

FGameplayTag UPDTabbedScreenBase::ResolveInitialTabId() const
{
	if (PendingInitialTab.IsValid())
	{
		return PendingInitialTab;
	}
	if (TabSet && TabSet->bRememberLastTab && LastTabId.IsValid())
	{
		return LastTabId;
	}
	if (TabSet && TabSet->DefaultTabId.IsValid())
	{
		return TabSet->DefaultTabId;
	}
	if (TabSet)
	{
		for (const FPDTabbedScreenEntry& Entry : TabSet->Tabs)
		{
			if (Entry.bEnabled && Entry.TabId.IsValid())
			{
				return Entry.TabId;
			}
		}
	}
	return FGameplayTag();
}