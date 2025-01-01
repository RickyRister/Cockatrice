#include "view_zone.h"

#include "../../server/pending_command.h"
#include "../cards/card_database.h"
#include "../cards/card_drag_item.h"
#include "../cards/card_item.h"
#include "../player/player.h"
#include "pb/command_dump_zone.pb.h"
#include "pb/command_move_card.pb.h"
#include "pb/response_dump_zone.pb.h"
#include "pb/serverinfo_card.pb.h"

#include <QBrush>
#include <QDebug>
#include <QGraphicsSceneWheelEvent>
#include <QPainter>
#include <QtMath>

/**
 * @param _p the player that the cards are revealed to.
 * @param _origZone the zone the cards were revealed from.
 * @param _revealZone if false, the cards will be face down.
 * @param _writeableRevealZone whether the player can interact with the revealed cards.
 * @param parent the parent QGraphicsWidget containing the reveal zone
 */
ZoneViewZone::ZoneViewZone(Player *_p,
                           CardZone *_origZone,
                           int _numberCards,
                           bool _revealZone,
                           bool _writeableRevealZone,
                           QGraphicsItem *parent,
                           bool _isReversed)
    : SelectZone(_p, _origZone->getName(), false, false, true, parent, true), bRect(QRectF()), minRows(0),
      numberCards(_numberCards), origZone(_origZone), revealZone(_revealZone),
      writeableRevealZone(_writeableRevealZone), groupBy(CardList::NoSort), sortBy(CardList::NoSort),
      isReversed(_isReversed)
{
    if (!(revealZone && !writeableRevealZone)) {
        origZone->getViews().append(this);
    }
}

ZoneViewZone::~ZoneViewZone()
{
    emit beingDeleted();
    qDebug("ZoneViewZone destructor");
    if (!(revealZone && !writeableRevealZone)) {
        origZone->getViews().removeOne(this);
    }
}

QRectF ZoneViewZone::boundingRect() const
{
    return bRect;
}

void ZoneViewZone::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    QBrush windowBrush(QColor(240, 240, 240));
    windowBrush.setColor(windowBrush.color().darker(150));
    painter->fillRect(boundingRect(), windowBrush);
}

void ZoneViewZone::initializeCards(const QList<const ServerInfo_Card *> &cardList)
{
    if (!cardList.isEmpty()) {
        for (int i = 0; i < cardList.size(); ++i)
            addCard(new CardItem(player, this, QString::fromStdString(cardList[i]->name()),
                                 QString::fromStdString(cardList[i]->provider_id()), cardList[i]->id(), revealZone),
                    false, i);
        reorganizeCards();
    } else if (!origZone->contentsKnown()) {
        Command_DumpZone cmd;
        cmd.set_player_id(player->getId());
        cmd.set_zone_name(name.toStdString());
        cmd.set_number_cards(numberCards);
        cmd.set_is_reversed(isReversed);

        PendingCommand *pend = player->prepareGameCommand(cmd);
        connect(pend, SIGNAL(finished(Response, CommandContainer, QVariant)), this,
                SLOT(zoneDumpReceived(const Response &)));
        player->sendGameCommand(pend);
    } else {
        const CardList &c = origZone->getCards();
        int number = numberCards == -1 ? c.size() : (numberCards < c.size() ? numberCards : c.size());
        for (int i = 0; i < number; i++) {
            CardItem *card = c.at(i);
            addCard(new CardItem(player, this, card->getName(), card->getProviderId(), card->getId(), revealZone),
                    false, i);
        }
        reorganizeCards();
    }
}

void ZoneViewZone::zoneDumpReceived(const Response &r)
{
    const Response_DumpZone &resp = r.GetExtension(Response_DumpZone::ext);
    const int respCardListSize = resp.zone_info().card_list_size();
    for (int i = 0; i < respCardListSize; ++i) {
        const ServerInfo_Card &cardInfo = resp.zone_info().card_list(i);
        auto cardName = QString::fromStdString(cardInfo.name());
        auto cardProviderId = QString::fromStdString(cardInfo.provider_id());
        auto *card = new CardItem(player, this, cardName, cardProviderId, cardInfo.id(), revealZone, this);
        cards.insert(i, card);
    }
    reorganizeCards();
    emit cardCountChanged();
}

// Because of boundingRect(), this function must not be called before the zone was added to a scene.
void ZoneViewZone::reorganizeCards()
{
    if (cards.isEmpty()) {
        return;
    }

    int cardCount = cards.size();
    if (!origZone->contentsKnown()) {

        /*
         * General idea: If the first card is selected, use the 2nd. If the 2nd card is selected, use the first.
         * We always need to know the "last" card we're working with, to start the baseline.
         *
         * If we remove card N, N>1, we have the first card's ID to default on to restore all the other IDs
         * If we remove the first card, we use the 2nd, which is now the first, and it fucks up.
         * We can look for a "jump", which would indicate the N>1 card was grabbed.
         * If no jump is detected, add a -1 option
         */

        // const auto &firstCardId = cards.first()->getId();
        // const auto &secondCardId = cards.at(1)->getId();
        //
        // int startId;
        // if (firstCardId + 1 == secondCardId) {
        //     // No jump detected, so we removed the first card, reset down by 1
        //     startId = firstCardId - 1;
        // } else {
        //     // Jump detected, so we removed
        //     startId = firstCardId;
        // }

        /**
         * If the first card is taken, every card will be in a row now. If that's teh case, no jump will exist
         * anywhere. As such, subtract one. If a jump is found anywhere, then we know we can start at the beginning
         * value instead.
         */
        // int jumpOffset = 0;
        // for (int i = 0; i < cards.size() - 1; i++) {
        //     if (cards.at(i)->getId() + 1 != cards.at(i + 1)->getId()) {
        //         qDebug() << "TRACK" << "INCONSISTENCY DETECTED!!, USE THE FIRST CARD";
        //         jumpOffset = 1;
        //         break;
        //     }
        // }
        //
        // if (jumpOffset) {
        //     qDebug() <<    "TRACK" << "INCONSISTENCY IS DETECTED, SO WE WILL SUBTRACT 1 FROM" ;
        // }
        //
        // int startId = 0;
        // if (isReversed) {
        //     if (jumpOffset == 1) {
        //         startId = cards.at(1)->getId() - jumpOffset;
        //     } else if (jumpOffset == 0) {
        //         startId = cards.first()->getId();
        //     }
        // }

        int startId = 0;

        if (isReversed) {
            /////////// Check back with how cards are added/removed from cards[] ig
            ///// i'm done for one night
            std::sort(cards.begin(), cards.end(), [](const auto &a, const auto &b) { return a->getId() < b->getId(); });
            /*
             * We have a list of card IDs (5,6,7,...N)
             */
            // for (int i = 0; i < origZone->getCards().size(); ++i) {
            //     qDebug() << "TRACK origZone CardsAt(i)" << i << origZone->getCards().at(i)->getId();
            // }
            // for (int i = 0; i < cards.size(); ++i) {
            //     qDebug() << "TRACK cards CardsAt(i)" << i << cards.at(i)->getId();
            // }
            startId = cards.first()->getId();
            //
            // bool problemFound = true;
            // // Detect if the FIRST card was removed, and we really got the 2nd card here
            // for (int i = 0; i < cards.size() - 1; ++i) {
            //     if (cards.at(i)->getId() + 1 != cards.at(i + 1)->getId()) {
            //
            //         problemFound = !problemFound;
            //         break;
            //     }
            // }

            qDebug() << "TRACK" << "NUMBER CARDS" << numberCards << "CARD SIZE" << cards.size();
            bool isFirstLoad = (numberCards == cards.size());
            qDebug() << "TRACK" << "IS FIRST LOAD" << isFirstLoad;

            // Sort cards, because i'm desperat
            // @CHATGPT: How to sort cards by cards.at(i)->getId()


            if (isFirstLoad && lowCardId == 0) {
                lowCardId = cards.first()->getId();
                highCardId = cards.last()->getId();
            }//cSScx xX CSSsdvvdsaavabnsgsdnarasads

// I'm out of ideas

            // We had 5,6,7,8,9 in our list. lowCardId = 5
            // We just checked and now 6 != 5
            // This means we need to decriment by 1
            // Update lowCardId = 6, for future reference
            // Since the deck size got smaller (somehow!) we can update the highcardId
            // idk what to do with the high card ID tho. If it's removed, nbd?

            if (cards.first()->getId() == lowCardId) {
                // Lowest card wasn't removed, do nothing from here?
                qDebug() << "TRACK" << "CONDITION 1, WE MIGHT HAVE TO DO SOMETHING";
                // startId += 1;
            } else if (cards.first()->getId() == lowCardId + 1) {
                // Lowest card _WAS_ removed. account for this!
                startId -= 1;
                lowCardId = cards.first()->getId();
                qDebug() << "TRACK" << "CONDITION 2, WE MIGHT HAVE TO DO SOMETHING";
            } else if (cards.last()->getId() == highCardId) {
                // Highest card wasn't removed, we're good
                qDebug() << "TRACK" << "CONDITION 3, WE MIGHT HAVE TO DO SOMETHING";
            } else if (cards.last()->getId() != highCardId) {
                // Clearly this means the last card was removed, right? Just... don't do anything stupid
                // startId += 1;
                highCardId = cards.last()->getId();
                qDebug() << "TRACK" << "CONDITION 4, WE MIGHT HAVE TO DO SOMETHING";
                lowCardId = cards.first()->getId() + 1; // Because fuck you i guess
            }



            // if (cards.first()->getId() != lowCardId) {
            //     // We removed the first card, subtract one
            //     startId -= 1;
            //     lowCardId = cards.first()->getId();
            //     // highCardId = cards.last()->getId();
            //     // ........................................................
            //     //
            // }
            // else if (cards.first()->getId() != lowCardId + 1) {
            //
            // } else if (cards.last()->getId() != highCardId) {
            //     // We removed the last card, DO NOT TOUCH SHIT??
            //     qDebug() << "TRACK" << "TODO SOMETHING??";
            //     startId += 1; // THIS NEVER HAPPENED. WHY NOT??
            // }

            // If the top card or bottom card are moved, we need to know.
            // Lets track them in the class. For shiggles.

            // Somehow we need this to flip if its a second time ... ... .......................
            // .....

            // The first card was fucked with, we're fucked.
            // However, this can be a problem only if the cardsize changed
            // Why did the cardsize change?? It'sm ad at us
            // Trying to figure this out is... WHY HENTAI
            // if (problemFound && !isFirstLoad) {
            //     startId -= 1;
            // }
        }

        // startId = isReversed ? cards.first()->getId() : 0;
        qDebug() << "TRACK" << "zone:" << name << "origZone:" << origZone->getCards().size()
                 << "cardCount:" << cardCount << "startId:" << startId;
        for (int i = 0; i < cardCount; ++i) {
            cards[i]->setId(startId + i);
        }
    }

    CardList cardsToDisplay(cards);

    // sort cards
    QList<CardList::SortOption> sortOptions;
    if (groupBy != CardList::NoSort) {
        sortOptions << groupBy;
    }

    if (sortBy != CardList::NoSort) {
        sortOptions << sortBy;

        // implicitly sort by name at the end so that cards with the same name appear together
        if (sortBy != CardList::SortByName) {
            sortOptions << CardList::SortByName;
        }
    }

    cardsToDisplay.sortBy(sortOptions);

    // position cards
    GridSize gridSize;
    if (pileView) {
        gridSize = positionCardsForDisplay(cardsToDisplay, groupBy);
    } else {
        gridSize = positionCardsForDisplay(cardsToDisplay);
    }

    // determine bounding rect
    qreal aleft = 0;
    qreal atop = 0;
    qreal awidth = gridSize.cols * CARD_WIDTH + (CARD_WIDTH / 2) + HORIZONTAL_PADDING;
    qreal aheight = (gridSize.rows * CARD_HEIGHT) / 3 + CARD_HEIGHT * 1.3;
    optimumRect = QRectF(aleft, atop, awidth, aheight);

    updateGeometry();
    emit optimumRectChanged();
}

/**
 * @brief Sets the position of each card to the proper position for the view
 *
 * @param cards The cards to reposition. Will modify the cards in the list.
 * @param pileOption Property used to group cards for the piles. Expects `cards` to be sorted by that property. Pass in
 * NoSort to not make piles.
 *
 * @returns The number of rows and columns to display
 */
ZoneViewZone::GridSize ZoneViewZone::positionCardsForDisplay(CardList &cards, CardList::SortOption pileOption)
{
    int cardCount = cards.size();

    if (pileOption != CardList::NoSort) {
        int row = 0;
        int col = 0;
        int longestRow = 0;

        QString lastColumnProp;

        const auto extractor = CardList::getExtractorFor(pileOption);

        for (int i = 0; i < cardCount; i++) {
            CardItem *c = cards.at(i);
            QString columnProp = extractor(c);

            if (i) { // if not the first card
                if (columnProp == lastColumnProp)
                    row++; // add below current card
                else {     // if no match then move card to next column
                    col++;
                    row = 0;
                }
            }

            lastColumnProp = columnProp;
            qreal x = col * CARD_WIDTH;
            qreal y = row * CARD_HEIGHT / 3;
            c->setPos(HORIZONTAL_PADDING + x, VERTICAL_PADDING + y);
            c->setRealZValue(i);
            longestRow = qMax(row, longestRow);
        }

        // +1 because the row/col variables used in the calculations are 0-indexed but
        // GridSize expects the actual row/col count
        return GridSize{longestRow + 1, qMax(col + 1, 3)};

    } else {
        int cols = qBound(1, qFloor(qSqrt((double)cardCount / 2)), 7);
        int rows = qMax(qCeil((double)cardCount / cols), 1);
        if (minRows == 0) {
            minRows = rows;
        } else if (rows < minRows) {
            rows = minRows;
            cols = qCeil((double)cardCount / minRows);
        }

        if (cols < 2)
            cols = 2;

        qDebug() << "reorganizeCards: rows=" << rows << "cols=" << cols;

        for (int i = 0; i < cardCount; i++) {
            CardItem *c = cards.at(i);
            qreal x = (i / rows) * CARD_WIDTH;
            qreal y = (i % rows) * CARD_HEIGHT / 3;
            c->setPos(HORIZONTAL_PADDING + x, VERTICAL_PADDING + y);
            c->setRealZValue(i);
        }

        return GridSize{rows, qMax(cols, 1)};
    }
}

void ZoneViewZone::setGroupBy(CardList::SortOption _groupBy)
{
    groupBy = _groupBy;
    reorganizeCards();
}

void ZoneViewZone::setSortBy(CardList::SortOption _sortBy)
{
    sortBy = _sortBy;
    reorganizeCards();
}

void ZoneViewZone::setPileView(int _pileView)
{
    pileView = _pileView;
    reorganizeCards();
}

void ZoneViewZone::addCardImpl(CardItem *card, int x, int /*y*/)
{
    // if x is negative set it to add at end
    if (x < 0 || x >= cards.size()) {
        x = cards.size();
    }

    if (isReversed) {
        qDebug() << "TRACK" << "INSERTING CARD AT POSITION" << cards.size();
        cards.append(card);
        // We're adding the newest card to the ass of the array
        // Shouldn't this card have some kind of information for us?
    } else {
        // TODO: figure out if we always just prepend anyways
        cards.insert(x, card);
        qDebug() << "TRACK" << "HOW DID WE END UP HERE??? # MINECRAFT";
    }

    card->setParentItem(this);
    card->update();
    reorganizeCards();
}

void ZoneViewZone::handleDropEvent(const QList<CardDragItem *> &dragItems,
                                   CardZone *startZone,
                                   const QPoint & /*dropPoint*/)
{
    Command_MoveCard cmd;
    cmd.set_start_player_id(startZone->getPlayer()->getId());
    cmd.set_start_zone(startZone->getName().toStdString());
    cmd.set_target_player_id(player->getId());
    cmd.set_target_zone(getName().toStdString());
    cmd.set_x(0);
    cmd.set_y(0);
    cmd.set_is_reversed(isReversed);

    for (int i = 0; i < dragItems.size(); ++i)
        cmd.mutable_cards_to_move()->add_card()->set_card_id(dragItems[i]->getId());

    player->sendGameCommand(cmd);
}

void ZoneViewZone::removeCard(int position)
{
    if (cards.isEmpty()) {
        return;
    }

    if (isReversed) {
        // TODO: comment
        position -= cards.first()->getId();
        if (position < 0 || position >= cards.size()) {
            reorganizeCards();
            return;
        }
    }

    if (position < 0 || position >= cards.size()) {
        return;
    }

    qDebug() << "TRACK" << "TAKING CARD FROM POSITION" << position;
    CardItem *card = cards.takeAt(position);
    card->deleteLater();
    reorganizeCards();
}

void ZoneViewZone::setGeometry(const QRectF &rect)
{
    prepareGeometryChange();
    setPos(rect.x(), rect.y());
    bRect = QRectF(0, 0, rect.width(), rect.height());
}

QSizeF ZoneViewZone::sizeHint(Qt::SizeHint /*which*/, const QSizeF & /*constraint*/) const
{
    return optimumRect.size();
}

void ZoneViewZone::setWriteableRevealZone(bool _writeableRevealZone)
{
    if (writeableRevealZone && !_writeableRevealZone) {
        origZone->getViews().append(this);
    } else if (!writeableRevealZone && _writeableRevealZone) {
        origZone->getViews().removeOne(this);
    }
    writeableRevealZone = _writeableRevealZone;
}

void ZoneViewZone::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    emit wheelEventReceived(event);
}
