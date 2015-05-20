#include <QtWidgets>
#include <coppa/oscquery/device/local.hpp>
#include <coppa/protocol/websockets/server.hpp>

#include <QPointer>
using namespace coppa;
using namespace coppa::oscquery;

#include <QDebug>
QString computeObjectPath(QObject* obj)
{
  QStringList names;
  while(obj)
  {
    QString name = obj->objectName();
    if(name.isEmpty())
      break;

    names += obj->objectName();
    obj = obj->parent();
  }

  std::reverse(names.begin(), names.end());
  return "/" + names.join("/");
}

#include "ThreadManager.hpp"
template<typename Device>
void exposeQObject(Device& dev, QObject* obj, ThreadManager* mgr)
{
  auto path = computeObjectPath(obj).toStdString();

  // Add nodes for each property
  for(int i = 0; i < obj->metaObject()->propertyCount(); i++)
  {
    auto prop = obj->metaObject()->property(i);

    Parameter p;
    p.accessmode = Access::Mode::Both;
    p.destination = path + "/property/" + prop.name();
    p.description = prop.name();
    switch(prop.type())
    {
      case QVariant::Type::Int:
        addValue(p, prop.read(obj).toInt());
        break;
      case QVariant::Type::Double:
        addValue(p, prop.read(obj).toFloat());
        break;
      case QVariant::Type::String:
        addValue(p, prop.read(obj).toString().toStdString());
        break;
      default:
        break;
    }

    dev.add(p);

    //Connect the changes of parameters with Qt.
    dev.addHandler(
          p.destination,
          [&, mgr, wrapper = QPointer<QObject>(obj)] (const Parameter& p) {
      if(!wrapper)
      {
        dev.removeHandler(p.destination);
        return;
      }
      auto& elt = p.values.front();
      switch(elt.which())
      {
        case 0:
          mgr->setPropAsync(wrapper, QString::fromStdString(p.description), eggs::variants::get<int>(elt));
          break;
        case 1:
          mgr->setPropAsync(wrapper, QString::fromStdString(p.description), eggs::variants::get<float>(elt));
          break;
          //case 2: array.add(get<bool>(val)); break;
        case 3:
          auto variant_str = eggs::variants::get<std::string>(elt);
          auto qstr =  QString::fromStdString(variant_str);
          wrapper->setProperty(p.description.c_str(), qstr);
          break;
          //case 4: array.add(get<const char*>(val)); break;
      }
    });
  }
}

#include <QFrame>
#include <QLabel>
#include <QGraphicsObject>

class SomeObject : public QGraphicsObject
{
    public:
        virtual QRectF boundingRect() const
        {
            return {0, 0, 50, 50};
        }

        virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
        {
            painter->setPen(Qt::black);
            painter->drawRect(boundingRect());
        }
};

int main(int argc, char** argv)
{
  // Set-up Qt stuff
  QApplication app{argc, argv};
  qRegisterMetaType<QPointer<QObject>>();
  auto window = new QMainWindow;
  auto widget = new QWidget;
  window->setCentralWidget(widget);
  auto layout = new QGridLayout{widget};
  auto label = new QLabel{"0"};
  auto pushbutton = new QPushButton{"Count"};

  layout->addWidget(label);
  layout->addWidget(pushbutton);

  window->show();

  auto g1 = new QFrame;
  g1->setObjectName("laPampa");
  g1->setLayout(new QHBoxLayout);
  auto g2 = new QLabel(g1);
  g2->setText("A text");
  g2->setObjectName("mammamia");
  layout->addWidget(g1);
  g1->layout()->addWidget(g2);

  auto g3 = new SomeObject;
  g3->setObjectName("GraphicsSquare");
  QGraphicsScene* s = new QGraphicsScene;
  s->addItem(g3);

  //g3->setPos(0, 0);
  g3->setPos(100, 100);
  g3->setRotation(45);
  QGraphicsView* v = new QGraphicsView(s);
  v->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  g1->layout()->addWidget(v);

  ThreadManager* mgr = new ThreadManager;
  // Set-up coppa
  SynchronizingLocalDevice<WebSocketServer, Answerer> dev;

  // Automatically expose some objects
  exposeQObject(dev, g1, mgr);
  exposeQObject(dev, g2, mgr);
  exposeQObject(dev, g3, mgr);

  // Manually expose another object
  // Add a corresponding parameter
  Parameter p;
  p.destination = "/label/value";
  p.description = "Value of a label";
  p.tags.push_back("GUI");

  int32_t count{};
  addValue(p, count);

  dev.add(p);

  // Update the value from the gui.
  QObject::connect(pushbutton, &QPushButton::clicked,
                   [&] () {
    Values v;
    v.values.push_back(++count);
    dev.update_attributes(p.destination, v);
  });

  // Update the label when the data is updated.
  dev.addHandler(
        p.destination,
        [&] (const Parameter& p) {
    label->setText(QString::number(eggs::variants::get<int>(p.values.front())));
  });

  // Start coppa.
  std::thread t([&] () { dev.expose(); });

  return app.exec();
}
