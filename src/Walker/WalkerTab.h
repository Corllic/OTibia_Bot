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

class WalkerTab : public QWidget {
    Q_OBJECT
public:
    explicit WalkerTab(QWidget* parent = nullptr);
    std::set<std::tuple<int,int,int>> get_blacklist() const;
protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
private slots:
    void add_waypoint();
    void add_blacklist_tile();
    void clear_blacklist();
    void clear_waypoints();
    void update_position();
    void update_interval_label(int val);
private:
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
