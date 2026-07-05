#ifndef FRAMERENDERER_H
#define FRAMERENDERER_H

#include <QPixmap>
#include <QPainter>
#include "Vision.h"

class FrameRenderer {

public:
    static void renderTarget(QPixmap &pix, VisionResult vr) {
        QPainter painter(&pix);
        painter.setPen(QPen((/*isGuiding ? Qt::red : */Qt::green), 2));
        painter.drawRect(vr.x - 15, vr.y - 15, 30, 30);
        //TO-DO: change size depending on FWHM/////////////////////////////
        painter.end();
    }

    static void renderPrimaryROI(QPixmap &pix, PrimaryROI primaryROI) {
        QPainter painter(&pix);

        painter.setPen(QPen(Qt::yellow, 2));
        painter.drawRect(primaryROI.CornerAx, primaryROI.CornerAy, primaryROI.CornerBx - primaryROI.CornerAx, primaryROI.CornerBy - primaryROI.CornerAy);
        
        painter.end();
    }

};

#endif