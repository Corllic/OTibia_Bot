#include "MainWindowTab.h"
#include "../Healing/HealingTab.h"
#include "../Spell/SpellTab.h"
#include "../Target/TargetTab.h"
#include "../Walker/WalkerTab.h"
#include "../Settings/SettingsTab.h"
#include "../Status/StatusTab.h"
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
    make_btn("Status",    &MainWindowTab::open_status,    2, 1);
}

void MainWindowTab::open_healing()   { if (!healingTab)  healingTab  = new HealingTab();  healingTab->show(); }
void MainWindowTab::open_spell()     { if (!spellTab)    spellTab    = new SpellTab();    spellTab->show(); }
void MainWindowTab::open_targeting() { if (!targetTab)   targetTab   = new TargetTab();   targetTab->show(); }
void MainWindowTab::open_walker()    { if (!walkerTab)   walkerTab   = new WalkerTab();   walkerTab->show(); }
void MainWindowTab::open_settings()  { if (!settingsTab) settingsTab = new SettingsTab(); settingsTab->show(); }
void MainWindowTab::open_status()    { if (!statusTab)   statusTab   = new StatusTab();   statusTab->show(); }

void MainWindowTab::closeEvent(QCloseEvent* event) {
    event->accept();
}
