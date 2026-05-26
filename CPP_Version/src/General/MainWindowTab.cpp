#include "MainWindowTab.h"
#include "../Healing/HealingTab.h"
#include "../Spell/SpellTab.h"
#include "../Target/TargetTab.h"
#include "../Walker/WalkerTab.h"
#include "../Settings/SettingsTab.h"
#include <QCloseEvent>
#include <QIcon>
#include <QVBoxLayout>

MainWindowTab::MainWindowTab(QWidget* parent) : QWidget(parent) {
    setWindowTitle("EasyBot");
    setWindowIcon(QIcon("Icon.ico"));

    auto* grid = new QGridLayout(this);

    auto make_btn = [&](const QString& label, auto slot, int r, int c) {
        auto* b = new QPushButton(label, this);
        connect(b, &QPushButton::clicked, this, slot);
        grid->addWidget(b, r, c);
    };

    make_btn("Healing",   &MainWindowTab::open_healing,   0, 0);
    make_btn("Spell",     &MainWindowTab::open_spell,     0, 1);
    make_btn("Targeting", &MainWindowTab::open_targeting, 1, 0);
    make_btn("Walker",    &MainWindowTab::open_walker,    1, 1);
    make_btn("Settings",  &MainWindowTab::open_settings,  2, 0);

    auto* status_gb = new QGroupBox("Bot Status", this);
    auto* sg = new QGridLayout(status_gb);
    healing_cb   = new QCheckBox("Healing",   this);
    spell_cb     = new QCheckBox("Spell",     this);
    targeting_cb = new QCheckBox("Targeting", this);
    walker_cb    = new QCheckBox("Walker",    this);
    connect(healing_cb,   &QCheckBox::stateChanged, this, &MainWindowTab::toggle_healing);
    connect(spell_cb,     &QCheckBox::stateChanged, this, &MainWindowTab::toggle_spell);
    connect(targeting_cb, &QCheckBox::stateChanged, this, &MainWindowTab::toggle_targeting);
    connect(walker_cb,    &QCheckBox::stateChanged, this, &MainWindowTab::toggle_walker);
    sg->addWidget(healing_cb,   0, 0); sg->addWidget(spell_cb,     0, 1);
    sg->addWidget(targeting_cb, 1, 0); sg->addWidget(walker_cb,    1, 1);
    grid->addWidget(status_gb, 3, 0, 1, 2);
}

void MainWindowTab::open_healing()   { if (!healingTab)  healingTab  = new HealingTab();  healingTab->show(); }
void MainWindowTab::open_spell()     { if (!spellTab)    spellTab    = new SpellTab();    spellTab->show(); }
void MainWindowTab::open_targeting() { if (!targetTab)   targetTab   = new TargetTab();   targetTab->show(); }
void MainWindowTab::open_walker()    { if (!walkerTab)   walkerTab   = new WalkerTab();   walkerTab->show(); }
void MainWindowTab::open_settings()  { if (!settingsTab) settingsTab = new SettingsTab(); settingsTab->show(); }

void MainWindowTab::toggle_healing(int state) {
    if (!healingTab) healingTab = new HealingTab();
    healingTab->start_thread(state);
}
void MainWindowTab::toggle_spell(int state) {
    if (!spellTab) spellTab = new SpellTab();
    spellTab->start_thread(state);
}
void MainWindowTab::toggle_targeting(int state) {
    if (!targetTab) targetTab = new TargetTab();
    targetTab->start_target_thread(state,
        walkerTab ? walkerTab->get_blacklist() : std::set<std::tuple<int,int,int>>{});
}
void MainWindowTab::toggle_walker(int state) {
    if (!walkerTab) walkerTab = new WalkerTab();
    walkerTab->start_walker_thread(state);
}

void MainWindowTab::closeEvent(QCloseEvent* event) {
    if (walkerTab)  walkerTab->stop_all_threads();
    if (targetTab)  targetTab->stop_all_threads();
    if (healingTab) healingTab->stop_all_threads();
    if (spellTab)   spellTab->stop_all_threads();
    event->accept();
}
