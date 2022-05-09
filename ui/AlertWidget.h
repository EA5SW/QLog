#ifndef ALERTWIDGET_H
#define ALERTWIDGET_H

#include <QWidget>
#include "data/SpotAlert.h"
#include "models/AlertTableModel.h"

namespace Ui {
class AlertWidget;
}

class AlertWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AlertWidget(QWidget *parent = nullptr);
    ~AlertWidget();

   int alertCount() const;

public slots:
    void addAlert(const SpotAlert &alert);
    void clearAllAlerts();
    void entryDoubleClicked(QModelIndex index);

signals:
    void alertsCleared();
    void tuneDx(QString, double);

private:
    Ui::AlertWidget *ui;
    AlertTableModel* alertTableModel;
};

#endif // ALERTWIDGET_H