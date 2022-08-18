#pragma once
#include "../core/interfaces.h"

class LegitBot {
public:
	void AimBot(UserCmd* cmd);
	void TriggerBot(UserCmd* cmd);
	void RecoilControl(UserCmd* cmd);
};