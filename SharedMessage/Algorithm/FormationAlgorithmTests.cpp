#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "../FormationAlgorithm.h"

namespace {

using SwarmFormation::FormationError;
using SwarmFormation::FormationRequest;
using SwarmFormation::FormationResult;
using SwarmFormation::Point2D;

constexpr double kTolerance = 1e-8;
int g_failures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "[FAIL] " << message << '\n';
    }
}

double Distance(const Point2D& lhs, const Point2D& rhs) {
    const double dx = lhs.x - rhs.x;
    const double dy = lhs.y - rhs.y;
    return std::sqrt(dx * dx + dy * dy);
}

double MinimumDistance(const std::vector<Point2D>& slots) {
    if (slots.size() < 2) {
        return 0.0;
    }
    double minimum = std::numeric_limits<double>::max();
    for (std::size_t i = 0; i < slots.size(); ++i) {
        for (std::size_t j = i + 1; j < slots.size(); ++j) {
            minimum = std::min(minimum, Distance(slots[i], slots[j]));
        }
    }
    return minimum;
}

struct NearestNeighborStats {
    double mean = 0.0;
    double minimum = 0.0;
    double maximum = 0.0;
};

NearestNeighborStats MeasureNearestNeighbors(const std::vector<Point2D>& slots) {
    if (slots.size() < 2) {
        return {};
    }
    std::vector<double> nearest(slots.size(), std::numeric_limits<double>::max());
    for (std::size_t i = 0; i < slots.size(); ++i) {
        for (std::size_t j = i + 1; j < slots.size(); ++j) {
            const double distance = Distance(slots[i], slots[j]);
            nearest[i] = std::min(nearest[i], distance);
            nearest[j] = std::min(nearest[j], distance);
        }
    }
    double sum = 0.0;
    for (const double distance : nearest) {
        sum += distance;
    }
    return {
        sum / nearest.size(),
        *std::min_element(nearest.begin(), nearest.end()),
        *std::max_element(nearest.begin(), nearest.end())
    };
}

void CheckValidResult(const FormationResult& result, unsigned int count, const std::string& label) {
    Check(result.success, label + " should succeed");
    if (!result.success) {
        return;
    }
    Check(result.error == FormationError::None, label + " should report no error");
    Check(result.slots.size() == count, label + " should return exactly N slots");
    for (const Point2D& point : result.slots) {
        Check(std::isfinite(point.x) && std::isfinite(point.y), label + " coordinates must be finite");
    }
    for (std::size_t i = 0; i < result.slots.size(); ++i) {
        for (std::size_t j = i + 1; j < result.slots.size(); ++j) {
            Check(Distance(result.slots[i], result.slots[j]) > kTolerance,
                  label + " must not contain duplicate slots");
        }
    }
}

FormationRequest PresetRequest(unsigned int formation_id, unsigned int count) {
    FormationRequest request;
    request.formation_id = formation_id;
    request.count = count;
    request.center_x = 10.0;
    request.center_y = -4.0;
    request.spacing = 2.5;
    return request;
}

void TestPresetFormations() {
    const unsigned int counts[] = {1, 2, 3, 5, 8, 20};
    for (unsigned int formation_id = 0; formation_id <= 3; ++formation_id) {
        for (unsigned int count : counts) {
            const std::string label = "preset " + std::to_string(formation_id) +
                                      " N=" + std::to_string(count);
            const FormationRequest request = PresetRequest(formation_id, count);
            const FormationResult first = SwarmFormation::GenerateSlots(request);
            const FormationResult second = SwarmFormation::GenerateSlots(request);
            CheckValidResult(first, count, label);
            Check(first.slots.size() == second.slots.size(), label + " should be deterministic");
            for (std::size_t i = 0; i < first.slots.size() && i < second.slots.size(); ++i) {
                Check(std::fabs(first.slots[i].x - second.slots[i].x) < kTolerance &&
                      std::fabs(first.slots[i].y - second.slots[i].y) < kTolerance,
                      label + " deterministic coordinates mismatch");
            }
            if (count > 1 && first.success) {
                Check(MinimumDistance(first.slots) + kTolerance >= request.spacing,
                      label + " minimum distance should be at least spacing");
            }
        }
    }
}

void TestCircleSingleDrone() {
    const FormationResult result = SwarmFormation::GenerateSlots(PresetRequest(3, 1));
    CheckValidResult(result, 1, "circle N=1");
    if (result.success) {
        Check(std::fabs(result.slots[0].x - 10.0) < kTolerance, "circle N=1 x should equal center");
        Check(std::fabs(result.slots[0].y + 4.0) < kTolerance, "circle N=1 y should equal center");
    }
}

void TestSquareLastRowStartsLeft() {
    FormationRequest request = PresetRequest(0, 5);
    request.center_x = 0.0;
    request.center_y = 0.0;
    request.spacing = 1.0;
    const FormationResult result = SwarmFormation::GenerateSlots(request);
    CheckValidResult(result, 5, "square N=5");
    if (result.success) {
        std::vector<Point2D> lower_row;
        const double minimum_y = std::min_element(
            result.slots.begin(), result.slots.end(),
            [](const Point2D& lhs, const Point2D& rhs) { return lhs.y < rhs.y; })->y;
        for (const Point2D& point : result.slots) {
            if (std::fabs(point.y - minimum_y) < kTolerance) {
                lower_row.push_back(point);
            }
        }
        std::sort(lower_row.begin(), lower_row.end(),
                  [](const Point2D& lhs, const Point2D& rhs) { return lhs.x < rhs.x; });
        Check(lower_row.size() == 2, "square N=5 last row should contain two slots");
        if (lower_row.size() == 2) {
            Check(std::fabs(lower_row[0].x + 1.0) < kTolerance,
                  "square N=5 last row should begin at the left column");
        }
    }
}

void TestTextFormations() {
    const std::string texts[] = {u8"S", u8"AI", u8"DDS", u8"中", u8"爱", u8"无人机"};
    const unsigned int counts[] = {1, 5, 8, 20, 50};
    for (const std::string& text : texts) {
        for (unsigned int count : counts) {
            FormationRequest request = PresetRequest(0, count);
            request.formation_text = text;
            const std::string label = "text N=" + std::to_string(count);
            const FormationResult first = SwarmFormation::GenerateSlots(request);
            const FormationResult second = SwarmFormation::GenerateSlots(request);
            CheckValidResult(first, count, label);
            Check(first.slots.size() == second.slots.size(), label + " should be deterministic");
            for (std::size_t i = 0; i < first.slots.size() && i < second.slots.size(); ++i) {
                Check(std::fabs(first.slots[i].x - second.slots[i].x) < kTolerance &&
                      std::fabs(first.slots[i].y - second.slots[i].y) < kTolerance,
                      label + " deterministic coordinates mismatch");
            }
            if (count > 1 && first.success) {
                Check(std::fabs(MinimumDistance(first.slots) - request.spacing) < 1e-6,
                      label + " minimum distance should equal spacing");
            }
        }
    }
}

void TestTextTakesPrecedence() {
    FormationRequest request = PresetRequest(999, 8);
    request.formation_text = u8"S";
    const FormationResult result = SwarmFormation::GenerateSlots(request);
    CheckValidResult(result, 8, "text precedence");
}

void TestHersheyChineseFormations() {
    struct Case {
        const char* text;
        unsigned int count;
    };
    const Case cases[] = {
        {u8"中", 60},
        {u8"爱你", 200},
        {u8"无人机", 120}
    };
    for (const Case& test_case : cases) {
        FormationRequest request = PresetRequest(0, test_case.count);
        request.formation_text = test_case.text;
        const FormationResult result = SwarmFormation::GenerateSlots(request);
        CheckValidResult(result, test_case.count,
                         "Hershey Chinese N=" + std::to_string(test_case.count));
        if (result.success && result.slots.size() > 1) {
            const NearestNeighborStats stats =
                MeasureNearestNeighbors(result.slots);
            Check(stats.mean <= request.spacing * 1.8,
                  "dense Chinese nearest-neighbor mean is too large");
            Check(stats.maximum / stats.minimum <= 2.25,
                  "dense Chinese nearest-neighbor max/min ratio is too large");
        }
    }
}

void TestBuiltInLatinAlphabet() {
    const char* texts[] = {
        "ABCDEFGHIJKLM",
        "NOPQRSTUVWXYZ",
        "abcdefghijklm",
        "nopqrstuvwxyz"
    };
    for (const char* text : texts) {
        FormationRequest request = PresetRequest(0, 260);
        request.formation_text = text;
        const FormationResult result = SwarmFormation::GenerateSlots(request);
        CheckValidResult(result, request.count,
                         std::string("built-in Latin ") + text);
        if (result.success) {
            const NearestNeighborStats stats =
                MeasureNearestNeighbors(result.slots);
            Check(stats.maximum / stats.minimum <= 2.5,
                  "built-in Latin nearest-neighbor ratio is too large");
        }
    }
}

void TestChineseDensityAcrossFleetSizes() {
    const unsigned int counts[] = {80, 120, 180, 260};
    for (const unsigned int count : counts) {
        FormationRequest request = PresetRequest(0, count);
        request.formation_text = u8"无人机";
        const FormationResult result = SwarmFormation::GenerateSlots(request);
        CheckValidResult(result, count,
                         "Chinese density N=" + std::to_string(count));
        if (!result.success || result.slots.size() < 2) {
            continue;
        }
        const NearestNeighborStats stats = MeasureNearestNeighbors(result.slots);
        const double ratio = stats.maximum / stats.minimum;
        // Corner locking trades a small amount of nearest-neighbor variance
        // for recognisable bends, but must remain materially tighter than the
        // former 2.25 limit.
        Check(ratio <= 2.1,
              "Chinese nearest-neighbor spread regressed across fleet sizes N=" +
              std::to_string(count) + " ratio=" + std::to_string(ratio));
    }
}

void TestInvalidInputs() {
    FormationRequest request = PresetRequest(0, 5);
    request.count = 0;
    Check(SwarmFormation::GenerateSlots(request).error == FormationError::InvalidCount,
          "count=0 should fail");

    request = PresetRequest(0, 5);
    request.spacing = 0.0;
    Check(SwarmFormation::GenerateSlots(request).error == FormationError::InvalidSpacing,
          "spacing=0 should fail");

    request = PresetRequest(99, 5);
    Check(SwarmFormation::GenerateSlots(request).error == FormationError::UnsupportedFormation,
          "unsupported formation should fail");

    request = PresetRequest(0, 4097);
    Check(SwarmFormation::GenerateSlots(request).error == FormationError::CountTooLarge,
          "count above 4096 should fail explicitly");

    request = PresetRequest(0, 20);
    request.formation_text = std::string(256, 'A');
    Check(SwarmFormation::GenerateSlots(request).error == FormationError::TextTooLong,
          "text above DDS byte limit should fail explicitly");

    request = PresetRequest(0, 20);
    request.formation_text = std::string(17, 'A');
    Check(SwarmFormation::GenerateSlots(request).error == FormationError::TextTooLong,
          "text above 16 characters should fail explicitly");

    request = PresetRequest(0, 5);
    request.formation_text = std::string("\xFF", 1);
    Check(SwarmFormation::GenerateSlots(request).error == FormationError::InvalidUtf8Text,
          "invalid UTF-8 should fail");
}

void TestGetSlot() {
    const FormationResult result = SwarmFormation::GenerateSlots(PresetRequest(1, 3));
    Point2D point;
    Check(SwarmFormation::GetSlot(result, 1, point), "GetSlot should retrieve valid index");
    Check(!SwarmFormation::GetSlot(result, 3, point), "GetSlot should reject invalid index");
}

} // namespace

int main() {
    TestPresetFormations();
    TestCircleSingleDrone();
    TestSquareLastRowStartsLeft();
    TestTextFormations();
    TestTextTakesPrecedence();
    TestHersheyChineseFormations();
    TestBuiltInLatinAlphabet();
    TestChineseDensityAcrossFleetSizes();
    TestInvalidInputs();
    TestGetSlot();

    if (g_failures == 0) {
        std::cout << "All formation algorithm tests passed.\n";
        return EXIT_SUCCESS;
    }
    std::cerr << g_failures << " formation algorithm test(s) failed.\n";
    return EXIT_FAILURE;
}
