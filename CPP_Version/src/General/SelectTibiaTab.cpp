#include "SelectTibiaTab.h"
#include "MainWindowTab.h"
#include "../Core/Addresses.h"
#include <QVBoxLayout>
#include <QIcon>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

struct EnumData {
    std::vector<ProcessInfo>* list;
    QListWidget* widget;
};

static BOOL CALLBACK enum_wnd_cb(HWND hwnd, LPARAM lp) {
    auto* d = reinterpret_cast<EnumData*>(lp);
    if (!IsWindowVisible(hwnd)) return TRUE;
    char title[256] = {};
    GetWindowTextA(hwnd, title, sizeof(title));
    std::string t(title);
    if (t.empty() || t.find("EasyBot") != std::string::npos) return TRUE;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (!pid) return TRUE;

    char exe[256] = {};
    HANDLE ph = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (ph) {
        DWORD sz = sizeof(exe);
        QueryFullProcessImageNameA(ph, 0, exe, &sz);
        CloseHandle(ph);
    }
    std::string pname(exe);
    size_t sl = pname.rfind('\\');
    if (sl != std::string::npos) pname = pname.substr(sl + 1);

    d->list->push_back({hwnd, t, pid, pname});
    std::string display = t + " (" + pname + " - PID: " + std::to_string(pid) + ")";
    d->widget->addItem(QString::fromStdString(display));
    return TRUE;
}

SelectTibiaTab::SelectTibiaTab(QWidget* parent) : QWidget(parent) {
    setWindowIcon(QIcon("Icon.ico"));
    setWindowTitle("EasyBot Select Client");
    setFixedSize(500, 400);

    auto* layout = new QVBoxLayout(this);

    auto* label = new QLabel("Select a process to attach:", this);
    label->setStyleSheet("font-weight: bold; font-size: 14px;");
    layout->addWidget(label);

    process_listwidget = new QListWidget(this);
    layout->addWidget(process_listwidget);

    connect_button = new QPushButton("Connect to Selected Process", this);
    connect(connect_button, &QPushButton::clicked, this, &SelectTibiaTab::load_tibia_button);
    layout->addWidget(connect_button);

    refresh_button = new QPushButton("Refresh Process List", this);
    connect(refresh_button, &QPushButton::clicked, this, &SelectTibiaTab::refresh_processes);
    layout->addWidget(refresh_button);

    refresh_processes();
}

void SelectTibiaTab::refresh_processes() {
    process_listwidget->clear();
    process_list.clear();
    EnumData d{&process_list, process_listwidget};
    EnumWindows(enum_wnd_cb, reinterpret_cast<LPARAM>(&d));
}

void SelectTibiaTab::load_tibia_button() {
    int idx = process_listwidget->currentRow();
    if (idx < 0 || idx >= (int)process_list.size()) return;
    auto& sel = process_list[idx];
    Addresses::load_tibia(sel.window_title, sel.proc_id, sel.hwnd);
    close();
    auto* mw = new MainWindowTab();
    mw->show();
}
