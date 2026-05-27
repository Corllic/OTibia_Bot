#pragma once
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QGridLayout>
#include <QScrollArea>
#include <unordered_map>
#include <string>

class SettingsTab : public QWidget {
    Q_OBJECT
public:
    explicit SettingsTab(QWidget* parent = nullptr);
private slots:
    void save_addresses();
    void start_set_character();
private:
    void load_addresses();
    void add_addr_row(QGridLayout* gl, int row, const QString& label, const QString& key);

    struct AddrWidgets { QLineEdit* addr; QLineEdit* offset; };
    std::unordered_map<std::string, AddrWidgets> address_widgets;

    QLineEdit* square_size_edit;
    QComboBox* attack_key_combo;
    QComboBox* walk_mode_combo;
    QLabel*    status_label;
};
