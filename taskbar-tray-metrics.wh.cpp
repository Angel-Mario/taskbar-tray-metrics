// ==WindhawkMod==
// @id              taskbar-tray-metrics
// @name            Taskbar tray metrics
// @description     Shows system performance metrics (network speed, CPU, RAM) inside the Windows 11 system tray, to the left of the tray icons. A TrafficMonitor-like display that lives natively in the XAML taskbar and can be styled with Windows 11 Taskbar Styler.
// @version         0.1.0
// @author          Angel Mario Roque Suárez
// @github          https://github.com/Angel-Mario/taskbar-tray-metrics
// @include         explorer.exe
// @architecture    x86-64
// @compilerOptions -lole32 -loleaut32 -lruntimeobject -lpdh -lversion
// ==/WindhawkMod==

// The MIT License (MIT)
//
// Copyright (c) 2026 Angel Mario Roque Suárez
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

// ==WindhawkModReadme==
/*
# Taskbar tray metrics

Shows system performance metrics - upload/download network speed, CPU usage
and RAM usage - inside the Windows 11 system tray, to the left of the tray
icons (before the overflow chevron), like TrafficMonitor's taskbar window,
but natively rendered in the taskbar XAML.

```
↑: 0.34 KB/s  CPU: 10 %
↓: 0.27 KB/s  MEM: 69 %      ^ [xp] [📶] [🔊]   5:30 PM
```

**Takes 2 seconds to kick in** after explorer reload - the network/CPU
counters need two samples before rates become available.

## Compatibility

* Windows 11 only (XAML taskbar).
* Designed to coexist with **Windows 11 Taskbar Styler** - the inserted
  element is a plain tray-area `StackPanel` named `TaskbarTrayMetricsStack`
  containing a `Grid` named `MetricsTextGrid` with up to four `TextBlock`
  cells (`MetricsTextBlock`, `MetricsRight1`, `MetricsLeft2`,
  `MetricsRight2`), so you can additionally style it from the Taskbar
  Styler mod with targets such as:
  `StackPanel#TaskbarTrayMetricsStack > TextBlock#MetricsTextBlock`
* The mod does not hook the clock; it complements "Taskbar Clock
  Customization".

## Formatting patterns

`Top line` and `Bottom line` support the following patterns:

* `%up%` - upload transfer rate (e.g. `0.27 KB/s`)
* `%down%` - download transfer rate (e.g. `0.34 KB/s`)
* `%total%` - combined up+down transfer rate
* `%cpu%` - CPU usage, e.g. `10 %`
* `%ram%` - RAM usage, e.g. `69 %`
* `%ram_used%` - used RAM amount, e.g. `9.3 GB`
* `%newline%` - a line break

Leave `Bottom line` empty to hide the second line.

## Notes

* Rates use a `.` decimal separator, independent of the system locale.
* `Network adapter name` matches by case-insensitive substring; leave empty
  to aggregate all non-loopback adapters.
* If the text color is not set, it is copied from a neighboring tray icon
  text block, so it follows the system theme.
* Fully removable - stop the mod or hide its window at any time; nothing is
  persisted outside the hosting taskbar window.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- line1Format: >-
    ↑: %up%  CPU: %cpu%
  $name: Top line
  $description: >-
    Text shown on the first line, supports the %up%, %down%, %total%, %cpu%,
    %ram%, %ram_used% and %newline% patterns.
- line2Format: >-
    ↓: %down%  MEM: %ram%
  $name: Bottom line
  $description: >-
    Text shown on the second line. Leave empty to hide the second line.
- showSecondLine: true
  $name: Show bottom line
- updateIntervalMs: 1000
  $name: Update interval (ms)
  $description: >-
    Sampling interval of the performance counters. Values below 250 ms are
    clamped.
- networkAdapterName: ""
  $name: Network adapter name
  $description: >-
    The network adapter to use for upload/download metrics (case-insensitive
    substring match). Leave empty to sum all non-loopback adapters. To list
    adapters, run:

    typeperf -qx "Network Interface"
- insertPosition: beforeChevron
  $name: Insert position
  $description: >-
    Where to insert the metrics block relative to the tray icons.
  $options:
  - beforeChevron: Before the chevron (left of all tray icons)
  - afterChevron: After the chevron (right of the chevron)
- textAlignment: left
  $name: Text alignment
  $options:
  - left: Left
  - center: Center
  - right: Right
- fontSize: 0
  $name: Font size
  $description: >-
    Set to zero to copy the font size of a neighboring tray icon text block.
- fontFamily: ""
  $name: Font family
  $description: >-
    Leave empty to copy the font family of a neighboring tray icon text
    block.
- textColor: ""
  $name: Text color
  $description: >-
    A color name (Red, Black, ...) or an RGB/ARGB color code (like #00FF00,
    #CC00FF00, ...). Leave empty to copy the color of a neighboring tray icon
    text block.
- marginLeft: 0
  $name: Left margin
- marginRight: 8
  $name: Right margin
- fixedWidth: 0
  $name: Fixed width (px)
  $description: >-
    Fixes the metrics block width so the taskbar does not resize while the
    numbers change. Zero keeps the width automatic. Use the text alignment
    setting to decide how the text fills this width.
- leftColWidth: 0
  $name: Left column width (px)
  $description: >-
    Each line is split into two columns at the first double space (use two
    spaces between the value part and the label part of the format, like
    "↑: %up%  CPU: %cpu%"). This fixes the width of the left column so the
    right column stays aligned. Zero keeps it automatic (both rows share the
    widest left cell).
- showTooltip: true
  $name: Hover tooltip
  $description: >-
    Shows a tooltip on hover with RAM used vs. total (GB and MB), CPU and
    network speeds, like Traffic Monitor.
*/
// ==/WindhawkModSettings==

#include <windhawk_utils.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cwchar>
#include <cwctype>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#undef GetCurrentTime

#include <pdh.h>
#include <pdhmsg.h>
#undef GetCurrentTime

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Controls.Primitives.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/base.h>

using namespace winrt::Windows::UI::Xaml;

// ---------------------------------------------------------------------------
// Diagnostics: appends to %TEMP%\taskbar-tray-metrics-debug.log
// ---------------------------------------------------------------------------

void LogDiagReset() {
    wchar_t path[MAX_PATH];
    if (GetTempPathW(ARRAYSIZE(path), path) == 0) {
        return;
    }
    wcscat_s(path, L"taskbar-tray-metrics-debug.log");

    HANDLE file = CreateFileW(path, DELETE, FILE_SHARE_READ, nullptr,
                              OPEN_EXISTING, 0, nullptr);
    if (file != INVALID_HANDLE_VALUE) {
        FILE_DISPOSITION_INFO info{TRUE};
        SetFileInformationByHandle(file, FileDispositionInfo, &info,
                                   sizeof(info));
        CloseHandle(file);
    }
}

void LogDiag(PCWSTR format, ...) {
    wchar_t path[MAX_PATH];
    if (GetTempPathW(ARRAYSIZE(path), path) == 0) {
        return;
    }
    wcscat_s(path, L"taskbar-tray-metrics-debug.log");

    wchar_t buffer[800];
    va_list args;
    va_start(args, format);
    _vsnwprintf_s(buffer, ARRAYSIZE(buffer), _TRUNCATE, format, args);
    va_end(args);

    SYSTEMTIME st;
    GetLocalTime(&st);

    wchar_t stamped[1000];
    _snwprintf_s(stamped, ARRAYSIZE(stamped), _TRUNCATE,
                 L"[%02u:%02u:%02u.%03u T%lu] %s\r\n", st.wHour, st.wMinute,
                 st.wSecond, st.wMilliseconds, GetCurrentThreadId(), buffer);

    HANDLE file = CreateFileW(path, FILE_APPEND_DATA,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              OPEN_ALWAYS, 0, nullptr);
    if (file != INVALID_HANDLE_VALUE) {
        SetFilePointer(file, 0, nullptr, FILE_END);
        DWORD written = 0;
        WriteFile(file, stamped, (DWORD)lstrlenW(stamped) * sizeof(wchar_t),
                  &written, nullptr);
        CloseHandle(file);
    }
}

// ---------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------

enum class InsertPosition {
    beforeChevron = 0,
    afterChevron = 1,
};

enum class MetricsTextAlignment {
    left = 0,
    center = 1,
    right = 2,
};

struct Settings {
    std::wstring line1Format;
    std::wstring line2Format;
    bool showSecondLine = true;
    int updateIntervalMs = 1000;
    std::wstring networkAdapterName;
    int insertPosition = (int)InsertPosition::beforeChevron;
    int textAlignment = (int)MetricsTextAlignment::left;
    int fontSize = 0;
    std::wstring fontFamily;
    std::wstring textColor;
    int marginLeft = 0;
    int marginRight = 8;
    int fixedWidth = 0;
    int leftColWidth = 0;
    bool showTooltip = true;
};

Settings ReadSettings() {
    Settings settings{};

    if (PCWSTR s = Wh_GetStringSetting(L"line1Format")) {
        settings.line1Format = s;
        Wh_FreeStringSetting(s);
    }
    if (PCWSTR s = Wh_GetStringSetting(L"line2Format")) {
        settings.line2Format = s;
        Wh_FreeStringSetting(s);
    }
    settings.showSecondLine = Wh_GetIntSetting(L"showSecondLine") != 0;
    settings.updateIntervalMs =
        std::clamp(Wh_GetIntSetting(L"updateIntervalMs"), 250, 60000);

    if (PCWSTR s = Wh_GetStringSetting(L"networkAdapterName")) {
        settings.networkAdapterName = s;
        Wh_FreeStringSetting(s);
    }

    PCWSTR insertPosition = Wh_GetStringSetting(L"insertPosition");
    settings.insertPosition = wcscmp(insertPosition, L"afterChevron") == 0
                                  ? (int)InsertPosition::afterChevron
                                  : (int)InsertPosition::beforeChevron;
    Wh_FreeStringSetting(insertPosition);

    PCWSTR textAlignment = Wh_GetStringSetting(L"textAlignment");
    if (wcscmp(textAlignment, L"center") == 0) {
        settings.textAlignment = (int)MetricsTextAlignment::center;
    } else if (wcscmp(textAlignment, L"right") == 0) {
        settings.textAlignment = (int)MetricsTextAlignment::right;
    } else {
        settings.textAlignment = (int)MetricsTextAlignment::left;
    }
    Wh_FreeStringSetting(textAlignment);

    settings.fontSize = Wh_GetIntSetting(L"fontSize");

    if (PCWSTR s = Wh_GetStringSetting(L"fontFamily")) {
        settings.fontFamily = s;
        Wh_FreeStringSetting(s);
    }
    if (PCWSTR s = Wh_GetStringSetting(L"textColor")) {
        settings.textColor = s;
        Wh_FreeStringSetting(s);
    }

    settings.marginLeft =
        std::clamp(Wh_GetIntSetting(L"marginLeft"), -100, 100);
    settings.marginRight =
        std::clamp(Wh_GetIntSetting(L"marginRight"), -100, 100);
    settings.fixedWidth = std::clamp(Wh_GetIntSetting(L"fixedWidth"), 0, 500);
    settings.leftColWidth =
        std::clamp(Wh_GetIntSetting(L"leftColWidth"), 0, 400);
    settings.showTooltip = Wh_GetIntSetting(L"showTooltip") != 0;

    return settings;
}

// ---------------------------------------------------------------------------
// Visual tree helpers (adapted from "Taskbar tray system icon tweaks" by
// m417z)
// ---------------------------------------------------------------------------

FrameworkElement EnumParentElements(
    FrameworkElement element,
    std::function<bool(FrameworkElement)> enumCallback) {
    auto parent = element;
    while (true) {
        parent = Media::VisualTreeHelper::GetParent(parent)
                     .try_as<FrameworkElement>();
        if (!parent) {
            return nullptr;
        }

        if (enumCallback(parent)) {
            return parent;
        }
    }
}

bool IsChildOfElementByName(FrameworkElement element, PCWSTR name) {
    return !!EnumParentElements(element, [name](FrameworkElement parent) {
        return parent.Name() == name;
    });
}

FrameworkElement EnumChildElements(
    FrameworkElement element,
    std::function<bool(FrameworkElement)> enumCallback);

Controls::Panel FindChildPanelByName(FrameworkElement element, PCWSTR name) {
    Controls::Panel result{nullptr};
    try {
        EnumChildElements(element, [&result, name](FrameworkElement child) {
            if (child.Name() == name) {
                result = child.try_as<Controls::Panel>();
                return !!result;
            }
            return false;
        });
    } catch (...) {
    }
    return result;
}

FrameworkElement FindChildElementByName(FrameworkElement element,
                                        PCWSTR name) {
    FrameworkElement result{nullptr};
    try {
        EnumChildElements(element, [&result, name](FrameworkElement child) {
            if (child.Name() == name) {
                result = child;
                return true;
            }
            return false;
        });
    } catch (...) {
    }
    return result;
}

FrameworkElement EnumChildElements(
    FrameworkElement element,
    std::function<bool(FrameworkElement)> enumCallback) {
    int childrenCount = Media::VisualTreeHelper::GetChildrenCount(element);

    for (int i = 0; i < childrenCount; i++) {
        auto child = Media::VisualTreeHelper::GetChild(element, i)
                         .try_as<FrameworkElement>();
        if (!child) {
            continue;
        }

        if (enumCallback(child)) {
            return child;
        }
    }

    return nullptr;
}

FrameworkElement FindChildByName(FrameworkElement element, PCWSTR name) {
    return EnumChildElements(element, [name](FrameworkElement child) {
        return child.Name() == name;
    });
}

FrameworkElement FindChildByClassName(FrameworkElement element,
                                      PCWSTR className) {
    return EnumChildElements(element, [className](FrameworkElement child) {
        return winrt::get_class_name(child) == className;
    });
}

Controls::Panel GetFirstPanelAncestor(FrameworkElement element) {
    auto parent = element;
    while (true) {
        parent = Media::VisualTreeHelper::GetParent(parent)
                     .try_as<FrameworkElement>();
        if (!parent) {
            return nullptr;
        }

        if (auto panel = parent.try_as<Controls::Panel>()) {
            return panel;
        }
    }
}

bool IsInMainTray(FrameworkElement element) {
    try {
        if (element.Name() == L"SystemTrayFrameGrid") {
            return true;
        }
    } catch (...) {
    }

    return IsChildOfElementByName(element, L"MainStack") ||
           IsChildOfElementByName(element, L"SystemTrayFrameGrid");
}

// Depth-limited DFS for the first TextBlock down the tree, used to copy the
// tray's native font and color.
Controls::TextBlock FindFirstTextBlock(FrameworkElement element, int depth) {
    if (!element || depth <= 0) {
        return nullptr;
    }

    if (auto textBlock = element.try_as<Controls::TextBlock>()) {
        return textBlock;
    }

    int childrenCount = Media::VisualTreeHelper::GetChildrenCount(element);
    for (int i = 0; i < childrenCount; i++) {
        auto child = Media::VisualTreeHelper::GetChild(element, i)
                         .try_as<FrameworkElement>();
        if (!child) {
            continue;
        }

        if (auto textBlock = FindFirstTextBlock(child, depth - 1)) {
            return textBlock;
        }
    }

    return nullptr;
}

// ---------------------------------------------------------------------------
// Formatting
// ---------------------------------------------------------------------------

std::wstring FormatDecimal(double value, int decimals) {
    value = std::max(0.0, value);

    wchar_t buffer[64];
    swprintf(buffer, ARRAYSIZE(buffer), L"%.*f", decimals, value);
    std::wstring result = buffer;

    // Defensive: use '.' as the decimal separator regardless of locale.
    std::replace(result.begin(), result.end(), L',', L'.');

    return result;
}

std::wstring FormatBytesPerSecond(double bytesPerSecond) {
    bytesPerSecond = std::max(0.0, bytesPerSecond);

    constexpr double kKB = 1024.0;
    constexpr double kMB = 1024.0 * 1024.0;

    if (bytesPerSecond < kMB) {
        return FormatDecimal(bytesPerSecond / kKB, 2) + L" KB/s";
    }

    return FormatDecimal(bytesPerSecond / kMB, 2) + L" MB/s";
}

std::wstring FormatPercent(double value) {
    wchar_t buffer[32];
    swprintf(buffer, ARRAYSIZE(buffer), L"%.0f %%",
             std::min(100.0, std::max(0.0, value)));
    std::wstring result = buffer;
    std::replace(result.begin(), result.end(), L',', L'.');
    return result;
}

std::wstring FormatGb(double bytes) {
    bytes = std::max(0.0, bytes);

    constexpr double kGB = 1024.0 * 1024.0 * 1024.0;
    return FormatDecimal(bytes / kGB, 1) + L" GB";
}

struct FormattedMetrics {
    std::wstring up;
    std::wstring down;
    std::wstring total;
    std::wstring cpu;
    std::wstring ram;
    std::wstring ram_used;
};

std::optional<std::wstring_view> PatternValue(std::wstring_view pattern,
                                              const FormattedMetrics& metrics) {
    if (pattern == L"up") {
        return std::optional<std::wstring_view>{metrics.up};
    }
    if (pattern == L"down") {
        return std::optional<std::wstring_view>{metrics.down};
    }
    if (pattern == L"total") {
        return std::optional<std::wstring_view>{metrics.total};
    }
    if (pattern == L"cpu") {
        return std::optional<std::wstring_view>{metrics.cpu};
    }
    if (pattern == L"ram") {
        return std::optional<std::wstring_view>{metrics.ram};
    }
    if (pattern == L"ram_used") {
        return std::optional<std::wstring_view>{metrics.ram_used};
    }
    return std::nullopt;
}

std::wstring ApplyFormat(const std::wstring& format,
                         const FormattedMetrics& metrics) {
    std::wstring result;
    result.reserve(format.size());

    for (size_t i = 0; i < format.size(); ++i) {
        if (format[i] != L'%') {
            result.push_back(format[i]);
            continue;
        }

        auto end = format.find(L'%', i + 1);
        if (end == std::wstring::npos) {
            result.push_back(format[i]);
            continue;
        }

        std::wstring_view pattern(format.data() + i + 1, end - i - 1);
        auto value = PatternValue(pattern, metrics);
        if (pattern == L"newline" || pattern == L"n") {
            result.push_back(L'\n');
        } else if (value) {
            result.append(*value);
        } else if (pattern.empty()) {
            // Escape: "%%" -> "%".
            result.push_back(L'%');
        } else if (pattern == L"up" || pattern == L"down" ||
                   pattern == L"total" || pattern == L"cpu" ||
                   pattern == L"ram" || pattern == L"ram_used") {
            // Known pattern, but temporarily unavailable (n/a).
            result.append(L"-");
        } else {
            // Not a pattern we know, keep the raw text.
            result.append(format, i, end - i + 1);
        }

        i = end;
    }

    return result;
}

std::optional<winrt::Windows::UI::Color> ParseColor(const std::wstring& text) {
    if (text.size() == 7 || text.size() == 9) {
        if (text[0] != L'#') {
            return std::nullopt;
        }

        auto isHex = [](wchar_t c) {
            return (c >= L'0' && c <= L'9') || (c >= L'a' && c <= L'f') ||
                   (c >= L'A' && c <= L'F');
        };
        for (size_t i = 1; i < text.size(); ++i) {
            if (!isHex(text[i])) {
                return std::nullopt;
            }
        }

        auto hexToByte = [isHex](size_t offset,
                                 const std::wstring& str) -> uint8_t {
            (void)isHex;
            auto digit = [](wchar_t c) -> int {
                if (c >= L'0' && c <= L'9') {
                    return c - L'0';
                }
                if (c >= L'a' && c <= L'f') {
                    return c - L'a' + 10;
                }
                return c - L'A' + 10;
            };
            return (uint8_t)((digit(str[offset]) << 4) | digit(str[offset + 1]));
        };

        winrt::Windows::UI::Color color{};
        color.A = text.size() == 9 ? hexToByte(1, text) : (uint8_t)0xFF;
        color.R = text.size() == 9 ? hexToByte(3, text) : hexToByte(1, text);
        color.G =
            text.size() == 9 ? hexToByte(5, text) : hexToByte(3, text);
        color.B =
            text.size() == 9 ? hexToByte(7, text) : hexToByte(5, text);

        return color;
    }

    if (_wcsicmp(text.c_str(), L"red") == 0) {
        return winrt::Windows::UI::Colors::Red();
    }
    if (_wcsicmp(text.c_str(), L"black") == 0) {
        return winrt::Windows::UI::Colors::Black();
    }
    if (_wcsicmp(text.c_str(), L"white") == 0) {
        return winrt::Windows::UI::Colors::White();
    }
    if (_wcsicmp(text.c_str(), L"gray") == 0) {
        return winrt::Windows::UI::Colors::Gray();
    }

    return std::nullopt;
}

// ---------------------------------------------------------------------------
// System metrics (PDH counters + GlobalMemoryStatusEx)
// ---------------------------------------------------------------------------

enum class MetricType {
    kDownloadSpeed,
    kUploadSpeed,
    kCpu,
    kCount,
};

class SystemMetrics {
   public:
    SystemMetrics() = default;
    SystemMetrics(const SystemMetrics&) = delete;
    SystemMetrics& operator=(const SystemMetrics&) = delete;
    ~SystemMetrics() { Clear(); }

    struct Sample {
        double downloadSum = 0;
        double uploadSum = 0;
        double cpuPercent = 0;
        int memoryLoadPercent = 0;
        double memoryUsedBytes = 0;
        bool downloadValid = false;
        bool uploadValid = false;
        bool cpuValid = false;
        bool memoryValid = false;
    };

    void SetAdapterName(std::wstring name) {
        if (name != appliedAdapterName_) {
            Reset();
        }
        adapterName_ = std::move(name);
    }

    std::optional<Sample> SampleOnce() {
        ++ticksSinceRebuild_;
        if (!valid_ || ticksSinceRebuild_ >= kRebuildIntervalTicks) {
            Rebuild();
            ticksSinceRebuild_ = 0;
        }

        if (!query_) {
            return std::nullopt;
        }

        PDH_STATUS hr = PdhCollectQueryData(query_);
        if (FAILED(hr)) {
            Wh_Log(L"PdhCollectQueryData error %08X", hr);
            return std::nullopt;
        }

        Sample sample;
        sample.downloadValid =
            QueryMetric(MetricType::kDownloadSpeed, sample.downloadSum);
        sample.uploadValid =
            QueryMetric(MetricType::kUploadSpeed, sample.uploadSum);
        sample.cpuValid = QueryMetric(MetricType::kCpu, sample.cpuPercent);

        MEMORYSTATUSEX status{sizeof(MEMORYSTATUSEX)};
        if (GlobalMemoryStatusEx(&status)) {
            sample.memoryValid = true;
            sample.memoryLoadPercent = status.dwMemoryLoad;
            sample.memoryUsedBytes =
                (double)status.ullTotalPhys - (double)status.ullAvailPhys;
        }

        static int reportedFirstSample = 0;
        if (reportedFirstSample < 3) {
            Wh_Log(L"Sample: down=%d up=%d cpu=%d", sample.downloadValid,
                   sample.uploadValid, sample.cpuValid);
            ++reportedFirstSample;
        }

        return sample;
    }

    void Clear() {
        if (query_) {
            PdhCloseQuery(query_);
            query_ = nullptr;
        }
        metrics_.clear();
        valid_ = false;
    }

   private:
    void Reset() {
        Clear();
        ticksSinceRebuild_ = kRebuildIntervalTicks;  // Force a rebuild soon.
    }

    void Rebuild() {
        Clear();

        if (FAILED(PdhOpenQuery(nullptr, 0, &query_))) {
            Wh_Log(L"PdhOpenQuery error");
            query_ = nullptr;
            return;
        }

        static constexpr struct StaticCounterDef {
            MetricType type;
            PCWSTR path;
            bool wildcard;
            // Only add this counter if the metric type has no other counter
            // yet (prevents summing overlapping CPU definitions).
            bool fallback;
        } defs[] = {
            {MetricType::kCpu,
             L"\\Processor(_Total)\\% Processor Time",
             false,
             false},
            {MetricType::kCpu,
             L"\\Processor Information(_Total)\\% Processor Utility",
             false,
             true},
            {MetricType::kDownloadSpeed,
             L"\\Network Interface(*)\\Bytes Received/sec",
             true,
             false},
            {MetricType::kUploadSpeed,
             L"\\Network Interface(*)\\Bytes Sent/sec",
             true,
             false},
        };

        for (const auto& def : defs) {
            if (def.fallback && !metrics_[def.type].counters.empty()) {
                continue;
            }
            AddMetric(def.type, def.path, def.wildcard);
        }

        appliedAdapterName_ = adapterName_;
        valid_ = true;
    }

    void AddMetric(MetricType type, PCWSTR path, bool wildcard) {
        auto& metric = metrics_[type];

        std::vector<std::wstring> paths;
        if (wildcard) {
            paths = ExpandEnglishWildcard(path);
        } else {
            paths.push_back(path);
        }

        for (const auto& p : paths) {
            if (!IsWantedInstance(p)) {
                continue;
            }

            PDH_HCOUNTER counter = nullptr;
            PDH_STATUS hr;
            if (wildcard) {
                // Expanded paths are already localized (and may carry a
                // machine name), so add them directly.
                hr = PdhAddCounter(query_, p.c_str(), 0, &counter);
            } else {
                hr = PdhAddEnglishCounter(query_, p.c_str(), 0, &counter);
            }
            if (FAILED(hr)) {
                Wh_Log(L"PdhAddCounter error %08X for %s", hr, p.c_str());
                LogDiag(L"AddMetric FAILED %08X for %ls", hr, p.c_str());
                continue;
            }

            metric.counters.emplace_back(p, counter);
        }
    }

    bool QueryMetric(MetricType type, double& out) {
        auto it = metrics_.find(type);
        if (it == metrics_.end()) {
            return false;
        }

        double sum = 0.0;
        bool anyValid = false;
        for (const auto& [path, counter] : it->second.counters) {
            (void)path;
            PDH_FMT_COUNTERVALUE value{};
            PDH_STATUS hr =
                PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, nullptr,
                                            &value);
            if (FAILED(hr) || value.CStatus != ERROR_SUCCESS) {
                continue;
            }

            sum += value.doubleValue;
            anyValid = true;
        }

        if (!anyValid) {
            return false;
        }

        out = sum;
        return true;
    }

    // Converts an English wildcard counter path to localized expanded paths,
    // per the PdhAddEnglishCounter docs.
    std::vector<std::wstring> ExpandEnglishWildcard(PCWSTR wildcardPath) {
        PDH_HCOUNTER tempCounter = nullptr;
        PDH_STATUS hr =
            PdhAddEnglishCounter(query_, wildcardPath, 0, &tempCounter);
        if (FAILED(hr)) {
            Wh_Log(L"PdhAddEnglishCounter error %08X", hr);
            return {};
        }

        DWORD required = 0;
        hr = PdhGetCounterInfo(tempCounter, FALSE, &required, nullptr);
        if (FAILED(hr) && hr != (PDH_STATUS)PDH_MORE_DATA) {
            Wh_Log(L"PdhGetCounterInfo error %08X", hr);
            PdhRemoveCounter(tempCounter);
            return {};
        }

        if (required == 0) {
            PdhRemoveCounter(tempCounter);
            return {};
        }

        std::vector<BYTE> counterInfoBuffer(required);
        auto counterInfo = (PDH_COUNTER_INFO*)counterInfoBuffer.data();

        hr = PdhGetCounterInfo(tempCounter, FALSE, &required, counterInfo);
        PdhRemoveCounter(tempCounter);
        if (FAILED(hr)) {
            Wh_Log(L"PdhGetCounterInfo error %08X", hr);
            return {};
        }

        required = 0;
        hr = PdhExpandWildCardPath(nullptr, counterInfo->szFullPath, nullptr,
                                   &required, 0);
        if (FAILED(hr) && hr != (PDH_STATUS)PDH_MORE_DATA) {
            Wh_Log(L"PdhExpandWildCardPath error %08X", hr);
            return {};
        }

        if (required == 0) {
            return {};
        }

        std::vector<WCHAR> pathBuffer(required);
        hr = PdhExpandWildCardPath(nullptr, counterInfo->szFullPath,
                                   pathBuffer.data(), &required, 0);
        if (FAILED(hr)) {
            Wh_Log(L"PdhExpandWildCardPath error %08X", hr);
            return {};
        }

        std::vector<std::wstring> paths;
        WCHAR* p = pathBuffer.data();
        while (*p) {
            paths.emplace_back(p);
            p += wcslen(p) + 1;
        }
        return paths;
    }

    static std::wstring_view ExtractInstanceName(std::wstring_view path) {
        auto start = path.find(L'(');
        auto end = path.rfind(L')');
        if (start == std::wstring_view::npos ||
            end == std::wstring_view::npos || end <= start) {
            return {};
        }
        return path.substr(start + 1, end - start - 1);
    }

    static bool ContainsNoCase(std::wstring_view haystack,
                               std::wstring_view needle) {
        if (needle.empty()) {
            return true;
        }

        std::wstring a, b;
        a.reserve(haystack.size());
        b.reserve(needle.size());
        for (wchar_t c : haystack) {
            a.push_back(towlower(c));
        }
        for (wchar_t c : needle) {
            b.push_back(towlower(c));
        }
        return a.find(b) != std::wstring::npos;
    }

    bool IsWantedInstance(const std::wstring& path) {
        auto instance = ExtractInstanceName(path);
        if (instance.empty()) {
            return true;
        }

        // Skip pseudo-adapters which skew the totals.
        if (ContainsNoCase(instance, L"isatap") ||
            ContainsNoCase(instance, L"loopback") ||
            ContainsNoCase(instance, L"teredo")) {
            return false;
        }

        return ContainsNoCase(instance, adapterName_);
    }

    static constexpr int kRebuildIntervalTicks = 600;  // ~10 min at 1s

    PDH_HQUERY query_ = nullptr;
    struct Metric {
        std::vector<std::pair<std::wstring, PDH_HCOUNTER>> counters;
    };
    std::map<MetricType, Metric> metrics_;
    std::wstring adapterName_;
    std::wstring appliedAdapterName_;
    bool valid_ = false;
    int ticksSinceRebuild_ = 0;
};

FormattedMetrics ComputeFormattedMetrics(const SystemMetrics::Sample& sample) {
    FormattedMetrics formatted;
    formatted.up =
        FormatBytesPerSecond(sample.uploadValid ? sample.uploadSum : 0.0);
    formatted.down = FormatBytesPerSecond(sample.downloadValid
                                              ? sample.downloadSum
                                              : 0.0);
    formatted.total =
        FormatBytesPerSecond((sample.uploadValid ? sample.uploadSum : 0.0) +
                             (sample.downloadValid ? sample.downloadSum : 0.0));
    formatted.cpu = FormatPercent(sample.cpuValid ? sample.cpuPercent : 0.0);
    formatted.ram = FormatPercent((double)sample.memoryLoadPercent);
    formatted.ram_used = FormatGb(sample.memoryUsedBytes);
    return formatted;
}

// ---------------------------------------------------------------------------
// Tray instances
// ---------------------------------------------------------------------------

struct TrayMetricsInstance {
    winrt::weak_ref<Controls::Panel> hostPanel;
    winrt::weak_ref<Controls::StackPanel> metricsStack;
    winrt::weak_ref<Controls::TextBlock> textBlock;
    std::wstring lastTooltipText;
    // std::optional because CoreDispatcher projections are not default
    // constructible in the C++/WinRT projection of this build.
    std::optional<winrt::Windows::UI::Core::CoreDispatcher> dispatcher;
    std::atomic<bool> dead{false};
};

std::mutex g_instancesMutex;
std::vector<std::shared_ptr<TrayMetricsInstance>> g_instances;

std::atomic<bool> g_systemTrayModuleHooked;
std::atomic<bool> g_unloading;
std::atomic<bool> g_workerStarted;
HANDLE g_settingsChangedEvent = nullptr;
std::thread g_workerThread;

using FrameworkElementLoadedEventRevoker = winrt::impl::event_revoker<
    IFrameworkElement,
    &winrt::impl::abi<IFrameworkElement>::type::remove_Loaded>;

[[clang::no_destroy]] std::optional<
    std::list<FrameworkElementLoadedEventRevoker>> g_autoRevokerList{
    std::in_place};

// ---------------------------------------------------------------------------
// UI-thread logic
// ---------------------------------------------------------------------------

Controls::TextBlock MapAlignmentTextBlock(const Settings& settings,
                                          Controls::TextBlock textBlock) {
    switch ((MetricsTextAlignment)settings.textAlignment) {
        case MetricsTextAlignment::center:
            textBlock.TextAlignment(TextAlignment::Center);
            break;
        case MetricsTextAlignment::right:
            textBlock.TextAlignment(TextAlignment::End);
            break;
        case MetricsTextAlignment::left:
        default:
            textBlock.TextAlignment(TextAlignment::Left);
            break;
    }

    return textBlock;
}

struct CreatedMetricsStack {
    Controls::StackPanel stack{nullptr};
    Controls::TextBlock textBlock{nullptr};
};

FrameworkElement FindDescendantByName(FrameworkElement element,
                                      PCWSTR name,
                                      int depth);
Controls::TextBlock FindReferenceTextBlock(FrameworkElement scopeRoot,
                                           FrameworkElement exclude);
void ApplyReferenceStyle(Controls::TextBlock textBlock,
                         Controls::TextBlock reference);

CreatedMetricsStack CreateMetricsStack(const Settings& settings,
                                        FrameworkElement trayContext) {
    Controls::StackPanel stack;
    stack.Name(L"TaskbarTrayMetricsStack");
    stack.Orientation(Controls::Orientation::Vertical);
    stack.HorizontalAlignment(HorizontalAlignment::Left);
    stack.VerticalAlignment(VerticalAlignment::Center);
    stack.IsHitTestVisible(false);
    if (settings.fixedWidth > 0) {
        stack.Width((double)settings.fixedWidth);
    }

    // Two-column grid so the right side of each line stays horizontally
    // aligned: the left column auto-sizes to the widest left cell of both
    // rows, so the right side never shifts relative to the other row.
    using GridLength = winrt::Windows::UI::Xaml::GridLength;
    using GridUnitType = winrt::Windows::UI::Xaml::GridUnitType;

    Controls::Grid grid;
    grid.Name(L"MetricsTextGrid");
    try {
        Controls::ColumnDefinition col0, col1;
        col0.Width(GridLength{0.0, GridUnitType::Auto});
        col1.Width(GridLength{1.0, GridUnitType::Star});
        grid.ColumnDefinitions().Append(col0);
        grid.ColumnDefinitions().Append(col1);

        Controls::RowDefinition row0, row1;
        row0.Height(GridLength{0.0, GridUnitType::Auto});
        row1.Height(GridLength{0.0, GridUnitType::Auto});
        grid.RowDefinitions().Append(row0);
        grid.RowDefinitions().Append(row1);
    } catch (...) {
    }

    struct CellDef {
        PCWSTR name;
        int row;
        int column;
    };
    static constexpr CellDef cellDefs[] = {
        {L"MetricsTextBlock", 0, 0},
        {L"MetricsRight1", 0, 1},
        {L"MetricsLeft2", 1, 0},
        {L"MetricsRight2", 1, 1},
    };

    Controls::TextBlock textBlock{nullptr};
    for (const auto& cell : cellDefs) {
        try {
            Controls::TextBlock block;
            block.Name(cell.name);
            block.TextWrapping(TextWrapping::NoWrap);
            block.IsHitTestVisible(false);
            Controls::Grid::SetRow(block, cell.row);
            Controls::Grid::SetColumn(block, cell.column);
            grid.Children().Append(block);
            if (wcscmp(cell.name, L"MetricsTextBlock") == 0) {
                textBlock = block;
            }
        } catch (...) {
        }
    }

    try {
        stack.Children().Append(grid);
    } catch (...) {
    }

    if (!textBlock) {
        LogDiag(L"CreateMetricsStack: grid children missing");
    }

    Controls::TextBlock neighbor = nullptr;
    try {
        neighbor = FindReferenceTextBlock(trayContext, nullptr);
    } catch (...) {
    }

    try {
        if (neighbor) {
            ApplyReferenceStyle(textBlock, neighbor);
        }

        if (!settings.fontFamily.empty()) {
            textBlock.FontFamily(Media::FontFamily(settings.fontFamily));
        }

        if (settings.fontSize > 0) {
            textBlock.FontSize(settings.fontSize);
        }
    } catch (...) {
    }

    CreatedMetricsStack result{};
    result.stack = stack;
    result.textBlock = textBlock;
    return result;
}

// ---------------------------------------------------------------------------
// Cached settings (worker thread copies instead of calling Wh_* APIs)
// ---------------------------------------------------------------------------

std::mutex g_settingsMutex;
Settings g_settings;

void UpdateCachedSettings() {
    Settings settings = ReadSettings();
    std::lock_guard<std::mutex> lock(g_settingsMutex);
    g_settings = std::move(settings);
}

Settings GetCachedSettings() {
    std::lock_guard<std::mutex> lock(g_settingsMutex);
    return g_settings;
}

// ---------------------------------------------------------------------------
// Instance bookkeeping
// ---------------------------------------------------------------------------

bool PanelIdentityMatches(const winrt::weak_ref<Controls::Panel>& panelRef,
                          Controls::Panel other) {
    Controls::Panel left = panelRef.get();
    if (!left || !other) {
        return false;
    }

    return winrt::get_abi(left) == winrt::get_abi(other);
}

void PruneInstances() {
    std::lock_guard<std::mutex> lock(g_instancesMutex);
    if (!g_instances.empty()) {
        std::erase_if(g_instances, [](const auto& instance) {
            return instance->dead.load();
        });
    }
}

void LogInstancesState() {
    std::lock_guard<std::mutex> lock(g_instancesMutex);
    LogDiag(L"state: %zu instance(s)", g_instances.size());
    for (size_t i = 0; i < g_instances.size(); i++) {
        auto& instance = g_instances[i];
        auto host = instance->hostPanel.get();
        auto stack = instance->metricsStack.get();
        int hostOk = host ? 1 : 0;
        int stackOk = 0;
        int indexFound = 0;
        if (host && stack) {
            stackOk = 1;
            try {
                uint32_t idx = 0;
                indexFound = host.Children().IndexOf(stack, idx) ? 1 : 0;
            } catch (...) {
                indexFound = -1;
            }
        }
        LogDiag(L"state[%zu]: host=%d stack=%d inHost=%d dead=%d", i, hostOk,
                stackOk, indexFound, (int)instance->dead.load());
    }
}

std::shared_ptr<TrayMetricsInstance> GetOrCreateInstanceForHost(
    Controls::Panel host) {
    if (g_unloading) {
        return nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(g_instancesMutex);
        for (auto& instance : g_instances) {
            if (PanelIdentityMatches(instance->hostPanel, host)) {
                return instance;
            }
        }
    }

    auto instance = std::make_shared<TrayMetricsInstance>();
    instance->hostPanel = host;

    try {
        instance->dispatcher.emplace(host.Dispatcher());
    } catch (...) {
        return nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(g_instancesMutex);
        g_instances.push_back(instance);
    }

    return instance;
}

// ---------------------------------------------------------------------------
// UI-thread element management
// ---------------------------------------------------------------------------

// The chevron lives in the same stack panel as the tray icons. Position 0
// puts the metrics block before all tray icons (and before the chevron);
// an insert position after the chevron is resolved by scanning the children.
uint32_t ComputeInsertIndex(Controls::Panel host, int insertPosition) {
    if ((InsertPosition)insertPosition != InsertPosition::afterChevron) {
        return 0;
    }

    auto children = host.Children();
    for (uint32_t i = 0; i < children.Size(); ++i) {
        auto child = children.GetAt(i).try_as<FrameworkElement>();
        if (!child) {
            continue;
        }

        try {
            if (winrt::get_class_name(child) == L"SystemTray.ChevronIconView" ||
                FindChildByClassName(child, L"SystemTray.ChevronIconView")) {
                return i + 1;
            }
        } catch (...) {
        }
    }

    return 0;
}

bool IsInsideStack(FrameworkElement child, FrameworkElement ancestor) {
    if (!child || !ancestor) {
        return false;
    }

    FrameworkElement parent = child;
    while (true) {
        try {
            parent = Media::VisualTreeHelper::GetParent(parent)
                         .try_as<FrameworkElement>();
        } catch (...) {
            return false;
        }

        if (!parent) {
            return false;
        }

        if (winrt::get_abi(parent) == winrt::get_abi(ancestor)) {
            return true;
        }
    }
}

Controls::TextBlock FindFirstTextBlockExcluding(FrameworkElement element,
                                                FrameworkElement exclude,
                                                int depth) {
    if (!element || depth <= 0) {
        return nullptr;
    }

    if (auto textBlock = element.try_as<Controls::TextBlock>()) {
        if (!IsInsideStack(textBlock, exclude)) {
            return textBlock;
        }
    }

    int childrenCount = 0;
    try {
        childrenCount = Media::VisualTreeHelper::GetChildrenCount(element);
    } catch (...) {
        return nullptr;
    }

    for (int i = 0; i < childrenCount; i++) {
        auto child = Media::VisualTreeHelper::GetChild(element, i)
                         .try_as<FrameworkElement>();
        if (!child) {
            continue;
        }

        if (auto textBlock =
                FindFirstTextBlockExcluding(child, exclude, depth - 1)) {
            return textBlock;
        }
    }

    return nullptr;
}

// Finds a native reference text block whose font/size/weight/color the
// metrics block should follow. Prefers the clock's "TimeInnerTextBlock",
// which carries the tray's native typography; falls back to the first
// text block that is not part of the metrics block itself.
Controls::TextBlock FindReferenceTextBlock(FrameworkElement scopeRoot,
                                           FrameworkElement exclude) {
    Controls::TextBlock reference = nullptr;

    try {
        if (auto timeBlock = FindDescendantByName(scopeRoot,
                                                  L"TimeInnerTextBlock", 96)
                                 .try_as<Controls::TextBlock>();
            timeBlock && !IsInsideStack(timeBlock, exclude)) {
            reference = timeBlock;
        }
    } catch (...) {
    }

    if (!reference) {
        try {
            reference = FindFirstTextBlockExcluding(scopeRoot, exclude, 96);
        } catch (...) {
        }
    }

    return reference;
}

void ApplyReferenceStyle(Controls::TextBlock textBlock,
                         Controls::TextBlock reference) {
    if (!textBlock || !reference) {
        return;
    }

    try {
        textBlock.FontFamily(reference.FontFamily());
    } catch (...) {
    }

    try {
        textBlock.FontSize(reference.FontSize());
    } catch (...) {
    }

    try {
        textBlock.FontWeight(reference.FontWeight());
    } catch (...) {
    }

    try {
        textBlock.Foreground(reference.Foreground());
    } catch (...) {
    }
}

void EnsureMetricsPosition(Controls::Panel host,
                           Controls::StackPanel metricsStack,
                           const Settings& settings) {
    try {
        uint32_t currentIndex = 0;
        if (!host.Children().IndexOf(metricsStack, currentIndex)) {
            return;
        }

        uint32_t desiredIndex =
            ComputeInsertIndex(host, settings.insertPosition);
        if (desiredIndex == currentIndex) {
            return;
        }

        host.Children().RemoveAt(currentIndex);
        uint32_t insertIndex = ComputeInsertIndex(host, settings.insertPosition);
        host.Children().InsertAt(insertIndex, metricsStack);
    } catch (...) {
    }
}

void EnsureMetricsInserted(std::shared_ptr<TrayMetricsInstance> instance,
                           const Settings& settings) {
    auto host = instance->hostPanel.get();
    if (!host) {
        LogDiag(L"dead: host panel lost");
        instance->dead = true;
        return;
    }

    auto stack = instance->metricsStack.get();
    if (stack) {
        uint32_t index = 0;
        if (host.Children().IndexOf(stack, index)) {
            EnsureMetricsPosition(host, stack, settings);
            return;
        }
        // Element got detached from the panel, re-adopt or re-insert below.
        instance->metricsStack = nullptr;
    }

    bool handled = false;
    try {
        auto adopted = FindChildByName(host, L"TaskbarTrayMetricsStack")
                           .try_as<Controls::StackPanel>();
        if (adopted) {
            auto adoptedText =
                FindChildByName(adopted, L"MetricsTextBlock")
                    .try_as<Controls::TextBlock>();
            instance->metricsStack = adopted;
            instance->textBlock = adoptedText;
            LogDiag(L"adopt: stack=%p text=%d text=%p",
                    (const void*)winrt::get_abi(adopted),
                    adoptedText ? 1 : 0,
                    adoptedText ? (const void*)winrt::get_abi(adoptedText)
                                : nullptr);
            handled = true;
        }
    } catch (...) {
    }

    if (!handled) {
        CreatedMetricsStack created;
        try {
            created = CreateMetricsStack(settings, host);
        } catch (...) {
            LogDiag(L"dead: CreateMetricsStack threw");
            instance->dead = true;
            return;
        }
        if (!created.stack || !created.textBlock) {
            LogDiag(L"dead: CreateMetricsStack incomplete");
            instance->dead = true;
            return;
        }
        instance->metricsStack = created.stack;
        instance->textBlock = created.textBlock;

        uint32_t insertIndex = 0;
        try {
            insertIndex = ComputeInsertIndex(host, settings.insertPosition);
            host.Children().InsertAt(insertIndex, created.stack);
            stack = created.stack;
        } catch (...) {
            LogDiag(L"dead: InsertAt failed");
            instance->dead = true;
            return;
        }

        LogDiag(L"created + inserted stack=%p text=%p index=%u",
                (const void*)winrt::get_abi(created.stack),
                (const void*)winrt::get_abi(created.textBlock), insertIndex);
    }
}

// ---------------------------------------------------------------------------
// Two-column render nodes and TrafficMonitor-style hover tooltip
// ---------------------------------------------------------------------------

struct MetricsRenderNodes {
    Controls::Grid grid{nullptr};
    Controls::TextBlock left1{nullptr};   // "MetricsTextBlock"
    Controls::TextBlock right1{nullptr};  // "MetricsRight1"
    Controls::TextBlock left2{nullptr};   // "MetricsLeft2"
    Controls::TextBlock right2{nullptr};  // "MetricsRight2"
    // True when the stack holds the single pre-grid TextBlock (created by
    // an older run; adopted stacks can look like this).
    bool legacy = false;
};

MetricsRenderNodes FetchRenderNodes(Controls::StackPanel stack) {
    MetricsRenderNodes nodes;
    if (!stack) {
        return nodes;
    }

    UIElement child{nullptr};
    try {
        child = stack.Children().GetAt(0);
    } catch (...) {
        return nodes;
    }

    // The first child is either the grid (new layout) or a single TextBlock
    // (legacy layout). The children collection holds live objects even when
    // our own cached weak references die, so this lookup is robust.
    nodes.left1 = child.try_as<Controls::TextBlock>();
    if (nodes.left1) {
        nodes.legacy = true;
        return nodes;
    }

    nodes.grid = child.try_as<Controls::Grid>();
    if (!nodes.grid) {
        return nodes;
    }

    try {
        nodes.left1 = FindChildElementByName(nodes.grid, L"MetricsTextBlock")
                          .try_as<Controls::TextBlock>();
        nodes.right1 = FindChildElementByName(nodes.grid, L"MetricsRight1")
                           .try_as<Controls::TextBlock>();
        nodes.left2 = FindChildElementByName(nodes.grid, L"MetricsLeft2")
                          .try_as<Controls::TextBlock>();
        nodes.right2 = FindChildElementByName(nodes.grid, L"MetricsRight2")
                           .try_as<Controls::TextBlock>();
    } catch (...) {
    }

    return nodes;
}

// Splits a rendered line into (left, right) columns at the first double
// space. No separator means everything goes to the left column.
std::pair<std::wstring, std::wstring> SplitTwoColumns(
    const std::wstring& text) {
    size_t pos = text.find(L"  ");
    if (pos == std::wstring::npos) {
        return {text, L""};
    }
    return {text.substr(0, pos), text.substr(pos + 2)};
}

// TrafficMonitor-style tooltip: RAM vs. total, CPU and network speeds.
std::wstring BuildTooltipText(const FormattedMetrics& metrics) {
    MEMORYSTATUSEX status{};
    status.dwLength = sizeof(status);

    double usedGB = 0.0;
    double totalGB = 0.0;
    long long usedMB = 0;
    long long totalMB = 0;
    DWORD loadPercent = 0;

    if (GlobalMemoryStatusEx(&status)) {
        usedMB = (long long)((status.ullTotalPhys - status.ullAvailPhys) >> 20);
        totalMB = (long long)(status.ullTotalPhys >> 20);
        usedGB = (double)usedMB / 1024.0;
        totalGB = (double)totalMB / 1024.0;
        loadPercent = status.dwMemoryLoad;
    }

    wchar_t buffer[128];
    if (totalMB > 0) {
        swprintf(buffer, ARRAYSIZE(buffer),
                 L"RAM: %.1f GB / %.1f GB (%lu%%)"
                 L"\r\nCPU: %s   ↓ %s   ↑ %s",
                 usedGB, totalGB, loadPercent,
                 metrics.cpu.c_str(), metrics.down.c_str(),
                 metrics.up.c_str());
    } else {
        swprintf(buffer, ARRAYSIZE(buffer), L"CPU: %s   ↓ %s   ↑ %s",
                 metrics.cpu.c_str(), metrics.down.c_str(),
                 metrics.up.c_str());
    }
    return buffer;
}

void UpdateTooltip(Controls::StackPanel stack,
                   const Settings& settings,
                   const FormattedMetrics& metrics,
                   std::wstring& lastTooltipText) {
    if (!settings.showTooltip) {
        try {
            stack.IsHitTestVisible(false);
        } catch (...) {
        }
        lastTooltipText.clear();
        return;
    }

    namespace Primitives = winrt::Windows::UI::Xaml::Controls::Primitives;

    try {
        // The tooltip needs the element to be hit-testable, so it needs a
        // background brush (a null background is transparent to hit tests).
        stack.IsHitTestVisible(true);
        if (!stack.Background()) {
            stack.Background(
                Media::SolidColorBrush{winrt::Windows::UI::Colors::Transparent()});
        }

        Controls::ToolTip tooltip =
            Controls::ToolTipService::GetToolTip(stack)
                .try_as<Controls::ToolTip>();
        if (!tooltip) {
            tooltip = Controls::ToolTip{};
            try {
                tooltip.Placement(Primitives::PlacementMode::Mouse);
            } catch (...) {
            }
            Controls::ToolTipService::SetToolTip(stack, tooltip);
        }

        std::wstring text = BuildTooltipText(metrics);
        if (text != lastTooltipText) {
            lastTooltipText = text;
            tooltip.Content(winrt::box_value(text));
        }
    } catch (...) {
    }
}

void UpdateInstanceOnUI(std::shared_ptr<TrayMetricsInstance> instance,
                        const FormattedMetrics& metrics,
                        const Settings& settings) {
    EnsureMetricsInserted(instance, settings);

    auto host = instance->hostPanel.get();
    auto stack = instance->metricsStack.get();
    if (!host || !stack) {
        LogDiag(L"dead: host or stack null");
        instance->dead = true;
        return;
    }

    auto nodes = FetchRenderNodes(stack);
    if (!nodes.left1) {
        LogDiag(L"dead: textblock null (stack=%p)",
                (const void*)winrt::get_abi(stack));
        instance->dead = true;
        return;
    }

    // Text style: follow the reference block (clock) so it follows the
    // system theme; explicit settings win.
    Controls::TextBlock reference = nullptr;
    try {
        reference = FindReferenceTextBlock(host, stack);
    } catch (...) {
    }

    Media::Brush foreground = nullptr;
    if (auto color = ParseColor(settings.textColor)) {
        try {
            foreground = Media::SolidColorBrush{*color};
        } catch (...) {
        }
    }
    if (!foreground) {
        try {
            if (reference) {
                foreground = reference.Foreground();
            }
        } catch (...) {
        }
    }

    auto styleOne = [&](Controls::TextBlock block) {
        if (!block) {
            return;
        }
        MapAlignmentTextBlock(settings, block);
        try {
            if (settings.fontSize > 0) {
                block.FontSize(settings.fontSize);
            } else if (reference) {
                block.FontSize(reference.FontSize());
            }
        } catch (...) {
        }
        try {
            if (!settings.fontFamily.empty()) {
                block.FontFamily(Media::FontFamily(settings.fontFamily));
            } else if (reference) {
                block.FontWeight(reference.FontWeight());
                block.FontFamily(reference.FontFamily());
            }
        } catch (...) {
        }
        try {
            if (foreground) {
                block.Foreground(foreground);
            }
        } catch (...) {
        }
    };

    styleOne(nodes.left1);
    styleOne(nodes.right1);
    styleOne(nodes.left2);
    styleOne(nodes.right2);

    // Margins live on the grid in grid mode, and on the text block itself
    // in legacy mode.
    try {
        Thickness margin{(float)settings.marginLeft, 0,
                         (float)settings.marginRight, 0};
        if (nodes.grid) {
            nodes.grid.Margin(margin);
        } else {
            nodes.left1.Margin(margin);
        }
    } catch (...) {
    }

    // Fixed width keeps the taskbar from resizing as the numbers change.
    try {
        if (settings.fixedWidth > 0 &&
            stack.Width() != (double)settings.fixedWidth) {
            stack.Width((double)settings.fixedWidth);
        }
    } catch (...) {
    }

    // Fixed left column width (0 keeps the automatic shared-width column).
    try {
        if (nodes.grid) {
            auto column = nodes.grid.ColumnDefinitions().GetAt(0);
            auto current = column.Width();
            using GridLength = winrt::Windows::UI::Xaml::GridLength;
            using GridUnitType = winrt::Windows::UI::Xaml::GridUnitType;
            if (settings.leftColWidth > 0) {
                GridLength desired{(double)settings.leftColWidth,
                                   GridUnitType::Pixel};
                if (!(current == desired)) {
                    column.Width(desired);
                }
            } else if (current.GridUnitType != GridUnitType::Auto) {
                column.Width(GridLength{0.0, GridUnitType::Auto});
            }
        }
    } catch (...) {
    }

    std::wstring line1;
    std::wstring line2;
    if (!settings.line1Format.empty()) {
        line1 = ApplyFormat(settings.line1Format, metrics);
    }
    if (settings.showSecondLine && !settings.line2Format.empty()) {
        line2 = ApplyFormat(settings.line2Format, metrics);
    }

    bool anyVisible = false;
    try {
        if (nodes.legacy) {
            std::wstring text = line1;
            if (!line2.empty()) {
                if (!text.empty()) {
                    text.push_back(L'\n');
                }
                text += line2;
            }
            nodes.left1.Text(text);
            anyVisible = !text.empty();
        } else {
            auto [left1Text, right1Text] = SplitTwoColumns(line1);
            auto [left2Text, right2Text] = SplitTwoColumns(line2);

            nodes.left1.Text(left1Text);
            auto showRight1 = !right1Text.empty();
            if (nodes.right1) {
                nodes.right1.Text(showRight1 ? right1Text : L"");
                nodes.right1.Visibility(showRight1 ? Visibility::Visible
                                                   : Visibility::Collapsed);
            }

            bool showSecond = !line2.empty() && nodes.left2;
            if (nodes.left2) {
                nodes.left2.Text(left2Text);
                nodes.left2.Visibility(!left2Text.empty()
                                           ? Visibility::Visible
                                           : Visibility::Collapsed);
            }
            bool showRight2 = showSecond && !right2Text.empty();
            if (nodes.right2) {
                nodes.right2.Text(showRight2 ? right2Text : L"");
                nodes.right2.Visibility(showRight2 ? Visibility::Visible
                                                   : Visibility::Collapsed);
            }

            anyVisible = !left1Text.empty() || showRight1 || !left2Text.empty() ||
                         showRight2;
        }

        stack.Visibility(anyVisible ? Visibility::Visible
                                    : Visibility::Collapsed);
    } catch (...) {
        LogDiag(L"dead: Text/Visibility failed");
        instance->dead = true;
        return;
    }

    UpdateTooltip(stack, settings, metrics, instance->lastTooltipText);
}

void RemoveInstanceElements(Controls::Panel host,
                            Controls::StackPanel metricsStack) {
    try {
        uint32_t index = 0;
        if (host.Children().IndexOf(metricsStack, index)) {
            host.Children().RemoveAt(index);
        }
    } catch (...) {
    }
}

void DispatchMetricsToInstances(const FormattedMetrics& metrics) {
    Settings settings = GetCachedSettings();

    std::vector<std::shared_ptr<TrayMetricsInstance>> instances;
    {
        std::lock_guard<std::mutex> lock(g_instancesMutex);
        for (auto& instance : g_instances) {
            if (!instance->dead) {
                instances.push_back(instance);
            }
        }
    }

    for (auto& instance : instances) {
        if (!instance->dispatcher || instance->dead) {
            LogDiag(L"dispatch: skipping instance (dispatcher=%d dead=%d)",
                    instance->dispatcher.has_value() ? 1 : 0,
                    (int)instance->dead.load());
            instance->dead = true;
            continue;
        }

        try {
            instance->dispatcher->RunAsync(
                winrt::Windows::UI::Core::CoreDispatcherPriority::Normal,
                [instance, metrics, settings]() {
                    UpdateInstanceOnUI(instance, metrics, settings);
                });
        } catch (winrt::hresult_error const& e) {
            Wh_Log(L"Failed to dispatch metrics: %08X", e.code().value);
            LogDiag(L"dead: dispatch hresult %08X", e.code().value);
            instance->dead = true;
        } catch (...) {
            LogDiag(L"dead: dispatch unknown failure");
            instance->dead = true;
        }
    }
}

// ---------------------------------------------------------------------------
// Icon view loaded anchor
// ---------------------------------------------------------------------------

void EnsureWorkerStarted();

FrameworkElement FindAncestorByName(FrameworkElement element, PCWSTR name);
FrameworkElement FindDescendantByName(FrameworkElement element,
                                      PCWSTR name,
                                      int depth);

void OnIconViewLoaded(winrt::Windows::Foundation::IInspectable const& sender,
                      RoutedEventArgs const& e) {
    if (g_unloading) {
        return;
    }

    auto iconView = sender.try_as<FrameworkElement>();
    if (!iconView) {
        return;
    }

    Controls::Panel host = nullptr;
    try {
        auto className = winrt::get_class_name(iconView);

        bool isChevron = className == L"SystemTray.ChevronIconView";
        bool isTrayIcon =
            className == L"SystemTray.IconView" &&
            iconView.Name() == L"SystemTrayIcon" &&
            IsChildOfElementByName(iconView, L"MainStack");

        if (!isChevron && !isTrayIcon) {
            LogDiag(L"OnIconViewLoaded: skip (class %ls)", className.c_str());
            return;
        }

        host = FindAncestorByName(iconView, L"SystemTrayFrameGrid")
                   .try_as<Controls::Panel>();
        if (!host) {
            host = GetFirstPanelAncestor(iconView);
        }
        if (!host) {
            LogDiag(L"OnIconViewLoaded: no panel ancestor");
            return;
        }

        // Ignore icon views outside the main tray area (e.g. the overflow
        // popup window).
        if (!IsInMainTray(host)) {
            LogDiag(L"OnIconViewLoaded: %ls not in main tray", className.c_str());
            return;
        }
    } catch (...) {
    }

    if (!host) {
        return;
    }

    auto instance = GetOrCreateInstanceForHost(host);
    if (!instance) {
        return;
    }

    try {
        UpdateInstanceOnUI(instance, FormattedMetrics{}, GetCachedSettings());
    } catch (...) {
    }

    EnsureWorkerStarted();
}

// ---------------------------------------------------------------------------
// Worker thread (samples performance counters and dispatches to the UI)
// ---------------------------------------------------------------------------

void ApplyOnTaskbarThread(bool cleanup);

DWORD WINAPI WorkerThreadProc(void* param) {
    (void)param;

    LogDiag(L"WorkerThreadProc: start");

    try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
    } catch (...) {
    }

    SystemMetrics metrics;
    unsigned long long sampleFailCount = 0;
    unsigned int applyRetryCount = 0;
    unsigned long long tick = 0;

    while (!g_unloading) {
        metrics.SetAdapterName(GetCachedSettings().networkAdapterName);

        if (auto sample = metrics.SampleOnce()) {
            if (sampleFailCount != 0) {
                LogDiag(L"SampleOnce recovered after %llu fails",
                        sampleFailCount);
            }
            sampleFailCount = 0;

            auto formatted = ComputeFormattedMetrics(*sample);
            DispatchMetricsToInstances(formatted);
        } else {
            sampleFailCount++;
            if (sampleFailCount == 1 ||
                sampleFailCount == 10 || sampleFailCount == 60) {
                LogDiag(L"SampleOnce failed (tick %llu)", sampleFailCount);
            }
        }

        PruneInstances();

        if (++tick % 15 == 0) {
            LogInstancesState();
        }

        // Self-heal: if no instance exists (e.g. the taskbar was created
        // after the mod, or an anchor search failed), retry on the taskbar
        // thread periodically.
        if (!g_unloading) {
            bool haveInstances;
            {
                std::lock_guard<std::mutex> lock(g_instancesMutex);
                haveInstances = !g_instances.empty();
            }
            if (!haveInstances && ++applyRetryCount % 5 == 1) {
                ApplyOnTaskbarThread(false);
            }
        }

        DWORD waitMs = (DWORD)GetCachedSettings().updateIntervalMs;
        if (g_settingsChangedEvent) {
            WaitForSingleObject(g_settingsChangedEvent, waitMs);
        } else {
            Sleep(waitMs);
        }
    }

    LogDiag(L"WorkerThreadProc: exit");

    return 0;
}

void EnsureWorkerStarted() {
    bool expected = false;
    if (!g_workerStarted.compare_exchange_strong(expected, true)) {
        return;
    }

    if (!g_settingsChangedEvent) {
        g_settingsChangedEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!g_settingsChangedEvent) {
            g_workerStarted = false;
            return;
        }
    }

    try {
        g_workerThread = std::thread(WorkerThreadProc, nullptr);
    } catch (...) {
        Wh_Log(L"Failed to start worker thread");
        g_workerStarted = false;
    }
}

// ---------------------------------------------------------------------------
// Taskbar thread helpers (for late loads and cleanup)
// (Tray system icon tweaks by m417z, MIT license)
// ---------------------------------------------------------------------------

void* CTaskBand_ITaskListWndSite_vftable;

using CTaskBand_GetTaskbarHost_t = void*(WINAPI*)(void* pThis, void** result);
CTaskBand_GetTaskbarHost_t CTaskBand_GetTaskbarHost_Original;

void* TaskbarHost_FrameHeight_Original;

using std__Ref_count_base__Decref_t = void(WINAPI*)(void* pThis);
std__Ref_count_base__Decref_t std__Ref_count_base__Decref_Original;

XamlRoot GetTaskbarXamlRoot(HWND hTaskbarWnd) {
    if (!CTaskBand_ITaskListWndSite_vftable ||
        !CTaskBand_GetTaskbarHost_Original ||
        !TaskbarHost_FrameHeight_Original) {
        return nullptr;
    }

    HWND hTaskSwWnd = (HWND)GetProp(hTaskbarWnd, L"TaskbandHWND");
    if (!hTaskSwWnd) {
        return nullptr;
    }

    void* taskBand = (void*)GetWindowLongPtr(hTaskSwWnd, 0);
    void* taskBandForTaskListWndSite = taskBand;
    for (int i = 0; *(void**)taskBandForTaskListWndSite !=
                    CTaskBand_ITaskListWndSite_vftable;
         i++) {
        if (i == 20) {
            return nullptr;
        }

        taskBandForTaskListWndSite = (void**)taskBandForTaskListWndSite + 1;
    }

    void* taskbarHostSharedPtr[2]{};
    CTaskBand_GetTaskbarHost_Original(taskBandForTaskListWndSite,
                                      taskbarHostSharedPtr);
    if (!taskbarHostSharedPtr[0] || !taskbarHostSharedPtr[1]) {
        return nullptr;
    }

    size_t taskbarElementIUnknownOffset = 0x10;

#if defined(_M_X64)
    {
        // 48:83EC 28 | sub rsp,28
        // 48:83C1 48 | add rcx,48
        const BYTE* b = (const BYTE*)TaskbarHost_FrameHeight_Original;
        if (b[0] == 0x48 && b[1] == 0x83 && b[2] == 0xEC && b[4] == 0x48 &&
            b[5] == 0x83 && b[6] == 0xC1 && b[7] <= 0x7F) {
            taskbarElementIUnknownOffset = b[7];
        } else {
            Wh_Log(L"Unsupported TaskbarHost::FrameHeight");
        }
    }
#elif defined(_M_ARM64)
    {
        // 7f2303d5 pacibsp
        // fd7bbfa9 stp     fp, lr, [sp, #-0x10]!
        // fd030091 mov     fp, sp
        // 080c41f8 ldr     x8, [x0, #0x10]!
        const DWORD* p = (const DWORD*)TaskbarHost_FrameHeight_Original;
        if (p[0] == 0xD503237F && (p[1] & 0xFFC07FFF) == 0xA9807BFD &&
            p[2] == 0x910003FD && (p[3] & 0xFFF00FE0) == 0xF8400C00) {
            taskbarElementIUnknownOffset = (p[3] >> 12) & 0xFF;
        } else {
            Wh_Log(L"Unsupported TaskbarHost::FrameHeight");
        }
    }
#else
#error "Unsupported architecture"
#endif

    auto* taskbarElementIUnknown =
        *(IUnknown**)((BYTE*)taskbarHostSharedPtr[0] +
                      taskbarElementIUnknownOffset);

    FrameworkElement taskbarElement = nullptr;
    taskbarElementIUnknown->QueryInterface(winrt::guid_of<FrameworkElement>(),
                                           winrt::put_abi(taskbarElement));

    auto result = taskbarElement ? taskbarElement.XamlRoot() : nullptr;

    if (std__Ref_count_base__Decref_Original) {
        std__Ref_count_base__Decref_Original(taskbarHostSharedPtr[1]);
    }

    return result;
}

using RunFromWindowThreadProc_t = void(WINAPI*)(void* parameter);

static UINT g_runFromWindowThreadRegisteredMsg = 0;

struct RUN_FROM_WINDOW_THREAD_PARAM {
    RunFromWindowThreadProc_t proc;
    void* procParam;
};

LRESULT WINAPI RunFromWindowThreadCallback(int nCode,
                                           WPARAM wParam,
                                           LPARAM lParam) {
    if (nCode == HC_ACTION) {
        const CWPSTRUCT* cwp = (const CWPSTRUCT*)lParam;
        if (cwp->message == g_runFromWindowThreadRegisteredMsg) {
            RUN_FROM_WINDOW_THREAD_PARAM* param =
                (RUN_FROM_WINDOW_THREAD_PARAM*)cwp->lParam;
            param->proc(param->procParam);
        }
    }

    return CallNextHookEx(nullptr, nCode, wParam, lParam);
}

bool RunFromWindowThread(HWND hWnd,
                         RunFromWindowThreadProc_t proc,
                         void* procParam) {
    static const UINT runFromWindowThreadRegisteredMsg =
        RegisterWindowMessage(L"Windhawk_RunFromWindowThread_" WH_MOD_ID);
    g_runFromWindowThreadRegisteredMsg = runFromWindowThreadRegisteredMsg;

    DWORD dwThreadId = GetWindowThreadProcessId(hWnd, nullptr);
    if (dwThreadId == 0) {
        return false;
    }

    if (dwThreadId == GetCurrentThreadId()) {
        proc(procParam);
        return true;
    }

    HHOOK hook = SetWindowsHookEx(WH_CALLWNDPROC,
                                  RunFromWindowThreadCallback, nullptr,
                                  dwThreadId);
    if (!hook) {
        return false;
    }

    RUN_FROM_WINDOW_THREAD_PARAM param;
    param.proc = proc;
    param.procParam = procParam;
    SendMessage(hWnd, runFromWindowThreadRegisteredMsg, 0, (LPARAM)&param);

    UnhookWindowsHookEx(hook);

    return true;
}

BOOL WINAPI EnumWindowsCallback(HWND hWnd, LPARAM lParam);

HWND FindCurrentProcessTaskbarWnd() {
    HWND hTaskbarWnd = nullptr;

    EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&hTaskbarWnd));

    return hTaskbarWnd;
}

BOOL WINAPI EnumWindowsCallback(HWND hWnd, LPARAM lParam) {
    DWORD dwProcessId;
    WCHAR className[32];
    if (GetWindowThreadProcessId(hWnd, &dwProcessId) &&
        dwProcessId == GetCurrentProcessId() &&
        GetClassName(hWnd, className, ARRAYSIZE(className)) &&
        _wcsicmp(className, L"Shell_TrayWnd") == 0) {
        *reinterpret_cast<HWND*>(lParam) = hWnd;
        return FALSE;
    }
    return TRUE;
}

// Recursively searches for the first descendant whose Name equals `name`.
FrameworkElement FindDescendantByName(FrameworkElement element,
                                      PCWSTR name,
                                      int depth) {
    if (depth <= 0) {
        return nullptr;
    }

    int childrenCount = 0;
    try {
        childrenCount = Media::VisualTreeHelper::GetChildrenCount(element);
    } catch (...) {
        return nullptr;
    }

    for (int i = 0; i < childrenCount; i++) {
        auto child = Media::VisualTreeHelper::GetChild(element, i)
                         .try_as<FrameworkElement>();
        if (!child) {
            continue;
        }

        try {
            if (child.Name() == name) {
                return child;
            }
        } catch (...) {
        }

        if (auto found = FindDescendantByName(child, name, depth - 1)) {
            return found;
        }
    }

    return nullptr;
}

// Recursively searches for the first descendant whose runtime class name
// equals `className`.
FrameworkElement FindDescendantByClassName(FrameworkElement element,
                                           PCWSTR className,
                                           int depth) {
    if (depth <= 0) {
        return nullptr;
    }

    int childrenCount = 0;
    try {
        childrenCount = Media::VisualTreeHelper::GetChildrenCount(element);
    } catch (...) {
        return nullptr;
    }

    for (int i = 0; i < childrenCount; i++) {
        auto child = Media::VisualTreeHelper::GetChild(element, i)
                         .try_as<FrameworkElement>();
        if (!child) {
            continue;
        }

        try {
            if (winrt::get_class_name(child) == className) {
                return child;
            }
        } catch (...) {
        }

        if (auto found = FindDescendantByClassName(child, className, depth - 1)) {
            return found;
        }
    }

    return nullptr;
}

// Walks up the visual tree looking for an ancestor with the given name.
FrameworkElement FindAncestorByName(FrameworkElement element, PCWSTR name) {
    FrameworkElement parent = element;

    while (true) {
        try {
            parent = Media::VisualTreeHelper::GetParent(parent)
                         .try_as<FrameworkElement>();
        } catch (...) {
            return nullptr;
        }

        if (!parent) {
            return nullptr;
        }

        try {
            if (parent.Name() == name) {
                return parent;
            }
        } catch (...) {
        }
    }
}

void DumpElementTree(FrameworkElement element,
                     int depth,
                     int indent,
                     unsigned int& count) {
    const unsigned int kMaxDumpEntries = 4000;

    if (!element || depth <= 0 || count >= kMaxDumpEntries) {
        return;
    }

    count++;

    wchar_t className[100] = L"?";
    wchar_t name[100] = L"";
    try {
        auto n = winrt::get_class_name(element);
        lstrcpynW(className, n.c_str(), ARRAYSIZE(className));
    } catch (...) {
    }
    try {
        auto n = element.Name();
        lstrcpynW(name, n.c_str(), ARRAYSIZE(name));
        if (name[0]) {
            LogDiag(L"tree%*s%s (%s)", indent, L"", className, name);
        } else {
            LogDiag(L"tree%*s%s", indent, L"", className);
        }
    } catch (...) {
        LogDiag(L"tree%*s%s", indent, L"", className);
    }

    if (count >= kMaxDumpEntries) {
        return;
    }

    int childrenCount = 0;
    try {
        childrenCount = Media::VisualTreeHelper::GetChildrenCount(element);
    } catch (...) {
        return;
    }

    for (int i = 0;
         i < childrenCount && count < kMaxDumpEntries;
         i++) {
        FrameworkElement child = nullptr;
        try {
            child = Media::VisualTreeHelper::GetChild(element, i)
                        .try_as<FrameworkElement>();
        } catch (...) {
        }

        if (child) {
            DumpElementTree(child, depth - 1, indent + 1, count);
        } else {
            count++;
        }
    }
}

void DumpTaskbarTree(FrameworkElement root, int depth) {
    LogDiag(L"DumpTaskbarTree begin");
    unsigned int count = 0;
    DumpElementTree(root, depth, 0, count);
    LogDiag(L"DumpTaskbarTree end (%u entries)", count);
}

// Recursively searches for the first anchor element: a tray icon view or the
// chevron which lives within the main tray stack.
FrameworkElement FindMainStackAnchor(FrameworkElement element, int depth) {
    if (depth <= 0) {
        return nullptr;
    }

    int childrenCount = Media::VisualTreeHelper::GetChildrenCount(element);
    for (int i = 0; i < childrenCount; i++) {
        auto child = Media::VisualTreeHelper::GetChild(element, i)
                         .try_as<FrameworkElement>();
        if (!child) {
            continue;
        }

        try {
            auto className = winrt::get_class_name(child);
            if (className == L"SystemTray.ChevronIconView" ||
                (className == L"SystemTray.IconView" &&
                 child.Name() == L"SystemTrayIcon" &&
                 IsChildOfElementByName(child, L"MainStack"))) {
                if (IsChildOfElementByName(child, L"MainStack")) {
                    return child;
                }
            }
        } catch (...) {
        }

        if (auto found = FindMainStackAnchor(child, depth - 1)) {
            return found;
        }
    }

    return nullptr;
}

struct TaskbarThreadParam {
    HWND hTaskbarWnd;
    bool cleanup;
};

void WINAPI TaskbarThreadProc(void* pRawParam) {
    auto* param = (TaskbarThreadParam*)pRawParam;

    if (param->cleanup) {
        // Runs on the taskbar UI thread. Revoke Loaded handlers and remove
        // the inserted elements.
        if (g_autoRevokerList) {
            for (auto& revoker : *g_autoRevokerList) {
                try {
                    revoker.revoke();
                } catch (...) {
                }
            }

            if (g_unloading) {
                g_autoRevokerList.reset();
            } else {
                g_autoRevokerList->clear();
            }
        }

        std::vector<std::shared_ptr<TrayMetricsInstance>> instances;
        {
            std::lock_guard<std::mutex> lock(g_instancesMutex);
            instances = std::move(g_instances);
            g_instances.clear();
        }

        for (auto& instance : instances) {
            auto host = instance->hostPanel.get();
            auto stack = instance->metricsStack.get();
            if (host && stack) {
                RemoveInstanceElements(host, stack);
            }
        }

        return;
    }

    if (g_unloading) {
        return;
    }

    XamlRoot xamlRoot = GetTaskbarXamlRoot(param->hTaskbarWnd);
    if (!xamlRoot) {
        Wh_Log(L"Getting taskbar XamlRoot failed");
        LogDiag(L"TaskbarThreadProc: XamlRoot null");
        return;
    }

    FrameworkElement contentRoot = nullptr;
    try {
        contentRoot = xamlRoot.Content().as<FrameworkElement>();
    } catch (...) {
        LogDiag(L"TaskbarThreadProc: xamlRoot.Content() failed");
    }

    // Host panel resolution. On current Windows builds (2611x+/26H2) the tray
    // sections (chevron stack, icon stacks, control center button, ...) are
    // direct children of a horizontal StackPanel named SystemTrayFrameGrid,
    // which is the ideal insertion container. On older builds the tray icons
    // and the chevron live inside MainStack, and the anchor search finds them.
    Controls::Panel hostPanel = nullptr;

    // Preferred on current builds (26H2+): the tray section stack. Inserting
    // into the items panel of a stack list gets evicted when the system
    // rebuilds the tray items, so prefer the stable section container.
    try {
        if (auto frameGrid =
                FindDescendantByName(contentRoot, L"SystemTrayFrameGrid", 64)
                    .try_as<Controls::StackPanel>()) {
            hostPanel = frameGrid;
            LogDiag(L"TaskbarThreadProc: SystemTrayFrameGrid host");
        }
    } catch (...) {
        LogDiag(L"TaskbarThreadProc: FrameGrid search threw");
    }

    // Fallback for older builds: an anchor inside MainStack.
    if (!hostPanel) {
        try {
            if (auto anchor = FindMainStackAnchor(contentRoot, 96)) {
                hostPanel = GetFirstPanelAncestor(anchor);
                if (hostPanel) {
                    LogDiag(L"TaskbarThreadProc: anchor found (MainStack path)");
                }
            }
        } catch (...) {
            LogDiag(L"TaskbarThreadProc: FindMainStackAnchor threw");
        }
    }

    if (!hostPanel && contentRoot) {
        LogDiag(L"TaskbarThreadProc: no host, dumping tree");
        DumpTaskbarTree(contentRoot, 96);
    }

    if (!hostPanel || !IsInMainTray(hostPanel)) {
        Wh_Log(L"Failed to find the main tray panel");
        LogDiag(L"TaskbarThreadProc: host panel invalid");
        return;
    }

    LogDiag(L"TaskbarThreadProc: inserting into host");
    auto instance = GetOrCreateInstanceForHost(hostPanel);
    if (!instance) {
        LogDiag(L"TaskbarThreadProc: GetOrCreateInstanceForHost failed");
        return;
    }

    try {
        UpdateInstanceOnUI(instance, FormattedMetrics{}, GetCachedSettings());
    } catch (...) {
        LogDiag(L"TaskbarThreadProc: UpdateInstanceOnUI threw");
    }

    EnsureWorkerStarted();
}

void ApplyOnTaskbarThread(bool cleanup) {
    HWND hTaskbarWnd = FindCurrentProcessTaskbarWnd();
    if (!hTaskbarWnd) {
        Wh_Log(L"No taskbar found");
        LogDiag(L"ApplyOnTaskbarThread(%d): no taskbar found", (int)cleanup);
        return;
    }

    LogDiag(L"ApplyOnTaskbarThread(%d): dispatching", (int)cleanup);

    TaskbarThreadParam param{
        .hTaskbarWnd = hTaskbarWnd,
        .cleanup = cleanup,
    };

    RunFromWindowThread(hTaskbarWnd, TaskbarThreadProc, &param);
}

// ---------------------------------------------------------------------------
// Symbols / hooks
// ---------------------------------------------------------------------------

using IconView_IconView_t = void*(WINAPI*)(void* pThis);
IconView_IconView_t IconView_IconView_Original;
void* WINAPI IconView_IconView_Hook(void* pThis) {
    Wh_Log(L">");

    void* ret = IconView_IconView_Original(pThis);

    // A new icon view has nothing to restore, and the revoker list is
    // released while unloading.
    if (g_unloading) {
        return ret;
    }

    FrameworkElement iconView = nullptr;
    ((IUnknown**)pThis)[1]->QueryInterface(winrt::guid_of<FrameworkElement>(),
                                           winrt::put_abi(iconView));
    if (!iconView) {
        return ret;
    }

    if (!g_autoRevokerList) {
        return ret;
    }

    g_autoRevokerList->emplace_back();
    auto autoRevokerIt = g_autoRevokerList->end();
    --autoRevokerIt;

    try {
        *autoRevokerIt =
            iconView.Loaded(winrt::auto_revoke_t{},
                            [autoRevokerIt](
                                winrt::Windows::Foundation::IInspectable const&
                                    sender,
                                RoutedEventArgs const& e) {
                                g_autoRevokerList->erase(autoRevokerIt);

                                OnIconViewLoaded(sender, e);
                            });
    } catch (...) {
        g_autoRevokerList->erase(autoRevokerIt);
    }

    return ret;
}

bool HookSystemTraySymbols(HMODULE module) {
    // SystemTray.dll, Taskbar.View.dll
    WindhawkUtils::SYMBOL_HOOK symbolHooks[] = {
        {
            {LR"(public: __cdecl winrt::SystemTray::implementation::IconView::IconView(void))"},
            &IconView_IconView_Original,
            IconView_IconView_Hook,
        },
    };

    return HookSymbols(module, symbolHooks, ARRAYSIZE(symbolHooks));
}

bool HookTaskbarDllSymbols() {
    HMODULE module =
        LoadLibraryEx(L"taskbar.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!module) {
        Wh_Log(L"Failed to load taskbar.dll");
        return false;
    }

    WindhawkUtils::SYMBOL_HOOK taskbarDllHooks[] = {
        {
            {LR"(const CTaskBand::`vftable'{for `ITaskListWndSite'})"},
            &CTaskBand_ITaskListWndSite_vftable,
        },
        {
            {LR"(public: virtual class std::shared_ptr<class TaskbarHost> __cdecl CTaskBand::GetTaskbarHost(void)const )"},
            &CTaskBand_GetTaskbarHost_Original,
        },
        {
            {LR"(public: int __cdecl TaskbarHost::FrameHeight(void)const )"},
            &TaskbarHost_FrameHeight_Original,
        },
        {
            {LR"(public: void __cdecl std::_Ref_count_base::_Decref(void))"},
            &std__Ref_count_base__Decref_Original,
        },
    };

    return HookSymbols(module, taskbarDllHooks, ARRAYSIZE(taskbarDllHooks));
}

VS_FIXEDFILEINFO* GetModuleVersionInfo(HMODULE hModule, UINT* puPtrLen) {
    void* pFixedFileInfo = nullptr;
    UINT uPtrLen = 0;

    HRSRC hResource =
        FindResource(hModule, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
    if (hResource) {
        HGLOBAL hGlobal = LoadResource(hModule, hResource);
        if (hGlobal) {
            void* pData = LockResource(hGlobal);
            if (pData) {
                if (!VerQueryValue(pData, L"\\", &pFixedFileInfo, &uPtrLen) ||
                    uPtrLen == 0) {
                    pFixedFileInfo = nullptr;
                    uPtrLen = 0;
                }
            }
        }
    }

    if (puPtrLen) {
        *puPtrLen = uPtrLen;
    }

    return (VS_FIXEDFILEINFO*)pFixedFileInfo;
}

HMODULE GetSystemTrayModuleHandle() {
    HMODULE module = GetModuleHandle(L"SystemTray.dll");
    if (!module) {
        module = GetModuleHandle(L"Taskbar.View.dll");
        if (module) {
            // Starting with Taskbar.View.dll 2604.8002.200.6000, the
            // SystemTray types moved out of Taskbar.View.dll into
            // SystemTray.dll, so don't hook Taskbar.View.dll at this version
            // and above.
            VS_FIXEDFILEINFO* fixedFileInfo =
                GetModuleVersionInfo(module, nullptr);
            WORD moduleMajor =
                fixedFileInfo ? HIWORD(fixedFileInfo->dwFileVersionMS) : 0;
            if (!moduleMajor || moduleMajor >= 2604) {
                Wh_Log(L"Skipping Taskbar.View.dll version %d", moduleMajor);
                module = nullptr;
            }
        }
    }
    if (!module) {
        module = GetModuleHandle(L"ExplorerExtensions.dll");
    }

    return module;
}

void HandleLoadedModuleIfSystemTray(HMODULE module, LPCWSTR lpLibFileName) {
    if (!g_systemTrayModuleHooked && GetSystemTrayModuleHandle() == module &&
        !g_systemTrayModuleHooked.exchange(true)) {
        Wh_Log(L"Loaded %s", lpLibFileName);

        if (HookSystemTraySymbols(module)) {
            Wh_ApplyHookOperations();
        }
    }
}

using LoadLibraryExW_t = decltype(&LoadLibraryExW);
LoadLibraryExW_t LoadLibraryExW_Original;
HMODULE WINAPI LoadLibraryExW_Hook(LPCWSTR lpLibFileName,
                                   HANDLE hFile,
                                   DWORD dwFlags) {
    HMODULE module = LoadLibraryExW_Original(lpLibFileName, hFile, dwFlags);
    if (module) {
        HandleLoadedModuleIfSystemTray(module, lpLibFileName);
    }

    return module;
}

// ---------------------------------------------------------------------------
// Windhawk entry points
// ---------------------------------------------------------------------------

BOOL Wh_ModInit() {
    Wh_Log(L">");

    LogDiagReset();
    LogDiag(L"Wh_ModInit: begin");

    UpdateCachedSettings();

    if (HMODULE systemTrayModule = GetSystemTrayModuleHandle()) {
        LogDiag(L"Wh_ModInit: system tray module already loaded");
        if (HookSystemTraySymbols(systemTrayModule)) {
            g_systemTrayModuleHooked = true;
            Wh_ApplyHookOperations();
            LogDiag(L"Wh_ModInit: HookSystemTraySymbols ok");
        } else {
            // Non-fatal: the icon view hook is optional, the taskbar thread
            // anchor path can work without it.
            LogDiag(L"Wh_ModInit: HookSystemTraySymbols FAILED (continuing)");
        }
    } else {
        LogDiag(L"Wh_ModInit: system tray module not loaded yet");

        HMODULE kernelBaseModule = GetModuleHandle(L"kernelbase.dll");
        auto pKernelBaseLoadLibraryExW =
            (decltype(&LoadLibraryExW))GetProcAddress(kernelBaseModule,
                                                      "LoadLibraryExW");
        if (!WindhawkUtils::SetFunctionHook(
                pKernelBaseLoadLibraryExW, LoadLibraryExW_Hook,
                &LoadLibraryExW_Original)) {
            LogDiag(L"Wh_ModInit: LoadLibraryExW hook FAILED (stopping)");
            return FALSE;
        }
    }

    if (!HookTaskbarDllSymbols()) {
        // Non-fatal: XamlRoot access will just fail; retries keep the crash
        // surface minimal while the icon view hook may still work.
        LogDiag(L"Wh_ModInit: HookTaskbarDllSymbols FAILED (continuing)");
    }

    LogDiag(L"Wh_ModInit: done");

    return TRUE;
}

void Wh_ModAfterInit() {
    Wh_Log(L">");

    if (!g_systemTrayModuleHooked) {
        if (HMODULE systemTrayModule = GetSystemTrayModuleHandle()) {
            if (!g_systemTrayModuleHooked.exchange(true)) {
                Wh_Log(L"Got system tray module");
                LogDiag(L"Wh_ModAfterInit: got system tray module");

                if (HookSystemTraySymbols(systemTrayModule)) {
                    Wh_ApplyHookOperations();
                }
            }
        }
    }

    LogDiag(L"Wh_ModAfterInit: applying");
    ApplyOnTaskbarThread(false);
}

void Wh_ModBeforeUninit() {
    Wh_Log(L">");

    g_unloading = true;

    if (g_settingsChangedEvent) {
        SetEvent(g_settingsChangedEvent);
    }

    if (g_workerThread.joinable()) {
        g_workerThread.join();
    }

    ApplyOnTaskbarThread(true);
}

void Wh_ModUninit() {
    Wh_Log(L">");

    if (g_settingsChangedEvent) {
        CloseHandle(g_settingsChangedEvent);
        g_settingsChangedEvent = nullptr;
    }
}

void Wh_ModSettingsChanged() {
    Wh_Log(L">");

    UpdateCachedSettings();

    if (g_settingsChangedEvent) {
        SetEvent(g_settingsChangedEvent);
    }

    ApplyOnTaskbarThread(false);
}
