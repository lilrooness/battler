#include <algorithm>
#include "game.h"

#include "../Compiler.h"

namespace Battler {

    Stack::Stack() {
        Attr ownerIDAttr = Attr();
        ownerIDAttr.type = AttributeType::PLAYER_REF;
        ownerIDAttr.playerRef = -1;
        this->attributes.Store("ownerID", ownerIDAttr);
    }

    Card Game::GenerateCard(string name) {
        Card c = cards[name];
        c.UUID = m_currentCardUUID;

        m_currentCardUUID += 1;

        return c;
    }

    vector<Card> Game::get_cards_of_type(string type) {
        vector<Card> matching_cards;

        for (auto card_entry: cards) {
            Card card = card_entry.second;
            if (card.parentName == type) {
                matching_cards.push_back(card);
            }
        }

        return matching_cards;
    }

    std::string Attr::ToString() {
        std::stringstream ss;

        if (type == AttributeType::BOOL) {
            ss << b;
        } else if (type == AttributeType::INT) {
            ss << i;
        } else if (type == AttributeType::FLOAT) {
            ss << f;
        } else if (type == AttributeType::STRING) {
            ss << s;
        }
        if (type == AttributeType::CARD_REF) {
            ss << cardRef;
        } else if (type == AttributeType::PLAYER_REF) {
            ss << playerRef;
        } else if (type == AttributeType::STACK_REF) {
            ss << stackRef;
        } else if (type == AttributeType::STACK_POSITION_REF) {
            ss << std::get<0>(stackPositionRef) << " " << std::get<1>(stackPositionRef);
        }

        return ss.str();
    }

    bool AttrCont::Contains(std::string name) {
        if (attrs.find(name) == attrs.end()) {
            return false;
        }
        return true;
    }

    void AttrCont::Store(std::string name, Attr a) {
        attrs[name] = a;
    }

    Attr &AttrCont::Get(std::string name) {
        return attrs[name];
    }

    std::string AttrCont::ToString(std::string prefix /* = "" */) {
        std::stringstream ss;
        for (auto pair: attrs) {
            ss << endl << prefix << pair.first << ": " << pair.second.ToString();
        }

        return ss.str();
    }

    void Game::Print() {
        cout << "Cards:" << endl;

        for (auto pair: cards) {
            cout << pair.first << ":" << pair.second.attributes.ToString("    ") << endl;
        }

        cout << "Stacks:" << endl;

        for (auto pair: stacks) {
            cout << pair.first << ":" << pair.second.attributes.ToString("    ") << endl;
        }

        cout << "Players:" << endl;

        for (int i = 0; i < players.size(); i++) {
            cout << i << ":" << players[i].attributes.ToString("    ") << endl;
        }

        cout << "Game Attributes:" << endl;

        cout << attributeCont.ToString("    ") << endl;
    }

    bool Stack::MatchesSequence(std::vector<CardMatcher> sequence, bool searchBottomUp/*=false*/) {

        if (cards.empty() && sequence.empty())
        {
            return true;
        }

        // In the sequence [A B C D], A compares to the top of the stack, and D the bottom.
        // stacks are arranged so the top of the stack is the end of the vector, so we must reverse the
        // stack before we compare. a searchBottomUp == true here means that we want to compare [D C B A] to the stack
        // (D being compared to the top card, and A being compared to the bottom card), so we would not reverse the stack
        // in that case
        std::vector<Card> cardsToTestAgainst = cards;
        if (!searchBottomUp) {
            std::reverse(cardsToTestAgainst.begin(), cardsToTestAgainst.end());
        }

        auto currentCard = cardsToTestAgainst.begin();
        auto cardsEnd = cardsToTestAgainst.end();

        auto currentMatcher = sequence.begin();
        auto matchersEnd = sequence.end();

        while (currentMatcher != matchersEnd)
        {
            if (currentMatcher->type == CardMatcherType::ANY)
            {
                if (currentCard == cardsEnd)
                {
                    return false;
                }
            }
            else if (currentMatcher->type == CardMatcherType::ID)
            {
                if (currentCard == cardsEnd)
                {
                    return false;
                }
                if (currentCard->ID != currentMatcher->id)
                {
                    return false;
                }
            }
            else if (currentMatcher->type == CardMatcherType::REST)
            {

                // ending on a match rest, means by definition we are matching whatever is left in the stack
                if (currentMatcher+1 == matchersEnd)
                {
                    return true;
                }

                currentMatcher++;
                bool foundMatchingCardForNextMatcher = false;
                while (!foundMatchingCardForNextMatcher && currentCard != cardsEnd)
                {
                    if (currentMatcher->type == CardMatcherType::REST)
                    {
                        throw VMError("Cannot match sequence. Cannot have two consecutive REST matchers");
                    }

                    if (currentMatcher->type == CardMatcherType::ANY || currentMatcher->id == currentCard->ID)
                    {
                        foundMatchingCardForNextMatcher = true;
                    }
                    else
                    {
                        currentCard++;
                    }
                }

                if (!foundMatchingCardForNextMatcher)
                {
                    return false;
                }
            }
            else
            {
                throw VMError("Cannot match sequence. Encountered Unsupported card matcher type");
            }

            currentMatcher++;
            currentCard++;
        }

        return true;
    }
}
