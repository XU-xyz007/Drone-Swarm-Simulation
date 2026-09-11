#pragma once

#include <functional>
#include "../SharedMessage/gen_code/DroneMessage.h"

namespace SwarmComm {

extern const char* const kCommandTopic;
extern const char* const kStateTopic;
extern const int kDomainId;

using CommandCallback = std::function<void(const SharedMessage::DroneCmd&)>;
using StateCallback = std::function<void(const SharedMessage::DroneState&)>;

bool InitDDS();
bool PublishCmd(const SharedMessage::DroneCmd& cmd);
bool PublishState(const SharedMessage::DroneState& state);
bool SubscribeCmd(CommandCallback onCommand);
bool SubscribeState(StateCallback onState);

// CDR payload estimate for application-level telemetry. DDS/UDP/IP headers
// and discovery traffic are intentionally excluded.
unsigned long EstimateStatePayloadBytes(const SharedMessage::DroneState& state);
void ShutdownDDS();

} // namespace SwarmComm
