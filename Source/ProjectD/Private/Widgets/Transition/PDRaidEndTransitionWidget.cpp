// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/Transition/PDRaidEndTransitionWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Core/PDPlayerController.h"
#include "Core/PDPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Widgets/Transition/PDPlayerRaidEntryWidget.h"

void UPDRaidEndTransitionWidget::Configure(bool bInSuccess,
                                           const TArray<FPDPlayerRaidEntryData>& Entries,
                                           float RaidDurationSeconds)
{
	bSuccess = bInSuccess;

	if (Text_Main)
	{
		Text_Main->SetText(bSuccess ? SuccessMainText : FailureMainText);
		Text_Main->SetColorAndOpacity(FSlateColor(bSuccess ? SuccessAccentColor : FailureAccentColor));
	}
	if (Text_Subtitle)
	{
		Text_Subtitle->SetText(bSuccess ? SuccessSubtitleText : FailureSubtitleText);
	}

	if (Text_RaidDuration)
	{
		const int32 TotalSeconds = FMath::Max(0, FMath::FloorToInt(RaidDurationSeconds));
		const int32 Minutes = TotalSeconds / 60;
		const int32 Seconds = TotalSeconds % 60;
		Text_RaidDuration->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds)));
	}

	// Entries 결정: 외부 전달이 비어 있으면 GameState->PlayerArray 로 fallback.
	TArray<FPDPlayerRaidEntryData> ResolvedEntries = Entries;
	if (ResolvedEntries.Num() == 0)
	{
		BuildFallbackEntries(ResolvedEntries);
	}

	// Step 4: C++ 가 직접 EntryWidgetClass 로 인스턴싱 + Configure. BP 위임(K2_PopulateEntries) 폐기.
	if (Box_PlayerEntries)
	{
		Box_PlayerEntries->ClearChildren();

		if (!EntryWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("PDRaidEndTransitionWidget: EntryWidgetClass 미할당 — 결산 라인 미생성."));
		}
		else
		{
			APlayerController* OwningPC = GetOwningPlayer();
			for (int32 Index = 0; Index < ResolvedEntries.Num(); ++Index)
			{
				UPDPlayerRaidEntryWidget* Entry = CreateWidget<UPDPlayerRaidEntryWidget>(OwningPC, EntryWidgetClass);
				if (!Entry) continue;

				Box_PlayerEntries->AddChildToVerticalBox(Entry);
				const float StaggerDelay = EntryRevealInitialDelay + Index * EntryStaggerInterval;
				Entry->Configure(ResolvedEntries[Index], StaggerDelay);
			}
		}
	}

	K2_ApplyAccent(bSuccess ? SuccessAccentColor : FailureAccentColor);
}

void UPDRaidEndTransitionWidget::BuildFallbackEntries(TArray<FPDPlayerRaidEntryData>& OutEntries) const
{
	const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GS) return;

	OutEntries.Reserve(GS->PlayerArray.Num());
	for (APlayerState* PS : GS->PlayerArray)
	{
		if (!PS) continue;

		FPDPlayerRaidEntryData Data;
		Data.PlayerName = PS->GetPlayerName();

		// Step 2-C: PDPlayerState 의 RaidStats / IsExtracted 를 실값으로 결합.
		if (const APDPlayerState* PDPS = Cast<APDPlayerState>(PS))
		{
			Data.bSurvived = PDPS->IsExtracted();
			Data.Stats     = PDPS->GetRaidStats();
		}
		else
		{
			Data.bSurvived = false;
			Data.Stats     = FPDRaidStats{};
		}

		OutEntries.Add(MoveTemp(Data));
	}
}

void UPDRaidEndTransitionWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	bReadyForInput = false;
	bInputConsumed = false;

	if (Text_ContinuePrompt)
	{
		Text_ContinuePrompt->SetVisibility(ESlateVisibility::Hidden);
	}

	if (Anim_IntroSequence)
	{
		PlayAnimation(Anim_IntroSequence);
	}
	else
	{
		OnIntroFinished();
	}
}

void UPDRaidEndTransitionWidget::NativeOnDeactivated()
{
	if (Anim_IntroSequence) StopAnimation(Anim_IntroSequence);
	if (Anim_BlackFade)     StopAnimation(Anim_BlackFade);

	Super::NativeOnDeactivated();
}

FReply UPDRaidEndTransitionWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bReadyForInput || bInputConsumed)
	{
		return FReply::Unhandled();
	}
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}

	bInputConsumed = true;

	if (Anim_BlackFade)
	{
		PlayAnimation(Anim_BlackFade);
	}
	else
	{
		HandleTravelTrigger();
	}

	return FReply::Handled();
}

void UPDRaidEndTransitionWidget::OnIntroFinished()
{
	bReadyForInput = true;

	if (Text_ContinuePrompt)
	{
		Text_ContinuePrompt->SetVisibility(ESlateVisibility::Visible);
	}
}

void UPDRaidEndTransitionWidget::HandleTravelTrigger()
{
	if (APDPlayerController* PDPC = Cast<APDPlayerController>(GetOwningPlayer()))
	{
		PDPC->Server_RequestBaseTravel();
	}
}