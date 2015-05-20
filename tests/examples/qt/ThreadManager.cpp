#include "ThreadManager.hpp"

ThreadManager::ThreadManager()
{
    connect(this, SIGNAL(setPropAsync(QPointer<QObject>,QString,QVariant)),
            this, SLOT(on_setProp(QPointer<QObject>,QString,QVariant)), Qt::QueuedConnection);
}

#include <QDebug>
void ThreadManager::on_setProp(QPointer<QObject> obj, QString str, QVariant prop)
{
    obj->setProperty(str.toLatin1().constData(), prop);

}
