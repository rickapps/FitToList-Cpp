#include <QApplication>

#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    // Names QSettings uses to locate the persisted config file; MainWindow's
    // own QSettings member is default-constructed and relies on these.
    QCoreApplication::setOrganizationName("FitToList");
    QCoreApplication::setApplicationName("FitToList");

    MainWindow window;
    window.show();

    return app.exec();
}
