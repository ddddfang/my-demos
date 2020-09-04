
#include <QTextStream>
#include <QVector>
#include <QList>
#include <QSet>
#include <QMap>
#include <algorithm>

#include <QAction>

#include "readerThread.h"
#include "alsaCaptureAudio.h"


int main()
{
    QTextStream out(stdout);

    //-------------------- QList
    {
        QList<QString> authors = {"jensen", "yann", "fabio", "anddy", "linken"};
        for (int i = 0; i < authors.size(); i++) {
            out << authors.at(i) << endl;
        }
        out << endl;
        authors << "lodon" << "balck";  //添加两个元素到 QList
        authors.append("hanson");   //append, prepend 都可用

        std::sort(authors.begin(), authors.end());  //调用c++标准算法

        out << "index of hanson is " << authors.indexOf("hanson") << endl;

        for (QString si : authors) {    //QList也提供基于index的随机访问
            out << si << endl;
        }
        out << endl;

        //从c++标准list
        std::list<double> stdlist;
        stdlist.push_back(1.2);
        stdlist.push_back(0.5);
        stdlist.push_back(3.14);
        QList<double> list = QList<double>::fromStdList(stdlist);
        for (double di : stdlist) {
            out << di << endl;
        }
        out << endl;

    }
    {
        QString s = "jensen,yann,friday,fabio,linke,hanson,diro";
        //QStringList sl = s.split(",");
        QList<QString> sl = s.split(",");   //看起来 QStringList 和 QList<QString> 等效啊
        for (QString si : sl) {
            out << si << endl;
        }
        out << endl;
    }
    //--------------------------QMap
    {
        QMap<QString,int> items = {
            {"hanson", 18},
            {"jensen", 38},
            {"orange", 46},
            {"fabio", 52},
            {"pinkman", 74}
        };

        items.insert("black", 65);  //插入键值对
        items["white"] = 43;        //插入键值对

        out << "jensen's val is " << items["jensen"] << endl;           //查询键为 jensen 的值,若不存在则会插入(val为int默认值0),推荐使用value()
        out << "fabio's val is " << items.value("fabio", 100) << endl;      //查询键为 fabio 的值,若没有则返回默认值100
        if (items.contains("orange")) {                                     //查找键值对集合中是否含有键为 orange 的元素
            out << "we have items[orange] = " << items["orange"] << endl;
        } else {
            out << "we don't have this element" << endl;
        }

        QList<QString> keys = items.keys();
        QList<int> vals = items.values();
        for (auto ki : keys) {
            out << QString("items[%1]=%2.").arg(ki).arg(items.value(ki,-1)) << endl;
        }
        out << endl;

        items.remove("pinkman");

        for (auto ki : items.keys()) {
            out << QString("items[%1]=%2.").arg(ki).arg(items.value(ki,-1)) << endl;
        }
        items.clear();
    }

    printf("test start!\n");

    //AlsaCaptureAudio alsaCaptureAudio;
    //alsaCaptureAudio.captureAudio();



    readerThread *pReader = new readerThread();
    pReader->start();
    //pReader->pause(true);
    QThread::sleep(10);

    if (pReader) {
        if (pReader->isRunning()) {
            pReader->stop();
            pReader->wait();    //thread join
        }
        delete pReader;
    }
    return 0;
}
