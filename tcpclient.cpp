#include "tcpclient.h"

TCPClient::TCPClient(QObject *parent) : QObject{parent} {

    socket = new QTcpSocket(this);

    QObject::connect(socket, &QTcpSocket::readyRead, this, &TCPClient::ReadyReed);
    QObject::connect(socket, &QTcpSocket::connected, this, [&] () {

        emit sig_connectStatus(STATUS_SUCCES);
    });
    QObject::connect(socket, &QTcpSocket::errorOccurred, this, [&] () {

        emit sig_connectStatus(ERR_CONNECT_TO_HOST);
    });
    QObject::connect(socket, &QTcpSocket::disconnected, this, &TCPClient::sig_Disconnected);
}



/* ServiceHeader
 * Для работы с потоками наши данные необходимо сериализовать.
 * Поскольку типы данных не стандартные перегрузим оператор << Для работы с ServiceHeader
*/

QDataStream &operator << (QDataStream &in, ServiceHeader &data) {

    in << data.id;
    in << data.idData;
    in << data.status;
    in << data.len;

    return in;
};

QDataStream &operator >> (QDataStream &out, ServiceHeader &data) {

    out >> data.id;
    out >> data.idData;
    out >> data.status;
    //out.skipRawData(3); если использовать memcpy (строка 96) и выравнивание по 4м байтам.
    out >> data.len;

    return out;
};



void TCPClient::SendRequest(ServiceHeader head) {

    QByteArray sendHdr;
    QDataStream outStr(&sendHdr, QIODevice::WriteOnly);

    outStr << head;

    socket->write(sendHdr);
}

void TCPClient::SendData(ServiceHeader head, QString str) {

    QByteArray sendData;
    QDataStream outStr(&sendData, QIODevice::WriteOnly);

    outStr << head;
    outStr << str;

    socket->write(sendData);
}


void TCPClient::ConnectToHost(QHostAddress host, uint16_t port) {

    socket->connectToHost(host, port);
}

void TCPClient::DisconnectFromHost() {

    socket->disconnectFromHost();
}


void TCPClient::ReadyReed() {

    QDataStream incStream(socket);

    if(incStream.status() != QDataStream::Ok){

        QMessageBox msg;

        msg.setIcon(QMessageBox::Warning);
        msg.setText("Ошибка открытия входящего потока для чтения данных!");
        msg.exec();
    }

    //Читаем до конца потока
    while(incStream.atEnd() == false) {

        //Если мы обработали предыдущий пакет, мы скинули значение idData в 0
        if(servHeader.idData == 0) {

            //Проверяем количество полученных байт. Если доступных байт меньше чем
            //заголовок, то выходим из обработчика и ждем новую посылку. Каждая новая
            //посылка дописывает данные в конец буфера
            if(socket->bytesAvailable() < sizeof(ServiceHeader)) {

                return;
            }
            else{

                //Читаем заголовок
                incStream >> servHeader;

                //Проверяем на корректность данных. Принимаем решение по заранее известному ID
                //пакета. Если он "битый" отбрасываем все данные в поисках нового ID.
                if(servHeader.id != ID) {

                    uint16_t hdr = 0;

                    while(incStream.atEnd()) {

                        incStream >> hdr;

                        if(hdr == ID) {

                            servHeader.id = hdr;

                            incStream >> servHeader.idData;
                            incStream >> servHeader.status;
                            incStream >> servHeader.len;

                            break;
                        }
                    }
                }
            }
        }

        //Если получены не все данные, то выходим из обработчика. Ждем новую посылку
        if(socket->bytesAvailable() < servHeader.len) {

            return;
        }
        else {

            //Обработка данных
            ProcessingData(servHeader, incStream);

            servHeader.idData = 0;
            servHeader.status = 0;
            servHeader.len = 0;
        }
    }
}

void TCPClient::ProcessingData(ServiceHeader header, QDataStream &stream) {

    switch (header.idData) {

        case GET_TIME: {

            QDateTime time;

            stream >> time;

            emit sig_sendTime(time);

            break;
        }

        case GET_SIZE: {

            uint32_t size = 0;

            stream >> size;

            emit sig_sendFreeSize(size);

            break;
        }

        case GET_STAT: {

            StatServer stat;

            stream >> stat.incBytes;
            stream >> stat.sendBytes;
            stream >> stat.revPck;
            stream >> stat.sendPck;
            stream >> stat.workTime;
            stream >> stat.clients;

            emit sig_sendStat(stat);

            break;
        }

        case SET_DATA: {

            if(header.status == ERR_NO_FREE_SPACE) {

                emit sig_SendReplyForSetData("Ошибка: сообщение больше свободного места на сервере.");
            }
            else if (header.status == ERR_CONNECT_TO_HOST) {

                emit sig_SendReplyForSetData("Ошибка: не указана длина сообщения в заголовке.");
            }
            else {

                QString msg;

                stream >> msg;

                emit sig_SendReplyForSetData(msg);
            }

            break;
        }

        case CLEAR_DATA: {

            emit sig_SendReplyForSetData("Сервер очищен.");

            break;
        }

    default:

        return;
    }
}
