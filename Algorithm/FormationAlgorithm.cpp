#include "include/FormationAlgorithm.h"

#include <cmath>
#include <deque>
#include <mutex>
#include <utility>

#include "Math/GeometryUtils.h"
#include "Formations/PresetFormations.h"
#include "Formations/TextFormation.h"

namespace SwarmFormation {
namespace {

constexpr unsigned int kMaximumDroneCount = 4096;
constexpr std::size_t kMaximumTextBytes = 255;
constexpr std::size_t kMaximumTextCharacters = 16;
constexpr std::size_t kMaximumCachedTextFormations = 16;

struct CachedTextFormation {
    FormationRequest request;
    FormationResult result;
};

bool SameTextFormationRequest(
    const FormationRequest& left,
    const FormationRequest& right) {
    return left.formation_id == right.formation_id &&
           left.formation_text == right.formation_text &&
           left.count == right.count &&
           left.center_x == right.center_x &&
           left.center_y == right.center_y &&
           left.spacing == right.spacing;
}

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

FormationResult GenerateCachedTextSlots(const FormationRequest& request) {
    // The desktop simulator hosts many virtual drones in one process. They
    // all receive the same deterministic text command, so regenerate the
    // expensive raster/skeleton result once and reuse it locally. DDS traffic
    // and the decentralized ID-to-slot mapping remain unchanged.
    static std::mutex cacheMutex;
    static std::deque<CachedTextFormation> cache;

    std::lock_guard<std::mutex> lock(cacheMutex);
    for (const CachedTextFormation& entry : cache) {
        if (SameTextFormationRequest(entry.request, request)) {
            return entry.result;
        }
    }

    const FormationResult generated = detail::GenerateTextFormation(request);
    if (!generated.success) {
        // Do not cache a transient Windows font/GDI failure: the next command
        // must be allowed to retry normally.
        return generated;
    }

    FormationResult validated = ValidateGenerated(request, generated.slots);
    if (!validated.success) {
        return validated;
    }

    if (cache.size() >= kMaximumCachedTextFormations) {
        cache.pop_front();
    }
    cache.push_back({request, validated});
    return validated;
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
        return GenerateCachedTextSlots(request);
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
