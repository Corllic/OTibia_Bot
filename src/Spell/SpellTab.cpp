#include "SpellTab.h"
#include "../Functions/GeneralFunctions.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QIcon>

SpellTab::SpellTab(QWidget* parent) : QWidget(parent) {
    setWindowIcon(QIcon("Icon.ico"));
    setWindowTitle("Spell");
    setFixedSize(450, 300);

    auto* grid = new QGridLayout(this);
    key_combo   = new QComboBox(this);
    for (int i = 1; i <= 9; i++) key_combo->addItem(QString("F%1").arg(i));
    key_combo->addItems({"First Rune", "Second Rune"});
    dist_combo  = new QComboBox(this);
    dist_combo->addItem("No dist");
    for (int i = 1; i <= 5; i++) dist_combo->addItem(QString::number(i));
    target_edit  = new QLineEdit(this); target_edit->setPlaceholderText("Orc, Minotaur, * - All");
    hp_from_edit = new QLineEdit(this); hp_from_edit->setPlaceholderText("100"); hp_from_edit->setFixedWidth(40);
    hp_to_edit   = new QLineEdit(this); hp_to_edit->setPlaceholderText("0");     hp_to_edit->setFixedWidth(40);
    min_mp_edit  = new QLineEdit(this); min_mp_edit->setPlaceholderText("300");  min_mp_edit->setFixedWidth(40);
    min_hp_edit  = new QLineEdit(this); min_hp_edit->setPlaceholderText("50");   min_hp_edit->setFixedWidth(40);
    spell_list   = new QListWidget(this);
    connect(spell_list, &QListWidget::itemDoubleClicked, [this](QListWidgetItem* item) {
        GeneralFunctions::delete_item(spell_list, item);
    });

    status_label = new QLabel("", this);
    status_label->setStyleSheet("color: Red; font-weight: bold;");
    status_label->setAlignment(Qt::AlignCenter);

    auto* gb = new QGroupBox("Spell", this);
    auto* vl = new QVBoxLayout(gb);
    vl->addWidget(spell_list);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(target_edit);
    row1->addWidget(new QLabel("Key:")); row1->addWidget(key_combo);
    row1->addWidget(new QLabel("Dist:")); row1->addWidget(dist_combo);
    vl->addLayout(row1);

    auto* row2 = new QHBoxLayout();
    row2->addWidget(new QLabel("HP%:")); row2->addWidget(hp_from_edit);
    row2->addWidget(new QLabel("-")); row2->addWidget(hp_to_edit);
    row2->addWidget(new QLabel("Min.MP:")); row2->addWidget(min_mp_edit);
    row2->addWidget(new QLabel("Min.HP%:")); row2->addWidget(min_hp_edit);
    vl->addLayout(row2);

    auto* add_btn = new QPushButton("Add", this);
    connect(add_btn, &QPushButton::clicked, this, &SpellTab::add_spell);
    vl->addWidget(add_btn);

    grid->addWidget(gb, 0, 0, 1, 2);
    grid->addWidget(status_label, 1, 0, 1, 2);
}

void SpellTab::add_spell() {
    if (target_edit->text().isEmpty() || hp_from_edit->text().isEmpty() || hp_to_edit->text().isEmpty()) {
        status_label->setStyleSheet("color: Red; font-weight: bold;");
        status_label->setText("Fill all required fields.");
        return;
    }
    int min_dist = (dist_combo->currentText() == "No dist") ? 0 : dist_combo->currentText().toInt();
    QVariantMap data;
    data["Name"]    = target_edit->text();
    data["Key"]     = key_combo->currentText();
    data["HpFrom"]  = hp_from_edit->text().toInt();
    data["HpTo"]    = hp_to_edit->text().toInt();
    data["MinMp"]   = min_mp_edit->text().isEmpty() ? 0 : min_mp_edit->text().toInt();
    data["MinHp"]   = min_hp_edit->text().isEmpty() ? 0 : min_hp_edit->text().toInt();
    data["MinDist"] = min_dist;

    QString lbl = QString("%1 : (%2%-%3%) : %4 : Dist %5")
        .arg(target_edit->text()).arg(data["HpFrom"].toInt()).arg(data["HpTo"].toInt())
        .arg(key_combo->currentText()).arg(dist_combo->currentText());
    auto* item = new QListWidgetItem(lbl);
    item->setData(Qt::UserRole, data);
    spell_list->addItem(item);
    target_edit->clear(); hp_from_edit->clear(); hp_to_edit->clear();
    min_mp_edit->clear(); min_hp_edit->clear();
    status_label->setStyleSheet("color: Green; font-weight: bold;");
    status_label->setText("Spell added!");
}

void SpellTab::start_thread(int state) {
    if (state == Qt::Checked) {
        if (spell_thread) { spell_thread->stop(); spell_thread->wait(2000); }
        std::vector<QVariantMap> data;
        for (int i = 0; i < spell_list->count(); i++)
            data.push_back(spell_list->item(i)->data(Qt::UserRole).toMap());
        spell_thread = new SpellThread(data, this);
        spell_thread->start();
    } else {
        if (spell_thread) { spell_thread->stop(); spell_thread->wait(2000); spell_thread = nullptr; }
    }
}

void SpellTab::stop_all_threads() { start_thread(Qt::Unchecked); }
