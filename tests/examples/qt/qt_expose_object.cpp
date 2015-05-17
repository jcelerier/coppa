#include <QtWidgets>
#include <coppa/oscquery/device/local.hpp>
#include <coppa/protocol/websockets/server.hpp>

int main(int argc, char** argv)
{
  QApplication app{argc, argv};
  // Set-up Qt stuff
  auto window = new QMainWindow;
  auto widget = new QWidget;
  window->setCentralWidget(widget);
  auto layout = new QHBoxLayout{widget};
  auto label = new QLabel{"0"};
  auto pushbutton = new QPushButton{"Count"};

  int32_t count{};
  QObject::connect(pushbutton, &QPushButton::clicked,
                   [&label, &count] () {
    label->setText(QString::number(++count));
  });

  layout->addWidget(label);
  layout->addWidget(pushbutton);

  window->show();

  // Set-up coppa
  using namespace coppa;
  using namespace coppa::oscquery;
  SynchronizingLocalDevice<WebSocketServer, Answerer> dev;
  Parameter p;
  p.destination = "/label/value";
  p.description = "Value of a label";
  p.tags.push_back("GUI");
  addValue(p, count);

  QObject::connect(pushbutton, &QPushButton::clicked,
                   [&] () {
    Values v;
    v.values.push_back(++count);
    dev.update(p.destination, v);

    label->setText(QString::number(count));
  });

  dev.device().receiver().addHandler(
        p.destination,
        [&] (osc::ReceivedMessageArgumentStream s) {
    Values v;
    int val;
    s >> val;
    v.values.push_back(val);
    dev.update(p.destination, v);

    label->setText(QString::number(count));
  });

  dev.add(p);
  std::thread ([&] () { dev.expose(); });

  return app.exec();
}
