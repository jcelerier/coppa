#pragma once
#include <QObject>
#include <QVariant>
#include <QPointer>

Q_DECLARE_METATYPE(QPointer<QObject>)
class ThreadManager : public QObject
{
        Q_OBJECT
    public:
        ThreadManager();

    signals:
        void setPropAsync(QPointer<QObject>, QString, QVariant);

    public slots:
        void on_setProp(QPointer<QObject>, QString, QVariant);
};
