#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QSettings>
#include <QTime>
#include <QScrollBar>
#include <QContextMenuEvent>
#include <QGraphicsSceneHoverEvent>
#include <QMessageBox>
#include <QWidgetAction>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mainFlow(NULL)
{
    ui->setupUi(this);
    R = B = 0;
    max_entring = 0;
    scene = new GraphicsScene(0, 0, 400, 400);
    scene->installEventFilter(this);
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setMouseTracking(true);

    qsrand(QTime::currentTime().msec());

    QSettings settings("Morteza", "FlowSim");
    ui->edtRuns->setText(settings.value("run", "1").toString());
    ui->edtCycleWidth->setText(settings.value("cycle width", "15").toString());
    CycleWidth = qMax<int>(5, ui->edtCycleWidth->text().toInt());

    QActionGroup *actGrp1 = new QActionGroup(this);
    actGrp1->addAction(ui->action_Splittable);
    actGrp1->addAction(ui->actionOnly_Processes);
    actGrp1->addAction(ui->actionReads_and_Processes);
    actGrp1->addAction(ui->action_Unsplittable);
    ui->action_Splittable->setChecked(true);

    QActionGroup *actGrp2 = new QActionGroup(this);
    actGrp2->addAction(ui->action_None);
    actGrp2->addAction(ui->actionStall_Injection_Write_Only);
    actGrp2->addAction(ui->actionStall_Injection_Read_and_Write);
    actGrp2->addAction(ui->action_Provisional_Execution);
    ui->action_None->setChecked(true);


    QWidgetAction *act1 = new QWidgetAction(this);
    act1->setDefaultWidget(ui->actionIteration);
    QWidgetAction *act2 = new QWidgetAction(this);
    act2->setDefaultWidget(ui->actionIteration_2);
    ui->menu_Settings->addActions(QList<QAction*>() << act1 << act2);
    ui->menu_Settings->addSeparator()->setText(tr("Split Policy"));
    ui->menu_Settings->addActions(actGrp1->actions());
    ui->menu_Settings->addSeparator()->setText(tr("Data Interfrence Policy"));
    ui->menu_Settings->addActions(actGrp2->actions());

    switch (splitPolicy = (BlockSplitPloicy)settings.value("split", 0).toInt()) {
    case Splittable: ui->action_Splittable->setChecked(true); break;
    case Unsplittable: ui->action_Unsplittable->setChecked(true); break;
    case OnlyProcess: ui->actionOnly_Processes->setChecked(true); break;
    case ReadAndProcess: ui->actionReads_and_Processes->setChecked(true); break;
    }

    switch(dataPolicy = (DataInterfrencePolicy)settings.value("policy", 0).toInt()) {
    case DontCare: ui->action_None->setChecked(true); break;
    case StallInjectionWriteOnly: ui->actionStall_Injection_Write_Only->setChecked(true); break;
    case StallInjectionReadWrite: ui->actionStall_Injection_Read_and_Write->setChecked(true); break;
    case ProvisionalExecution: ui->action_Provisional_Execution->setChecked(true); break;
    }

    resizeScene();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::resizeScene()
{
    bool vsnap = ui->graphicsView->verticalScrollBar()->value() == ui->graphicsView->verticalScrollBar()->maximum();
    bool hsnap = ui->graphicsView->horizontalScrollBar()->value() == ui->graphicsView->horizontalScrollBar()->maximum();
    QRect b = ui->graphicsView->contentsRect();
    QRect r = QRect(0, 0, qMax<int>(b.width(), R*CycleWidth), qMax<int>(b.height(), B));
    scene->setSceneRect(r);
    scene->update();
    if(vsnap) ui->graphicsView->verticalScrollBar()->setValue(ui->graphicsView->verticalScrollBar()->maximum());
    if(hsnap) ui->graphicsView->horizontalScrollBar()->setValue(ui->graphicsView->horizontalScrollBar()->maximum());

    int tr=n_total_flows+n_ignored_flows, rr=n_ignored_flows, rh=0, wh=0, bt=0;
    uint64_t lat=0;

    for(QMap<QString, QMap<uint64_t, timeslot_t>>::iterator it = readHistory.begin(); it != readHistory.end(); it++) {
        rh += it->size();
    }

    for(QMap<QString, QMap<uint64_t, timeslot_t>>::iterator it = writeHistory.begin(); it != writeHistory.end(); it++) {
        wh += it->size();
    }

    for(QMap<QString, QMap<uint64_t, char>>::iterator it = BTST.begin(); it != BTST.end(); it++) {
        bt += it->size();
    }

    for(QVector<Flow*>::reverse_iterator it=run.rbegin(); it!=run.rend(); ++it) {
        if(!((*it)->ignored())) {
            lat = qMax<uint64_t>(lat, (*it)->exitingTime());
        }
    }

    ui->lblBTST->setText(QString::number(bt));
    ui->lblFlows->setText(QString::number(tr));
    ui->lblLat->setText(QString::number(lat));
    ui->lblReadHist->setText(QString::number(rh));
    ui->lblWriteHist->setText(QString::number(wh));
    ui->lblRerun->setText(QString().sprintf("%d (%.2f%%)", rr, 100*(double)(rr)/(double)(tr)));

    for(QMap<QString, counter_t>::iterator it = Counter.begin(); it != Counter.end(); it++) {
        if(it.value().item) {
            it.value().item->setText(1, QString::number(it.value().count));
            it.value().item->setText(2, QString().sprintf("%.3g", (double)(it.value().count-1)/(double)it.value().max_entring));
        }
    }
}

qreal MainWindow::nextStep(qreal t, uint64_t max)
{
    if(JobQueue.empty()) return t;
    Job j = JobQueue.takeFirst();

    n_total_flows++;
    Flow *flow = j.flow;
    flow->updateLatency();
    flow->setY(t);
    int latency = flow->run();
    max_entring = flow->entringTime();
    if(max != 0) {
        if(flow->entringTime() > max) {
            delete flow;
            return t;
        }
    }
    run.append(flow);
    t += flow->boundingRect().height();
    R = qMax<int>(R, latency);
    B = flow->bottom();
    scene->addItem(flow);

    executeIgnoreList([this](Flow *f){
        int i = run.indexOf(f);
        if(i > -1) run.remove(i);
    });
    return t;
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if(watched == scene) {
        if(event->type() == QEvent::GraphicsSceneMouseMove) {
            event->accept();
            QPoint p = ((QGraphicsSceneMouseEvent*)event)->scenePos().toPoint();
            ui->statusBar->showMessage(QString().sprintf("Time: %d \tFlow: %d", p.x()/CycleWidth, p.y()/20));
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QPoint p = ui->treeWidget->mapFromGlobal(event->globalPos());
    if(ui->treeWidget->contentsRect().contains(p)) {
        QMenu menu(this);
        menu.addAction(ui->actionExpandAll);
        menu.addAction(ui->actionCollapseAll);
        menu.exec(event->globalPos());
    }
}

void MainWindow::expand(QTreeWidgetItem *item, bool e)
{
    item->setExpanded(e);
    for(int i=0; i<item->childCount(); i++) {
        expand(item->child(i), e);
    }
}

bool MainWindow::event(QEvent *event)
{
    switch(event->type()) {
    case QEvent::Show:
    case QEvent::Resize:
        event->accept();
        resizeScene();
        break;
    default: break;
    }
    return QMainWindow::event(event);
}

void MainWindow::on_edtRuns_textChanged(const QString &arg1)
{
    QSettings settings("Morteza", "FlowSim");
    settings.setValue("run", arg1);
}

void MainWindow::on_edtCycleWidth_textChanged(const QString &arg1)
{
    QSettings settings("Morteza", "FlowSim");
    settings.setValue("cycle width", arg1);
    CycleWidth = qMax<int>(5, arg1.toInt());
    resizeScene();
}


void MainWindow::setDataPolicy(int index)
{
    QSettings settings("Morteza", "FlowSim");
    settings.setValue("policy", index);
    dataPolicy = DataInterfrencePolicy(index);
}

void MainWindow::setSplitPolicy(int index)
{
    QSettings settings("Morteza", "FlowSim");
    settings.setValue("split", index);
    splitPolicy = BlockSplitPloicy(index);
}

void MainWindow::on_actionCollapseAll_triggered()
{
    for(int i=0; i<ui->treeWidget->topLevelItemCount(); i++) {
        expand(ui->treeWidget->topLevelItem(i), false);
    }
}

void MainWindow::on_actionExpandAll_triggered()
{
    for(int i=0; i<ui->treeWidget->topLevelItemCount(); i++) {
        expand(ui->treeWidget->topLevelItem(i), true);
    }
    ui->treeWidget->resizeColumnToContents(0);
    ui->treeWidget->resizeColumnToContents(1);
    ui->treeWidget->resizeColumnToContents(2);
}

void MainWindow::on_actionOpen_triggered()
{
    QSettings settings("Morteza", "FlowSim");
    QString filename = settings.value("filename").toString();
    filename = QFileDialog::getOpenFileName(this, tr("Open Flow File"), filename, tr("Flow Files (*.flow)"));
    if(!filename.isEmpty()) {
        settings.setValue("filename", filename);
        QFile f(filename);
        if (!f.open(QFile::ReadOnly | QFile::Text)) {
            qDebug() << "Could not read file";
            return;
        }
        QTextStream in(&f);
        code = in.readAll();
        code.replace(";", " ;");
        code.replace("{", " {");
        code.replace("}", " }");
        code.replace(QRegExp("//[^\\n]*\\n"), "\n");
        Counter.clear();
        on_actionPlayAll_triggered();
    }
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::information(this, "About ...",
                             "This program is for demonstration and validation\n"
                             "of Flow-Based simulation methodology.\n\n"
                             "Written by: Morteza Hoseinzadeh\n"
                             "E-mail: mhoseinzadeh@cs.ucsd.edu");
}

void MainWindow::on_actionReopen_triggered()
{
    QSettings settings("Morteza", "FlowSim");
    QString filename = settings.value("filename").toString();
    if(!filename.isEmpty()) {
        settings.setValue("filename", filename);
        QFile f(filename);
        if (!f.open(QFile::ReadOnly | QFile::Text)) {
            qDebug() << "Could not read file";
            return;
        }
        QTextStream in(&f);
        code = in.readAll();
        code.replace(";", " ;");
        code.replace("{", " {");
        code.replace("}", " }");
        code.replace(QRegExp("//[^\\n]*\\n"), "\n");
        Counter.clear();
        on_actionPlayAll_triggered();
    }
}

void MainWindow::on_actionExit_triggered()
{
    close();
}


void MainWindow::on_actionPlayAll_triggered()
{
    int vvalue = ui->graphicsView->verticalScrollBar()->value();
    int hvalue = ui->graphicsView->horizontalScrollBar()->value();
    on_actionManualPlay_triggered();
    for(int i=0; i<ui->edtRuns->text().toInt(); i++) {
        globalTop = nextStep(globalTop);
    }
    resizeScene();
    ui->graphicsView->verticalScrollBar()->setValue(vvalue);
    ui->graphicsView->horizontalScrollBar()->setValue(hvalue);
}

void MainWindow::on_actionPlayExact_triggered()
{
    int max = ui->edtRuns->text().toInt();
    bool ok;
    max = QInputDialog::getInt(this, "Play Up To ...", "Insert the number of flows", max, 0, std::numeric_limits<int>::max(), 1, &ok);
    if(ok) {
        int vvalue = ui->graphicsView->verticalScrollBar()->value();
        int hvalue = ui->graphicsView->horizontalScrollBar()->value();
        ui->edtRuns->setText(QString::number(max));
        on_edtRuns_textChanged(ui->edtRuns->text());
        on_actionManualPlay_triggered();
        for(int i=1; i<ui->edtRuns->text().toInt(); i++) {
            JobQueue.push(Job(0, new Flow(mainFlow, 0)));
        }
        stopRepeating = true;
        while(!JobQueue.isEmpty()) {
            globalTop = nextStep(globalTop);
        }
        stopRepeating = false;
        resizeScene();
        ui->graphicsView->verticalScrollBar()->setValue(vvalue);
        ui->graphicsView->horizontalScrollBar()->setValue(hvalue);
    }
}

void MainWindow::on_actionPlayUpTo_triggered()
{
    int max = ui->edtRuns->text().toInt();
    bool ok;
    max = QInputDialog::getInt(this, "Play Up To ...", "Insert the maximum time", max, 0, std::numeric_limits<int>::max(), 1, &ok);
    if(ok) {
        int vvalue = ui->graphicsView->verticalScrollBar()->value();
        int hvalue = ui->graphicsView->horizontalScrollBar()->value();
        ui->edtRuns->setText(QString::number(max));
        on_edtRuns_textChanged(ui->edtRuns->text());
        on_actionManualPlay_triggered();
        bool changed = true;
        qreal t=0;
        while(changed) {
            t = nextStep(globalTop, max);
            if(globalTop == t) changed = false;
            globalTop = t;
        }
        resizeScene();
        ui->graphicsView->verticalScrollBar()->setValue(vvalue);
        ui->graphicsView->horizontalScrollBar()->setValue(hvalue);
    }
}

void MainWindow::on_actionManualPlay_triggered()
{
    R = 0;
    B = 0;
    globalTop = 0;
    n_total_flows = n_ignored_flows = 0;
    ResetCounter();
    BTST.clear();
    readHistory.clear();
    writeHistory.clear();
    Palette.clear();
    JobQueue.clear();// = FibQueue<Job>();
    foreach (Flow *flow, run) {
        delete flow;
    }
    run.clear();
    mainFlow = new Flow;
    mainFlow->parse(code);
    mainFlow->pushToJobQueue();
    if(Counter.isEmpty()) {
        ui->treeWidget->clear();
        mainFlow->toTreeWidgetItem(ui->treeWidget);
    }
    globalTop=0;
    resizeScene();
}

void MainWindow::on_actionStep_Backward_triggered()
{
    if(!run.isEmpty()) {
        Flow *flow = NULL;
        for(int i=run.size()-1; i>0; i--) {
            flow = run[i];
            if(!flow->ignored()) break;
            flow = NULL;
        }
        if(flow) {
            JobQueue.push(Job(flow->entringTime(), flow->copy()));
            flow->rollback();
            resizeScene();
        }
    }
}

void MainWindow::on_actionStep_Forward_triggered()
{
    globalTop = nextStep(globalTop);
    resizeScene();
}

void MainWindow::on_action_None_triggered()
{
    setDataPolicy(DontCare);
}

void MainWindow::on_actionStall_Injection_Write_Only_triggered()
{
    setDataPolicy(StallInjectionWriteOnly);
}

void MainWindow::on_actionStall_Injection_Read_and_Write_triggered()
{
    setDataPolicy(StallInjectionReadWrite);
}

void MainWindow::on_action_Provisional_Execution_triggered()
{
    setDataPolicy(ProvisionalExecution);
}

void MainWindow::on_action_Splittable_triggered()
{
    setSplitPolicy(Splittable);
}

void MainWindow::on_actionOnly_Processes_triggered()
{
    setSplitPolicy(OnlyProcess);
}

void MainWindow::on_actionReads_and_Processes_triggered()
{
    setSplitPolicy(ReadAndProcess);
}

void MainWindow::on_action_Unsplittable_triggered()
{
    setSplitPolicy(Unsplittable);
}
