#pragma once

#include <Windows.h>
#include <gdiplus.h>
#include <string>
#include <vector>

#pragma comment (lib,"Gdiplus.lib")

struct Point3D {
    double x = 0.0, y = 0.0, z = 0.0;
};

struct TelemetryPoint {
    int timestamp_ms;
    double speed_ms;
    int gear;
    int rpm;
    Point3D position;
};

double CatmullRom(double p0, double p1, double p2, double p3, float t);

class TrackRenderer {
public:
    TrackRenderer() {}

    bool init(const wchar_t* path = nullptr, const wchar_t* filename = nullptr);
    std::vector<Point3D> loadControlPointsFromFile(const std::wstring& filePath);

    void Draw(HDC hdc, const RECT& clientRect, const Point3D& carPosition);

    // View control inputs to be called from the host window's WndProc
    void OnMouseDown(int x, int y, bool isRightButton);
    void OnMouseUp(int x, int y, bool isRightButton);
    void OnMouseMove(int x, int y);
    void OnMouseWheel(int zDelta);

    void SetAngleView(int degree);
    int  GetAngleView() const { return angle_deg_; }

    // Reset view to default centering/zoom
    void ResetView();

    float fixed_scale_ = 0.0f;
    std::wstring m_fullPath;

private:
    std::vector<Point3D> smoothed_track_path_;
    double min_x_ = 0, max_x_ = 0, min_z_ = 0, max_z_ = 0;

    double center_x_ = 0.0, center_z_ = 0.0;
    double bounding_radius_ = 1.0;
    int angle_deg_ = 0;

    // Interactive view states
    double zoom_factor_ = 1.0;
    double pan_offset_x_ = 0.0; // Pan offset in world units
    double pan_offset_z_ = 0.0;

    bool is_panning_ = false;
    bool is_rotating_ = false;
    POINT last_mouse_pos_ = { 0, 0 };

    void BuildSmoothedPath(const std::vector<Point3D>& controlPoints);
    void ComputeBounds();
    Point3D RotateAroundCenter(const Point3D& p) const;

    Gdiplus::PointF WorldToScreen(const Point3D& worldPos, const RECT& clientRect, double worldW, double worldH);
};

