#include "MainWindow.h"

#include <QApplication>
#include <QFont>
#include <QPalette>

int main(int argc, char *argv[]) {
    QApplication application(argc, argv);
    application.setApplicationName("Codingbox");
    application.setOrganizationName("Codingbox");
    application.setOrganizationDomain("codingbox.dev");
    application.setStyle("Fusion");

    QFont mono("JetBrains Mono");
    if (!mono.exactMatch()) {
        mono = QFont("Cascadia Mono");
    }
    if (!mono.exactMatch()) {
        mono = QFont("Menlo");
    }
    mono.setStyleHint(QFont::Monospace);
    mono.setPointSize(10);
    application.setFont(mono);

    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#071b40"));
    palette.setColor(QPalette::WindowText, QColor("#dcecff"));
    palette.setColor(QPalette::Base, QColor("#0a2350"));
    palette.setColor(QPalette::Text, QColor("#dcecff"));
    palette.setColor(QPalette::Button, QColor("#0b285b"));
    palette.setColor(QPalette::ButtonText, QColor("#cce8ff"));
    palette.setColor(QPalette::Highlight, QColor("#1677b7"));
    palette.setColor(QPalette::HighlightedText, QColor("#ffffff"));
    application.setPalette(palette);

    MainWindow window;
    window.show();
    return application.exec();
}
