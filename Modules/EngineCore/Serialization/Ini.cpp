#include "Ini.h"

namespace {
    String Trim(const String& InString) {
        size_t first = InString.find_first_not_of(" \t\r\n");
        if (first == String::npos) {
            return "";
        }
        size_t last = InString.find_last_not_of(" \t\r\n");
        return InString.substr(first, last - first + 1);
    }
}

bool IniFile::Line::IsBlank() const {
    return Trim(Text).empty();
}

IniFile IniFile::Parse(const String& InText) {
    IniFile file;
    String section;
    size_t start = 0;
    while (start < InText.size()) {
        size_t end = InText.find('\n', start);
        if (end == String::npos) {
            end = InText.size();
        }
        Line line;
        line.Text = InText.substr(start, end - start);
        if (!line.Text.empty() && line.Text.back() == '\r') {
            line.Text.pop_back();
        }
        start = end + 1;

        const String trimmed = Trim(line.Text);
        if (trimmed.size() >= 2 && trimmed.front() == '[' && trimmed.back() == ']') {
            section = Trim(trimmed.substr(1, trimmed.size() - 2));
            line.Section = section;
            line.IsHeader = true;
        } else if (!trimmed.empty() && trimmed[0] != ';' && trimmed[0] != '#') {
            const size_t equals = trimmed.find('=');
            if (equals != String::npos) {
                line.Section = section;
                line.Key = Trim(trimmed.substr(0, equals));
                line.Value = Trim(trimmed.substr(equals + 1));
            }
        }
        file.m_Lines.Add(line);
    }
    return file;
}

String IniFile::ToString() const {
    String text;
    for (const Line& line : m_Lines) {
        text += line.Text;
        text += '\n';
    }
    return text;
}

Array<IniFile::Entry> IniFile::GetEntries() const {
    Array<Entry> entries;
    for (const Line& line : m_Lines) {
        if (line.IsEntry()) {
            entries.Add(Entry{ line.Section, line.Key, line.Value });
        }
    }
    return entries;
}

const String* IniFile::Find(const String& InSection, const String& InKey) const {
    const int32_t index = FindEntry(InSection, InKey);
    return index >= 0 ? &m_Lines[index].Value : nullptr;
}

void IniFile::Set(const String& InSection, const String& InKey, const String& InValue) {
    Line entry;
    entry.Text = InKey + "=" + InValue;
    entry.Section = InSection;
    entry.Key = InKey;
    entry.Value = InValue;

    const int32_t existing = FindEntry(InSection, InKey);
    if (existing >= 0) {
        m_Lines[existing] = entry;
        return;
    }

    const int32_t header = FindHeader(InSection);
    if (header < 0 && !InSection.empty()) {
        if (!m_Lines.IsEmpty() && !m_Lines.LastItem().IsBlank()) {
            m_Lines.Add(Line());
        }
        Line headerLine;
        headerLine.Text = "[" + InSection + "]";
        headerLine.Section = InSection;
        headerLine.IsHeader = true;
        m_Lines.Add(headerLine);
        m_Lines.Add(entry);
        return;
    }

    int32_t insertAt = header + 1;
    const int32_t end = FindSectionEnd(header);
    for (int32_t i = header + 1; i < end; i++) {
        if (!m_Lines[i].IsBlank()) {
            insertAt = i + 1;
        }
    }
    if (InSection.empty() && insertAt < m_Lines.Size() && m_Lines[insertAt].IsHeader) {
        m_Lines.Insert(insertAt, Line());
    }
    m_Lines.Insert(insertAt, entry);
}

bool IniFile::Remove(const String& InSection, const String& InKey) {
    const int32_t index = FindEntry(InSection, InKey);
    if (index < 0) {
        return false;
    }
    m_Lines.RemoveAt(index);
    PruneSection(InSection);
    return true;
}

bool IniFile::IsEmpty() const {
    for (const Line& line : m_Lines) {
        if (!line.IsBlank()) {
            return false;
        }
    }
    return true;
}

int32_t IniFile::FindEntry(const String& InSection, const String& InKey) const {
    for (int32_t i = 0; i < m_Lines.Size(); i++) {
        if (m_Lines[i].IsEntry() && m_Lines[i].Section == InSection && m_Lines[i].Key == InKey) {
            return i;
        }
    }
    return -1;
}

int32_t IniFile::FindHeader(const String& InSection) const {
    if (InSection.empty()) {
        return -1;
    }
    for (int32_t i = 0; i < m_Lines.Size(); i++) {
        if (m_Lines[i].IsHeader && m_Lines[i].Section == InSection) {
            return i;
        }
    }
    return -1;
}

int32_t IniFile::FindSectionEnd(int32_t InHeader) const {
    for (int32_t i = InHeader + 1; i < m_Lines.Size(); i++) {
        if (m_Lines[i].IsHeader) {
            return i;
        }
    }
    return m_Lines.Size();
}

void IniFile::PruneSection(const String& InSection) {
    const int32_t header = FindHeader(InSection);
    if (header >= 0) {
        const int32_t end = FindSectionEnd(header);
        bool empty = true;
        for (int32_t i = header + 1; i < end && empty; i++) {
            empty = m_Lines[i].IsBlank();
        }
        if (empty) {
            for (int32_t i = end - 1; i >= header; i--) {
                m_Lines.RemoveAt(i);
            }
        }
    }
    while (!m_Lines.IsEmpty() && m_Lines.LastItem().IsBlank()) {
        m_Lines.RemoveLastItem();
    }
}
