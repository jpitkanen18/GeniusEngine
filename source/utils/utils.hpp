#pragma once
#include <string>
#include <iostream>
#include <experimental/filesystem>

namespace Utils {
	using std::string, std::experimental::filesystem::current_path, std::cout;

	template<typename T> static void Println(T printable) {
		cout << printable << '\n';
	}

	template<typename T> static string FileFromSourceDir(T fileName) {
		string stringName(fileName);
		string path = static_cast<string>(current_path());
		path.append("/source");
		path.append(stringName);
		return path;
	}
};
