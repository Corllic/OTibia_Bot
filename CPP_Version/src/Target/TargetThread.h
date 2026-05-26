#pragma once
#include <QThread>
#include <QVariantMap>
#include <vector>
#include <set>
#include <tuple>

class TargetThread : public QThread {
    Q_OBJECT
public:
    explicit TargetThread(std::vector<QVariantMap> targets,
                          int attack_key,
                          std::set<std::tuple<int,int,int>> blacklist = {},
                          QObject* parent = nullptr);
    void stop();
protected:
    void run() override;
private:
    std::vector<QVariantMap> targets;
    std::set<std::tuple<int,int,int>> blacklist_tiles;
    std::set<std::pair<int,int>> discovered_obstacles;
    std::pair<int,int> last_target_pos = {-1, -1};
    int attack_key;
    bool running = true;
};
