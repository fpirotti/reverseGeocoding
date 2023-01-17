#include "curler.h"
#include <unistd.h>
 
curler::curler()
{

    httpRequestAborted=false;
    addressCount=0;
    // la prima di massive geocoding api A2A google cloud
    keys   << QString("putYourApikeyhere")
 
    bingkeys  << QString("putYourApikeyhere")
 
    hereKeys  << QString("putYourApikeyhere");
    
    APIS << QString("https://maps.googleapis.com/maps/api/geocode/json?key=%2&region=IT&address=%1")
      << QString("https://maps.googleapis.com/maps/api/geocode/json?latlng=%1&key=%2&region=IT")
       << QString("http://dev.virtualearth.net/REST/v1/Locations?CountryRegion=IT&addressLine=%1&key=%2")
       << QString("https://www.bing.com/maps/overlay?q=%1")
       << QString("https://geocoder.ls.hereapi.com/6.2/geocode.json?apiKey=%2&searchtext=%1");


    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);

 //   curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    if(!curl){
        logit("errore!!", 2);
    }

}
// --- DECONSTRUCTOR ---
curler::~curler() {
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    // free resources
}
void curler::process() {


    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    addressCount=0;
    //logit(QString("Address length %1, count %2").arg(this->addresses.length()).arg(addressCount));
    while( addressCount<addresses.length() ){
        if(*terminated) endcode=1;
        if(endcode!=0) {
            break;
        }
        geocode(addresses.at(addressCount));
        // qui sotto in caso un temporaneo utilizzo di BING per risolvere un indirizzo NON trovato.
        if(apitype==0 && bing==true) {
            this->apitype=2;
            geocode(addresses.at(addressCount));
            this->apitype=0;
            bing=false;
        }

        if(endcode!=0) {
            break;
        }
        addressCount++;
    }
     emit valueChanged( addressCount );
    emit ended(endcode);
}





void curler::geocode(QString value)
{
    // asterisco!!!
    if( value.at(0).toLatin1() == '#')
    {
        coords->append(QString("#\"\"\t\"\"\t\"#\""));
        return;
    }
    if( value.at(0).toLatin1() == '*'  )
    {
       value.remove(0, 1);
       addresses[addressCount]=value;
       if(cachedAddressMap.contains(value))
       {
           int removedn = cachedAddressMap.remove(value);
           if(removedn>0) logit("Trovato asterisco: aggiorno la Cache per indirizzo: "+value);
           else logit("Trovato asterisco: ma non sono riuscito ad aggiornare la Cache per indirizzo: "+value, 1);
       }
    }
    // già registrato nel cache!!!
    if(cachedAddressMap.contains(value)){
        coords->append(cachedAddressMap[value]);
        if(addressCount%100 == 0) emit valueChanged( addressCount );
        return;
    }
    if(addressCount%100 == 0 && addressCount>0) {
        cacheNwrite();
    }
//    if(this->isReverse) {
//        QStringList sl = value.split(",");
//        value = QString("%1,%2").arg( QString(sl.at(1)).trimmed() ).arg( QString(sl.at(0)).trimmed() );
//    }
    emit valueChanged( addressCount );

    switch(apitype){
        case 0: {
            strurl = QString(APIS.at(apitype)).arg(value).arg(keys.at(googlekeyn));
            break;
        }
        case 2: {
            strurl = QString(APIS.at(apitype)).arg(value).arg(bingkeys.at(bingkeyn));
            break;
            }
       default:
        strurl = QString(APIS.at(apitype)).arg(value);
    }
    // 2 and 3 API bing


    QUrl url(strurl);
    url.setUrl(url.toEncoded());

//    std::string str = urlst.toStdString().c_str();
    curl_easy_setopt(curl, CURLOPT_URL, url.toEncoded().toStdString().c_str());
    buffer.clear();
    res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    /* Check for errors */
    if(res != CURLE_OK)
    {
        logit(QString("Alla riga %1, url '%2' è stato rilevato il seguente errore:\n%3").arg((addressCount+1)).arg(url.toString()).arg(curl_easy_strerror(res)), 2);

        endcode=2;
       // endGeocode();
        return;
    }
    if(httpCode != 200)
    {
        if(this->apitype==2 && httpCode==401){

            QString response2 = buffer; //.c_str();
            logit( QString("Codice: %1<br>Chiave n. %2 di BING aspetto qualche secondo e riprovo: risposta=<br>%3<br>da url<br><br>").arg(httpCode).arg(this->bingkeyn).arg(response2).arg(strurl), 1 ) ;
            usleep(waitsecs*1000000);
            waitsecs++;
            addressCount--;
            return;

        }
        if(this->apitype==2){
            this->bingkeyn++;
            if(bingkeys.count()==this->bingkeyn) {
                logit( QString("Codice: %2 - %3<br>Tutte le chiavi BING (%1) sono terminate! Termino l'esecuzione.").arg(this->bingkeyn).arg(httpCode).arg(curl_easy_strerror(res)), 1 ) ;
                this->bingkeyn--;
                endcode=4;
            }
            else {
                logit( QString("Codice: %3<br>Chiave n. %1 di BING ha raggiunto il limite, passo alla prossima per chiamata %2").arg(this->bingkeyn).arg(strurl).arg(httpCode), 1 ) ;
                // step back one to get right in counter
                addressCount--;
                }


            return;
        }
        QString err = QString("Alla riga %1, url '%2' non è stato possibile eseguire CURL. Codice HTTP = %3 ").arg((addressCount+1)).arg(url.toString()).arg(httpCode);
        logit(err, 2);
        endcode=3;
      //  endGeocode();
        return;
    }

    QString response = buffer; //.c_str();

   // logit(response);
    if(this->apitype==3) {

             QRegExp rx("<ul class=\"geocoderInfoModule\"><li class=\"geocoderInfoAddress\">(.+[^<])</li></ul>");
             QRegExp rx2("<div class=\"geochainModuleLatLong\">([0-9][0-9][,.][0-9]+), ([0-9][0-9][,.][0-9]+)</div>");

             QString addressg;
             int pos=0;
             int pos2 = 0;
             double lon_g;
             double lat_g;
             bool isok;
             while ((pos2 = rx2.indexIn(response, pos2)) != -1) {
                 lat_g = rx2.cap(1).replace(",",".").toDouble(&isok);
                 lon_g = rx2.cap(2).replace(",",".").toDouble(&isok);
                 pos2 += rx2.matchedLength();
             }
             while ((pos = rx.indexIn(response, pos)) != -1) {
                 addressg = rx.cap(1).split("</li>").at(0);
                 pos += rx.matchedLength();
             }
             if(pos==0 || pos2==0)
             {
                 logit( QString("riga %1:%2 - nessun risultato trovato neanche con Super BING!").arg((addressCount+1)).arg(addresses.at(addressCount)) ) ;
                 return;
             }
             QString st = addresses.at(addressCount)+QString("\t%1\t%2\t%3\t%4").arg(lon_g).arg(lat_g).arg("SuperBing").arg(addressg);
             coords->append(st);
             cachedAddressMap.insert(addresses.at(addressCount),coords->last());
             return;
    }
    else {
        QJsonDocument jsonResponse = QJsonDocument::fromJson( response.toUtf8() );
        QJsonObject jsonObject = jsonResponse.object();
        if(jsonObject.count()==0){
            logit( "Risposta API non pervenuta: zero oggetti",2 );
            // httpRequestAborted=true;
            return;
        }
        if(this->apitype<2) getFromGoogleJSON( jsonObject);
        else   getFromBingJSON( jsonObject);
    }

 //  reply= manager->get(QNetworkRequest(url));
 //  connect(reply, &QNetworkReply::finished, this, &curler::handleNetworkData);
}





void curler::getFromGoogleJSON(QJsonObject jsonObject)
{
    if(jsonObject["status"].toString()=="OVER_QUERY_LIMIT")
    {
        this->googlekeyn++;
        if(keys.count()==this->googlekeyn) {
            logit( QString("Tutte le chiavi Google (%1) sono terminate! Termino l'esecuzione.").arg(this->googlekeyn), 1 ) ;
            this->googlekeyn--;
            endcode=4;
        }
        else {
            logit( QString("Chiave n. %1 di Google ha raggiunto il limite, passo alla prossima").arg(this->googlekeyn), 1 ) ;
            // step back one to get right in counter
            addressCount--;
            }
        return;
    }

    if(jsonObject.contains("error_message"))
    {
        QString err = QString("Errore da URL ")+strurl+"<br>____________"
                                                       "<br>"+
                jsonObject["status"].toString()+"\n"+ jsonObject["error_message"].toString();
        logit(err, 2 );

        this->googlekeyn++;
        logit( QString("Provo a passare alla prossima chiave").arg(this->googlekeyn), 1 ) ;

        // step back one to get right in counter
        addressCount--;
        return;
    }

    if(jsonObject["status"].toString()=="ZERO_RESULTS")
    {
        logit( QString("riga %1:%2 - nessun risultato trovato con Google ...provo BING!").arg((addressCount+1)).arg(addresses.at(addressCount)) ) ;
        // provo bing!
        bing=true;
        return;
    }

    if(jsonObject["status"].toString()=="OK")
    {
        QJsonObject qo = jsonObject["results"].toArray()[0].toObject();
        QString addressg = qo["formatted_address"].toString();
        QJsonObject geom = qo["geometry"].toObject();
        QJsonObject loc = geom["location"].toObject();
        QString loctype = geom["location_type"].toString();
        double lon_g = loc["lng"].toDouble();
        double lat_g = loc["lat"].toDouble();
        QString st = addresses.at(addressCount)+QString("\t%1\t%2\t%3\t%4").arg(lon_g).arg(lat_g).arg(loctype).arg(addressg);
       // qDebug() << st;
        if(this->isReverse) {
            QStringList sl = QString(addresses.at(addressCount)).split(",");

            double lon = sl.at(1).toDouble();
            double lat = sl.at(0).toDouble();

            double dist = sqrt( pow( getm2(lat)*(lon - lon_g), 2 ) + pow( getm(lat)*(lat - lat_g), 2 ) );

            st = QString("%1\t%2\t%3\t%4\t%5\t%6\t%7").arg(lon, 0, 'g', 9).arg(lat, 0, 'g', 9).arg(lon_g, 0, 'g', 9).arg(lat_g, 0, 'g', 9).arg(loctype).arg(dist).arg( qo["formatted_address"].toString() );
        }
        coords->append(st);
        cachedAddressMap.insert(addresses.at(addressCount),coords->last());
        return;
    }





 //   if(jsonObject["status"].toString()=="INVALID_REQUEST") or UNKNOWN_ERROR


        logit( QString("Richiesta Google non valida: %1").arg(jsonObject["status"].toString()), 2 );
        endcode=6;
        return;


}


void curler::getFromBingJSON(QJsonObject jsonObject)
{

    if(jsonObject.contains("errorDetails"))
    {
        logit( QString("Errore API BING al URL:%1\n\n%2 ").arg(strurl).arg(jsonObject["errorDetails"].toString()), 2 );
        endcode=5;
        return;
    }

    int nres = jsonObject["resourceSets"].toArray()[0].toObject()["estimatedTotal"].toInt();

    if(nres==0)
    {
        logit( QString("riga %1: nessun risultato trovato con BING<br>%2").arg((addressCount+1)).arg(strurl), 1 ) ;
        coords->append(addresses.at(addressCount)+QString("\t\"9999.99\"\t\"9999.99\"\t\"")+jsonObject["statusDescription"].toString()+"\"\t\"BING\"" );
       // qui allora veramente metto l'errore!
        cachedErrAddressMap.insert(addresses.at(addressCount),coords->last());

        return;
    }

    if(jsonObject["statusDescription"].toString()=="OK")
    {
        QJsonObject qo = jsonObject["resourceSets"].toArray()[0].toObject()["resources"].toArray()[0].toObject();
        QString addressg = qo["name"].toString();
        QJsonObject geom = qo["point"].toObject();
        QJsonArray loc = geom["coordinates"].toArray();
        QString loctype = qo["confidence"].toString()+"|"+
                qo["matchCodes"].toArray()[0].toString()+"|"+
                qo["geocodePoints"].toArray()[0].toObject()["calculationMethod"].toString();
        double lon_g = loc.at(1).toDouble();
        double lat_g = loc.at(0).toDouble();
        QString st = addresses.at(addressCount)+QString("\t%1\t%2\t%3\t%4").arg(lon_g).arg(lat_g).arg(loctype).arg(addressg);
       // qDebug() << st;
        if(this->isReverse) {
            QStringList sl = QString(addresses.at(addressCount)).split(",");
            double lon = sl.at(1).toDouble();
            double lat = sl.at(0).toDouble();
            double dist = sqrt( pow( getm2(lat)*(lon - lon_g), 2 ) + pow( getm(lat)*(lat - lat_g), 2 ) );
            st = QString("%1\t%2\t%3\t%4\t%5\t%6\t%7").arg(lon, 0, 'g', 9).arg(lat, 0, 'g', 9).arg(lon_g, 0, 'g', 9).arg(lat_g, 0, 'g', 9).arg(loctype).arg(dist).arg( qo["formatted_address"].toString() );
        }
        coords->append(st);
        cachedAddressMap.insert(addresses.at(addressCount),coords->last());


        return;
    }

    logit( QString("riga %1: errore con BING<br>%2").arg((addressCount+1)).arg(strurl), 2 ) ;
    endcode=6;
    return;

}



void curler::cacheNwrite()
{

    if (!cacheFile->open(QIODevice::WriteOnly) || !cacheErrFile->open(QIODevice::WriteOnly))
    {
        logit(QString("Non sono riuscito a scrivere sul file cache: %1  <br>Errore: %2").arg(ADDRESS_CACHE).arg(cacheFile->errorString()), 2);
        return;
    }
    QDataStream out(cacheFile);
    out.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    out << cachedAddressMap;
    cacheFile->close();
    QDataStream oute(cacheErrFile);
    oute.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    oute << cachedErrAddressMap;
    cacheErrFile->close();
}
