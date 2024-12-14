#ifndef DLG_MOVE_TOP_CARDS_UNTIL_H
#define DLG_MOVE_TOP_CARDS_UNTIL_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

class FilterString;

class DlgMoveTopCardsUntil : public QDialog
{
    Q_OBJECT

    QLabel *exprLabel, *numberOfHitsLabel;
    QLineEdit *exprEdit;
    QSpinBox *numberOfHitsEdit;
    QDialogButtonBox *buttonBox;

    void validateAndAccept();
    bool validateMatchExists(const FilterString &filterString);

    void showSearchSyntaxHelp();

public:
    explicit DlgMoveTopCardsUntil(QWidget *parent = nullptr, QString expr = QString(), uint numberOfHits = 1);

signals:
    void exprSet(QString expr) const;
    void numberOfHitsSet(uint numberOfHits) const;
};

#endif // DLG_MOVE_TOP_CARDS_UNTIL_H
