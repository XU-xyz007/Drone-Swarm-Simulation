#pragma once

#include "FormationTypes.h"

namespace SwarmFormation {

// Generates exactly request.count deterministic target slots.
// Non-empty formation_text takes precedence over formation_id.
FormationResult GenerateSlots(const FormationRequest& request);

// Safely retrieves one slot from a successful result.
bool GetSlot(const FormationResult& result, unsigned int index, Point2D& output);

const char* FormationErrorMessage(FormationError error);

} // namespace SwarmFormation
