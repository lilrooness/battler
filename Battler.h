#pragma once

enum class CardAttributeType {CAT_INT, CAT_STRING, CAT_BOOL};

class Attribute {

	public:
		std::string name;
		
		int _int;
		bool _bool;
		std::string _string;

		CardAttributeType t;
		Attribute(std::string name, CardAttributeType t): name{name}, t{t} {}  
};

class CardClass {
	public:
		std::string name;
		int parentIndex;
		std::vector<Attribute> attributes;
		CardClass(std::string name, std::vector<Attribute> attributes): name{name}, attributes{attributes} {}
};

class Card {
	public:
		CardClass cardClass;
};

class CardStack {
	public:
		std::vector<Card> cards;
		std::string name;
		CardStack(std::string name): name{name} {}
};

class BattlerGame {
	public:
		std::string name;
		std::vector<CardClass> cardClasses;
		std::vector<CardStack> stacks;
};
