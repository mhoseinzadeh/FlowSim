#include "block.h"
#include "flow.h"
#include <limits>
#include <QRegExp>
#include <QPainter>
#include <bst.h>
#include <QGraphicsSceneMouseEvent>
#include <QMessageBox>
#include <QGraphicsScene>
#include <QGraphicsView>

DataInterfrencePolicy dataPolicy = StallInjectionWriteOnly;
BlockSplitPloicy splitPolicy = Splittable;
QMap<QString, counter_t> Counter;
QMap<QString, QMap<uint64_t, char>> BTST;
QMap<QString, QMap<uint64_t, timeslot_t>> readHistory;
QMap<QString, QMap<uint64_t, timeslot_t>> writeHistory;
int n_total_flows=0;
int n_ignored_flows=0;
int CycleWidth=CYCLE_WIDTH;

QColor pastel[] = {
    QColor(135, 206, 235),
    QColor(50, 205, 50),
    QColor(186, 85, 211),
    QColor(240, 128, 128),
    QColor(70, 130, 180),
    QColor(154, 205, 50),
    QColor(64, 224, 208),
    QColor(255, 105, 180),
    QColor(240, 230, 140),
    QColor(210, 180, 140),
    QColor(143, 188, 139),
    QColor(100, 149, 237),
    QColor(221, 160, 221),
    QColor(95, 158, 160),
    QColor(255, 218, 185),
    QColor(255, 160, 122)
};

QMap<QString, QColor> Palette;
std::ostream &operator<< (std::ostream &out, timeslot_t &b) {
    return out << QString().sprintf("[%llu, %llu]", b.start, b.end).toStdString();
}

QDebug operator<<(QDebug debug, timeslot_t b) {
    return debug.noquote() << QString().sprintf("[%llu, %llu]", b.start, b.end);
}

void ResetCounter() {
    for(QMap<QString, counter_t>::iterator it=Counter.begin(); it!=Counter.end(); it++) {
        it.value().count = 0;
        if(it.value().item) {
            it.value().item->setText(1, "0");
        }
    }
}

Block::Block(QGraphicsItem *parent) : QGraphicsObject(parent),
    m_latency(std::numeric_limits<uint32_t>::max()), m_latencyMin(0),
    m_latencyMax(0), m_name(""), f_name(""), body(nullptr)
{
    btst = nullptr;
    m_state = Realized;
    type = Process;
    m_entringTime = 0;
    m_split = false;
    m_stalling = false;
    m_simulated = false;
}

//Block::~Block()
//{
//    if(body) {
//        delete body;
//    }
//}

QRectF Block::boundingRect() const
{
    if(body != nullptr) {
        return QRectF(m_entringTime*CycleWidth, 0, m_latency*CycleWidth, body->boundingRect().height());
    } else {
        return QRectF(m_entringTime*CycleWidth, 0, m_latency*CycleWidth, 20);
    }
}

void Block::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    if(m_latency == std::numeric_limits<uint32_t>::max()) {
        updateLatency();
    }
    if(m_latency>0) {
        if(!Palette.count(m_name)) {
            int s = Palette.size();
            Palette[m_name] = pastel[s%16].lighter(100+(s/16)*50);
        }
        QRectF r = QRectF(m_entringTime*CycleWidth, 0, m_latency*CycleWidth, 20);
        painter->setBrush(Palette[m_name]);
        if(m_split) {
            QPen pen = painter->pen();
            pen.setWidth(2);
            painter->setPen(pen);
            QFont f = painter->font();
            f.setBold(true);
            painter->setFont(f);
        }
        painter->drawRect(r);
        int x = 3;
        QString txt = m_name;
        if(type == Read) {
            QPixmap pix(":/eye");
            x += pix.width();
            painter->drawPixmap(m_entringTime*CycleWidth+1, 2, pix);
            txt = txt.split(" ").last();
        } else if(type == Write) {
            QPixmap pix(":/pencil");
            x += pix.width();
            painter->drawPixmap(m_entringTime*CycleWidth+1, 2, pix);
            txt = txt.split(" ").last();
        }
        if(x < r.width())
            painter->drawText(r.adjusted(x,0,0,0), Qt::AlignCenter, txt);
        r.adjust(0, 0, 1, 1);
        if(m_state == Provisioned) {
            painter->fillRect(r, QColor(255, 255, 255, 128));
        } else if(m_state == Ignored) {
            painter->fillRect(r, QColor(255, 255, 255, 128));
            painter->drawLine(r.left()-5, r.top()+10, r.right()+5, r.top()+10);
        }
    }
}


uint64_t getMax(uint64_t &t, QMap<uint64_t, timeslot_t> &m1, QMap<uint64_t, timeslot_t> &m2) {
    if(m1.isEmpty() && m2.isEmpty()) return t;
    if(!m1.isEmpty() && m2.isEmpty()) return qMax<uint64_t>(t, m1.last().end);
    if(m1.isEmpty() && !m2.isEmpty()) return qMax<uint64_t>(t, m2.last().end);
    return qMax<uint64_t>(t, qMax<uint64_t>(m1.last().end, m2.last().end));
}

uint64_t getMax(uint64_t &t, QMap<uint64_t, timeslot_t> &m) {
    if(m.isEmpty()) return t;
    return qMax<uint64_t>(t, m.last().end);
}

uint64_t Block::run(uint64_t cycle)
{
    uint64_t t=cycle;
    m_simulated=true;
    int original_latency=m_latency;
    if(!m_split) Counter[m_name].count++;
    QMap<uint64_t, timeslot_t>::iterator itwb;

    if(btst != nullptr && m_latency > 0) {
        QString var;
        if(type != Process) {
            var = m_name.split(' ')[1];
            if(dataPolicy == StallInjectionWriteOnly) {
                if(type == Write) {
                    t = getMax(t, readHistory[var], writeHistory[var]);
                }
            } else if(dataPolicy == StallInjectionReadWrite) {
                if(type == Read) {
                    t = getMax(t, writeHistory[var]);
                } else if(type == Write) {
                    t = getMax(t, readHistory[var], writeHistory[var]);
                }
            }
        }
        do {
            QMap<uint64_t, char>::iterator it = btst->lowerBound(t);
            if(m_latency <= 0) {
                if(it != btst->end()) {
                    if(t != it.key()) {
                        if(it.value() == 'e') {
                            t = it.key();
                        }
                    }
                }
                break;
            } else {
                if(it == btst->end()) {
                    btst->insert(t, 's');
                    btst->insert(t+m_latency, 'e');
                    break;
                } else if(t == it.key()) {
                    if(it.value() == 's') {
                        t = t + 1;
                    } else { // it.value() == 'e'
                        uint64_t s=it.key();
                        btst->erase(it);
                        QMap<uint64_t, char>::iterator it2 = btst->lowerBound(t+1);
                        if(it2 == btst->end()) {
                            btst->insert(t+m_latency, 'e');
                            break;
                        } else {
                            if(it2.key()-s == m_latency) {
                                btst->erase(it2);
                                break;
                            } else if(it2.key()-s > m_latency) {
                                btst->insert(t+m_latency, 'e');
                                break;
                            } else { // it2.key()-s < m_latency
                                if(splitPolicy == Unsplittable) {
                                    t = it2.key();
                                } else if((splitPolicy == OnlyProcess && type == Process) ||
                                          (splitPolicy == ReadAndProcess && (type == Process || type == Read)) ||
                                          (splitPolicy == Splittable)) {
                                    t = split(t, it2.key());
                                    btst->erase(it2);
                                    break;
                                }
                            }
                        }
                    }
                } else { // t < it.key
                    if(it.value() == 'e') {
                        t = it.key();
                    } else { // it.value() == 's'
                        if(it.key()-t == m_latency) {
                            btst->erase(it);
                            btst->insert(t, 's');
                            break;
                        } else if(it.key()-t > m_latency) {
                            btst->insert(t, 's');
                            btst->insert(t+m_latency, 'e');
                            break;
                        } else { // it.key()-t < m_latency
                            if(splitPolicy == Unsplittable) {
                                t = it.key() + 1;
                            } else if((splitPolicy == OnlyProcess && type == Process) ||
                                      (splitPolicy == ReadAndProcess && (type == Process || type == Read)) ||
                                      (splitPolicy == Splittable)) {
                                t = split(t, it.key());
                                btst->erase(it);
                                btst->insert(t, 's');
                                break;
                            }
                        }
                    }
                }
            }
        } while(true);
        m_entringTime = t;
        if(!m_split) Counter[m_name].max_entring = qMax<uint64_t>(Counter[m_name].max_entring, t);
        if(m_stalling) {
            Block *pb = parentBlock();
            if(pb) {
                pb->mergeBusy(t, m_latency);
            }
        }
        timeslot_t timeslot = timeslot_t(t, t+m_latency, this);

        if(type == Read)
            readHistory[var].insert(t, timeslot);
        else if(type == Write)
            itwb = writeHistory[var].insert(t, timeslot);
    }
    if (body != nullptr) {
        t = body->run(t+m_latency);
        if(m_latency == 0) {
            m_entringTime = body->entringTime();
            if(!m_split) Counter[m_name].max_entring = qMax<uint64_t>(Counter[m_name].max_entring, t);
        }
    }

    if(type == Write && dataPolicy == ProvisionalExecution) {
        if(btst != nullptr && m_latency > 0) {
            rerunNextReadFlows(itwb);
        }
    }

    return t+original_latency;
}

void Block::updateLatency()
{
    if(m_latency == std::numeric_limits<uint32_t>::max()) {
        if(m_latencyMax <= m_latencyMin) {
            m_latency = m_latencyMin;
        } else {
            m_latency = m_latencyMin + (qrand() % (m_latencyMax-m_latencyMin+1));
        }
    }
    if(body != nullptr) {
        body->updateLatency();
    }
}

void Block::parse(const QString &code)
{
    bool ok;
    this->code = code;
    QStringList list = code.split(QRegExp("\\s"), QString::SkipEmptyParts);
    if(list.size() < 2) return;
    m_name = list.takeFirst();
    if(m_name == "wait") {
        m_stalling = true;
        m_name = list.takeFirst();
    }
    if(m_name == "serial" || m_name == "concurrent" || m_name == "conditional") {
        body = new Flow(this);
        body->parse(code);
    } else {
        if(m_name == "read") {
            type = Read;
            m_name = "read "+list.takeFirst();
        } else if(m_name == "write") {
            type = Write;
            m_name = "write "+list.takeFirst();
        }
        f_name = m_name;
        setToolTip(m_name);
        btst = &BTST[m_name];
        QString lm = list.takeFirst();
        if(lm == "rand") {
            lm = list.takeFirst();
            if(list.isEmpty()) {
                qDebug() << "Error: `rand` needs 2 arguments";
                return;
            }
            QString lM = list.takeFirst();
            m_latencyMin = lm.toInt(&ok);
            if(!ok) {
                qDebug() << "Error: `" << lm << "` is not a valid number";
                return;
            }
            m_latencyMax = lM.toInt(&ok);
            if(!ok) {
                qDebug() << "Error: `" << lM << "` is not a valid number";
                return;
            }
        } else {
            m_latencyMin = m_latencyMax = m_latency = lm.toInt(&ok);
            if(!ok && lm != "_") {
                qDebug() << "Error: `" << lm << "` is not a valid number";
                return;
            }
        }
        if(!list.isEmpty()) {
            QString w = list.takeFirst();
            if(w == "{") {
                w.clear();
                int open = 1;
                while(open>0) {
                    if(list.isEmpty()) {
                        qDebug() << "Error: Bad syntax";
                        return;
                    }
                    QString t = list.takeFirst();
                    if(t == "{") open++;
                    if(t == "}") open--;
                    if(open == 0) {

                    } else {
                        w += " " + t;
                    }
                }
                w = "serial { " + w + " }";
                body = new Flow(this);
                body->parse(w);
            }
        }
    }
}

void Block::rollback()
{
    if(!m_name.isEmpty() && btst != nullptr && m_name != "case" && m_latency > 0) {
        uint64_t t1 = m_entringTime, t2 = m_entringTime + m_latency;
        QMap<uint64_t, char>::iterator b = btst->lowerBound(t1);
        QMap<uint64_t, char>::iterator e = btst->lowerBound(t2);
        if(b.key() == t1 && e.key() == t2) {
            btst->erase(b);
            btst->erase(e);
        } else if(b.key() == t1) {
            btst->erase(b);
            btst->insert(t2, 's');
        } else if(e.key() == t2) {
            btst->erase(e);
            btst->insert(t1, 'e');
        } else {
            btst->insert(t1, 'e');
            btst->insert(t2, 's');
        }

        if(type != Process) {
            QString v = m_name.split(' ')[1];
            readHistory[v].remove(t1);
            writeHistory[v].remove(t1);
        }
    }
    if(body != nullptr) body->rollback();
}

void Block::rerunNextReadFlows(QMap<uint64_t, timeslot_t>::iterator &itwe)
{
    QString var = m_name.split(' ')[1];
    QMap<uint64_t, timeslot_t> *listw = &writeHistory[var];
    QMap<uint64_t, timeslot_t> listr = readHistory[var];
    QMap<uint64_t, timeslot_t>::iterator itwb = itwe++;

    QMap<uint64_t, timeslot_t>::iterator it = listr.lowerBound(itwb.key()+1);
    while(it != listr.end()) {
        if(itwe != listw->end()) {
            if(it.key() > itwe.key()) break;
        }
        Flow *flow = it.value().block->ancestorFlow();
        flow->ignoreLater();
        it++;
    }
}

QString Block::name() const
{
    return m_name;
}

void Block::setName(const QString &name)
{
    m_name = name;
}

uint32_t Block::latencyMin() const
{
    return m_latencyMin;
}

void Block::setLatencyMin(uint32_t latencyMin)
{
    m_latencyMin = latencyMin;
}

uint32_t Block::latencyMax() const
{
    return m_latencyMax;
}

void Block::setLatencyMax(uint32_t latencyMax)
{
    m_latencyMax = latencyMax;
}

uint64_t Block::entringTime() const
{
    return m_entringTime;
}

void Block::setEntringTime(uint64_t entringTime)
{
    m_entringTime = entringTime;
}

uint64_t Block::split(uint64_t start, uint64_t end)
{
    bool ok=false;
    switch(splitPolicy) {
    case Unsplittable: break;
    case Splittable: ok = true; break;
    case ReadAndProcess: if(type == Read) ok = true;
    case OnlyProcess: if(type == Process) ok = true;
    }
    if(ok) {
        setToolTip(m_name + QString().sprintf(" (split, lat=%d)", m_latency));
        m_split = true;
        Block *b = clone();
        m_latency = end - start;
        b->m_latency -= m_latency;
        parentFlow()->blocks.push_back(b);
    }
    return start;
}

Block *Block::clone()
{
    Block *b = new Block(this->parentItem());
    b->m_name = m_name;
    b->type = type;
    b->btst = btst;
    b->m_split = m_split;
    b->m_latency = m_latency;
    b->setToolTip(toolTip());
    return b;
}

Block *Block::copy(QGraphicsItem *parent)
{
    Block *b = new Block(parent);
    b->setToolTip(toolTip());
    b->m_name = m_name;
    b->type = type;
    b->btst = btst;
    b->m_latency = m_latency;
    b->m_latencyMin = m_latencyMin;
    b->m_latencyMax = m_latencyMax;
    b->m_state = Realized;
    b->code = code;
    b->m_entringTime = 0;
    b->m_stalling = m_stalling;
    b->m_split = false;
    b->m_simulated = false;
    if(body != nullptr) {
        b->body = body->copy(b);
    }

    return b;
}

Block *Block::parentBlock()
{
    Flow *p = parentFlow();
    if(p) {
        Block *pb = p->parentBlock();
        if(pb) {
            return pb;
        }
    }
    return NULL;
}

Flow *Block::parentFlow()
{
    return ((Flow*)this->parentItem());
}

Flow *Block::ancestorFlow()
{
    QGraphicsItem *item = parentItem();
    if(item) {
        while(item->parentItem())
            item = item->parentItem();
    }
    if (item) {
        return (Flow*) item;
    } else {
        return NULL;
    }
}

QString Block::toString(const QString &indent) const
{
    if(m_name.isEmpty()) {
        QString res;
        if(body != nullptr) {
            res += body->toString(indent);
        }
        return res;
    }
    QString res = indent + m_name + " ";
    if(m_latencyMin == m_latencyMax) {
        res += QString::number(m_latencyMin);
    } else {
        res += "rand " + QString::number(m_latencyMin) + " " + QString::number(m_latencyMax);
    }
    if(body == nullptr) {
        res += ";\n";
    } else {
        res += " {\n";
        res += body->toString(indent+"  ");
        res += indent + "}\n";
    }
    return res;
}

void Block::toTreeWidgetItem(QTreeWidget *view, QTreeWidgetItem *top)
{
    QTreeWidgetItem *item = NULL;
    if(view) {
        item = new QTreeWidgetItem(view);
        view->addTopLevelItem(item);
    } else {
        item = new QTreeWidgetItem(top);
    }
    if(m_name == "case") {
        item->setText(0, QString().sprintf("%s %d%%", m_name.toStdString().c_str(), m_latency));
    } else {
        item->setText(0, m_name);
    }
    item->setText(1, "0");
    Counter[m_name].item = item;
    if(body != nullptr) {
        body->toTreeWidgetItem(0, item);
    }

}

uint32_t Block::latency() const
{
    return m_latency;
}

bool Block::setIgnored(const bool &p)
{
    bool r = true;
    if(p) {
        m_state = Ignored;
        r = setBusy(false);
    } else {
        m_state = Realized;
        r = setBusy(true);
    }
    if(body != nullptr) { r &= body->setIgnored(p); }
    return r;
}

#define OTHER_INFO << m_name << t << m_latency << b

bool Block::setBusy(const bool &b)
{
    if(m_latency == 0) return true;
    uint64_t t = m_entringTime;

    QString var;
    if(type != Process) {
        // TODO: Should consider other affected flows as well
        var = m_name.split(' ')[1];
        QMap<uint64_t, timeslot_t> *list = NULL;
        if(type == Write) {
            list = &writeHistory[var];
        } else { // type == Read
            list = &readHistory[var];
        }
        if(b) {
            QMap<uint64_t, timeslot_t>::iterator it = list->find(t);
            if(it == list->end())
                list->insert(t, timeslot_t(t, t+m_latency, this));
        } else {
            QMap<uint64_t, timeslot_t>::iterator it = list->find(t);
            if(it != list->end())
                list->erase(it);
        }
    }

    if(b) {
        QMap<uint64_t, char>::iterator it = btst->lowerBound(t);
        if(it == btst->end()) {
            btst->insert(t, 's');
            btst->insert(t+m_latency, 'e');
        } else if(t == it.key()) {
            if(it.value() == 's') {
                qDebug() << "Error: should rerun 1" OTHER_INFO;
                return false;
            } else { // it.value() == 'e'
                uint64_t s = it.key();
                btst->erase(it);
                QMap<uint64_t, char>::iterator it2 = btst->lowerBound(t+1);
                if(it2 == btst->end()) {
                    btst->insert(t+m_latency, 'e');
                } else {
                    if(it2.key()-s == m_latency) {
                        btst->erase(it2);
                    } else if(it2.key()-s > m_latency) {
                        btst->insert(t+m_latency, 'e');
                    } else { // it2.key()-s < m_latency
                        btst->insert(s, it.value());
                        qDebug() << "Error: should rerun 2" OTHER_INFO;
                        return false;
                    }
                }
            }
        } else { // t < it.key
            if(it.value() == 'e') {
                qDebug() << "Error: should rerun 3" OTHER_INFO;
                return false;
            } else { // it.value() == 's'
                if(it.key()-t == m_latency) {
                    btst->erase(it);
                    btst->insert(t, 's');
                } else if(it.key()-t > m_latency) {
                    btst->insert(t, 's');
                    btst->insert(t+m_latency, 'e');
                } else { // it.key()-t < m_latency
                    qDebug() << "Error: should rerun 4" OTHER_INFO;
                    return false;
                }
            }
        }
    } else {

    }
    return true;
}

bool Block::mergeBusy(uint64_t &t, uint32_t lat)
{
    if(btst == nullptr || m_latency == 0) {
        if(f_name.isEmpty()) {
            Block *pb = parentBlock();
            if(pb) {
                return pb->mergeBusy(t, lat);
            }
            return false;
        } else {
            if(btst == nullptr) {
                btst = &BTST[f_name];
                m_latency = 1;
            }
        }
    }
    do {
        if(lat == 0) break;
        QMap<uint64_t, char>::iterator it = btst->lowerBound(t);
        if(it == btst->end()) {
            btst->insert(t, 's');
            btst->insert(t+lat, 'e');
            break;
        } else if(t == it.key()) {
            if(it.value() == 's') {
                QMap<uint64_t, char>::iterator it2 = btst->lowerBound(t+1);
                if(it2.key()-it.key() >= lat) {
                    break;
                } else { // it2.key()-it.key() < lat
                    t = it2.key();
                    lat -= it2.key()-t;
                }
            } else { // it.value() == 'e'
                uint64_t s = it.key();
                btst->erase(it);
                QMap<uint64_t, char>::iterator it2 = btst->lowerBound(t+1);
                if(it2 == btst->end()) {
                    btst->insert(t+lat, 'e');
                    break;
                } else {
                    if(it2.key()-s == lat) {
                        btst->erase(it2);
                        break;
                    } else if(it2.key()-s > lat) {
                        btst->insert(t+lat, 'e');
                        break;
                    } else { // it2.key()-s < lat
                        t = it2.key();
                        lat -= it2.key()-s;
                    }
                }
            }
        } else { // t < it.key
            if(it.value() == 'e') {
                lat -= it.key()-t;
                t = it.key();
            } else { // it.value() == 's'
                if(it.key()-t == lat) {
                    btst->erase(it);
                    btst->insert(t, 's');
                    break;
                } else if(it.key()-t > lat) {
                    btst->insert(t, 's');
                    btst->insert(t+lat, 'e');
                    break;
                } else { // it.key()-t < lat
                    uint64_t st = it.key();
                    btst->erase(it);
                    btst->insert(t, 's');
                    lat -= st-t;
                    t = st;
                }
            }
        }
    } while (true);
    return true;
}

void Block::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    event->accept();
    Flow *flow = ancestorFlow();
    if (flow) {
//        if(!flow->setIgnored(!flow->ignored())) {
//            if(QMessageBox::warning(scene()->views()[0]->window(), tr("Warning"),
//                                    tr("The flow cannot be placed as previous.\n"
//                                       "Do you want to rerun it?"),
//                                    QMessageBox::Yes | QMessageBox::No,
//                                    QMessageBox::No) == QMessageBox::Yes) {
//                flow->run(flow->entringTime());
//            }
//        }
//        flow->update();
    }
    QGraphicsObject::mouseDoubleClickEvent(event);
}

BlockState Block::state() const
{
    return m_state;
}

void Block::setState(const BlockState &state)
{
    m_state = state;
}


