#pragma once
#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QProgressBar>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QFutureWatcher>
#include "../Creature/CreatureScanner.h"

class StatusTab : public QWidget {
    Q_OBJECT
public:
    explicit StatusTab(QWidget* parent = nullptr);
protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
private slots:
    void refresh();
    void scan_creatures();
    void detect_name_offset();
    void auto_detect_offsets();
    void on_scan_finished();
    void on_detect_finished();
private:
    QLabel*       pos_label;
    QLabel*       hp_label;
    QLabel*       mp_label;
    QLabel*       attack_label;
    QProgressBar* hp_bar;
    QProgressBar* mp_bar;
    QProgressBar* scan_progress;
    QListWidget*  creature_list;
    QLabel*       creature_count_label;
    QLabel*       offsets_label;
    QLineEdit*    name_hint_edit;
    QLineEdit*    creature_x_edit;
    QLineEdit*    creature_y_edit;
    QLineEdit*    creature_z_edit;
    QLineEdit*    name_hint_edit2;
    QLineEdit*    creature_x_edit2;
    QLineEdit*    creature_y_edit2;
    QLineEdit*    creature_z_edit2;
    QTimer*       timer;
    QTimer*       progress_timer;
    QTimer*       scan_timer;
    QFutureWatcher<bool>*                          detect_watcher;
    QFutureWatcher<std::vector<CreatureScanner::Creature>>* scan_watcher;
};
