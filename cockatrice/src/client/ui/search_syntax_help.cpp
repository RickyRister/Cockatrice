#include "search_syntax_help.h"

#include <QMenu>
#include <QtCore/QFile>
#include <QtCore/qregularexpression.h>

QTextBrowser *createSearchSyntaxHelp(QWidget *parent)
{
    QFile file("theme:help/search.md");

    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        return nullptr;
    }

    QTextStream in(&file);
    QString text = in.readAll();
    file.close();

    // Poor Markdown Converter
    auto opts = QRegularExpression::MultilineOption;
    text = text.replace(QRegularExpression("^(###)(.*)", opts), "<h3>\\2</h3>")
               .replace(QRegularExpression("^(##)(.*)", opts), "<h2>\\2</h2>")
               .replace(QRegularExpression("^(#)(.*)", opts), "<h1>\\2</h1>")
               .replace(QRegularExpression("^------*", opts), "<hr />")
               .replace(QRegularExpression(R"(\[([^[]+)\]\(([^\)]+)\))", opts), R"(<a href='\2'>\1</a>)");

    auto browser = new QTextBrowser;
    browser->setParent(parent, Qt::Window | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                                   Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint |
                                   Qt::WindowFullscreenButtonHint);
    browser->setWindowTitle("Search Help");
    browser->setReadOnly(true);
    browser->setMinimumSize({500, 600});

    QString sheet = QString("a { text-decoration: underline; color: rgb(71,158,252) };");
    browser->document()->setDefaultStyleSheet(sheet);

    browser->setHtml(text);
    return browser;
}