#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "taskdialog.h"
#include <QDebug>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QRegularExpression>
#include <QFileDialog>

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

    updateRecommendations();
}

void MainWindow::displayTasksByDeadline()
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) db.open();

    allTasks.clear();
    QSqlQuery query("SELECT * FROM tasks ORDER BY due_date", db);

    TaskQueue queue;

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

void MainWindow::displayNotifications()
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) db.open();

    QSqlQuery query(db);
    query.prepare("SELECT * FROM tasks WHERE status IN ('pending', 'in progress')");
    query.exec();

    ui->NotificationlistWidget->clear();

    TaskNode* head = nullptr;
    TaskNode* tail = nullptr;
    QHash<int, TaskNode*> taskTable;

    QDate today = QDate::currentDate();
    QDate tomorrow = today.addDays(1);

    while (query.next()) {
        Task t;
        t.id = query.value("id").toInt();
        t.title = query.value("title").toString();
        t.description = query.value("description").toString();
        t.dueDate = query.value("due_date").toString();
        t.subTasks = query.value("sub_tasks").toString();
        t.priority = query.value("priority").toInt();
        t.status = query.value("status").toString();

        QDate taskDate = QDate::fromString(t.dueDate, "yyyy-MM-dd");
        QString dueText;

        if (taskDate < today) {
            dueText = "Overdue!";
        } else if (taskDate == today) {
            dueText = "Due Today! Stay focused.";
        } else if (taskDate == tomorrow) {
            dueText = "Due Tomorrow! Be prepared.";
        } else {
            continue;
        }

        TaskNode* newNode = new TaskNode(t);
        if (!head) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            tail = newNode;
        }
        taskTable.insert(t.id, newNode);

        QString itemText = QString("(%1) %2 - %3")
                               .arg(t.id)
                               .arg(t.title)
                               .arg(dueText);
        ui->NotificationlistWidget->addItem(itemText);
    }

    db.close();
    ui->stackedWidget->setCurrentWidget(ui->page_4);

    TaskNode* current = head;
    while (current) {
        TaskNode* temp = current;
        current = current->next;
        delete temp;
    }
}

void MainWindow::buildTaskDependencyGraph() {
    dependencyGraph.clear();
    QMap<QString, int> titleToId;
    for (const Task& t : allTasks)
        titleToId[t.title.toLower()] = t.id;

    for (const Task& t : allTasks) {
        QSet<int> deps;
        QString desc = t.description.toLower();
        for (const QString& otherTitle : titleToId.keys()) {
            if (t.id == titleToId[otherTitle])
                continue;
            if (desc.contains(otherTitle) || t.title.toLower().contains(otherTitle)) {
                deps.insert(titleToId[otherTitle]);
            }
        }
        dependencyGraph[t.id] = deps;
    }
}

QVector<Task> MainWindow::getGraphRecommendedTasks(int maxRecs) {
    QSet<int> completed;
    for (const Task& t : allTasks) {
        if (t.status == "complete")
            completed.insert(t.id);
    }
    QVector<Task> candidates;
    for (const Task& t : allTasks) {
        if (t.status == "complete") continue;
        bool allDepsDone = true;
        for (int dep : dependencyGraph.value(t.id)) {
            if (!completed.contains(dep)) {
                allDepsDone = false;
                break;
            }
        }
        if (allDepsDone)
            candidates.push_back(t);
    }
    std::sort(candidates.begin(), candidates.end(), [](const Task& a, const Task& b) {
        if (a.priority != b.priority)
            return a.priority > b.priority;
        QDate da = QDate::fromString(a.dueDate, "yyyy-MM-dd");
        QDate db = QDate::fromString(b.dueDate, "yyyy-MM-dd");
        if (da.isValid() && db.isValid())
            return da < db;
        if (da.isValid()) return true;
        if (db.isValid()) return false;
        return a.id < b.id;
    });
    if (candidates.size() > maxRecs)
        candidates.resize(maxRecs);
    return candidates;
}

void MainWindow::updateRecommendations() {
    QVector<Task> recs = getGraphRecommendedTasks(5);
    ui->taskRecommendationListWidget->clear();
    for (const Task& t : recs) {
        QString rec = QString("%1 (Priority: %2, Due: %3)").arg(t.title).arg(t.priority).arg(t.dueDate);
        ui->taskRecommendationListWidget->addItem(rec);
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

Task getTaskById(int id) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) db.open();
    QSqlQuery q(db);
    q.prepare("SELECT * FROM tasks WHERE id = ?");
    q.addBindValue(id);
    q.exec();
    if (q.next()) {
        Task t;
        t.id = q.value("id").toInt();
        t.title = q.value("title").toString();
        t.description = q.value("description").toString();
        t.dueDate = q.value("due_date").toString();
        t.subTasks = q.value("sub_tasks").toString();
        t.priority = q.value("priority").toInt();
        t.status = q.value("status").toString();
        return t;
    }
    return Task{};
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
    QRegularExpression re("^\\((\\d+)\\)\\s*(.+)$");
    QRegularExpressionMatch match = re.match(leftPart);
    if (!match.hasMatch()) {
        QMessageBox::warning(this, "Parsing Error", "Could not extract task ID and title.");
        return;
    }
    int id = match.captured(1).toInt();
    Task t = getTaskById(id);
    QStringList subTasksArray = t.subTasks.split(",", Qt::SkipEmptyParts);
    TaskDialog dlg(t.title, t.description, t.dueDate, t.priority, subTasksArray, t.status, this);
    dlg.setWindowTitle("Task Options");
    dlg.exec();
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) db.open();
    QString newStatus;
    switch (dlg.result()) {
    case TaskActionDialogResult::SetToInProgress:
        pushTaskToUndoStack(id);
        newStatus = "in progress";
        break;
    case TaskActionDialogResult::SetToComplete:
        pushTaskToUndoStack(id);
        newStatus = "complete";
        break;
    case TaskActionDialogResult::Delete: {
        pushDeletedTaskToUndoStack(id);
        QSqlQuery deleteQuery(db);
        deleteQuery.prepare("DELETE FROM tasks WHERE id = ?");
        deleteQuery.addBindValue(id);
        deleteQuery.exec();
        refreshAllTasksFromDb();
        displayTasks();
        QMessageBox::information(this, "Task Deleted", QString("The task '%1' has been deleted.").arg(t.title));
        return;
    }
    default:
        return;
    }
    QSqlQuery updateQuery(db);
    updateQuery.prepare("UPDATE tasks SET status = ? WHERE id = ?");
    updateQuery.addBindValue(newStatus);
    updateQuery.addBindValue(id);
    updateQuery.exec();
    refreshAllTasksFromDb();
    displayTasks();
    QMessageBox::information(this, "Task Updated", QString("The task '%1' has been updated to '%2'.").arg(t.title, newStatus));
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
    QRegularExpression re("^\\((\\d+)\\)\\s*(.+)$");
    QRegularExpressionMatch match = re.match(leftPart);
    if (!match.hasMatch()) {
        QMessageBox::warning(this, "Parsing Error", "Could not extract task ID and title.");
        return;
    }
    int id = match.captured(1).toInt();
    Task t = getTaskById(id);
    QStringList subTasksArray = t.subTasks.split(",", Qt::SkipEmptyParts);
    TaskDialog dlg(t.title, t.description, t.dueDate, t.priority, subTasksArray, t.status, this);
    dlg.setWindowTitle("Task Options");
    dlg.exec();
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) db.open();
    QString newStatus;
    switch (dlg.result()) {
    case TaskActionDialogResult::SetToPending:
        pushTaskToUndoStack(id);
        newStatus = "pending";
        break;
    case TaskActionDialogResult::SetToComplete:
        pushTaskToUndoStack(id);
        newStatus = "complete";
        break;
    case TaskActionDialogResult::Delete: {
        pushDeletedTaskToUndoStack(id);
        QSqlQuery deleteQuery(db);
        deleteQuery.prepare("DELETE FROM tasks WHERE id = ?");
        deleteQuery.addBindValue(id);
        deleteQuery.exec();
        refreshAllTasksFromDb();
        displayTasks();
        QMessageBox::information(this, "Task Deleted", QString("The task '%1' has been deleted.").arg(t.title));
        return;
    }
    default:
        return;
    }
    QSqlQuery updateQuery(db);
    updateQuery.prepare("UPDATE tasks SET status = ? WHERE id = ?");
    updateQuery.addBindValue(newStatus);
    updateQuery.addBindValue(id);
    updateQuery.exec();
    refreshAllTasksFromDb();
    displayTasks();
    QMessageBox::information(this, "Task Updated", QString("The task '%1' has been updated to '%2'.").arg(t.title, newStatus));
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
    QRegularExpression re("^\\((\\d+)\\)\\s*(.+)$");
    QRegularExpressionMatch match = re.match(leftPart);
    if (!match.hasMatch()) {
        QMessageBox::warning(this, "Parsing Error", "Could not extract task ID and title.");
        return;
    }
    int id = match.captured(1).toInt();
    Task t = getTaskById(id);
    QStringList subTasksArray = t.subTasks.split(",", Qt::SkipEmptyParts);
    TaskDialog dlg(t.title, t.description, t.dueDate, t.priority, subTasksArray, t.status, this);
    dlg.setWindowTitle("Task Options");
    dlg.exec();
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) db.open();
    QString newStatus;
    switch (dlg.result()) {
    case TaskActionDialogResult::SetToPending:
        pushTaskToUndoStack(id);
        newStatus = "pending";
        break;
    case TaskActionDialogResult::SetToInProgress:
        pushTaskToUndoStack(id);
        newStatus = "in progress";
        break;
    case TaskActionDialogResult::Delete: {
        pushDeletedTaskToUndoStack(id);
        QSqlQuery deleteQuery(db);
        deleteQuery.prepare("DELETE FROM tasks WHERE id = ?");
        deleteQuery.addBindValue(id);
        deleteQuery.exec();
        refreshAllTasksFromDb();
        displayTasks();
        QMessageBox::information(this, "Task Deleted", QString("The task '%1' has been deleted.").arg(t.title));
        return;
    }
    default:
        return;
    }
    QSqlQuery updateQuery(db);
    updateQuery.prepare("UPDATE tasks SET status = ? WHERE id = ?");
    updateQuery.addBindValue(newStatus);
    updateQuery.addBindValue(id);
    updateQuery.exec();
    refreshAllTasksFromDb();
    displayTasks();
    QMessageBox::information(this, "Task Updated", QString("The task '%1' has been updated to '%2'.").arg(t.title, newStatus));
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

void MainWindow::on_SearchPageButton_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_3);
}


void MainWindow::on_SearchButton_clicked()
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) db.open();
    QString searchText = ui->SearchLineEdit->text();
    if (searchText.isEmpty()) {
        QMessageBox::warning(this, "Search Error", "Please enter a search term.");
        return;
    }
    QSqlQuery query(db);
    query.prepare("SELECT * FROM tasks WHERE title LIKE ? OR description LIKE ?");
    query.addBindValue("%" + searchText + "%");
    query.addBindValue("%" + searchText + "%");
    query.exec();
    ui->SearchListWidget->clear();
    while(query.next()) {
        Task t;
        t.id = query.value("id").toInt();
        t.title = query.value("title").toString();
        t.description = query.value("description").toString();
        t.dueDate = query.value("due_date").toString();
        t.subTasks = query.value("sub_tasks").toString();
        t.priority = query.value("priority").toInt();
        t.status = query.value("status").toString();
        QString itemText = QString("(%1) %2 - Due: %3").arg(t.id).arg(t.title).arg(t.dueDate);
        ui->SearchListWidget->addItem(itemText);
    }
    if (ui->SearchListWidget->count() == 0) {
        ui->SearchListWidget->addItem("No tasks found.");
    }
    db.close();

}


void MainWindow::on_BackButtonSearch_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_2);
    ui->SearchLineEdit->clear();
    ui->SearchListWidget->clear();
    refreshAllTasksFromDb();
    displayTasks();
}


void MainWindow::on_NotificationButton_clicked()
{
    displayNotifications();
}


void MainWindow::on_NotificationlistWidget_doubleClicked(const QModelIndex &index)
{
    QListWidgetItem* item = ui->NotificationlistWidget->item(index.row());
    if (!item) return;

    QString taskDetails = item->text();
    QStringList details = taskDetails.split(" - ");
    if (details.isEmpty()) {
        QMessageBox::warning(this, "Selection Error", "Please select a valid task.");
        return;
    }

    QString leftPart = details[0];
    QRegularExpression re("^\\((\\d+)\\)\\s*(.+)$");
    QRegularExpressionMatch match = re.match(leftPart);
    if (!match.hasMatch()) {
        QMessageBox::warning(this, "Parsing Error", "Could not extract task ID and title.");
        return;
    }

    int id = match.captured(1).toInt();
    Task t = getTaskById(id);
    QStringList subTasksArray = t.subTasks.split(",", Qt::SkipEmptyParts);

    TaskDialog dlg(t.title, t.description, t.dueDate, t.priority, subTasksArray, t.status, this);
    dlg.setWindowTitle("Task Options");
    dlg.exec();

    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) db.open();

    QString newStatus;
    switch (dlg.result()) {
    case TaskActionDialogResult::SetToPending:
        pushTaskToUndoStack(id);
        newStatus = "pending";
        break;
    case TaskActionDialogResult::SetToComplete:
        pushTaskToUndoStack(id);
        newStatus = "complete";
        break;
    case TaskActionDialogResult::Delete: {
        pushDeletedTaskToUndoStack(id);
        QSqlQuery deleteQuery(db);
        deleteQuery.prepare("DELETE FROM tasks WHERE id = ?");
        deleteQuery.addBindValue(id);
        deleteQuery.exec();
        refreshAllTasksFromDb();
        displayTasks();
        QMessageBox::information(this, "Task Deleted", QString("The task '%1' has been deleted.").arg(t.title));
        return;
    }
    default:
        return;
    }

    QSqlQuery updateQuery(db);
    updateQuery.prepare("UPDATE tasks SET status = ? WHERE id = ?");
    updateQuery.addBindValue(newStatus);
    updateQuery.addBindValue(id);
    updateQuery.exec();

    refreshAllTasksFromDb();
    displayTasks();
    QMessageBox::information(this, "Task Updated", QString("The task '%1' has been updated to '%2'.").arg(t.title, newStatus));
}



void MainWindow::on_BackButtonNotif_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_2);
}

QString MainWindow::buildTaskJson(const Task& task, int level, bool isLastItem)
    {
        QString indent(level * 2, ' ');
        QString json;

        json += indent + "{\n";
        json += indent + "  \"id\": " + QString::number(task.id) + ",\n";

        QString escapedTitle = task.title;
        escapedTitle.replace("\"", "\\\"");
        json += indent + "  \"title\": \"" + escapedTitle + "\",\n";

        QString escapedDesc = task.description;
        escapedDesc.replace("\"", "\\\"");
        json += indent + "  \"description\": \"" + escapedDesc + "\",\n";

        json += indent + "  \"dueDate\": \"" + task.dueDate + "\",\n";
        json += indent + "  \"priority\": " + QString::number(task.priority) + ",\n";
        json += indent + "  \"status\": \"" + task.status + "\",\n";

        if (!task.subTasks.isEmpty()) {
            QString escapedSubTasks = task.subTasks;
            escapedSubTasks.replace("\"", "\\\"");
            json += indent + "  \"subtasks\": \"" + escapedSubTasks + "\"\n";
        } else {
            json += indent + "  \"subtasks\": \"\"\n";
        }

        json += indent + "}";

        if (!isLastItem)
            json += ",";

        json += "\n";

        return json;
    }

    void MainWindow::on_ExportButton_clicked()
    {
        QString fileName = QFileDialog::getSaveFileName(this,
                                                        tr("Export Tasks"), "",
                                                        tr("JSON Files (*.json);;All Files (*)"));

        if (fileName.isEmpty())
            return;

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, "Export Error", "Could not open file for writing.");
            return;
        }

        QTextStream out(&file);

        out << "{\n";
        out << "  \"tasks\": [\n";

        refreshAllTasksFromDb();
        for (int i = 0; i < allTasks.size(); ++i) {
            out << buildTaskJson(allTasks[i], 2, i == allTasks.size()-1);
        }

        out << "  ]\n";
        out << "}\n";

        file.close();
        QMessageBox::information(this, "Export Successful",
                                 QString("Tasks exported to %1").arg(fileName));
    }
