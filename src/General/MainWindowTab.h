#pragma once
#include <QWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QGroupBox>

class HealingTab;
class SpellTab;
class TargetTab;
class WalkerTab;
class SettingsTab;
class StatusTab;

class MainWindowTab : public QWidget {
    Q_OBJECT
public:
    explicit MainWindowTab(QWidget* parent = nullptr);
protected:
    void closeEvent(QCloseEvent* event) override;
private slots:
    void open_healing();
    void open_spell();
    void open_targeting();
    void open_walker();
    void open_settings();
    void open_status();
private:
    HealingTab*  healingTab  = nullptr;
    SpellTab*    spellTab    = nullptr;
    TargetTab*   targetTab   = nullptr;
    WalkerTab*   walkerTab   = nullptr;
    SettingsTab* settingsTab = nullptr;
    StatusTab*   statusTab   = nullptr;
};
