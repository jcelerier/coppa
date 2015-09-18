#include <QtWidgets>
#include <coppa/oscquery/device/local.hpp>
#include <coppa/protocol/websockets/server.hpp>

#include <QDebug>
#include <QPointer>
#include "ThreadManager.hpp"

using namespace coppa::oscquery;

/**
 * @brief computeObjectPath Path of an object
 * @param obj a QObject in a named object tree.
 *
 * @return The path of the QObject from the first object that does not have a objectName.
 */
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

template<typename Device>
/**
 * @brief exposeQObject Exposes a QObject over a query server
 * @param dev A query server device
 * @param obj The object to expose
 * @param mgr Handles thread-safety for Qt
 */
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
        addValue(p, prop.read(obj).toInt(), {0, 100, {}});
        break;
      case QVariant::Type::Double:
        addValue(p, prop.read(obj).toFloat(), {0.f, 100.f, {}});
        break;
      case QVariant::Type::String:
        addValue(p, prop.read(obj).toString().toStdString());
        break;
      default:
        break;
    }

    dev.insert(p);

    // Connect the changes of parameters with Qt.
    dev.add_handler(
          p.destination,
          [&, mgr, wrapper = QPointer<QObject>(obj)] (const Parameter& p) {
      if(!wrapper)
      {
        dev.remove_handler(p.destination);
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
/**
 * @brief The SomeObject class
 *
 * A simple graphics square.
 */
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

  // Create the objects that we will expose.
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
  synchronizing_local_device dev;

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

  dev.insert(p);

  // update the value from the gui.
  QObject::connect(pushbutton, &QPushButton::clicked,
                   [&] () {
    Values v;
    v.values.push_back(++count);
    dev.update_attributes(p.destination, v);
  });

  // Update the label when the data is updated.
  dev.add_handler(
        p.destination,
        [&] (const Parameter& p) {
    label->setText(QString::number(eggs::variants::get<int>(p.values.front())));
  });

  // Start coppa.
  std::thread t([&] () { dev.expose(); });

  return app.exec();
}
