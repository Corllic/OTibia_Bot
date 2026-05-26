#include "GeneralFunctions.h"
#include <filesystem>
#include <fstream>
#include <QJsonDocument>
#include <QListWidget>

namespace GeneralFunctions {

    bool manage_profile(const std::string& action, const std::string& directory,
                        const std::string& profile_name, const QJsonObject& data) {
        std::string file_path = directory + "/" + profile_name + ".json";

        if (action == "save") {
            std::filesystem::create_directories(directory);
            QJsonDocument doc(data);
            std::ofstream f(file_path);
            if (!f.is_open()) return false;
            auto bytes = doc.toJson(QJsonDocument::Indented);
            f.write(bytes.constData(), bytes.size());
            return true;
        } else if (action == "load") {
            if (!std::filesystem::exists(file_path)) return false;
            return true;
        }
        return false;
    }

    void delete_item(QListWidget* list, QListWidgetItem* item) {
        int idx = list->row(item);
        list->takeItem(idx);
    }
}
