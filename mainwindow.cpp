#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    client = new TCPClient(this);

    //Доступность полей по умолчанию
    ui->le_data->setEnabled(false);
    ui->pb_request->setEnabled(false);
    ui->lb_connectStatus->setText("Отключено");
    ui->lb_connectStatus->setStyleSheet("color: red");

    //При отключении меняем надписи и доступность полей.
    QObject::connect(client, &TCPClient::sig_Disconnected, this, [&] () {

        ui->lb_connectStatus->setText("Отключено");
        ui->lb_connectStatus->setStyleSheet("color: red");
        ui->pb_connect->setText("Подключиться");
        ui->le_data->setEnabled(false);
        ui->pb_request->setEnabled(false);
        ui->spB_port->setEnabled(true);
        ui->spB_ip1->setEnabled(true);
        ui->spB_ip2->setEnabled(true);
        ui->spB_ip3->setEnabled(true);
        ui->spB_ip4->setEnabled(true);
    });
    QObject::connect(client, &TCPClient::sig_sendTime, this, &MainWindow::DisplayTime);
    QObject::connect(client, &TCPClient::sig_connectStatus, this, &MainWindow::DisplayConnectStatus);
    QObject::connect(client, &TCPClient::sig_sendFreeSize, this, &MainWindow::DisplayFreeSpace);
    QObject::connect(client, &TCPClient::sig_sendStat, this, &MainWindow::DisplayStat);
    QObject::connect(client, &TCPClient::sig_SendReplyForSetData, this, &MainWindow::SetDataReply);
}

MainWindow::~MainWindow() {

    delete ui;
}



void MainWindow::on_pb_connect_clicked() {

    if(ui->pb_connect->text() == "Подключиться") {

        uint16_t port = ui->spB_port->value();

        QString ip = ui->spB_ip4->text() + "." +
                     ui->spB_ip3->text() + "." +
                     ui->spB_ip2->text() + "." +
                     ui->spB_ip1->text();

        client->ConnectToHost(QHostAddress(ip), port);
    }
    else {

        client->DisconnectFromHost();
    }
}

void MainWindow::on_pb_request_clicked() {

    ServiceHeader header;

    header.id = ID;
    header.status = STATUS_SUCCES;
    header.len = 0;

    switch (ui->cb_request->currentIndex()) {

            //Получить время
        case 0: {

            header.idData = GET_TIME;

            client->SendRequest(header);

            break;
        }

            //Получить свободное место
        case 1: {

            header.idData = GET_SIZE;

            client->SendRequest(header);

            break;
        }

            //Получить статистику
        case 2: {

            header.idData = GET_STAT;

            client->SendRequest(header);

            break;
        }

            //Отправить данные
        case 3: {

            QString data = ui->le_data->text();

            header.idData = SET_DATA;
            header.len += data.toUtf8().size();

            client->SendData(header, data);

            break;
        }

            //Очистить память на сервере
        case 4: {

            header.idData = CLEAR_DATA;

            client->SendRequest(header);

            break;
        }

    default:

        ui->tb_result->append("Такой запрос не реализован в текущей версии");

        return;
    }
}

void MainWindow::DisplayTime(QDateTime time) {

    ui->tb_result->append("Время сервера: " + time.toString());
}

void MainWindow::DisplayFreeSpace(uint32_t freeSpace) {

    QString num;

    ui->tb_result->append("Свободного места на сервере осталось: " + num.setNum(freeSpace) + " байт.");
}

void MainWindow::SetDataReply(QString replyString) {

    ui->tb_result->append(replyString);
}

void MainWindow::DisplayStat(StatServer stat) {

    QString num;

    ui->tb_result->append("Статистика сервера:");
    ui->tb_result->append("Принято байт: " + num.setNum(stat.incBytes));
    ui->tb_result->append("Передано байт: " + num.setNum(stat.sendBytes));
    ui->tb_result->append("Принто пакетов: " + num.setNum(stat.revPck));
    ui->tb_result->append("Передано пакетов: " + num.setNum(stat.sendPck));
    ui->tb_result->append("Время работы сервера секунд: " + num.setNum(stat.workTime));
    ui->tb_result->append("Количество подключенных клиентов: " + num.setNum(stat.clients));
}

void MainWindow::DisplayConnectStatus(uint16_t status) {

    if(status == ERR_CONNECT_TO_HOST) {

        ui->tb_result->append("Ошибка подключения к порту: " + QString::number(ui->spB_port->value()));
    }
    else {

        ui->lb_connectStatus->setText("Подключено");
        ui->lb_connectStatus->setStyleSheet("color: green");
        ui->pb_connect->setText("Отключиться");
        ui->spB_port->setEnabled(false);
        ui->pb_request->setEnabled(true);
        ui->spB_ip1->setEnabled(false);
        ui->spB_ip2->setEnabled(false);
        ui->spB_ip3->setEnabled(false);
        ui->spB_ip4->setEnabled(false);
    }
}

void MainWindow::on_cb_request_currentIndexChanged(int index) {

    if(ui->cb_request->currentIndex() == 3) {

        ui->le_data->setEnabled(true);
    }
    else {

        ui->le_data->setEnabled(false);
    }
}
