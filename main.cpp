#include <QApplication>
#include "FolderVisualizer.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Start visualizer with the home directory or another path
    QString startPath = QDir::homePath();
    if (argc > 1) {
        startPath = QString(argv[1]); // Allow passing a directory via command line
    }

    FolderVisualizer window(startPath);
    window.show();

    return app.exec();
}
