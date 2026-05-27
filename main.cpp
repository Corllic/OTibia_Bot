#include <QApplication>
#include <QStyleFactory>
#include "src/General/SelectTibiaTab.h"
#include "src/Core/Logger.h"
#include "src/Core/Addresses.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));

    if (Addresses::log_level >= 2)
        Logger::open_file();

    SelectTibiaTab* sel = new SelectTibiaTab();
    sel->show();

    int ret = app.exec();
    Logger::close_file();
    return ret;
}
