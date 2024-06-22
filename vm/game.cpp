#include "game.h"

vector<Card> Game::get_cards_of_type(string type)
{
	vector<Card> matching_cards;

	for (auto card_entry : cards)
	{
		Card card = card_entry.second;
		if (card.parentName == type)
		{
			matching_cards.push_back(card);
		}
	}

	return matching_cards;
}
