#ifndef TITLE_H
#define TITLE_H

#include <QWidget>

class Title : public QWidget {

    Q_OBJECT

public:
    explicit Title(QWidget *parent = 0);

protected:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *event);

private:
    int offset;
    int w;
    int h;
    QPoint sp;
    QString s;

signals:
    void MouseShow();

public slots:
    void ChangeTitle( QString );
};

#endif // TITLE_H
