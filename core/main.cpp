#include <QApplication>
#include <QtSql/QtSql>
#include <QMessageBox>
#include <QProgressDialog>
#include <QResource>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>
#include <QTime>
#include <QSystemSemaphore>
#include <QSharedMemory>
#include <QMessageBox>
#include <QDebug>

#include "Migration.h"
#include "ui/MainWindow.h"
#include "Rig.h"
#include "Rotator.h"
#include "AppGuard.h"

static void loadStylesheet(QApplication* app) {
    QFile style(":/res/stylesheet.css");
    style.open(QFile::ReadOnly | QIODevice::Text);
    app->setStyleSheet(style.readAll());
    style.close();
}

static void setupTranslator(QApplication* app) {
    QTranslator* qtTranslator = new QTranslator(app);
    qtTranslator->load("qt_" + QLocale::system().name(),
    QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    app->installTranslator(qtTranslator);

    QTranslator* translator = new QTranslator(app);
    translator->load(":/i18n/qlog_" + QLocale::system().name().left(2));
    app->installTranslator(translator);
}

static void createDataDirectory() {
    QDir dataDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));

    if (!dataDir.exists()) {
        dataDir.mkpath(dataDir.path());
    }
}

static bool openDatabase() {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    QString path = dir.filePath("qlog.db");
    db.setDatabaseName(path);

    if (!db.open()) {
        qCritical() << db.lastError();
        return false;
    }
    else {
        return true;
    }
}

static bool migrateDatabase() {
    Migration m;
    return m.run();
}

static void startRigThread() {
    QThread* rigThread = new QThread;
    Rig* rig = Rig::instance();
    rig->moveToThread(rigThread);
    QObject::connect(rigThread, SIGNAL(started()), rig, SLOT(start()));
    rigThread->start();
}

static void startRotThread() {
    QThread* rotThread = new QThread;
    Rotator* rot = Rotator::instance();
    rot->moveToThread(rotThread);
    QObject::connect(rotThread, SIGNAL(started()), rot, SLOT(start()));
    rotThread->start();
}

static void debugMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    QString severity_string;

    switch (type)
    {
    case QtDebugMsg:
        severity_string = "[DEBUG   ]";
        break;
    case QtInfoMsg:
        severity_string = "[INFO    ]";
        break;
    case QtWarningMsg:
        severity_string = "[WARNING ]";
        break;
    case QtCriticalMsg:
        severity_string = "[CRITICAL]";
        break;
    case QtFatalMsg:
        severity_string = "[FATAL   ]";
        break;
    default:
        severity_string = "[UNKNOWN ]";
    }

    fprintf(stderr, "%s %s: %s [%s:%u, %s]\n", QTime::currentTime().toString("HH:mm:ss.zzz").toLocal8Bit().constData(),
                                               severity_string.toLocal8Bit().constData(),
                                               localMsg.constData(),
                                               context.file,
                                               context.line,
                                               context.function);
    if ( type == QtFatalMsg )
    {
        abort();
    }
}

int main(int argc, char* argv[]) {

    qInstallMessageHandler(debugMessageOutput);
    QApplication app(argc, argv);

    app.setApplicationVersion(VERSION);
    app.setOrganizationName("hamradio");
    app.setApplicationName("QLog");

    loadStylesheet(&app);
    setupTranslator(&app);

    /* Application Singleton
     *
     * Only one instance of QLog application is allowed
     */
    AppGuard guard( "QLog" );
    if ( !guard.tryToRun() )
    {
        QMessageBox::critical(nullptr, QMessageBox::tr("QLog Error"),
                              QMessageBox::tr("QLog is already running"));
        return 1;
    }

    createDataDirectory();

    if (!openDatabase()) {
        QMessageBox::critical(nullptr, QMessageBox::tr("QLog Error"),
                              QMessageBox::tr("Could not connect to database."));
        return 1;
    }

    if (!migrateDatabase()) {
        QMessageBox::critical(nullptr, QMessageBox::tr("QLog Error"),
                              QMessageBox::tr("Database migration failed."));
        return 1;
    }

    startRigThread();
    startRotThread();

    MainWindow w;
    QIcon icon(":/res/qlog.png");
    w.setWindowIcon(icon);
    w.show();

    return app.exec();
}
