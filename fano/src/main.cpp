#include <algorithm>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <functional>
#include <fstream>
#include <map>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include <nvx/BitField.hpp>
#include <nvx/serialization.hpp>

constexpr unsigned int const CONTROL_NUMBER = 3247928473u;


using namespace std;





/****************** BIT FIELD SERIALIZATION *****************/
namespace nvx
{
	template<typename Ostream, typename Meta>
	int serialize(
		nvx::archive<Ostream, Meta> &arch, 
		nvx::BitField const *obj,
		bool write = true
	)
	{
		return
			serialize(arch, nvx::R<int>(obj->bitsize()), write) +
			serialize(arch, (std::vector<uint8_t> *)obj, write);
	}

	template<typename Ostream, typename Meta>
	int deserialize(
		nvx::archive<Ostream, Meta> &arch, 
		nvx::BitField *obj
	)
	{
		int bitsize;
		int res =
			deserialize(arch, &bitsize) +
			deserialize(arch, (std::vector<uint8_t> *)obj);
		obj->bitresize(bitsize);
		return res;
	}
}





/************************ ITEM STRUCT ***********************/
/*
 * Структура, представляющая один символ, с
 * которым сопоставлено число встреч этого
 * символа во всём тексте и код Фано
 */
struct item_t
{
	string  key; // код символа по алгоритму Фано
	wchar_t ch;  // символ
	int     c;   // количество этих символов во всём тексте

	NVX_SERIALIZABLE(&key, &ch, &c);
};

bool char_cmp(item_t const &lhs, item_t const &rhs)
{
	return lhs.ch < rhs.ch;
}

bool key_cmp(item_t const &lhs, item_t const &rhs)
{
	return lhs.key < rhs.key;
}

bool count_cmp(item_t const &lhs, item_t const &rhs)
{
	return lhs.c < rhs.c;
}





/******************** ASSISTIVE FUNCTIONS *******************/
/*
 * Считать все данные из stdin в строку, одновременно
 * подсчитывая количество встречающихся символов
 */
void read_text(wstring &data, vector<item_t> &items)
{
	unordered_map<wchar_t, int> syms;

	data.clear();
	wchar_t ch;
	while ( wcin.get(ch) )
	{
		data.push_back(ch);
		++syms[ch];
	}

	items.clear();
	for (auto b = syms.begin(), e = syms.end(); b != e; ++b)
		items.push_back( { "", b->first, b->second } );
	items.push_back( { "", L'\0', 1 } );

	return;
}

/*
 * Расчитывает код по Фано для каждого символа в тексте
 */
void calculate_keys(vector<item_t> &items)
{
	if (items.size() < 2)
		return;

	/*
	 * Отсортировываем по убыванию частоты символа
	 */
	sort(items.rbegin(), items.rend(), count_cmp);

	items[0].key = "0";
	items[1].key = "1";

	int c, minval, tmpval;
	for (auto b = items.begin()+2, e = items.end(); b != e; ++b)
	{
		c = b->c;
		auto min = items.begin();
		minval = items.front().c + items.front().key.size() * c;

		for (auto bb = items.begin(); bb != b; ++bb)
		{
			tmpval = bb->c + bb->key.size() * c;
			if (tmpval < minval)
			{
				minval = tmpval;
				min = bb;
			}
		}

		b->key = min->key + "1";
		min->key.push_back('0');
	}

	return;
}

/*
 * Кодирует данные из строки согласно кодам Фано в битовое поле
 */
void data2bitfield(wstring const &data, vector<item_t> &items, nvx::BitField &bits)
{
	sort( items.begin(), items.end(), char_cmp );

	for (auto ch = data.begin(), e = data.end(); ch != e+1; ++ch)
	{
		auto it = lower_bound(
			items.begin(), items.end(),
			item_t { "", (ch == e ? '\0' : *ch), 0 }, char_cmp
		);

		if (it == items.end() or it->ch != *ch)
		{
			fwprintf(stderr, L"Error: unknown symbol '%lc' in data2bitfield\n", ch);
			throw "Unknown symbol";
		}

		for (auto bb = it->key.begin(), ee = it->key.end(); bb != ee; ++bb)
			bits.pushbit( *bb == '1' ? 1 : 0 );
	}

	return;
}





/********************* ENCODE AND DECODE ********************/
/*
 * Считывает данные из stdin, на их основе расчитывает
 * ключи и записывает закодированные данные в stdout
 */
void encode()
{
	wstring        data;
	vector<item_t> items;
	nvx::BitField  bits;

	read_text(data, items);
	calculate_keys(items);
	data2bitfield(data, items, bits);

	nvx::archive(&cout) <<
		nvx::R<unsigned int>(CONTROL_NUMBER) <<
		&items << &bits;

	return;
}

/*
 * Считывает закодированные данные из stdin, декодирует
 * и записывает получившийся результат в stdout
 */
void decode()
{
	vector<item_t> items;
	nvx::BitField  bits;

	{
		unsigned int contnum;
		nvx::archive arch(&cin);
		arch >> &contnum;

		if (contnum != CONTROL_NUMBER)
			throw "Decode error: fail check control number";

		arch >> &items >> &bits;
	}

	sort( items.begin(), items.end(), key_cmp );

	int bitn = 0;
	string key;
	vector<item_t>::const_iterator it;
	while (bitn < bits.bitsize())
	{
		key.clear();
		do
		{
			if (bitn >= bits.bitsize())
				throw "Decode error";

			key.push_back( bits[bitn] ? '1' : '0' );
			++bitn;
			it = lower_bound(
				items.begin(), items.end(),
				item_t { key, L'\0', 0 }, key_cmp
			);
		}
		while ( it->key != key );

		if (it->ch == '\0')
			break;
		wcout.put(it->ch);
	}

	return;
}





// main
int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");

	try
	{
		if ( argc > 1 && !strcmp(argv[1], "-e") )
		{
			encode();
			return 0;
		}

		if ( argc > 1 && !strcmp(argv[1], "-d") )
		{
			decode();
			return 0;
		}

		if ( argc > 1 && !strcmp(argv[1], "-k") )
		{
			vector<item_t> items;
			nvx::archive(&cin) >> &items;
			// print items
		}
	}
	catch (char const *err)
	{
		fprintf(stderr, "%s\n", err);
		return 1;
	}

	wcout << "Type -e to encode file, -d to decode, -k to print keys" << endl;

	return 0;
}





// END