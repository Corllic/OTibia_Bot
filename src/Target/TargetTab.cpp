#include "TargetTab.h"
#include "../Functions/GeneralFunctions.h"
#include "../Core/Addresses.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QIcon>

TargetTab::TargetTab(QWidget* parent) : QWidget(parent) {
    setWindowIcon(QIcon("Icon.ico"));
    setWindowTitle("Targeting");
    setFixedSize(350, 280);

    auto* grid = new QGridLayout(this);

    dist_combo = new QComboBox(this);
    dist_combo->addItems({"All","1","2","3","4","5","6","7"});
    stance_combo = new QComboBox(this);
    stance_combo->addItems({"Do Nothing","Chase","Diagonal","Chase-Diagonal"});
    skin_combo = new QComboBox(this);
    skin_combo->addItem("No Action");
    for (int i = 1; i <= 12; i++) skin_combo->addItem(QString("F%1").arg(i));
    target_edit = new QLineEdit(this); target_edit->setPlaceholderText("Orc, * - All Monsters");
    target_list = new QListWidget(this); target_list->setFixedSize(150, 150);
    connect(target_list, &QListWidget::itemDoubleClicked, [this](QListWidgetItem* item) {
        GeneralFunctions::delete_item(target_list, item);
    });

    status_label = new QLabel("", this);
    status_label->setAlignment(Qt::AlignCenter);

    auto* gb  = new QGroupBox("Targeting", this);
    auto* ghl = new QHBoxLayout(gb);

    auto* ll     = new QVBoxLayout();
    auto* cl_btn = new QPushButton("Clear List", this);
    connect(cl_btn, &QPushButton::clicked, this, &TargetTab::clear_target_list);
    ll->addWidget(target_list); ll->addWidget(cl_btn);

    auto* rl  = new QVBoxLayout();
    auto* r1  = new QHBoxLayout();
    auto* add_btn = new QPushButton("Add", this);
    connect(add_btn, &QPushButton::clicked, this, &TargetTab::add_target);
    r1->addWidget(target_edit); r1->addWidget(add_btn); rl->addLayout(r1);

    auto* r2 = new QHBoxLayout(); r2->addWidget(new QLabel("Dist:"));   r2->addWidget(dist_combo);   rl->addLayout(r2);
    auto* r3 = new QHBoxLayout(); r3->addWidget(new QLabel("Stance:")); r3->addWidget(stance_combo); rl->addLayout(r3);
    auto* r4 = new QHBoxLayout(); r4->addWidget(new QLabel("Skin:"));   r4->addWidget(skin_combo);   rl->addLayout(r4);

    ghl->addLayout(ll); ghl->addLayout(rl);
    grid->addWidget(gb, 0, 0, 1, 2);
    grid->addWidget(status_label, 1, 0, 1, 2);
}

void TargetTab::add_target() {
    QString name = target_edit->text().trimmed();
    if (name.isEmpty()) { status_label->setText("Enter monster name."); return; }
    for (int i = 0; i < target_list->count(); i++)
        if (target_list->item(i)->text().toUpper() == name.toUpper()) return;

    QVariantMap data;
    data["Name"]   = name;
    data["Dist"]   = dist_combo->currentIndex();
    data["Stance"] = stance_combo->currentIndex();
    data["Skin"]   = skin_combo->currentIndex();
    auto* item = new QListWidgetItem(name);
    item->setData(Qt::UserRole, data);
    target_list->addItem(item);
    target_edit->clear();
    status_label->setText(QString("Target '%1' added!").arg(name));
}

void TargetTab::clear_target_list() { target_list->clear(); }

void TargetTab::start_target_thread(int state, std::set<std::tuple<int,int,int>> blacklist) {
    if (state == Qt::Checked) {
        if (target_thread) { target_thread->stop(); target_thread->wait(2000); }
        std::vector<QVariantMap> targets;
        for (int i = 0; i < target_list->count(); i++)
            targets.push_back(target_list->item(i)->data(Qt::UserRole).toMap());
        target_thread = new TargetThread(targets, Addresses::attack_key - 1, blacklist, this);
        target_thread->start();
    } else {
        if (target_thread) { target_thread->stop(); target_thread->wait(2000); target_thread = nullptr; }
    }
}

void TargetTab::stop_all_threads() { start_target_thread(Qt::Unchecked); }
