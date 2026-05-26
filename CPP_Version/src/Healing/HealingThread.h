#pragma once
#include <QThread>
#include <QVariantMap>
#include <vector>

class HealThread : public QThread {
    Q_OBJECT
public:
    explicit HealThread(std::vector<QVariantMap> spell_data,
                        std::vector<QVariantMap> item_data,
                        QObject* parent = nullptr);
    void stop();
protected:
    void run() override;
private:
    bool eval_condition(const QVariantMap& entry, int hp, int max_hp, int mp, int max_mp) const;
    std::vector<QVariantMap> spell_data;
    std::vector<QVariantMap> item_data;
    bool running = true;
};
