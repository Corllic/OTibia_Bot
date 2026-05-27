#include "HealingTab.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QIcon>

HealingTab::HealingTab(QWidget* parent) : QWidget(parent) {
    setWindowIcon(QIcon("Icon.ico"));
    setWindowTitle("Healing");
    setFixedWidth(350);

    auto* root_vl = new QVBoxLayout(this);
    root_vl->setSpacing(4);
    root_vl->setContentsMargins(6, 6, 6, 6);
    {
        auto* gb = new QGroupBox("Spell Healing", this);
        auto* vl = new QVBoxLayout(gb);
        vl->setSpacing(3);

        spell_list = new QListWidget(this);
        spell_list->setFixedHeight(120);
        vl->addWidget(spell_list);

        spell_when_cb = new QComboBox(this);
        spell_when_cb->addItems({"Current Mana","Current Health","Mana Percent","Health Percent"});
        spell_is_cb = new QComboBox(this);
        spell_is_cb->addItems({"Below","Above","Equal To"});
        spell_val_edit = new QLineEdit(this);
        spell_val_edit->setPlaceholderText("Value");

        spell_cost_edit = new QLineEdit(this); spell_cost_edit->setPlaceholderText("MP Cost");
        spell_key_cb = new QComboBox(this);
        for (int i = 1; i <= 12; i++) spell_key_cb->addItem(QString("F%1").arg(i));

        auto* row1 = new QHBoxLayout();
        row1->addWidget(new QLabel("When")); row1->addWidget(spell_when_cb);
        row1->addWidget(new QLabel("Is")); row1->addWidget(spell_is_cb);
        row1->addWidget(new QLabel("Value")); row1->addWidget(spell_val_edit);
        vl->addLayout(row1);

        auto* row2 = new QHBoxLayout();
        row2->addWidget(new QLabel("MP Cost")); row2->addWidget(spell_cost_edit);
        row2->addWidget(new QLabel("Key")); row2->addWidget(spell_key_cb);
        vl->addLayout(row2);

        auto* row3 = new QHBoxLayout();
        auto* up  = new QPushButton("Move Up",   this);
        auto* dn  = new QPushButton("Move Down", this);
        auto* rm  = new QPushButton("Remove",    this);
        auto* add = new QPushButton("Add",       this);
        connect(up,  &QPushButton::clicked, this, &HealingTab::move_spell_up);
        connect(dn,  &QPushButton::clicked, this, &HealingTab::move_spell_down);
        connect(rm,  &QPushButton::clicked, this, &HealingTab::remove_spell);
        connect(add, &QPushButton::clicked, this, &HealingTab::add_spell);
        row3->addWidget(up); row3->addWidget(dn); row3->addWidget(rm); row3->addStretch(); row3->addWidget(add);
        vl->addLayout(row3);

        root_vl->addWidget(gb);
    }

    {
        auto* gb = new QGroupBox("Item Healing", this);
        auto* vl = new QVBoxLayout(gb);
        vl->setSpacing(3);

        item_list = new QListWidget(this);
        item_list->setFixedHeight(120);
        vl->addWidget(item_list);

        item_when_cb = new QComboBox(this);
        item_when_cb->addItems({"Current Mana","Current Health","Mana Percent","Health Percent"});
        item_is_cb = new QComboBox(this);
        item_is_cb->addItems({"Below","Above","Equal To"});
        item_val_edit = new QLineEdit(this); item_val_edit->setPlaceholderText("Value");

        item_key_cb = new QComboBox(this);
        for (int i = 1; i <= 12; i++) item_key_cb->addItem(QString("F%1").arg(i));
        item_key_cb->addItem("Coordinates");

        item_x_edit = new QLineEdit(this); item_x_edit->setPlaceholderText("X"); item_x_edit->setFixedWidth(55); item_x_edit->setEnabled(false);
        item_y_edit = new QLineEdit(this); item_y_edit->setPlaceholderText("Y"); item_y_edit->setFixedWidth(55); item_y_edit->setEnabled(false);

        connect(item_key_cb, &QComboBox::currentTextChanged, this, &HealingTab::on_item_key_changed);

        auto* row1 = new QHBoxLayout();
        row1->addWidget(new QLabel("When")); row1->addWidget(item_when_cb);
        row1->addWidget(new QLabel("Is")); row1->addWidget(item_is_cb);
        row1->addWidget(new QLabel("Value")); row1->addWidget(item_val_edit);
        vl->addLayout(row1);

        auto* row2 = new QHBoxLayout();
        row2->addWidget(new QLabel("Key")); row2->addWidget(item_key_cb);
        row2->addWidget(item_x_edit); row2->addWidget(item_y_edit);
        vl->addLayout(row2);

        auto* row3 = new QHBoxLayout();
        auto* up  = new QPushButton("Move Up",   this);
        auto* dn  = new QPushButton("Move Down", this);
        auto* rm  = new QPushButton("Remove",    this);
        auto* add = new QPushButton("Add",       this);
        connect(up,  &QPushButton::clicked, this, &HealingTab::move_item_up);
        connect(dn,  &QPushButton::clicked, this, &HealingTab::move_item_down);
        connect(rm,  &QPushButton::clicked, this, &HealingTab::remove_item);
        connect(add, &QPushButton::clicked, this, &HealingTab::add_item);
        row3->addWidget(up); row3->addWidget(dn); row3->addWidget(rm); row3->addStretch(); row3->addWidget(add);
        vl->addLayout(row3);

        root_vl->addWidget(gb);
    }
}

void HealingTab::on_item_key_changed(const QString& text) {
    bool coords = (text == "Coordinates");
    item_x_edit->setEnabled(coords);
    item_y_edit->setEnabled(coords);
    if (!coords) {
        item_x_edit->setPlaceholderText("X");
        item_y_edit->setPlaceholderText("Y");
    }
}

static QString make_spell_label(const QString& when, const QString& is, const QString& val,
                                 const QString& cost, const QString& key) {
    return QString("%1 %2 %3 | MP>=%4 | %5").arg(when, is, val, cost, key);
}

static QString make_item_label(const QString& when, const QString& is, const QString& val,
                                const QString& key_type, const QString& x, const QString& y) {
    if (key_type == "Coordinates")
        return QString("%1 %2 %3 | Coord(%4,%5)").arg(when, is, val, x, y);
    return QString("%1 %2 %3 | %4").arg(when, is, val, key_type);
}

void HealingTab::add_spell() {
    QVariantMap d;
    d["When"]     = spell_when_cb->currentText();
    d["Is"]       = spell_is_cb->currentText();
    d["Value"]    = spell_val_edit->text().toInt();
    d["ManaCost"] = spell_cost_edit->text().toInt();
    d["Key"]      = spell_key_cb->currentText();
    auto* item = new QListWidgetItem(make_spell_label(
        spell_when_cb->currentText(), spell_is_cb->currentText(),
        spell_val_edit->text(), spell_cost_edit->text(), spell_key_cb->currentText()));
    item->setData(Qt::UserRole, d);
    spell_list->addItem(item);
}

void HealingTab::add_item() {
    QVariantMap d;
    d["When"]  = item_when_cb->currentText();
    d["Is"]    = item_is_cb->currentText();
    d["Value"] = item_val_edit->text().toInt();
    QString key_str = item_key_cb->currentText();
    if (key_str == "Coordinates") {
        d["KeyType"] = QString("Coordinates");
        d["X"]       = item_x_edit->text().toInt();
        d["Y"]       = item_y_edit->text().toInt();
    } else {
        d["KeyType"] = QString("Hotkey");
        d["Key"]     = key_str;
    }
    auto* item = new QListWidgetItem(make_item_label(
        item_when_cb->currentText(), item_is_cb->currentText(),
        item_val_edit->text(), key_str, item_x_edit->text(), item_y_edit->text()));
    item->setData(Qt::UserRole, d);
    item_list->addItem(item);
}

static void move_up(QListWidget* lw) {
    int r = lw->currentRow(); if (r <= 0) return;
    auto* it = lw->takeItem(r); lw->insertItem(r-1, it); lw->setCurrentRow(r-1);
}
static void move_down(QListWidget* lw) {
    int r = lw->currentRow(); if (r < 0 || r >= lw->count()-1) return;
    auto* it = lw->takeItem(r); lw->insertItem(r+1, it); lw->setCurrentRow(r+1);
}
static void remove_sel(QListWidget* lw) {
    int r = lw->currentRow(); if (r < 0) return; delete lw->takeItem(r);
}

void HealingTab::move_spell_up()   { move_up(spell_list); }
void HealingTab::move_spell_down() { move_down(spell_list); }
void HealingTab::remove_spell()    { remove_sel(spell_list); }
void HealingTab::move_item_up()    { move_up(item_list); }
void HealingTab::move_item_down()  { move_down(item_list); }
void HealingTab::remove_item()     { remove_sel(item_list); }

