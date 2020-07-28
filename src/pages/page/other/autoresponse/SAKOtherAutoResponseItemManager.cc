﻿/*
 * Copyright 2018-2020 Qter(qsaker@qq.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part
 * of QtSwissArmyKnife project.
 *
 * QtSwissArmyKnife is licensed according to the terms in
 * the file LICENCE in the root of the source code directory.
 */
#include <QFile>
#include <QDebug>
#include <QDateTime>
#include <QJsonArray>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QJsonObject>
#include <QApplication>
#include <QJsonDocument>
#include <QStandardPaths>

#include "SAKDebugPage.hh"
#include "SAKDataStruct.hh"
#include "SAKOtherAutoResponseItem.hh"
#include "SAKOtherAutoResponseItemManager.hh"
#include "SAKDebugPageCommonDatabaseInterface.hh"

#include "ui_SAKOtherAutoResponseItemManager.h"

SAKOtherAutoResponseItemManager::SAKOtherAutoResponseItemManager(SAKDebugPage *debugPage, QWidget *parent)
    :QWidget (parent)
    ,debugPage (debugPage)
    ,ui (new Ui::SAKOtherAutoResponseItemManager)
{
    ui->setupUi(this);
    listWidget = ui->listWidget;
    forbidAllCheckBox = ui->forbidAllCheckBox;
    deleteItemPushButton = ui->deleteItemPushButton;
    addItemPushButton = ui->addItemPushButton;
    outportPushButton = ui->outportPushButton;
    importPushButton = ui->importPushButton;
    msgLabel = ui->msgLabel;

    setWindowTitle(tr("自动回复设置"));
//    databaseInterface = SAKDebugPageDatabaseInterface::instance();

    clearMessageInfoTimer.setInterval(SAK_CLEAR_MESSAGE_INTERVAL);
    connect(&clearMessageInfoTimer, &QTimer::timeout, this, &SAKOtherAutoResponseItemManager::clearMessage);

    /// @brief 从数据库读入记录
    readInRecord();
}

SAKOtherAutoResponseItemManager::~SAKOtherAutoResponseItemManager()
{
    delete ui;
}

void innerCreateItem(SAKDataStruct::SAKStructAutoResponseItem &var, SAKDebugPage *debugPage, QListWidget *listWidget)
{
    QListWidgetItem *item = new QListWidgetItem(listWidget);
    listWidget->addItem(item);
    SAKOtherAutoResponseItem *itemWidget = new SAKOtherAutoResponseItem(debugPage,
                                                                        var.id,
                                                                        var.name,
                                                                        var.referenceData,
                                                                        var.responseData,
                                                                        var.enable,
                                                                        var.referenceFormat,
                                                                        var.responseFormat,
                                                                        var.option,
                                                                        var.delay,
                                                                        var.interval,
                                                                        listWidget);
    item->setSizeHint(QSize(itemWidget->width(), itemWidget->height()));
    listWidget->setItemWidget(item, itemWidget);
}

void SAKOtherAutoResponseItemManager::outputMessage(QString msg, bool isInfo)
{
    QString color = "black";
    if (!isInfo){
        color = "red";
        QApplication::beep();
    }
    msgLabel->setStyleSheet(QString("QLabel{color:%1}").arg(color));

    msgLabel->setText(QTime::currentTime().toString("hh:mm:ss ") + msg);
    clearMessageInfoTimer.start();
}

void SAKOtherAutoResponseItemManager::clearMessage()
{
    clearMessageInfoTimer.stop();
    msgLabel->clear();
}

void SAKOtherAutoResponseItemManager::readInRecord()
{
    QString tableName = SAKDataStruct::autoResponseTableName(debugPage->pageType());
//    QList<SAKDataStruct::SAKStructAutoResponseItem> itemList = databaseInterface->selectAutoResponseItem(tableName);
//    for (auto var : itemList){
//        innerCreateItem(var, debugPage, listWidget);
//    }
}

bool SAKOtherAutoResponseItemManager::contains(quint64 paraID)
{
    bool contain = false;
    for (int i = 0; i < listWidget->count(); i++){
        QListWidgetItem *item = listWidget->item(i);
        QWidget *w = listWidget->itemWidget(item);
        SAKOtherAutoResponseItem *itemWidget = reinterpret_cast<SAKOtherAutoResponseItem*>(w);
        if (itemWidget->itemID() == paraID){
            contain = true;
            break;
        }
    }

    return contain;
}

void SAKOtherAutoResponseItemManager::on_forbidAllCheckBox_clicked()
{
    for(int i = 0; i < listWidget->count(); i++){
        QListWidgetItem *item = listWidget->item(i);
        QWidget *widget = listWidget->itemWidget(item);
        bool disAble = forbidAllCheckBox->isChecked();
        reinterpret_cast<SAKOtherAutoResponseItem*>(widget)->setAllAutoResponseDisable(disAble);
    }
}

void SAKOtherAutoResponseItemManager::on_deleteItemPushButton_clicked()
{
    QListWidgetItem *item = listWidget->currentItem();
    if (!item){
        outputMessage(tr("未选择行，请选择要删除的行后重试"), false);
        return;
    }

    /// @brief 删除数据库记录
    QString tableName = SAKDataStruct::autoResponseTableName(debugPage->pageType());
    SAKDataStruct::SAKStructAutoResponseItem dataItem;
    SAKOtherAutoResponseItem *w = reinterpret_cast<SAKOtherAutoResponseItem*>(listWidget->itemWidget(item));
    dataItem.id = w->itemID();
//    databaseInterface->deleteAutoResponseItem(tableName, dataItem);

    listWidget->removeItemWidget(item);
    delete item;
}

void SAKOtherAutoResponseItemManager::on_addItemPushButton_clicked()
{
    /// @brief 限制数量
    if (listWidget->count() >= SAK_MAX_AUTO_RESPONSE_COUNT){
        outputMessage(tr("数据已达到上限，不能新建数据"), false);
        return;
    }

    QListWidgetItem *item = new QListWidgetItem(listWidget);
    listWidget->addItem(item);
    SAKOtherAutoResponseItem *itemWidget = new SAKOtherAutoResponseItem(debugPage, listWidget);
    item->setSizeHint(QSize(itemWidget->width(), itemWidget->height()));
    listWidget->setItemWidget(item, itemWidget);

    /// @brief 添加数据库记录
    SAKDataStruct::SAKStructAutoResponseItem dataItem;
    dataItem.id = itemWidget->itemID();
    dataItem.name = itemWidget->itemDescription();
    dataItem.enable = itemWidget->itemEnable();
    dataItem.responseData = itemWidget->itemResponseText();
    dataItem.referenceData = itemWidget->itemRefernceText();
    dataItem.responseFormat = itemWidget->itemResponseFormat();
    dataItem.referenceFormat = itemWidget->itemReferenceFormat();
    dataItem.option = itemWidget->itemOption();
    QString tableName = SAKDataStruct::autoResponseTableName(debugPage->pageType());
//    databaseInterface->insertAutoResponseItem(tableName, dataItem);
}

void SAKOtherAutoResponseItemManager::on_outportPushButton_clicked()
{
    /// @brief 从数据库中读入记录
    QString tableName = SAKDataStruct::autoResponseTableName(debugPage->pageType());
    QList<SAKDataStruct::SAKStructAutoResponseItem> itemList;// = databaseInterface->selectAutoResponseItem(tableName);
    if (itemList.isEmpty()){
        return;
    }

    QJsonArray jsonArray;
    AutoResponseItemKey itemKey;
    for (auto var : itemList){
        QJsonObject obj;
        obj.insert(itemKey.id, QVariant::fromValue(var.id).toJsonValue());
        obj.insert(itemKey.name, QVariant::fromValue(var.name).toJsonValue());
        obj.insert(itemKey.enable, QVariant::fromValue(var.enable).toJsonValue());
        obj.insert(itemKey.option, QVariant::fromValue(var.option).toJsonValue());
        obj.insert(itemKey.responseData, QVariant::fromValue(var.responseData).toJsonValue());
        obj.insert(itemKey.referenceData, QVariant::fromValue(var.referenceData).toJsonValue());
        obj.insert(itemKey.responseFormat, QVariant::fromValue(var.responseFormat).toJsonValue());
        obj.insert(itemKey.referenceFormat, QVariant::fromValue(var.referenceFormat).toJsonValue());
        jsonArray.append(QJsonValue(obj));
    }
    QJsonDocument jsonDoc;
    jsonDoc.setArray(jsonArray);

    /// @brief 打开文件，导出的数据将保存至该文件
    QString defaultName = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    defaultName.append(QString("/"));
    defaultName.append(QDateTime::currentDateTime().toString("yyyyMMddhhmmss"));
    defaultName.append(".json");
    QString fileName = QFileDialog::getSaveFileName(this, tr("导出数据"), defaultName, QString("json (*.json)"));
    if (fileName.isEmpty()){
        return;
    }

    /// @brief 保存至文件
    QFile file(fileName);
    if (file.open(QFile::ReadWrite)){
        file.write(jsonDoc.toJson());
        file.close();
    }else{
        outputMessage(file.errorString(), false);
    }
}

void SAKOtherAutoResponseItemManager::on_importPushButton_clicked()
{
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    QString fileName = QFileDialog::getOpenFileName(this, tr("导出数据"), defaultPath, QString("json (*.json)"));
    QFile file(fileName);
    if (file.open(QFile::ReadWrite)){
        QByteArray array = file.readAll();
        file.close();

        QJsonDocument jsc = QJsonDocument::fromJson(array);
        if (!jsc.isArray()){
            outputMessage(QString("QJsonDocument is not json array"), false);
            return;
        }

        QJsonArray jsa = jsc.array();
        for (int i = 0; i < jsa.count(); i++){
            if (jsa.at(i).isObject()){
                QJsonObject jso = jsa.at(i).toObject();
                AutoResponseItemKey itemKey;
                SAKDataStruct::SAKStructAutoResponseItem responseItem;
                responseItem.id = jso.value(itemKey.id).toVariant().toULongLong();
                responseItem.name = jso.value(itemKey.name).toVariant().toString();
                responseItem.enable = jso.value(itemKey.enable).toVariant().toBool();
                responseItem.option = jso.value(itemKey.option).toVariant().toUInt();
                responseItem.responseData = jso.value(itemKey.responseData).toVariant().toString();
                responseItem.referenceData = jso.value(itemKey.referenceData).toVariant().toString();
                responseItem.responseFormat = jso.value(itemKey.responseFormat).toVariant().toUInt();
                responseItem.referenceFormat = jso.value(itemKey.referenceFormat).toVariant().toUInt();

                /// @brief 不存在则新建
                if (!contains(responseItem.id)){
                    innerCreateItem(responseItem, debugPage, listWidget);

                    QString tableName = SAKDataStruct::autoResponseTableName(debugPage->pageType());
//                    databaseInterface->insertAutoResponseItem(tableName, responseItem);
                }
            }
        }
    }else{
        outputMessage(file.errorString(), false);
    }
}
