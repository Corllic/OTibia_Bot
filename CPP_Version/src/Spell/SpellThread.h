#pragma once
#include <QThread>
#include <QVariantMap>
#include <vector>

class SpellThread : public QThread {
    Q_OBJECT
public:
    explicit SpellThread(std::vector<QVariantMap> data, QObject* parent = nullptr);
    void stop();
protected:
    void run() override;
private:
    std::vector<QVariantMap> spell_data;
    bool running = true;
};
