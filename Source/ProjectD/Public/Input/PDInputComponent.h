
#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "Input/PDInputConfig.h"
#include "PDInputComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTD_API UPDInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	UPDInputComponent();
	//�̵�, �þ� ��ȯ, GAS�� ��ġ�� �ﰢ ����Ǵ� �⺻ ���۵� ���⿡ ���ε� �ϸ� ��.
	template<class UserClass, typename FuncType>
	void BindNativeAction(const UPDInputConfig* InputConfig, const FGameplayTag& InputTag,
						  ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func)
	{
		if (const UInputAction* IA=InputConfig->FindNativeInputActionForTag(InputTag))
		{
			BindAction(IA, TriggerEvent, Object, Func);
		}
	}
	
	//���, ��ų, GAS Ability �ʿ��� ���۵� ���⼭ Ȱ��.(Started, Completed) �ʿ��ϴٸ� release������ �߰��ϸ� �˴ϴ�.
	template<class UserClass, typename PressedFuncType, typename ReleasedFuncType>
	void BindAbilityActions(const UPDInputConfig* InputConfig, UserClass* Object,
							PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc)
	{
		for (const FPDInputAction& Entry:InputConfig->AbilityInputActions)
		{
			if (!Entry.InputAction||!Entry.InputTag.IsValid()) continue;

			BindAction(Entry.InputAction, ETriggerEvent::Started, Object, PressedFunc, Entry.InputTag);
			BindAction(Entry.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Entry.InputTag);
		}
	}
};
