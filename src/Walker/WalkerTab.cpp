#include "WalkerTab.h"
#include "../Functions/MemoryFunctions.h"
#include "../Functions/GeneralFunctions.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QRadioButton>
#include <QIcon>

WalkerTab::WalkerTab(QWidget* parent) : QWidget(parent) {
    setWindowIcon(QIcon("Icon.ico"));
    setWindowTitle("Walker");
    setFixedSize(550, 450);

    auto* grid = new QGridLayout(this);

    waypoint_list  = new QListWidget(this);
    blacklist_widget = new QListWidget(this); blacklist_widget->setFixedSize(150, 200);
    record_cb      = new QCheckBox("Auto Recording", this);
    interval_slider = new QSlider(Qt::Horizontal, this);
    interval_slider->setRange(1, 4); interval_slider->setValue(1);
    interval_label = new QLabel("Record every: 1 square", this);
    status_label   = new QLabel("", this); status_label->setStyleSheet("color: red; font-weight: bold;"); status_label->setAlignment(Qt::AlignCenter);
    pos_label      = new QLabel("Current Pos: -, -, -", this); pos_label->setAlignment(Qt::AlignCenter);

    bl_x = new QLineEdit(this); bl_x->setPlaceholderText("X"); bl_x->setFixedWidth(45);
    bl_y = new QLineEdit(this); bl_y->setPlaceholderText("Y"); bl_y->setFixedWidth(45);
    bl_z = new QLineEdit(this); bl_z->setPlaceholderText("Z"); bl_z->setFixedWidth(45);

    dir_group = new QButtonGroup(this);
    act_group = new QButtonGroup(this);

    auto* wpt_gb = new QGroupBox("Waypoints", this);
    auto* wpt_hl = new QHBoxLayout(wpt_gb);

    auto* left_vl = new QVBoxLayout();
    auto* clear_btn = new QPushButton("Clear List", this);
    connect(clear_btn, &QPushButton::clicked, this, &WalkerTab::clear_waypoints);
    left_vl->addWidget(waypoint_list);
    left_vl->addWidget(clear_btn);

    auto* right_vl = new QVBoxLayout();

    auto* dir_gb = new QGroupBox("Direction", this);
    auto* dir_gl = new QGridLayout(dir_gb);
    struct DirCfg { QString txt; int idx; int r; int c; };
    DirCfg dirs[] = {{"NW",6,0,0},{"N",1,0,1},{"NE",5,0,2},{"W",4,1,0},{"C",0,1,1},{"E",3,1,2},{"SW",8,2,0},{"S",2,2,1},{"SE",7,2,2}};
    for (auto& d : dirs) {
        auto* rb = new QRadioButton(d.txt, this);
        dir_group->addButton(rb, d.idx);
        dir_gl->addWidget(rb, d.r, d.c);
        if (d.idx == 0) rb->setChecked(true);
    }

    auto* act_gb = new QGroupBox("Action", this);
    auto* act_gl = new QGridLayout(act_gb);
    struct ActCfg { QString txt; int idx; int r; int c; };
    ActCfg acts[] = {{"Stand",0,0,0},{"Lure",4,0,1},{"Rope",1,1,0},{"Shovel",2,1,1},{"Ladder",3,2,0}};
    for (auto& a : acts) {
        auto* rb = new QRadioButton(a.txt, this);
        act_group->addButton(rb, a.idx);
        act_gl->addWidget(rb, a.r, a.c);
        if (a.idx == 0) rb->setChecked(true);
    }

    auto* add_wpt_btn = new QPushButton("Add Waypoint", this);
    connect(add_wpt_btn, &QPushButton::clicked, this, &WalkerTab::add_waypoint);

    connect(interval_slider, &QSlider::valueChanged, this, &WalkerTab::update_interval_label);
    connect(record_cb, &QCheckBox::stateChanged, this, &WalkerTab::start_record_thread);

    right_vl->addWidget(dir_gb);
    right_vl->addWidget(act_gb);
    right_vl->addWidget(add_wpt_btn);
    right_vl->addWidget(record_cb);
    right_vl->addWidget(interval_label);
    right_vl->addWidget(interval_slider);

    wpt_hl->addLayout(left_vl, 2);
    wpt_hl->addLayout(right_vl, 1);
    grid->addWidget(wpt_gb, 0, 0, 1, 2);

    auto* bl_gb = new QGroupBox("Tiles Black List", this);
    auto* bl_vl = new QVBoxLayout(bl_gb);
    auto* bl_input = new QHBoxLayout();
    bl_input->addWidget(bl_x); bl_input->addWidget(bl_y); bl_input->addWidget(bl_z);
    bl_vl->addLayout(bl_input);
    auto* bl_btns = new QHBoxLayout();
    auto* bl_add = new QPushButton("Add", this);
    auto* bl_clr = new QPushButton("Clear", this);
    connect(bl_add, &QPushButton::clicked, this, &WalkerTab::add_blacklist_tile);
    connect(bl_clr, &QPushButton::clicked, this, &WalkerTab::clear_blacklist);
    bl_btns->addWidget(bl_add); bl_btns->addWidget(bl_clr);
    bl_vl->addLayout(bl_btns);
    bl_vl->addWidget(blacklist_widget);
    bl_vl->addWidget(pos_label);
    connect(blacklist_widget, &QListWidget::itemDoubleClicked, [this](QListWidgetItem* item) {
        GeneralFunctions::delete_item(blacklist_widget, item);
    });
    grid->addWidget(bl_gb, 0, 2, 2, 1);

    grid->addWidget(status_label, 2, 0, 1, 2);

    connect(waypoint_list, &QListWidget::itemDoubleClicked, [this](QListWidgetItem* item) {
        GeneralFunctions::delete_item(waypoint_list, item);
    });

    pos_timer = new QTimer(this);
    connect(pos_timer, &QTimer::timeout, this, &WalkerTab::update_position);
    pos_timer->start(500);
}

void WalkerTab::add_waypoint() {
    auto [x, y, z] = MemoryFunctions::read_my_wpt();
    int act = act_group->checkedId();
    int dir = dir_group->checkedId();
    QVariantMap data;
    data["X"] = x; data["Y"] = y; data["Z"] = z; data["Action"] = act; data["Direction"] = dir;
    QString dir_txt = dir_group->button(dir)->text();
    if (dir_txt == "C") dir_txt = "Center";
    static const QStringList act_names = {"Stand","Rope","Shovel","Ladder","Lure"};
    QString lbl = QString("%1: %2 %3 %4 %5").arg(act < act_names.size() ? act_names[act] : "?")
        .arg(x).arg(y).arg(z).arg(dir_txt);
    auto* item = new QListWidgetItem(lbl);
    item->setData(Qt::UserRole, data);
    waypoint_list->addItem(item);
    status_label->setStyleSheet("color: green; font-weight: bold;");
    status_label->setText("Waypoint added!");
}

void WalkerTab::add_blacklist_tile() {
    if (bl_x->text().isEmpty() || bl_y->text().isEmpty() || bl_z->text().isEmpty()) {
        status_label->setText("Enter X, Y, Z coordinates.");
        return;
    }
    int x = bl_x->text().toInt(), y = bl_y->text().toInt(), z = bl_z->text().toInt();
    QString txt = QString("%1, %2, %3").arg(x).arg(y).arg(z);
    for (int i = 0; i < blacklist_widget->count(); i++)
        if (blacklist_widget->item(i)->text() == txt) return;
    QVariantMap td; td["X"] = x; td["Y"] = y; td["Z"] = z;
    auto* item = new QListWidgetItem(txt);
    item->setData(Qt::UserRole, td);
    blacklist_widget->addItem(item);
    bl_x->clear(); bl_y->clear(); bl_z->clear();
    status_label->setStyleSheet("color: green; font-weight: bold;");
    status_label->setText(QString("Tile %1 added!").arg(txt));
}

void WalkerTab::clear_blacklist() { blacklist_widget->clear(); }
void WalkerTab::clear_waypoints() { waypoint_list->clear(); }

std::set<std::tuple<int,int,int>> WalkerTab::get_blacklist() const {
    std::set<std::tuple<int,int,int>> s;
    for (int i = 0; i < blacklist_widget->count(); i++) {
        QVariantMap j = blacklist_widget->item(i)->data(Qt::UserRole).toMap();
        s.insert({j["X"].toInt(), j["Y"].toInt(), j["Z"].toInt()});
    }
    return s;
}

void WalkerTab::start_walker_thread(int state) {
    if (state == Qt::Checked) {
        if (walker_thread) { walker_thread->stop(); walker_thread->wait(2000); }
        std::vector<PathfindingFunctions::Waypoint> wpts;
        for (int i = 0; i < waypoint_list->count(); i++) {
            QVariantMap j = waypoint_list->item(i)->data(Qt::UserRole).toMap();
            wpts.push_back({j["X"].toInt(), j["Y"].toInt(), j["Z"].toInt(), j["Action"].toInt(), j["Direction"].toInt()});
        }
        if (wpts.empty()) return;
        walker_thread = new WalkerThread(wpts, this);
        connect(walker_thread, &WalkerThread::index_update, [this](int opt, int val) {
            if (opt == 0) waypoint_list->setCurrentRow(val);
        });
        walker_thread->start();
    } else {
        if (walker_thread) { walker_thread->stop(); walker_thread->wait(2000); walker_thread = nullptr; }
    }
}

void WalkerTab::start_record_thread(int state) {
    if (state == Qt::Checked) {
        if (record_thread) { record_thread->stop(); record_thread->wait(2000); }
        record_thread = new RecordThread(interval_slider->value(), this);
        connect(record_thread, &RecordThread::wpt_recorded, this, &WalkerTab::on_waypoint_recorded);
        if (sync_timer) sync_timer->stop();
        sync_timer = new QTimer(this);
        connect(sync_timer, &QTimer::timeout, [this]() {
            if (!record_thread) return;
            int did = dir_group->checkedId();
            auto* btn = dir_group->button(did);
            QString dt = btn ? btn->text() : "Center";
            if (dt == "C") dt = "Center";
            record_thread->update_snapshot(act_group->checkedId(), did, dt.toStdString());
        });
        sync_timer->start(200);
        record_thread->start();
    } else {
        if (record_thread) { record_thread->stop(); record_thread->wait(2000); record_thread = nullptr; }
        if (sync_timer) sync_timer->stop();
    }
}

void WalkerTab::on_waypoint_recorded(int x, int y, int z, int action, int direction, QString display) {
    static const QStringList act_names = {"Stand","Rope","Shovel","Ladder","Lure"};
    QString lbl = QString("%1: %2 %3 %4 %5").arg(action < act_names.size() ? act_names[action] : "?")
        .arg(x).arg(y).arg(z).arg(display);
    QVariantMap data;
    data["X"]=x; data["Y"]=y; data["Z"]=z; data["Action"]=action; data["Direction"]=direction;
    auto* item = new QListWidgetItem(lbl);
    item->setData(Qt::UserRole, data);
    waypoint_list->addItem(item);
    waypoint_list->scrollToBottom();
}

void WalkerTab::update_position() {
    try {
        auto [x, y, z] = MemoryFunctions::read_my_wpt();
        pos_label->setText(QString("Current Pos: %1, %2, %3").arg(x).arg(y).arg(z));
    } catch (...) {
        pos_label->setText("Current Pos: -, -, -");
    }
}

void WalkerTab::update_interval_label(int val) {
    interval_label->setText(QString("Record every: %1 %2").arg(val).arg(val == 1 ? "square" : "squares"));
}

void WalkerTab::stop_all_threads() {
    start_walker_thread(Qt::Unchecked);
    start_record_thread(Qt::Unchecked);
}
