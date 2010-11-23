#ifndef OVERLAY_WIDGET_H
#define OVERLAY_WIDGET_H

#include <QWidget>
#include <QX11Info>

#define __user
#include "../linux/omapfb.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <FCam/Image.h>

/** This widget manages an fbdev YUV overlay, suitable for drawing
 * viewfinder frames on. */
class OverlayWidget : public QWidget {
  public:
    OverlayWidget(QWidget *parent = NULL);
    ~OverlayWidget();

    // If you draw on a widget at the same place as this one, using
    // any color but the one below, it will show through the overlay.
    static QColor colorKey() {return QColor(10, 0, 10);}

    // A reference to the frame buffer. Modifying this image will
    // update what's visible on screen immediately. (i.e., there's no
    // double-buffering).
    FCam::Image framebuffer();
    
 protected:
    struct omapfb_color_key old_color_key;
    
    void resizeEvent(QResizeEvent *);
    void moveEvent(QMoveEvent *);
    void showEvent(QShowEvent *);
    void hideEvent(QHideEvent *);
    bool eventFilter(QObject *receiver, QEvent *event);

    void enable();    
    void disable();

    FCam::Image framebuffer_;

    //struct fb_var_screeninfo var_info;
    struct fb_var_screeninfo overlay_info;
    struct omapfb_mem_info mem_info;
    struct omapfb_plane_info plane_info;
    int overlay_fd;

    bool filterInstalled;
};

#endif
