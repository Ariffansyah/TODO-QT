#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDebug>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <algorithm>

int findTaskIndexById(const QVector<Task>& tasks, int id) {
    for (int i = 0; i < tasks.size(); ++i) {
        if (tasks[i].id == id)
            return i;
    }
    return -1;
}

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
        QMessageBox::critical(this, "Database Error", "Unable to open the database.");
        return;
    }
    QSqlQuery query(db);
    query.exec(
        "CREATE TABLE IF NOT EXISTS tasks ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "title TEXT NOT NULL,"
        "description TEXT,"
        "due_date TEXT,"
        "sub_tasks TEXT,"
        "priority INTEGER DEFAULT 0,"
        "completed INTEGER DEFAULT 0,"
        "status TEXT DEFAULT 'pending',"
        "UNIQUE(id)"
        ")"
        );
    db.close();
}

void MainWindow::refreshAllTasksFromDb()
{
    allTasks.clear();
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) db.open();
    QSqlQuery query("SELECT * FROM tasks", db);
    while(query.next()) {
        Task t;
        t.id = query.value("id").toInt();
        t.title = query.value("title").toString();
        t.description = query.value("description").toString();
        t.dueDate = query.value("due_date").toString();
        t.subTasks = query.value("sub_tasks").toString();
        t.priority = query.value("priority").toInt();
        t.status = query.value("status").toString();
        allTasks.push_back(t);
    }
}

void MainWindow::pushTaskToUndoStack(int id)
{
    int idx = findTaskIndexById(allTasks, id);
    if(idx != -1) {
        undoStack.push_back({allTasks[idx], TaskActionType::Update});
    }
    redoStack.clear();
}

void MainWindow::pushDeletedTaskToUndoStack(int id)
{
    int idx = findTaskIndexById(allTasks, id);
    if(idx != -1) {
        undoStack.push_back({allTasks[idx], TaskActionType::Delete});
    }
    redoStack.clear();
}

void MainWindow::pushTaskToRedoStack(int id, TaskActionType type)
{
    int idx = findTaskIndexById(allTasks, id);
    if(idx != -1) {
        redoStack.push_back({allTasks[idx], type});
    }
}

void MainWindow::displayTasks()
{
    refreshAllTasksFromDb();
    ui->PendingList->clear();
    ui->InProgressList->clear();
    ui->CompleteList->clear();
    for (const Task& t : allTasks) {
        QString item = "(" + QString::number(t.id) + ") " + t.title + " - Due: " + t.dueDate;
        if (t.status == "pending") {
            ui->PendingList->addItem(item);
        } else if (t.status == "in progress") {
            ui->InProgressList->addItem(item);
        } else if (t.status == "complete") {
            ui->CompleteList->addItem(item);
        }
    }
}

void MainWindow::displayTasksByDeadline()
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) db.open();

    allTasks.clear();
    QSqlQuery query("SELECT * FROM tasks ORDER BY due_date", db);
    QQueue<Task> queue;

    while(query.next()) {
        Task t;
        t.id = query.value("id").toInt();
        t.title = query.value("title").toString();
        t.description = query.value("description").toString();
        t.dueDate = query.value("due_date").toString();
        t.subTasks = query.value("sub_tasks").toString();
        t.priority = query.value("priority").toInt();
        t.status = query.value("status").toString();
        queue.enqueue(t);
    }

    ui->PendingList->clear();
    ui->InProgressList->clear();
    ui->CompleteList->clear();

    while (!queue.isEmpty()) {
        Task t = queue.dequeue();
        QString item = "(" + QString::number(t.id) + ") " + t.title + " - Due: " + t.dueDate;
        if (t.status == "pending") {
            ui->PendingList->addItem(item);
        } else if (t.status == "in progress") {
            ui->InProgressList->addItem(item);
        } else if (t.status == "complete") {
            ui->CompleteList->addItem(item);
        }
    }
}
void MainWindow::displayTasksByPriority()
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) db.open();
    allTasks.clear();
    QSqlQuery query("SELECT * FROM tasks ORDER BY priority DESC", db);
    while(query.next()) {
        Task t;
        t.id = query.value("id").toInt();
        t.title = query.value("title").toString();
        t.description = query.value("description").toString();
        t.dueDate = query.value("due_date").toString();
        t.subTasks = query.value("sub_tasks").toString();
        t.priority = query.value("priority").toInt();
        t.status = query.value("status").toString();
        allTasks.push_back(t);
    }
    ui->PendingList->clear();
    ui->InProgressList->clear();
    ui->CompleteList->clear();
    for (const Task& t : allTasks) {
        QString item = "(" + QString::number(t.id) + ") " + t.title + " - Due: " + t.dueDate;
        if (t.status == "pending") {
            ui->PendingList->addItem(item);
        } else if (t.status == "in progress") {
            ui->InProgressList->addItem(item);
        } else if (t.status == "complete") {
            ui->CompleteList->addItem(item);
        }
    }
}

void MainWindow::on_StartButton_clicked()
{
    createDatabase();
    displayTasks();
    ui->stackedWidget->setCurrentWidget(ui->page_2);
}

void MainWindow::on_AddButton_clicked()
{
    QString taskTitle = ui->TaskLineEdit->text();
    QString description = ui->DescriptionLineEdit->text();
    QString dueDate = ui->DueDateLineEdit->text();
    QString subTasks = ui->SubTaskLineEdit->text();
    int priority = ui->PriorityLineEdit->text().toInt();
    if (taskTitle.isEmpty() || description.isEmpty() || dueDate.isEmpty() || priority < 0 || priority > 5) {
        QMessageBox::warning(this, "Input Error", "Please fill in all fields correctly.\nPriority must be between 0 and 5.");
        return;
    }
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) db.open();
    QSqlQuery query(db);
    query.prepare("INSERT INTO tasks (title, description, due_date, sub_tasks, priority) VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(taskTitle);
    query.addBindValue(description);
    query.addBindValue(dueDate);
    query.addBindValue(subTasks);
    query.addBindValue(priority);
    query.exec();
    db.close();
    refreshAllTasksFromDb();
    displayTasks();
    ui->TaskLineEdit->clear();
    ui->DescriptionLineEdit->clear();
    ui->DueDateLineEdit->clear();
    ui->SubTaskLineEdit->clear();
    ui->PriorityLineEdit->clear();
}

void MainWindow::on_ReloadButton_clicked()
{
    displayTasks();
}

void MainWindow::on_SortByDeadlineButton_clicked()
{
    displayTasksByDeadline();
}

void MainWindow::on_SortByPriorityButton_clicked()
{
    displayTasksByPriority();
}

void MainWindow::on_PendingList_doubleClicked(const QModelIndex &index)
{
    QListWidgetItem* item = ui->PendingList->item(index.row());
    if (!item) return;
    QString taskDetails = item->text();
    QStringList details = taskDetails.split(" - Due: ");
    if (details.size() < 2) {
        QMessageBox::warning(this, "Selection Error", "Please select a valid task.");
        return;
    }
    QString leftPart = details[0];
    QString dueDate = details[1];
    QRegularExpression re("^\\((\\d+)\\)\\s*(.+)$");
    QRegularExpressionMatch match = re.match(leftPart);
    if (!match.hasMatch()) {
        QMessageBox::warning(this, "Parsing Error", "Could not extract task ID and title.");
        return;
    }
    QString id = match.captured(1);
    QString taskTitle = match.captured(2);
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) db.open();
    QSqlQuery selectQuery(db);
    selectQuery.prepare("SELECT * FROM tasks WHERE id = ?");
    selectQuery.addBindValue(id.trimmed());
    selectQuery.exec();
    if (!selectQuery.next()) {
        QMessageBox::warning(this, "Task Not Found", "The selected task could not be found in the database.");
        return;
    }
    QString message = QString("Task: %1\nDue Date: %2\n\nWhat would you like to do with this task?").arg(taskTitle, dueDate);
    QMessageBox msgBox(this);
    msgBox.setText(message);
    msgBox.setWindowTitle("Task Options");
    QPushButton* Btn1 = msgBox.addButton("Set to In Progress", QMessageBox::ActionRole);
    QPushButton* Btn2 = msgBox.addButton("Set to Complete", QMessageBox::ActionRole);
    QPushButton* DeleteBtn = msgBox.addButton("Delete", QMessageBox::ActionRole);
    QPushButton* cancelBtn = msgBox.addButton("Cancel", QMessageBox::RejectRole);
    msgBox.exec();
    QString newStatus;
    if (msgBox.clickedButton() == Btn1) {
        pushTaskToUndoStack(id.toInt());
        newStatus = "in progress";
    } else if (msgBox.clickedButton() == Btn2) {
        pushTaskToUndoStack(id.toInt());
        newStatus = "complete";
    } else if (msgBox.clickedButton() == DeleteBtn) {
        pushDeletedTaskToUndoStack(id.toInt());
        QSqlQuery deleteQuery(db);
        deleteQuery.prepare("DELETE FROM tasks WHERE id = ?");
        deleteQuery.addBindValue(id.trimmed());
        deleteQuery.exec();
        refreshAllTasksFromDb();
        displayTasks();
        QMessageBox::information(this, "Task Deleted", QString("The task '%1' has been deleted.").arg(taskTitle));
        return;
    } else {
        return;
    }
    QSqlQuery updateQuery(db);
    updateQuery.prepare("UPDATE tasks SET status = ? WHERE id = ?");
    updateQuery.addBindValue(newStatus);
    updateQuery.addBindValue(id.trimmed());
    updateQuery.exec();
    refreshAllTasksFromDb();
    displayTasks();
    QMessageBox::information(this, "Task Updated", QString("The task '%1' has been updated to '%2'.").arg(taskTitle, newStatus));
}

void MainWindow::on_InProgressList_doubleClicked(const QModelIndex &index)
{
    QListWidgetItem* item = ui->InProgressList->item(index.row());
    if (!item) return;
    QString taskDetails = item->text();
    QStringList details = taskDetails.split(" - Due: ");
    if (details.size() < 2) {
        QMessageBox::warning(this, "Selection Error", "Please select a valid task.");
        return;
    }
    QString leftPart = details[0];
    QString dueDate = details[1];
    QRegularExpression re("^\\((\\d+)\\)\\s*(.+)$");
    QRegularExpressionMatch match = re.match(leftPart);
    if (!match.hasMatch()) {
        QMessageBox::warning(this, "Parsing Error", "Could not extract task ID and title.");
        return;
    }
    QString id = match.captured(1);
    QString taskTitle = match.captured(2);
    QString message = QString("Task: %1\nDue Date: %2\n\nWhat would you like to do with this task?").arg(taskTitle, dueDate);
    QMessageBox msgBox(this);
    msgBox.setText(message);
    msgBox.setWindowTitle("Task Options");
    QPushButton* Btn1 = msgBox.addButton("Set to Pending", QMessageBox::ActionRole);
    QPushButton* Btn2 = msgBox.addButton("Set to Complete", QMessageBox::ActionRole);
    QPushButton* DeleteBtn = msgBox.addButton("Delete", QMessageBox::ActionRole);
    QPushButton* cancelBtn = msgBox.addButton("Cancel", QMessageBox::RejectRole);
    msgBox.exec();
    QString newStatus;
    if (msgBox.clickedButton() == Btn1) {
        pushTaskToUndoStack(id.toInt());
        newStatus = "pending";
    } else if (msgBox.clickedButton() == Btn2) {
        pushTaskToUndoStack(id.toInt());
        newStatus = "complete";
    } else if(msgBox.clickedButton() == DeleteBtn) {
        pushDeletedTaskToUndoStack(id.toInt());
        QSqlDatabase db = QSqlDatabase::database();
        if (!db.isOpen()) db.open();
        QSqlQuery query(db);
        query.prepare("DELETE FROM tasks WHERE id = ?");
        query.addBindValue(id.trimmed());
        query.exec();
        refreshAllTasksFromDb();
        displayTasks();
        QMessageBox::information(this, "Task Deleted", QString("The task '%1' has been deleted.").arg(taskTitle));
        return;
    } else {
        return;
    }
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) db.open();
    QSqlQuery query(db);
    query.prepare("UPDATE tasks SET status = ? WHERE id = ?");
    query.addBindValue(newStatus);
    query.addBindValue(id.trimmed());
    query.exec();
    refreshAllTasksFromDb();
    displayTasks();
    QMessageBox::information(this, "Task Updated", QString("The task '%1' has been updated to '%2'.").arg(taskTitle, newStatus));
}

void MainWindow::on_CompleteList_doubleClicked(const QModelIndex &index)
{
    QListWidgetItem* item = ui->CompleteList->item(index.row());
    if (!item) return;
    QString taskDetails = item->text();
    QStringList details = taskDetails.split(" - Due: ");
    if (details.size() < 2) {
        QMessageBox::warning(this, "Selection Error", "Please select a valid task.");
        return;
    }
    QString leftPart = details[0];
    QString dueDate = details[1];
    QRegularExpression re("^\\((\\d+)\\)\\s*(.+)$");
    QRegularExpressionMatch match = re.match(leftPart);
    if (!match.hasMatch()) {
        QMessageBox::warning(this, "Parsing Error", "Could not extract task ID and title.");
        return;
    }
    QString id = match.captured(1);
    QString taskTitle = match.captured(2);
    QString message = QString("Task: %1\nDue Date: %2\n\nWhat would you like to do with this task?").arg(taskTitle, dueDate);
    QMessageBox msgBox(this);
    msgBox.setText(message);
    msgBox.setWindowTitle("Task Options");
    QPushButton* Btn1 = msgBox.addButton("Set to Pending", QMessageBox::ActionRole);
    QPushButton* Btn2 = msgBox.addButton("Set to In Progress", QMessageBox::ActionRole);
    QPushButton* DeleteBtn = msgBox.addButton("Delete", QMessageBox::ActionRole);
    QPushButton* cancelBtn = msgBox.addButton("Cancel", QMessageBox::RejectRole);
    msgBox.exec();
    QString newStatus;
    if (msgBox.clickedButton() == Btn1) {
        pushTaskToUndoStack(id.toInt());
        newStatus = "pending";
    } else if (msgBox.clickedButton() == Btn2) {
        pushTaskToUndoStack(id.toInt());
        newStatus = "in progress";
    } else if(msgBox.clickedButton() == DeleteBtn) {
        pushDeletedTaskToUndoStack(id.toInt());
        QSqlDatabase db = QSqlDatabase::database();
        if (!db.isOpen()) db.open();
        QSqlQuery query(db);
        query.prepare("DELETE FROM tasks WHERE id = ?");
        query.addBindValue(id.trimmed());
        query.exec();
        refreshAllTasksFromDb();
        displayTasks();
        QMessageBox::information(this, "Task Deleted", QString("The task '%1' has been deleted.").arg(taskTitle));
        return;
    } else {
        return;
    }
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) db.open();
    QSqlQuery query(db);
    query.prepare("UPDATE tasks SET status = ? WHERE id = ?");
    query.addBindValue(newStatus);
    query.addBindValue(id.trimmed());
    query.exec();
    refreshAllTasksFromDb();
    displayTasks();
    QMessageBox::information(this, "Task Updated", QString("The task '%1' has been updated to '%2'.").arg(taskTitle, newStatus));
}

void MainWindow::on_UndoButton_clicked()
{
    if (undoStack.isEmpty()) {
        QMessageBox::information(this, "Undo", "Nothing to undo.");
        return;
    }
    TaskAction last = undoStack.takeLast();
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) db.open();
    if (last.type == TaskActionType::Delete) {
        QSqlQuery q(db);
        q.prepare("INSERT INTO tasks (id, title, description, due_date, sub_tasks, priority, status) VALUES (?, ?, ?, ?, ?, ?, ?)");
        q.addBindValue(last.task.id);
        q.addBindValue(last.task.title);
        q.addBindValue(last.task.description);
        q.addBindValue(last.task.dueDate);
        q.addBindValue(last.task.subTasks);
        q.addBindValue(last.task.priority);
        q.addBindValue(last.task.status);
        q.exec();
        redoStack.push_back({last.task, TaskActionType::Delete});
    } else if (last.type == TaskActionType::Update) {
        int idx = findTaskIndexById(allTasks, last.task.id);
        if (idx != -1) {
            QSqlQuery q(db);
            q.prepare("UPDATE tasks SET title=?, description=?, due_date=?, sub_tasks=?, priority=?, status=? WHERE id=?");
            q.addBindValue(last.task.title);
            q.addBindValue(last.task.description);
            q.addBindValue(last.task.dueDate);
            q.addBindValue(last.task.subTasks);
            q.addBindValue(last.task.priority);
            q.addBindValue(last.task.status);
            q.addBindValue(last.task.id);
            q.exec();
            pushTaskToRedoStack(last.task.id, TaskActionType::Update);
        }
    }
    refreshAllTasksFromDb();
    displayTasks();
    QMessageBox::information(this, "Undo", "Undo performed.");
}

void MainWindow::on_RedoButton_clicked()
{
    if (redoStack.isEmpty()) {
        QMessageBox::information(this, "Redo", "Nothing to redo.");
        return;
    }
    TaskAction redoAction = redoStack.takeLast();
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) db.open();
    if (redoAction.type == TaskActionType::Delete) {
        QSqlQuery q(db);
        q.prepare("DELETE FROM tasks WHERE id=?");
        q.addBindValue(redoAction.task.id);
        q.exec();
        undoStack.push_back({redoAction.task, TaskActionType::Delete});
    } else if (redoAction.type == TaskActionType::Update) {
        int idx = findTaskIndexById(allTasks, redoAction.task.id);
        if (idx != -1) {
            QSqlQuery q(db);
            q.prepare("UPDATE tasks SET title=?, description=?, due_date=?, sub_tasks=?, priority=?, status=? WHERE id=?");
            q.addBindValue(redoAction.task.title);
            q.addBindValue(redoAction.task.description);
            q.addBindValue(redoAction.task.dueDate);
            q.addBindValue(redoAction.task.subTasks);
            q.addBindValue(redoAction.task.priority);
            q.addBindValue(redoAction.task.status);
            q.addBindValue(redoAction.task.id);
            q.exec();
            pushTaskToUndoStack(redoAction.task.id);
        }
    }
    refreshAllTasksFromDb();
    displayTasks();
    QMessageBox::information(this, "Redo", "Redo performed.");
}
