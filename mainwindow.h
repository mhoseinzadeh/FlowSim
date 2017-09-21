#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <flow.h>
#include <QGraphicsScene>
#include <QVector>
#include <QLabel>
#include <graphicsscene.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void resizeScene();
    qreal nextStep(qreal t, uint64_t max=0);

protected:
    bool eventFilter(QObject *watched, QEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
    void expand(QTreeWidgetItem *item, bool e=true);
    void setDataPolicy(int index);
    void setSplitPolicy(int index);

private slots:
    bool event(QEvent *event);
    void on_edtRuns_textChanged(const QString &arg1);
    void on_edtCycleWidth_textChanged(const QString &arg1);
    void on_actionCollapseAll_triggered();
    void on_actionExpandAll_triggered();
    void on_actionOpen_triggered();
    void on_actionAbout_triggered();
    void on_actionReopen_triggered();
    void on_actionExit_triggered();
    void on_actionPlayAll_triggered();
    void on_actionPlayExact_triggered();
    void on_actionPlayUpTo_triggered();
    void on_actionManualPlay_triggered();
    void on_actionStep_Backward_triggered();
    void on_actionStep_Forward_triggered();
    void on_action_None_triggered();
    void on_actionStall_Injection_Write_Only_triggered();
    void on_actionStall_Injection_Read_and_Write_triggered();
    void on_action_Provisional_Execution_triggered();
    void on_action_Splittable_triggered();
    void on_actionOnly_Processes_triggered();
    void on_actionReads_and_Processes_triggered();
    void on_action_Unsplittable_triggered();

private:
    Ui::MainWindow *ui;
    Flow *mainFlow;
    GraphicsScene *scene;
    QVector<Flow*> run;
    QString code;
    qreal globalTop;
    uint64_t max_entring;
    int R, B;
};

#endif // MAINWINDOW_H
