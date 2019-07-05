#ifndef RENDERAREA_H
#define RENDERAREA_H

#include <QtWidgets>

#include "Solver.h"

#define PATH_TO_RES "../src/"
#define DROPLET_IMG "res/droplet.png"
#define DETECTOR_IMG "res/detector.png"
#define DETECTING_IMG "res/detecting.png"
#define DISPENSER_IMG "res/dispenser.png"
#define SINK_IMG "res/sink.png"
#define MIXING_IMG "res/mixing.png"
#define EMPTY_IMG "res/empty.png"

class RenderArea : public QWidget
{
    Q_OBJECT

public:
    enum Shape { Line, Points, Polyline, Polygon, Rect, RoundedRect, Ellipse, Arc,
                 Chord, Pie, Path, Text, Pixmap };

    RenderArea(QWidget *parent = 0);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    // data from solver
    std::vector<std::vector<int>> gridData;
    std::vector<Node> sinkDispData;
    std::vector<std::vector<std::pair<bool, std::string>>> detectorData;
    void do_update() { update(); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    Shape shape;
    QPen pen;
    QBrush brush;
    bool antialiased;
    bool transformed;
    QPixmap pixmap;

    QImage droplet_img;
    QRect droplet_src;
    QImage detector_img;
    QRect detector_src;
    QImage detecting_img;
    QRect detecting_src;
    QImage dispenser_img;
    QRect dispenser_src;
    QImage sink_img;
    QRect sink_src;
    QImage mixing_img;
    QRect mixing_src;
    QImage empty_img;
    QRect empty_src;

};

#endif // RENDERAREA_H

