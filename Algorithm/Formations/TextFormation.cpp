#include "TextFormation.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

#include "../Math/GeometryUtils.h"
#include "HersheyFont.h"
#include "TextRasterizerWin.h"

namespace SwarmFormation {
namespace detail {
namespace {

double RasterDistanceSquared(const RasterPoint& lhs, const RasterPoint& rhs) {
    const double dx = static_cast<double>(lhs.x - rhs.x);
    const double dy = static_cast<double>(lhs.y - rhs.y);
    return dx * dx + dy * dy;
}

std::vector<RasterPoint> DownsampleInkBeforeThinning(
    const std::vector<RasterPoint>& pixels) {
    if (pixels.empty()) {
        return {};
    }
    int min_x = pixels.front().x;
    int max_x = pixels.front().x;
    int min_y = pixels.front().y;
    int max_y = pixels.front().y;
    for (const RasterPoint& point : pixels) {
        min_x = std::min(min_x, point.x);
        max_x = std::max(max_x, point.x);
        min_y = std::min(min_y, point.y);
        max_y = std::max(max_y, point.y);
    }

    constexpr int kCellSize = 3;
    const int grid_width = (max_x - min_x) / kCellSize + 1;
    const int grid_height = (max_y - min_y) / kCellSize + 1;
    std::vector<unsigned int> coverage(
        static_cast<std::size_t>(grid_width) * grid_height, 0);
    for (const RasterPoint& point : pixels) {
        const int grid_x = (point.x - min_x) / kCellSize;
        const int grid_y = (point.y - min_y) / kCellSize;
        ++coverage[static_cast<std::size_t>(grid_y) * grid_width + grid_x];
    }

    std::vector<RasterPoint> downsampled;
    for (int grid_y = 0; grid_y < grid_height; ++grid_y) {
        for (int grid_x = 0; grid_x < grid_width; ++grid_x) {
            // Requiring more than one ink pixel rejects isolated antialiasing
            // noise while retaining thin diagonal strokes.
            if (coverage[static_cast<std::size_t>(grid_y) * grid_width + grid_x] >= 2) {
                downsampled.push_back({grid_x, grid_y});
            }
        }
    }
    return downsampled;
}

std::vector<RasterPoint> ThinToStrokeSkeleton(const std::vector<RasterPoint>& pixels) {
    if (pixels.empty()) {
        return {};
    }

    int min_x = pixels.front().x;
    int max_x = pixels.front().x;
    int min_y = pixels.front().y;
    int max_y = pixels.front().y;
    for (const RasterPoint& point : pixels) {
        min_x = std::min(min_x, point.x);
        max_x = std::max(max_x, point.x);
        min_y = std::min(min_y, point.y);
        max_y = std::max(max_y, point.y);
    }

    const int width = max_x - min_x + 3;
    const int height = max_y - min_y + 3;
    std::vector<unsigned char> image(static_cast<std::size_t>(width) * height, 0);
    auto at = [&](int x, int y) -> unsigned char& {
        return image[static_cast<std::size_t>(y) * width + x];
    };
    for (const RasterPoint& point : pixels) {
        at(point.x - min_x + 1, point.y - min_y + 1) = 1;
    }

    bool changed = true;
    unsigned int thinning_rounds = 0;
    std::vector<std::pair<int, int>> remove;
    // A malformed or unusually thick glyph must not block a DDS command callback
    // forever. Normal Microsoft YaHei glyphs converge well before this limit.
    constexpr unsigned int kMaximumThinningRounds = 64;
    while (changed && thinning_rounds++ < kMaximumThinningRounds) {
        changed = false;
        for (int pass = 0; pass < 2; ++pass) {
            remove.clear();
            for (int y = 1; y < height - 1; ++y) {
                for (int x = 1; x < width - 1; ++x) {
                    if (at(x, y) == 0) {
                        continue;
                    }
                    const int p2 = at(x, y - 1), p3 = at(x + 1, y - 1);
                    const int p4 = at(x + 1, y), p5 = at(x + 1, y + 1);
                    const int p6 = at(x, y + 1), p7 = at(x - 1, y + 1);
                    const int p8 = at(x - 1, y), p9 = at(x - 1, y - 1);
                    const int neighbors = p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9;
                    if (neighbors < 2 || neighbors > 6) {
                        continue;
                    }
                    const int transitions =
                        (p2 == 0 && p3 == 1) + (p3 == 0 && p4 == 1) +
                        (p4 == 0 && p5 == 1) + (p5 == 0 && p6 == 1) +
                        (p6 == 0 && p7 == 1) + (p7 == 0 && p8 == 1) +
                        (p8 == 0 && p9 == 1) + (p9 == 0 && p2 == 1);
                    if (transitions != 1) {
                        continue;
                    }
                    const bool keep_corner = pass == 0
                        ? (p2 * p4 * p6 != 0 || p4 * p6 * p8 != 0)
                        : (p2 * p4 * p8 != 0 || p2 * p6 * p8 != 0);
                    if (!keep_corner) {
                        remove.push_back({x, y});
                    }
                }
            }
            if (!remove.empty()) {
                changed = true;
                for (const auto& point : remove) {
                    at(point.first, point.second) = 0;
                }
            }
        }
    }

    std::vector<RasterPoint> skeleton;
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            if (at(x, y) != 0) {
                skeleton.push_back({x + min_x - 1, y + min_y - 1});
            }
        }
    }
    return skeleton;
}

std::vector<RasterPoint> SelectFarthestPoints(
    const std::vector<RasterPoint>& candidates,
    unsigned int count) {
    std::vector<RasterPoint> selected;
    if (candidates.empty() || count == 0 || candidates.size() < count) {
        return selected;
    }

    selected.reserve(count);
    std::size_t first_index = 0;
    for (std::size_t i = 1; i < candidates.size(); ++i) {
        if (candidates[i].x < candidates[first_index].x ||
            (candidates[i].x == candidates[first_index].x &&
             candidates[i].y < candidates[first_index].y)) {
            first_index = i;
        }
    }

    std::vector<double> minimum_distance(candidates.size(),
                                         std::numeric_limits<double>::max());
    std::vector<bool> used(candidates.size(), false);
    std::size_t selected_index = first_index;
    for (unsigned int selection = 0; selection < count; ++selection) {
        selected.push_back(candidates[selected_index]);
        used[selected_index] = true;

        double farthest_distance = -1.0;
        std::size_t farthest_index = selected_index;
        for (std::size_t i = 0; i < candidates.size(); ++i) {
            if (used[i]) {
                continue;
            }
            minimum_distance[i] = std::min(
                minimum_distance[i], RasterDistanceSquared(candidates[i], candidates[selected_index]));
            if (minimum_distance[i] > farthest_distance) {
                farthest_distance = minimum_distance[i];
                farthest_index = i;
            } else if (std::fabs(minimum_distance[i] - farthest_distance) <= 1e-12) {
                const RasterPoint& current = candidates[i];
                const RasterPoint& previous = candidates[farthest_index];
                if (current.y < previous.y ||
                    (current.y == previous.y && current.x < previous.x)) {
                    farthest_index = i;
                }
            }
        }
        selected_index = farthest_index;
    }
    return selected;
}

std::vector<RasterPoint> SelectFarthestPointsWithAnchors(
    const std::vector<RasterPoint>& candidates,
    const std::vector<RasterPoint>& anchors,
    unsigned int count) {
    if (anchors.empty()) {
        return SelectFarthestPoints(candidates, count);
    }
    if (anchors.size() >= count) {
        return anchors.size() == count
            ? anchors
            : SelectFarthestPoints(anchors, count);
    }

    std::vector<RasterPoint> selected = anchors;
    selected.reserve(count);
    while (selected.size() < count) {
        double best_distance = -1.0;
        std::size_t best_index = 0;
        for (std::size_t i = 0; i < candidates.size(); ++i) {
            bool already_selected = false;
            double nearest = std::numeric_limits<double>::max();
            for (const RasterPoint& point : selected) {
                const double distance = RasterDistanceSquared(candidates[i], point);
                if (distance <= 1e-12) {
                    already_selected = true;
                    break;
                }
                nearest = std::min(nearest, distance);
            }
            if (!already_selected && best_distance < nearest) {
                best_distance = nearest;
                best_index = i;
            }
        }
        if (best_distance < 0.0) {
            return {};
        }
        selected.push_back(candidates[best_index]);
    }
    return selected;
}

std::vector<RasterPoint> RelaxSkeletonSamples(
    const std::vector<RasterPoint>& candidates,
    std::vector<RasterPoint> selected,
    const std::vector<RasterPoint>& locked_points = {}) {
    if (selected.empty()) {
        return selected;
    }

    // Farthest-point seeding recursively halves long strokes.  Without enough
    // relaxation this leaves an alternating long/short gap pattern.
    constexpr int kRelaxationRounds = 24;
    for (int round = 0; round < kRelaxationRounds; ++round) {
        std::vector<std::vector<std::size_t>> clusters(selected.size());
        for (std::size_t candidate_index = 0;
             candidate_index < candidates.size(); ++candidate_index) {
            std::size_t nearest = 0;
            double nearest_distance = RasterDistanceSquared(
                candidates[candidate_index], selected[0]);
            for (std::size_t i = 1; i < selected.size(); ++i) {
                const double distance = RasterDistanceSquared(
                    candidates[candidate_index], selected[i]);
                if (distance < nearest_distance) {
                    nearest_distance = distance;
                    nearest = i;
                }
            }
            clusters[nearest].push_back(candidate_index);
        }

        for (std::size_t i = 0; i < selected.size(); ++i) {
            const bool locked = std::any_of(
                locked_points.begin(), locked_points.end(),
                [&](const RasterPoint& point) {
                    return point.x == selected[i].x && point.y == selected[i].y;
                });
            if (locked) {
                continue;
            }
            if (clusters[i].empty()) {
                continue;
            }
            double center_x = 0.0;
            double center_y = 0.0;
            for (std::size_t candidate_index : clusters[i]) {
                center_x += candidates[candidate_index].x;
                center_y += candidates[candidate_index].y;
            }
            center_x /= static_cast<double>(clusters[i].size());
            center_y /= static_cast<double>(clusters[i].size());

            std::size_t best = clusters[i].front();
            double best_distance = std::numeric_limits<double>::max();
            for (std::size_t candidate_index : clusters[i]) {
                const double dx = candidates[candidate_index].x - center_x;
                const double dy = candidates[candidate_index].y - center_y;
                const double distance = dx * dx + dy * dy;
                if (distance < best_distance) {
                    best_distance = distance;
                    best = candidate_index;
                }
            }
            selected[i] = candidates[best];
        }
    }
    return selected;
}

FormationError MapRasterError(TextRasterError error) {
    switch (error) {
    case TextRasterError::InvalidUtf8:
        return FormationError::InvalidUtf8Text;
    case TextRasterError::NoVisiblePixels:
        return FormationError::InsufficientTextPoints;
    case TextRasterError::GdiFailure:
        return FormationError::TextRasterizationFailed;
    case TextRasterError::None:
    default:
        return FormationError::None;
    }
}

} // namespace

FormationResult GenerateTextFormation(const FormationRequest& request) {
    if (request.count == 0) {
        return {false, {}, FormationError::InvalidCount};
    }
    std::vector<RasterPoint> candidates;
    std::vector<RasterPoint> skeleton;
    std::vector<RasterPoint> anchors;
    const bool used_hershey = BuildHersheyTextCandidates(
        request.formation_text, candidates, &anchors);
    if (used_hershey) {
        skeleton = std::move(candidates);
        std::vector<RasterPoint> uniform_seeds;
        if (BuildHersheyTextUniformSeeds(
                request.formation_text, request.count, uniform_seeds)) {
            // Snap nearby uniform samples onto structural corners instead of
            // adding every corner as another locked point. This preserves a
            // bend without creating a dense corner cluster. A snap is rejected
            // when it would crowd another sample.
            std::vector<double> nearest_distances;
            nearest_distances.reserve(uniform_seeds.size());
            for (std::size_t i = 0; i < uniform_seeds.size(); ++i) {
                double nearest = std::numeric_limits<double>::max();
                for (std::size_t j = 0; j < uniform_seeds.size(); ++j) {
                    if (i != j) {
                        nearest = std::min(nearest, RasterDistanceSquared(
                            uniform_seeds[i], uniform_seeds[j]));
                    }
                }
                if (nearest < std::numeric_limits<double>::max()) {
                    nearest_distances.push_back(std::sqrt(nearest));
                }
            }
            double nominal_distance = 0.0;
            if (!nearest_distances.empty()) {
                const std::size_t middle = nearest_distances.size() / 2;
                std::nth_element(nearest_distances.begin(),
                                 nearest_distances.begin() + middle,
                                 nearest_distances.end());
                nominal_distance = nearest_distances[middle];
            }

            std::vector<bool> snapped(uniform_seeds.size(), false);
            const double minimum_corner_separation_squared =
                nominal_distance * nominal_distance * 0.64;
            for (const RasterPoint& corner : anchors) {
                std::size_t nearest_index = uniform_seeds.size();
                double nearest = std::numeric_limits<double>::max();
                for (std::size_t i = 0; i < uniform_seeds.size(); ++i) {
                    if (!snapped[i]) {
                        const double distance = RasterDistanceSquared(
                            corner, uniform_seeds[i]);
                        if (distance < nearest) {
                            nearest = distance;
                            nearest_index = i;
                        }
                    }
                }
                if (nearest_index == uniform_seeds.size()) {
                    break;
                }
                bool crowds_neighbor = false;
                for (std::size_t i = 0; i < uniform_seeds.size(); ++i) {
                    if (i != nearest_index && RasterDistanceSquared(
                            corner, uniform_seeds[i]) <
                            minimum_corner_separation_squared) {
                        crowds_neighbor = true;
                        break;
                    }
                }
                if (!crowds_neighbor) {
                    uniform_seeds[nearest_index] = corner;
                    snapped[nearest_index] = true;
                }
            }
            anchors = std::move(uniform_seeds);
        }
    } else {
        const TextRasterError raster_error = RasterizeUtf8Text(
            request.formation_text, request.count, candidates);
        if (raster_error != TextRasterError::None) {
            return {false, {}, MapRasterError(raster_error)};
        }
        const std::vector<RasterPoint> coarse_ink =
            DownsampleInkBeforeThinning(candidates);
        skeleton = ThinToStrokeSkeleton(coarse_ink);
    }
    if (skeleton.size() < request.count) {
        return {false, {}, FormationError::InsufficientTextPoints};
    }

    std::vector<RasterPoint> selected;
    if (skeleton.size() == request.count) {
        selected = skeleton;
    } else if (used_hershey) {
        selected = SelectFarthestPointsWithAnchors(
            skeleton, anchors, request.count);
    } else {
        selected = SelectFarthestPoints(skeleton, request.count);
        selected = RelaxSkeletonSamples(skeleton, std::move(selected));
    }
    if (selected.size() != request.count) {
        return {false, {}, FormationError::InsufficientTextPoints};
    }

    std::vector<Point2D> slots;
    slots.reserve(selected.size());
    for (const RasterPoint& point : selected) {
        slots.push_back({static_cast<double>(point.x), -static_cast<double>(point.y)});
    }

    CenterBoundingBox(slots);
    if (slots.size() >= 2) {
        const double minimum_distance = MinimumPairDistance(slots);
        if (!(minimum_distance > 0.0) || !std::isfinite(minimum_distance)) {
            return {false, {}, FormationError::InvalidGeneratedSlot};
        }
        ScalePoints(slots, request.spacing / minimum_distance);
    }
    TranslatePoints(slots, request.center_x, request.center_y);
    SortSlotsStable(slots);
    return {true, std::move(slots), FormationError::None};
}

} // namespace detail
} // namespace SwarmFormation
