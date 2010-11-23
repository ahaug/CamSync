#include "ScrollArea.h"
#include "Viewfinder.h"
#include "ThumbnailView.h"
#include "CameraThread.h"

ScrollArea::ScrollArea(QWidget *parent) :
    QWidget(parent) {

    contents = new QWidget(this);
    current = 0;

    // Make an animation that moves the scrollArea up and down
    animation = new QPropertyAnimation(contents, "geometry", this);
    QObject::connect(animation, SIGNAL(finished()),
                     this, SLOT(animationFinished()));
}


void ScrollArea::jumpTo(int idx) {
    // Can't jump while being dragged. That pins the widget to the user's finger.    
    if (dragging == this) return;
    if (idx < 0) jumpTo(0);
    if (idx >= (int)childWidgets.size()) jumpTo(childWidgets.size()-1);
    QWidget *w = childWidgets[idx];
    contents->setGeometry(-w->x(), -w->y(), contents->width(), contents->height());    
    current = idx;
}

void ScrollArea::jumpTo(QWidget *w) {
    for (int i = 0; i < (int)childWidgets.size(); i++) {
        if (childWidgets[i] == w) {
            jumpTo(i);
            return;
        }
    }
    printf("Error: Asked to jump to a non-child widget\n");
}

void ScrollArea::animationFinished() {
    emit slidTo(current);
    emit slidTo(childWidgets[current]);
}

void ScrollArea::slideTo(int idx) {
    // Can't slide while being dragged. That pins the widget to the user's finger.
    if (dragging == this) return;
    if (idx < 0) slideTo(0);
    if (idx >= (int)childWidgets.size()) slideTo(childWidgets.size()-1);

    QWidget *w = childWidgets[idx];
    current = idx;    
    animation->setDuration(250);
    animation->setEasingCurve(QEasingCurve::InOutQuad);
    animation->setStartValue(contents->geometry());
    animation->setEndValue(QRect(-w->x(), -w->y(), contents->width(), contents->height()));
    animation->start();
}

void ScrollArea::slideTo(QWidget *w) {
    for (int i = 0; i < (int)childWidgets.size(); i++) {
        if (childWidgets[i] == w) {
            slideTo(i);
            return;
        }
    }
}


ScrollArea *ScrollArea::dragging = NULL;

void VScrollArea::addWidget(QWidget *w) {
    w->setParent(contents);
    w->setGeometry(0, childWidgets.size()*height(), width(), height());
    childWidgets.push_back(w);    
    contents->setGeometry(0, -current*height(), width(), height()*childWidgets.size());
    w->show();
}

void HScrollArea::addWidget(QWidget *w) {
    w->setParent(contents);
    w->setGeometry(childWidgets.size()*width(), 0, width(), height());
    childWidgets.push_back(w);    
    contents->setGeometry(-current*width(), 0, width()*childWidgets.size(), height());
}

void ScrollArea::mousePressEvent(QMouseEvent *ev) {
    //printf("scroll area press\n");
    //ev->ignore(); return;
    mouseAnchor = ev->globalPos();
    ev->ignore();
}

void VScrollArea::mouseReleaseEvent(QMouseEvent *ev) {
    //printf("scroll area release\n");
    //ev->ignore(); return;
    if (dragging == this) {
        qreal dy = ev->globalY() - filteredMouseAnchor.y();
        // snap according to whether the swipe was up or down
        animation->setStartValue(contents->geometry());

        int delta = 0;
        if (dy > 3 && current > 0 && contents->y() > -height()*current) {
            delta = -1;
        } else if (dy < -3 && current < (int)childWidgets.size() - 1 && contents->y() < -height()*current) {
            delta = +1;
        } else if (abs(dy) < 3) {
            // snap to closest
            int dUp   = abs(contents->y() + height()*(current-1));
            int dHere = abs(contents->y() + height()*current);
            int dDown = abs(contents->y() + height()*(current+1));
            if (dUp < dHere && dUp < dDown) {
                delta = -1;
            } else if (dDown < dHere) {
                delta = +1;
            } else {
            }
        }

        current += delta;
        animation->setEndValue(QRect(contents->x(), -current*height(), 
                                     contents->width(), contents->height()));
        animation->setDuration(100);
        animation->setEasingCurve(QEasingCurve::OutQuad);
        animation->start();
        dragging = NULL;
    } else {
        ev->ignore();
    }
}

void HScrollArea::mouseReleaseEvent(QMouseEvent *ev) {
    
    if (dragging == this) {
        qreal dx = ev->globalX() - filteredMouseAnchor.x();
        // snap according to whether the swipe was left or right
        animation->setStartValue(contents->geometry());

        int delta = 0;
        if (dx > 3 && current > 0 && contents->x() > -width()*current) {
            delta = -1;
        } else if (dx < -3 && current < (int)childWidgets.size() - 1 && contents->x() < -width()*current) {
            delta = +1;
        } else if (abs(dx) < 3) {
            // snap to closest
            int dLeft  = abs(contents->x() + width()*(current-1));
            int dHere  = abs(contents->x() + width()*current);
            int dRight = abs(contents->x() + width()*(current+1));
            if (dLeft < dHere && dLeft < dRight) {
                delta = -1;
            } else if (dRight < dHere) {
                delta = +1;
            } else {
            }
        }

        current += delta;
        animation->setEndValue(QRect(-current*width(), contents->y(),
                                     contents->width(), contents->height()));
        animation->setDuration(100);
        animation->setEasingCurve(QEasingCurve::OutQuad);
        animation->start();
        dragging = NULL;
    } else {
        ev->ignore();
    }

}

void VScrollArea::mouseMoveEvent(QMouseEvent *ev) {
    //printf("scroll area mouse move event\n");
    //ev->ignore(); return;
    int dx = ev->globalX() - mouseAnchor.x();
    int dy = ev->globalY() - mouseAnchor.y();

    if (abs(dy) > abs(dx) && abs(dy) > 40 && dragging == NULL) {
        dragging = this;
        filteredMouseAnchor = mouseAnchor;
    } else if (dragging != this) {
        ev->ignore();
        return;
    }

    if (dragging == this) {
        // stop any animation
        animation->stop();

        int newY = contents->y()+dy;
        if (newY > 0) newY = 0;
        if (newY < height()-contents->height()) newY = height()-contents->height();
        contents->setGeometry(contents->x(), newY, contents->width(), contents->height());
        mouseAnchor = ev->globalPos();
        filteredMouseAnchor += mouseAnchor;
        filteredMouseAnchor *= 0.5;
    } else {
        ev->ignore();
    }
}

void HScrollArea::mouseMoveEvent(QMouseEvent *ev) {
    int dx = ev->globalX() - mouseAnchor.x();
    int dy = ev->globalY() - mouseAnchor.y();
    
    if (abs(dy) < abs(dx) && abs(dx) > 40 && dragging == NULL) {
        dragging = this;
        filteredMouseAnchor = mouseAnchor;
    } else if (dragging != this) {
        ev->ignore();
        return;
    }

    if (dragging == this) {
        // stop any animation
        animation->stop();

        int newX = contents->x()+dx;
        if (newX > 0) newX = 0;
        if (newX < width()-contents->width()) newX = width()-contents->width();
        contents->setGeometry(newX, contents->y(), contents->width(), contents->height());
        mouseAnchor = ev->globalPos();
        filteredMouseAnchor += mouseAnchor;
        filteredMouseAnchor *= 0.5;
    } else {
        ev->ignore();
    }
}
