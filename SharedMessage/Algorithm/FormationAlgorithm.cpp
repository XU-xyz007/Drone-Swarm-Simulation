#include "FormationAlgorithm.h"

#include <cmath>
#include <utility>

#include "GeometryUtils.h"
#include "PresetFormations.h"
#include "TextFormation.h"

namespace SwarmFormation {
namespace {

constexpr unsigned int kMaximumDroneCount = 4096;
constexpr std::size_t kMaximumTextBytes = 255;
constexpr std::size_t kMaximumTextCharacters = 16;

std::size_t Utf8CharacterCount(const std::string& text) {
    std::size_t count = 0;
    for (const unsigned char byte : text) {
        if ((byte & 0xC0u) != 0x80u) {
            ++count;
        }
    }
    return count;
}

FormationResult Failure(FormationError error) {
    return {false, {}, error};
}

FormationResult ValidateGenerated(
    const FormationRequest& request,
    std::vector<Point2D> slots) {
    if (slots.size() != request.count || detail::HasDuplicateSlots(slots)) {
        return Failure(FormationError::InvalidGeneratedSlot);
    }
    for (const Point2D& point : slots) {
        if (!detail::IsFinitePoint(point)) {
            return Failure(FormationError::InvalidGeneratedSlot);
        }
    }
    return {true, std::move(slots), FormationError::None};
}

} // namespace

FormationResult GenerateSlots(const FormationRequest& request) {
    if (request.count == 0) {
        return Failure(FormationError::InvalidCount);
    }
    if (request.count > kMaximumDroneCount) {
        return Failure(FormationError::CountTooLarge);
    }
    if (!(request.spacing > 0.0) || !std::isfinite(request.spacing)) {
        return Failure(FormationError::InvalidSpacing);
    }
    if (!std::isfinite(request.center_x) || !std::isfinite(request.center_y)) {
        return Failure(FormationError::InvalidGeneratedSlot);
    }

    if (!request.formation_text.empty()) {
        if (request.formation_text.size() > kMaximumTextBytes ||
            Utf8CharacterCount(request.formation_text) > kMaximumTextCharacters) {
            return Failure(FormationError::TextTooLong);
        }
        const FormationResult text_result = detail::GenerateTextFormation(request);
        if (!text_result.success) {
            return text_result;
        }
        return ValidateGenerated(request, text_result.slots);
    }

    std::vector<Point2D> slots;
    switch (static_cast<FormationType>(request.formation_id)) {
    case FormationType::Square:
        slots = detail::GenerateSquareSlots(request);
        break;
    case FormationType::Line:
        slots = detail::GenerateLineSlots(request);
        break;
    case FormationType::Triangle:
        slots = detail::GenerateTriangleSlots(request);
        break;
    case FormationType::Circle:
        slots = detail::GenerateCircleSlots(request);
        break;
    default:
        return Failure(FormationError::UnsupportedFormation);
    }
    return ValidateGenerated(request, std::move(slots));
}

bool GetSlot(const FormationResult& result, unsigned int index, Point2D& output) {
    if (!result.success || index >= result.slots.size()) {
        return false;
    }
    output = result.slots[index];
    return true;
}

const char* FormationErrorMessage(FormationError error) {
    switch (error) {
    case FormationError::None:
        return "no error";
    case FormationError::InvalidCount:
        return "count must be greater than zero";
    case FormationError::CountTooLarge:
        return "count exceeds the supported maximum of 4096";
    case FormationError::InvalidSpacing:
        return "spacing must be finite and greater than zero";
    case FormationError::InvalidIndex:
        return "slot index is out of range";
    case FormationError::UnsupportedFormation:
        return "formation_id is not supported";
    case FormationError::InvalidUtf8Text:
        return "formation_text is not valid UTF-8";
    case FormationError::TextTooLong:
        return "formation_text exceeds 16 characters or 255 UTF-8 bytes";
    case FormationError::TextRasterizationFailed:
        return "Windows text rasterization failed";
    case FormationError::InsufficientTextPoints:
        return "text did not produce enough visible points";
    case FormationError::InvalidGeneratedSlot:
        return "generated slots contain invalid or duplicate coordinates";
    default:
        return "unknown formation error";
    }
}

} // namespace SwarmFormation
