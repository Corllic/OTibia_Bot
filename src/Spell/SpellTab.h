#pragma once
#include <QWidget>
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <string>

class SpellTab : public QWidget {
    Q_OBJECT
public:
    explicit SpellTab(QWidget* parent = nullptr);
private slots:
    void add_spell();
private:
    QListWidget* spell_list;
    QComboBox*   key_combo;
    QComboBox*   dist_combo;
    QLineEdit*   target_edit;
    QLineEdit*   hp_from_edit;
    QLineEdit*   hp_to_edit;
    QLineEdit*   min_mp_edit;
    QLineEdit*   min_hp_edit;
    QLabel*      status_label;
};
