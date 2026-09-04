#pragma once

#include "CoreMinimal.h"

#include "LDBeatTypes.generated.h"

/** The teaching role a level-design beat plays in a playable sequence. */
UENUM(BlueprintType)
enum class ELDBeatType : uint8
{
	Teach UMETA(DisplayName = "TEACH"),
	Test UMETA(DisplayName = "TEST"),
	Twist UMETA(DisplayName = "TWIST"),
	Payoff UMETA(DisplayName = "PAYOFF"),
	Debug UMETA(DisplayName = "DEBUG")
};
