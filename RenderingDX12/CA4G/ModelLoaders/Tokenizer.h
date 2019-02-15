#pragma once
#include <stdio.h>
#include <math.h>
#include <cmath>
#include <Windows.h>

using namespace std;

class Tokenizer
{
	char* buffer;
	int count;
	int pos = 0;
public:
	Tokenizer(FILE* stream) {
		fseek(stream, 0, SEEK_END);
		fpos_t count;
		fgetpos(stream, &count);
		fseek(stream, 0, SEEK_SET);
		buffer = new char[count];
		int offset = 0;
		int read;
		do
		{
			read = fread_s(&buffer[offset], count, 1, min(count, 1024 * 1024 * 20), stream);
			count -= read;
			offset += read;
		} while (read > 0);
		this->count = offset;
	}
	~Tokenizer() {
		delete[] buffer;
	}
	inline bool isEof() {
		return pos == count;
	}

	inline bool isEol()
	{
		return isEof() || peek() == 10;
	}

	void skipCurrentLine() {
		while (!isEol()) pos++;
		if (!isEof()) pos++;
	}

	inline char peek() {
		return buffer[pos];
	}

	bool match(const char* token)
	{
		while (!isEof() && (buffer[pos] == ' ' || buffer[pos] == '\t'))
			pos++;
		int initialPos = pos;
		int p = 0;
		while (!isEof() && token[p] == buffer[pos]) {
			p++; pos++;
		}
		if (token[p] == '\0')
			return true;
		pos = initialPos;
		return false;
	}
	bool matchDigit(int &d) {
		char ch = peek();

		if (ch >= '0' && ch <= '9')
		{
			d = ch - '0';
			pos++;
			return true;
		}
		return false;
	}
	bool matchSymbol(char c)
	{
		if (!isEof() && buffer[pos] == c)
		{
			pos++;
			return true;
		}
		return false;
	}

	string readTextToken() {

		int start = pos;
		while (!isEol() && buffer[pos] != ' ' && buffer[pos] != '/' && buffer[pos] != ';'&& buffer[pos] != ':'&& buffer[pos] != '.'&& buffer[pos] != ','&& buffer[pos] != '('&& buffer[pos] != ')')
			pos++;
		int end = pos - 1;
		return string((char*)(buffer + start), end - start + 1);
	}

	string readToEndOfLine()
	{
		int start = pos;
		while (!isEol())
			pos++;
		int end = pos - 1;
		if (!isEof()) pos++;
		return string((char*)(buffer + start), end - start + 1);
	}

	inline bool endsInteger(char c)
	{
		return (c < '0') || (c > '9');
	}

	void ignoreWhiteSpaces() {
		while (!isEol() && (buffer[pos] == ' ' || buffer[pos] == '\t'))
			pos++;
	}

	bool readIntegerToken(int &i) {
		i = 0;
		if (isEol())
			return false;
		int initialPos = pos;
		ignoreWhiteSpaces();
		int sign = 1;
		if (buffer[pos] == '-')
		{
			sign = -1;
			pos++;
		}
		ignoreWhiteSpaces();
		int end = pos;
		while (pos < count && !endsInteger(buffer[pos])) {
			i = i * 10 + (buffer[pos] - '0');
			pos++;
		}
		i *= sign;

		if (pos > end)
			return true;
		pos = initialPos;
		return false;
	}
	bool readFloatToken(float &f) {
		int initialPos = pos;
		int sign = 1;
		ignoreWhiteSpaces();
		if (buffer[pos] == '-')
		{
			sign = -1;
			pos++;
		}
		int intPart = 0;
		float scale = 1;
		if ((pos < count && buffer[pos] == '.') || readIntegerToken(intPart))
		{
			f = intPart;
			if (pos < count && buffer[pos] == '.')
			{
				int fracPos = pos;
				int fracPart;
				pos++;
				if (!readIntegerToken(fracPart))
				{
					pos = initialPos;
					return false;
				}
				else
				{
					int expPart = 0;
					if (buffer[pos] == 'e') {
						pos++;
						readIntegerToken(expPart);
						scale = pow(10, expPart);
					}
				}
				float fracPartf = fracPart / pow(10, pos - fracPos - 1);
				f += fracPartf;
			}
			f *= sign * scale;
			return true;
		}
		pos = initialPos;
		return false;
	}
};