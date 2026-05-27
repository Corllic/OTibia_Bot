#include "SettingsTab.h"
#include "../Core/Addresses.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QIcon>
#include <QScrollArea>
#include <QSettings>

SettingsTab::SettingsTab(QWidget* parent) : QWidget(parent) {
    setWindowIcon(QIcon("Icon.ico"));
    setWindowTitle("Settings");
    setMinimumWidth(420);

    status_label = new QLabel("", this);
    status_label->setAlignment(Qt::AlignCenter);

    auto* main_vl = new QVBoxLayout(this);
    main_vl->setSpacing(6);

    {
        auto* gb = new QGroupBox("Coordinate Settings", this);
        auto* hl = new QHBoxLayout(gb);
        auto* btn = new QPushButton("Set Character", this);
        connect(btn, &QPushButton::clicked, this, &SettingsTab::start_set_character);
        hl->addWidget(btn);
        main_vl->addWidget(gb);
    }

    {
        auto* gb = new QGroupBox("Game Configuration", this);
        auto* gl = new QGridLayout(gb);
        gl->setColumnStretch(1, 1);

        gl->addWidget(new QLabel("Square Size:"), 0, 0);
        square_size_edit = new QLineEdit(this);
        square_size_edit->setPlaceholderText("75");
        gl->addWidget(square_size_edit, 0, 1);

        gl->addWidget(new QLabel("Attack Key:"), 1, 0);
        attack_key_combo = new QComboBox(this);
        for (int i = 1; i <= 12; i++) attack_key_combo->addItem(QString("F%1").arg(i));
        gl->addWidget(attack_key_combo, 1, 1);

        gl->addWidget(new QLabel("Walk by:"), 2, 0);
        walk_mode_combo = new QComboBox(this);
        walk_mode_combo->addItems({"Keyboard", "Mouse"});
        gl->addWidget(walk_mode_combo, 2, 1);

        main_vl->addWidget(gb);
    }

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    auto* scroll_widget = new QWidget();
    auto* scroll_vl = new QVBoxLayout(scroll_widget);
    scroll_vl->setSpacing(6);

    {
        struct Row { const char* lbl; const char* key; };
        static const Row player_rows[] = {
            {"X",       "my_x"},
            {"Y",       "my_y"},
            {"Z",       "my_z"},
            {"HP",      "my_hp"},
            {"HP Max",  "my_hp_max"},
            {"MP",      "my_mp"},
            {"MP Max",  "my_mp_max"},
            {"Attack",  "attack"},
        };
        auto* gb = new QGroupBox("Player", scroll_widget);
        auto* gl = new QGridLayout(gb);
        gl->setColumnStretch(1, 2);
        gl->setColumnStretch(2, 2);
        auto* h_addrs = new QLabel("Address");
        h_addrs->setAlignment(Qt::AlignCenter);
        h_addrs->setStyleSheet("font-weight: bold;");
        auto* h_off = new QLabel("Offset");
        h_off->setAlignment(Qt::AlignCenter);
        h_off->setStyleSheet("font-weight: bold;");
        gl->addWidget(new QLabel(""), 0, 0);
        gl->addWidget(h_addrs, 0, 1);
        gl->addWidget(h_off,   0, 2);
        for (int r = 0; r < (int)(sizeof(player_rows)/sizeof(player_rows[0])); r++)
            add_addr_row(gl, r + 1, player_rows[r].lbl, player_rows[r].key);
        scroll_vl->addWidget(gb);
    }

    {
        struct Row { const char* lbl; const char* key; };
        static const Row target_rows[] = {
            {"X Offset",    "target_x"},
            {"Y Offset",    "target_y"},
            {"Z Offset",    "target_z"},
            {"HP Offset",   "target_hp"},
            {"Name Offset", "target_name"},
        };
        auto* gb = new QGroupBox("Target", scroll_widget);
        auto* gl = new QGridLayout(gb);
        gl->setColumnStretch(1, 2);
        gl->setColumnStretch(2, 2);
        auto* h_addrs = new QLabel("Address");
        h_addrs->setAlignment(Qt::AlignCenter);
        h_addrs->setStyleSheet("font-weight: bold;");
        auto* h_off = new QLabel("Offset");
        h_off->setAlignment(Qt::AlignCenter);
        h_off->setStyleSheet("font-weight: bold;");
        gl->addWidget(new QLabel(""), 0, 0);
        gl->addWidget(h_addrs, 0, 1);
        gl->addWidget(h_off,   0, 2);
        for (int r = 0; r < (int)(sizeof(target_rows)/sizeof(target_rows[0])); r++)
            add_addr_row(gl, r + 1, target_rows[r].lbl, target_rows[r].key);
        scroll_vl->addWidget(gb);
    }

    auto* save_btn = new QPushButton("Save && Reload Addresses", scroll_widget);
    connect(save_btn, &QPushButton::clicked, this, &SettingsTab::save_addresses);
    scroll_vl->addWidget(save_btn);
    scroll_vl->addStretch();

    scroll->setWidget(scroll_widget);
    main_vl->addWidget(scroll, 1);
    main_vl->addWidget(status_label);

    load_addresses();
}

void SettingsTab::add_addr_row(QGridLayout* gl, int row, const QString& label, const QString& key) {
    auto* lbl = new QLabel(label);
    lbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    gl->addWidget(lbl, row, 0);

    auto* addr_e = new QLineEdit(); addr_e->setPlaceholderText("0x0");
    auto* off_e  = new QLineEdit(); off_e->setPlaceholderText("0x0, 0x4");
    gl->addWidget(addr_e, row, 1);
    gl->addWidget(off_e,  row, 2);

    address_widgets[key.toStdString()] = {addr_e, off_e};
}

void SettingsTab::save_addresses() {
    QSettings s("EasyBot", "Addresses");

    s.beginGroup("game_config");
    s.setValue("square_size", square_size_edit->text());
    s.setValue("attack_key",  attack_key_combo->currentIndex() + 1);
    s.setValue("walk_mode",   walk_mode_combo->currentIndex());
    s.endGroup();

    for (auto& [key, w] : address_widgets) {
        s.beginGroup(QString::fromStdString(key));
        s.setValue("address", w.addr->text());
        s.setValue("offset",  w.offset->text());
        s.endGroup();
    }

    Addresses::load_custom_addresses();
    status_label->setText("Addresses saved and reloaded!");
}

void SettingsTab::load_addresses() {
    QSettings s("EasyBot", "Addresses");

    s.beginGroup("game_config");
    square_size_edit->setText(s.value("square_size", "75").toString());
    int ak = s.value("attack_key", 1).toInt();
    if (ak >= 1 && ak <= 12) attack_key_combo->setCurrentIndex(ak - 1);
    walk_mode_combo->setCurrentIndex(s.value("walk_mode", 0).toInt());
    s.endGroup();

    for (auto& [key, w] : address_widgets) {
        s.beginGroup(QString::fromStdString(key));
        w.addr->setText(s.value("address").toString());
        w.offset->setText(s.value("offset").toString());
        s.endGroup();
    }
}

void SettingsTab::start_set_character() {
    status_label->setText("Click on character position...");
}
