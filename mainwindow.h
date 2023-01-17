#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <QString>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDropEvent>
#include <QFileInfo>
#include <QFile>
#include <QMessageBox>
#include <QVariantList>
#include <QList>
#include <QUrl>
#include <QDateTime>
#include <QProgressDialog>
#include <QLineEdit>
#include <QProgressDialog>
#include <QThread>
#include <worker.h>
#include <QDataStream>
#include <QDebug>
#include <QHash>
#include <QtSvg>
#include <QStringList>
#include "curler.h"
#include <dialog.h>


namespace Ui {
class MainWindow;
}
using namespace std;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    inline double getm(double rlat)
    {
      return 111132.92 - 559.82 * cos(2* (rlat*3.141593/180)) + 1.175*cos(4*(rlat*3.141593/180));
    }

    inline double getm2(double rlat)
    {
      return 111412.84 * cos((rlat*3.141593/180)) - 93.5 * cos(3*(rlat*3.141593/180));
    }
public:
    explicit MainWindow(QWidget *parent = 0);
  //  QGeoRectangle rect;
  // QGeoCodeReply *geo(QString address);

    QHash<QString, QString> cachedAddressMap ;
    QHash<QString, QString> cachedErrAddressMap ;


    QStringList errorscurl;
    QStringList logs;
    QStringList addresses;
    QStringList coords;
    QFileInfo fni;
    bool terminated=false;
    int addressCount;
    bool overqlimit;
    QFile cacheFile, cacheErrFile;
  //  QNetworkAccessManager *manager;
 //   QNetworkReply *reply;
    ~MainWindow();
    void downloadFile();
    void readExcel(QFileInfo file);
public slots:
    void logit(QString mes, int type=0);
    void processFinished();
    void setAddresses(QStringList ql);
    void setProgress(int val);
    void endGeocode(int code);
  //  void handleNetworkData(QJsonDocument jsonResponse);
protected:
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);
private slots:
 //   void cancelDownload();
    void resetCache();
    void cancelCurl();
    void about();
Q_SIGNALS:
    void valueChanged(int newValue);
private:
    Ui::MainWindow *ui;
    bool checkDate();
    bool isReverse;
    Dialog *dialog;
    QProgressDialog *progressDialog;
    QStringList errors;
    QPushButton *downloadButton;
    QDataStream dataStream;
    const QString completeAddresses = "";
    bool httpRequestAborted;
 //   void geocode(QString value);
    QString urlst ;
    int googlekeyn = 0;
    QHash<QString, QString> readCache();
 //   void writeCache(QString key, QString val);
 //   void writeErrCache(QString key, QString val);
    void getFromGoogleJSON(QJsonObject jsonObject);
    void getFromBingJSON(QJsonObject jsonObject);
    void writeCSV();
};

#endif // MAINWINDOW_H
