#pragma once
#include <string>
#include <iostream>
#include <experimental/filesystem>

using namespace std;

namespace Utils {
	template<typename T> static void Println(T printable) {
		cout << printable << endl;
	}

	template<typename T> static std::string FileFromSourceDir(T fileName) {
		std::string stringName(fileName);
		std::string path = static_cast<std::string>(std::experimental::filesystem::current_path());
		path.append("/source");
		path.append(stringName);
		return path;
	}
};