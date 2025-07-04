#ifndef DLG_DEFAULT_TAGS_EDITOR_H
#define DLG_DEFAULT_TAGS_EDITOR_H

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

class DlgDefaultTagsEditor : public QDialog
{
    Q_OBJECT

public:
    explicit DlgDefaultTagsEditor(QWidget *parent = nullptr);

private slots:
    void addItem();
    void deleteItem(QListWidgetItem *item);
    void confirmChanges();

private:
    QVBoxLayout *mainLayout;
    QHBoxLayout *inputLayout;
    QListWidget *listWidget;
    QHBoxLayout *buttonLayout;
    QLineEdit *inputField;
    QPushButton *addButton;
    QPushButton *confirmButton;
    QPushButton *cancelButton;

    void loadStringList();
    void retranslateUi();
};

#endif // DLG_DEFAULT_TAGS_EDITOR_H
