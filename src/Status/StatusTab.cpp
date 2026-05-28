#include "StatusTab.h"
#include "../Functions/Memory.h"
#include "../Core/Addresses.h"
#include "../Creature/CreatureScanner.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QIcon>
#include <QShowEvent>
#include <QtConcurrent/QtConcurrent>

StatusTab::StatusTab(QWidget* parent) : QWidget(parent) {
    setWindowTitle("Status");
    setWindowIcon(QIcon("Icon.ico"));
    setFixedSize(340, 460);

    auto* root = new QVBoxLayout(this);
    root->setSpacing(6);
    root->setContentsMargins(8, 8, 8, 8);

    {
        auto* gb = new QGroupBox("Character Status", this);
        auto* gl = new QGridLayout(gb);
        gl->setSpacing(4);

        hp_bar = new QProgressBar(this);
        hp_bar->setRange(0, 100);
        hp_bar->setTextVisible(false);
        hp_bar->setStyleSheet("QProgressBar::chunk { background-color: #c0392b; }");

        mp_bar = new QProgressBar(this);
        mp_bar->setRange(0, 100);
        mp_bar->setTextVisible(false);
        mp_bar->setStyleSheet("QProgressBar::chunk { background-color: #2980b9; }");

        hp_label     = new QLabel("HP: -", this);
        mp_label     = new QLabel("MP: -", this);
        pos_label    = new QLabel("Pos: -, -, -", this);
        attack_label = new QLabel("Attacking: No", this);

        gl->addWidget(new QLabel("HP:"),   0, 0);
        gl->addWidget(hp_bar,              0, 1);
        gl->addWidget(hp_label,            0, 2);
        gl->addWidget(new QLabel("MP:"),   1, 0);
        gl->addWidget(mp_bar,              1, 1);
        gl->addWidget(mp_label,            1, 2);
        gl->addWidget(pos_label,           2, 0, 1, 3);
        gl->addWidget(attack_label,        3, 0, 1, 3);

        root->addWidget(gb);
    }

    {
        auto* gb = new QGroupBox("Creatures Nearby", this);
        auto* vl = new QVBoxLayout(gb);

        scan_progress = new QProgressBar(this);
        scan_progress->setRange(0, 100);
        scan_progress->setValue(0);
        scan_progress->setTextVisible(true);
        scan_progress->setFixedHeight(16);
        scan_progress->hide();

        auto* top_hl = new QHBoxLayout();
        creature_count_label = new QLabel("Found: 0", this);
        top_hl->addWidget(creature_count_label);
        top_hl->addStretch();

        creature_list = new QListWidget(this);
        creature_list->setFixedHeight(200);

        vl->addWidget(scan_progress);
        vl->addLayout(top_hl);
        vl->addWidget(creature_list);
        root->addWidget(gb);
    }

    map_watcher = new QFutureWatcher<std::vector<CreatureScanner::Creature>>(this);
    connect(map_watcher, &QFutureWatcher<std::vector<CreatureScanner::Creature>>::finished,
            this, &StatusTab::on_map_scan_finished);

    refresh_watcher = new QFutureWatcher<void>(this);
    connect(refresh_watcher, &QFutureWatcher<void>::finished,
            this, &StatusTab::on_fast_refresh_finished);

    progress_timer = new QTimer(this);
    progress_timer->setInterval(200);
    connect(progress_timer, &QTimer::timeout, this, [this]() {
        scan_progress->setValue(CreatureScanner::scan_progress_pct.load());
    });

    timer = new QTimer(this);
    timer->setInterval(500);
    connect(timer, &QTimer::timeout, this, &StatusTab::refresh);

    map_scan_timer = new QTimer(this);
    map_scan_timer->setInterval(500);
    connect(map_scan_timer, &QTimer::timeout, this, &StatusTab::do_map_scan);

    fast_refresh_timer = new QTimer(this);
    fast_refresh_timer->setInterval(50);
    connect(fast_refresh_timer, &QTimer::timeout, this, &StatusTab::do_fast_refresh);
}

void StatusTab::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    timer->start();
    map_scan_timer->start();
    fast_refresh_timer->start();
    do_map_scan();
}

void StatusTab::hideEvent(QHideEvent* event) {
    QWidget::hideEvent(event);
    timer->stop();
    map_scan_timer->stop();
    fast_refresh_timer->stop();
    progress_timer->stop();
}

void StatusTab::refresh() {
    auto hp     = Memory::read(Addresses::my_stats_address, Addresses::my_hp_offset);
    auto max_hp = Memory::read(Addresses::my_stats_address, Addresses::my_hp_max_offset);
    auto mp     = Memory::read(Addresses::my_stats_address, Addresses::my_mp_offset);
    auto max_mp = Memory::read(Addresses::my_stats_address, Addresses::my_mp_max_offset);

    if (hp && max_hp) {
        int h = static_cast<int>(*hp), mh = static_cast<int>(*max_hp);
        hp_label->setText(QString("HP: %1 / %2").arg(h).arg(mh));
        hp_bar->setValue(mh > 0 ? h * 100 / mh : 0);
    } else { hp_label->setText("HP: -"); hp_bar->setValue(0); }

    if (mp && max_mp) {
        int m = static_cast<int>(*mp), mm = static_cast<int>(*max_mp);
        mp_label->setText(QString("MP: %1 / %2").arg(m).arg(mm));
        mp_bar->setValue(mm > 0 ? m * 100 / mm : 0);
    } else { mp_label->setText("MP: -"); mp_bar->setValue(0); }

    auto x = Memory::read(Addresses::my_x_address, Addresses::my_x_address_offset, Addresses::my_x_type);
    auto y = Memory::read(Addresses::my_y_address, Addresses::my_y_address_offset, Addresses::my_y_type);
    auto z = Memory::read(Addresses::my_z_address, Addresses::my_z_address_offset, Addresses::my_z_type);

    if (x && y && z)
        pos_label->setText(QString("Pos: %1, %2, %3").arg(*x).arg(*y).arg(*z));
    else
        pos_label->setText("Pos: -, -, -");

    auto atk = Memory::read(Addresses::attack_address, Addresses::attack_address_offset);
    bool attacking = atk && *atk != 0;
    attack_label->setText(attacking ? "Attacking: Yes" : "Attacking: No");
    attack_label->setStyleSheet(attacking ? "color: #c0392b; font-weight: bold;" : "");
}

void StatusTab::do_map_scan() {
    if (map_watcher->isRunning()) return;

    auto x = Memory::read(Addresses::my_x_address, Addresses::my_x_address_offset);
    auto y = Memory::read(Addresses::my_y_address, Addresses::my_y_address_offset);
    auto z = Memory::read(Addresses::my_z_address, Addresses::my_z_address_offset, Addresses::my_z_type);
    if (!x || !y || !z) return;

    int px = static_cast<int>(*x), py = static_cast<int>(*y), pz = static_cast<int>(*z);

    scan_progress->setValue(0);
    scan_progress->show();
    progress_timer->start();

    auto future = QtConcurrent::run([px, py, pz]() -> std::vector<CreatureScanner::Creature> {
        return CreatureScanner::scan_map(px, py, pz);
    });
    map_watcher->setFuture(future);
}

void StatusTab::on_map_scan_finished() {
    progress_timer->stop();
    scan_progress->hide();

    std::vector<CreatureScanner::Creature> creatures;
    {
        std::lock_guard<std::mutex> lk(CreatureScanner::creatures_mutex);
        creatures = CreatureScanner::known_creatures;
    }

    creature_list->clear();
    for (auto& c : creatures) {
        creature_list->addItem(QString("%1  [%2, %3, %4]  HP:%5%")
            .arg(QString::fromStdString(c.name))
            .arg(c.x).arg(c.y).arg(c.z)
            .arg(c.hp_pct));
    }
    creature_count_label->setText(QString("Found: %1").arg(creatures.size()));
}

void StatusTab::do_fast_refresh() {
    if (refresh_watcher->isRunning()) return;

    {
        std::lock_guard<std::mutex> lk(CreatureScanner::creatures_mutex);
        if (CreatureScanner::known_creatures.empty()) return;
    }

    auto future = QtConcurrent::run([]() {
        CreatureScanner::refresh_known();
    });
    refresh_watcher->setFuture(future);
}

void StatusTab::on_fast_refresh_finished() {
    std::vector<CreatureScanner::Creature> creatures;
    {
        std::lock_guard<std::mutex> lk(CreatureScanner::creatures_mutex);
        creatures = CreatureScanner::known_creatures;
    }

    for (int i = 0; i < creature_list->count() && i < (int)creatures.size(); ++i) {
        auto& c = creatures[i];
        creature_list->item(i)->setText(QString("%1  [%2, %3, %4]  HP:%5%")
            .arg(QString::fromStdString(c.name))
            .arg(c.x).arg(c.y).arg(c.z)
            .arg(c.hp_pct));
    }
}
