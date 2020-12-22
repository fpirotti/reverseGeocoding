#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{


    ui->setupUi(this);
    setAcceptDrops(true);
    httpRequestAborted = false; overqlimit=false;
    addressCount=0;

    isReverse=false;

    if(!checkDate())  {
        exit(0);
    }

   errorscurl << QString("Operazione terminata correttamente")
          << QString("Elaborazione terminata dall'utente")
          << QString("Elaborazione terminata per errore da CURL")
          << QString("Elaborazione terminata verifica riga rossa in LOG")
          << QString("Elaborazione terminata per errore da API")
          << QString("Elaborazione terminata per errore nella risposta")
          << QString("Elaborazione terminata per errore generico") ;
    dialog = new Dialog;

    //dialog->setWindowFlags(Qt::Popup);

    this->ui->terminaButton->setEnabled(false);
    /// inizio il cache delle coordinate
    cacheFile.setFileName(ADDRESS_CACHE);
    cacheErrFile.setFileName(ADDRESSERR_CACHE);
    if (cacheFile.exists() && !cacheFile.open(QIODevice::ReadOnly))
    {
        logit(QString("<font color=red>Non sono riuscito a leggere il file cache: %1 <br>Eliminarlo manualmente. <br>Errore: %2</font>").arg(ADDRESS_CACHE).arg(cacheFile.errorString()));
        return;
    }
    if (cacheErrFile.exists() && !cacheErrFile.open(QIODevice::ReadOnly))
    {
        logit(QString("<font color=red>Non sono riuscito a leggere il file cache errori: %1 <br>Eliminarlo manualmente. <br>Errore: %2</font>").arg(ADDRESSERR_CACHE).arg(cacheErrFile.errorString()));
        return;
    }

    cacheFile.close();
    cacheErrFile.close();

    if (cacheFile.exists() && cacheFile.open(QIODevice::ReadOnly) )
    {
        QDataStream in(&cacheFile);
        in.setVersion(QDataStream::Qt_DefaultCompiledVersion);
        in >> cachedAddressMap;
        int i=0;
        while( in.atEnd() == false ) {
          in >> cachedAddressMap;
          i++;
        }
        cacheFile.close();
        logit(QString("<font color=green>File cache: %1 <br>Esiste, lo utilizzo.</font>").arg(ADDRESS_CACHE));

    } else {
        logit(QString("<font color=green>File cache: %1 <br>Non esiste, verrà creata.</font>").arg(ADDRESS_CACHE));
    }


    if (cacheErrFile.exists() && cacheErrFile.open(QIODevice::ReadOnly) )
    {
        QDataStream in(&cacheErrFile);
        in.setVersion(QDataStream::Qt_DefaultCompiledVersion);
        in >> cachedErrAddressMap;
        int i=0;
        while( in.atEnd() == false ) {
          in >> cachedErrAddressMap;
          i++;
        }
        cacheErrFile.close();
        logit(QString("<font color=green>File  cache2: %1 <br>Esiste, lo utilizzo.</font>").arg(ADDRESSERR_CACHE));

    } else {
        logit(QString("<font color=green>File cache: %1 <br>Non esiste, verrà creata.</font>").arg(ADDRESSERR_CACHE));
    }



    ///////////////

    QObject::connect(this->ui->terminaButton, SIGNAL(released()), this, SLOT(cancelCurl()));

    QObject::connect(this, SIGNAL(valueChanged(int)),
                     this, SLOT(setProgress(int)));

    QObject::connect(ui->pushButton, SIGNAL(released()),
                     this, SLOT(resetCache()));

    QObject::connect(this->ui->menu_2, SIGNAL(aboutToShow()),
                     this, SLOT(about()) );

  //  manager = new QNetworkAccessManager(this);


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::cancelCurl() {
    terminated=true; // 1 = terminato da utente
}


bool MainWindow::checkDate()
{
    if(QDateTime::currentDateTime().date().year() > 2021)
    {
        //QMessageBox::warning(this,QString("Warning"),QString("Le chiavi API di Google devono essere aggiornate, contattare lo sviluppatore fpirotti@gmail.com ."),
         //                 QMessageBox::Ok);
        return false;
    }
    return true;
}

void MainWindow::resetCache()
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Attenzione", "Sicuro di voler cancellare il file cache?",
                                  QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        cachedAddressMap.clear();
        cachedErrAddressMap.clear();
        if(cacheFile.isOpen()){
            cacheFile.flush();
            cacheFile.close();
        }
        if(cacheErrFile.isOpen()){
            cacheErrFile.flush();
            cacheErrFile.close();
        }
        if (!cacheFile.open(QIODevice::WriteOnly))
        {
            logit(QString("<font color=red>Non sono riuscito a scrivere sul file cache: %1 <br>Errore: %2</font>").arg(ADDRESS_CACHE).arg(cacheFile.errorString()));
            return;
        }
        cacheFile.close();

        if (!cacheErrFile.open(QIODevice::WriteOnly))
        {
            logit(QString("<font color=red>Non sono riuscito a scrivere sul file cache: %1 <br>Errore: %2</font>").arg(ADDRESSERR_CACHE).arg(cacheErrFile.errorString()));
            return;
        }
        cacheErrFile.close();

//        out.setDevice(&cacheFile);
//        out.setVersion(QDataStream::Qt_DefaultCompiledVersion);
    } else {
      qDebug() << "Yes was *not* clicked";
    }
}

void MainWindow::about()
{
    dialog->show();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls()) {
        e->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *e)
{

    httpRequestAborted = false; overqlimit=false;     terminated=false;
    if(ui->bing->isChecked()){  logit("Uso BING"); }
    if(ui->google->isChecked()){  logit("Uso Google"); }

    foreach (const QUrl &url, e->mimeData()->urls()) {
        QString fileName = url.toLocalFile();
        fni=QFileInfo(fileName);
        if( fni.suffix().compare("xlsx", Qt::CaseInsensitive)==0  ||
                fni.suffix().compare("xls", Qt::CaseInsensitive)==0 )
        {

            return;
        }
        if(  fni.suffix().compare("txt", Qt::CaseInsensitive)==0 ||
            fni.suffix().compare("csv", Qt::CaseInsensitive)==0  ||
            fni.suffix().compare("kml", Qt::CaseInsensitive)==0  )
        {
            ui->lineEdit_2->setText(fileName);
            if(fni.suffix().compare("kml", Qt::CaseInsensitive)==0)
            {
                isReverse=true;
            }
            readExcel(fni);
        }
        else{
            QString a("<font color=red>Errore: Estensione file \""+fni.baseName()+".(<b>"+fni.completeSuffix()+"</b>)\" non compatibile, trascinare files con estensione xls o xlsx.</font>");
            logit(a);
        }
    }

    this->coords.clear();
}

void MainWindow::logit(QString mes, int type)
{
    if(type==2)  mes.prepend("<font color=red>").append("</font>");
    if(type==1)  mes.prepend("<font color=orange>").append("</font>");
    logs.append(mes);
    ui->textBrowser->append(mes);
}

void MainWindow::setAddresses(QStringList ql){
    this->addresses= ql;
}

void MainWindow::readExcel(QFileInfo file)
{

    logit(QString("Leggo file %1").arg(file.baseName()));
    httpRequestAborted = false;
    valueChanged(0);
    ui->progressBar->setValue(0);


    QThread* thread = new QThread;
    Worker* workerThread = new Worker();
    workerThread->col = 1;
    workerThread->isRev = this->isReverse;
    workerThread->skip = ui->skipfirstline->isChecked();
    workerThread->file = file;
    this->ui->terminaButton->setEnabled(true);

    connect(workerThread, SIGNAL(finished()), workerThread, SLOT(deleteLater()));

    workerThread->moveToThread(thread);

    connect(workerThread, SIGNAL(errMessage(QString, int)), this, SLOT(logit(QString, int)));
    connect(workerThread, SIGNAL(resultReady(QStringList)), this, SLOT(setAddresses(QStringList)));
    connect(workerThread, SIGNAL(valueChanged(int)),  this, SLOT(setProgress(int)));
//    connect(workerThread, SIGNAL(finished()), this, SLOT(processFinished()));
    connect(workerThread, SIGNAL(finished()), thread, SLOT(quit()));
    connect(thread, SIGNAL(finished()), this, SLOT(processFinished()));
    connect(thread, SIGNAL(started()), workerThread, SLOT(process()));
    thread->start();
}

void MainWindow::processFinished()
{
    logit(QString("Finito di leggere %1 righe").arg(this->addresses.count()));
    ui->progressBar->setRange(0, this->addresses.count());
    ui->progressBar->setFormat( QString( "%v di %1" ).arg(this->addresses.count()) );
    emit valueChanged(0);

    if(errors.count()>0){
        QMessageBox msgBox;
        msgBox.setText("Errore: <br>"+errors.join("<br>"));
        msgBox.exec();
        return;
    }
    if(addresses.length() <1){
        QMessageBox msgBox;
        msgBox.setText("Errore: Nessun indirizzo trovato nel file");
        msgBox.exec();
        return;
    }

    QThread* thread = new QThread;
    curler* curlThread = new curler();



    curlThread->timeout = ui->timout->value();
    curlThread->addresses = addresses;

    if(ui->google->isChecked()) curlThread->apitype=0;
    if(this->isReverse) curlThread->apitype=1;
    if(ui->bing->isChecked()) curlThread->apitype=2;
    if(ui->superbing->isChecked()) curlThread->apitype=3;

    curlThread->cacheErrFile = &cacheErrFile;
    curlThread->cacheFile = &cacheFile;
    curlThread->terminated = &terminated;
    curlThread->coords = &coords;
    curlThread->cachedAddressMap = this->cachedAddressMap;
    curlThread->cachedErrAddressMap = this->cachedErrAddressMap;
    this->ui->terminaButton->setEnabled(true);

    curlThread->moveToThread(thread);

    connect(curlThread, SIGNAL(finished()), thread, SLOT(deleteLater()));


    connect(curlThread, SIGNAL(logit(QString, int)), this, SLOT(logit(QString, int)));
    //connect(curlThread, SIGNAL(resultReady(QStringList)), this, SLOT());
    connect(curlThread, SIGNAL(valueChanged(int)),  this, SLOT(setProgress(int)));
    connect(curlThread, SIGNAL(ended(int)), this, SLOT(endGeocode(int)));

//    connect(workerThread, SIGNAL(finished()), this, SLOT(processFinished()));
    connect(curlThread, SIGNAL(finished()), thread, SLOT(quit()));
    connect(thread, SIGNAL(started()), curlThread, SLOT(process()));
    thread->start();

}

void MainWindow::setProgress(int val)
{
    ui->progressBar->setValue( val );
}



void MainWindow::endGeocode(int code)
{

    writeCSV();

    QMessageBox msgBox;
    msgBox.setText(errorscurl.at(code));
    msgBox.exec();
    return;
}


void MainWindow::writeCSV(){
    QFile outf(  fni.absolutePath().append("/").append(fni.baseName()).append("_indirizzi.csv"));
    if ( outf.open(QIODevice::WriteOnly) )
    {
        QTextStream stream( &outf );
        qDebug() << outf.readAll() ;
        if(this->isReverse) stream << QString("lon\tlat\tlon_google\tlat_google\tprecisione\tdistanza\tindirizzo\n");
        stream <<  this->coords.join("\n");
        outf.close();
    }

    QFile outf2(  fni.absolutePath().append("/").append(fni.baseName()).append("_LOG__.html"));
    if ( outf2.open(QIODevice::WriteOnly) )
    {
        QTextStream stream( &outf2 );
        stream <<  this->logs.join("</br>");
        outf2.close();
    }

}





