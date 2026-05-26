#include <QApplication>
#include <QStyleFactory>
#include "src/General/SelectTibiaTab.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));

    SelectTibiaTab* sel = new SelectTibiaTab();
    sel->show();

    return app.exec();
}
