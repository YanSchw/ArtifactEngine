#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "Common/Array.h"
#include "Common/Types.h"
#include "Common/UUID.h"
#include "EditorCache.gen.h"

class EditorWindow;

struct EditorWindowPlacement {
    ARTIFACT_STRUCT();

    PROPERTY()
    String Monitor;

    /** Tells apart monitors of the same model, which share a name. */
    PROPERTY()
    Vec2 MonitorPosition = Vec2(0.0f);

    /** The restored (not maximized) rect, positioned relative to the monitor. */
    PROPERTY()
    Vec2 Offset = Vec2(0.0f);

    PROPERTY()
    Vec2 Size = Vec2(0.0f);

    PROPERTY()
    bool Maximized = false;
};

struct EditorTabRecord {
    ARTIFACT_STRUCT();

    PROPERTY()
    String TabClass;

    PROPERTY()
    UUID AssetId;
};

struct EditorWindowRecord {
    ARTIFACT_STRUCT();

    PROPERTY()
    EditorWindowPlacement Placement;

    PROPERTY()
    Array<EditorTabRecord> Tabs;

    PROPERTY()
    int32_t ActiveTab = 0;
};

/** The project's last editor session: every editor window with its placement and open tabs.
 *  Kept in the project's Saved directory, which stays out of version control. */
class EditorCache : public Object {
public:
    ARTIFACT_CLASS();

    static EditorCache& Get();
    static String GetFilePath();

    PROPERTY()
    Array<EditorWindowRecord> Windows;

    void Load();
    void Save() const;
    void Capture();

    /** The main editor window, placed where the first window of the last session was. */
    SharedObjectPtr<EditorWindow> CreateMainWindow() const;
    /** Reopens the last session's tabs, opening further windows for the ones that held tabs too.
     *  False when no tab could be reopened in the main window. */
    bool ReopenTabs(EditorWindow& InMainWindow) const;
};
