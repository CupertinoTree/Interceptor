#include <QApplication>
#include "ui/MainWindow.h"
#include "controllers/AppController.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow mainWindow;
    mainWindow.show();

    AppController controller(&mainWindow);

    return app.exec();
}