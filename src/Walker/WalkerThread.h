#pragma once
#include <QThread>
#include <QMutex>
#include <vector>
#include <set>
#include <tuple>
#include "../Functions/PathfindingFunctions.h"

class WalkerThread : public QThread {
    Q_OBJECT
public:
    explicit WalkerThread(std::vector<PathfindingFunctions::Waypoint> waypoints, QObject* parent = nullptr);
    void stop();
signals:
    void index_update(int option, int value);
    void waypoint_append(QString text);
protected:
    void run() override;
private:
    int find_wpt();
    std::vector<PathfindingFunctions::Waypoint> waypoints;
    std::set<std::pair<int,int>> discovered_obstacles;
    std::pair<int,int> last_target_pos = {-1, -1};
    bool running = true;
};

class RecordThread : public QThread {
    Q_OBJECT
public:
    explicit RecordThread(int interval = 1, QObject* parent = nullptr);
    void stop();
    void update_snapshot(int action_id, int direction_id, const std::string& direction_text);
signals:
    void wpt_recorded(int x, int y, int z, int action, int direction, QString display);
protected:
    void run() override;
private:
    int interval;
    bool running = true;
    QMutex data_lock;
    int current_action = 0;
    int current_direction = 0;
    std::string current_dir_text = "Center";
};
