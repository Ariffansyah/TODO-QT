#include "taskdialog.h"
#include <QHBoxLayout>
#include <QMessageBox>

TaskDialog::TaskDialog(const QString &taskTitle,
                       const QString &description,
                       const QString &dueDate,
                       int priority,
                       const QStringList &subTasks,
                       const QString &status,
                       QWidget *parent)
    : QDialog(parent), m_result(TaskActionDialogResult::None), root(nullptr)
{
    setupUi(taskTitle, description, dueDate, priority, subTasks, status);
}

void TaskDialog::setupUi(const QString &taskTitle, const QString &description,
                         const QString &dueDate, int priority,
                         const QStringList &subTasks, const QString &status)
{
    mainLayout = new QVBoxLayout(this);

    QString html = "<b>" + taskTitle + "</b>";
    if (!dueDate.isEmpty())
        html += "<br>Due Date: " + dueDate;
    if (!description.isEmpty())
        html += "<br>Description: " + description;
    if (priority > 0)
        html += "<br>Priority: " + QString::number(priority);
    html += "<br>Status: " + status;
    if (!subTasks.isEmpty())
        html += "<br><br><b>Check subtasks as you complete them.</b>";

    QLabel *label = new QLabel(html, this);
    label->setWordWrap(true);
    mainLayout->addWidget(label);

    buildSubTaskTree(subTasks);
    if (root) {
        displaySubTaskTree(root, mainLayout);
    }

    QHBoxLayout *btnLayout = new QHBoxLayout;
    pendingBtn = new QPushButton("Set to Pending", this);
    inProgressBtn = new QPushButton("Set to In Progress", this);
    completeBtn = new QPushButton("Set to Complete", this);
    deleteBtn = new QPushButton("Delete", this);
    cancelBtn = new QPushButton("Cancel", this);

    btnLayout->addWidget(pendingBtn);
    btnLayout->addWidget(inProgressBtn);
    btnLayout->addWidget(completeBtn);
    btnLayout->addWidget(deleteBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);

    connect(pendingBtn, &QPushButton::clicked, this, &TaskDialog::onButtonClicked);
    connect(inProgressBtn, &QPushButton::clicked, this, &TaskDialog::onButtonClicked);
    connect(completeBtn, &QPushButton::clicked, this, &TaskDialog::onButtonClicked);
    connect(deleteBtn, &QPushButton::clicked, this, &TaskDialog::onButtonClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &TaskDialog::onButtonClicked);

    if (status == "pending") pendingBtn->setDisabled(true);
    if (status == "in progress") inProgressBtn->setDisabled(true);
    if (status == "complete") completeBtn->setDisabled(true);

    completeBtn->setEnabled(areAllSubTasksCompleted());
}

void TaskDialog::buildSubTaskTree(const QStringList &subTasks)
{
    root = new TreeNode("Subtasks");
    for (const QString &st : subTasks) {
        if (!st.trimmed().isEmpty())
            root->addChild(new TreeNode(st.trimmed()));
    }
}

void TaskDialog::displaySubTaskTree(TreeNode* node, QVBoxLayout *layout)
{
    if (!node) return;
    for (TreeNode* child : node->children) {
        QCheckBox *checkBox = new QCheckBox(child->name, this);
        connect(checkBox, &QCheckBox::toggled, this, &TaskDialog::onSubtaskToggled);
        checkBoxes.append(checkBox);
        layout->addWidget(checkBox);
    }
}

bool TaskDialog::areAllSubTasksCompleted() const
{
    for (QCheckBox* cb : checkBoxes) {
        if (!cb->isChecked())
            return false;
    }
    return true;
}

void TaskDialog::onSubtaskToggled(bool)
{
    completeBtn->setEnabled(areAllSubTasksCompleted());
}

void TaskDialog::onButtonClicked()
{
    QObject *btn = sender();
    if (btn == pendingBtn) {
        m_result = TaskActionDialogResult::SetToPending;
    } else if (btn == inProgressBtn) {
        m_result = TaskActionDialogResult::SetToInProgress;
    } else if (btn == completeBtn) {
        if (!areAllSubTasksCompleted()) {
            QMessageBox::warning(this, "Incomplete Subtasks", "Please complete all subtasks before marking the task as complete.");
            return;
        }
        m_result = TaskActionDialogResult::SetToComplete;
    } else if (btn == deleteBtn) {
        m_result = TaskActionDialogResult::Delete;
    } else {
        m_result = TaskActionDialogResult::None;
    }
    accept();
}
