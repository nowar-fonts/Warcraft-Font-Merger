#include <format>
#include <fstream>
#include <iostream>

#include <otfcc/sfnt.hpp>

using namespace std;

int main(int argc, char **argv) {
	if (argc != 2) {
		cerr << format("Usage: {} <font>", argv[0]) << endl;
		return 1;
	}

	std::ifstream fontFile(argv[1], std::ios::binary);
	if (!fontFile) {
		cerr << "Cannot open font file." << endl;
		return 1;
	}

	std::string font{std::istreambuf_iterator<char>(fontFile), std::istreambuf_iterator<char>()};

	otfcc::sfnt::TableDirectory td = otfcc::sfnt::readSfnt(font);

	cout << "Table directory:" << endl;
	for (auto &record : td.tableRecords) {
		cout << format("  {}: {}", std::string(record.first), record.second.length()) << endl;
	}
}
