#ifndef CURLER_H
#define CURLER_H

#define ADDRESS_CACHE ".cachedaddresses"
#define ADDRESSERR_CACHE ".cachederraddresses"

#include "math.h"
#include <QObject>
#include <QDataStream>
#include <QHash>
#include <QStringList>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDomDocument>
#include <QUrl>
#include "curl/curl.h"

struct MemoryStruct {
  char *memory;
  size_t size;
};

namespace
{
    inline double getm(double rlat)
    {
      return 111132.92 - 559.82 * cos(2* (rlat*3.141593/180)) + 1.175*cos(4*(rlat*3.141593/180));
    }

    inline double getm2(double rlat)
    {
      return 111412.84 * cos((rlat*3.141593/180)) - 93.5 * cos(3*(rlat*3.141593/180));
    }


    static size_t
    WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
    {
      size_t realsize = size * nmemb;
      struct MemoryStruct *mem = (struct MemoryStruct *)userp;

      char *ptr = (char * )realloc(mem->memory, mem->size + realsize + 1);
      if(ptr == NULL) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
      }

      mem->memory = ptr;
      memcpy(&(mem->memory[mem->size]), contents, realsize);
      mem->size += realsize;
      mem->memory[mem->size] = 0;

      return realsize;
    }

    size_t writeCallback2(char* contents, size_t size, size_t nmemb, std::string* ibuffer)
    {
      size_t realsize = size * nmemb;
      ibuffer = new std::string();
      if(ibuffer == NULL) {
        return 0;
      }
      ibuffer->append(contents, realsize);
      return realsize;
    }
//    size_t header_callback(char *buffer, size_t size,
//                                  size_t nitems, void *userdata)
//    {
//      /* received header is nitems * size long in 'buffer' NOT ZERO TERMINATED */
//      /* 'userdata' is set with CURLOPT_HEADERDATA */
//        FILE* fp;
//        fp=fopen("D:\\header.txt", "r");
//        fprintf(fp, "--%s", buffer);
//        fclose(fp);
//        return nitems * size;
//    }
}

class curler : public QObject
{
    Q_OBJECT
    void geocode(QString value);
    void getFromGoogleJSON(QJsonObject jsonObject);
    void getFromBingJSON(QJsonObject jsonObject);
    void cacheNwrite();
    bool overqlimit=false;
    QStringList APIS;
    QString strurl;
    QStringList keys;
    QStringList bingkeys, hereKeys;

public:
    curler();
    ~curler();

    CURL *curl;
    CURLcode res;
    int httpCode=0;
    QString buffer="";
    bool *terminated;
    bool httpRequestAborted;
    int addressCount;
    int timeout=5;
    bool bing=false;
    int waitsecs = 1;
    int apitype=0; // 0= google 1=revgoogle 2=bing 3=superbing....
    bool isReverse=false;
    int endcode=0;  // 0=ok, 1=terminato da utente 2=curl 3=API 4=chiavi terminate e limite query 5=limite query
    QStringList addresses;
    QStringList *coords;
    QHash<QString, QString> cachedAddressMap ;
    QHash<QString, QString> cachedErrAddressMap ;
    int googlekeyn = 0;
    int bingkeyn = 0;
    QFile *cacheFile, *cacheErrFile;

signals:
    void logit(const QString &s, int type=0);
    void resultReady(const QStringList &s);
    void valueChanged(int newValue);
    void ended(int code);

public slots:
    void process();
};

#endif // CURLER_H
