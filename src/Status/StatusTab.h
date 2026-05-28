#pragma once
#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QProgressBar>
#include <QListWidget>
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
    void do_map_scan();
    void do_fast_refresh();
    void on_map_scan_finished();
    void on_fast_refresh_finished();
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
    QTimer*       timer;
    QTimer*       progress_timer;
    QTimer*       map_scan_timer;
    QTimer*       fast_refresh_timer;
    QFutureWatcher<std::vector<CreatureScanner::Creature>>* map_watcher;
    QFutureWatcher<void>*                                   refresh_watcher;
};
