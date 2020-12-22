#ifndef Worker_H
#define Worker_H

//#define GSEARCH_URL "https://maps.googleapis.com/maps/api/geocode/json?address=%1"
//#define OSSEARCH_URL "http://nominatim.openstreetmap.org/search?q=%1"
//#define GSUGGEST_URL "http://google.com/complete/search?output=toolbar&q=%1"

#include <QObject>
//#include <QAxObject>
#include <QVariant>
#include <QFileInfo>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDomDocument>
#include <QDesktopServices>

using namespace std;
class Worker : public QObject
{
    Q_OBJECT
    void go();
   // QUrl url;
    //QNetworkReply *reply;
    void httpReadyRead();
    void startRequest(const QUrl url);
    void go4txt();
    void go4txtRev();
    void TraverseXmlNode(const QDomNode &node);
public:
    Worker();
    ~Worker();
    bool httpRequestAborted;
    //QAxObject* excel, *workbooks, *workbook, *sheets , *sheet , *range;
    QStringList addresses;
    int col;
    bool skip;
    bool isRev;
    QFileInfo file;
signals:
    void errMessage(const QString &s, int type=0);
    void resultReady(const QStringList &s);
    void valueChanged(int newValue);
    void finished();

public slots:
    void process();
    void stopProgress();



};

#endif // Worker_H
