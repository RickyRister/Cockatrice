#include "dlg_move_top_cards_until.h"

#include "../client/ui/search_syntax_help.h"
#include "../game/cards/card_database.h"
#include "../game/cards/card_database_manager.h"
#include "../game/filters/filter_string.h"
#include "trice_limits.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QSpinBox>
#include <QString>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QWidget>

DlgMoveTopCardsUntil::DlgMoveTopCardsUntil(QWidget *parent, QString _expr, uint _numberOfHits) : QDialog(parent)
{
    exprLabel = new QLabel();

    exprEdit = new QLineEdit(this);
    exprEdit->setFocus();
    exprEdit->setText(_expr);
    exprLabel->setBuddy(exprEdit);

    auto help = exprEdit->addAction(QPixmap("theme:icons/info"), QLineEdit::TrailingPosition);
    connect(help, &QAction::triggered, this, &DlgMoveTopCardsUntil::showSearchSyntaxHelp);

    numberOfHitsLabel = new QLabel();
    numberOfHitsEdit = new QSpinBox(this);
    numberOfHitsEdit->setRange(1, 99);
    numberOfHitsEdit->setValue(_numberOfHits);
    numberOfHitsLabel->setBuddy(numberOfHitsEdit);

    auto *grid = new QGridLayout;
    grid->addWidget(numberOfHitsLabel, 0, 0);
    grid->addWidget(numberOfHitsEdit, 0, 1);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &DlgMoveTopCardsUntil::validateAndAccept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(exprLabel);
    mainLayout->addWidget(exprEdit);
    mainLayout->addItem(grid);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);

    retranslateUi();
}

void DlgMoveTopCardsUntil::retranslateUi()
{
    setWindowTitle(tr("Put top cards on stack until..."));
    exprLabel->setText(tr("Card name (or search expressions):"));
    numberOfHitsLabel->setText(tr("Number of hits:"));
}

/**
 * @brief Checks if a card matching the expr exists in the card database.
 *
 * @returns true if a card matching the expression exists.
 */
static bool matchExistsInDb(const FilterString &filterString)
{
    const auto cardDatabase = CardDatabaseManager::getInstance();
    const auto allCards = cardDatabase->getCardList();

    const auto it = std::find_if(allCards.begin(), allCards.end(),
                                 [&filterString](const CardInfoPtr &card) { return filterString.check(card); });

    return it != allCards.end();
}

/**
 * @brief Validates that a card matching the expr exists in the card database.
 * If no match is found, then pop up a window to warn the user, giving them a chance to back out.
 *
 * @returns whether to proceed with the action
 */
bool DlgMoveTopCardsUntil::validateMatchExists(const FilterString &filterString)
{
    if (matchExistsInDb(filterString)) {
        return true;
    }

    const auto msg = tr("No cards matching the search expression exists in the card database. Proceed anyways?");
    const auto res =
        QMessageBox::warning(this, tr("Cockatrice"), msg, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (res == QMessageBox::No) {
        return false;
    }

    return true;
}

void DlgMoveTopCardsUntil::validateAndAccept()
{
    auto movingCardsUntilFilter = FilterString(exprEdit->text());
    if (!movingCardsUntilFilter.valid()) {
        QMessageBox::warning(this, tr("Invalid filter"), movingCardsUntilFilter.error(), QMessageBox::Ok);
        return;
    }

    if (!validateMatchExists(movingCardsUntilFilter)) {
        return;
    }

    emit exprSet(exprEdit->text());
    emit numberOfHitsSet(numberOfHitsEdit->text().toUInt());
    accept();
}

void DlgMoveTopCardsUntil::showSearchSyntaxHelp()
{
    auto *browser = createSearchSyntaxHelp(this);

    if (!browser) {
        return;
    }

    connect(browser, &QTextBrowser::anchorClicked, [=](const QUrl &link) { exprEdit->setText(link.fragment()); });
    browser->show();
}
