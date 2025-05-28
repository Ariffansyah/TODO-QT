#ifndef TASKDIALOG_H
#define TASKDIALOG_H

#include <QDialog>
#include <QVector>
#include <QPushButton>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLabel>

enum class TaskActionDialogResult {
    None,
    SetToPending,
    SetToInProgress,
    SetToComplete,
    Delete
};

struct TreeNode {
    QString name;
    bool completed;
    QVector<TreeNode*> children;

    TreeNode(const QString &name)
        : name(name), completed(false) {}

    ~TreeNode() {
        qDeleteAll(children);
    }

    void addChild(TreeNode* child) {
        children.append(child);
    }
};

class TaskDialog : public QDialog {
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

private slots:
    void onSubtaskToggled(bool);
    void onButtonClicked();

private:
    TaskActionDialogResult m_result;
    TreeNode* root;
    QVector<QCheckBox*> checkBoxes;
    QPushButton *pendingBtn, *inProgressBtn, *completeBtn, *deleteBtn, *cancelBtn;
    QVBoxLayout *mainLayout;

    void setupUi(const QString &taskTitle, const QString &description, const QString &dueDate, int priority, const QStringList &subTasks, const QString &status);
    void buildSubTaskTree(const QStringList &subTasks);
    void displaySubTaskTree(TreeNode* node, QVBoxLayout *layout);
    bool areAllSubTasksCompleted() const;
};

#endif // TASKDIALOG_H
