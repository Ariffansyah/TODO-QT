#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtSql>
#include <QSqlDatabase>

using namespace std;

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

struct TaskNode {
    Task data;
    TaskNode* next;
    TaskNode(Task t) : data(t), next(nullptr) {}
};

class TaskQueue {
private:
    TaskNode* front;
    TaskNode* rear;

public:
    TaskQueue() : front(nullptr), rear(nullptr) {}

    ~TaskQueue() {
        while (front != nullptr) {
            TaskNode* temp = front;
            front = front->next;
            delete temp;
        }
    }

    void enqueue(const Task& task) {
        TaskNode* newNode = new TaskNode(task);
        if (rear == nullptr) { // kosong
            front = rear = newNode;
        } else {
            rear->next = newNode;
            rear = newNode;
        }
    }

    bool isEmpty() const {
        return front == nullptr;
    }

    Task dequeue() {
        if (isEmpty()) {
            throw std::runtime_error("Queue is empty");
        }
        TaskNode* temp = front;
        Task ret = temp->data;
        front = front->next;
        if (front == nullptr) {
            rear = nullptr;
        }
        delete temp;
        return ret;
    }
};

struct StackNode {
    TaskAction data;
    StackNode* next;
};

class Stack {
    private:
        StackNode* top;
    public:
        Stack() : top(nullptr) {}

        ~Stack() {
            while (top != nullptr) {
                StackNode* temp = top;
                top = top->next;
                delete temp;
            }
        }

        bool isEmpty() const {
            return top == nullptr;
        }

        void clear() {
            while (top != nullptr) {
                StackNode* temp = top;
                top = top->next;
                delete temp;
            }
        }

        void push_back(const TaskAction& action) {
            StackNode* newNode = new StackNode{action, top};
            top = newNode;
        }

        TaskAction takeLast() {
            if (isEmpty()) {
                throw out_of_range("Stack is empty");
            }
            StackNode* temp = top;
            TaskAction result = top->data;
            top = top->next;
            delete temp;
            return result;
        }
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
    void displayNotifications();
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
    void on_SearchPageButton_clicked();
    void on_SearchButton_clicked();
    void on_BackButtonSearch_clicked();
    void on_NotificationButton_clicked();

    void on_NotificationlistWidget_doubleClicked(const QModelIndex &index);

    void on_BackButtonNotif_clicked();

private:
    Ui::MainWindow *ui;
    QVector<Task> allTasks;
    Stack undoStack, redoStack;
};

int findTaskIndexById(const QVector<Task>& tasks, int id);

#endif // MAINWINDOW_H
