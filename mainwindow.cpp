#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentWidget(ui->page);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("./todo.db");

    if (!db.open()) {
        qDebug() << "Error: Unable to open database.";
        QMessageBox::critical(this, "Database Error", "Unable to open the database.");
        return;
    }

    QSqlQuery query(db);

    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS tasks ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "title TEXT NOT NULL,"
            "description TEXT,"
            "due_date TEXT,"
            "priority INTEGER DEFAULT 0,"
            "completed INTEGER DEFAULT 0,"
            "parent_id INTEGER,"
            "status TEXT DEFAULT 'pending',"
            "UNIQUE(id)"
            ")"
            )) {
        qDebug() << "Error creating tasks table:" << query.lastError().text();
        QMessageBox::critical(this, "Database Error", "Unable to create the tasks table.");
        db.close();
        return;
    }

    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS dependencies ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "task_id INTEGER NOT NULL,"
            "depends_on_id INTEGER NOT NULL,"
            "FOREIGN KEY(task_id) REFERENCES tasks(id),"
            "FOREIGN KEY(depends_on_id) REFERENCES tasks(id)"
            ")"
            )) {
        qDebug() << "Error creating dependencies table:" << query.lastError().text();
        QMessageBox::critical(this, "Database Error", "Unable to create the dependencies table.");
        db.close();
        return;
    }

    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS undo_stack ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "action TEXT NOT NULL,"
            "task_id INTEGER,"
            "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
            ")"
            )) {
        qDebug() << "Error creating undo_stack table:" << query.lastError().text();
        QMessageBox::critical(this, "Database Error", "Unable to create the undo_stack table.");
        db.close();
        return;
    }

    if (!query.exec(
            "CREATE TABLE IF NOT EXISTS redo_stack ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "action TEXT NOT NULL,"
            "task_id INTEGER,"
            "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP"
            ")"
            )) {
        qDebug() << "Error creating redo_stack table:" << query.lastError().text();
        QMessageBox::critical(this, "Database Error", "Unable to create the redo_stack table.");
        db.close();
        return;
    }

    db.close();
}

void MainWindow::displayTasks()
{
    QSqlDatabase db = QSqlDatabase::database();
    db.setDatabaseName("./todo.db");
    if (!db.isOpen()) {
        qDebug() << "Error: Database is not open.";
        return;
    }

    QSqlQuery query(db);
    if (!query.exec("SELECT * FROM tasks")) {
        qDebug() << "Error executing query:" << query.lastError().text();
        QMessageBox::warning(this, "Database Error", "Unable to retrieve tasks from the database.");
        return;
    }

    ui->PendingList->clear();
    ui->InProgressList->clear();
    ui->CompleteList->clear();

    while (query.next()) {
        QString task = query.value("title").toString();
        QString status = query.value("status").toString();
        QString dueDate = query.value("due_date").toString();

        if (status == "pending") {
            ui->PendingList->addItem(task + " - Due: " + dueDate);
        } else if (status == "in progress") {
            ui->InProgressList->addItem(task + " - Due: " + dueDate);
        } else if (status == "complete") {
            ui->CompleteList->addItem(task + " - Due: " + dueDate);
        } else {
            qDebug() << "Unknown status:" << status;
        }
    }
    db.close();
}

void MainWindow::on_StartButton_clicked()
{
    createDatabase();
    displayTasks();
    ui->stackedWidget->setCurrentWidget(ui->page_2);
}

