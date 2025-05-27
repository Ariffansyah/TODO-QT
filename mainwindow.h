#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSql>
#include <QSqlDatabase>

enum class TaskActionType { Update, Delete };

struct Task {
    int id;
    QString title;
    QString description;
    QString dueDate;
    QString subTasks;
    int priority;
    QString status;
};

struct TaskAction {
    Task task;
    TaskActionType type;
};

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void createDatabase();
    void displayTasks();
    void displayTasksByDeadline();
    void displayTasksByPriority();
    void pushTaskToUndoStack(const int id);
    void pushDeletedTaskToUndoStack(const int id);
    void pushTaskToRedoStack(const int id, TaskActionType type);
    void refreshAllTasksFromDb();
    void on_StartButton_clicked();
    void on_AddButton_clicked();
    void on_ReloadButton_clicked();
    void on_SortByDeadlineButton_clicked();
    void on_SortByPriorityButton_clicked();
    void on_PendingList_doubleClicked(const QModelIndex &index);
    void on_InProgressList_doubleClicked(const QModelIndex &index);
    void on_CompleteList_doubleClicked(const QModelIndex &index);
    void on_UndoButton_clicked();
    void on_RedoButton_clicked();

private:
    Ui::MainWindow *ui;
    QVector<Task> allTasks;
    QVector<TaskAction> undoStack, redoStack;
};

int findTaskIndexById(const QVector<Task>& tasks, int id);

#endif // MAINWINDOW_H
