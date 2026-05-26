#pragma once
#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QSlider>
#include <QButtonGroup>
#include <QTimer>
#include <set>
#include <tuple>
#include <string>
#include "WalkerThread.h"

class WalkerTab : public QWidget {
    Q_OBJECT
public:
    explicit WalkerTab(QWidget* parent = nullptr);
    void start_walker_thread(int state);
    void stop_all_threads();
    std::set<std::tuple<int,int,int>> get_blacklist() const;
private slots:
    void add_waypoint();
    void add_blacklist_tile();
    void clear_blacklist();
    void clear_waypoints();
    void start_record_thread(int state);
    void on_waypoint_recorded(int x, int y, int z, int action, int direction, QString display);
    void update_position();
    void update_interval_label(int val);
private:
    WalkerThread* walker_thread = nullptr;
    RecordThread* record_thread = nullptr;
    QListWidget* waypoint_list;
    QListWidget* blacklist_widget;
    QLineEdit* bl_x; QLineEdit* bl_y; QLineEdit* bl_z;
    QCheckBox* record_cb;
    QSlider* interval_slider;
    QLabel* interval_label;
    QLabel* status_label;
    QLabel* pos_label;
    QButtonGroup* dir_group;
    QButtonGroup* act_group;
    QTimer* pos_timer;
    QTimer* sync_timer = nullptr;
};
