#include "Core/PDGameInstance.h"

#include "Blueprint/UserWidget.h"
#include "Subsystems/PDFrontendUISubsystem.h"
#include "Widgets/PDPrimaryLayout.h"

void UPDGameInstance::Init()
{
	Super::Init();

	checkf(!PrimaryLayoutClass.IsNull(),
		TEXT("PrimaryLayoutClass가 BP_PDGameInstance에 지정되지 않았습니다."));

	UClass* LoadedLayoutClass = PrimaryLayoutClass.LoadSynchronous();
	check(LoadedLayoutClass);

	PrimaryLayoutWidget = CreateWidget<UPDPrimaryLayout>(this, LoadedLayoutClass);
	check(PrimaryLayoutWidget);

	PrimaryLayoutWidget->AddToViewport(0);

	if (UPDFrontendUISubsystem* Subsystem = GetSubsystem<UPDFrontendUISubsystem>())
	{
		Subsystem->RegisterCreatePrimaryLayoutWidget(PrimaryLayoutWidget);
	}
}

void UPDGameInstance::Shutdown()
{
	if (UPDFrontendUISubsystem* Subsystem = GetSubsystem<UPDFrontendUISubsystem>())
	{
		Subsystem->UnregisterPrimaryLayout();
	}

	if (PrimaryLayoutWidget)
	{
		PrimaryLayoutWidget->RemoveFromParent();
		PrimaryLayoutWidget = nullptr;
	}

	Super::Shutdown();
}

void UPDGameInstance::SavePlayerData(const FPDPlayerData& InData)
{
	PlayerData=InData;
}

FPDPlayerData UPDGameInstance::LoadPlayerData() const
{
	return PlayerData;
}