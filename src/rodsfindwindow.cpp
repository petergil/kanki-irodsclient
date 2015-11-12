/**
 * @file rodsfindwindow.cpp
 * @brief Implementation of class RodsFindWindow
 *
 * The RodsFindWindow class extends the Qt window class QMainWindow and
 * implements an iRODS find window UI.
 *
 * Copyright (C) 2014-2015 University of Jyväskylä. All rights reserved.
 * License: The BSD 3-Clause License, see LICENSE file for details.
 *
 * @author Ilari Korhonen
 */

// application class RodsFindWindow header
#include "rodsfindwindow.h"

// generated UI class Ui::RodsFindWindow header
#include "ui_rodsfindwindow.h"

RodsFindWindow::RodsFindWindow(Kanki::RodsConnection *rodsConn, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::RodsFindWindow)
{
    this->conn = rodsConn;

    this->ui->setupUi(this);
    this->ui->criteriaLayout->setAlignment(Qt::AlignTop);

    // setup combo box for condition selection
    this->ui->condSel->addItem("Data Object Name", RodsFindWindow::DataObjName);
    this->ui->condSel->addItem("Data Object Created", RodsFindWindow::DataObjCreated);
    this->ui->condSel->addItem("Data Object Modified", RodsFindWindow::DataObjModified);
    this->ui->condSel->addItem("Data Object Checksum", RodsFindWindow::DataObjChksum);
    this->ui->condSel->addItem("Collection Name (Path)", RodsFindWindow::CollName);
    this->ui->condSel->addItem("Collection Created", RodsFindWindow::CollCreated);
    this->ui->condSel->addItem("Collection Modified", RodsFindWindow::CollModified);

    // connect ui event signals to handler slots
    connect(this->ui->condAdd, &QPushButton::clicked, this, &RodsFindWindow::addCondition);
    connect(this->ui->resetButton, &QPushButton::clicked, this, &RodsFindWindow::reset);
    connect(this->ui->executeButton, &QPushButton::clicked, this, &RodsFindWindow::executeSearch);
}

RodsFindWindow::~RodsFindWindow()
{
    this->removeCondWidgets();
    delete (this->ui);
}

void RodsFindWindow::closeEvent(QCloseEvent *event)
{
    (void)event;

    // signal out unregistering
    this->unregister();
}

void RodsFindWindow::addCondition()
{
    RodsConditionWidget *widget = NULL;
    int cond = this->ui->condSel->currentData().toInt();
    QString label = this->ui->condSel->currentText();

    // depending on condition, instantiate appropriate widget
    switch (cond)
    {
        case RodsFindWindow::DataObjName:
            widget = new RodsStringConditionWidget(COL_DATA_NAME, label);
        break;

        case RodsFindWindow::DataObjCreated:
            widget = new RodsDateConditionWidget(COL_D_CREATE_TIME, label);
        break;

        case RodsFindWindow::DataObjModified:
            widget = new RodsDateConditionWidget(COL_D_MODIFY_TIME, label);
        break;

        case RodsFindWindow::DataObjChksum:
            widget = new RodsStringConditionWidget(COL_D_DATA_CHECKSUM, label);
        break;

        case RodsFindWindow::CollName:
            widget = new RodsStringConditionWidget(COL_COLL_NAME, label);
        break;

        case RodsFindWindow::CollCreated:
            widget = new RodsDateConditionWidget(COL_COLL_CREATE_TIME, label);
        break;

        case RodsFindWindow::CollModified:
            widget = new RodsDateConditionWidget(COL_COLL_MODIFY_TIME, label);
        break;

        default:
        break;
    }

    // if we have a widget, add it
    if (widget)
    {
        this->ui->criteriaLayout->addWidget(widget);
        this->condWidgets.push_back(widget);
    }
}

void RodsFindWindow::executeSearch()
{
    int status = 0;
    Kanki::RodsGenQuery query(this->conn);
    query.addQueryAttribute(COL_DATA_NAME);
    query.addQueryAttribute(COL_COLL_NAME);

    // evaluate genquery conditions from the condition widgets
    for (std::vector<RodsConditionWidget*>::iterator i = this->condWidgets.begin();
         i != this->condWidgets.end(); i++)
    {
        RodsConditionWidget *widget = *i;
        widget->evaluateConds(&query);
    }

    if (this->conn->isReady())
    {
        // execute a timed query
        std::chrono::high_resolution_clock::time_point t0 = std::chrono::high_resolution_clock::now();
        status = query.execute();
        std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();

        // report errors
        if (status < 0)
        {

        }

        // when successful, report results
        else {
            this->ui->treeWidget->clear();

            std::vector<std::string> names = query.getResultSetForAttr(COL_DATA_NAME);
            std::vector<std::string> colls = query.getResultSetForAttr(COL_COLL_NAME);
            std::chrono::milliseconds diff = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0);

            QString statusMsg = "Search query successful: " + QVariant((int)names.size()).toString();
            statusMsg += " results (execution time " + QVariant(((double)diff.count() / (double)1000)).toString() + " sec).";
            this->statusBar()->showMessage(statusMsg);

            for (unsigned int i = 0; i < names.size() && i < colls.size(); i++)
            {
                QTreeWidgetItem *item = new QTreeWidgetItem();
                std::string path = colls.at(i) + '/' + names.at(i);

                item->setText(0, path.c_str());
                this->ui->treeWidget->addTopLevelItem(item);
            }
        }
    }
}

void RodsFindWindow::reset()
{
    this->removeCondWidgets();
}

void RodsFindWindow::removeCondWidgets()
{
    // delete dynamically created widgets
    for (std::vector<RodsConditionWidget*>::iterator i = this->condWidgets.begin();
         i != this->condWidgets.end(); i++)
    {
        RodsConditionWidget *ptr = *i;

        this->ui->criteriaLayout->removeWidget(ptr);
        delete (ptr);
    }

    this->condWidgets.clear();
}
