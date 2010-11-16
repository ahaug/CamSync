#ifndef MAINVIEWERWINDOW_H
#define MAINVIEWERWINDOW_H

#include <QMainWindow>

class QListWidgetItem;
class QLabel;
class QPropertyAnimation;

namespace Ui {
    class MainViewerWindow;
}

class MainViewerWindow : public QMainWindow {
    Q_OBJECT

public:
    enum View {
        Fileview,
        Imageview
    };

public:
    MainViewerWindow(QWidget *parent = 0);
    ~MainViewerWindow();

public slots:
    void showImageView(QListWidgetItem *item);
    void showFileView();

protected:
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void resizeEvent(QResizeEvent *event);

private:
    void updateFileList();
private:
    Ui::MainViewerWindow *ui;
    QImage image;
};

#endif // MAINVIEWERWINDOW_H
