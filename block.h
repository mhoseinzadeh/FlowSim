#ifndef BLOCK_H
#define BLOCK_H

#include <QtCore>
#include <QGraphicsObject>
#include <QTreeWidgetItem>
#include <QVector>
#include <QMap>
#include <string>
#include <bst.h>

class Flow;
class Block;

enum BlockState {Realized=0, Provisioned=1, Ignored=2};
enum BlockType {Process=0, Read=1, Write=2};
enum BlockSplitPloicy {Splittable=0, Unsplittable=1, OnlyProcess=2, ReadAndProcess=3};
enum DataInterfrencePolicy {DontCare=0, StallInjectionWriteOnly=1, StallInjectionReadWrite=2, ProvisionalExecution=3};

extern DataInterfrencePolicy dataPolicy;
extern BlockSplitPloicy splitPolicy;

struct timeslot_t {
    uint64_t start;
    uint64_t end;
    Block *block;
    timeslot_t(uint64_t start=0, uint64_t end=0, Block *block=NULL) :
        start(start), end(end), block(block)
    {}
    bool operator==(const timeslot_t &other) {
        return start == other.start && end == other.end;
    }
};
std::ostream &operator<< (std::ostream &out, timeslot_t &b);
QDebug operator<<(QDebug debug, timeslot_t b);

class Block : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit Block(QGraphicsItem *parent = 0);

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = Q_NULLPTR);

    uint64_t run(uint64_t cycle);
    void updateLatency();
    void parse(const QString &code);
    void rollback();
    void rerunNextReadFlows(QMap<uint64_t, timeslot_t>::iterator &itwe);
    uint64_t split(uint64_t start, uint64_t end);
    Block *clone();
    Block *copy(QGraphicsItem *parent);
    Block *parentBlock();
    Flow *parentFlow();
    Flow *ancestorFlow();
    bool setIgnored(const bool &p);
    bool setBusy(const bool &b);
    bool mergeBusy(uint64_t &t, uint32_t lat);
    uint32_t latency() const;

    QString name() const;
    void setName(const QString &name);

    uint32_t latencyMin() const;
    void setLatencyMin(uint32_t latencyMin);

    uint32_t latencyMax() const;
    void setLatencyMax(uint32_t latencyMax);

    uint64_t entringTime() const;
    void setEntringTime(uint64_t entringTime);

    BlockState state() const;
    void setState(const BlockState &state);

    QString toString(const QString &indent="") const;
    void toTreeWidgetItem(QTreeWidget *view, QTreeWidgetItem *top);

    friend class Flow;

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);

signals:

public slots:

private:
    uint32_t m_latency;
    uint32_t m_latencyMin;
    uint32_t m_latencyMax;
    bool m_split;
    bool m_stalling;
    bool m_simulated;
    uint64_t m_entringTime;
    QString m_name;
    QString f_name;
    QString code;
    Flow* body;
    BlockState m_state;
    BlockType type;
    QMap<uint64_t, char> *btst;
};

struct counter_t{
    long count;
    uint64_t max_entring;
    QTreeWidgetItem *item;
    counter_t(long count=0, QTreeWidgetItem *item=0, uint64_t max=1) :
        count(count), max_entring(max), item(item) {}
};
void ResetCounter();

extern QMap<QString, counter_t> Counter;
extern QMap<QString, QMap<uint64_t, char>> BTST;
extern QMap<QString, QMap<uint64_t, timeslot_t>> readHistory;
extern QMap<QString, QMap<uint64_t, timeslot_t>> writeHistory;
extern QMap<QString, QColor> Palette;
extern int n_total_flows;
extern int n_ignored_flows;
extern int CycleWidth;

#endif // BLOCK_H
