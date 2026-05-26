#pragma once
#include <QWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QGroupBox>

class HealingTab;
class SpellTab;
class TargetTab;
class WalkerTab;
class SettingsTab;

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
    void toggle_healing(int state);
    void toggle_spell(int state);
    void toggle_targeting(int state);
    void toggle_walker(int state);
private:
    HealingTab*  healingTab  = nullptr;
    SpellTab*    spellTab    = nullptr;
    TargetTab*   targetTab   = nullptr;
    WalkerTab*   walkerTab   = nullptr;
    SettingsTab* settingsTab = nullptr;

    QCheckBox* healing_cb;
    QCheckBox* spell_cb;
    QCheckBox* targeting_cb;
    QCheckBox* walker_cb;
};
