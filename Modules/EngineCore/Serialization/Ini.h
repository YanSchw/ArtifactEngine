#pragma once
#include "Common/String.h"
#include "Common/Array.h"

class IniFile {
public:
    struct Entry {
        String Section;
        String Key;
        String Value;
    };

    static IniFile Parse(const String& InText);
    String ToString() const;

    Array<Entry> GetEntries() const;
    const String* Find(const String& InSection, const String& InKey) const;
    void Set(const String& InSection, const String& InKey, const String& InValue);
    bool Remove(const String& InSection, const String& InKey);
    bool IsEmpty() const;

private:
    struct Line {
        String Text;
        String Section;
        String Key;
        String Value;
        bool IsHeader = false;

        bool IsEntry() const { return !IsHeader && !Key.empty(); }
        bool IsBlank() const;
    };

    int32_t FindEntry(const String& InSection, const String& InKey) const;
    int32_t FindHeader(const String& InSection) const;
    int32_t FindSectionEnd(int32_t InHeader) const;
    void PruneSection(const String& InSection);

    Array<Line> m_Lines;
};
