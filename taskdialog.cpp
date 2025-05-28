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
    : QDialog(parent), m_result(TaskActionDialogResult::None)
{
    setupUi(taskTitle, description, dueDate, priority, subTasks, status);
}

void TaskDialog::setupUi(const QString &taskTitle, const QString &description, const QString &dueDate, int priority, const QStringList &subTasks, const QString &status)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

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
    layout->addWidget(label);

    treeWidget = new QTreeWidget(this);
    treeWidget->setHeaderHidden(true);
    if (!subTasks.isEmpty()) {
        QTreeWidgetItem *root = new QTreeWidgetItem(treeWidget, QStringList() << "Subtasks");
        root->setFlags(root->flags() & ~Qt::ItemIsUserCheckable);
        for (const QString &st : subTasks) {
            if (st.trimmed().isEmpty()) continue;
            QTreeWidgetItem *child = new QTreeWidgetItem(root, QStringList() << st.trimmed());
            child->setCheckState(0, Qt::Unchecked);
        }
        treeWidget->expandAll();
        connect(treeWidget, &QTreeWidget::itemChanged, this, &TaskDialog::onSubTaskStateChanged);
        layout->addWidget(treeWidget);
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
    layout->addLayout(btnLayout);

    connect(pendingBtn, &QPushButton::clicked, this, &TaskDialog::onButtonClicked);
    connect(inProgressBtn, &QPushButton::clicked, this, &TaskDialog::onButtonClicked);
    connect(completeBtn, &QPushButton::clicked, this, &TaskDialog::onButtonClicked);
    connect(deleteBtn, &QPushButton::clicked, this, &TaskDialog::onButtonClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &TaskDialog::onButtonClicked);

    // Only show available actions
    if (status == "pending") pendingBtn->setDisabled(true);
    if (status == "in progress") inProgressBtn->setDisabled(true);
    if (status == "complete") completeBtn->setDisabled(true);

    completeBtn->setEnabled(areAllSubTasksCompleted());
}

bool TaskDialog::areAllSubTasksCompleted() const
{
    if (!treeWidget || treeWidget->topLevelItemCount() == 0)
        return true; // No subtasks
    QTreeWidgetItem *root = treeWidget->topLevelItem(0);
    for (int i = 0; i < root->childCount(); ++i) {
        if (root->child(i)->checkState(0) != Qt::Checked)
            return false;
    }
    return true;
}

void TaskDialog::onSubTaskStateChanged(QTreeWidgetItem*, int)
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
