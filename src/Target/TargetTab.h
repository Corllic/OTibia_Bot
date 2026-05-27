#pragma once
#include <QWidget>
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
class TargetTab : public QWidget {
    Q_OBJECT
public:
    explicit TargetTab(QWidget* parent = nullptr);
private slots:
    void add_target();
    void clear_target_list();
private:
    QListWidget* target_list;
    QLineEdit*   target_edit;
    QComboBox*   dist_combo;
    QComboBox*   stance_combo;
    QComboBox*   skin_combo;
    QLabel*      status_label;
};
