#ifndef RECEIVER_H
#define RECEIVER_H

#include <QObject>

class Receiver : public QObject
{
    Q_OBJECT
public:
    explicit Receiver(QObject *parent = 0);

signals:
    void UpdateCore( char *, int );

public slots:
    void Check();
};

#endif // SERVER_H
