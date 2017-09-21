#include "flow.h"
#include <stdlib.h>     /* srand, rand */
#include <limits>
#include <QPainter>
#include <QVector>

bool stopRepeating=false;
uint64_t globalTick = 0;
QPriorityQueue<Job> JobQueue;
QVector<Flow*> ignoreList;

void executeIgnoreList(std::function<void(Flow *)> removeFromList) {
    for(QVector<Flow*>::iterator it = ignoreList.begin(); it != ignoreList.end(); it++) {
        Flow *flow = *it;
        flow->ignore();
        Job *j = flow->repeated();
        if(!j) {
          flow->repeat(true);
        } else if(j->flow->m_simulated) {
            flow->repeat(true);
        } else {
            removeFromList(j->flow);
            delete j->flow;
            j->flow = flow->copy();
        }
    }
    ignoreList.clear();
}


std::ostream &operator<< (std::ostream &out, const Job &b)
{
    if(b.flow) {
        if(!b.flow->blocks.empty()) {
            return out << QString().sprintf("[%llu, %s]", b.time, b.flow->blocks[0]->name().toStdString().c_str()).toStdString();
        }
    }
    return out << QString().sprintf("%llu", b.time).toStdString();
}

Flow::Flow(QGraphicsItem *parent) : QGraphicsObject(parent),
    m_flowType(ftSerial)
{
    m_ignored = false;
    m_simulated = false;
    m_stalling = false;
    m_repeated = NULL;
    m_entringTime = 0;
    m_exitingTime = 0;
}

Flow::Flow(Flow *other, QGraphicsItem *parent) :
    QGraphicsObject(parent)
{
    m_ignored = false;
    m_simulated = false;
    m_repeated = NULL;
    m_stalling = other->m_stalling;
    code = other->code;
    m_flowType = other->m_flowType;
    m_entringTime = other->m_entringTime;
    m_exitingTime = other->m_exitingTime;
    parse(code);
}

//Flow::~Flow()
//{
//    for(int i=0; i<blocks.size(); i++) {
//        delete blocks[i];
//    }
//    blocks.clear();
//}

QRectF Flow::boundingRect() const
{
    float r=0, h=0;
    float l = std::numeric_limits<float>::max();
    for(int i=0; i<blocks.size(); i++) {
        r = qMax<float>(r, blocks[i]->boundingRect().right());
        if(blocks[i]->m_latency != 0) {
            l = qMin<float>(l, blocks[i]->boundingRect().left());
        }
        if(m_flowType == ftSerial) {
            h = qMax<float>(h, blocks[i]->boundingRect().height());
        } else {
            h += blocks[i]->boundingRect().height();
        }
    }
    if(l == std::numeric_limits<float>::max())
        l = m_entringTime*CycleWidth;
    return QRectF(l, 0, r-l, h);
}

void Flow::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if(boundingRect().height() > 0) {
        QBrush brush;
        brush.setStyle(Qt::BDiagPattern);
        painter->setBrush(brush);
        painter->drawRect(boundingRect());
    }
}

uint64_t Flow::run(uint64_t cycle)
{
    uint64_t c = cycle;
    uint64_t latency = 0;
    m_entringTime = cycle;
    m_simulated = true;
    if(m_flowType == ftConditional) {
        updateLatency();
    }
    for(int i=0; i<blocks.size(); i++) {
        Block *block = blocks[i];
        if(block->name() == "repeat") {
            if(!stopRepeating) {
                block->updateLatency();
                repeat();
            }
        } else {
            if(m_flowType == ftSerial) {
                c = block->run(c);
                latency += c-block->m_entringTime;
                if(block == blocks[0])
                    m_entringTime = block->m_entringTime;
            } else if(m_flowType == ftConcurrent) {
                c = qMax<int>(c, block->run(cycle));
                if(latency < c-block->m_entringTime) {
                    latency = c-block->m_entringTime;
                }
                if(block->m_entringTime < m_entringTime) {
                    m_entringTime = block->m_entringTime;
                }
            }
        }
    }
    if(m_stalling) {
        Block *pb = parentBlock();
        if(pb) {
            pb->mergeBusy(m_entringTime, c-m_entringTime);
        }
    }
    m_exitingTime = c;
    if(!parentItem()) {
//        runIgnoreList();
    }
    return c;
}

void Flow::updateLatency()
{
    qreal t=0;
    if(m_flowType == ftConditional) {
        uint32_t r = rand()%100;
        Block *exe=NULL;
        for(int i=0; i<blocks.size(); i++) {
            blocks[i]->updateLatency();
            if(r < blocks[i]->m_latency && !exe) {
                exe = new Block(this);
                if(blocks[i]->body) {
                    exe->parse(blocks[i]->body->code);
                }
                exe->updateLatency();
            }
            r -= blocks[i]->m_latency;
            delete blocks[i];
        }
        blocks.clear();
        if(exe) blocks.append(exe);
        m_flowType = ftSerial;
    }
    for(int i=0; i<blocks.size(); i++) {
        blocks[i]->setY(t);
        blocks[i]->updateLatency();
        if(m_flowType == ftConcurrent) {
            t += blocks[i]->boundingRect().height();
        }
    }
}

void Flow::parse(QString code)
{
    bool ok=false;
    this->code = code;
    QStringList list = code.split(QRegExp("\\s"), QString::SkipEmptyParts);
    if(list.size() < 2) return;
    QString w = list.takeFirst();
    if(w == "wait") {
        m_stalling = true;
        w = list.takeFirst();
    }
    if(w == "concurrent") {
        m_flowType = ftConcurrent;
    } else if(w == "conditional") {
        m_flowType = ftConditional;
    } else if (w == "serial") {
        m_flowType = ftSerial;
    } else {
        list.push_front(w);
        ok = true;
    }
    if(!ok) {
        list.takeFirst(); // {
        list.takeLast();  // }
    }
    w.clear();
    while(!list.isEmpty()) {
        QString s = list.takeFirst();
        ok = false;
        if(s == "{") {
            ok = true;
            int open = 1;
            while(open>0) {
                if(list.isEmpty()) {
                    qDebug() << "Error: Bad syntax";
                    return;
                }
                QString t = list.takeFirst();
                if(t == "{") open++;
                if(t == "}") open--;
                s += " " + t;
                if(open == 0) {
                    break;
                }
            }
        }
        w += " " + s;
        if(s == ";" || ok) {
            Block *b = new Block(this);
            b->parse(w);
            blocks.push_back(b);
            w.clear();
        }
    }
    setToolTip(toString());
}

void Flow::rollback()
{
    for(int i=0; i<blocks.size(); i++) {
        blocks[i]->rollback();
    }
    setIgnored(true);
}

int Flow::bottom()
{
    if(blocks.empty()) return boundingRect().bottom();
    int b=0;
    foreach (Block *blk, blocks) {
        b = qMax<int>(b, blk->y()+blk->boundingRect().height());
    }
    return y()+b;
}

void Flow::ignore()
{
    n_ignored_flows++;
    setIgnored(true);
}

void Flow::ignoreLater()
{
    ignoreList << this;
}

void Flow::pushToJobQueue()
{
    bool allFlow=true;
    for(int i=0; i<blocks.size(); i++) {
        if(!blocks[i]->body) allFlow=false;
    }
    if(allFlow) {
        for(int i=0; i<blocks.size(); i++) {
            JobQueue.push(Job(m_entringTime, new Flow(blocks[i]->body, 0)));
        }
    } else {
        JobQueue.push(Job(m_entringTime, new Flow(this, 0)));
    }
}

Block *Flow::parentBlock()
{
    return (Block *)this->parentItem();
}

Flow *Flow::copy(QGraphicsItem *parent)
{
    Flow *f = new Flow(parent);
    f->m_ignored = false;
    f->m_simulated = false;
    f->m_stalling = m_stalling;
    f->m_repeated = NULL;
    f->code = code;
    f->m_flowType = m_flowType;
    f->m_entringTime = m_entringTime;
    f->m_exitingTime = m_exitingTime;
    for(int i=0; i < blocks.size(); i++) {
        f->blocks << blocks[i]->copy(f);
    }
    return f;
}

Flow::FlowType Flow::flowType() const
{
    return m_flowType;
}

void Flow::setFlowType(const FlowType &flowType)
{
    m_flowType = flowType;
}

bool Flow::setIgnored(const bool &p)
{
    bool r=true;
    m_ignored = p;
    for(int i=0; i<blocks.size(); i++) {
        r &= blocks[i]->setIgnored(p);
    }
    return r;
}

QString Flow::toString(const QString &indent) const
{
    QString res = indent;
    if(blocks.isEmpty()) return "";
    if(blocks.size() == 1) {
        return blocks[0]->toString(indent);
    } else {
        if(m_flowType == ftSerial) {
            res += "serial {\n";
        } else if(m_flowType == ftConcurrent) {
            res += "concurrent {\n";
        } else {
            res += "conditional {\n";
        }
        for(int i=0; i<blocks.size(); i++) {
            res += blocks[i]->toString(indent+"  ");
        }
        res += indent + "}\n";
        return res;
    }
}

void Flow::toTreeWidgetItem(QTreeWidget *view, QTreeWidgetItem *top)
{
    if(blocks.isEmpty()) return;
    if(blocks.size() == 1) {
        blocks[0]->toTreeWidgetItem(view, top);
    } else {
        for(int i=0; i<blocks.size(); i++) {
            blocks[i]->toTreeWidgetItem(view, top);
        }
    }
}

uint64_t Flow::entringTime() const
{
    return m_entringTime;
}

bool Flow::ignored()
{
    if(!blocks.isEmpty()) {
        m_ignored = blocks[0]->state() == Ignored;
    }
    return m_ignored;
}

void Flow::repeat(bool rerun)
{
    if(rerun) {
        m_repeated = new Job(m_entringTime, this->copy());
    } else {
        m_repeated = new Job(m_entringTime, new Flow(this, 0));
    }
    JobQueue.push(*m_repeated);
}

Job *Flow::repeated()
{
    return m_repeated;
}

uint64_t Flow::exitingTime() const
{
    return m_exitingTime;
}
