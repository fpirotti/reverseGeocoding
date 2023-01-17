#include "worker.h"
#include "math.h"
// --- CONSTRUCTOR ---
Worker::Worker() {
    httpRequestAborted=false;
    isRev=false;
    // you could copy data from constructor arguments to internal variables here.
}

// --- DECONSTRUCTOR ---
Worker::~Worker() {
    // free resources
}

// --- PROCESS ---
// Start processing data.
void Worker::process() {
    // allocate resources using new here
    go();
    emit finished();
}
void Worker::stopProgress() {
    this->httpRequestAborted=true;
    emit resultReady(addresses);
    emit finished();
}

void Worker::go4txt()
{
    QFile ff(file.absoluteFilePath());
    QString st;
    quint64 size=ff.size();
    int every = ceil(size / 100);
    int cc=0, cct=0; int gsize=0;

    if (ff.open(QIODevice::ReadOnly))
    {
       QTextStream in(&ff);
       emit this->errMessage(QString("Inizio lettura file."));

       if(skip) {
           skip=false;
           if(file.suffix().compare("kml", Qt::CaseInsensitive)!=0 )
               st=in.readLine();
       }

       while (!in.atEnd())
       {
          st=in.readLine();

          gsize += st.size();
          cc = gsize/every;
          addresses.append(st);
          if(cc!=cct)
          {
              if(cc>100) cc=100;
              emit valueChanged(cc);
              cct=cc;
          }
       }
       ff.close();
    }
}


void Worker::go4txtRev()
{
    QDomDocument doc("kml");
    QFile file(this->file.filePath());
    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "File non apribilei";
        emit this->errMessage(QString("File %1 non apribile.").arg( QFileInfo(file).fileName()));
        return;
    }

    if (!doc.setContent(&file)) {
        file.close();
        qDebug() << "File non aprii";
        return;
    }

    file.close();

    QDomNode n = doc.namedItem("kml").childNodes().at(0);
    TraverseXmlNode(n);

}

void Worker::TraverseXmlNode(const QDomNode& node)
{

      QDomNode domNode = node.firstChild();
      QDomElement domElement;
      QDomText domText;

      static int level = 0;

      level++;

      while(!(domNode.isNull()))
      {
        if( domNode.isText() && domNode.parentNode().toElement().tagName().compare("coordinates", Qt::CaseInsensitive)==0 )
        {
          domText = domNode.toText();
          if(!domText.isNull())
          {

              QStringList sl = domText.data().trimmed().split(",");
              QString value = QString("%1,%2").arg( QString(sl.at(1)).trimmed() ).arg( QString(sl.at(0)).trimmed() );
              addresses.append(value);
          }
        }

        TraverseXmlNode(domNode);
        domNode = domNode.nextSibling();
      }
      level--;
}

void Worker::go()
{
    //int count=0;
    addresses.clear();
    if(!file.isFile()) {
       emit this->errMessage(QString("File non esiste."));
        return;
    }
    if(file.suffix().compare("txt", Qt::CaseInsensitive)==0 ||
            file.suffix().compare("csv", Qt::CaseInsensitive)==0 )
    {
        go4txt();
        emit resultReady(addresses);
        return;
    }
    if(file.suffix().compare("kml", Qt::CaseInsensitive)==0 )
    {
        go4txtRev();
        emit resultReady(addresses);
        return;
    }
    else {
        emit this->errMessage(QString("Per questa versione solo file con estensione KML, CSV e TXT sono accettati. I file devono contenere una tabella in forma di testo leggibile"));
        return;
    }


//    excel = new QAxObject( "Excel.Application", 0 );
//    workbooks = excel->querySubObject( "Workbooks" );
//    workbook = workbooks->querySubObject( "Open(const QString&)", file.absoluteFilePath() );
//    sheets = workbook->querySubObject( "Worksheets" );

//    count = sheets->dynamicCall("Count()").toInt();


//    sheet = sheets->querySubObject( "Item( int )", 1 );
//    range = sheet->querySubObject("UsedRange");
//    QAxObject* rows = range->querySubObject( "Rows" );
//    int rowCount = rows->dynamicCall( "Count()" ).toInt(); //unfortunately, always returns 255, so you have to check somehow validity of cell values

//    qDebug() << rowCount << " rowCount.." << endl;
//    QAxObject* columns = range->querySubObject( "Columns" );
//    int columnCount = columns->dynamicCall( "Count()" ).toInt(); //similarly, always returns 65535
//    qDebug() << columnCount << " columnCount.." << endl;

//    int every = ceil( (double) rowCount / 100.0);
//    int start=1;
//    if(skip) start=2;
// //    bool isEmpty = true;	//when all the cells of row are empty, it means that file is at end (of course, it maybe not right for different excel files. it's just criteria to calculate somehow row count for my file)
//    for (int row=start; row <= rowCount; row++)
//    {
//        QAxObject* cell = range->querySubObject( "Cells( int, int )", row, col );
//        QVariant value = cell->dynamicCall( "Value()" );
//        if ( httpRequestAborted)
//        {
//            emit errMessage(QString("Terminato da utente a riga %1").arg(row));
//            break;
//        }
//        if (value.toString().isEmpty())
//        {
//            emit errMessage(QString("Errore: Vuoto a riga %1").arg(row));
//            break;
//        }
//        if(row==start)
//        {
//            emit errMessage(QString("Esempio prima riga letta:<br>%2").arg(value.toString()));
//        }
//        addresses.append(value.toString());
//       if(row%every==0)  emit valueChanged(ceil(row/every));

//    }
//    workbook->dynamicCall("Close()");
//    excel->dynamicCall("Quit()");
//    emit resultReady(addresses);
}




