#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QDataStream>
#include <QDateTime>
#include <QHostAddress>
#include <QMessageBox>
#include "structs.h"

class TCPClient : public QObject {

    Q_OBJECT

public:

    explicit TCPClient(QObject *parent = nullptr);

    void SendRequest(ServiceHeader head);
    void SendData(ServiceHeader head, QString data);
    void ConnectToHost(QHostAddress host, uint16_t port);
    void DisconnectFromHost(void);

signals:

    void sig_sendFreeSize(uint32_t size);
    void sig_sendStat(StatServer stat);
    void sig_sendTime(QDateTime time);
    void sig_SendReplyForSetData(QString data);
    void sig_connectStatus(uint16_t status);
    void sig_Disconnected(void);

private slots:

    void ReadyReed(void);
    void ProcessingData(ServiceHeader header, QDataStream &stream);

private:

    QTcpSocket* socket;
    ServiceHeader servHeader;
};

#endif // TCPCLIENT_H
