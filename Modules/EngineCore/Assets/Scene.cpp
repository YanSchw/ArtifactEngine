#include "Scene.h"
#include "NodeRecord.h"
#include "AssetManager.h"
#include "GameFramework/World.h"
#include "GameFramework/SceneRootNode.h"
#include "Platform/FileIO.h"
#include "Core/Log.h"
#include <filesystem>
#include <unordered_set>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <optional>
#include <vector>

namespace fs = std::filesystem;
using json = nlohmann::json;

static const String s_NodeFileExtension = ".node";
static constexpr size_t s_MaxNodeFileStem = 64;
static constexpr int64_t s_NodeOrderStride = 1000;

static String ToLower(String InText) {
    std::transform(InText.begin(), InText.end(), InText.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return InText;
}

static bool IsReservedFileName(const String& InStem) {
    String upper = InStem;
    std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char c) { return (char)std::toupper(c); });
    if (upper == "CON" || upper == "PRN" || upper == "AUX" || upper == "NUL") {
        return true;
    }
    return upper.size() == 4 && (upper.starts_with("COM") || upper.starts_with("LPT")) && upper[3] >= '1' && upper[3] <= '9';
}

static String NodeFileStem(const String& InName) {
    String stem;
    for (const char c : InName) {
        const bool invalid = (unsigned char)c < 32 || String("<>:\"/\\|?*").find(c) != String::npos;
        stem += invalid ? '_' : c;
    }
    if (stem.size() > s_MaxNodeFileStem) {
        size_t cut = s_MaxNodeFileStem;
        while (cut > 0 && ((unsigned char)stem[cut] & 0xC0) == 0x80) {
            cut--;
        }
        stem.resize(cut);
    }
    while (!stem.empty() && (stem.back() == '.' || stem.back() == ' ')) {
        stem.pop_back();
    }
    if (!stem.empty() && stem.front() == '.') {
        stem.front() = '_';
    }
    if (stem.empty()) {
        return "Node";
    }
    return IsReservedFileName(stem) ? stem + "_" : stem;
}

static String ClaimNodeFileStem(const String& InStem, std::unordered_set<String>& InOutTaken) {
    String stem = InStem;
    for (int32_t suffix = 2; InOutTaken.contains(ToLower(stem)) || InOutTaken.contains(ToLower(stem + s_NodeFileExtension)); suffix++) {
        stem = InStem + "_" + std::to_string(suffix);
    }
    InOutTaken.insert(ToLower(stem));
    InOutTaken.insert(ToLower(stem + s_NodeFileExtension));
    return stem;
}

static std::optional<int64_t> ReadNodeOrder(const json& InFile) {
    if (!InFile.is_object() || !InFile.contains("Order") || !InFile["Order"].is_number_integer()) {
        return std::nullopt;
    }
    return InFile["Order"].get<int64_t>();
}

static std::optional<int64_t> ReadNodeOrder(const fs::path& InFile) {
    std::error_code error;
    if (!fs::is_regular_file(InFile, error)) {
        return std::nullopt;
    }
    return ReadNodeOrder(json::parse(FileIO::ReadFileToString(InFile.string()), nullptr, false));
}

static std::vector<int64_t> AssignNodeOrders(const std::vector<std::optional<int64_t>>& InExisting) {
    const int32_t count = (int32_t)InExisting.size();

    std::vector<int32_t> tails;
    std::vector<int32_t> previous(count, -1);
    for (int32_t i = 0; i < count; i++) {
        if (!InExisting[i]) {
            continue;
        }
        const auto slot = std::lower_bound(tails.begin(), tails.end(), *InExisting[i],
                                           [&](int32_t InTail, int64_t InValue) { return *InExisting[InTail] < InValue; });
        previous[i] = slot == tails.begin() ? -1 : *(slot - 1);
        if (slot == tails.end()) {
            tails.push_back(i);
        } else {
            *slot = i;
        }
    }

    std::vector<std::optional<int64_t>> orders(count);
    for (int32_t i = tails.empty() ? -1 : tails.back(); i != -1; i = previous[i]) {
        orders[i] = InExisting[i];
    }

    for (int32_t start = 0; start < count;) {
        if (orders[start]) {
            start++;
            continue;
        }
        int32_t end = start;
        while (end < count && !orders[end]) {
            end++;
        }
        const int64_t missing = end - start;
        const std::optional<int64_t> lower = start > 0 ? orders[start - 1] : std::nullopt;
        const std::optional<int64_t> upper = end < count ? orders[end] : std::nullopt;
        if (lower && upper && *upper - *lower <= missing) {
            std::vector<int64_t> renumbered(count);
            for (int32_t i = 0; i < count; i++) {
                renumbered[i] = i * s_NodeOrderStride;
            }
            return renumbered;
        }
        const int64_t step = lower && upper ? (*upper - *lower) / (missing + 1) : s_NodeOrderStride;
        const int64_t first = lower ? *lower + step : upper ? *upper - step * missing : 0;
        for (int64_t i = 0; i < missing; i++) {
            orders[start + i] = first + step * i;
        }
        start = end;
    }

    std::vector<int64_t> assigned(count);
    for (int32_t i = 0; i < count; i++) {
        assigned[i] = *orders[i];
    }
    return assigned;
}

static void CollectNodeFiles(const NodeRecord& InParent, const fs::path& InDirectory, Map<String, String>& OutFiles) {
    std::unordered_set<String> taken;
    std::vector<const NodeRecord*> children;
    std::vector<String> stems;
    std::vector<std::optional<int64_t>> existing;
    for (const SharedObjectPtr<NodeRecord>& child : InParent.Children) {
        if (!child.Get()) {
            continue;
        }
        children.push_back(child.Get());
        stems.push_back(ClaimNodeFileStem(NodeFileStem(child->Name), taken));
        existing.push_back(ReadNodeOrder(InDirectory / (stems.back() + s_NodeFileExtension)));
    }

    const std::vector<int64_t> orders = AssignNodeOrders(existing);
    for (size_t i = 0; i < children.size(); i++) {
        json file = children[i]->ToJson();
        file.erase("Children");
        file["Order"] = orders[i];
        OutFiles[(InDirectory / (stems[i] + s_NodeFileExtension)).generic_string()] = file.dump(4);
        if (!children[i]->Children.IsEmpty()) {
            CollectNodeFiles(*children[i], InDirectory / stems[i], OutFiles);
        }
    }
}

static void ReadNodeFiles(NodeRecord& OutParent, const fs::path& InDirectory) {
    struct Entry {
        int64_t Order;
        SharedObjectPtr<NodeRecord> Record;
    };
    Array<Entry> entries;

    std::error_code error;
    for (const fs::directory_entry& entry : fs::directory_iterator(InDirectory, error)) {
        if (!entry.is_regular_file(error) || entry.path().extension().string() != s_NodeFileExtension) {
            continue;
        }
        const json file = json::parse(FileIO::ReadFileToString(entry.path().string()), nullptr, false);
        SharedObjectPtr<NodeRecord> record = file.is_object() ? NodeRecord::FromJson(file) : nullptr;
        if (!record) {
            AE_WARN("'{0}' is not a valid node file and is skipped", entry.path().string());
            continue;
        }
        const fs::path children = InDirectory / entry.path().stem();
        if (fs::is_directory(children, error)) {
            ReadNodeFiles(*record, children);
        }
        entries.Add({ ReadNodeOrder(file).value_or(INT64_MAX), record });
    }

    entries.Sort([](const Entry& InA, const Entry& InB) {
        return InA.Order != InB.Order ? InA.Order < InB.Order : InA.Record->Name < InB.Record->Name;
    });
    for (const Entry& entry : entries) {
        OutParent.Children.Add(entry.Record);
    }
}

SceneRootNode* Scene::Populate(World& OutWorld) {
    AssetManager::Get().LoadAsset(this);

    SceneRootNode* root = OutWorld.Spawn<SceneRootNode>();
    root->SetName(GetDisplayName());
    if (NodeRecord* record = GetRoot()) {
        record->Apply(*root);
    }
    root->BindScene(this);
    return root;
}

String Scene::SerializeToJson() const {
    const String serialized = Super::SerializeToJson();
    if (!m_OneFilePerNode) {
        return serialized;
    }

    json asset = json::parse(serialized);
    if (asset.contains("Root") && asset["Root"].is_object()) {
        asset["Root"].erase("Children");
    }
    return asset.dump(4);
}

void Scene::DeserializeFromJson(const String& InJson) {
    Super::DeserializeFromJson(InJson);
    if (!m_OneFilePerNode || !m_Root.Get() || !AssetManager::IsAvailable()) {
        return;
    }

    const String path = AssetManager::Get().GetAssetPath(GetId());
    if (path.empty()) {
        return;
    }
    const String directory = GetExternalDirectory(path);
    std::error_code error;
    if (!fs::is_directory(directory, error)) {
        AE_WARN("Scene '{0}' stores one file per node, but '{1}' does not exist", GetDisplayName(), directory);
        return;
    }
    ReadNodeFiles(*m_Root, directory);
}

bool Scene::SaveExternalFiles(const String& InAssetPath) const {
    const fs::path directory(GetExternalDirectory(InAssetPath));
    std::error_code error;
    if (!m_OneFilePerNode || !m_Root.Get()) {
        fs::remove_all(directory, error);
        if (error) {
            AE_ERROR("Failed to remove '{0}': {1}", directory.string(), error.message());
        }
        return !error;
    }

    Map<String, String> files;
    CollectNodeFiles(*m_Root, directory, files);

    std::unordered_set<String> folders;
    for (const auto& [file, content] : files) {
        for (fs::path folder = fs::path(file).parent_path(); folder != directory && folder.has_relative_path(); folder = folder.parent_path()) {
            if (!folders.insert(folder.generic_string()).second) {
                break;
            }
        }
    }

    Array<fs::path> stale;
    std::error_code walk;
    for (auto it = fs::recursive_directory_iterator(directory, walk); !walk && it != fs::recursive_directory_iterator(); it.increment(walk)) {
        const String key = it->path().generic_string();
        const bool isFolder = it->is_directory(error);
        if (isFolder ? !folders.contains(key) : it->path().extension().string() == s_NodeFileExtension && !files.ContainsKey(key)) {
            stale.Add(it->path());
        }
    }
    for (const fs::path& path : stale) {
        fs::remove_all(path, error);
    }

    bool written = true;
    for (const auto& [file, content] : files) {
        if (FileIO::FileExists(file) && FileIO::ReadFileToString(file) == content) {
            continue;
        }
        fs::create_directories(fs::path(file).parent_path(), error);
        if (!FileIO::WriteStringToFile(file, content)) {
            AE_ERROR("Failed to write node file '{0}'", file);
            written = false;
        }
    }
    return written;
}

Scene* Scene::CreateEmpty(const String& InDirectory, const String& InName) {
    Scene* scene = Cast<Scene>(AssetManager::Get().CreateAsset(StaticClass(), InDirectory, InName));
    if (!scene) {
        return nullptr;
    }

    NodeRecord* record = new NodeRecord();
    record->ClassName = SceneRootNode::StaticClass().Name;
    scene->SetRoot(SharedObjectPtr<NodeRecord>(record));

    AssetManager::Get().SaveAsset(scene);
    return scene;
}
