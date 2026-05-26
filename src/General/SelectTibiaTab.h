#pragma once
#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <vector>
#include <string>
#include <windows.h>

struct ProcessInfo {
    HWND hwnd;
    std::string window_title;
    DWORD proc_id;
    std::string process_name;
};

class SelectTibiaTab : public QWidget {
    Q_OBJECT
public:
    explicit SelectTibiaTab(QWidget* parent = nullptr);
private slots:
    void refresh_processes();
    void load_tibia_button();
private:
    QListWidget* process_listwidget;
    QPushButton* connect_button;
    QPushButton* refresh_button;
    std::vector<ProcessInfo> process_list;
    QWidget* main_window = nullptr;
};
