#ifndef QSODETAILDIALOG_H
#define QSODETAILDIALOG_H

#include <QDialog>
#include <QDataWidgetMapper>
#include <QItemDelegate>
#include <QLabel>
#include <QWebEnginePage>
#include <QPointer>

#include "models/LogbookModel.h"
#include "core/Gridsquare.h"

namespace Ui {
class QSODetailDialog;
}

class QSOEditMapperDelegate : public QItemDelegate
{
        Q_OBJECT
public:
    QSOEditMapperDelegate(QObject *parent = 0) : QItemDelegate(parent) {};
    void setEditorData(QWidget *editor,
                           const QModelIndex &index) const override;
    void setModelData(QWidget *editor,
                          QAbstractItemModel *model,
                          const QModelIndex &index) const override;
signals:
    void keyEscapePressed(QObject *);

private:
    bool eventFilter(QObject *object, QEvent *event) override;
};

class QSODetailDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QSODetailDialog(const QSqlRecord &qso,
                             QWidget *parent = nullptr);
    void accept() override;
    void keyPressEvent(QKeyEvent *evt) override;
    ~QSODetailDialog();

private slots:
    void editButtonPressed();
    void resetButtonPressed();
    void resetKeyPressed(QObject *);
    void setReadOnlyMode(bool);
    void modeChanged(QString);
    void showPaperButton();
    void showEQSLButton();
    void dateTimeOnChanged(const QDateTime &);
    void dateTimeOffChanged(const QDateTime &);
    void freqTXChanged(double);
    void freqRXChanged(double);
    void timeLockToggled(bool);
    void freqLockToggled(bool);
    void callsignChanged(QString);
    void propagationModeChanged(const QString &);
    bool doValidation();
    void doValidationDateTime(const QDateTime&);
    void mapLoaded(bool);
    void myGridChanged(QString);
    void DXGridChanged(QString);

private:
    /* It is modified logbook model when only basic
     * validation are done. The extended validations
     * are done in Form itself */
    class LogbookModelPrivate : public LogbookModel
    {

    public:
        explicit LogbookModelPrivate(QObject* parent = nullptr, QSqlDatabase db = QSqlDatabase());

        QVariant data(const QModelIndex &index, int role) const override;
        bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    };

    bool highlightInvalid(QLabel *, bool, const QString&);
    void blockMappedWidgetSignals(bool);
    void drawDXOnMap(const QString &label, const Gridsquare &dxGrid);
    void drawMyQTHOnMap(const QString &label, const Gridsquare &myGrid);
    void enableWidgetChangeHandlers();

    Ui::QSODetailDialog *ui;
    QPointer<QDataWidgetMapper> mapper;
    QPointer<LogbookModelPrivate> model;
    QSqlRecord *editedRecord;
    QPointer<QPushButton> editButton;
    QPointer<QPushButton> resetButton;
    qint64 timeLockDiff;
    double freqLockDiff;
    bool isMainPageLoaded;
    QPointer<QWebEnginePage> main_page;
    QString postponedScripts;

    static const QString SAVE_BUTTON_TEXT;
    static const QString EDIT_BUTTON_TEXT;
};

#endif // QSODETAILDIALOG_H
