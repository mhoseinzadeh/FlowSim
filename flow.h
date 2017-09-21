#ifndef FLOW_H
#define FLOW_H

#include <QGraphicsObject>
#include <QVector>
#include <block.h>
#include <queue>
#include <qpriorityqueue.h>
#include <fiboqueue.h>
#include <functional>
#include <QTreeWidgetItem>

struct Job;

class Flow : public QGraphicsObject
{
    Q_OBJECT
public:
    enum FlowType {ftSerial=0, ftConcurrent=1, ftConditional=2};

    explicit Flow(QGraphicsItem *parent = 0);
    explicit Flow(Flow *other, QGraphicsItem *parent);

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    uint64_t run(uint64_t cycle=0);
    void updateLatency();
    void parse(QString code);
    void rollback();
    int bottom();
    void ignore();
    void ignoreLater();
    void pushToJobQueue();
    Block *parentBlock();
    Flow *copy(QGraphicsItem *parent=0);

    FlowType flowType() const;
    void setFlowType(const FlowType &flowType);
    bool setIgnored(const bool &p);

    QString toString(const QString &indent="") const;
    void toTreeWidgetItem(QTreeWidget *view, QTreeWidgetItem *top=0);

    friend std::ostream &operator<< (std::ostream &out, const Job &b);

    uint64_t entringTime() const;
    uint64_t exitingTime() const;

    Block* operator[](const int &idx) {
        return blocks[idx];
    }
    bool ignored();
    void repeat(bool rerun=false);
    Job *repeated();

    friend class Block;
    friend struct Job;
    friend void executeIgnoreList(std::function<void(Flow *)>);

signals:

public slots:

private:
    bool m_ignored;
    bool m_simulated;
    bool m_stalling;
    Job *m_repeated;
    uint64_t m_entringTime;
    uint64_t m_exitingTime;
    QString code;
    FlowType m_flowType;
    QVector<Block*> blocks;
};

template<typename T>
class SortedList: public QVector<T> {
public:
    void sortinsert(const T &v) {
        if(this->isEmpty()) {
            this->push_back(v);
        } else {
            int len=this->size()/2;
            typename QVector<T>::iterator it = this->begin()+len;
            while(len > 0) {
                if(v == *it) {
                    break;
                } else if(v > *it) {
                    it += len / 2;
                } else {
                    it -= len / 2;
                }
                len /= 2;
            }
            this->insert(it, v);
        }
    }
};

struct Job {
    uint64_t time;
    Flow *flow;
    Job(uint64_t t=0, Flow *f=0) {
        flow = f;
        time = t;
    }
    operator QString() {
        if(flow) {
            if(!flow->blocks.isEmpty()) {
                return QString().sprintf("(%llu, %s)", time, flow->blocks[0]->name().toStdString().c_str());
            }
        }
        return QString().sprintf("(%llu, null)", time);
    }
};
std::ostream &operator<< (std::ostream &out, const Job &b);


inline bool operator<(const Job &a, const Job &b) {
    return a.time > b.time; // The smaller time, the higher priority
}

//inline bool operator>(const Job &a, const Job &b) {
//    return a.time > b.time;
//}

//inline bool operator==(const Job &a, const Job &b) {
//    return a.time == b.time;
//}

namespace std {
template <>
struct hash<Job>
{
    std::size_t operator()(const Job& k) const
    {
        using std::hash;
        return (hash<uint64_t>()(k.time) ^ (hash<void*>()(k.flow)));
    }
};
}

extern QPriorityQueue<Job> JobQueue;
extern QVector<Flow*> ignoreList;
extern uint64_t globalTick;
extern bool stopRepeating;

void executeIgnoreList(std::function<void(Flow *)> removeFromList);

#endif // FLOW_H
