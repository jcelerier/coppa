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

template<typename Device>
void exposeQObject(Device& dev, QObject* obj)
{
  auto path = computeObjectPath(obj).toStdString();
  qDebug() << QString::fromStdString(path);

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

    dev.addHandler(
          p.destination,
          [&, wrapper = QPointer<QObject>(obj)] (const Parameter& p) {
      if(!wrapper)
      {
        dev.removeHandler(p.destination);
        return;
      }

      auto& elt = p.values.front();
      switch(elt.which())
      {
        case 0:
          wrapper->setProperty(p.description.c_str(), eggs::variants::get<int>(elt));
          break;
        case 1:
          wrapper->setProperty(p.description.c_str(), eggs::variants::get<float>(elt));
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
int main(int argc, char** argv)
{
  // Set-up Qt stuff
  QApplication app{argc, argv};
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
  g2->setObjectName("mammamia");
  layout->addWidget(g1);
  g1->layout()->addWidget(g2);

  // Set-up coppa
  SynchronizingLocalDevice<WebSocketServer, Answerer> dev;

  exposeQObject(dev, g1);
  exposeQObject(dev, g2);

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
