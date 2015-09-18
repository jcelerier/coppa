#include <chrono>
#include <thread>
#include <QHostInfo>
#include <QCoreApplication>
#include <KF5/KDNSSD/DNSSD/ServiceBrowser>
#include <KF5/KDNSSD/DNSSD/ServiceModel>
#include <coppa/oscquery/device/remote.hpp>


class MainObj : public QObject
{
  public:
    MainObj()
    {
      using namespace KDNSSD;
      auto browser = new ServiceBrowser("_coppa._tcp");
      connect(browser, &ServiceBrowser::serviceAdded,
              this, [] (auto service)
      {
        RemoteService* data = service.data();
        data->resolve();

        auto ipAddressesList = QHostInfo::fromName(data->hostName()).addresses();
        QString ipAddress;

        qDebug() << data->hostName() << ipAddressesList;

        if(!ipAddressesList.empty())
        {
          for(int i = 0; i < ipAddressesList.size(); ++i)
          {
            if(ipAddressesList.at(i).toIPv4Address())
            {
              ipAddress = ipAddressesList.at(i).toString();
              break;
            }
          }
        }
        else
        {
          ipAddress = "127.0.0.1";
        }

        qDebug() << data->isResolved()<< ipAddress << data->port();

        using namespace std;
        using namespace coppa;
        using namespace coppa::oscquery;

        QString addr = QString("http://") + ipAddress + ":" + QString::number(data->port());
        remote_device dev(addr.toStdString());
        dev.query_connect_async();
        while(!dev.query_is_connected())
          this_thread::sleep_for(chrono::milliseconds(100));

        dev.query_request_namespace();

        this_thread::sleep_for(chrono::seconds(1));

        cerr <<  dev.size() << endl;
        exit(0);

      });

      browser->startBrowse();
    }


};

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    MainObj obj;
    return app.exec();
}
