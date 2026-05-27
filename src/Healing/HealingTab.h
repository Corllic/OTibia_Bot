#pragma once
#include <QWidget>
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

class HealingTab : public QWidget {
    Q_OBJECT
public:
    explicit HealingTab(QWidget* parent = nullptr);
private slots:
    void add_spell();
    void add_item();
    void move_spell_up();
    void move_spell_down();
    void remove_spell();
    void move_item_up();
    void move_item_down();
    void remove_item();
    void on_item_key_changed(const QString& text);
private:
    QListWidget* spell_list;
    QComboBox*   spell_when_cb;
    QComboBox*   spell_is_cb;
    QLineEdit*   spell_val_edit;
    QLineEdit*   spell_cost_edit;
    QComboBox*   spell_key_cb;

    QListWidget* item_list;
    QComboBox*   item_when_cb;
    QComboBox*   item_is_cb;
    QLineEdit*   item_val_edit;
    QComboBox*   item_key_cb;
    QLineEdit*   item_x_edit;
    QLineEdit*   item_y_edit;
};
