#include "Data/PDBodyPartConfig.h"

EBodyPart UPDBodyPartConfig::GetBodyPartFromName(FName InName) const
{
	if (InName.IsNone()) return EBodyPart::None;

	for (const FBodyPartMapping& Mapping:Mappings)
	{
		if (Mapping.HitBoxName==InName)
		{
			return Mapping.BodyPart;
		}

		for (const FName& HitBoxName:Mapping.HitBoxNames)
		{
			if (HitBoxName==InName)
			{
				return Mapping.BodyPart;
			}
		}
	}
	return EBodyPart::None;
}
