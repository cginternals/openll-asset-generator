#include <iostream>

#include <ft2build.h> // NOLINT include order required by freetype
#include FT_FREETYPE_H

#include <QApplication>
#include <QLabel>
#include <QGraphicsScene>
#include <QGraphicsView>

#include <llassetgen/llassetgen.h>

int main(int argc, char** argv) {
    llassetgen::init();

    QApplication a(argc, argv);

    QString path(a.applicationDirPath());
    QString fileName(path + "/../../data/arial_regular_300_512.png");
    QGraphicsScene scene;
    scene.addPixmap(QPixmap(fileName));
    QGraphicsView view(&scene);
    view.show();
    
    return a.exec();
}
