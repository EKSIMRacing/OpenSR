#include "TrackRenderer.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
    constexpr double kPi = 3.14159265358979323846;
}

double CatmullRom(double p0, double p1, double p2, double p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    return 0.5 * ((2.0 * p1) +
        (-p0 + p2) * t +
        (2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3) * t2 +
        (-p0 + 3.0 * p1 - 3.0 * p2 + p3) * t3);
}

std::vector<Point3D> TrackRenderer::loadControlPointsFromFile(const std::wstring& filePath) {
    std::vector<Point3D> points;
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open telemetry dump file.");
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;
        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }

        if (tokens.size() < 7) continue;

        try {
            Point3D p;
            p.x = std::stod(tokens[4]);
            p.y = std::stod(tokens[5]);
            p.z = std::stod(tokens[6]);
            points.push_back(p);
        }
        catch (...) {
            continue;
        }
    }

    return points;
}

bool TrackRenderer::init(const wchar_t* path, const wchar_t* filename) {
    if (!filename) return false;

    std::wstring fullPath;
    if (path) {
        fullPath = std::wstring(path);
        if (!fullPath.empty() && fullPath.back() != L'\\') fullPath += L'\\';
        fullPath += filename;
    }
    else {
        fullPath = filename;
    }
    m_fullPath = fullPath;

    std::vector<Point3D> controlPoints;
    try {
        controlPoints = loadControlPointsFromFile(fullPath);
    }
    catch (const std::exception&) {
        return false;
    }

    if (controlPoints.size() < 4) {
        return false;
    }

    BuildSmoothedPath(controlPoints);
    ComputeBounds();
    ResetView();

    return true;
}

void TrackRenderer::ResetView() {
    fixed_scale_ = 0.0f;
    zoom_factor_ = 1.0;
    pan_offset_x_ = 0.0;
    pan_offset_z_ = 0.0;
    angle_deg_ = 0;
}

void TrackRenderer::BuildSmoothedPath(const std::vector<Point3D>& controlPoints) {
    smoothed_track_path_.clear();

    const int n = static_cast<int>(controlPoints.size());
    const int kSegmentsPerSpan = 8;

    for (int i = 0; i < n; ++i) {
        const Point3D& p0 = controlPoints[std::max<>(i - 1, 0)];
        const Point3D& p1 = controlPoints[i];
        const Point3D& p2 = controlPoints[std::min<>(i + 1, n - 1)];
        const Point3D& p3 = controlPoints[std::min<>(i + 2, n - 1)];

        for (int s = 0; s < kSegmentsPerSpan; ++s) {
            float t = static_cast<float>(s) / kSegmentsPerSpan;

            Point3D pt;
            pt.x = CatmullRom(p0.x, p1.x, p2.x, p3.x, t);
            pt.y = CatmullRom(p0.y, p1.y, p2.y, p3.y, t);
            pt.z = CatmullRom(p0.z, p1.z, p2.z, p3.z, t);

            smoothed_track_path_.push_back(pt);
        }
    }

    if (!controlPoints.empty()) {
        smoothed_track_path_.push_back(controlPoints.back());
    }
}

void TrackRenderer::ComputeBounds() {
    if (smoothed_track_path_.empty()) return;

    min_x_ = max_x_ = smoothed_track_path_[0].x;
    min_z_ = max_z_ = smoothed_track_path_[0].z;

    for (const auto& p : smoothed_track_path_) {
        min_x_ = std::min<>(min_x_, p.x);
        max_x_ = std::max<>(max_x_, p.x);
        min_z_ = std::min<>(min_z_, p.z);
        max_z_ = std::max<>(max_z_, p.z);
    }

    center_x_ = (min_x_ + max_x_) * 0.5;
    center_z_ = (min_z_ + max_z_) * 0.5;

    double worldW = max_x_ - min_x_;
    double worldH = max_z_ - min_z_;

    bounding_radius_ = 0.5 * std::sqrt(worldW * worldW + worldH * worldH);
    if (bounding_radius_ < 1.0) bounding_radius_ = 1.0;
}

void TrackRenderer::SetAngleView(int degree) {
    angle_deg_ = degree % 360;
    if (angle_deg_ < 0) angle_deg_ += 360;
}

Point3D TrackRenderer::RotateAroundCenter(const Point3D& p) const {
    if (angle_deg_ == 0) return p;

    double rad = angle_deg_ * kPi / 180.0;
    double cosA = std::cos(rad);
    double sinA = std::sin(rad);

    double dx = p.x - center_x_;
    double dz = p.z - center_z_;

    Point3D r;
    r.x = dx * cosA - dz * sinA + center_x_;
    r.z = dx * sinA + dz * cosA + center_z_;
    r.y = p.y;
    return r;
}

// ---------------------------------------------------------------------------
// Input Handling Implementation
// ---------------------------------------------------------------------------

void TrackRenderer::OnMouseDown(int x, int y, bool isRightButton) {
    last_mouse_pos_ = { x, y };
    if (isRightButton) {
        is_rotating_ = true;
    }
    else {
        is_panning_ = true;
    }
}

void TrackRenderer::OnMouseUp(int x, int y, bool isRightButton) {
    if (isRightButton) {
        is_rotating_ = false;
    }
    else {
        is_panning_ = false;
    }
}

void TrackRenderer::OnMouseMove(int x, int y) {
    int dx = x - last_mouse_pos_.x;
    int dy = y - last_mouse_pos_.y;
    last_mouse_pos_ = { x, y };

    if (is_panning_ && fixed_scale_ > 0.0f) {
        // Convert screen pixel movement to world coordinate movement
        pan_offset_x_ += dx / (fixed_scale_ * zoom_factor_);
        pan_offset_z_ += dy / (fixed_scale_ * zoom_factor_); // Screen Y inverted
    }
    else if (is_rotating_) {
        int new_angle = (GetKeyState(VK_SHIFT) & 0x8000) ? ((angle_deg_ + dx + 22) / 45) * 45 : angle_deg_ + dx;
        SetAngleView(new_angle);
    }
    //else if (is_rotating_) {
    //    // Dragging right/left rotates the track view
    //    SetAngleView(angle_deg_ + dx);
    //}
}

void TrackRenderer::OnMouseWheel(int zDelta) {
    double zoomFactorChange = (zDelta > 0) ? 1.15 : (1.0 / 1.15);
    zoom_factor_ *= zoomFactorChange;

    // Clamp zoom bounds
    if (zoom_factor_ < 0.1) zoom_factor_ = 0.1;
    if (zoom_factor_ > 50.0) zoom_factor_ = 50.0;
}

// ---------------------------------------------------------------------------
// Projection & Draw
// ---------------------------------------------------------------------------

Gdiplus::PointF TrackRenderer::WorldToScreen(const Point3D& worldPos, const RECT& clientRect,
    double worldW, double worldH) {
    int clientW = clientRect.right - clientRect.left;
    int clientH = clientRect.bottom - clientRect.top;

    const float kPadding = 40.0f;
    float usableW = std::max<>(1, clientW) - 2 * kPadding;
    float usableH = std::max<>(1, clientH) - 2 * kPadding;
    usableW = std::max<>(usableW, 1.0f);
    usableH = std::max<>(usableH, 1.0f);

    if (fixed_scale_ <= 0.0f) {
        float scaleX = worldW > 0 ? usableW / static_cast<float>(worldW) : 1.0f;
        float scaleY = worldH > 0 ? usableH / static_cast<float>(worldH) : 1.0f;
        fixed_scale_ = std::min<>(scaleX, scaleY);
    }

    // Apply rotation around center first
    Point3D rotated = RotateAroundCenter(worldPos);

    float effectiveScale = fixed_scale_ * static_cast<float>(zoom_factor_);

    // Apply pan offsets and center mapping
    float screenX = clientRect.left + clientW / 2.0f +
        static_cast<float>((rotated.x - center_x_) + pan_offset_x_) * effectiveScale;

    float screenY = clientRect.top + clientH / 2.0f +
        static_cast<float>((rotated.z - center_z_) + pan_offset_z_) * effectiveScale;

    return Gdiplus::PointF(screenX, screenY);
}

void TrackRenderer::Draw(HDC hdc, const RECT& clientRect, const Point3D& carPosition) {
    if (smoothed_track_path_.empty()) return;

    Gdiplus::Graphics graphics(hdc);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

   /* Gdiplus::SolidBrush bgBrush(Gdiplus::Color(255, 20, 20, 20));
    graphics.FillRectangle(&bgBrush,
        (int)clientRect.left, (int)clientRect.top,
        (int)(clientRect.right - clientRect.left),
        (int)(clientRect.bottom - clientRect.top));*/

    double worldSize = bounding_radius_ * 2.0;

    std::vector<Gdiplus::PointF> screenPoints;
    screenPoints.reserve(smoothed_track_path_.size());
    for (const auto& p : smoothed_track_path_) {
        screenPoints.push_back(WorldToScreen(p, clientRect, worldSize, worldSize));
    }

    Gdiplus::Pen trackPen(Gdiplus::Color(255, 80, 180, 255), 4.0f);
    trackPen.SetLineJoin(Gdiplus::LineJoinRound);
    trackPen.SetStartCap(Gdiplus::LineCapRound);
    trackPen.SetEndCap(Gdiplus::LineCapRound);

    if (screenPoints.size() >= 2) {
        graphics.DrawLines(&trackPen, screenPoints.data(), static_cast<INT>(screenPoints.size()));
    }

    Gdiplus::PointF carScreen = WorldToScreen(carPosition, clientRect, worldSize, worldSize);
    const float carRadius = 7.0f;

    Gdiplus::SolidBrush carBrush(Gdiplus::Color(255, 255, 60, 60));
    graphics.FillEllipse(&carBrush,
        carScreen.X - carRadius, carScreen.Y - carRadius,
        carRadius * 2.0f, carRadius * 2.0f);

    Gdiplus::Pen carOutline(Gdiplus::Color(255, 255, 255, 255), 2.0f);
    graphics.DrawEllipse(&carOutline,
        carScreen.X - carRadius, carScreen.Y - carRadius,
        carRadius * 2.0f, carRadius * 2.0f);
}

