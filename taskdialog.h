#ifndef TASKDIALOG_H
#define TASKDIALOG_H

#include <QDialog>
#include <QTreeWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>

enum class TaskActionDialogResult {
    None,
    SetToPending,
    SetToInProgress,
    SetToComplete,
    Delete
};

class TaskDialog : public QDialog
{
    Q_OBJECT
public:
    TaskDialog(const QString &taskTitle,
               const QString &description,
               const QString &dueDate,
               int priority,
               const QStringList &subTasks,
               const QString &status,
               QWidget *parent = nullptr);

    TaskActionDialogResult result() const { return m_result; }
    bool areAllSubTasksCompleted() const;

private slots:
    void onButtonClicked();
    void onSubTaskStateChanged(QTreeWidgetItem*, int);

private:
    QTreeWidget *treeWidget;
    QPushButton *pendingBtn, *inProgressBtn, *completeBtn, *deleteBtn, *cancelBtn;
    TaskActionDialogResult m_result;

    void setupUi(const QString &taskTitle, const QString &description, const QString &dueDate, int priority, const QStringList &subTasks, const QString &status);
};

#endif // TASKDIALOG_H
