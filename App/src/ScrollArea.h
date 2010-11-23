#ifndef SCROLL_AREA_H
#define SCROLL_AREA_H

#include <QWidget>
#include <QPropertyAnimation>
#include <QMouseEvent>
#include <vector>

/* A ScrollArea is a container widget that displays one child at a
 * time. The user can swipe left or right (in the case of an
 * HScrollArea) or up and down (for a VScrollArea) to display
 * different children. We use a VScrollArea to hold the three
 * different fcamera screens (ExtendedSettings, Viewfinder, and
 * ThumbnailView), and an HScrollArea within the ThumbnailView to show
 * pictures taken. */
class ScrollArea: public QWidget {
    Q_OBJECT

public:
    ScrollArea(QWidget *parent = 0);

    // Adds a new child widget. The child will be resized to be the
    // same size as the scroll area, and will have its parent set to
    // the scroll area instance
    virtual void addWidget(QWidget *) = 0;

    void mousePressEvent(QMouseEvent *);

public slots:
    // Teleport to a given child by index or pointer
    void jumpTo(int);
    void jumpTo(QWidget *);

    // Perform an animated slide to a given child by index or
    // pointer. Emits slidTo when done.
    void slideTo(int);
    void slideTo(QWidget *);

signals:
    // These signals are emitted when the contents are slid and the
    // animation finishes. They are never emitted due to a call
    // jumpTo.
    void slidTo(int);
    void slidTo(QWidget *);

protected slots:
    void animationFinished();

protected:
    QWidget *contents;
    std::vector<QWidget *> childWidgets;
    int current;
    QPropertyAnimation *animation;
    QPoint mouseAnchor;
    QPointF filteredMouseAnchor;

    static ScrollArea *dragging;
};

class HScrollArea : public ScrollArea {
public:
    HScrollArea(QWidget *parent = 0) : ScrollArea(parent) {}
    void addWidget(QWidget *);
protected:
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
};

class VScrollArea : public ScrollArea {
public:
    VScrollArea(QWidget *parent = 0) : ScrollArea(parent) {}
    void addWidget(QWidget *);
protected:
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);    
};

#endif
