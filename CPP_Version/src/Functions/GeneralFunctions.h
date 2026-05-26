#pragma once
#include <string>
#include <QListWidget>
#include <QJsonObject>

namespace GeneralFunctions {
    bool manage_profile(const std::string& action, const std::string& directory,
                        const std::string& profile_name, const QJsonObject& data = {});
    void delete_item(QListWidget* list, QListWidgetItem* item);
}
