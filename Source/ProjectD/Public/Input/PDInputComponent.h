
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
	//ï¿½Ìµï¿½, ï¿½Ã¾ï¿½ ï¿½ï¿½È¯, GASï¿½ï¿½ ï¿½ï¿½Ä¡ï¿½ï¿½ ï¿½ï°¢ ï¿½ï¿½ï¿½ï¿½Ç´ï¿½ ï¿½âº» ï¿½ï¿½ï¿½Ûµï¿½ ï¿½ï¿½ï¿½â¿¡ ï¿½ï¿½ï¿½Îµï¿½ ï¿½Ï¸ï¿½ ï¿½ï¿½.
	UPDInputComponent();
	//ÀÌµ¿, ½Ã¾ß ÀüÈ¯, GAS¾È °ÅÄ¡°í Áï°¢ ½ÇÇàµÇ´Â ±âº» µ¿ÀÛµé ¿©±â¿¡ ¹ÙÀÎµù ÇÏ¸é µÊ.
	template<class UserClass, typename FuncType>
	void BindNativeAction(const UPDInputConfig* InputConfig, const FGameplayTag& InputTag,
						  ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func)
	{
		if (const UInputAction* IA=InputConfig->FindNativeInputActionForTag(InputTag))
		{
			BindAction(IA, TriggerEvent, Object, Func);
		}
	}
	
	//»ç°Ý, ½ºÅ³, GAS Ability ÇÊ¿äÇÑ µ¿ÀÛµé ¿©±â¼­ È°¿ë.(Started, Completed) ÇÊ¿äÇÏ´Ù¸é release°°Àº°Å Ãß°¡ÇÏ¸é µË´Ï´Ù.
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
