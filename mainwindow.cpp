#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent, Qt::FramelessWindowHint),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->saveBox->setEnabled(false);
    ui->charBox->setEnabled(false);
    ui->optBox->setEnabled(false);
    ui->saveSlotCB->setEnabled(false);

    QObject::connect(ui->findButton, SIGNAL(clicked()), this, SLOT(selectFile()));

    QObject::connect(this, SIGNAL(isFileDMC(bool)), ui->saveBox, SLOT(setEnabled(bool)));
    QObject::connect(this, SIGNAL(isFileDMC(bool)), ui->charBox, SLOT(setEnabled(bool)));
    QObject::connect(this, SIGNAL(isFileDMC(bool)), ui->optBox, SLOT(setEnabled(bool)));
    QObject::connect(this, SIGNAL(isFileDMC(bool)), ui->saveSlotCB, SLOT(setEnabled(bool)));

    QObject::connect(ui->saveSlotCB, SIGNAL(currentIndexChanged(int)), this, SLOT(updateInfo(int)));

    QObject::connect(ui->saveButton, SIGNAL(clicked()), this, SLOT(saveClicked()));

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    m_nMouseClick_X_Coordinate = event->x();
    m_nMouseClick_Y_Coordinate = event->y();
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    move(event->globalX()-m_nMouseClick_X_Coordinate,event->globalY()-m_nMouseClick_Y_Coordinate);
}

void MainWindow::selectFile(){
    ui->directoryLine->setText(QFileDialog::getOpenFileName());
    if (ui->directoryLine->text().compare("") == 0){ //Check if valid file selected
        QMessageBox msg;
        msg.setText("No valid file has been selected.");
        msg.setIcon(QMessageBox::Critical);
        msg.exec();
    } else {
        //Open file and load to buffer
        char buffer[5];
        int isValidSave = 0, i, validSlots[10] = {0};
        std::ifstream saveFile (ui->directoryLine->text().toStdString(), std::ios::binary | std::ios::in);

        //Check for a valid DMC1 saveslot on file
        for (i=0; i<24150; i+=2416){ //Go through all the 10 save header positions
            saveFile.seekg(i);
            saveFile.read(buffer, 5);
            if (buffer[0] == 1 && buffer[1] == 0 && buffer[2] == 0){ //Check if the header is of a DMC save file (0x01 0x00 0x00)
                isValidSave = 1;
                validSlots[i/2416] = 1;
            }
        }

        if (isValidSave == 0) {
            QMessageBox msg;
            msg.setText("The file selected is not a Devil May Cry 1 save file.");
            msg.setIcon(QMessageBox::Critical);
            msg.exec();
            emit isFileDMC(false);
        } else {
            ui->saveSlotCB->clear();

            for (i=0; i<10; i++){
                if (validSlots[i] == 1){
                    ui->saveSlotCB->addItem(QString::number(i+1));
                }
            }

            emit isFileDMC(true);

        }
        saveFile.close();
    }
}

void MainWindow::updateInfo(int index){
    if (index == -1) return; //quick fix for file change crash

    int saveSlot = std::stoi(ui->saveSlotCB->itemText(index).toStdString())-1, playtime;

    char buffer[4];
    std::ifstream saveFile (ui->directoryLine->text().toStdString(), std::ios::binary | std::ios::in);

    saveFile.seekg(saveSlot*2416+32); //Read save count and times beaten
    saveFile.read(buffer, 4);
    ui->scSpinBox->setValue(buffer[0] | (buffer[1]<<8));
    ui->tbSpinBox->setValue(buffer[2] | (buffer[3]<<8));

    saveFile.seekg(saveSlot*2416+36); //Read mission
    saveFile.read(buffer, 1);
    ui->cmSpinBox->setValue(buffer[0]);

    saveFile.seekg(saveSlot*2416+38); //Read difficulty
    saveFile.read(buffer, 1);
    ui->dComboBox->setCurrentIndex((buffer[0]-1)/2);

    saveFile.seekg(saveSlot*2416+1572); //Read vitality and devil trigger
    saveFile.read(buffer, 2);
    ui->vSpinBox->setValue(buffer[0]);
    ui->dtSpinBox->setValue(buffer[1]);

    saveFile.seekg(saveSlot*2416+1588); //Read red orbs
    saveFile.read(buffer, 4);
    ui->rSpinBox->setValue(buffer[0] | (buffer[1]<<8) | (buffer[2]<<16) | (buffer[3]<<24));

    saveFile.seekg(saveSlot*2416+1592); //Read blue orbs
    saveFile.read(buffer, 1);
    ui->bSpinBox->setValue(buffer[0]);

    saveFile.seekg(saveSlot*2416+1568); //Read yellow orbs
    saveFile.read(buffer, 2);
    ui->ySpinBox->setValue(buffer[0] | (buffer[1]<<8));

    saveFile.seekg(saveSlot*2416+44); //Read playtime and convert
    saveFile.read(buffer, 4);
    playtime = (buffer[0] | (buffer[1]<<8) | (buffer[2]<<16) | (buffer[3]<<24))/60;
    ui->hSpinBox->setValue(playtime/3600);
    playtime = playtime%3600;
    ui->mSpinBox->setValue(playtime/60);
    playtime = playtime%60;
    ui->sSpinBox->setValue(playtime);

    saveFile.close();
}

void MainWindow::saveClicked(){
    QMessageBox msg;
    msg.setText("Saving will OVERWRITE your save file! Make sure to backup your save before using this tool.");
    msg.setInformativeText("Are you sure you want to save?");
    msg.setStandardButtons(QMessageBox::Save | QMessageBox::Discard);
    msg.setDefaultButton(QMessageBox::Discard);
    msg.setIcon(QMessageBox::Warning);
    int ret = msg.exec();

    if (ret == QMessageBox::Discard) return;

    char saveMem[24420]; //temp file memory allocation

    int aux, saveSlot = std::stoi(ui->saveSlotCB->currentText().toStdString())-1;
    std::fstream saveFile (ui->directoryLine->text().toStdString(), std::ios::binary | std::ios::out | std::ios::in);

    saveFile.read(saveMem, 24420); //load save file into memory

    aux = ui->scSpinBox->value(); //Write save count
    saveMem[saveSlot*2416+32] = aux;
    saveMem[saveSlot*2416+33] = aux>>8;

    aux = ui->cmSpinBox->value(); //Write current mission
    saveMem[saveSlot*2416+36] = aux;

    aux = ui->tbSpinBox->value(); //Write times beaten
    saveMem[saveSlot*2416+34] = aux;
    saveMem[saveSlot*2416+35] = aux>>8;

    aux = ui->vSpinBox->value(); //Write vitality
    saveMem[saveSlot*2416+1572] = aux;

    aux = ui->dtSpinBox->value(); //Write devil trigger
    saveMem[saveSlot*2416+1573] = aux;

    aux = ui->rSpinBox->value(); //Write red orbs
    saveMem[saveSlot*2416+1588] = aux;
    saveMem[saveSlot*2416+1589] = aux>>8;
    saveMem[saveSlot*2416+1590] = aux>>16;
    saveMem[saveSlot*2416+1591] = aux>>24;

    aux = ui->bSpinBox->value(); //Write blue orbs
    saveMem[saveSlot*2416+1592] = aux;

    aux = ui->ySpinBox->value(); //Write yellow orbs
    saveMem[saveSlot*2416+1568] = aux;
    saveMem[saveSlot*2416+1569] = aux>>8;

    aux = ui->dComboBox->currentIndex()*2+1; //Write difficulty
    saveMem[saveSlot*2416+38] = aux;

    aux = ui->hSpinBox->value()*60*60; //Write playtime
    aux+= ui->mSpinBox->value()*60;
    aux+= ui->sSpinBox->value();
    aux = aux*60;
    saveMem[saveSlot*2416+44] = aux;
    saveMem[saveSlot*2416+45] = aux>>8;
    saveMem[saveSlot*2416+46] = aux>>16;
    saveMem[saveSlot*2416+47] = aux>>24;

    saveFile.seekp(0);
    saveFile.write(saveMem,24420);
    saveFile.close();

    QMessageBox msgOk;
    msgOk.setText("Your save file has been modified.");
    msgOk.exec();
}
