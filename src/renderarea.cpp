#include "renderarea.h"

#include <QPainter>
#include <string>
#include <cstdio>

using namespace std;

RenderArea::RenderArea(QWidget *parent)
    : QWidget(parent)
{
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);

    droplet_img.load((string("") + PATH_TO_RES + DROPLET_IMG).c_str());
    droplet_src = QRect(0, 0, 600, 600);
    detector_img.load((string("") + PATH_TO_RES + DETECTOR_IMG).c_str());
    detector_src = QRect(0, 0, 225, 225);
    detecting_img.load((string("") + PATH_TO_RES + DETECTING_IMG).c_str());
    detecting_src = QRect(0, 0, 225, 225);
    dispenser_img.load((string("") + PATH_TO_RES + DISPENSER_IMG).c_str());
    dispenser_src = QRect(0, 0, 256, 256);
    sink_img.load((string("") + PATH_TO_RES + SINK_IMG).c_str());
    sink_src = QRect(0, 0, 225, 225);
    mixing_img.load((string("") + PATH_TO_RES + MIXING_IMG).c_str());
    mixing_src = QRect(0, 0, 225, 225);
    empty_img.load((string("") + PATH_TO_RES + EMPTY_IMG).c_str());
    empty_src = QRect(0, 0, 225, 225);
}

QSize RenderArea::minimumSizeHint() const
{
    return QSize(100, 100);
}

QSize RenderArea::sizeHint() const
{
    return QSize(400, 400);
}

void getPos(int& x, int& y, int w, int m, int n, int z){
    if(z <= m-1){
        y = 0;
        x = w*(z+1);
    }
    else if(z<=m+n-1){
        x = w*(m+1);
        y = w*(z-m+1);
    }
    else if(z<=2*m + n -1){
        y = w*(n+1);
        x = w*(2*m+n-z);
    }
    else if(z<=2*m + 2*n -1){
        x = 0;
        y = w*(2*m+2*n-z);
    }
}

void RenderArea::paintEvent(QPaintEvent * /* event */)
{
    QPainter painter(this);

    if(gridData.size() == 0){
        return;
    }

    int D = width() / (gridData.size()+2);
    int w = gridData[0].size();
    int h = gridData.size();
    for(int p = 0; p < sinkDispData.size(); p++){
        int x, y;
        getPos(x, y, D, w, h, p);
        QRect target(x, y, D, D);
        if(sinkDispData[p].type_ == 1)
            painter.drawImage(target, sink_img, sink_src);
        else if(sinkDispData[p].type_ == 2)
            painter.drawImage(target, dispenser_img, dispenser_src);
    }

    for(int y = 0; y < detectorData.size(); y++){
        for(int x = 0; x < detectorData.size(); x++){
            if(detectorData[y][x].first && !(gridData[y][x]==-1)){
                QRect target((x+1)*D, (y+1)*D, D, D);
                painter.drawImage(target, detector_img, detector_src);
            }
        }
    }

    for(int y = 0; y < gridData.size(); y++){
        for(int x = 0; x < gridData[y].size(); x++){
            if(gridData[y][x] == -2){ // mixing
                QRect target((x+1)*D, (y+1)*D, D, D);
                painter.drawImage(target, mixing_img, mixing_src);
            }else if(gridData[y][x] == -1){ // detecting
                QRect target((x+1)*D, (y+1)*D, D, D);
                painter.drawImage(target, detecting_img, detecting_src);
            }else if(gridData[y][x] >= 0){
                QRect target((x+1)*D, (y+1)*D, D, D);
                painter.drawImage(target, droplet_img, droplet_src);
                char id[10];
                sprintf(id, "%d", gridData[y][x]);
                painter.drawText(target, Qt::AlignCenter, id);
            }else if(!detectorData[y][x].first){
                QRect target((x+1)*D, (y+1)*D, D, D);
                painter.drawImage(target, empty_img, empty_src);
            }
        }
    }
}
