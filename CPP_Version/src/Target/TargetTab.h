#pragma once
#include <QWidget>
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <set>
#include <tuple>
#include "TargetThread.h"

class TargetTab : public QWidget {
    Q_OBJECT
public:
    explicit TargetTab(QWidget* parent = nullptr);
    void start_target_thread(int state,
                              std::set<std::tuple<int,int,int>> blacklist = {});
    void stop_all_threads();
private slots:
    void add_target();
    void clear_target_list();
private:
    TargetThread* target_thread = nullptr;
    QListWidget* target_list;
    QLineEdit*   target_edit;
    QComboBox*   dist_combo;
    QComboBox*   stance_combo;
    QComboBox*   skin_combo;
    QLabel*      status_label;
};
