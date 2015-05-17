#include <QtWidgets>
#include <coppa/oscquery/device/local.hpp>
#include <coppa/protocol/websockets/server.hpp>

int main(int argc, char** argv)
{
  // Set-up Qt stuff
  QApplication app{argc, argv};
  auto window = new QMainWindow;
  auto widget = new QWidget;
  window->setCentralWidget(widget);
  auto layout = new QHBoxLayout{widget};
  auto label = new QLabel{"0"};
  auto pushbutton = new QPushButton{"Count"};

  layout->addWidget(label);
  layout->addWidget(pushbutton);

  window->show();

  // Set-up coppa
  using namespace coppa;
  using namespace coppa::oscquery;
  SynchronizingLocalDevice<WebSocketServer, Answerer> dev;

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
