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
#include <atomic>

StatusTab::StatusTab(QWidget* parent) : QWidget(parent) {
    setWindowTitle("Status");
    setWindowIcon(QIcon("Icon.ico"));
    setFixedSize(340, 600);

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

        auto* name_hl = new QHBoxLayout();
        name_hint_edit = new QLineEdit(this);
        name_hint_edit->setPlaceholderText("Creature 1 name...");
        name_hl->addWidget(new QLabel("Name 1:", this));
        name_hl->addWidget(name_hint_edit);

        auto* coord_hl = new QHBoxLayout();
        creature_x_edit = new QLineEdit(this); creature_x_edit->setPlaceholderText("X"); creature_x_edit->setFixedWidth(60);
        creature_y_edit = new QLineEdit(this); creature_y_edit->setPlaceholderText("Y"); creature_y_edit->setFixedWidth(60);
        creature_z_edit = new QLineEdit(this); creature_z_edit->setPlaceholderText("Z"); creature_z_edit->setFixedWidth(40);
        coord_hl->addWidget(creature_x_edit);
        coord_hl->addWidget(creature_y_edit);
        coord_hl->addWidget(creature_z_edit);
        coord_hl->addStretch();

        auto* name_hl2 = new QHBoxLayout();
        name_hint_edit2 = new QLineEdit(this);
        name_hint_edit2->setPlaceholderText("Creature 2 name...");
        name_hl2->addWidget(new QLabel("Name 2:", this));
        name_hl2->addWidget(name_hint_edit2);

        auto* coord_hl2 = new QHBoxLayout();
        creature_x_edit2 = new QLineEdit(this); creature_x_edit2->setPlaceholderText("X"); creature_x_edit2->setFixedWidth(60);
        creature_y_edit2 = new QLineEdit(this); creature_y_edit2->setPlaceholderText("Y"); creature_y_edit2->setFixedWidth(60);
        creature_z_edit2 = new QLineEdit(this); creature_z_edit2->setPlaceholderText("Z"); creature_z_edit2->setFixedWidth(40);
        auto* auto_btn = new QPushButton("Auto-Detect", this);
        connect(auto_btn, &QPushButton::clicked, this, &StatusTab::auto_detect_offsets);
        coord_hl2->addWidget(creature_x_edit2);
        coord_hl2->addWidget(creature_y_edit2);
        coord_hl2->addWidget(creature_z_edit2);
        coord_hl2->addStretch();
        coord_hl2->addWidget(auto_btn);

        scan_progress = new QProgressBar(this);
        scan_progress->setRange(0, 100);
        scan_progress->setValue(0);
        scan_progress->setTextVisible(true);
        scan_progress->setFixedHeight(16);
        scan_progress->hide();

        offsets_label = new QLabel("", this);
        offsets_label->setStyleSheet("color: #888; font-size: 10px;");

        auto* top_hl = new QHBoxLayout();
        creature_count_label = new QLabel("Found: 0", this);
        top_hl->addWidget(creature_count_label);
        top_hl->addStretch();

        creature_list = new QListWidget(this);
        creature_list->setFixedHeight(150);

        vl->addLayout(name_hl);
        vl->addLayout(coord_hl);
        vl->addLayout(name_hl2);
        vl->addLayout(coord_hl2);
        vl->addWidget(scan_progress);
        vl->addWidget(offsets_label);
        vl->addLayout(top_hl);
        vl->addWidget(creature_list);
        root->addWidget(gb);
    }

    detect_watcher = new QFutureWatcher<bool>(this);
    connect(detect_watcher, &QFutureWatcher<bool>::finished, this, &StatusTab::on_detect_finished);

    scan_watcher = new QFutureWatcher<std::vector<CreatureScanner::Creature>>(this);
    connect(scan_watcher, &QFutureWatcher<std::vector<CreatureScanner::Creature>>::finished,
            this, &StatusTab::on_scan_finished);

    progress_timer = new QTimer(this);
    progress_timer->setInterval(200);
    connect(progress_timer, &QTimer::timeout, this, [this]() {
        scan_progress->setValue(CreatureScanner::scan_progress_pct.load());
    });

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &StatusTab::refresh);

    scan_timer = new QTimer(this);
    scan_timer->setInterval(1000);
    connect(scan_timer, &QTimer::timeout, this, &StatusTab::scan_creatures);
}

void StatusTab::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    timer->start(500);
    if (Addresses::creature_found_address != 0)
        scan_timer->start();
}

void StatusTab::hideEvent(QHideEvent* event) {
    QWidget::hideEvent(event);
    timer->stop();
    scan_timer->stop();
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

    auto x = Memory::read(Addresses::my_x_address, Addresses::my_x_address_offset);
    auto y = Memory::read(Addresses::my_y_address, Addresses::my_y_address_offset);
    auto z = Memory::read(Addresses::my_z_address, Addresses::my_z_address_offset);

    if (x && y && z)
        pos_label->setText(QString("Pos: %1, %2, %3").arg(*x).arg(*y).arg(*z));
    else
        pos_label->setText("Pos: -, -, -");

    auto atk = Memory::read(Addresses::attack_address, Addresses::attack_address_offset);
    bool attacking = atk && *atk != 0;
    attack_label->setText(attacking ? "Attacking: Yes" : "Attacking: No");
    attack_label->setStyleSheet(attacking ? "color: #c0392b; font-weight: bold;" : "");
}

void StatusTab::auto_detect_offsets() {
    if (detect_watcher->isRunning()) return;

    QString name1 = name_hint_edit->text().trimmed();
    QString name2 = name_hint_edit2->text().trimmed();
    if (name1.isEmpty() || creature_x_edit->text().isEmpty() ||
        creature_y_edit->text().isEmpty() || creature_z_edit->text().isEmpty() ||
        name2.isEmpty() || creature_x_edit2->text().isEmpty() ||
        creature_y_edit2->text().isEmpty() || creature_z_edit2->text().isEmpty()) {
        creature_count_label->setText("Fill both name + X/Y/Z");
        return;
    }

    int cx1 = creature_x_edit->text().toInt();
    int cy1 = creature_y_edit->text().toInt();
    int cz1 = creature_z_edit->text().toInt();
    int cx2 = creature_x_edit2->text().toInt();
    int cy2 = creature_y_edit2->text().toInt();
    int cz2 = creature_z_edit2->text().toInt();
    std::string sname1 = name1.toStdString();
    std::string sname2 = name2.toStdString();

    creature_count_label->setText("Detecting...");
    scan_progress->setValue(0);
    scan_progress->show();
    progress_timer->start();

    auto future = QtConcurrent::run([cx1, cy1, cz1, sname1, cx2, cy2, cz2, sname2]() {
        return CreatureScanner::detect_all_offsets(cx1, cy1, cz1, sname1, cx2, cy2, cz2, sname2);
    });
    detect_watcher->setFuture(future);
}

void StatusTab::on_detect_finished() {
    progress_timer->stop();
    scan_progress->setValue(100);
    scan_progress->hide();
    bool ok = detect_watcher->result();
    if (ok) {
        offsets_label->setText(
            QString("x:0x%1 y:0x%2 z:0x%3 name:0x%4 id:%5")
                .arg(Addresses::creature_x_off, 0, 16)
                .arg(Addresses::creature_y_off, 0, 16)
                .arg(Addresses::creature_z_off, 0, 16)
                .arg(Addresses::creature_name_off, 0, 16)
                .arg(Addresses::creature_id_prefix)
        );
        scan_timer->start();
        scan_creatures();
    } else {
        creature_count_label->setText("Not found");
    }
}

void StatusTab::detect_name_offset() {
    if (detect_watcher->isRunning()) return;
    QString hint = name_hint_edit->text().trimmed();
    if (hint.isEmpty()) { creature_count_label->setText("Enter name hint first"); return; }

    auto x = Memory::read(Addresses::my_x_address, Addresses::my_x_address_offset);
    auto y = Memory::read(Addresses::my_y_address, Addresses::my_y_address_offset);
    auto z = Memory::read(Addresses::my_z_address, Addresses::my_z_address_offset);
    if (!x || !y || !z) { creature_count_label->setText("No position"); return; }

    int px = static_cast<int>(*x), py = static_cast<int>(*y), pz = static_cast<int>(*z);
    std::string sname = hint.toStdString();

    creature_count_label->setText("Detecting...");
    scan_progress->show();

    auto future = QtConcurrent::run([px, py, pz, sname]() {
        return CreatureScanner::detect_name_offset(px, py, pz, sname);
    });
    detect_watcher->setFuture(future);
}

void StatusTab::scan_creatures() {
    if (scan_watcher->isRunning()) return;

    auto x = Memory::read(Addresses::my_x_address, Addresses::my_x_address_offset);
    auto y = Memory::read(Addresses::my_y_address, Addresses::my_y_address_offset);
    auto z = Memory::read(Addresses::my_z_address, Addresses::my_z_address_offset);
    if (!x || !y || !z) return;

    int px = static_cast<int>(*x), py = static_cast<int>(*y), pz = static_cast<int>(*z);

    auto future = QtConcurrent::run([px, py, pz]() -> std::vector<CreatureScanner::Creature> {
        return CreatureScanner::scan(px, py, pz);
    });
    scan_watcher->setFuture(future);
}

void StatusTab::on_scan_finished() {
    auto creatures = scan_watcher->result();
    creature_list->clear();
    for (auto& c : creatures) {
        creature_list->addItem(QString("%1  [%2, %3, %4]")
            .arg(QString::fromStdString(c.name))
            .arg(c.x).arg(c.y).arg(c.z));
    }
    creature_count_label->setText(QString("Found: %1").arg(creatures.size()));
}
